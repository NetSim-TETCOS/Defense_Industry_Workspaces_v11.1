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
#include "../Firewall/Firewall.h"

#define ETHERNET_FRAME_HEADER_LEN	26 //Bytes

NETSIM_ID find_forward_interface(ptrETH_LAN lan, NetSim_PACKET* packet)
{
#pragma warning(disable:4189) // For debug
	PNETSIM_MACADDRESS src = packet->pstruMacData->szSourceMac;
#pragma warning(default:4189)
	PNETSIM_MACADDRESS dest = packet->pstruMacData->szDestMac;
	ptrSWITCHTABLE table = SWITCHTABLE_FIND(lan, dest);
	return table ? table->outPort : 0;
}

void send_to_phy(NETSIM_ID d, NETSIM_ID in, NetSim_PACKET* packet, double time)
{
	NetSim_EVENTDETAILS pevent;
	memset(&pevent, 0, sizeof pevent);
	pevent.dEventTime = time;
	pevent.dPacketSize = packet->pstruMacData->dPacketSize;
	if (packet->pstruAppData)
	{
		pevent.nApplicationId = packet->pstruAppData->nApplicationId;
		pevent.nSegmentId = packet->pstruAppData->nSegmentId;
	}
	pevent.nDeviceId = d;
	pevent.nDeviceType = DEVICE_TYPE(d);
	pevent.nEventType = PHYSICAL_OUT_EVENT;
	pevent.nInterfaceId = in;
	pevent.nPacketId = packet->nPacketId;
	pevent.nProtocolId = MAC_PROTOCOL_IEEE802_3;
	pevent.pPacket = packet;
	fnpAddEvent(&pevent);
}

static void add_mac_header(NetSim_PACKET* packet, double time)
{
	packet->pstruMacData->dPayload = packet->pstruNetworkData->dPacketSize;
	packet->pstruMacData->dOverhead = ETHERNET_FRAME_HEADER_LEN;
	packet->pstruMacData->dPacketSize = packet->pstruMacData->dPayload +
		packet->pstruMacData->dOverhead;

	packet->pstruMacData->dArrivalTime = time;
	packet->pstruMacData->dEndTime = time;
	packet->pstruMacData->dStartTime = time;

	packet->pstruMacData->nMACProtocol = MAC_PROTOCOL_IEEE802_3;
}

static int packet_arrive_from_network_layer()
{
	NETSIM_ID d = pstruEventDetails->nDeviceId;
	NETSIM_ID in = pstruEventDetails->nInterfaceId;
	double time = pstruEventDetails->dEventTime;

	NetSim_PACKET* packet = fn_NetSim_Packet_GetPacketFromBuffer(DEVICE_ACCESSBUFFER(d, in), 1);
	while (packet)
	{
		UINT16 vlanId = eth_get_incoming_vlanid(packet, d);
		NETSIM_ID incoming = eth_get_incoming_port(packet, d);
		clear_eth_incoming(packet);

		if (vlanId)
		{
			vlan_macout_forward_packet(d, in, vlanId, incoming, packet, time);
			//Take the next packet
			packet = fn_NetSim_Packet_GetPacketFromBuffer(DEVICE_ACCESSBUFFER(d, in), 1);
			continue;
		}

		NETSIM_ID out = 0;
		ptrETH_LAN lan = GET_ETH_LAN(d, in, 0);
		if (lan->interfaceCount > 1)
		{
			out = find_forward_interface(lan, packet);
		}

		add_mac_header(packet, time);
		if (out)
		{
			if (fn_NetSim_MAC_Firewall(d, out, packet, ACLTYPE_OUTBOUND) == ACLACTION_PERMIT &&
				out != incoming)
			{
				clear_eth_incoming(packet);
				send_to_phy(d, out, packet, time);
			}
			else
				fn_NetSim_Packet_FreePacket(packet);
		}
		else
		{
			UINT j;
			for (j = 0; j < lan->interfaceCount; j++)
			{
				if (lan->ethIfVar[j]->portState == PORTSTATE_BLOCKING)
					continue;

				NetSim_PACKET* p = fn_NetSim_Packet_CopyPacket(packet);
				if (fn_NetSim_MAC_Firewall(d, lan->ethIfVar[j]->interfaceId, p, ACLTYPE_OUTBOUND) == ACLACTION_PERMIT &&
					lan->ethIfVar[j]->interfaceId != incoming)
				{
					clear_eth_incoming(p);
					send_to_phy(d, lan->ethIfVar[j]->interfaceId, p, time);
				}
				else
					fn_NetSim_Packet_FreePacket(p);
			}

			vlan_forward_to_all_trunk(d, packet, lan, time);
			fn_NetSim_Packet_FreePacket(packet);
		}

		//Take the next packet
		packet = fn_NetSim_Packet_GetPacketFromBuffer(DEVICE_ACCESSBUFFER(d, in), 1);
	}
	return 0;
}

