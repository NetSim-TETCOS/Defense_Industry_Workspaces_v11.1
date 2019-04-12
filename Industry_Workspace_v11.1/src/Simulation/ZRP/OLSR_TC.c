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
#include "ZRP.h"
#include "ZRP_Enum.h"

int fn_NetSim_OLSR_ScheduleTCTransmission(NETSIM_ID nNodeId,double dTime)
{
	double jitter;
	NODE_OLSR* olsr=GetOLSRData(nNodeId);
	jitter = (int)((fn_NetSim_Utilities_GenerateRandomNo(&NETWORK->ppstruDeviceList[nNodeId-1]->ulSeed[0],
		&NETWORK->ppstruDeviceList[nNodeId-1]->ulSeed[1])/NETSIM_RAND_MAX)*MAXJITTER);
	pstruEventDetails->dEventTime = dTime+olsr->dTCInterval-jitter;
	pstruEventDetails->dPacketSize=0;
	pstruEventDetails->nApplicationId=0;
	pstruEventDetails->nDeviceId=nNodeId;
	pstruEventDetails->nDeviceType=DEVICE_TYPE(nNodeId);
	pstruEventDetails->nEventType=TIMER_EVENT;
	pstruEventDetails->nInterfaceId=1;
	pstruEventDetails->nPacketId=0;
	pstruEventDetails->nProtocolId=NW_PROTOCOL_OLSR;
	pstruEventDetails->nSegmentId=0;
	pstruEventDetails->nSubEventType=OLSR_ScheduleTCTransmission;
	pstruEventDetails->pPacket=NULL;
	pstruEventDetails->szOtherDetails=NULL;
	fnpAddEvent(pstruEventDetails);
	return 0;
}
int fn_NetSim_OLSR_TransmitTCMessage()
{
	NetSim_EVENTDETAILS pevent;
	NODE_OLSR* olsr=GetOLSRData(pstruEventDetails->nDeviceId);
	NetSim_PACKET* packet;
	OLSR_HEADER* header=(OLSR_HEADER*)calloc(1,sizeof* header);
	OLSR_TC_MESSAGE* tc=(OLSR_TC_MESSAGE*)calloc(1,sizeof* tc);
	OLSR_MPR_SELECTION_SET* set=olsr->mprSelectionSet;
	olsr->bTCUpdateFlag=false;
	//Fill TC information
	tc->ANSN = ++olsr->ANSN;
	while(set)
	{
		tc->AdvertisedNeighborCount++;
		tc->AdvertisedNeighborMainAddress=(NETSIM_IPAddress*)realloc(tc->AdvertisedNeighborMainAddress,
			tc->AdvertisedNeighborCount*sizeof* tc->AdvertisedNeighborMainAddress);
		tc->AdvertisedNeighborMainAddress[tc->AdvertisedNeighborCount-1]=IP_COPY(set->MS_main_addr);
		set=(OLSR_MPR_SELECTION_SET*)LIST_NEXT(set);
	}

	//Fill header information
	header->message = (OLSR_HEADER_MESSAGE*)calloc(1,sizeof* header->message);
	header->message->HopCount=min(olsr->nZoneRadius,TC_MESSAGE_TTL)+1;
	header->message->MESSAGE=tc;
	header->message->MessageSequenceNumber=++olsr->nMessageSequenceNumber;
	header->message->MessageType = TC_MESSAGE;
	header->message->OriginatorAddress = IP_COPY(olsr->mainAddress);
	header->message->TimeToLive = min(olsr->nZoneRadius,TC_MESSAGE_TTL);
	header->message->Vtime = olsrConvertDoubleToME(TOP_HOLD_TIME);
	header->message->MessageSize = TC_MESSAGE_SIZE_FIXED+tc->AdvertisedNeighborCount*4+MESSAGE_HEADER_SIZE;

	header->PacketLength = header->message->MessageSize+OLSR_HEADER_SIZE;
	header->PacketSequenceNumber = ++olsr->nPacketSequenceNumber;

	//Create Packet
	packet=fn_NetSim_ZRP_GeneratePacket(pstruEventDetails->dEventTime,
		OLSR_CONTROL_PACKET(TC_MESSAGE),
		NW_PROTOCOL_OLSR,
		pstruEventDetails->nDeviceId,
		0,
		header->PacketLength);
	
	packet->pstruNetworkData->Packet_RoutingProtocol=header;

	packet->pstruNetworkData->szNextHopIp = NULL;
	packet->pstruNetworkData->nTTL = min(olsr->nZoneRadius,TC_MESSAGE_TTL);
	//Add network out event
	pevent.dEventTime=pstruEventDetails->dEventTime;
	pevent.dPacketSize = header->PacketLength;
	pevent.nDeviceId=pstruEventDetails->nDeviceId;
	pevent.nDeviceType=pstruEventDetails->nDeviceType;
	pevent.nApplicationId = 0;
	pevent.nEventType = NETWORK_OUT_EVENT;
	pevent.nInterfaceId = 0;
	pevent.nPacketId = 0;
	pevent.nProtocolId = NW_PROTOCOL_IPV4;
	pevent.nSegmentId = 0;
	pevent.nSubEventType = 0;
	pevent.pPacket = packet;
	fnpAddEvent(&pevent);
	return 0;
}
int fn_NetSim_OLSR_CopyTCMessage(OLSR_HEADER_MESSAGE* dest,OLSR_HEADER_MESSAGE* src)
{
	unsigned int i;
	OLSR_TC_MESSAGE* srcTC=(OLSR_TC_MESSAGE*)src->MESSAGE;
	OLSR_TC_MESSAGE* destTC = (OLSR_TC_MESSAGE*)calloc(1,sizeof* destTC);
	destTC->AdvertisedNeighborCount=srcTC->AdvertisedNeighborCount;
	destTC->ANSN=srcTC->ANSN;
	destTC->Reserved=srcTC->Reserved;
	if(destTC->AdvertisedNeighborCount)
		destTC->AdvertisedNeighborMainAddress=(NETSIM_IPAddress*)calloc(destTC->AdvertisedNeighborCount,sizeof* destTC->AdvertisedNeighborMainAddress);
	for(i=0;i<srcTC->AdvertisedNeighborCount;i++)
		destTC->AdvertisedNeighborMainAddress[i]=IP_COPY(srcTC->AdvertisedNeighborMainAddress[i]);
	dest->MESSAGE=destTC;
	return 0;
}
int fn_NetSim_OLSR_FreeTCMessage(OLSR_HEADER_MESSAGE* message)
{
	unsigned int i;
	OLSR_TC_MESSAGE* tc = (OLSR_TC_MESSAGE*)message->MESSAGE;
	for(i=0;i<tc->AdvertisedNeighborCount;i++)
		IP_FREE(tc->AdvertisedNeighborMainAddress[i]);
	free(tc->AdvertisedNeighborMainAddress);
	free(tc);
	message->MESSAGE=NULL;
	return 0;
}

