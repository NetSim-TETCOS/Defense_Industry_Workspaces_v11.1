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
#include "../Application/Application.h"
/* Function prototype */
_declspec(dllexport) int fn_NetSim_GSM_Init_F(struct stru_NetSim_Network *NETWORK_Formal,
													NetSim_EVENTDETAILS *pstruEventDetails_Formal,
													char *pszAppPath_Formal,
													char *pszWritePath_Formal,
													int nVersion_Type,
													void **fnPointer);
_declspec(dllexport) int fn_NetSim_GSM_Configure_F(void** var);

_declspec(dllexport) int fn_NetSim_GSM_Metrics_F(char* szMetrics);





/** This function is used to initialize the GSM parameters in a network */
_declspec(dllexport) int fn_NetSim_GSM_Init(struct stru_NetSim_Network *NETWORK_Formal,
													NetSim_EVENTDETAILS *pstruEventDetails_Formal,
													char *pszAppPath_Formal,
													char *pszWritePath_Formal,
													int nVersion_Type,
													void **fnPointer)
{
	NETWORK=NETWORK_Formal;
	pstruEventDetails=pstruEventDetails_Formal;
	return fn_NetSim_GSM_Init_F(NETWORK_Formal,
		pstruEventDetails_Formal,
		pszAppPath_Formal,
		pszWritePath_Formal,
		nVersion_Type,
		fnPointer);
}
/** This function is called by the NetworkStack.dll, while configuring the Network for GSM protocol */
_declspec(dllexport) int fn_NetSim_GSM_Configure(void** var)
{
	return fn_NetSim_GSM_Configure_F(var);
}
/**	
This function is called by NetworkStack.dll, which inturn calls the 
Cellular run function which is present in Cellular.c
 */