int fn_NetSim_Ethernet_HandleMacOut()
{
	switch (pstruEventDetails->nSubEventType)
	{
		case 0:
			return packet_arrive_from_network_layer();
			break;
		case SEND_CONFIG_BPDU:
			return multicast_config_bpdu();
		default:
			fnNetSimError("Unknown subevent %d for Ethernet in %s",
						  pstruEventDetails->nSubEventType,
						  __FUNCTION__);
			return -1;
	}
}

static bool validate_mac_frame(ptrETH_LAN lan, PNETSIM_MACADDRESS mac)
{
	if (lan->ispromiscuousMode)
		return true;

	if (isBroadcastMAC(mac))
		return true;

	if (isMulticastMAC(mac))
		return true;

	if (lan->switchingTechnique)
		return true;

	UINT i;
	for (i = 0; i < lan->interfaceCount; i++)
	{
		ptrETH_IF inter = lan->ethIfVar[i];
		PNETSIM_MACADDRESS m = inter->macAddress;
		if (!MAC_COMPARE(m, mac))
			return true;
	}
	return false;
}

void check_move_frame_up(NETSIM_ID d,
						 NETSIM_ID in,
						 ptrETH_LAN lan,
						 NetSim_PACKET* packet,
						 double time)
{
	PNETSIM_MACADDRESS dest = packet->pstruMacData->szDestMac;
	bool isforme = validate_mac_frame(lan, dest);
	if (isforme)
	{
		NetSim_EVENTDETAILS pevent;
		memset(&pevent, 0, sizeof pevent);
		pevent.dEventTime = time;
		pevent.dPacketSize = packet->pstruNetworkData->dPacketSize;
		if (packet->pstruAppData)
		{
			pevent.nApplicationId = packet->pstruAppData->nApplicationId;
			pevent.nSegmentId = packet->pstruAppData->nSegmentId;
		}
		pevent.nDeviceId = d;
		pevent.nDeviceType = DEVICE_TYPE(d);
		pevent.nEventType = NETWORK_IN_EVENT;
		pevent.nInterfaceId = in;
		pevent.nPacketId = packet->nPacketId;
		pevent.nProtocolId = NW_PROTOCOL_IPV4;
		pevent.pPacket = packet;
		fnpAddEvent(&pevent);
	}
	else
	{
		fn_NetSim_Packet_FreePacket(packet);
		pstruEventDetails->pPacket = NULL;
	}
}

static void forward_packet_to_other_protocol(NETSIM_ID d,
											 NETSIM_ID in,
											 NetSim_PACKET* packet,
											 double time)
{
	NetSim_BUFFER* buffer = DEVICE_ACCESSBUFFER(d, in);
	NetSim_PACKET* p = fn_NetSim_Packet_CopyPacket(packet);

	if (!fn_NetSim_GetBufferStatus(buffer))
	{
		NetSim_EVENTDETAILS pevent;
		memset(&pevent, 0, sizeof pevent);
		pevent.dEventTime = time;
		pevent.dPacketSize = p->pstruNetworkData->dPacketSize;
		if (p->pstruAppData)
		{
			pevent.nApplicationId = p->pstruAppData->nApplicationId;
			pevent.nSegmentId = p->pstruAppData->nSegmentId;
		}
		pevent.nDeviceId = d;
		pevent.nDeviceType = DEVICE_TYPE(d);
		pevent.nEventType = MAC_OUT_EVENT;
		pevent.nInterfaceId = in;
		pevent.nPacketId = p->nPacketId;
		pevent.nProtocolId = DEVICE_MACLAYER(d, in)->nMacProtocolId;
		fnpAddEvent(&pevent);
	}
	fn_NetSim_Packet_AddPacketToList(buffer, p, 0);
}

