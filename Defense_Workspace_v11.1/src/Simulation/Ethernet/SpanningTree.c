/************************************************************************************
* Copyright (C) 2017                                                               *
* TETCOS, Bangalore. India                                                         *
*                                                                                  *
* Tetcos owns the intellectual property rights in the Product and its content.     *
* The copying, redistribution, reselling or publication of any or all of the       *
* Product or its content without express prior written consent of Tetcos is        *
* prohibited. Ownership and / or any other right relating to the software and all  *
* intellectual property rights therein shall remain at all times with Tetcos.      *
*                                                                                  *
* Author:    Shashi Kant Suman                                                     *
*                                                                                  *
* ---------------------------------------------------------------------------------*/

#include "main.h"
#include "Ethernet.h"
#include "Ethernet_enum.h"

static void send_config_bpdu(NETSIM_ID d, UINT lanId, double time);

static char* eth_get_lowest_mac(ptrETH_LAN lan)
{
	UINT i;
	PNETSIM_MACADDRESS ret = NULL;
	for (i = 0; i < lan->interfaceCount; i++)
	{
		ptrETH_IF in = lan->ethIfVar[i];
		if (!ret)
			ret = in->macAddress;
		else if (in->macAddress->nmacaddress < ret->nmacaddress)
				ret = in->macAddress;
	}
	return ret->szmacaddress;
}

static void configure_switch_id(ptrETH_LAN lan)
{
	char id[20];
	strcpy(id, eth_get_lowest_mac(lan));
	lan->spt->switchId = calloc(20, sizeof(char));
	sprintf(lan->spt->switchId, "%d%s", lan->spt->priority, id);
}

static void eth_spt_init_port(NETSIM_ID d, UINT lanId)
{
	ptrETH_VAR eth = GET_ETH_VAR(d);
	ptrETH_LAN lan = eth->lanVar[lanId];

	UINT i;
	for (i = 0; i < lan->interfaceCount; i++)
	{
		NETSIM_ID c, ci;
		NETSIM_ID ifi = lan->interfaceId[i];
		NETSIM_ID l = fn_NetSim_Stack_GetConnectedDevice(d, ifi, &c, &ci);
		if (!l)
			lan->ethIfVar[i]->portState = PORTSTATE_BLOCKING; // No link to port

		ptrETH_VAR e = GET_ETH_VAR(c);
		if (!e || !e->isSPT)
		{
			print_ethernet_log("Port %d, Setting status as Forwarding", ifi);
			lan->ethIfVar[i]->portState = PORTSTATE_FORWARDING;
		}
		else
		{
			ptrETH_LAN la = GET_ETH_LAN(c, ci, 0);
			if (!la->STPStatus)
			{
				print_ethernet_log("Port %d, Setting status as Forwarding", ifi);
				lan->ethIfVar[i]->portState = PORTSTATE_FORWARDING;
			}
			else
			{
				lan->ethIfVar[i]->portState = PORTSTATE_FORWARDING;
				lan->ethIfVar[i]->portType = PORTTYPE_DESIGNATED;
			}
		}
	}
}

static void start_spt_for_lan(NETSIM_ID d, UINT lanId)
{
	ptrETH_VAR eth = GET_ETH_VAR(d);
	ptrETH_LAN lan = eth->lanVar[lanId];

	print_ethernet_log("Stating SPT for LAN %d", lanId);
	eth_spt_init_port(d, lanId);

	if (!lan->spt->switchId)
		configure_switch_id(lan);

	print_ethernet_log("Starting as root");
	lan->spt->rootId = _strdup(lan->spt->switchId); //Start as root
	print_ethernet_log("Root Id = %s", lan->spt->rootId);

	print_ethernet_log("Sending config BPDU");
	send_config_bpdu(d, lanId, 0);
}
 
static ptrETH_BPDU create_bpdu(ptrETH_LAN lan)
{
	ptrETH_BPDU bpdu = calloc(1, sizeof* bpdu);
	bpdu->rootId = lan->spt->rootId;
	bpdu->rootPathCost = lan->spt->stpCost;
	bpdu->bridgeId = lan->spt->switchId;
	bpdu->maxAge = 20;
	bpdu->msgAge = 1;
	bpdu->helloTime = 2;
	bpdu->forwardDelay = 15;

	print_ethernet_log("Root Id = %s, Root path cost = %d, Bridge Id = %s",
					   bpdu->rootId,
					   bpdu->rootPathCost,
					   bpdu->bridgeId);
	return bpdu;
}

