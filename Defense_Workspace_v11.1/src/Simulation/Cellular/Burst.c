/************************************************************************************
 * Copyright (C) 2014                                                               *
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
#include "Cellular.h"
unsigned int nBurstId=0;
/**
Burst is used for the standard communications between the basestation and the mobile, and typically 
transfers the digitised voice data.
This function is used to form the burst.
*/
int fn_NetSim_Cellular_FormBurst(NetSim_PACKET* packet,Cellular_MS_MAC* MSMac)
{
	unsigned int flag=0;
	double dPacketSize;
	NetSim_PACKET* temp,*temp1;
	NETSIM_ID nApplicationId=packet->pstruAppData->nApplicationId;
	NETSIM_ID nSourceId=packet->nSourceId;
	NETSIM_ID nDestinationId = get_first_dest_from_packet(packet);
	if(MSMac->pstruPacketList==NULL)
	{
		NETSIM_ID i,j;
		MSMac->pstruPacketList=calloc(NETWORK->nApplicationCount+1,sizeof* MSMac->pstruPacketList);
		for(i=0;i<NETWORK->nApplicationCount+1;i++)
		{
			MSMac->pstruPacketList[i]=calloc(NETWORK->nDeviceCount,sizeof* MSMac->pstruPacketList[i]);
			for(j=0;j<NETWORK->nDeviceCount;j++)
			{
				MSMac->pstruPacketList[i][j]=calloc(NETWORK->nDeviceCount,sizeof* MSMac->pstruPacketList[i][j]);
			}
		}
	}
	if(pstruEventDetails->nProtocolId==MAC_PROTOCOL_GSM)
	{
		dPacketSize=packet->pstruNetworkData->dPacketSize;
		nBurstId++;
		temp1=MSMac->pstruPacketList[nApplicationId][nSourceId-1][nDestinationId-1];
		if(temp1==NULL)flag=1;
		while(temp1 && temp1->pstruNextPacket)
			temp1=temp1->pstruNextPacket;//move last
		while(dPacketSize>0)
		{
			Cellular_PACKET* gsmPacket=calloc(1,sizeof* gsmPacket);
			temp=fn_NetSim_Packet_CopyPacket(packet);
			temp->pstruMacData->dArrivalTime=packet->pstruNetworkData->dEndTime;
			if(dPacketSize>=GSM_PAYLOAD)
			{
				temp->pstruMacData->dPayload=GSM_PAYLOAD;
				temp->pstruAppData->dPayload=GSM_PAYLOAD;
				temp->pstruTransportData->dPayload=GSM_PAYLOAD;
				temp->pstruNetworkData->dPayload=GSM_PAYLOAD;
			}
			else
			{
				temp->pstruMacData->dPayload=dPacketSize;
				temp->pstruAppData->dPayload=dPacketSize;
				temp->pstruTransportData->dPayload=dPacketSize;
				temp->pstruNetworkData->dPayload=dPacketSize;
			}
			dPacketSize-=GSM_PAYLOAD;
			temp->pstruMacData->nMACProtocol=MAC_PROTOCOL_GSM;
			temp->pstruMacData->Packet_MACProtocol=gsmPacket;
			gsmPacket->nId=nBurstId;
			gsmPacket->originalPacket=packet;
			gsmPacket->nApplicationId=nApplicationId;
			if(dPacketSize<=0)
				gsmPacket->isLast=1;
			if(temp1)
			{
				temp1->pstruNextPacket=temp;
				temp1=temp;
			}
			else
			{
				temp1=temp;
				MSMac->pstruPacketList[nApplicationId][nSourceId-1][nDestinationId-1]=temp;
			}
		}
	}
	else if(pstruEventDetails->nProtocolId==MAC_PROTOCOL_CDMA)
	{
		Cellular_PACKET* gsmPacket=calloc(1,sizeof* gsmPacket);
		temp1=MSMac->pstruPacketList[nApplicationId][nSourceId-1][nDestinationId-1];
		if(temp1==NULL)flag=1;
		while(temp1 && temp1->pstruNextPacket)
			temp1=temp1->pstruNextPacket;//move last
		temp=packet;
		temp->pstruMacData->dArrivalTime=packet->pstruNetworkData->dEndTime;
		temp->pstruMacData->dPayload=packet->pstruNetworkData->dPacketSize;
		temp->pstruMacData->nMACProtocol=MAC_PROTOCOL_CDMA;
		temp->pstruMacData->Packet_MACProtocol=gsmPacket;
		gsmPacket->nId=1;
		gsmPacket->originalPacket=packet;
		gsmPacket->nApplicationId=nApplicationId;
		gsmPacket->isLast=1;
		if(temp1)
		{
			temp1->pstruNextPacket=temp;
			temp1=temp;
		}
		else
		{
			temp1=temp;
			MSMac->pstruPacketList[nApplicationId][nSourceId-1][nDestinationId-1]=temp;
		}
	}
	if(flag && MSMac->pstruAllocatedChannel)
	{
		//Transmit next packet
		Cellular_PACKET* gsmPacket;
		Cellular_CHANNEL* channel=MSMac->pstruAllocatedChannel;
		NetSim_PACKET* packet=MSMac->pstruPacketList[channel->nApplicationId][channel->nMSId-1][channel->nDestId-1];
		packet->pstruMacData->dEndTime=pstruEventDetails->dEventTime;
		if(pstruEventDetails->nProtocolId==MAC_PROTOCOL_GSM)
			packet->pstruMacData->dOverhead=GSM_OVERHEAD;
		else
			packet->pstruMacData->dOverhead=0;
		packet->pstruMacData->dPacketSize=packet->pstruMacData->dOverhead+packet->pstruMacData->dPayload;
		packet->pstruMacData->dStartTime=pstruEventDetails->dEventTime;
		gsmPacket=packet->pstruMacData->Packet_MACProtocol;
		gsmPacket->nTimeSlot=channel->nTimeSlot;
		//Write physical out event
		if(DEVICE_PHYLAYER(pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId)->dLastPacketEndTime>pstruEventDetails->dEventTime)
			pstruEventDetails->dEventTime=DEVICE_PHYLAYER(pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId)->dLastPacketEndTime;
		pstruEventDetails->dPacketSize=fnGetPacketSize(packet);
		pstruEventDetails->nApplicationId=packet->pstruAppData->nApplicationId;
		pstruEventDetails->nEventType=PHYSICAL_OUT_EVENT;
		pstruEventDetails->nPacketId=packet->nPacketId;
		pstruEventDetails->nSegmentId=packet->pstruAppData->nSegmentId;
		pstruEventDetails->pPacket=packet;
		fnpAddEvent(pstruEventDetails);
		MSMac->pstruPacketList[channel->nApplicationId][channel->nMSId-1][channel->nDestId-1]=MSMac->pstruPacketList[channel->nApplicationId][channel->nMSId-1][channel->nDestId-1]->pstruNextPacket;
	}
	return 1;
}
/**
This function is used to reassemble the burst.
*/
int fn_NetSim_Cellular_MS_ReassembleBurst()
{
	NetSim_PACKET* temp=pstruEventDetails->pPacket;
	Cellular_PACKET* gsmPacket=pstruEventDetails->pPacket->pstruMacData->Packet_MACProtocol;
	if(gsmPacket->isLast)
	{
		NetSim_PACKET* packet=gsmPacket->originalPacket;
		//add network in event
		pstruEventDetails->dPacketSize=fnGetPacketSize(packet);
		pstruEventDetails->nEventType=NETWORK_IN_EVENT;
		pstruEventDetails->nPacketId=packet->nPacketId;
		pstruEventDetails->nProtocolId=fn_NetSim_Stack_GetNWProtocol(pstruEventDetails->nDeviceId);
		pstruEventDetails->pPacket=packet;
		fnpAddEvent(pstruEventDetails);
	}
	if(temp->pstruMacData->nMACProtocol==MAC_PROTOCOL_GSM)
		fn_NetSim_Packet_FreePacket(temp);
	return 1;
}