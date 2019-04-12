/************************************************************************************
* Copyright (C) 2013                                                               *
* TETCOS, Bangalore. India                                                         *
*                                                                                  *
* Tetcos owns the intellectual property rights in the Product and its content.     *
* The copying, redistribution, reselling or publication of any or all of the       *
* Product or its content without express prior written consent of Tetcos is        *
* prohibited. Ownership and / or any other right relating to the software and all *
* intellectual property rights therein shall remain at all times with Tetcos.      *
*                                                                                  *
* Author:    Thangarasu                                                       *
*                                                                                  *
* ---------------------------------------------------------------------------------*/
#define _CRT_SECURE_NO_DEPRECATE
#include "main.h"
#include "Routing.h"
/**
	This function is called by NetworkStack.dll, while configuring the device 
	Application layer for RIP protocol.	
*/
_declspec(dllexport)int fn_NetSim_RIP_Configure(void** var)
{
	return fn_NetSim_RIP_Configure_F(var);
}
/**
	This functon initializes the RIP parameters.
*/
_declspec (dllexport) int fn_NetSim_RIP_Init(struct stru_NetSim_Network *NETWORK_Formal,\
	NetSim_EVENTDETAILS *pstruEventDetails_Formal,char *pszAppPath_Formal,\
	char *pszWritePath_Formal,int nVersion_Type,void **fnPointer)
{
	fn_NetSim_RIP_Init_F(NETWORK_Formal,pstruEventDetails_Formal,pszAppPath_Formal,\
		pszWritePath_Formal,nVersion_Type,fnPointer);
	return 0;
}
/**
	This function is called by NetworkStack.dll, once simulation end to free the 
	allocated memory for the network.	
*/
_declspec(dllexport) int fn_NetSim_RIP_Finish()
{
	fn_NetSim_RIP_Finish_F();
	return 0;
}
/**
	This function is called by NetworkStack.dll, while writing the evnt trace 
	to get the sub event as a string.
*/
_declspec (dllexport) char *fn_NetSim_RIP_Trace(int nSubEvent)
{
	return (fn_NetSim_RIP_Trace_F(nSubEvent));
}
/**
	This function is called by NetworkStack.dll, to free the packet of RIP protocol
*/
_declspec(dllexport) int fn_NetSim_RIP_FreePacket(NetSim_PACKET* pstruPacket)
{
	return fn_NetSim_RIP_FreePacket_F(pstruPacket);	
}
/**
	This function is called by NetworkStack.dll, to copy the packet of RIP protocol
	from source to destination.
*/
_declspec(dllexport) int fn_NetSim_RIP_CopyPacket(NetSim_PACKET* pstruDestPacket,NetSim_PACKET* pstruSrcPacket)
{
	return fn_NetSim_RIP_CopyPacket_F(pstruDestPacket,pstruSrcPacket);	
}
/**
	This function write the Metrics in Metrics.txt	
*/
_declspec(dllexport) int fn_NetSim_RIP_Metrics(PMETRICSWRITER metricsWriter)
{
	return fn_NetSim_RIP_Metrics_F(metricsWriter);	
}
/**
	This function will return the string to write packet trace heading.
*/
_declspec(dllexport)char* fn_NetSim_RIP_ConfigPacketTrace()
{
	return "";
}
/**
   This function will return the string to write packet trace.																									
*/
_declspec(dllexport)char* fn_NetSim_RIP_WritePacketTrace()
{
	return "";
}
/**
	This function is called by NetworkStack.dll, whenever the event gets triggered	<br/>
	inside the NetworkStack.dll for the Application layer RIP protocol<br/>
	It includes APPLICATION_IN,NETWORK_OUT,NETWORK_IN and TIMER_EVENT.<br/>
		
 */