void forward_frame(NETSIM_ID d,
				   NETSIM_ID in,
				   ptrETH_LAN lan,
				   NetSim_PACKET* packet,
				   double time)
{

	NETSIM_ID f = find_forward_interface(lan, packet);

	if (f)
	{
		if (fn_NetSim_MAC_Firewall(d, f, packet, ACLTYPE_OUTBOUND) == ACLACTION_PERMIT)
		{
			clear_eth_incoming(packet);
			send_to_phy(d, f, packet, time);
		}
		else
			fn_NetSim_Packet_FreePacket(packet);
	}
	else
	{
		if (DEVICE_TYPE(d) == SWITCH)
		{
			UINT j;
			for (j = 0; j < lan->interfaceCount; j++)
			{
				if (lan->ethIfVar[j]->interfaceId == in)
					continue;

				if (lan->ethIfVar[j]->portState == PORTSTATE_BLOCKING)
					continue;

				NetSim_PACKET* p = fn_NetSim_Packet_CopyPacket(packet);
				if (fn_NetSim_MAC_Firewall(d, lan->ethIfVar[j]->interfaceId, packet, ACLTYPE_OUTBOUND) == ACLACTION_PERMIT)
				{
					clear_eth_incoming(p);
					send_to_phy(d, lan->ethIfVar[j]->interfaceId, p, time);
				}
				else
					fn_NetSim_Packet_FreePacket(p);
			}

			vlan_forward_to_all_trunk(d, packet, lan, time);
		}
		else if(!lan->lanIP)
		{
			NETSIM_ID j;
			//Forward to all other interface other than switch
			for (j = 0; j < DEVICE(d)->nNumOfInterface; j++)
			{
				if (j + 1 == in)
					continue;

				forward_packet_to_other_protocol(d, j+1, packet, time);
			}
		}
		fn_NetSim_Packet_FreePacket(packet);
	}
}

static bool decide_move_up(ptrETH_LAN lan)
{
	if (lan->lanIP)
		return true;
	else
		return false;
}

static void ethernet_mac_process_frame()
{
	NETSIM_ID d = pstruEventDetails->nDeviceId;
	NETSIM_ID in = pstruEventDetails->nInterfaceId;
	double time = pstruEventDetails->dEventTime;
	NetSim_PACKET* packet = pstruEventDetails->pPacket;

	if (fn_NetSim_MAC_Firewall(d, in, packet, ACLTYPE_INBOUND) == ACLACTION_DENY)
	{
		fn_NetSim_Packet_FreePacket(packet);
		return;
	}

#pragma warning(disable:4189) // For debug
	PNETSIM_MACADDRESS dest = packet->pstruMacData->szDestMac;
#pragma warning(default:4189)

	PNETSIM_MACADDRESS src = packet->pstruMacData->szSourceMac;

	ptrETH_LAN lan = GET_ETH_LAN(d, in, 0);
	if (lan->switchingTechnique)
		SWITCHTABLE_NEW(lan, src, in);

	bool isTag = isVLANTagPresent(packet);
	store_eth_param_macin(packet, d, in, lan);

	bool isMoveup = decide_move_up(lan);
	if (isMoveup)
	{
		check_move_frame_up(d, in, lan, packet, time);
	}
	else
	{
		if (isTag)
			vlan_macin_forward_packet();
		else
			forward_frame(d, in, lan, packet, time);
	}
}

int fn_NetSim_Ethernet_HandleMacIn()
{
	NetSim_PACKET* packet = pstruEventDetails->pPacket;
	if (packet->nControlDataType == ETH_CONFIGBPDU)
		process_configbpdu();
	else
		ethernet_mac_process_frame();
	return 0;
}
