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
#include "P2P.h"

#define LIGHT_SPEED		299792458.0

double get_propagation_delay(NETSIM_ID i, NETSIM_ID j)
{
	double d = fn_NetSim_Utilities_CalculateDistance(DEVICE_POSITION(i), DEVICE_POSITION(j));
	return (d / LIGHT_SPEED)*SECOND;
}

#define calculate_tx_time(size,rate) ((size * 8.0) / rate)

static void add_phy_in(NetSim_PACKET* packet,
					   NETSIM_ID d,
					   NETSIM_ID in,
					   NETSIM_ID c,
					   NETSIM_ID ci)
{
	packet->nTransmitterId = d;
	packet->nReceiverId = c;
	NetSim_EVENTDETAILS pevent;
	pevent.dEventTime = packet->pstruPhyData->dEndTime;
	pevent.dPacketSize = packet->pstruPhyData->dPacketSize;
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
	pevent.nDeviceId = c;
	pevent.nDeviceType = DEVICE_TYPE(c);
	pevent.nEventType = PHYSICAL_IN_EVENT;
	pevent.nInterfaceId = ci;
	pevent.nPacketId = packet->nPacketId;
	pevent.nProtocolId = DEVICE_MACLAYER(c, ci)->nMacProtocolId;
	pevent.nSubEventType = 0;
	pevent.pPacket = packet;
	pevent.szOtherDetails = NULL;
	fnpAddEvent(&pevent);
}

static double transmit_over_wired(NetSim_PACKET* packet, NETSIM_ID nDeviceId, NETSIM_ID nInterfaceId)
{
	NETSIM_ID c, ci;
	NetSim_PHYSICALLAYER* phy = DEVICE_PHYLAYER(nDeviceId, nInterfaceId);
	NetSim_LINKS* link = phy->pstruNetSimLinks;
	fn_NetSim_Stack_GetConnectedDevice(nDeviceId, nInterfaceId, &c, &ci);
	double dDataRate = link->puniMedProp.pstruWiredLink.dDataRateUp;
	double dPropagationDelay = link->puniMedProp.pstruWiredLink.dPropagationDelayUp;

	double txTime = calculate_tx_time(packet->pstruPhyData->dPacketSize,
									  dDataRate);
	if (packet->pstruMacData->dEndTime <= phy->dLastPacketEndTime)
		packet->pstruPhyData->dArrivalTime = phy->dLastPacketEndTime;
	else
		packet->pstruPhyData->dArrivalTime = packet->pstruMacData->dEndTime;
	packet->pstruPhyData->dStartTime = packet->pstruPhyData->dArrivalTime + txTime;
	packet->pstruPhyData->dEndTime = packet->pstruPhyData->dStartTime + dPropagationDelay;
	add_phy_in(packet, nDeviceId, nInterfaceId, c, ci);
	phy->dLastPacketEndTime = packet->pstruPhyData->dStartTime;
	return phy->dLastPacketEndTime;
}

static double wireless_transmit(NetSim_PACKET* packet,
							  NETSIM_ID d,
							  NETSIM_ID in,
							  NETSIM_ID c,
							  NETSIM_ID ci)
{
	NetSim_PHYSICALLAYER* phy = DEVICE_PHYLAYER(d, in);
	ptrP2P_NODE_PHY sphy = P2P_PHY(d, in);
	ptrP2P_NODE_PHY dphy = P2P_PHY(c, ci);

	double txtime = calculate_tx_time(packet->pstruPhyData->dPacketSize, sphy->dDataRate);
	if (packet->pstruMacData->dEndTime <= phy->dLastPacketEndTime)
		packet->pstruPhyData->dArrivalTime = phy->dLastPacketEndTime;
	else
		packet->pstruPhyData->dArrivalTime = packet->pstruMacData->dEndTime;

	double time = packet->pstruPhyData->dArrivalTime;

	phy->dLastPacketEndTime = time + txtime;

	double pdbm = propagation_get_received_power_dbm(propagationHandle, d, in, c, ci, time);
	if (pdbm < dphy->dReceiverSensitivity)
	{
		fn_NetSim_Packet_FreePacket(packet);
		return time + txtime;
	}

	double dPropagationDelay = get_propagation_delay(d, c);
	packet->pstruPhyData->dStartTime = time + txtime;
	packet->pstruPhyData->dEndTime = packet->pstruPhyData->dStartTime + dPropagationDelay;
	add_phy_in(packet, d, in, c, ci);
	phy->dLastPacketEndTime = packet->pstruPhyData->dStartTime;
	return packet->pstruPhyData->dStartTime;
}

static double wireless_P2MP_unicast(NetSim_PACKET* packet, NETSIM_ID d, NETSIM_ID in)
{
	NETSIM_ID c, ci;
	c = packet->nReceiverId;
	ci = fn_NetSim_Stack_GetConnectedInterface(d, in, c);
	return wireless_transmit(packet, d, in, c, ci);
}

