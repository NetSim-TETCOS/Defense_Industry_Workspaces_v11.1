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
#include "CSMACD.h"

bool isMediumIdle()
{
	NETSIM_ID i,j;
	for(i=0;i<NETWORK->nDeviceCount;i++)
	{
		for(j=0;j<DEVICE(i+1)->nNumOfInterface;j++)
		{
			if(DEVICE_MACLAYER(i+1,j+1))
			{
				PCSMACD_PHYVAR phy = CSMACD_PHY(i+1,j+1);
				if(phy && phy->link_state == LinkState_UP)
					return false;
			}
		}
	}
	return true;
}

void transmit_packet(NetSim_PACKET* packet)
{
	PCSMACD_PHYVAR phyt = CSMACD_PHY(packet->nTransmitterId,1);

	phyt->link_state = LinkState_UP;

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

void check_for_collision(NetSim_PACKET* p)
{
	NETSIM_ID i;
	for(i=0;i<NETWORK->nDeviceCount;i++)
	{
		PCSMACD_PHYVAR phy = CSMACD_PHY(i+1,1);

		if(i+1==p->nTransmitterId)
			continue;

		if(phy->link_state != LinkState_DOWN)
		{
			p->nPacketStatus = PacketStatus_Collided;
			phy->collision_count++;

			if(file_collision != NULL)
			{
				fprintf(file_collision,"%d\t%d\t%f\n",i+1,phy->collision_count,pstruEventDetails->dEventTime);
			}
			break;
		}
	}
}

void reinit_phy(NETSIM_ID id)
{
	PCSMACD_PHYVAR phy = CSMACD_PHY(id,1);
	phy->link_state = LinkState_DOWN;

	if(isMediumIdle())
	{
		NETSIM_ID i;
		for(i=0;i<NETWORK->nDeviceCount;i++)
		{
			if(DEVICE_TYPE(i+1)==NODE)
			{
				pstruEventDetails->nDeviceId = i+1;
				pstruEventDetails->dPacketSize = 0;
				pstruEventDetails->nApplicationId = 0;
				pstruEventDetails->nSegmentId = 0;
				pstruEventDetails->nDeviceType = DEVICE_TYPE(i+1);
				pstruEventDetails->nEventType = MAC_OUT_EVENT;
				pstruEventDetails->nInterfaceId = 1;
				pstruEventDetails->nProtocolId = MAC_PROTOCOL_CSMACD;
				pstruEventDetails->pPacket=NULL;
				fnpAddEvent(pstruEventDetails);
			}
		}
	}
}

void fn_NetSim_CSMACD_PhyOut()
{

	double dTxTime;
	double* t = &DEVICE_PHYLAYER(pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId)->dLastPacketEndTime;
	NetSim_PACKET* packet= pstruEventDetails->pPacket;
	packet->pstruPhyData->dArrivalTime=pstruEventDetails->dEventTime;
	packet->pstruPhyData->dPayload=packet->pstruMacData->dPacketSize;
	packet->pstruPhyData->dOverhead=CSMACD_PHY_OVERHEAD;
	packet->pstruPhyData->dPacketSize=packet->pstruPhyData->dOverhead+packet->pstruPhyData->dPayload;
	packet->pstruPhyData->dStartTime=pstruEventDetails->dEventTime;

	if(*t<packet->pstruPhyData->dStartTime)
		*t=packet->pstruPhyData->dStartTime;
	else
		packet->pstruPhyData->dStartTime=*t;

	dTxTime=calulate_transmission_time(packet);

	CSMACD_CURRMAC->macState=Idle;


	packet->pstruPhyData->dEndTime=packet->pstruPhyData->dStartTime+dTxTime;
	*t = packet->pstruPhyData->dEndTime;

	if(packet->nReceiverId)
	{
		transmit_packet(packet);
		check_for_collision(packet);
	}
	else
	{
		//Broadcast packet
		NETSIM_ID loop;
		for(loop=0;loop<NETWORK->nDeviceCount;loop++)
		{
			if(DEVICE_TYPE(loop+1) != HUB && loop+1!=packet->nTransmitterId)
			{
				NetSim_PACKET* p = fn_NetSim_Packet_CopyPacket(packet);
				//Add physical in event
				p->nReceiverId=loop+1;
				transmit_packet(p);
				check_for_collision(p);
			}
		}
		fn_NetSim_Packet_FreePacket(packet);
	}
}

void fn_NetSim_CSMACD_PhyIn()
{
	double ber;
	NETSIM_ID tx = pstruEventDetails->pPacket->nTransmitterId;

	check_for_collision(pstruEventDetails->pPacket);

	if(pstruEventDetails->pPacket->nPacketStatus == PacketStatus_Collided)
		goto RET_PHYIN;

	fn_NetSim_Packet_FreePacket(CSMACD_MAC(tx,1)->currentPacket);
	CSMACD_MAC(tx,1)->currentPacket=NULL;
	CSMACD_MAC(tx,1)->nRetryCount = 0;

	ber = DEVICE_PHYLAYER(tx,1)->pstruNetSimLinks->puniMedProp.pstruWiredLink.dErrorRateUp;

	pstruEventDetails->pPacket->nPacketStatus = 
		fn_NetSim_Packet_DecideError(ber,
		pstruEventDetails->pPacket->pstruPhyData->dPacketSize);

	if(pstruEventDetails->pPacket->nPacketStatus == PacketStatus_Error)
		goto RET_PHYIN;

	pstruEventDetails->nEventType = MAC_IN_EVENT;
	fnpAddEvent(pstruEventDetails);

RET_PHYIN:
	fn_NetSim_WritePacketTrace(pstruEventDetails->pPacket);
	fn_NetSim_Metrics_Add(pstruEventDetails->pPacket);

	if(pstruEventDetails->pPacket->nPacketStatus != PacketStatus_NoError)
		fn_NetSim_Packet_FreePacket(pstruEventDetails->pPacket);

	reinit_phy(tx);;
}