_declspec (dllexport) int fn_NetSim_GSM_Run()
{
	return fn_NetSim_Cellular_Run();
}
/**
This function is called by NetworkStack.dll, while writing the evnt trace 
to get the sub event as a string.
*/
_declspec (dllexport) char *fn_NetSim_GSM_Trace(int nSubEvent)
{
	switch(nSubEvent%100)
	{
	case Subevent_DropCall:
		return "DropCall";
	case Subevent_TxNextBurst:
		return "TxNextBurst";
	default:
		return "GSM_UnknownEvent";
	}
}
/**
This function is called by NetworkStack.dll, to free the GSM protocol control packets.
*/
_declspec(dllexport) int fn_NetSim_GSM_FreePacket(NetSim_PACKET* pstruPacket)
{
	return fn_NetSim_Cellular_FreePacket(pstruPacket);
}
/**
This function is called by NetworkStack.dll, to copy the GSM protocol
related information to a new packet 
*/
_declspec(dllexport) int fn_NetSim_GSM_CopyPacket(NetSim_PACKET* pstruDestPacket,NetSim_PACKET* pstruSrcPacket)
{
	return fn_NetSim_Cellular_CopyPacket(pstruDestPacket,pstruSrcPacket);
}
/**
This function writes the GSM metrics in Metrics.txt	
*/
_declspec(dllexport) int fn_NetSim_GSM_Metrics(char* szMetrics)
{
	return fn_NetSim_GSM_Metrics_F(szMetrics);
}
/**
This function is used to configure the packet trace
*/
_declspec(dllexport) char* fn_NetSim_GSM_ConfigPacketTrace()
{
	return "";
}
/**
This function is used to write the packet trace																									
*/
_declspec(dllexport) int fn_NetSim_GSM_WritePacketTrace(NetSim_PACKET* pstruPacket, char** ppszTrace)
{
	return 1;
}
/**
This function is called by NetworkStack.dll, once simulation ends, to free the 
allocated memory 	
*/
_declspec(dllexport) int fn_NetSim_GSM_Finish()
{
	return 1;
}
/** This function is used to add a packet to the MAC buffer of the mobile station */
int fn_NetSim_Cellular_AddPacketToBuffer(NetSim_PACKET* packet,NETSIM_ID nDeviceId,NETSIM_ID nInterfaceId)
{
	Cellular_MS_MAC* MSMac=DEVICE_MACVAR(nDeviceId,nInterfaceId);
	fn_NetSim_Cellular_FormBurst(packet,MSMac);
	return 1;
}
/** This function is used to allocate the channels */
int fn_NetSim_Cellular_AllocateChannel(NetSim_EVENTDETAILS* pstruEventDetails,NetSim_PACKET* packet)
{
	APP_INFO** appInfo=(APP_INFO**)NETWORK->appInfo;
	Cellular_MS_MAC* MSMac=DEVICE_MACVAR(pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId);
	NETSIM_ID nApplicationId=packet->pstruAppData->nApplicationId;
	if(MSMac->nMSStatusFlag!=Status_IDLE && MSMac->nMSStatusFlag != Status_CallEnd)
	{
		if(MSMac->nApplicationId!=nApplicationId)
		{
			unsigned int nProtocol=pstruEventDetails->nProtocolId;
			NetSim_EVENTDETAILS pevent;
			APP_CALL_INFO* info=appInfo[nApplicationId-1]->appData;
			memcpy(&pevent,pstruEventDetails,sizeof pevent);
			//Notify application layer
			info->fn_BlockCall(appInfo[nApplicationId-1],
							   packet->nSourceId,
							   get_first_dest_from_packet(packet),
							   pstruEventDetails->dEventTime);
			pstruEventDetails->nProtocolId=nProtocol;
			//Delete the packet from access buffer
			fn_NetSim_Packet_FreePacket(packet);
			MSMac->MSMetrics.nCallBlocked++;
			memcpy(pstruEventDetails,&pevent,sizeof* pstruEventDetails);
		}
	}
	else
	{
		NetSim_PACKET* request;
		Cellular_PACKET* cellularPacket=calloc(1,sizeof* cellularPacket);
		Cellular_CHANNEL_REQUEST* requestData=calloc(1,sizeof* requestData);
		MSMac->nMSStatusFlag=Status_ChannelRequested;
		MSMac->nApplicationId=nApplicationId;
		MSMac->nSourceFlag=1;
		request=fn_NetSim_Cellular_createPacket(pstruEventDetails->dEventTime,
			CELLULAR_PACKET_TYPE(pstruEventDetails->nProtocolId,PacketType_ChannelRequest),
			pstruEventDetails->nDeviceId,
			MSMac->nBTSId,
			1,
			pstruEventDetails->nProtocolId);
		request->pstruMacData->Packet_MACProtocol=cellularPacket;
		cellularPacket->controlData=requestData;
		requestData->nApplicationId=nApplicationId;
		requestData->nMSId=pstruEventDetails->nDeviceId;
		requestData->nDestId= get_first_dest_from_packet(packet);
		requestData->nRequestType=PacketType_ChannelRequest;
		//Add packet to physical out
		pstruEventDetails->dPacketSize=1;
		pstruEventDetails->nEventType=PHYSICAL_OUT_EVENT;
		pstruEventDetails->nPacketId=0;
		pstruEventDetails->pPacket=request;
		fnpAddEvent(pstruEventDetails);
		fn_NetSim_Cellular_AddPacketToBuffer(packet,pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId);
		MSMac->MSMetrics.nChannelRequestSent++;
	}
	return 1;
}
/** This function is called whenever PHYSICAL OUT event is triggered at the base station */
int fn_NetSim_GSM_BS_PhyOut()
{
	Cellular_BS_MAC* MSMac=DEVICE_MACVAR(pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId);
	NetSim_PACKET* packet = pstruEventDetails->pPacket;
	Cellular_PACKET* gsmPacket=packet->pstruMacData->Packet_MACProtocol;
	unsigned int nTimeSlot=gsmPacket->nTimeSlot;
	double dStartTime;
	double dDataRate=DATA_RATE;
	packet->pstruPhyData->dArrivalTime=pstruEventDetails->dEventTime;
	dStartTime=fn_NetSim_GSM_GetPacketStartTime(pstruEventDetails->dEventTime,nTimeSlot);
	
	packet->pstruPhyData->dOverhead=0;
	packet->pstruPhyData->dPacketSize=packet->pstruMacData->dPacketSize;
	packet->pstruPhyData->dPayload=packet->pstruMacData->dPacketSize;
	packet->pstruPhyData->dStartTime=dStartTime;
	packet->pstruPhyData->dEndTime=dStartTime+fnGetPacketSize(packet)*8/dDataRate;
	pstruEventDetails->dEventTime=packet->pstruPhyData->dEndTime;
	pstruEventDetails->nDeviceId=packet->nReceiverId;
	pstruEventDetails->nDeviceType=DEVICE_TYPE(packet->nReceiverId);
	pstruEventDetails->nEventType=PHYSICAL_IN_EVENT;
	pstruEventDetails->nInterfaceId=1;
	fnpAddEvent(pstruEventDetails);
	//Call packet trace function
	fn_NetSim_WritePacketTrace(packet);
	fn_NetSim_Metrics_Add(packet);
	return 1;
}
/** This function is called at the end of a call */
int fn_NetSim_Cellular_SendCallend(NETSIM_ID nMSID,NETSIM_ID nMSInterface,NETSIM_ID nDestinationId,double time)
{
	Cellular_MS_MAC* MSMac=DEVICE_MACVAR(nMSID,nMSInterface);
	NETSIM_ID nApplicationId=MSMac->nApplicationId;
	NetSim_PACKET* packet=fn_NetSim_Cellular_createPacket(time,
		CELLULAR_PACKET_TYPE(pstruEventDetails->nProtocolId,PacketType_CallEnd),
		nMSID,
		nDestinationId,
		1,
		pstruEventDetails->nProtocolId);
	Cellular_PACKET* gsmPacket=calloc(1,sizeof* gsmPacket);
	gsmPacket->nApplicationId=nApplicationId;
	gsmPacket->nTimeSlot=MSMac->pstruAllocatedChannel->nTimeSlot;
	packet->pstruMacData->Packet_MACProtocol=gsmPacket;
	//Add physical out event
	pstruEventDetails->dEventTime=time;
	pstruEventDetails->dPacketSize=1;
	pstruEventDetails->nApplicationId=nApplicationId;
	pstruEventDetails->nDeviceId=nMSID;
	pstruEventDetails->nDeviceType=MOBILESTATION;
	pstruEventDetails->nEventType=PHYSICAL_OUT_EVENT;
	pstruEventDetails->nInterfaceId=nMSInterface;
	pstruEventDetails->nPacketId=0;
	pstruEventDetails->nProtocolId=pstruEventDetails->nProtocolId;
	pstruEventDetails->nSegmentId=0;
	pstruEventDetails->nSubEventType=0;
	pstruEventDetails->pPacket=packet;
	fnpAddEvent(pstruEventDetails);
	//Release the channel
	fn_NetSim_Cellular_MS_SendChannelRelease(MSMac->pstruAllocatedChannel,nMSID,nMSInterface,time+GSM_CARRIER_LENGTH);
	if(MSMac->nMSStatusFlag==Status_CallInProgress)
		MSMac->nMSStatusFlag=Status_IDLE;
	else
		MSMac->nMSStatusFlag=Status_CallEnd;
	return 1;
}