static double wireless_P2MP_broadcast(NetSim_PACKET* packet, NETSIM_ID d, NETSIM_ID in)
{
	double time = packet->pstruPhyData->dArrivalTime;
	double t;
	NetSim_LINKS* link = DEVICE_PHYLAYER(d, in)->pstruNetSimLinks;
	NETSIM_ID i;
	NETSIM_ID c, ci;
	if (d == link->puniDevList.pstrup2MP.nCenterDeviceId)
	{
		for (i = 0; i < link->puniDevList.pstrup2MP.nConnectedDeviceCount; i++)
		{
			c = link->puniDevList.pstrup2MP.anDevIds[i];
			ci = link->puniDevList.pstrup2MP.anDevInterfaceIds[i];
			NetSim_PACKET* p = fn_NetSim_Packet_CopyPacket(packet);
			t = wireless_transmit(p, d, in, c, ci);
			if (t > time)
				time = t;
		}
		fn_NetSim_Packet_FreePacket(packet);
	}
	else
	{
		c = link->puniDevList.pstrup2MP.nCenterDeviceId;
		ci = link->puniDevList.pstrup2MP.nCenterInterfaceId;
		t = wireless_transmit(packet, d, in, c, ci);
		if (t > time)
			time = t;
	}
	return time;
}

static double wireless_MP2MP_unicast(NetSim_PACKET* packet, NETSIM_ID d, NETSIM_ID in)
{
	NETSIM_ID c, ci;
	c = packet->nReceiverId;
	ci = fn_NetSim_Stack_GetConnectedInterface(d, in, c);
	return wireless_transmit(packet, d, in, c, ci);
}

static double wireless_MP2MP_broadcast(NetSim_PACKET* packet, NETSIM_ID d, NETSIM_ID in)
{
	NetSim_LINKS* link = DEVICE_PHYLAYER(d, in)->pstruNetSimLinks;
	NETSIM_ID i;
	NETSIM_ID c, ci;
	double time = packet->pstruPhyData->dArrivalTime;
	double torg = DEVICE_PHYLAYER(d, in)->dLastPacketEndTime;
	for (i = 0; i < link->puniDevList.pstruMP2MP.nConnectedDeviceCount; i++)
	{
		if (d == link->puniDevList.pstruMP2MP.anDevIds[i])
			continue;
		c = link->puniDevList.pstruMP2MP.anDevIds[i];
		ci = link->puniDevList.pstruMP2MP.anDevInterfaceIds[i];
		NetSim_PACKET* p = fn_NetSim_Packet_CopyPacket(packet);
		DEVICE_PHYLAYER(d, in)->dLastPacketEndTime = torg;
		double t = wireless_transmit(p, d, in, c, ci);
		if (t > time)
			time = t;
	}
	DEVICE_PHYLAYER(d, in)->dLastPacketEndTime = time;
	fn_NetSim_Packet_FreePacket(packet);
	return time;
}

static double transmit_over_wireless(NetSim_PACKET* packet, NETSIM_ID d, NETSIM_ID in)
{
	NETSIM_ID c, ci;
	NetSim_LINKS* link = DEVICE_PHYLAYER(d, in)->pstruNetSimLinks;
	if (link->nLinkType == LinkType_P2P)
	{
		fn_NetSim_Stack_GetConnectedDevice(d, in, &c, &ci);
		return wireless_transmit(packet, d, in, c, ci);
	}
	else if(link->nLinkType == LinkType_P2MP)
	{
		if (packet->nReceiverId)
			return wireless_P2MP_unicast(packet, d, in);
		else
			return wireless_P2MP_broadcast(packet, d, in);
	}
	else if (link->nLinkType == LinkType_MP2MP)
	{
		if (packet->nReceiverId)
			return wireless_MP2MP_unicast(packet, d, in);
		else
			return wireless_MP2MP_broadcast(packet, d, in);
	}
	return packet->pstruPhyData->dArrivalTime;
}

static double fnTransmitPacket(NetSim_PACKET* pPacket, NETSIM_ID nDeviceId, NETSIM_ID nInterfaceId)
{
	NetSim_LINKS* link = DEVICE_PHYLAYER(nDeviceId, nInterfaceId)->pstruNetSimLinks;
	if (link->isLinkFailureMode)
	{
		if (link->struLinkFailureModel.linkState == LINKSTATE_DOWN)
		{
			fn_NetSim_Packet_FreePacket(pPacket);
			return pstruEventDetails->dEventTime;
		}
	}

	if (link->nLinkMedium == PHY_MEDIUM_WIRED)
		return transmit_over_wired(pPacket, nDeviceId, nInterfaceId);
	else
		return transmit_over_wireless(pPacket, nDeviceId, nInterfaceId);
}

