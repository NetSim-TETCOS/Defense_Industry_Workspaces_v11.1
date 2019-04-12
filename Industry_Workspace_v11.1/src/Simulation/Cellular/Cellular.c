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
/**	
This function is called by NetworkStack.dll, whenever the event gets triggered	
inside the NetworkStack.dll for CDMA or GSM protocols. This is the main function for GSM / CDMA.
It processes MAC_OUT,MAC_IN, PHYSICAL_OUT, PHYSICAL_IN and TIMER events for mobile station 
and base station.
 */
int fn_NetSim_Cellular_Run()
{
	switch(pstruEventDetails->nEventType)
	{
	case MAC_OUT_EVENT:
		{
			switch(pstruEventDetails->nDeviceType)
			{
			case MOBILESTATION:
				{
					NetSim_BUFFER* buffer=DEVICE_MAC_NW_INTERFACE(pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId)->pstruAccessBuffer;
					do
					{
						NetSim_PACKET* packet=fn_NetSim_Packet_GetPacketFromBuffer(buffer,1);
						NETSIM_ID nAppId=packet->pstruAppData->nApplicationId;
						if(isCellularChannelAllocated(pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId,nAppId))
							fn_NetSim_Cellular_AddPacketToBuffer(packet,pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId);
						else
						{
							((Cellular_MS_MAC*)DEVICE_MACVAR(pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId))->MSMetrics.nCallGenerated++;
							fn_NetSim_Cellular_AllocateChannel(pstruEventDetails,packet);
						}
					}while(fn_NetSim_GetBufferStatus(buffer));
				}
				break;
			}
		}
		break;
	case MAC_IN_EVENT:
		{
			switch(pstruEventDetails->nDeviceType)
			{
			case MOBILESTATION:
				{
					NetSim_PACKET* packet=pstruEventDetails->pPacket;
					switch(packet->nControlDataType)
					{
					case CELLULAR_PACKET_TYPE(MAC_PROTOCOL_CDMA,PacketType_ChannelUngranted):
					case CELLULAR_PACKET_TYPE(MAC_PROTOCOL_CDMA,PacketType_ChannelGranted):
					case CELLULAR_PACKET_TYPE(MAC_PROTOCOL_GSM,PacketType_ChannelUngranted):
					case CELLULAR_PACKET_TYPE(MAC_PROTOCOL_GSM,PacketType_ChannelGranted):
						fn_NetSim_Cellular_ChannelResponse(packet);
						fn_NetSim_Packet_FreePacket(packet);
						break;
					case CELLULAR_PACKET_TYPE(MAC_PROTOCOL_CDMA,PacketType_CallRequest):
					case CELLULAR_PACKET_TYPE(MAC_PROTOCOL_GSM,PacketType_CallRequest):
						fn_NetSim_Cellular_MS_ProcessCallRequest();
						break;
					case CELLULAR_PACKET_TYPE(MAC_PROTOCOL_GSM,PacketType_CallAccepted):
					case CELLULAR_PACKET_TYPE(MAC_PROTOCOL_GSM,PacketType_CallRejected):
					case CELLULAR_PACKET_TYPE(MAC_PROTOCOL_CDMA,PacketType_CallAccepted):
					case CELLULAR_PACKET_TYPE(MAC_PROTOCOL_CDMA,PacketType_CallRejected):
						fn_NetSim_Cellular_MS_ProcessCallResponse();
						break;
					case CELLULAR_PACKET_TYPE(MAC_PROTOCOL_CDMA,PacketType_CallEnd):
					case CELLULAR_PACKET_TYPE(MAC_PROTOCOL_GSM,PacketType_CallEnd):
						{
							Cellular_MS_MAC* MSMac=DEVICE_MACVAR(pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId);
							if(MSMac->pstruAllocatedChannel)
							fn_NetSim_Cellular_MS_SendChannelRelease(MSMac->pstruAllocatedChannel,pstruEventDetails->nDeviceId,
								pstruEventDetails->nInterfaceId,
								pstruEventDetails->dEventTime);
							MSMac->nApplicationId=0;
							MSMac->nSourceFlag=0;
							MSMac->nMSStatusFlag=Status_IDLE;
						}
						break;
					case CELLULAR_PACKET_TYPE(MAC_PROTOCOL_CDMA,PacketType_DropCall):
					case CELLULAR_PACKET_TYPE(MAC_PROTOCOL_GSM,PacketType_DropCall):
						fn_NetSim_Cellular_DropCall();
						break;
					default:
						fn_NetSim_Cellular_MS_ReassembleBurst();
						break;
					}
				}
				break;
			case BASESTATION:
				{
					if(NETWORK->ppstruDeviceList[pstruEventDetails->nDeviceId-1]->ppstruInterfaceList[pstruEventDetails->nInterfaceId-1]->pstruPhysicalLayer->nPhyMedium==PHY_MEDIUM_WIRELESS)
					{
						NetSim_PACKET* packet=pstruEventDetails->pPacket;
						switch(packet->nControlDataType)
						{
						case CELLULAR_PACKET_TYPE(MAC_PROTOCOL_CDMA,PacketType_ChannelRequestForHandover):
						case CELLULAR_PACKET_TYPE(MAC_PROTOCOL_CDMA,PacketType_ChannelRequest):
						case CELLULAR_PACKET_TYPE(MAC_PROTOCOL_CDMA,PacketType_ChannelRequestForIncoming):
						case CELLULAR_PACKET_TYPE(MAC_PROTOCOL_GSM,PacketType_ChannelRequestForHandover):
						case CELLULAR_PACKET_TYPE(MAC_PROTOCOL_GSM,PacketType_ChannelRequest):
						case CELLULAR_PACKET_TYPE(MAC_PROTOCOL_GSM,PacketType_ChannelRequestForIncoming):
							fn_NetSim_Cellular_allocateChannel(packet);
							break;
						case CELLULAR_PACKET_TYPE(MAC_PROTOCOL_CDMA,PacketType_ChannelRelease):
						case CELLULAR_PACKET_TYPE(MAC_PROTOCOL_GSM,PacketType_ChannelRelease):
							fn_NetSim_Cellular_BS_ReleaseChannel();
							break;
						default:
							fn_NetSim_Cellular_ForwardToMSC();
							break;
						}
					}
					else
					{
						if(DEVICE_MACLAYER(pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId)->nMacProtocolId == MAC_PROTOCOL_GSM)
						{
							int flag=0;
							fn_NetSim_Cellular_BS_AssignTimeSlot(pstruEventDetails->pPacket,pstruEventDetails->nDeviceId);
							if(pstruEventDetails->pPacket->pstruAppData && pstruEventDetails->pPacket->pstruAppData->nApplicationId)
								flag = isCellularChannelAllocated(get_first_dest_from_packet(pstruEventDetails->pPacket),1,pstruEventDetails->pPacket->pstruAppData->nApplicationId);
							if(flag ||	pstruEventDetails->pPacket->nControlDataType/100 == MAC_PROTOCOL_GSM)
							{
								pstruEventDetails->pPacket->nTransmitterId=pstruEventDetails->nDeviceId;
								pstruEventDetails->pPacket->nReceiverId= get_first_dest_from_packet(pstruEventDetails->pPacket);
								pstruEventDetails->nInterfaceId=1;
								pstruEventDetails->nEventType=PHYSICAL_OUT_EVENT;
								fnpAddEvent(pstruEventDetails);
							}
							else
							{
								//Drop the packet
								fn_NetSim_Packet_FreePacket(pstruEventDetails->pPacket);
							}
						}
						else
						{
							pstruEventDetails->pPacket->nTransmitterId=pstruEventDetails->nDeviceId;
							pstruEventDetails->pPacket->nReceiverId= get_first_dest_from_packet(pstruEventDetails->pPacket);
							pstruEventDetails->nInterfaceId=1;
							pstruEventDetails->nEventType=PHYSICAL_OUT_EVENT;
							fnpAddEvent(pstruEventDetails);
						}
					}
				}
				break;
			case MSC:
				{
					fn_NetSim_Cellular_Msc_ProcessPacket();
				}
				break;
			}
		}
		break;
	case PHYSICAL_OUT_EVENT:
		{
			switch(pstruEventDetails->nDeviceType)
			{
			case MOBILESTATION:
				{
					fn_NetSim_Cellular_MS_PhyOut();
				}
				break;
			case BASESTATION:
				if(NETWORK->ppstruDeviceList[pstruEventDetails->nDeviceId-1]->ppstruInterfaceList[pstruEventDetails->nInterfaceId-1]->pstruPhysicalLayer->nPhyMedium==PHY_MEDIUM_WIRELESS)
					fn_NetSim_GSM_BS_PhyOut();
				else if(NETWORK->ppstruDeviceList[pstruEventDetails->nDeviceId-1]->ppstruInterfaceList[pstruEventDetails->nInterfaceId-1]->pstruPhysicalLayer->nPhyMedium==PHY_MEDIUM_WIRED)
					fn_NetSim_Cellular_TransmitOnwireline();
				else
				{
					fnNetSimError("Unknown phy medium %d for device %d Interface %d",NETWORK->ppstruDeviceList[pstruEventDetails->nDeviceId-1]->ppstruInterfaceList[pstruEventDetails->nInterfaceId-1]->pstruPhysicalLayer->nPhyMedium,pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId);
				}
				break;
			case MSC:
				fn_NetSim_Cellular_TransmitOnwireline();
				break;
			}
		}
		break;
	case PHYSICAL_IN_EVENT:
		{
			pstruEventDetails->nEventType=MAC_IN_EVENT;
			fnpAddEvent(pstruEventDetails);
		}
		break;
	case TIMER_EVENT:
		{
			switch(pstruEventDetails->nSubEventType)
			{
			case CELLULAR_SUBEVENT(MAC_PROTOCOL_CDMA,Subevent_DropCall):
			case CELLULAR_SUBEVENT(MAC_PROTOCOL_GSM,Subevent_DropCall):
				fn_NetSim_Cellular_DropCall();
				break;
			case CELLULAR_SUBEVENT(MAC_PROTOCOL_GSM,Subevent_TxNextBurst):
			case CELLULAR_SUBEVENT(MAC_PROTOCOL_CDMA,Subevent_TxNextBurst):
				{
					NetSim_PACKET**** list=pstruEventDetails->szOtherDetails;
					list[pstruEventDetails->pPacket->pstruAppData->nApplicationId][pstruEventDetails->pPacket->nSourceId-1][get_first_dest_from_packet(pstruEventDetails->pPacket)-1]=list[pstruEventDetails->pPacket->pstruAppData->nApplicationId][pstruEventDetails->pPacket->nSourceId-1][get_first_dest_from_packet(pstruEventDetails->pPacket)-1]->pstruNextPacket;
					pstruEventDetails->nEventType=PHYSICAL_OUT_EVENT;
					pstruEventDetails->nSubEventType=0;
					pstruEventDetails->szOtherDetails=NULL;
					fnpAddEvent(pstruEventDetails);
				}
				break;
			default:
				fnNetSimError("Unknown timer event for cellular protocol");
				break;
			}
		}
		break;
	default:
		fnNetSimError("Unknown event type for cellular protocol");
		break;
	}
	return 1;
}
/**
This function is used to allocate the channels.
Channel allocation deals with the allocation of channels to cells in a cellular network. 
Once the channels are allocated, cells may then allow users within the cell to communicate 
via the available channels. 
*/
int fn_NetSim_Cellular_allocateChannel(NetSim_PACKET* packet)
{
	Cellular_CHANNEL_RESPONSE* response=calloc(1,sizeof* response);
	Cellular_PACKET* gsmPacket=packet->pstruMacData->Packet_MACProtocol;
	Cellular_CHANNEL_REQUEST* request=gsmPacket->controlData;
	Cellular_BS_MAC* BSMac=DEVICE_MACVAR(pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId);
	Cellular_CHANNEL* channel=BSMac->pstruChannelList;
	NetSim_PACKET* returnPacket;
	unsigned int nPacketType;
	while(channel)
	{
		if(channel->nAllocationFlag == 0 &&
			channel->nChannelType==ChannelType_TRAFFICCHANNEL)
		{
			channel->nAllocationFlag=1;
			channel->nApplicationId=request->nApplicationId;
			channel->nMSId=request->nMSId;
			channel->nDestId=request->nDestId;
			break;
		}
		channel=channel->pstru_NextChannel;
	}
	
	if(channel)
	{
		response->channel=channel;
		response->nAllocationFlag=1;
		nPacketType=CELLULAR_PACKET_TYPE(pstruEventDetails->nProtocolId,PacketType_ChannelGranted);
	}
	else
	{
		response->channel=calloc(1,sizeof* response->channel);
		response->channel->nApplicationId=request->nApplicationId;
		response->channel->nMSId=request->nMSId;
		response->channel->nDestId=request->nDestId;
		nPacketType=CELLULAR_PACKET_TYPE(pstruEventDetails->nProtocolId,PacketType_ChannelUngranted);
	}
	returnPacket=fn_NetSim_Cellular_createPacket(pstruEventDetails->dEventTime,
		nPacketType,
		pstruEventDetails->nDeviceId,
		request->nMSId,
		1,
		pstruEventDetails->nProtocolId);
	gsmPacket=calloc(1,sizeof* gsmPacket);
	gsmPacket->controlData=response;
	returnPacket->pstruMacData->Packet_MACProtocol=gsmPacket;
	pstruEventDetails->nEventType=PHYSICAL_OUT_EVENT;
	pstruEventDetails->pPacket=returnPacket;
	fnpAddEvent(pstruEventDetails);
	return 1;
}
/** 
This functio is called to perform the channel response functionality 
This function determines if the call is accepted or rejected based on channel availability
*/
int fn_NetSim_Cellular_ChannelResponse(NetSim_PACKET* packet)
{
	Cellular_MS_MAC* MSMac=DEVICE_MACVAR(pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId);
	Cellular_PACKET* gsmPacket=packet->pstruMacData->Packet_MACProtocol;
	Cellular_CHANNEL_RESPONSE* response=gsmPacket->controlData;
	if(response->nAllocationFlag==1 && MSMac->nMSStatusFlag==Status_ChannelRequested)
	{
		NetSim_PACKET* packet;
		Cellular_PACKET* gsmPacket;
		Cellular_CHANNEL* channel=response->channel;
		MSMac->pstruAllocatedChannel=channel;
		packet = fn_NetSim_Cellular_createPacket(pstruEventDetails->dEventTime,
			CELLULAR_PACKET_TYPE(pstruEventDetails->nProtocolId,PacketType_CallRequest),
			channel->nMSId,
			channel->nDestId,
			1,
			pstruEventDetails->nProtocolId);
		gsmPacket=calloc(1,sizeof* gsmPacket);
		gsmPacket->nApplicationId=channel->nApplicationId;
		packet->pstruMacData->Packet_MACProtocol=gsmPacket;
		pstruEventDetails->dPacketSize=fnGetPacketSize(packet);
		pstruEventDetails->nApplicationId=0;
		pstruEventDetails->nEventType=PHYSICAL_OUT_EVENT;
		pstruEventDetails->nPacketId=0;
		pstruEventDetails->nProtocolId=pstruEventDetails->nProtocolId;
		pstruEventDetails->nSegmentId=0;
		pstruEventDetails->pPacket=packet;
		fnpAddEvent(pstruEventDetails);
		MSMac->nMSStatusFlag=Status_CallRequested;
		MSMac->MSMetrics.nCallRequestSent++;
	}
	else if(response->nAllocationFlag==1 && MSMac->nMSStatusFlag==Status_ChannelRequestedForIncoming)
	{
		//Generate call accepted message
		NetSim_PACKET* packet;
		Cellular_PACKET* gsmPacket;
		Cellular_CHANNEL* channel=response->channel;
		MSMac->pstruAllocatedChannel=channel;
		packet = fn_NetSim_Cellular_createPacket(pstruEventDetails->dEventTime,
			CELLULAR_PACKET_TYPE(pstruEventDetails->nProtocolId,PacketType_CallAccepted),
			channel->nMSId,
			channel->nDestId,
			1,
			pstruEventDetails->nProtocolId);
		gsmPacket=calloc(1,sizeof* gsmPacket);
		gsmPacket->nApplicationId=channel->nApplicationId;
		gsmPacket->nTimeSlot=channel->nTimeSlot;
		packet->pstruMacData->Packet_MACProtocol=gsmPacket;
		pstruEventDetails->dPacketSize=fnGetPacketSize(packet);
		pstruEventDetails->nApplicationId=0;
		pstruEventDetails->nEventType=PHYSICAL_OUT_EVENT;
		pstruEventDetails->nPacketId=0;
		pstruEventDetails->nProtocolId=pstruEventDetails->nProtocolId;
		pstruEventDetails->nSegmentId=0;
		pstruEventDetails->pPacket=packet;
		fnpAddEvent(pstruEventDetails);
		MSMac->nMSStatusFlag=Status_CallInProgress;
		MSMac->MSMetrics.nCallAccepted++;
	}
	else if(MSMac->nMSStatusFlag==Status_ChannelRequestedForHandover)
	{
		fn_NetSim_Cellular_ChannelResponseForHandover();
	}
	else if(response->nAllocationFlag==0 && MSMac->nMSStatusFlag==Status_ChannelRequested)
	{
		unsigned int nProtocol=pstruEventDetails->nProtocolId;
		Cellular_CHANNEL* channel=response->channel;
		NetSim_PACKET* packet=MSMac->pstruPacketList[channel->nApplicationId][channel->nMSId-1][channel->nDestId-1];
		APP_INFO** appInfo=(APP_INFO**)NETWORK->appInfo;
		APP_CALL_INFO* info=appInfo[packet->pstruAppData->nApplicationId-1]->appData;
		info->fn_BlockCall(appInfo[packet->pstruAppData->nApplicationId-1],
						   packet->nSourceId,
						   get_first_dest_from_packet(packet),
						   pstruEventDetails->dEventTime);
		pstruEventDetails->nProtocolId=nProtocol;
		while(packet)
		{
			NetSim_PACKET* temp=packet->pstruNextPacket;
			fn_NetSim_Packet_FreePacket(packet);
			packet=temp;
		}
		MSMac->pstruPacketList[channel->nApplicationId][channel->nMSId-1][channel->nDestId-1]=NULL;
		MSMac->nMSStatusFlag=Status_IDLE;
		MSMac->nSourceFlag=0;
		MSMac->nApplicationId=0;
		MSMac->MSMetrics.nCallBlocked++;
	}
	else if(response->nAllocationFlag==0 && MSMac->nMSStatusFlag==Status_ChannelRequestedForIncoming)
	{
		//Generate call rejected message
		NetSim_PACKET* packet;
		Cellular_PACKET* gsmPacket;
		Cellular_CHANNEL* channel=response->channel;
		MSMac->pstruAllocatedChannel=NULL;
		packet = fn_NetSim_Cellular_createPacket(pstruEventDetails->dEventTime,
			CELLULAR_PACKET_TYPE(pstruEventDetails->nProtocolId,PacketType_CallRejected),
			channel->nMSId,
			channel->nDestId,
			1,
			pstruEventDetails->nProtocolId);
		gsmPacket=calloc(1,sizeof* gsmPacket);
		gsmPacket->nApplicationId=channel->nApplicationId;
		gsmPacket->nTimeSlot=0;
		packet->pstruMacData->Packet_MACProtocol=gsmPacket;
		pstruEventDetails->dPacketSize=fnGetPacketSize(packet);
		pstruEventDetails->nApplicationId=0;
		pstruEventDetails->nEventType=PHYSICAL_OUT_EVENT;
		pstruEventDetails->nPacketId=0;
		pstruEventDetails->nProtocolId=pstruEventDetails->nProtocolId;
		pstruEventDetails->nSegmentId=0;
		pstruEventDetails->pPacket=packet;
		fnpAddEvent(pstruEventDetails);
		MSMac->nMSStatusFlag=Status_IDLE;
		MSMac->nApplicationId=0;
		MSMac->MSMetrics.nCallRejected++;
	}
	else if(MSMac->nMSStatusFlag==Status_CallEnd)
	{
		//Free the channel
		Cellular_CHANNEL* channel=response->channel;
		channel->nAllocationFlag=0;
		channel->nApplicationId=0;
		channel->nDestId=0;
		channel->nMSId=0;
		MSMac->nMSStatusFlag=Status_IDLE;
	}
	else
	{
		//assert(false);//Unknown condition
	}
	return 1;
}
/** 
This functio is called to forward the packet to the MSC
*/
int fn_NetSim_Cellular_ForwardToMSC()
{
	pstruEventDetails->nInterfaceId=2;
	pstruEventDetails->nEventType=PHYSICAL_OUT_EVENT;
	fnpAddEvent(pstruEventDetails);
	return 0;
}
/**
This function is used to tranmit packet on wired link
*/