static ptrETH_BPDU copy_bpdu(ptrETH_LAN lan, ptrETH_BPDU b)
{
	ptrETH_BPDU bpdu = calloc(1, sizeof* bpdu);
	bpdu->rootId = b->rootId;
	bpdu->rootPathCost = lan->spt->stpCost + b->rootPathCost;
	bpdu->bridgeId = lan->spt->switchId;
	bpdu->maxAge = 20;
	bpdu->msgAge = 1;
	bpdu->helloTime = 2;
	bpdu->forwardDelay = 15;

	print_ethernet_log("Root Id = %s, Root path cost = %d, Bridge Id = %s",
					   bpdu->rootId,
					   bpdu->rootPathCost,
					   bpdu->bridgeId);
	return bpdu;
}

static NetSim_PACKET* create_bpdu_packet(NETSIM_ID d, ptrETH_LAN lan, double time)
{
	NetSim_PACKET* packet = fn_NetSim_Packet_CreatePacket(MAC_LAYER);
	packet->nControlDataType = ETH_CONFIGBPDU;
	packet->nPacketType = PacketType_Control;
	packet->nSourceId = d;
	packet->nTransmitterId = d;
	packet->pstruMacData->dArrivalTime = time;
	packet->pstruMacData->dStartTime = time;
	packet->pstruMacData->dEndTime = time;
	packet->pstruMacData->dOverhead = ETH_BPDU_LEN;
	packet->pstruMacData->dPacketSize = ETH_BPDU_LEN;
	packet->pstruMacData->nMACProtocol = MAC_PROTOCOL_IEEE802_3;
	//packet->pstruMacData->Packet_MACProtocol = create_bpdu(lan);
	set_eth_hdr(packet, ETH_HDR_CBPDU, create_bpdu(lan), NULL, NULL);
	packet->pstruMacData->szDestMac = multicastSPTMAC;
	strcpy(packet->szPacketType, "CONFIG_BPDU");
	return packet;
}

static NetSim_PACKET* copy_bpdu_packet(NETSIM_ID d, ptrETH_LAN lan, double time, ptrETH_BPDU bpdu)
{
	NetSim_PACKET* packet = fn_NetSim_Packet_CreatePacket(MAC_LAYER);
	packet->nControlDataType = ETH_CONFIGBPDU;
	packet->nPacketType = PacketType_Control;
	packet->nSourceId = d;
	packet->nTransmitterId = d;
	packet->pstruMacData->dArrivalTime = time;
	packet->pstruMacData->dStartTime = time;
	packet->pstruMacData->dEndTime = time;
	packet->pstruMacData->dOverhead = ETH_BPDU_LEN;
	packet->pstruMacData->dPacketSize = ETH_BPDU_LEN;
	packet->pstruMacData->nMACProtocol = MAC_PROTOCOL_IEEE802_3;
	//packet->pstruMacData->Packet_MACProtocol = copy_bpdu(lan, bpdu);
	set_eth_hdr(packet, ETH_HDR_CBPDU, copy_bpdu(lan, bpdu), NULL, NULL);
	packet->pstruMacData->szDestMac = multicastSPTMAC;
	strcpy(packet->szPacketType, "CONFIG_BPDU");
	return packet;
}

