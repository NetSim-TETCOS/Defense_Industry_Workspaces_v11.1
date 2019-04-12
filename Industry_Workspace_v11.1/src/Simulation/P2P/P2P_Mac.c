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

int P2P_MacOut_Handler()
{
	NETSIM_ID d = pstruEventDetails->nDeviceId;
	NETSIM_ID in = pstruEventDetails->nInterfaceId;
	NetSim_BUFFER* buf = DEVICE_ACCESSBUFFER(d, in);
	NetSim_PACKET* pstruPacket;
	
	//Get the packet from the Interface buffer
	pstruPacket = fn_NetSim_Packet_GetPacketFromBuffer(buf, 0);
	if (!pstruPacket)
	{
		fnNetSimError("MacOut-No packet in access buffer\n");
		return 0;
	}
	fnValidatePacket(pstruPacket);
	
	//Place the packet to physical layer
	//Write physical out event.
	//Append mac details in packet
	pstruPacket->pstruMacData->dArrivalTime = ldEventTime;
	pstruPacket->pstruMacData->dEndTime = ldEventTime;
	if (pstruPacket->pstruNetworkData)
	{
		pstruPacket->pstruMacData->dOverhead = 0;
		pstruPacket->pstruMacData->dPayload = pstruPacket->pstruNetworkData->dPacketSize;
		pstruPacket->pstruMacData->dPacketSize = pstruPacket->pstruMacData->dOverhead +
			pstruPacket->pstruMacData->dPayload;
	}
	if (pstruPacket->pstruMacData->Packet_MACProtocol &&
		!pstruPacket->pstruMacData->dontFree)
	{
		fn_NetSim_Packet_FreeMacProtocolData(pstruPacket);
		pstruPacket->pstruMacData->Packet_MACProtocol = NULL;
	}
	pstruPacket->pstruMacData->nMACProtocol = pstruEventDetails->nProtocolId;
	pstruPacket->pstruMacData->dStartTime = ldEventTime;
	//Update the event details
	pstruEventDetails->dEventTime = ldEventTime;
	pstruEventDetails->nEventType = PHYSICAL_OUT_EVENT;
	pstruEventDetails->pPacket = pstruPacket;
	pstruEventDetails->nPacketId = pstruPacket->nPacketId;
	if (pstruPacket->pstruAppData)
	{
		pstruEventDetails->nApplicationId = pstruPacket->pstruAppData->nApplicationId;
		pstruEventDetails->nSegmentId = pstruPacket->pstruAppData->nSegmentId;
	}
	else
	{
		pstruEventDetails->nApplicationId = 0;
		pstruEventDetails->nSegmentId = 0;
	}
	//Add physical out event
	fnpAddEvent(pstruEventDetails);
	return 0;
}

int P2P_MacIn_Handler()
{
	NETSIM_ID d = pstruEventDetails->nDeviceId;
	NETSIM_ID in = pstruEventDetails->nInterfaceId;
	NetSim_PACKET* packet = pstruEventDetails->pPacket;
	if (DEVICE_NWLAYER(d))
	{
		//Add packet to Network in
		//Prepare event details for network in
		pstruEventDetails->dEventTime = ldEventTime;
		pstruEventDetails->nEventType = NETWORK_IN_EVENT;
		pstruEventDetails->nProtocolId = fn_NetSim_Stack_GetNWProtocol(d);
		//Add network in event
		fnpAddEvent(pstruEventDetails);
	}
	else
	{
		int tejas = 0;
		//Broadcast to all other interface
		NETSIM_ID i;
		for (i = 0; i < DEVICE(d)->nNumOfInterface; i++)
		{
			if (i + 1 == in)
				continue;

			NetSim_BUFFER* buf = DEVICE_ACCESSBUFFER(d, i + 1);
			//Check for buffer
			if (!fn_NetSim_GetBufferStatus(buf))
			{
				//Prepare event details for mac out
				pstruEventDetails->dEventTime = ldEventTime;
				pstruEventDetails->nEventType = MAC_OUT_EVENT;
				pstruEventDetails->nInterfaceId = i + 1;
				pstruEventDetails->pPacket = NULL;
				pstruEventDetails->nProtocolId = fn_NetSim_Stack_GetMacProtocol(d, i + 1);
				//Add mac out event
				fnpAddEvent(pstruEventDetails);
			}
			//Add packet to mac buffer
			fn_NetSim_Packet_AddPacketToList(buf,
											 tejas == 0 ? packet : fn_NetSim_Packet_CopyPacket(packet), 0);
			tejas = 1;
		}
	}
	return 0;
}
