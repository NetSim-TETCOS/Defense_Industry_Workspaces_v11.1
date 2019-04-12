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


void transmit_packet(NetSim_PACKET* packet)
{
	//Add physical in event
	pstruEventDetails->dEventTime=packet->pstruPhyData->dEndTime;
	packet->pstruPhyData->dEndTime = pstruEventDetails->dEventTime;
	pstruEventDetails->dPacketSize=packet->pstruPhyData->dPacketSize;
	pstruEventDetails->nDeviceId=packet->nReceiverId;
	pstruEventDetails->nDeviceType=DEVICE_TYPE(packet->nReceiverId);
	pstruEventDetails->nEventType=PHYSICAL_IN_EVENT;
	pstruEventDetails->nInterfaceId=1;
	pstruEventDetails->pPacket=packet;
	fnpAddEvent(pstruEventDetails);
}

double calulate_transmission_time(NetSim_PACKET* packet)
{
	double size=fnGetPacketSize(packet);
	NETSIM_ID source=packet->nTransmitterId;
	double dDataRate=DEVICE_PHYLAYER(source,1)->pstruNetSimLinks->puniMedProp.pstruWiredLink.dDataRateUp;
	double dTxTime= size*8.0/dDataRate;
	return dTxTime;
}

void fn_NetSim_Token_PhyOut()
{
	
	double dTxTime;
	double* t = &DEVICE_PHYLAYER(pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId)->dLastPacketEndTime;
	NetSim_PACKET* packet= pstruEventDetails->pPacket;
	packet->pstruPhyData->dArrivalTime=pstruEventDetails->dEventTime;
	packet->pstruPhyData->dPayload=packet->pstruMacData->dPacketSize;
	packet->pstruPhyData->dOverhead=Token_PHY_OVERHEAD;
	packet->pstruPhyData->dPacketSize=packet->pstruPhyData->dOverhead+packet->pstruPhyData->dPayload;
	packet->pstruPhyData->dStartTime=pstruEventDetails->dEventTime;

	if(*t<packet->pstruPhyData->dStartTime)
		*t=packet->pstruPhyData->dStartTime;
	else
		packet->pstruPhyData->dStartTime=*t;

	dTxTime=calulate_transmission_time(packet);


	packet->pstruPhyData->dEndTime=packet->pstruPhyData->dStartTime+dTxTime;
	*t = packet->pstruPhyData->dEndTime;
	
	transmit_packet(packet);
}

void fn_NetSim_Token_PhyIn()
{
	double ber;
	NETSIM_ID tx = pstruEventDetails->pPacket->nTransmitterId;

	ber = DEVICE_PHYLAYER(tx,1)->pstruNetSimLinks->puniMedProp.pstruWiredLink.dErrorRateUp;

	pstruEventDetails->pPacket->nPacketStatus = 
		fn_NetSim_Packet_DecideError(ber,
		pstruEventDetails->pPacket->pstruPhyData->dPacketSize);

	pstruEventDetails->nEventType = MAC_IN_EVENT;
	fnpAddEvent(pstruEventDetails);

	fn_NetSim_WritePacketTrace(pstruEventDetails->pPacket);
	fn_NetSim_Metrics_Add(pstruEventDetails->pPacket);
}