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
* Author:    Siddharth Makadia                                                     *
*                                                                                  *
* ---------------------------------------------------------------------------------*/

#include "Application.h"
#include "CoAP.h"
#include "main.h"
#include "Stack.h"

#define COAP_REQUEST_SIZE 12
#define COAP_ACK_SIZE 8

#define COAP_ACT_REQUIRED_DEFAULT TRUE
#define COAP_MULTICAST_RESPONSE_DEFAULT TRUE
#define COAP_VARIABLE_RESPONSE_TIME_DEFAULT	5
#define COAP_PIGGYBACKED_TIME_DEFAULT 2000
#define COAP_ACK_RESPONSE_TIME_DEFAULT 1000

#define COAP_ACK_TIMEOUT_DEFAULT 2000		//in milliseconds
#define COAP_ACK_RANDOM_FACTOR_DEFAULT 1.5		
#define COAP_MAX_RETRANSMIT_DEFAULT 4		
#define COAP_NSTART_DEFAULT 1		
#define COAP_DEFAULT_LEISURE_DEFAULT 5		//in seconds
#define COAP_PROBING_RATE_DEFAULT 1		//in byte/second


int fn_NetSim_Application_ConfigureCOAPTraffic(APP_INFO* appInfo, void* xmlNetSimNode)
{
	char* szVal;
	APP_COAP_INFO* info = calloc(1, sizeof* info);
	void* xmlChild;
	appInfo->appData = info;
	xmlChild = fn_NetSim_xmlGetChildElement(xmlNetSimNode, "COAP_REQUEST_INTERARRIVAL_TIME", 0);
	if (xmlChild)
	{
		szVal = fn_NetSim_xmlConfig_GetVal(xmlChild, "DISTRIBUTION", 1);
		if (szVal)
			info->pageIATDistribution = fn_NetSim_Config_GetDistribution(szVal);
		free(szVal);
		szVal = fn_NetSim_xmlConfig_GetVal(xmlChild, "VALUE", 1);
		if (szVal)
			info->pageIAT = atof(szVal)*SECOND;
		free(szVal);
	}
	else
		return 0;
	xmlChild = fn_NetSim_xmlGetChildElement(xmlNetSimNode, "PAGE_PROPERTY", 0);
	if (xmlChild)
	{
		szVal = fn_NetSim_xmlConfig_GetVal(xmlChild, "PAGE_SIZE", 1);
		if (szVal)
			info->pageSize = atof(szVal);
		free(szVal);
		szVal = fn_NetSim_xmlConfig_GetVal(xmlChild, "PAGE_SIZE_DISTRIBUTION", 1);
		if (szVal)
			info->pageSizeDistribution = fn_NetSim_Config_GetDistribution(szVal);
		free(szVal);
	}

	xmlChild = fn_NetSim_xmlGetChildElement(xmlNetSimNode, "COAP_PROPERTY", 0);


	if (xmlChild) {
		getXmlVar(&info->ackRequired, ACT_REQUIRED, xmlChild, 1, _BOOL, COAP);
		getXmlVar(&info->multicastResponse, MULTICAST_RESPONSE, xmlChild, 1, _BOOL, COAP);
		getXmlVar(&info->variableResponseTime, VARIABLE_RESPONSE_TIME, xmlChild, 1, _INT, COAP);
		getXmlVar(&info->piggybackedTime, PIGGYBACKED_TIME, xmlChild, 1, _INT, COAP);
		getXmlVar(&info->ackResponseTime, ACK_RESPONSE_TIME, xmlChild, 1, _INT, COAP);
		getXmlVar(&info->ackTimeOut, ACK_TIMEOUT, xmlChild, 1, _INT, COAP);
		getXmlVar(&info->ackRandomFactor, ACK_RANDOM_FACTOR, xmlChild, 1, _DOUBLE, COAP);
		getXmlVar(&info->maxRetransmit, MAX_RETRANSMIT, xmlChild, 1, _INT, COAP);
		getXmlVar(&info->nstart, NSTART, xmlChild, 1, _INT, COAP);
		getXmlVar(&info->defaultLeisure, DEFAULT_LEISURE, xmlChild, 1, _INT, COAP);
		getXmlVar(&info->probingRate, PROBING_RATE, xmlChild, 1, _INT, COAP);

		COAP_Data* pCOAPData = calloc(1, sizeof* pCOAPData);
		info->pCOAPData = pCOAPData;

		info->pCOAPData->Response_Received = 0;
		info->pCOAPData->RequestACK_Received = 0;
		info->pCOAPData->Request_Received = 0;
		info->pCOAPData->ResponseACK_Received = 0;

		info->pCOAPData->_RETRANSMIT = 0;
	}
	return 1; 
}

