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
#include "OSPF.h"
#include "OSPF_Msg.h"
#include "OSPF_Interface.h"

typedef struct stru_ospf_msg_database
{
	char name[50];
	void (*fnNew)(ptrOSPFPACKETHDR hdr);
	void(*fnFree)(void*);
	void*(*fnCopy)(void*);
}OSPFMSG_DATABASE;
static OSPFMSG_DATABASE pstruOSPFMsgCallback[] = {
	{"UNKNOWN"},
	{"OSPF_HELLO",OSPF_HELLO_MSG_NEW,OSPF_HELLO_MSG_FREE,OSPF_HELLO_MSG_COPY},
	{"OSPF_DD",OSPF_DD_MSG_NEW,OSPF_DD_MSG_FREE,OSPF_DD_MSG_COPY},
	{"OSPF_LSREQ",OSPF_LSREQ_MSG_NEW,OSPF_LSREQ_MSG_FREE,OSPF_LSREQ_MSG_COPY},
	{"OSPF_LSUPDATE",OSPF_LSUPDATE_MSG_NEW,OSPF_LSUPDATE_MSG_FREE,OSPF_LSUPDATE_MSG_COPY},
	{"OSPF_LSACK",OSPF_LSACK_NEW,OSPF_LSACK_FREE,OSPF_LSACK_COPY}
};

static char* OSPFMSG_TO_STR(OSPFMSG type)
{
	return pstruOSPFMsgCallback[type].name;
}

ptrOSPFPACKETHDR OSPF_HDR_NEW()
{
	ptrOSPFPACKETHDR hdr = calloc(1, sizeof* hdr);
	hdr->Version = OSPF_VERSION;
	hdr->Packet_length = OSPFPACKETHDR_LEN;
	return hdr;
}

ptrOSPFPACKETHDR OSPF_HDR_COPY(ptrOSPFPACKETHDR hdr)
{
	if (!hdr)
		return NULL;

	ptrOSPFPACKETHDR newHdr = calloc(1, sizeof* newHdr);
	memcpy(newHdr, hdr, sizeof* hdr);
	newHdr->ospfMSG = pstruOSPFMsgCallback[hdr->Type].fnCopy(hdr->ospfMSG);
	return newHdr;
}

void OSPF_HDR_FREE(ptrOSPFPACKETHDR hdr)
{
	if (hdr)
	{
		if (hdr->Type &&
			pstruOSPFMsgCallback[hdr->Type].fnFree)
			pstruOSPFMsgCallback[hdr->Type].fnFree(hdr->ospfMSG);
		free(hdr);
	}
}

void OSPF_HDR_SET_MSG(ptrOSPFPACKETHDR hdr,
					  OSPFMSG type,
					  void* msg,
					  UINT16 len)
{
	hdr->Type = type;
	hdr->ospfMSG = msg;
	hdr->Packet_length += len;
}

void OSPF_HDR_INCREASE_LEN(NetSim_PACKET* packet,
						   UINT16 len)
{
	ptrOSPFPACKETHDR hdr = OSPF_PACKET_GET_HDR(packet);
	hdr->Packet_length += len;
	packet->pstruAppData->dOverhead = (double)hdr->Packet_length;
	packet->pstruAppData->dPacketSize = packet->pstruAppData->dPayload +
		packet->pstruAppData->dOverhead;
}

static void ospf_packet_update_src(NetSim_PACKET* packet,
								   NETSIM_ID d,
								   NETSIM_ID in)
{
	packet->nSourceId = d;
	packet->pstruNetworkData->szSourceIP = DEVICE_NWADDRESS(d, in);
}

static ptrOSPFPACKETHDR ospf_get_new_hdr(OSPFMSG type)
{
	ptrOSPFPACKETHDR hdr = OSPF_HDR_NEW();
	if(pstruOSPFMsgCallback[type].fnNew)
		pstruOSPFMsgCallback[type].fnNew(hdr);
	return hdr;
}

NetSim_PACKET* OSPF_PACKET_NEW(double time,
							   OSPFMSG type,
							   NETSIM_ID d,
							   NETSIM_ID in)
{
	NetSim_PACKET* packet = fn_NetSim_Packet_CreatePacket(APPLICATION_LAYER);
	packet->nPacketPriority = Priority_High;
	packet->nPacketType = PacketType_Control;
	packet->nControlDataType = OSPFMSG_TO_PACKETTYPE(type);
	packet->pstruAppData->dArrivalTime = time;
	packet->pstruAppData->dEndTime = time;
	packet->pstruAppData->dStartTime = time;
	packet->pstruAppData->nApplicationProtocol = APP_PROTOCOL_OSPF;
	ptrOSPFPACKETHDR hdr = ospf_get_new_hdr(type);
	packet->pstruAppData->Packet_AppProtocol = hdr;
	packet->pstruAppData->dOverhead = OSPF_HDR_GET_LEN(hdr);
	packet->pstruAppData->dPacketSize = OSPF_HDR_GET_LEN(hdr);
	packet->dEventTime = time;
	strcpy(packet->szPacketType, OSPFMSG_TO_STR(type));
	packet->pstruNetworkData->IPProtocol = IPPROTOCOL_OSPF;

	ospf_packet_update_src(packet, d, in);
	ptrOSPF_PDS ospf = OSPF_PDS_GET(d);
	OSPF_HDR_SET_ROUTERID(hdr, ospf->routerId);

	ptrOSPF_IF ospfif = OSPF_IF_GET(ospf, in);
	OSPF_HDR_SET_AREAID(hdr, ospfif->areaId);
	return packet;
}