static void send_config_bpdu(NETSIM_ID d, UINT lanId, double time)
{
	bool flag = false;
	ptrETH_VAR eth = GET_ETH_VAR(d);
	ptrETH_LAN lan = eth->lanVar[lanId];
	
	NetSim_PACKET* packet = create_bpdu_packet(d, lan, time);

	UINT i;
	for (i = 0; i < lan->interfaceCount; i++)
	{
		NETSIM_ID c, ci;
		NETSIM_ID in = lan->interfaceId[i];
		NETSIM_ID l = fn_NetSim_Stack_GetConnectedDevice(d, in, &c, &ci);
		if (l)
		{
			ptrETH_VAR ce = GET_ETH_VAR(c);
			if (ce)
			{
				ptrETH_LAN cl = GET_ETH_LAN(c, ci, 0);
				if (cl && cl->STPStatus)
				{
					add_dest_to_packet(packet, c);
					flag = true;
				}

			}
		}
	}
	if (flag)
	{
		NetSim_EVENTDETAILS pevent;
		memset(&pevent, 0, sizeof pevent);
		pevent.dEventTime = time;
		pevent.dPacketSize = packet->pstruMacData->dPacketSize;
		pevent.nDeviceId = d;
		pevent.nDeviceType = DEVICE_TYPE(d);
		pevent.nEventType = MAC_OUT_EVENT;
		pevent.nProtocolId = MAC_PROTOCOL_IEEE802_3;
		pevent.pPacket = packet;
		pevent.nSubEventType = SEND_CONFIG_BPDU;
		fnpAddEvent(&pevent);
	}
	else
	{
		fn_NetSim_Packet_FreePacket(packet);
	}
}

static void start_spanning_tree_protocol(NETSIM_ID d)
{
	ptrETH_VAR eth = GET_ETH_VAR(d);
	if (!eth->isSPT)
		return; //SPT is not configured

	UINT i;
	
	for (i = 0; i < eth->lanCount; i++)
	{
		if (!eth->lanVar[i]->STPStatus)
			continue; //SPT is disable for this LAN
		print_ethernet_log("Starting SPT protocol for device %d, LAN %d", d, i + 1);
		start_spt_for_lan(d, i);
		print_ethernet_log("");
	}
}

bool isSingleInterface(NETSIM_ID d, ptrETH_LAN lan)
{
	if (lan->interfaceCount == 1)
		return true;
	UINT i;
	UINT k = 0;
	for (i = 0; i< lan->interfaceCount; i++)
	{
		NETSIM_ID in = lan->interfaceId[i];
		NETSIM_ID c, ci;
		NETSIM_ID l = fn_NetSim_Stack_GetConnectedDevice(d, in, &c, &ci);
		if (l)
		{
			ptrETH_VAR eth = GET_ETH_VAR(c);
			if (eth->isSPT)
				k++;
			if (k > 1)
				return false;
		}
	}
	return true;
}

void init_spanning_tree_protocol()
{
	NETSIM_ID i;
	for (i = 0; i < NETWORK->nDeviceCount; i++)
	{
		ptrETH_VAR eth = GET_ETH_VAR(i + 1);
		if (!eth)
			continue;
		UINT j;
		for (j = 0; j < eth->lanCount; j++)
		{
			if (!eth->isSPT || (isSingleInterface(i+1, eth->lanVar[j]) &&
				eth->lanVar[j]->lanIP))
			{
				eth->lanVar[j]->STPStatus = false;
				eth->lanVar[j]->ethIfVar[0]->portState = PORTSTATE_FORWARDING;
				print_ethernet_log("Device %d, Port %d: Setting port state as Forwarding\n",
								   i + 1, eth->lanVar[j]->ethIfVar[0]->interfaceId);
			}
			else
			{
				eth->lanVar[j]->STPStatus = true;
			}
		}
	}
	for (i = 0; i < NETWORK->nDeviceCount; i++)
	{
		ptrETH_VAR eth = GET_ETH_VAR(i + 1);
		if (!eth)
			continue;
		if (eth->isSPT)
			start_spanning_tree_protocol(i + 1);
	}
}

int multicast_config_bpdu()
{
	NETSIM_ID d = pstruEventDetails->nDeviceId;

	NetSim_PACKET* packet = pstruEventDetails->pPacket;
	UINT c;
	NETSIM_ID* dest = get_dest_from_packet(packet, &c);
	UINT i;

	for (i = 0; i < c; i++)
	{
		NETSIM_ID des = dest[i];
		NetSim_PACKET* p;
		if (i)
			p = fn_NetSim_Packet_CopyPacket(packet);
		else
			p = packet;

		NETSIM_ID in = get_interface_id(d, des);

		NetSim_EVENTDETAILS pevent;
		memcpy(&pevent, pstruEventDetails, sizeof pevent);
		pevent.nEventType = PHYSICAL_OUT_EVENT;
		pevent.nInterfaceId = in;
		pevent.nSubEventType = 0;
		pevent.pPacket = p;
		fnpAddEvent(&pevent);
	}
	return 0;
}