void fn_NetSim_Application_COAP_AppIn(APP_INFO* pstruappinfo, NetSim_PACKET* pstruPacket) {

	COAP_Header* pCOAPHeader = pstruPacket->pstruAppData->Packet_AppProtocol;
	APP_COAP_INFO* info = pstruappinfo->appData; 
	COAP_Data* pCOAPData = info->pCOAPData;
	
	//check for request Duplicate detection
	if (pCOAPHeader->Code == REQUEST && pCOAPHeader->Type != ACKNOWLEDGEMENT) {
		if (pCOAPData->Request_messageID == pCOAPHeader->MessageID && pCOAPData->Request_tokenValue == pCOAPHeader->tokenvalue) {
			return;
		}
		else {
			pCOAPData->Request_messageID = pCOAPHeader->MessageID;
			pCOAPData->Request_tokenValue = pCOAPHeader->tokenvalue;
		}
	}
	//check for response Duplicate detection
	if (pCOAPHeader->Code == RESPONSE) {
		if (pCOAPData->Response_messageID == pCOAPHeader->MessageID && pCOAPData->Response_tokenValue == pCOAPHeader->tokenvalue) {
			return;
		} 
		else {
			pCOAPData->Response_messageID = pCOAPHeader->MessageID;
			pCOAPData->Response_tokenValue = pCOAPHeader->tokenvalue;
		}
	}
	
	//check of request
	if (pCOAPHeader->Code == REQUEST) {
		
		//check for ack and piggybacked condition
		if (pCOAPHeader->Type == CONFIRMABLE && info->variableResponseTime > info->piggybackedTime) {
			
			fn_NetSim_Application_COAP_Sent_ACK(pstruappinfo, pstruPacket);
			
		}
		//send response
		if (pCOAPHeader->Type != ACKNOWLEDGEMENT) {
		
				fn_NetSim_Application_COAP_ProcessRequest(pstruappinfo, pstruPacket);
			
		}
	}
	//check for Response
	if (pCOAPHeader->Code == RESPONSE) {
		//check for Response and send ACK
		if ((pCOAPHeader->Type == CONFIRMABLE || pCOAPHeader->Type == ACKNOWLEDGEMENT) && pCOAPHeader->Code == RESPONSE) {
			fn_NetSim_Application_COAP_Sent_ACK(pstruappinfo, pstruPacket);
			pstruEventDetails->dEventTime = pstruEventDetails->dEventTime - info->ackResponseTime*MILLISECOND;	
		}
		//Start the next timer event to sent the request packet again after receving the reponse and ACK 
		fn_NetSim_Application_COAP_Start(pstruappinfo, pstruEventDetails);

	}
	

	//check for Ack Packet
	if (pCOAPHeader->Type == ACKNOWLEDGEMENT) {
		if(!strcmp(pstruEventDetails->pPacket->szPacketType, "COAP_REQUEST") || !strcmp(pstruEventDetails->pPacket->szPacketType, "COAP_REQUEST_ACK") )
			info->pCOAPData->Response_Received = 1;
	
	}
	
	
}

int fn_NetSim_Application_COAP_Start(APP_INFO* appInfo, NetSim_EVENTDETAILS* pstruEventDetails) {
	fn_NetSim_Application_StartCOAPAPP(appInfo, pstruEventDetails->dEventTime);
	return 1;
}

