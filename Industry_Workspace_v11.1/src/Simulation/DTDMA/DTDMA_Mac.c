/************************************************************************************
 * Copyright (C) 2015                                                               *
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
#include "DTDMA_enum.h"
#include "DTDMA.h"

int fn_NetSim_DTDMA_SendToPhy()
{
	NetSim_PACKET* packet=get_packet_from_buffer(pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId,0);
	if(!packet)
		return 1;//no packet in buffer

	fn_NetSim_FormDTDMABlock();
	return 0;
}
int fn_NetSim_FormDTDMABlock()
{
	DTDMA_NODE_MAC* mac=DTDMA_MAC(pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId);
	
	double dSize=(mac->bitsPerSlot-mac->overheadPerSlot)/8.0;

	double dTime=pstruEventDetails->dEventTime;

	NetSim_PACKET* packet=get_packet_from_buffer(pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId,0);
	NetSim_PACKET* ctrl;

	NetSim_EVENTDETAILS pevent;

	if(!packet)
		return 0;

	ctrl = fn_NetSim_Packet_CreatePacket(MAC_LAYER);
	ctrl->dEventTime = dTime;
	ctrl->nControlDataType = DTDMA_FRAME;
	ctrl->nPacketType = PacketType_Control;
	strcpy(ctrl->szPacketType,"DTDMA_FRAME");
	ctrl->nReceiverId = packet->nReceiverId;
	ctrl->nSourceId = pstruEventDetails->nDeviceId;
	ctrl->nTransmitterId = packet->nTransmitterId;
	add_dest_to_packet(ctrl, packet->nReceiverId);

	ctrl->pstruMacData->dArrivalTime = dTime;
	ctrl->pstruMacData->dEndTime = dTime;
	ctrl->pstruMacData->dStartTime = dTime;
	ctrl->pstruMacData->dOverhead = mac->overheadPerSlot/8.0;
	ctrl->pstruMacData->dPayload = 0;
	ctrl->pstruMacData->dPacketSize = ctrl->pstruMacData->dPayload + ctrl->pstruMacData->dOverhead;
	ctrl->pstruMacData->nMACProtocol = MAC_PROTOCOL_DTDMA;
	ctrl->pstruMacData->szDestMac = MAC_COPY(packet->pstruMacData->szDestMac);
	ctrl->pstruMacData->szNextHopMac = MAC_COPY(packet->pstruMacData->szNextHopMac);
	ctrl->pstruMacData->szSourceMac = MAC_COPY(packet->pstruMacData->szSourceMac);

	memcpy(&pevent,pstruEventDetails,sizeof pevent);
	pevent.dPacketSize = ctrl->pstruMacData->dPacketSize;
	pevent.nEventType=PHYSICAL_OUT_EVENT;
	pevent.nSubEventType=0;
	pevent.pPacket=ctrl;
	fnpAddEvent(&pevent);

	while(dSize>0.0)
	{
		double size;
		NetSim_PACKET* p=get_packet_from_buffer(pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId,0);
		if (!p)
			break;

		update_receiver(p);
		
		if(p->nReceiverId != packet->nReceiverId)
			break;

		p=get_packet_from_buffer_with_size(pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId,dSize);
		size=fnGetPacketSize(p);

		if(size>dSize)
			break;
		else
			dSize-=size;
		
		p->pstruMacData->dStartTime=dTime;
		p->pstruMacData->dEndTime=dTime;

		//Add physical out event
		pstruEventDetails->dEventTime=dTime;
		pstruEventDetails->dPacketSize=p->pstruMacData->dPacketSize;
		if(p->pstruAppData)
		{
			pstruEventDetails->nApplicationId=p->pstruAppData->nApplicationId;
			pstruEventDetails->nSegmentId=p->pstruAppData->nSegmentId;
		}
		pstruEventDetails->nEventType=PHYSICAL_OUT_EVENT;
		pstruEventDetails->nPacketId=p->nPacketId;
		pstruEventDetails->nProtocolId=MAC_PROTOCOL_DTDMA;
		pstruEventDetails->nSubEventType=0;
		pstruEventDetails->pPacket=p;
		fnpAddEvent(pstruEventDetails);
	}
	return 0;
}