typedef enum
{
	BLOCK,
	FORWARD,
	DONT_FORWARD,
}COMPARE_BPDU;
static COMPARE_BPDU compare_bpdu(ptrETH_BPDU bpdu, ptrETH_SPT spt, NETSIM_ID port)
{
	int cid = _stricmp(bpdu->rootId, spt->rootId);

	if (cid == 0)
	{
		if (!_stricmp(spt->rootId, spt->switchId))
		{
			return DONT_FORWARD; // I am root
		}
		else
		{
			if (bpdu->rootPathCost == spt->rootPathCost)
			{
				if (spt->rootPort == port)
					return DONT_FORWARD;
				else
					return BLOCK;
			}
			else if (bpdu->rootPathCost < spt->rootPathCost)
			{
				return FORWARD;
			}
			else
			{
				return BLOCK;
			}
		}
	}
	else if (cid < 0)
	{
		return FORWARD;
	}
	else
	{
		return DONT_FORWARD;
	}
}

static void block_port(NETSIM_ID d, ptrETH_LAN lan, NETSIM_ID in)
{
	print_ethernet_log("Setting switch %d, port %d state as BLOCKED", d, in);
	ptrETH_IF inter = GET_ETH_IF(lan, in);
	inter->portState = PORTSTATE_BLOCKING;
	inter->portType = PORTTYPE_NONDESIGNATED;
}

static void forward_port(NETSIM_ID d, ptrETH_LAN lan, NETSIM_ID in)
{
	print_ethernet_log("Setting switch %d, port %d state as FORWARDING", d, in);
	ptrETH_IF inter = GET_ETH_IF(lan, in);
	inter->portState = PORTSTATE_FORWARDING;
}

static void update_spt(ptrETH_SPT spt, ptrETH_BPDU bpdu, NETSIM_ID port)
{
	print_ethernet_log("Updating SPT data");
	spt->rootId = bpdu->rootId;
	spt->rootPathCost = bpdu->rootPathCost;
	spt->rootPort = port;
	print_ethernet_log("Root id = %s, Cost = %d, Root port = %d", spt->rootId, spt->rootPathCost, port);
}

static void mark_port(ptrETH_LAN lan, NETSIM_ID port)
{
	UINT i;
	lan->spt->rootPort = port;
	for (i = 0; i < lan->interfaceCount; i++)
	{
		ptrETH_IF inter = lan->ethIfVar[i];
		if (inter->interfaceId == port)
		{
			inter->portType = PORTTYPE_ROOT;
		}
		else
		{
			inter->portType = PORTTYPE_DESIGNATED;
		}
	}
}

static void mark_port_as_nondesignated(ptrETH_LAN lan, NETSIM_ID port)
{
	UINT i;
	for (i = 0; i < lan->interfaceCount; i++)
	{
		ptrETH_IF inter = lan->ethIfVar[i];
		if (inter->interfaceId == port)
		{
			inter->portType = PORTTYPE_NONDESIGNATED;
			break;
		}
	}
}

static void forward_bpdu(NETSIM_ID d, NETSIM_ID inter, double time, ptrETH_LAN lan, ptrETH_BPDU configBPDU)
{
	bool flag = false;

	print_ethernet_log("Forwarding config BPDU");
	NetSim_PACKET* packet = copy_bpdu_packet(d, lan, time, configBPDU);

	UINT i;
	for (i = 0; i < lan->interfaceCount; i++)
	{
		NETSIM_ID c, ci;
		NETSIM_ID in = lan->interfaceId[i];
		if (in == inter)
			continue;

		NETSIM_ID l = fn_NetSim_Stack_GetConnectedDevice(d, in, &c, &ci);
		if (l)
		{
			ptrETH_VAR ce = GET_ETH_VAR(c);
			if (ce)
			{
				ptrETH_LAN cl = GET_ETH_LAN(c, ci, 0);
				if (cl && cl->STPStatus)
				{
					add_dest_to_packet(packet, c);
					flag = true;
				}

			}
		}
	}
	if (flag)
	{
		NetSim_EVENTDETAILS pevent;
		memset(&pevent, 0, sizeof pevent);
		pevent.dEventTime = time;
		pevent.dPacketSize = packet->pstruMacData->dPacketSize;
		pevent.nDeviceId = d;
		pevent.nDeviceType = DEVICE_TYPE(d);
		pevent.nEventType = MAC_OUT_EVENT;
		pevent.nProtocolId = MAC_PROTOCOL_IEEE802_3;
		pevent.pPacket = packet;
		pevent.nSubEventType = SEND_CONFIG_BPDU;
		fnpAddEvent(&pevent);
	}
	else
	{
		fn_NetSim_Packet_FreePacket(packet);
	}
}