void OSPF_SEND_PACKET(NetSim_PACKET* packet)
{
	static int packetId = 0;
	packet->pstruTransportData->dPayload = packet->pstruAppData->dPacketSize;
	packet->pstruTransportData->dPacketSize = packet->pstruAppData->dPacketSize;
	packet->pstruTransportData->dArrivalTime = pstruEventDetails->dEventTime;
	packet->pstruTransportData->dEndTime = pstruEventDetails->dEventTime;
	packet->pstruTransportData->dStartTime = pstruEventDetails->dEventTime;
	packet->nPacketId = packetId;
	//--packetId;
	NetSim_EVENTDETAILS pevent;
	memset(&pevent, 0, sizeof pevent);
	pevent.dEventTime = pstruEventDetails->dEventTime;
	pevent.dPacketSize = packet->pstruAppData->dPacketSize;
	pevent.nDeviceId = pstruEventDetails->nDeviceId;
	pevent.nDeviceType = DEVICE_TYPE(pevent.nDeviceId);
	pevent.nEventType = NETWORK_OUT_EVENT;
	pevent.nInterfaceId = pstruEventDetails->nInterfaceId;
	pevent.nPacketId = packet->nPacketId;
	pevent.pPacket = packet;
	pevent.nProtocolId = NW_PROTOCOL_IPV4;
	fnpAddEvent(&pevent);
}

static bool validate_areaid(NetSim_PACKET* packet)
{
	ptrOSPF_IF ospf = OSPF_IF_CURRENT();

	if (!ospf)
		fnNetSimError("OSPF is not configured for device %d interface %d.\n",
					  DEVICE_CONFIGID(pstruEventDetails->nDeviceId),
					  DEVICE_INTERFACE_CONFIGID(pstruEventDetails->nDeviceId, pstruEventDetails->nInterfaceId));

	ptrOSPF_PDS pds = OSPF_PDS_CURRENT();
	ptrOSPFPACKETHDR hdr = OSPF_PACKET_GET_HDR(packet);
	OSPFID area = ospf->areaId;
	if (!IP_COMPARE(area, hdr->AreaId))
	{
		if (ospf->Type != OSPFIFTYPE_P2P)
		{
			NETSIM_IPAddress src = packet->pstruNetworkData->szSourceIP;
			NETSIM_IPAddress myip = DEVICE_NWADDRESS(pstruEventDetails->nDeviceId, pstruEventDetails->nInterfaceId);
			NETSIM_IPAddress mymask = DEVICE_SUBNETMASK(pstruEventDetails->nDeviceId, pstruEventDetails->nInterfaceId);
			if (IP_IS_IN_SAME_NETWORK_IPV4(src, myip, mymask))
				return true;
			else
				return false;
		}
		else
		{
			return true;
		}
	}
	else if (!IP_COMPARE(hdr->AreaId, NullID))
	{
		if (pds->routerType != OSPFRTYPE_ABR)
			return false;
#pragma message(__LOC__"Change after virtual link implementation")
		return false;
	}
	else
	{
		return false;
	}
}

bool validate_ospf_packet(NetSim_PACKET* packet,
						  NETSIM_ID d,
						  NETSIM_ID in)
{
	//Section 8.2
	NETSIM_IPAddress dest = packet->pstruNetworkData->szDestIP;
	NETSIM_IPAddress src = packet->pstruNetworkData->szSourceIP;
	NETSIM_IPAddress myip = DEVICE_NWADDRESS(d, in);
	
	if (packet->pstruNetworkData->IPProtocol != IPPROTOCOL_OSPF)
		return false; // Not a OSPF packet

	if (!(!IP_COMPARE(dest, myip) ||
		!IP_COMPARE(dest, AllSPFRouters) ||
		!IP_COMPARE(dest, AllDRouters)))
		return false; // Not mine

	if (!IP_COMPARE(src, myip))
		return false; // I received my own packet

	ptrOSPFPACKETHDR hdr = OSPF_PACKET_GET_HDR(packet);
	ptrOSPF_PDS pds = OSPF_PDS_GET(d);
	ptrOSPF_IF ospf = OSPF_IF_GET(pds, in);

	if (hdr->Version != OSPF_VERSION)
		return false;

	if (!validate_areaid(packet))
		return false;

	if (!IP_COMPARE(dest, AllDRouters))
	{
		if (!(ospf->State == OSPFIFSTATE_BACKUP ||
			  ospf->State == OSPFIFSTATE_DR))
			return false;
	}

	//No authentication check

	return true;
}