int fn_NetSim_Application_StartCOAPAPP(APP_INFO* appInfo, double time) {

	if (appInfo->dEndTime <= time)
		return 0;

	fnCreatePort(appInfo);

	NETSIM_ID nSource = appInfo->sourceList[0];
	NETSIM_ID nDestination = appInfo->destList[0];

	//Create the socket buffer
	fnCreateSocketBuffer(appInfo);
	APP_COAP_INFO* info = appInfo->appData;

	if (appInfo->nTransmissionType == MULTICAST)
	{
		fnNetSimError("In NETSIM, COAP application can't be configured as multicast.\n"
					  "Stopping application...\n");
		return -1;
	}

	//Generate the app start event
	fn_NetSim_Application_COAP_Genrate_RequestPacket(appInfo, nSource, nDestination, time, "COAP_REQUEST");

	// Setting COAP application header 
	COAP_Header* pCOAPHeader = fn_NetSim_Application_COAP_GenrateHeader(info);

	pstruEventDetails->pPacket->pstruAppData->nApplicationProtocol = PROTOCOL_APPLICATION;
	pstruEventDetails->pPacket->pstruAppData->Packet_AppProtocol = pCOAPHeader;
	//fnpAddEvent(pstruEventDetails);
	

	fnpAddEvent(pstruEventDetails);
		
	// Create a new Event for Stop and Wait method
	

	return 0;
}

int fn_NetSim_Application_COAP_ProcessRequest(APP_INFO* pstruappinfo, NetSim_PACKET* pstruPacket)
{
	NETSIM_ID nSourceId = get_first_dest_from_packet(pstruPacket);
	NETSIM_ID nDestinationId = pstruPacket->nSourceId;
	APP_COAP_INFO* info = pstruappinfo->appData;
	if (info->multicastResponse == FALSE && pstruappinfo->nTransmissionType == MULTICAST)
	{
		return 0;
	}

	COAP_Header* pCOAPHeader = pstruEventDetails->pPacket->pstruAppData->Packet_AppProtocol;

	//int ackreqired = pCOAPHeader->Type;

	
	fn_NetSim_Application_COAP_Genrate_Packet(pstruappinfo, nSourceId, nDestinationId, 0.0, "COAP_RESPONSE");
	int messageID = pCOAPHeader->MessageID;
	int tokenValue = pCOAPHeader->tokenvalue;
	
	//genrate and modify COAP header
	COAP_Header* pCOAPHeaderNew = fn_NetSim_Application_COAP_GenrateHeader(info);
	//Piggybacked condition check 
	if (info->variableResponseTime < info->piggybackedTime  && pCOAPHeaderNew->Type == CONFIRMABLE) {
		pCOAPHeaderNew->Type = ACKNOWLEDGEMENT;
		pCOAPHeaderNew->Code = RESPONSE;
		pCOAPHeaderNew->MessageID = messageID;
		pCOAPHeaderNew->tokenvalue = tokenValue;	
	}
	else
	{
		//pCOAPHeaderNew->Type = CONFIRMABLE;
		pCOAPHeaderNew->Code = RESPONSE;
		pCOAPHeaderNew->MessageID = rand() % 65535;
		pCOAPHeaderNew->tokenvalue = tokenValue;
	}

	pstruEventDetails->pPacket->pstruAppData->nApplicationProtocol = PROTOCOL_APPLICATION;
	pstruEventDetails->pPacket->pstruAppData->Packet_AppProtocol = pCOAPHeaderNew;
	fnpAddEvent(pstruEventDetails);

	return 1;
}