int fn_NetSim_Cellular_TransmitOnwireline()
{
	NETSIM_ID nConnectedDevice,nConnectedInterface,nLinkId;
	double dDataRate;
	NetSim_PACKET* packet=pstruEventDetails->pPacket;
	nLinkId=fn_NetSim_Stack_GetConnectedDevice(pstruEventDetails->nDeviceId,
		pstruEventDetails->nInterfaceId,
		&(nConnectedDevice),
		&(nConnectedInterface));
	dDataRate = NETWORK->ppstruNetSimLinks[nLinkId-1]->puniMedProp.pstruWiredLink.dDataRateDown;
	packet->pstruPhyData->dArrivalTime=pstruEventDetails->dEventTime;
	packet->pstruPhyData->dEndTime=pstruEventDetails->dEventTime+fnGetPacketSize(packet)*8/dDataRate;
	packet->pstruPhyData->dStartTime=pstruEventDetails->dEventTime;
	packet->nTransmitterId=pstruEventDetails->nDeviceId;
	packet->nReceiverId=nConnectedDevice;
	pstruEventDetails->dEventTime=packet->pstruPhyData->dEndTime;
	pstruEventDetails->nDeviceId=nConnectedDevice;
	pstruEventDetails->nDeviceType=DEVICE_TYPE(nConnectedDevice);
	pstruEventDetails->nEventType=PHYSICAL_IN_EVENT;
	pstruEventDetails->nInterfaceId=nConnectedInterface;
	fnpAddEvent(pstruEventDetails);
	//Call packet trace function
	fn_NetSim_WritePacketTrace(packet);
	fn_NetSim_Metrics_Add(packet);
	return 1;
}
/**
The VisitorLocationRegister(VLR) contains a copy of most of the data stored at the HomeLocationRegister(HLR). It is, however, 
temporary data which exists for only as long as the subscriber is “active” 
in the particular area covered by the VLR. The VLR provides a local database for 
the subscribers wherever they are physically located within a PLMN, this may or may not 
be the “home” system. This function eliminates the need for excessive and time-consuming 
references to the “home” HLR database.

*/
int fn_NetSim_Cellular_VLR(NETSIM_ID nMSCId,NETSIM_ID nId,NETSIM_ID* nBTSId,NETSIM_ID* nInterfaceId)
{
	DEVVAR_MSC* devVar=NETWORK->ppstruDeviceList[nMSCId-1]->deviceVar;
	if(devVar->VLRList[nId-1])
	{
		*nBTSId=devVar->VLRList[nId-1]->nBTSId;
		*nInterfaceId=devVar->VLRList[nId-1]->nInterfaceId;
	}
	else
	{
		*nBTSId=0;
		*nInterfaceId=0;
	}
	return 0;
}
/** This function is used to process the packet at the MSC */
int fn_NetSim_Cellular_Msc_ProcessPacket()
{
	NETSIM_ID nBTSId,nInterfaceId;
	NetSim_PACKET* packet=pstruEventDetails->pPacket;
	NETSIM_ID nSourceId=packet->nSourceId;
	NETSIM_ID nDestinationId= get_first_dest_from_packet(packet);
	//Contact VLR list
	fn_NetSim_Cellular_VLR(pstruEventDetails->nDeviceId,nDestinationId,&nBTSId,&nInterfaceId);
	if(nBTSId)
	{
		packet->nTransmitterId=pstruEventDetails->nDeviceId;
		packet->nReceiverId=nBTSId;
		pstruEventDetails->nInterfaceId=nInterfaceId;
		pstruEventDetails->nEventType=PHYSICAL_OUT_EVENT;
		fnpAddEvent(pstruEventDetails);
	}
	else
		assert(false);
	return 1;
}
/** This function is used to process the call request at the mobile station */
int fn_NetSim_Cellular_MS_ProcessCallRequest()
{
	Cellular_MS_MAC* MSMac=DEVICE_MACVAR(pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId);
	NetSim_PACKET* packet=pstruEventDetails->pPacket;
	NETSIM_ID nSouceId= get_first_dest_from_packet(packet);
	NETSIM_ID nDestId=packet->nSourceId;
	MSMac->MSMetrics.nCallRequestReceived++;
	if(MSMac->nMSStatusFlag == Status_IDLE || MSMac->nMSStatusFlag==Status_CallEnd)
	{
		NetSim_PACKET* request;
		Cellular_PACKET* gsmPacket=calloc(1,sizeof* gsmPacket);
		Cellular_CHANNEL_REQUEST* requestData=calloc(1,sizeof* requestData);
		MSMac->nMSStatusFlag=Status_ChannelRequested;
		MSMac->nApplicationId=((Cellular_PACKET*)packet->pstruMacData->Packet_MACProtocol)->nApplicationId;
		request=fn_NetSim_Cellular_createPacket(pstruEventDetails->dEventTime,
			CELLULAR_PACKET_TYPE(pstruEventDetails->nProtocolId,PacketType_ChannelRequestForIncoming),
			pstruEventDetails->nDeviceId,
			MSMac->nBTSId,
			1,
			pstruEventDetails->nProtocolId);
		request->pstruMacData->Packet_MACProtocol=gsmPacket;
		gsmPacket->controlData=requestData;
		requestData->nApplicationId=((Cellular_PACKET*)packet->pstruMacData->Packet_MACProtocol)->nApplicationId;;
		requestData->nMSId=pstruEventDetails->nDeviceId;
		requestData->nDestId=nDestId;
		requestData->nRequestType=PacketType_ChannelRequestForIncoming;
		//Add packet to physical out
		pstruEventDetails->dPacketSize=1;
		pstruEventDetails->nEventType=PHYSICAL_OUT_EVENT;
		pstruEventDetails->nPacketId=0;
		pstruEventDetails->pPacket=request;
		fnpAddEvent(pstruEventDetails);
		MSMac->nMSStatusFlag=Status_ChannelRequestedForIncoming;
		MSMac->MSMetrics.nChannelRequestSent++;
	}
	else
	{
		//Call rejected
		//Generate call rejected message
		NetSim_PACKET* response;
		Cellular_PACKET* gsmPacket;
		response = fn_NetSim_Cellular_createPacket(pstruEventDetails->dEventTime,
												   CELLULAR_PACKET_TYPE(pstruEventDetails->nProtocolId, PacketType_CallRejected),
												   get_first_dest_from_packet(packet),
												   packet->nSourceId,
												   1,
												   pstruEventDetails->nProtocolId);
		gsmPacket = calloc(1, sizeof* gsmPacket);
		gsmPacket->nApplicationId = ((Cellular_PACKET*)packet->pstruMacData->Packet_MACProtocol)->nApplicationId;
		gsmPacket->nTimeSlot=0;
		response->pstruMacData->Packet_MACProtocol=gsmPacket;
		pstruEventDetails->dPacketSize=fnGetPacketSize(packet);
		pstruEventDetails->nApplicationId=0;
		pstruEventDetails->nEventType=PHYSICAL_OUT_EVENT;
		pstruEventDetails->nPacketId=0;
		pstruEventDetails->nProtocolId=pstruEventDetails->nProtocolId;
		pstruEventDetails->nSegmentId=0;
		pstruEventDetails->pPacket=response;
		fnpAddEvent(pstruEventDetails);
		MSMac->MSMetrics.nCallRejected++;
	}
	return 1;
}
/** This function is used to process the call response at the mobile station */
int fn_NetSim_Cellular_MS_ProcessCallResponse()
{
	Cellular_MS_MAC* MSMac=DEVICE_MACVAR(pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId);
	switch(pstruEventDetails->pPacket->nControlDataType)
	{
	case CELLULAR_PACKET_TYPE(MAC_PROTOCOL_GSM,PacketType_CallAccepted):
	case CELLULAR_PACKET_TYPE(MAC_PROTOCOL_CDMA,PacketType_CallAccepted):
		{
			Cellular_CHANNEL* channel=MSMac->pstruAllocatedChannel;
			NetSim_PACKET* packet=MSMac->pstruPacketList[channel->nApplicationId][channel->nMSId-1][channel->nDestId-1];
			Cellular_PACKET* gsmPacket=packet->pstruMacData->Packet_MACProtocol;
			MSMac->pstruPacketList[channel->nApplicationId][channel->nMSId-1][channel->nDestId-1] = MSMac->pstruPacketList[channel->nApplicationId][channel->nMSId-1][channel->nDestId-1]->pstruNextPacket;
			MSMac->nMSStatusFlag=Status_CallInProgress;
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
			pstruEventDetails->dPacketSize=fnGetPacketSize(packet);
			pstruEventDetails->nApplicationId=packet->pstruAppData->nApplicationId;
			pstruEventDetails->nEventType=PHYSICAL_OUT_EVENT;
			pstruEventDetails->nPacketId=packet->nPacketId;
			pstruEventDetails->nProtocolId=pstruEventDetails->nProtocolId;
			pstruEventDetails->nSegmentId=packet->pstruAppData->nSegmentId;
			//Free old packet
			fn_NetSim_Packet_FreePacket(pstruEventDetails->pPacket);
			pstruEventDetails->pPacket=packet;
			fnpAddEvent(pstruEventDetails);
		}
		break;
	case CELLULAR_PACKET_TYPE(MAC_PROTOCOL_CDMA,PacketType_CallRejected):
	case CELLULAR_PACKET_TYPE(MAC_PROTOCOL_GSM,PacketType_CallRejected):
		{
			NetSim_PACKET* current=pstruEventDetails->pPacket;
			unsigned int nProtocol=pstruEventDetails->nProtocolId;
			NETSIM_ID nMSId = pstruEventDetails->nDeviceId;
			NETSIM_ID nMSInterface=pstruEventDetails->nInterfaceId;
			double time=pstruEventDetails->dEventTime;
			Cellular_CHANNEL* channel=MSMac->pstruAllocatedChannel;
			NetSim_PACKET* packet=MSMac->pstruPacketList[channel->nApplicationId][channel->nMSId-1][channel->nDestId-1];
			APP_INFO** appInfo=(APP_INFO**)NETWORK->appInfo;
			APP_CALL_INFO* info=appInfo[packet->pstruAppData->nApplicationId-1]->appData;
			info->fn_BlockCall(appInfo[packet->pstruAppData->nApplicationId-1],
							   packet->nSourceId,
							   get_first_dest_from_packet(packet),
							   pstruEventDetails->dEventTime);
			pstruEventDetails->nProtocolId=nProtocol;
			fn_NetSim_Cellular_MS_SendChannelRelease(channel,nMSId,nMSInterface,time);
			while(packet)
			{
				NetSim_PACKET* temp=packet->pstruNextPacket;
				fn_NetSim_Packet_FreePacket(packet);
				packet=temp;
			}
			MSMac->pstruPacketList[channel->nApplicationId][channel->nMSId-1][channel->nDestId-1]=NULL;
			MSMac->nMSStatusFlag=Status_IDLE;
			MSMac->MSMetrics.nCallBlocked++;
			fn_NetSim_Packet_FreePacket(current);
		}
		break;
	}
	return 1;
}
/** This function runs at base station to assign the time slots */
int fn_NetSim_Cellular_BS_AssignTimeSlot(NetSim_PACKET* packet,NETSIM_ID nBTSId)
{
	Cellular_PACKET* gsmPacket=packet->pstruMacData->Packet_MACProtocol;
	NETSIM_ID nApplicationId=gsmPacket->nApplicationId;
	gsmPacket->nTimeSlot=0;
	if(!nApplicationId)
	{
		assert(false);
	}
	else
	{
		Cellular_BS_MAC* BSMac=DEVICE_MACVAR(nBTSId,1);
		Cellular_CHANNEL* channel=BSMac->pstruChannelList;
		while(channel)
		{
			if(channel->nAllocationFlag==1 && channel->nApplicationId==nApplicationId
				&& channel->nMSId == get_first_dest_from_packet(packet))
			{
				gsmPacket->nTimeSlot=channel->nTimeSlot;
				break;
			}
			channel=channel->pstru_NextChannel;
		}
	}
	return 1;
}
/** This function is used by the mobile station to send the request for channel release to the base station */
int fn_NetSim_Cellular_MS_SendChannelRelease(Cellular_CHANNEL* channel,NETSIM_ID nMSId,NETSIM_ID nMSInterface,double time)
{
	Cellular_MS_MAC* MSMac=DEVICE_MACVAR(nMSId,nMSInterface);
	NetSim_PACKET* packet = fn_NetSim_Cellular_createPacket(time,
		CELLULAR_PACKET_TYPE(pstruEventDetails->nProtocolId,PacketType_ChannelRelease),
		channel->nMSId,
		MSMac->nBTSId,
		1,
		pstruEventDetails->nProtocolId);
	Cellular_PACKET* gsmPacket=calloc(1,sizeof* gsmPacket);
	gsmPacket->nApplicationId=channel->nApplicationId;
	gsmPacket->nTimeSlot=0;
	packet->pstruMacData->Packet_MACProtocol=gsmPacket;
	//Write physical out event
	pstruEventDetails->dEventTime=time;
	pstruEventDetails->dPacketSize=1;
	pstruEventDetails->nApplicationId=0;
	pstruEventDetails->nDeviceId=nMSId;
	pstruEventDetails->nDeviceType=DEVICE_TYPE(nMSId);
	pstruEventDetails->nEventType=PHYSICAL_OUT_EVENT;
	pstruEventDetails->nInterfaceId=nMSInterface;
	pstruEventDetails->nPacketId=0;
	pstruEventDetails->nProtocolId=pstruEventDetails->nProtocolId;
	pstruEventDetails->nSegmentId=0;
	pstruEventDetails->nSubEventType=0;
	pstruEventDetails->pPacket=packet;
	fnpAddEvent(pstruEventDetails);
	MSMac->pstruAllocatedChannel=NULL;
	return 1;
}
/** This funtion is used by the base station to release the assigned channels */
int fn_NetSim_Cellular_BS_ReleaseChannel()
{
	Cellular_BS_MAC* BSMac=DEVICE_MACVAR(pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId);
	Cellular_CHANNEL* channel=BSMac->pstruChannelList;
	NETSIM_ID nApplicationId=((Cellular_PACKET*)pstruEventDetails->pPacket->pstruMacData->Packet_MACProtocol)->nApplicationId;
	NETSIM_ID nMSId=pstruEventDetails->pPacket->nSourceId;
	while(channel)
	{
		if(channel->nAllocationFlag==1 && channel->nMSId==nMSId && channel->nApplicationId==nApplicationId)
		{
			channel->nAllocationFlag=0;
			channel->nApplicationId=0;
			channel->nDestId=0;
			channel->nMSId=0;
			break;
		}
		channel=channel->pstru_NextChannel;
	}
	return 1;
}
/** This funcion is called at the mobile station when the PHYSICAL_OUT event is triggered */
int fn_NetSim_Cellular_MS_PhyOut()
{
	NETSIM_ID nMSID=pstruEventDetails->nDeviceId;
	NETSIM_ID nMSInterface=pstruEventDetails->nInterfaceId;
	Cellular_MS_MAC* MSMac=DEVICE_MACVAR(pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId);
	NetSim_PACKET* packet = pstruEventDetails->pPacket;
	Cellular_PACKET* gsmPacket=packet->pstruMacData->Packet_MACProtocol;
	unsigned int nTimeSlot=gsmPacket->nTimeSlot;
	double dStartTime;
	double dDataRate=DATA_RATE;
	packet->pstruPhyData->dArrivalTime=pstruEventDetails->dEventTime;
	if(pstruEventDetails->nProtocolId==MAC_PROTOCOL_GSM)
		dStartTime=fn_NetSim_GSM_GetPacketStartTime(pstruEventDetails->dEventTime,nTimeSlot);
	else
		dStartTime=pstruEventDetails->dEventTime;
	packet->pstruPhyData->dOverhead=0;
	packet->pstruPhyData->dPacketSize=packet->pstruMacData->dPacketSize;
	packet->pstruPhyData->dPayload=packet->pstruMacData->dPacketSize;
	packet->pstruPhyData->dStartTime=dStartTime;
	packet->pstruPhyData->dEndTime=dStartTime+fnGetPacketSize(packet)*8/dDataRate;
	if(packet->nControlDataType == CELLULAR_PACKET_TYPE(MAC_PROTOCOL_GSM,PacketType_ChannelRequestForHandover) ||
		packet->nControlDataType == CELLULAR_PACKET_TYPE(MAC_PROTOCOL_CDMA,PacketType_ChannelRequestForHandover))
		packet->nReceiverId=MSMac->nNewBTS;
	else
		packet->nReceiverId=MSMac->nBTSId;
	packet->nTransmitterId=pstruEventDetails->nDeviceId;
	pstruEventDetails->dEventTime=packet->pstruPhyData->dEndTime;
	pstruEventDetails->nDeviceId=packet->nReceiverId;
	pstruEventDetails->nDeviceType=BASESTATION;
	pstruEventDetails->nEventType=PHYSICAL_IN_EVENT;
	pstruEventDetails->nInterfaceId=MSMac->nBTSInterface;
	fnpAddEvent(pstruEventDetails);
	DEVICE_PHYLAYER(nMSID,nMSInterface)->dLastPacketEndTime = pstruEventDetails->dEventTime;
	//Call packet trace function
	fn_NetSim_WritePacketTrace(packet);
	fn_NetSim_Metrics_Add(packet);
	//Check for next packet
	if(packet->nControlDataType==CELLULAR_PACKET_TYPE(pstruEventDetails->nProtocolId,PacketType_DropCall))
	{
		pstruEventDetails->dPacketSize=0;
		pstruEventDetails->nApplicationId=0;
		pstruEventDetails->nDeviceId=nMSID;
		pstruEventDetails->nDeviceType=MOBILESTATION;
		pstruEventDetails->nEventType=TIMER_EVENT;
		pstruEventDetails->nInterfaceId=nMSInterface;
		pstruEventDetails->nPacketId=0;
		pstruEventDetails->nSegmentId=0;
		pstruEventDetails->nSubEventType=CELLULAR_SUBEVENT(pstruEventDetails->nProtocolId,Subevent_DropCall);
		pstruEventDetails->pPacket=NULL;
		fnpAddEvent(pstruEventDetails);
	}
	else if(MSMac->pstruAllocatedChannel && 
		MSMac->pstruPacketList &&
		MSMac->pstruPacketList[pstruEventDetails->nApplicationId][packet->nSourceId-1][get_first_dest_from_packet(packet)-1])
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
		if(pstruEventDetails->nProtocolId==MAC_PROTOCOL_GSM)
			pstruEventDetails->dEventTime+=577*8;
		pstruEventDetails->dPacketSize=fnGetPacketSize(packet);
		pstruEventDetails->nApplicationId=packet->pstruAppData->nApplicationId;
		pstruEventDetails->nEventType=TIMER_EVENT;
		pstruEventDetails->nDeviceId=nMSID;
		pstruEventDetails->nInterfaceId=nMSInterface;
		pstruEventDetails->nDeviceType=MOBILESTATION;
		pstruEventDetails->nPacketId=packet->nPacketId;
		pstruEventDetails->nSegmentId=packet->pstruAppData->nSegmentId;
		pstruEventDetails->pPacket=packet;
		pstruEventDetails->nSubEventType=CELLULAR_SUBEVENT(pstruEventDetails->nProtocolId,Subevent_TxNextBurst);
		pstruEventDetails->szOtherDetails=MSMac->pstruPacketList;
		fnpAddEvent(pstruEventDetails);
	}
	else if(packet->pstruAppData && packet->pstruAppData->nAppEndFlag==1 && gsmPacket->isLast==1)
	{
		//Add call end event
		fn_NetSim_Cellular_SendCallend(nMSID,
									   nMSInterface,
									   get_first_dest_from_packet(packet),
									   pstruEventDetails->dEventTime);
	}
	return 1;
}

