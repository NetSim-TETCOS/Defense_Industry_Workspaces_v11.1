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

static void init_eth_phy_buffer(ptrETH_PHY phy)
{
	phy->packet = calloc(1, sizeof* phy->packet);
	NETSIM_ID d = pstruEventDetails->nDeviceId;
	NETSIM_ID in = phy->interfaceId;
	NetSim_BUFFER* buff = DEVICE_ACCESSBUFFER(d, in);
	memcpy(phy->packet, buff, sizeof* phy->packet);
	phy->packet->pstruPacketlist = NULL;
}

static void eth_phy_packet_add_to_list(ptrETH_PHY phy, NetSim_PACKET* packet)
{
	if (!packet)
		return;

	if (!phy->packet)
		init_eth_phy_buffer(phy);

	fn_NetSim_Packet_AddPacketToList(phy->packet, packet, 0);
}

static NetSim_PACKET* eth_phy_packet_get_from_list(ptrETH_PHY phy)
{
	if (!phy->packet)
		return NULL;

	return fn_NetSim_Packet_GetPacketFromBuffer(phy->packet, 1);
}

static bool isPhyHasPacket(ptrETH_PHY phy)
{
	if (phy->packet)
		return fn_NetSim_GetBufferStatus(phy->packet);
	else
		return false;
}

double calculate_txtime(ptrETH_PHY phy, double size)
{
	double rate = phy->speed;
	return (size * 8.0) / rate; //In Microsec
}

static void eth_phy_packet_set_param(NetSim_PACKET* packet,
									 double start,
									 double end,
									 double tx)
{
	packet->pstruPhyData->dArrivalTime = start;
	packet->pstruPhyData->dEndTime = end;
	packet->pstruPhyData->dOverhead = 0;
	packet->pstruPhyData->dPacketSize = packet->pstruMacData->dPacketSize;
	packet->pstruPhyData->dPayload = packet->pstruMacData->dPacketSize;
	packet->pstruPhyData->dStartTime = start + tx;
	packet->pstruPhyData->nPhyMedium = PHY_MEDIUM_WIRED;
}

static void eth_phy_add_phy_in(NetSim_PACKET* packet,
							   NETSIM_ID c,
							   NETSIM_ID ci)
{
	packet->nReceiverId = c;
	packet->nTransmitterId = pstruEventDetails->nDeviceId;

	NetSim_EVENTDETAILS pevent;
	memset(&pevent, 0, sizeof pevent);
	pevent.dEventTime = packet->pstruPhyData->dEndTime;
	pevent.dPacketSize = packet->pstruPhyData->dPacketSize;
	if (packet->pstruAppData)
	{
		pevent.nApplicationId = packet->pstruAppData->nApplicationId;
		pevent.nSegmentId = packet->pstruAppData->nSegmentId;
	}
	pevent.nDeviceId = c;
	pevent.nDeviceType = DEVICE_TYPE(c);
	pevent.nEventType = PHYSICAL_IN_EVENT;
	pevent.nInterfaceId = ci;
	pevent.nPacketId = packet->nPacketId;
	pevent.nProtocolId = MAC_PROTOCOL_IEEE802_3;
	pevent.pPacket = packet;
	fnpAddEvent(&pevent);
}

static void eth_phy_add_phy_out(double time, NetSim_PACKET* packet)
{
	NetSim_EVENTDETAILS pevent;
	memcpy(&pevent, pstruEventDetails, sizeof pevent);
	pevent.dEventTime = time;
	pevent.dPacketSize = packet->pstruMacData->dPacketSize;
	if (packet->pstruAppData)
	{
		pevent.nApplicationId = packet->pstruAppData->nApplicationId;
		pevent.nSegmentId = packet->pstruAppData->nSegmentId;
	}
	else
	{
		pevent.nApplicationId = 0;
		pevent.nSegmentId = 0;
	}
	pevent.nPacketId = packet->nPacketId;
	pevent.nSubEventType = 0;
	pevent.pPacket = NULL;
	fnpAddEvent(&pevent);
}
int fn_NetSim_Ethernet_HandlePhyOut()
{
	NETSIM_ID d = pstruEventDetails->nDeviceId;
	NETSIM_ID in = pstruEventDetails->nInterfaceId;
	ptrETH_PHY phy = ETH_PHY_GET(d, in);
	NetSim_PACKET* packet = pstruEventDetails->pPacket;
	bool flag = true;

	if (isPhyHasPacket(phy))
		flag = false;

	eth_phy_packet_add_to_list(phy, packet);

	if (phy->lastPacketEndTime > pstruEventDetails->dEventTime)
	{
		// Wait for current transmission to finish
		if (flag)
			goto ADD_NEXT;
		else
			return 1;
	}

	packet = eth_phy_packet_get_from_list(phy);
	if (!packet)
		return 2; // No packet is there for transmission

	double start;
	if (phy->lastPacketEndTime + phy->IFG <= pstruEventDetails->dEventTime)
		start = pstruEventDetails->dEventTime;
	else
		start = phy->lastPacketEndTime + phy->IFG;

	double size = packet->pstruMacData->dPacketSize;

	double txtime = calculate_txtime(phy, size);
	double end = start + txtime + phy->propagationDelay;
	eth_phy_packet_set_param(packet, start, end, txtime);

	eth_phy_add_phy_in(packet, phy->connectedDevice, phy->connectedInterface);
	phy->lastPacketEndTime = start + txtime;

ADD_NEXT:
	if (isPhyHasPacket(phy))
		eth_phy_add_phy_out(phy->lastPacketEndTime, 
							fn_NetSim_Packet_GetPacketFromBuffer(phy->packet,0));
	return 0;
}

int fn_NetSim_Ethernet_HandlePhyIn()
{
	NetSim_PACKET* packet = pstruEventDetails->pPacket;
	NETSIM_ID d = pstruEventDetails->nDeviceId;
	NETSIM_ID in = pstruEventDetails->nInterfaceId;
	ptrETH_PHY phy = ETH_PHY_GET(d, in);

	if (phy->isErrorChkReqd)
	{
		packet->nPacketStatus = fn_NetSim_Packet_DecideError(phy->BER, packet->pstruPhyData->dPacketSize);
		if (packet->nPacketStatus)
		{
			fn_NetSim_WritePacketTrace(packet);
			fn_NetSim_Metrics_Add(packet);
			fn_NetSim_Packet_FreePacket(packet);
			return -1; //Packet is error. Dropped
		}
	}

	fn_NetSim_WritePacketTrace(packet);
	fn_NetSim_Metrics_Add(packet);
	pstruEventDetails->nEventType = MAC_IN_EVENT;
	fnpAddEvent(pstruEventDetails);
	return 0;
}