int fn_NetSim_OLSR_ReceiveTC()
{
	//Section 9.5
	NetSim_PACKET* packet=pstruEventDetails->pPacket;
	NODE_OLSR* olsr=GetOLSRData(pstruEventDetails->nDeviceId);
	OLSR_HEADER* header=(OLSR_HEADER*)pstruEventDetails->pPacket->pstruNetworkData->Packet_RoutingProtocol;
	OLSR_TC_MESSAGE* tc = (OLSR_TC_MESSAGE*)header->message->MESSAGE;
	//Condition 1
	OLSR_NEIGHBOR_SET* neighbor=olsrFindNeighborSet(olsr->neighborSet,pstruEventDetails->pPacket->pstruNetworkData->szGatewayIP);
	if(IP_COMPARE(header->message->OriginatorAddress,pstruEventDetails->pPacket->pstruNetworkData->szGatewayIP) &&
		neighbor && neighbor->N_status==SYM_NEIGH)
	{
		fn_NetSim_Packet_FreePacket(pstruEventDetails->pPacket);
		pstruEventDetails->pPacket=NULL;
		return -1;
	}
	//Condition 2
	if(olsrValidateTopologyInfoOnCondition2(olsr->topologyInfoBase,header->message->OriginatorAddress,tc->ANSN))
	{
		fn_NetSim_Packet_FreePacket(pstruEventDetails->pPacket);
		pstruEventDetails->pPacket=NULL;
		return -2;
	}
	
	//Condition 4
	olsrUpdateTopologySet(olsr,&olsr->topologyInfoBase,header->message);


	fn_NetSim_OLSR_PacketForwarding();
	fn_NetSim_Packet_FreePacket(packet);
	pstruEventDetails->pPacket=NULL;
	return 0;
}