int fn_NetSim_Application_COAP_Sent_ACK(APP_INFO* pstruappinfo, NetSim_PACKET* pstruPacket) {

	NETSIM_ID nSourceId = get_first_dest_from_packet(pstruPacket);
	NETSIM_ID nDestinationId = pstruPacket->nSourceId;
	APP_COAP_INFO* info = pstruappinfo->appData;
	
	COAP_Header* pCOAPHeader = pstruEventDetails->pPacket->pstruAppData->Packet_AppProtocol;
	int messageID = pCOAPHeader->MessageID;
	//genrate and modify COAP header
	COAP_Header* pCOAPHeaderNew = fn_NetSim_Application_COAP_GenrateHeader(info); 
	if (pCOAPHeader->Code == REQUEST) {
		fn_NetSim_Application_COAP_Genrate_Packet(pstruappinfo, nSourceId, nDestinationId,0.0, "COAP_REQUEST_ACK");
	}
	else {
		fn_NetSim_Application_COAP_Genrate_Packet(pstruappinfo, nSourceId, nDestinationId, 0.0, "COAP_RESPONSE_ACK");
	}
	
	pCOAPHeaderNew->Type = ACKNOWLEDGEMENT;
	pCOAPHeaderNew->Code = REQUEST;
	pCOAPHeaderNew->MessageID = messageID;
	pCOAPHeaderNew->tokenvalue = 0;

	pstruEventDetails->pPacket->pstruAppData->nApplicationProtocol = PROTOCOL_APPLICATION;
	pstruEventDetails->pPacket->pstruAppData->Packet_AppProtocol = pCOAPHeaderNew;

	fnpAddEvent(pstruEventDetails);
	return 1;
}


int fn_NetSim_Application_COAP_Genrate_RequestPacket(APP_INFO* appInfo, NETSIM_ID nSource, NETSIM_ID nDestination, double time, char* PacketType) {
	double arrivalTime; //interarrival time
	APP_COAP_INFO* info = appInfo->appData;
	
	do
	{
		fnDistribution(info->pageIATDistribution,
			&arrivalTime,
			&(NETWORK->ppstruDeviceList[nSource - 1]->ulSeed[0]),
			&(NETWORK->ppstruDeviceList[nSource - 1]->ulSeed[1]),
			&(info->pageIAT));
	} while (arrivalTime <= 0.0);

	pstruEventDetails->dEventTime = time + arrivalTime;
	pstruEventDetails->dPacketSize = COAP_REQUEST_SIZE;
	pstruEventDetails->nApplicationId = appInfo->id;
	pstruEventDetails->nDeviceId = nDestination;
	pstruEventDetails->nDeviceType = DEVICE_TYPE(nDestination);
	pstruEventDetails->nEventType = TIMER_EVENT;
	pstruEventDetails->nInterfaceId = 0;
	pstruEventDetails->nPacketId = 0;
	pstruEventDetails->nProtocolId = PROTOCOL_APPLICATION;
	pstruEventDetails->nSegmentId = 0;
	pstruEventDetails->nSubEventType = event_APP_START;
	pstruEventDetails->pPacket = fn_NetSim_Application_GeneratePacket(appInfo,
																		pstruEventDetails->dEventTime,
																		nDestination,
																		1,
																		&nSource,
																		0,
																		appInfo->nAppType,
																		appInfo->qos,
																		appInfo->destPort,
																		appInfo->sourcePort);
	pstruEventDetails->pPacket->nControlDataType = packet_COAP_REQUEST;
	pstruEventDetails->pPacket->nPacketType = PacketType_COAP;
	strcpy(pstruEventDetails->pPacket->szPacketType, PacketType);
	
	pstruEventDetails->szOtherDetails = appInfo;
	pstruEventDetails->pPacket->pstruAppData->nPacketFragment = Segment_Unfragment;
	return 1;
}