_declspec(dllexport)int fn_NetSim_RIP_Run()
{
	if(pstruEventDetails->pPacket && !fnCheckRoutingPacket())
		return 0; //Not routing packet
	return fnRouter[pstruEventDetails->nProtocolId?pstruEventDetails->nProtocolId%100:pstruEventDetails->pPacket->nControlDataType/100%100]();
}
/** This function is used to send the RIP updates whenever routing table is updated*/ 
_declspec(dllexport)int fn_NetSim_RIP_TriggeredUpdate(struct stru_NetSim_Network *pstruNETWORK,NetSim_EVENTDETAILS *pstruEventDetails)
{
	int nLoop1=0,nLink_Id=0;
	NETSIM_ID nConnectedDevId=0,nDeviceid;
	NETSIM_ID nConnectedInterfaceId=0;
	NETSIM_ID nInterfaceId,nLoop=0;
	NETSIM_ID nInterfaceCount;
	DEVICE_ROUTER *pstruRouter;
	RIP_PACKET *pstruRIPPacket = NULL;
	NetSim_PACKET* pstruControlPacket=NULL;
	RIP_ROUTING_DATABASE *pstrudatabase;
	RIP_ENTRY *pstruRIPNewEntry,*pstruRIPCurrentEntry;
	NETWORK=pstruNETWORK;
	nDeviceid=pstruEventDetails->nDeviceId;
	nInterfaceCount=NETWORK->ppstruDeviceList[nDeviceid-1]->nNumOfInterface;

	for(nLoop=0;nLoop<nInterfaceCount;nLoop++)
	{
		if(NETWORK->ppstruDeviceList[nDeviceid-1]->ppstruInterfaceList[nLoop]->szAddress &&
			NETWORK->ppstruDeviceList[nDeviceid-1]->ppstruInterfaceList[nLoop]->nInterfaceType==INTERFACE_WAN_ROUTER)
		{
			nLink_Id = fn_NetSim_Stack_GetConnectedDevice(nDeviceid,nLoop+1,&nConnectedDevId,&nConnectedInterfaceId);
			if(!nLink_Id)
				continue;
			if(NETWORK->ppstruDeviceList[nDeviceid-1]->pstruApplicationLayer)
			{
				pstruRouter = get_RIP_var(nDeviceid);
				if(pstruRouter && pstruRouter->RoutingProtocol[nLoop] == APP_PROTOCOL_RIP)
				{
					//Creating the RIP packet
					pstruRIPPacket=calloc(1,sizeof(RIP_PACKET));
					//Assign the Version type
					pstruRIPPacket->nVersion=pstruRouter->uniInteriorRouting.struRIP.n_RIP_Version;
					pstrudatabase=pstruRouter->pstruRoutingTables->pstruRIP_RoutingTable;
					//Add all the entries in routing database to RIP packet
					while(pstrudatabase!=NULL)
					{		
						pstruRIPNewEntry=calloc(1,sizeof(struct stru_RIPEntry));
						pstruRIPNewEntry->szIPv4_address=IP_COPY(pstrudatabase->szAddress);
						pstruRIPNewEntry->szNext_Hop=IP_COPY(pstrudatabase->szRouter);
						pstruRIPNewEntry->nAddress_family_identifier=2; // The Address family identifier value is 2 for IP
						pstruRIPNewEntry->szSubnet_Mask=IP_COPY(pstrudatabase->szSubnetmask);
						pstruRIPNewEntry->nMetric=pstrudatabase->nMetric;
						if(pstruRIPPacket->pstruRIPEntry)
						{
							pstruRIPCurrentEntry = pstruRIPPacket->pstruRIPEntry;
							while(pstruRIPCurrentEntry->pstru_RIP_NextEntry != NULL)
								pstruRIPCurrentEntry = pstruRIPCurrentEntry->pstru_RIP_NextEntry;
							pstruRIPCurrentEntry->pstru_RIP_NextEntry = pstruRIPNewEntry;
						}
						else
							pstruRIPPacket->pstruRIPEntry = pstruRIPNewEntry;

						pstrudatabase=pstrudatabase->pstru_Router_NextEntry;
					}
					nInterfaceId=nLoop+1;
					//Create the Application layer packet to carry the RIP packet
					pstruControlPacket=fn_NetSim_Packet_CreatePacket(APPLICATION_LAYER);
					pstruControlPacket->nControlDataType=RIP_Packet;
					pstruControlPacket->nPacketId=0;
					pstruControlPacket->nPacketType=PacketType_Control;
					pstruControlPacket->nSourceId=nDeviceid;
					add_dest_to_packet(pstruControlPacket, nConnectedDevId);
					pstruControlPacket->nTransmitterId=nDeviceid;
					pstruControlPacket->nReceiverId=nConnectedDevId;
					//Assign the Application layer details of the packet
					pstruControlPacket->pstruAppData->dArrivalTime=pstruEventDetails->dEventTime;
					pstruControlPacket->pstruAppData->dEndTime=pstruEventDetails->dEventTime;
					pstruControlPacket->pstruAppData->dStartTime=pstruEventDetails->dEventTime;
					pstruControlPacket->pstruAppData->dPayload=RIP_PACKET_SIZE_WITH_HEADER;
					pstruControlPacket->pstruAppData->dOverhead=0;
					pstruControlPacket->pstruAppData->dPacketSize=pstruControlPacket->pstruAppData->dPayload+pstruControlPacket->pstruAppData->dOverhead;
					pstruControlPacket->pstruAppData->nApplicationProtocol=APP_PROTOCOL_RIP;
					pstruControlPacket->pstruAppData->Packet_AppProtocol=pstruRIPPacket;
					//Assign the Network layer details of the packet
					pstruControlPacket->pstruNetworkData->nNetworkProtocol=NW_PROTOCOL_IPV4;
					pstruControlPacket->pstruNetworkData->nTTL = 1;
					pstruControlPacket->pstruNetworkData->szSourceIP=IP_COPY(fn_NetSim_Stack_GetIPAddressAsId(nDeviceid,nInterfaceId));
					pstruControlPacket->pstruNetworkData->szDestIP=IP_COPY(fn_NetSim_Stack_GetIPAddressAsId(nConnectedDevId,nConnectedInterfaceId));
					if(!pstruControlPacket->pstruNetworkData->szDestIP)
					{
						char str[5000];
						sprintf(str,"Unable to get the connected device IP address for Router %d Interface %d",nDeviceid,nInterfaceId);
						fnNetSimError(str);
					}
					//Assign the Transport layer details of the packet
					pstruControlPacket->pstruTransportData->nSourcePort=RIP_PORT+1;
					pstruControlPacket->pstruTransportData->nDestinationPort=RIP_PORT;
					if(NETWORK->ppstruDeviceList[nDeviceid-1]->pstruTransportLayer->isUDP)
						pstruControlPacket->pstruTransportData->nTransportProtocol=TX_PROTOCOL_UDP;
					else if(NETWORK->ppstruDeviceList[nDeviceid-1]->pstruTransportLayer->isTCP)
						pstruControlPacket->pstruTransportData->nTransportProtocol=TX_PROTOCOL_TCP;
					else
						pstruControlPacket->pstruTransportData->nTransportProtocol=0;
					//Add the packets to the socket buffer
					if(!fn_NetSim_Socket_GetBufferStatus(pstruRouter->sock))
					{
						fn_NetSim_Socket_PassPacketToInterface(nDeviceid, pstruControlPacket, pstruRouter->sock);
						pstruEventDetails->dEventTime=pstruEventDetails->dEventTime;
						pstruEventDetails->dPacketSize=pstruControlPacket->pstruAppData->dPacketSize;
						pstruEventDetails->nApplicationId=0;
						pstruEventDetails->nProtocolId=pstruControlPacket->pstruTransportData->nTransportProtocol;
						pstruEventDetails->nDeviceId=nDeviceid;
						pstruEventDetails->nInterfaceId=0;
						pstruEventDetails->nEventType=TRANSPORT_OUT_EVENT;
						pstruEventDetails->nSubEventType=0;
						pstruEventDetails->pPacket=NULL;
						pstruEventDetails->szOtherDetails = pstruRouter->sock;
						fnpAddEvent(pstruEventDetails);	
					}
					else
					{
						fn_NetSim_Socket_PassPacketToInterface(nDeviceid, pstruControlPacket, pstruRouter->sock);
					}	
				}
			}
		}
	}
	return 0;
}
/** This function is used to execute the RIP functionalities.*/
int fn_NetSim_RIP_Run_F()
{
	SUB_EVENT nSub_Event_Type;

	nEventType=pstruEventDetails->nEventType;

	nSub_Event_Type = pstruEventDetails->nSubEventType;
	//	
	//Check  the event type
	switch(nEventType)	
	{			
	case APPLICATION_IN_EVENT:
		switch(nSub_Event_Type)
		{
		case ROUTING_TABLE_UPDATION:
			fn_NetSim_RIP_DistanceVectorAlgorithm(NETWORK,pstruEventDetails);
#ifdef _RIP_Trace
			fn_NetSim_RIPTrace(NETWORK,pstruEventDetails->nDeviceId,1);
#endif		
			break;
		default:
			fn_NetSim_RIP_ReceivingOf_RIP_Message(NETWORK,pstruEventDetails);
			break;
		}
		break;
	case TIMER_EVENT:
		switch(nSub_Event_Type)
		{
		case SEND_RIP_UPDATE_PKT:
			fn_NetSim_RIP_Update_Timer(NETWORK,pstruEventDetails);
			break;
		case RIP_TIMEOUT:
			fn_NetSim_RIP_Timeout_Timer(NETWORK,pstruEventDetails);	
			break;
		case RIP_GARBAGE_COLLECTION:
			fn_NetSim_RIP_Garbage_Collection_Timer(NETWORK,pstruEventDetails);
			break;
		default:
			printf("\nInvalid sub event");
			break;
		}
		break;
	default:	
		printf("\nInvalid event");
		break;	
	}	

	return 0;
}


