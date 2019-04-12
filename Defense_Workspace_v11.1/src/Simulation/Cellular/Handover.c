
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
#include "GSM.h"
#include "Cellular.h"
#include "../Application/Application.h"
/** 
	As the mobile stations moves out of one cell to the next, it must be possible to hand the call over 
	from the base station of the first cell, to that of the next with no discernable disruption to the call.
	It is necessary to ensure it can be performed reliably and without disruption to any calls. 
	Failure for it to perform reliably can result in dropped calls. 
	When the handover occurs it is necessary to re-route the call to the relevant base station along with 
	changing the communication between the mobile and the base station to a new channel. All of this needs 
	to be undertaken without any noticeable interruption to the call. 
*/
int fn_NetSim_Cellular_HandoverCall(NETSIM_ID nMSId,NETSIM_ID nMSInterface,double time)
{
	Cellular_MS_MAC* MSMac=DEVICE_MACVAR(nMSId,nMSInterface);
	//Send channel request to new BTS
	NetSim_PACKET* request;
	Cellular_PACKET* gsmPacket=calloc(1,sizeof* gsmPacket);
	Cellular_CHANNEL_REQUEST* requestData=calloc(1,sizeof* requestData);
	unsigned int nProtocol=DEVICE_MACLAYER(nMSId,nMSInterface)->nMacProtocolId;
	if(MSMac->pstruAllocatedChannel==NULL)
		return 0;
	MSMac->nMSStatusFlag=Status_ChannelRequestedForHandover;
	request=fn_NetSim_Cellular_createPacket(pstruEventDetails->dEventTime,
		CELLULAR_PACKET_TYPE(nProtocol,PacketType_ChannelRequestForHandover),
		nMSId,
		MSMac->nNewBTS,
		1,
		nProtocol);
	request->pstruMacData->Packet_MACProtocol=gsmPacket;
	gsmPacket->controlData=requestData;
	requestData->nApplicationId=MSMac->nApplicationId;
	requestData->nMSId=nMSId;
	requestData->nDestId=MSMac->pstruAllocatedChannel->nDestId;
	requestData->nRequestType=PacketType_ChannelRequestForHandover;
	//Add packet to physical out
	pstruEventDetails->nApplicationId=MSMac->nApplicationId;
	pstruEventDetails->nDeviceId=nMSId;
	pstruEventDetails->nDeviceType=MOBILESTATION;
	pstruEventDetails->nInterfaceId=1;
	pstruEventDetails->dPacketSize=1;
	pstruEventDetails->nEventType=PHYSICAL_OUT_EVENT;
	pstruEventDetails->nProtocolId=nProtocol;
	pstruEventDetails->nPacketId=0;
	pstruEventDetails->pPacket=request;
	fnpAddEvent(pstruEventDetails);
	MSMac->MSMetrics.nHandoverRequest++;
	MSMac->MSMetrics.nChannelRequestSent++;
	return 1;
}
/** This function is called as a response for handover */
int fn_NetSim_Cellular_ChannelResponseForHandover()
{
	Cellular_MS_MAC* MSMac=DEVICE_MACVAR(pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId);
	Cellular_PACKET* gsmPacket=pstruEventDetails->pPacket->pstruMacData->Packet_MACProtocol;
	Cellular_CHANNEL_RESPONSE* response=gsmPacket->controlData;
	if(response->nAllocationFlag==1)
	{
		//Release old channel
		MSMac->pstruAllocatedChannel->nAllocationFlag=0;
		MSMac->pstruAllocatedChannel->nApplicationId=0;
		MSMac->pstruAllocatedChannel->nDestId=0;
		MSMac->pstruAllocatedChannel->nMSId=0;

		MSMac->pstruAllocatedChannel=response->channel;
		MSMac->nMSStatusFlag=Status_CallInProgress;
		fn_NetSim_Cellular_MoveMS(pstruEventDetails->nDeviceId,MSMac->nNewBTS);
	}
	else
	{
		NetSim_PACKET* packet=fn_NetSim_Cellular_createPacket(pstruEventDetails->dEventTime,
			CELLULAR_PACKET_TYPE(pstruEventDetails->nProtocolId,PacketType_DropCall),
			pstruEventDetails->nDeviceId,
			MSMac->pstruAllocatedChannel->nDestId,
			1,pstruEventDetails->nProtocolId);
		Cellular_PACKET* gsmPacket=calloc(1,sizeof* gsmPacket);
		gsmPacket->nApplicationId=MSMac->pstruAllocatedChannel->nApplicationId;
		gsmPacket->nTimeSlot=MSMac->pstruAllocatedChannel->nTimeSlot;
		packet->pstruMacData->Packet_MACProtocol=gsmPacket;
		pstruEventDetails->dPacketSize=1;
		pstruEventDetails->nEventType=PHYSICAL_OUT_EVENT;
		pstruEventDetails->pPacket=packet;
		fnpAddEvent(pstruEventDetails);
	}
	return 1;
}
/** This function is called to drop a call */
int fn_NetSim_Cellular_DropCall()
{
	unsigned int nProtocol=pstruEventDetails->nProtocolId;
	unsigned int nSubevent=pstruEventDetails->nSubEventType;
	Cellular_MS_MAC* MSMac=DEVICE_MACVAR(pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId);
	APP_INFO* appInfo=((APP_INFO**)NETWORK->appInfo)[MSMac->nApplicationId-1];
	MSMac->nMSStatusFlag=Status_IDLE;
	if(MSMac->pstruAllocatedChannel==NULL)
		return 1;
	//Drop call
	if(MSMac->nSourceFlag==1)
	{
		((APP_CALL_INFO*)appInfo->appData)->fn_BlockCall(appInfo,
		pstruEventDetails->nDeviceId,
		MSMac->pstruAllocatedChannel->nDestId,
		pstruEventDetails->dEventTime);
		pstruEventDetails->nProtocolId=nProtocol;
	}
	while(MSMac->pstruPacketList && MSMac->pstruPacketList[MSMac->nApplicationId][MSMac->pstruAllocatedChannel->nMSId-1][MSMac->pstruAllocatedChannel->nDestId-1])
	{
		NetSim_PACKET* packet=MSMac->pstruPacketList[MSMac->nApplicationId][MSMac->pstruAllocatedChannel->nMSId-1][MSMac->pstruAllocatedChannel->nDestId-1];
		MSMac->pstruPacketList[MSMac->nApplicationId][MSMac->pstruAllocatedChannel->nMSId-1][MSMac->pstruAllocatedChannel->nDestId-1]=packet->pstruNextPacket;
		fn_NetSim_Packet_FreePacket(packet);
	}
	while(MSMac->receivedPacketList && MSMac->receivedPacketList[MSMac->nApplicationId][MSMac->pstruAllocatedChannel->nMSId-1][MSMac->pstruAllocatedChannel->nDestId-1])
	{
		NetSim_PACKET* packet=MSMac->receivedPacketList[MSMac->nApplicationId][MSMac->pstruAllocatedChannel->nMSId-1][MSMac->pstruAllocatedChannel->nDestId-1];
		MSMac->receivedPacketList[MSMac->nApplicationId][MSMac->pstruAllocatedChannel->nMSId-1][MSMac->pstruAllocatedChannel->nDestId-1]=packet->pstruNextPacket;
		fn_NetSim_Packet_FreePacket(packet);
	}
	//Release old channel
	MSMac->pstruAllocatedChannel->nAllocationFlag=0;
	MSMac->pstruAllocatedChannel->nApplicationId=0;
	MSMac->pstruAllocatedChannel->nDestId=0;
	MSMac->pstruAllocatedChannel->nMSId=0;
	//Drop call
	MSMac->pstruAllocatedChannel=NULL;
	MSMac->nApplicationId=0;
	MSMac->nSourceFlag=0;
	if(nSubevent==CELLULAR_SUBEVENT(MAC_PROTOCOL_GSM,Subevent_DropCall) ||
		nSubevent==CELLULAR_SUBEVENT(MAC_PROTOCOL_CDMA,Subevent_DropCall))
	{
		fn_NetSim_Cellular_MoveMS(pstruEventDetails->nDeviceId,MSMac->nNewBTS);
		MSMac->MSMetrics.nCallDropeed++;
	}
	return 1;
}