int P2P_PhyOut_Handler()
{
	NETSIM_ID d = pstruEventDetails->nDeviceId;
	NETSIM_ID in = pstruEventDetails->nInterfaceId;
	NetSim_BUFFER* buf = DEVICE_ACCESSBUFFER(d, in);
	fn_NetSim_Packet_GetPacketFromBuffer(buf, 1);
	NetSim_PACKET* pstruPacket = pstruEventDetails->pPacket;
	pstruPacket->pstruPhyData->dArrivalTime = ldEventTime;
	pstruPacket->pstruPhyData->dOverhead = 0;
	pstruPacket->pstruPhyData->dPayload = pstruPacket->pstruMacData->dPacketSize;
	pstruPacket->pstruPhyData->dPacketSize = pstruPacket->pstruPhyData->dOverhead + pstruPacket->pstruPhyData->dPayload;
	//Transmit the packet.
	double time = fnTransmitPacket(pstruEventDetails->pPacket, d, in);
	//Check for buffer
	if (fn_NetSim_GetBufferStatus(buf))
	{
		NetSim_PACKET* p = fn_NetSim_Packet_GetPacketFromBuffer(buf, 0);
		//Prepare event details for MAC out
		pstruEventDetails->dEventTime = time;
		pstruEventDetails->dPacketSize = 0;
		pstruEventDetails->nDeviceId = d;
		pstruEventDetails->nDeviceType = DEVICE_TYPE(d);
		pstruEventDetails->nInterfaceId = in;
		pstruEventDetails->nEventType = MAC_OUT_EVENT;
		pstruEventDetails->nProtocolId = fn_NetSim_Stack_GetMacProtocol(d, in);
		pstruEventDetails->pPacket = NULL;
		pstruEventDetails->nPacketId = p->nPacketId;
		if (p->pstruAppData)
		{
			pstruEventDetails->nApplicationId = p->pstruAppData->nApplicationId;
			pstruEventDetails->nSegmentId = p->pstruAppData->nSegmentId;
		}
		else
		{
			pstruEventDetails->nApplicationId = 0;
			pstruEventDetails->nSegmentId = 0;
		}
		//Add MAC out Event
		fnpAddEvent(pstruEventDetails);
	}
	return 0;
}

PACKET_STATUS P2P_Wireless_CalculateError(NETSIM_ID d, NETSIM_ID in, NetSim_PACKET* packet)
{
	NETSIM_ID c = packet->nTransmitterId;
	NETSIM_ID ci = fn_NetSim_Stack_GetConnectedInterface(d, in, c);
	ptrP2P_NODE_PHY phy = P2P_PHY(c, ci);
	double pb = calculate_BER(phy->modulation,
							  propagation_get_received_power_dbm(propagationHandle, c, ci, d, in, packet->pstruPhyData->dArrivalTime),
							  phy->dBandwidth);

	return fn_NetSim_Packet_DecideError(pb, packet->pstruPhyData->dPacketSize);
}

PACKET_STATUS P2P_Wired_CalculateError(NETSIM_ID d, NETSIM_ID in, NetSim_PACKET* packet)
{
	NetSim_LINKS* link = DEVICE_PHYLAYER(d, in)->pstruNetSimLinks;
	double pb = link->puniMedProp.pstruWiredLink.dErrorRateUp;
	return fn_NetSim_Packet_DecideError(pb, packet->pstruPhyData->dPacketSize);
}

static void P2P_Calculate_Error()
{
	NETSIM_ID d = pstruEventDetails->nDeviceId;
	NETSIM_ID in = pstruEventDetails->nInterfaceId;
	NetSim_PACKET* packet = pstruEventDetails->pPacket;
	if (isP2PWireless(d,in))
		packet->nPacketStatus = P2P_Wireless_CalculateError(d, in, packet);
	else
		packet->nPacketStatus = P2P_Wired_CalculateError(d, in, packet);
}

int P2P_PhyIn_Handler()
{
	NetSim_PACKET* packet = pstruEventDetails->pPacket;
	fnValidatePacket(packet);

	P2P_Calculate_Error();

	fn_NetSim_WritePacketTrace(packet);
	//Place the packet to mac layer
	//Write mac in event
	packet->pstruPhyData->dArrivalTime = ldEventTime;
	packet->pstruPhyData->dStartTime = ldEventTime;
	packet->pstruPhyData->dEndTime = ldEventTime;
	fn_NetSim_Metrics_Add(packet);
	//Prepare the mac in event details
	pstruEventDetails->dEventTime = ldEventTime;
	pstruEventDetails->nEventType = MAC_IN_EVENT;
	//Add mac in event
	fnpAddEvent(pstruEventDetails);
	return 0;
}