void process_configbpdu()
{
	NETSIM_ID d = pstruEventDetails->nDeviceId;
	NETSIM_ID in = pstruEventDetails->nInterfaceId;
	double time = pstruEventDetails->dEventTime;
	NetSim_PACKET* packet = pstruEventDetails->pPacket;
	ptrETH_LAN lan = GET_ETH_LAN(d, in, 0);

	if (!lan->STPStatus)
	{
		fnNetSimError("ConfigBPDU packet is arrived to device %d interface %d. Spanning tree is not enable.",
					  d, in);
		fn_NetSim_Packet_FreePacket(packet);
		pstruEventDetails->pPacket = NULL;
		return;
	}

	ptrETH_BPDU bpdu = get_eth_hdr_var(packet);
	if (!bpdu)
	{
		fnNetSimError("ConfigBPDU hdr is NULL\n");
		fn_NetSim_Packet_FreePacket(packet);
		pstruEventDetails->pPacket = NULL;
		return;
	}

	print_ethernet_log("Switch %d, Port %d, Time %0.0lf: Config BPDU packet arrives",
					   d, in, time);

	print_ethernet_log("Config BPDU data: Root = %s, Cost = %d, Bridge Id = %d",
					   bpdu->rootId,
					   bpdu->rootPathCost,
					   bpdu->bridgeId);
	print_ethernet_log("SPT data: Root = %s, cost = %d, Bridge Id = %s, Root port = %d",
					   lan->spt->rootId,
					   lan->spt->rootPathCost,
					   lan->spt->switchId,
					   lan->spt->rootPort);

	COMPARE_BPDU isBlocked = compare_bpdu(bpdu, lan->spt, in);
	if (isBlocked == BLOCK)
	{
		block_port(d, lan, in);
	}
	else if (isBlocked == FORWARD)
	{
		mark_port(lan, in);
		update_spt(lan->spt, bpdu, in);
		forward_bpdu(d, in, time, lan, bpdu);
	}
	fn_NetSim_Packet_FreePacket(packet);
	pstruEventDetails->pPacket = NULL;
	print_ethernet_log("");
}

void print_spanning_tree()
{
	NETSIM_ID i;
	FILE* fp;
	char buf[BUFSIZ];
	sprintf(buf, "%s%s%s",
			pszIOPath,
			pathSeperator,
			"SpanningTree.txt");
	fp = fopen(buf, "w");
	if (!fp)
	{
		perror(buf);
		fnSystemError("Unable to open SpanningTree.txt file\n");
		return;
	}

	for (i = 0; i < NETWORK->nDeviceCount; i++)
	{
		ptrETH_VAR eth = GET_ETH_VAR(i + 1);
		if (!eth || !eth->isSPT)
			continue;

		UINT j;
		for (j = 0; j < eth->lanCount; j++)
		{
			ptrETH_LAN lan = eth->lanVar[j];
			if (!lan->STPStatus)
				continue;

			UINT k;
			for (k = 0; k < lan->interfaceCount; k++)
			{
				ptrETH_IF inter = lan->ethIfVar[k];
				NETSIM_ID l, c, ci;
				l = fn_NetSim_Stack_GetConnectedDevice(i+1, lan->ethIfVar[k]->interfaceId, &c, &ci);
				ptrETH_LAN lm = GET_ETH_LAN(c, ci, 0);
				if (!lm->STPStatus)
					continue;

				fprintf(fp, "%s,%d,%s\n",
						DEVICE_NAME(i + 1),
						DEVICE_INTERFACE_CONFIGID(i + 1, inter->interfaceId),
						inter->portState == PORTSTATE_BLOCKING ? "BLOCKED" : "FORWARD");
			}
		}
	}
	fclose(fp);
}
