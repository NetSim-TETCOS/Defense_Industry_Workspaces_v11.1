/************************************************************************************
* Copyright (C) 2012     
*
* TETCOS, Bangalore. India                                                         *

* Tetcos owns the intellectual property rights in the Product and its content.     *
* The copying, redistribution, reselling or publication of any or all of the       *
* Product or its content without express prior written consent of Tetcos is        *
* prohibited. Ownership and / or any other right relating to the software and all  *
* intellectual property rights therein shall remain at all times with Tetcos.      *

* Author:  Thangarasu.K															   *
* ---------------------------------------------------------------------------------*/
#include "main.h"
#include "Routing.h"
/**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ 
	Every 30 seconds, the RIP process is awakened to send an unsolicited response message 
	containing the complete routing table to every neighboring router.

		1. Create the RIP packet
		2. Add the RIP packets in the WAN ports of the current router
		3. Add the packet in the socket buffer for transmitting the RIP packet
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/
int fn_NetSim_RIP_Update_Timer(struct stru_NetSim_Network *pstruNETWORK,NetSim_EVENTDETAILS *pstruEventDetails)
{
	int nLoop1=0,nLink_Id=0;
	NETSIM_ID nConnectedDevId=0,nDeviceid;
	NETSIM_ID nConnectedInterfaceId=0;
	NETSIM_ID nInterfaceId,nLoop=0;
	NETSIM_ID nInterfaceCount;
	DEVICE_ROUTER *pstruRouter;
	bool bWanPort=false;
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
			pstruRouter = get_RIP_var(nDeviceid);
				if(pstruRouter && pstruRouter->RoutingProtocol[nLoop] == APP_PROTOCOL_RIP)
				{
					//Get the Interface type 
					bWanPort=true;
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
	//To add the next update event
	if(bWanPort)
	{
		pstruEventDetails->nPacketId=0;
		pstruEventDetails->nApplicationId= 0;
		pstruEventDetails->nEventType=TIMER_EVENT;
		pstruEventDetails->nSubEventType=SEND_RIP_UPDATE_PKT;
		pstruEventDetails->nProtocolId=APP_PROTOCOL_RIP;
		pstruEventDetails->dEventTime=pstruEventDetails->dEventTime+get_RIP_var(nDeviceid)->uniInteriorRouting.struRIP.n_Update_timer;
		pstruEventDetails->pPacket = NULL;
		fnpAddEvent(pstruEventDetails);
	}
	return 0;
}