int fn_NetSim_Application_COAP_Genrate_Packet(APP_INFO* pstruappinfo, NETSIM_ID nSourceId, NETSIM_ID nDestinationId, double size, char* PacketType) {

	APP_COAP_INFO* info = pstruappinfo->appData;
	//COAP_Header* pCOAPHeader = pstruEventDetails->pPacket->pstruAppData->Packet_AppProtocol;
	if (!strcmp(PacketType, "COAP_RESPONSE")) {
		do
		{
			fnDistribution(info->pageSizeDistribution, &size,
				&(NETWORK->ppstruDeviceList[nSourceId - 1]->ulSeed[0]),
				&(NETWORK->ppstruDeviceList[nSourceId - 1]->ulSeed[1]),
				&(info->pageSize));
		} while (size <= 0.0);
		pstruEventDetails->dEventTime = pstruEventDetails->dEventTime + info->variableResponseTime*MILLISECOND;
	}
	else {
		double  ackSize = COAP_ACK_SIZE;
		do
		{

			fnDistribution(info->pageSizeDistribution, &size,
				&(NETWORK->ppstruDeviceList[nSourceId - 1]->ulSeed[0]),
				&(NETWORK->ppstruDeviceList[nSourceId - 1]->ulSeed[1]),
				&(ackSize));
		} while (size <= 0.0);
		pstruEventDetails->dEventTime = pstruEventDetails->dEventTime + info->ackResponseTime*MILLISECOND;
	}

	pstruEventDetails->dPacketSize = size;
	pstruEventDetails->nEventType = APPLICATION_OUT_EVENT;
	pstruEventDetails->nPacketId = ++pstruappinfo->nPacketId;
	pstruEventDetails->nProtocolId = PROTOCOL_APPLICATION;
	pstruEventDetails->nSubEventType = 0;
	pstruEventDetails->nPacketId = 0;
	pstruEventDetails->pPacket = fn_NetSim_Application_GeneratePacket(pstruappinfo,
																		pstruEventDetails->dEventTime,
																		nSourceId,
																		1,
																		&nDestinationId,
																		pstruEventDetails->nPacketId,
																		TRAFFIC_COAP,
																		pstruappinfo->qos,
																		pstruappinfo->sourcePort,
																		pstruappinfo->destPort);
	pstruEventDetails->szOtherDetails = pstruappinfo;
	pstruEventDetails->pPacket->nControlDataType = packet_COAP_REQUEST;
	pstruEventDetails->pPacket->nPacketType = PacketType_COAP;
	strcpy(pstruEventDetails->pPacket->szPacketType, PacketType);
	pstruEventDetails->pPacket->pstruAppData->nAppEndFlag = 1;
	pstruEventDetails->pPacket->pstruAppData->nPacketFragment = Segment_Unfragment;
	return 1;
}

COAP_Header* fn_NetSim_Application_COAP_CopyHeader(COAP_Header* src) {
	
	COAP_Header* pCOAPHeader = calloc(1, sizeof* pCOAPHeader);
	pCOAPHeader->Version = 1;
	pCOAPHeader->Type = src->Type;
	pCOAPHeader->TokenLength = src->TokenLength;
	pCOAPHeader->Code = src->Code;
	pCOAPHeader->MessageID = src->MessageID;
	pCOAPHeader->tokenvalue = src->tokenvalue;
	
	COAP_Options* pCOAPOptions;
	pCOAPOptions = calloc(1, sizeof* pCOAPOptions);
	pCOAPOptions->optionDelta = src->Options->optionDelta;
	pCOAPOptions->optionLength = src->Options->optionLength;
	pCOAPOptions->optionvalue = src->Options->optionvalue;
	pCOAPHeader->Options = pCOAPOptions;
	return pCOAPHeader;
}
COAP_Header* fn_NetSim_Application_COAP_GenrateHeader(APP_COAP_INFO* info) {
	COAP_Header* pCOAPHeader = calloc(1, sizeof* pCOAPHeader);

	pCOAPHeader->Version = 1;
	pCOAPHeader->Type = (info->ackRequired == TRUE) ? CONFIRMABLE : NON_CONFIRMABLE;
	pCOAPHeader->TokenLength = 2;
	pCOAPHeader->Code = REQUEST;
	pCOAPHeader->MessageID = rand() % 65535;
	pCOAPHeader->tokenvalue = rand() % 4294967296;

	COAP_Options* pCOAPOptions;
	pCOAPOptions = calloc(1, sizeof* pCOAPOptions);
	pCOAPOptions->optionDelta = 0;
	pCOAPOptions->optionLength = 0;

	// if required set nCOAPOptions->optionvalue 

	pCOAPHeader->Options = pCOAPOptions;

	return pCOAPHeader;
}