OLSR_TOPOLOGY_INFORMATION_BASE* olsrFindTopologyInfo(OLSR_TOPOLOGY_INFORMATION_BASE* topology,NETSIM_IPAddress originator,NETSIM_IPAddress neighbor)
{
	while(topology)
	{
		if(!IP_COMPARE(topology->T_last_addr,originator) && 
			!IP_COMPARE(topology->T_dest_addr,neighbor))
			return topology;
		topology=(OLSR_TOPOLOGY_INFORMATION_BASE*)LIST_NEXT(topology);
	}
	return NULL;
}

bool olsrValidateTopologyInfoOnCondition2(OLSR_TOPOLOGY_INFORMATION_BASE* topology,NETSIM_IPAddress originator,unsigned short int ANSN)
{
	while(topology)
	{
		if(!IP_COMPARE(topology->T_last_addr,originator) &&
			topology->T_seq > ANSN)
			return true;
		topology = (OLSR_TOPOLOGY_INFORMATION_BASE*)LIST_NEXT(topology);
	}
	return false;
}
int olsrRemoveFromTopologySet(OLSR_TOPOLOGY_INFORMATION_BASE** topology,NETSIM_IPAddress originator,unsigned short int ANSN)
{
	OLSR_TOPOLOGY_INFORMATION_BASE* info=*topology;
	while(info)
	{
		if(!IP_COMPARE(info->T_last_addr,originator) &&
			info->T_seq < ANSN)
		{
			LIST_FREE((void**)topology,info);
			info=*topology;
			continue;
		}
		info=(OLSR_TOPOLOGY_INFORMATION_BASE*)LIST_NEXT(info);
	}
	return 0;
}
int olsrUpdateTopologySet(NODE_OLSR* olsr,OLSR_TOPOLOGY_INFORMATION_BASE** topology,OLSR_HEADER_MESSAGE* message)
{
	bool flag = false;
	unsigned int i;
	OLSR_TC_MESSAGE* tc = (OLSR_TC_MESSAGE*)message->MESSAGE;
	OLSR_TOPOLOGY_INFORMATION_BASE* info;
	double validityTime=olsrConvertMEToDouble(message->Vtime);
	for(i=0;i<tc->AdvertisedNeighborCount;i++)
	{
		info=olsrFindTopologyInfo(*topology,message->OriginatorAddress,tc->AdvertisedNeighborMainAddress[i]);
		if(info)
		{
			//condition 4.1
			info->T_time=validityTime*SECOND+pstruEventDetails->dEventTime;
		}
		else
		{
			//Condition 4.2
			info=TOPOLOGY_INFO_BASE_ALLOC();
			info->T_dest_addr=IP_COPY(tc->AdvertisedNeighborMainAddress[i]);
			info->T_last_addr=IP_COPY(message->OriginatorAddress);
			info->T_seq=tc->ANSN;
			info->T_time=validityTime*SECOND+pstruEventDetails->dEventTime;
			LIST_ADD_LAST((void**)topology,info);
			olsr->bRoutingTableUpdate=true;
			flag=true;
		}
	}
	if(flag && !olsr->bTCUpdateFlag)
	{
		NetSim_EVENTDETAILS pevent;
		pevent.nSubEventType = OLSR_ScheduleTCTransmission;
		pevent.dEventTime =pstruEventDetails->dEventTime + (fn_NetSim_Utilities_GenerateRandomNo(
			&NETWORK->ppstruDeviceList[pstruEventDetails->nDeviceId-1]->ulSeed[0],
			&NETWORK->ppstruDeviceList[pstruEventDetails->nDeviceId-1]->ulSeed[1])/NETSIM_RAND_MAX)*SECOND;
		pevent.dPacketSize=0;
		pevent.nApplicationId=0;
		pevent.nDeviceId=pstruEventDetails->nDeviceId;
		pevent.nDeviceType=pstruEventDetails->nDeviceType;
		pevent.nEventType=TIMER_EVENT;
		pevent.nInterfaceId=1;
		pevent.nPacketId=0;
		pevent.nProtocolId=NW_PROTOCOL_OLSR;
		pevent.pPacket=NULL;
		pevent.nSegmentId=0;
		pevent.szOtherDetails=NULL;
		fnpAddEvent(&pevent);
		olsr->bTCUpdateFlag=true;
	}
	return 0;
}

