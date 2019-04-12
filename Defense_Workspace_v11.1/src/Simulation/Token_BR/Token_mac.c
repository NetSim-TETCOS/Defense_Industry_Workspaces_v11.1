/************************************************************************************
 * Copyright (C) 2016                                                               *
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
#include "Token.h"

void forward_packet()
{
	PTOKEN_PHYVAR phy = TOKEN_CURRPHY;
	NetSim_BUFFER* buf = DEVICE_ACCESSBUFFER(pstruEventDetails->nDeviceId,
		pstruEventDetails->nInterfaceId);
	NetSim_PACKET* packet = fn_NetSim_Packet_GetPacketFromBuffer(buf,1);

	packet->nTransmitterId = pstruEventDetails->nDeviceId;
	packet->nReceiverId = phy->nextDevId;

	packet->pstruMacData->dPayload = packet->pstruNetworkData->dPacketSize;
	packet->pstruMacData->dOverhead = Token_MAC_OVERHEAD;
	packet->pstruMacData->dPacketSize = packet->pstruMacData->dPayload+
		packet->pstruMacData->dOverhead;

	packet->pstruMacData->dArrivalTime=packet->pstruNetworkData->dEndTime;
	packet->pstruMacData->dStartTime = pstruEventDetails->dEventTime;
	packet->pstruMacData->dEndTime = pstruEventDetails->dEventTime;

	//Add phy out event
	pstruEventDetails->dPacketSize = fnGetPacketSize(packet);
	if(packet->pstruAppData)
	{
		pstruEventDetails->nApplicationId = packet->pstruAppData->nApplicationId;
		pstruEventDetails->nSegmentId = packet->pstruAppData->nSegmentId;
	}
	else
	{
		pstruEventDetails->nApplicationId = 0;
		pstruEventDetails->nSegmentId = 0;
	}
	pstruEventDetails->nEventType=PHYSICAL_OUT_EVENT;
	pstruEventDetails->nPacketId=packet->nPacketId;
	pstruEventDetails->nSubEventType=0;
	pstruEventDetails->pPacket=packet;
	pstruEventDetails->szOtherDetails=NULL;
	fnpAddEvent(pstruEventDetails);
}

void forward_token()
{
	PTOKEN_PHYVAR phy = TOKEN_CURRPHY;
	PTOKEN_MACVAR mac = TOKEN_CURRMAC;

	NetSim_PACKET* packet = mac->token;

	mac->tokenHeld = false;
	mac->token=NULL;

	packet->nTransmitterId = pstruEventDetails->nDeviceId;
	packet->nReceiverId = phy->nextDevId;

	packet->nSourceId = pstruEventDetails->nDeviceId;
	NETSIM_ID d = get_first_dest_from_packet(packet);
	remove_dest_from_packet(packet, d);
	add_dest_to_packet(packet, phy->nextDevId);

	packet->pstruMacData->dStartTime = pstruEventDetails->dEventTime;
	packet->pstruMacData->dEndTime = pstruEventDetails->dEventTime;

	//Add phy out event
	pstruEventDetails->dPacketSize = fnGetPacketSize(packet);
	if(packet->pstruAppData)
	{
		pstruEventDetails->nApplicationId = packet->pstruAppData->nApplicationId;
		pstruEventDetails->nSegmentId = packet->pstruAppData->nSegmentId;
	}
	else
	{
		pstruEventDetails->nApplicationId = 0;
		pstruEventDetails->nSegmentId = 0;
	}
	pstruEventDetails->nEventType=PHYSICAL_OUT_EVENT;
	pstruEventDetails->nPacketId=packet->nPacketId;
	pstruEventDetails->nSubEventType=0;
	pstruEventDetails->pPacket=packet;
	pstruEventDetails->szOtherDetails=NULL;
	fnpAddEvent(pstruEventDetails);
}

void fn_NetSim_Token_StartTokenTransmission()
{
	PTOKEN_MACVAR mac = TOKEN_CURRMAC;
	if(mac->tokenHeld)
	{
		NetSim_BUFFER* buf = DEVICE_ACCESSBUFFER(pstruEventDetails->nDeviceId,
			pstruEventDetails->nInterfaceId);
		if(fn_NetSim_GetBufferStatus(buf))
			forward_packet();
		else
			forward_token();
	}
}

void forward_to_network_layer()
{
	pstruEventDetails->nEventType = NETWORK_IN_EVENT;
	pstruEventDetails->pPacket = fn_NetSim_Packet_CopyPacket(pstruEventDetails->pPacket);
	fnpAddEvent(pstruEventDetails);
}

void forward_to_next()
{
	PTOKEN_PHYVAR phy = TOKEN_CURRPHY;
	NetSim_PACKET* packet = pstruEventDetails->pPacket;

	packet->nTransmitterId = pstruEventDetails->nDeviceId;
	packet->nReceiverId = phy->nextDevId;

	packet->pstruMacData->dStartTime = pstruEventDetails->dEventTime;
	packet->pstruMacData->dEndTime = pstruEventDetails->dEventTime;

	//Add phy out event
	pstruEventDetails->dPacketSize = fnGetPacketSize(packet);
	if(packet->pstruAppData)
	{
		pstruEventDetails->nApplicationId = packet->pstruAppData->nApplicationId;
		pstruEventDetails->nSegmentId = packet->pstruAppData->nSegmentId;
	}
	else
	{
		pstruEventDetails->nApplicationId = 0;
		pstruEventDetails->nSegmentId = 0;
	}
	pstruEventDetails->nEventType=PHYSICAL_OUT_EVENT;
	pstruEventDetails->nPacketId=packet->nPacketId;
	pstruEventDetails->nSubEventType=0;
	pstruEventDetails->pPacket=packet;
	pstruEventDetails->szOtherDetails=NULL;
	fnpAddEvent(pstruEventDetails);
}

void fn_NetSim_Token_MacIn()
{
	NetSim_PACKET* p=pstruEventDetails->pPacket;
	if(p->nControlDataType == TOKEN_PACKET)
	{
		TOKEN_CURRMAC->tokenHeld=true;
		TOKEN_CURRMAC->token = p;
	}
	else
	{
		if(p->nSourceId == pstruEventDetails->nDeviceId)
			fn_NetSim_Packet_FreePacket(p);
		else if(get_first_dest_from_packet(p) == pstruEventDetails->nDeviceId ||
			isBroadcastPacket(p))
		{
			forward_to_network_layer();
			pstruEventDetails->pPacket = p;
			forward_to_next();
		}
		else
			forward_to_next();
	}
	fn_NetSim_Token_StartTokenTransmission();
}