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

#define HELLO_MESSAGE_HOP_COUNT		1


int fn_NetSim_NDP_Init()
{
	NETSIM_ID i;
	for(i=0;i<NETWORK->nDeviceCount;i++)
	{
		NODE_OLSR* olsr=GetOLSRData(i+1);
		if(!olsr)
			continue;
		fn_NetSim_NDP_ScheduleHelloTransmission(i+1,0);
		pstruEventDetails->dEventTime = 0;
		fn_NetSim_NDP_TransmitHello();
	}
	return 0;
}

int fn_NetSim_NDP_ScheduleHelloTransmission(NETSIM_ID nNodeId,double dEventTime)
{
	NODE_OLSR* olsr=GetOLSRData(nNodeId);
	double jitter;
	if(!olsr)
	{
		fnNetSimError("OLSR protocol is not configured for %d node\n",nNodeId);
		return -1;
	}
	//Generate random time between 0 to MAXJITTER
	jitter = (int)((fn_NetSim_Utilities_GenerateRandomNo(&NETWORK->ppstruDeviceList[nNodeId-1]->ulSeed[0],
		&NETWORK->ppstruDeviceList[nNodeId-1]->ulSeed[1])/NETSIM_RAND_MAX)*MAXJITTER);
	
	//Add Timer event to transmit hello packet
	pstruEventDetails->dEventTime=dEventTime+olsr->dHelloInterval-jitter;
	pstruEventDetails->dPacketSize=0;
	pstruEventDetails->nApplicationId=0;
	pstruEventDetails->nDeviceId=nNodeId;
	pstruEventDetails->nDeviceType=DEVICE_TYPE(nNodeId);
	pstruEventDetails->nEventType=TIMER_EVENT;
	pstruEventDetails->nInterfaceId=0;
	pstruEventDetails->nPacketId=0;
	pstruEventDetails->pPacket = NULL;
	pstruEventDetails->nProtocolId=NW_PROTOCOL_NDP;
	pstruEventDetails->nSegmentId=0;
	pstruEventDetails->nSubEventType=NDP_ScheduleHelloTransmission;
	fnpAddEvent(pstruEventDetails);
	return 0;
}
int fn_NetSim_NDP_TransmitHello()
{
	unsigned short int linkCount;
	NODE_OLSR* olsr=GetOLSRData(pstruEventDetails->nDeviceId);
	NetSim_PACKET* packet;
	OLSR_HEADER* header=(OLSR_HEADER*)calloc(1,sizeof* header);
	OLSR_HELLO_PACKET* hello=(OLSR_HELLO_PACKET*)calloc(1,sizeof* hello);

	//Fill Hello information
	hello->HTime = olsrConvertDoubleToME(olsr->dHelloInterval);
	hello->Willingness = WILL_DEFAULT;
	//Fill neighbor set
	linkCount=fn_NetSim_Fill_Link_To_Hello(olsr,hello,pstruEventDetails->dEventTime);
	
	//Fill header information
	header->message = (OLSR_HEADER_MESSAGE*)calloc(1,sizeof* header->message);
	header->message->HopCount=HELLO_MESSAGE_HOP_COUNT;
	header->message->MESSAGE=hello;
	header->message->MessageSequenceNumber=++olsr->nMessageSequenceNumber;
	header->message->MessageType = HELLO_MESSAGE;
	header->message->OriginatorAddress = IP_COPY(DEVICE_NWADDRESS(pstruEventDetails->nDeviceId,1));
	header->message->TimeToLive = HELLO_MESSAGE_TTL;
	header->message->Vtime = olsrConvertDoubleToME(NEIGHB_HOLD_TIME);
	header->message->MessageSize = HELLO_LINK_SIZE*linkCount+HELLO_MESSAGE_SIZE+MESSAGE_HEADER_SIZE;

	header->PacketLength = header->message->MessageSize+OLSR_HEADER_SIZE;
	header->PacketSequenceNumber = ++olsr->nPacketSequenceNumber;

	//Create Packet
	packet=fn_NetSim_ZRP_GeneratePacket(pstruEventDetails->dEventTime,
		OLSR_CONTROL_PACKET(HELLO_MESSAGE),
		NW_PROTOCOL_OLSR,
		pstruEventDetails->nDeviceId,
		0,
		header->PacketLength);
	
	packet->pstruNetworkData->Packet_RoutingProtocol=header;

	packet->pstruNetworkData->szNextHopIp = NULL;
	packet->pstruNetworkData->nTTL = HELLO_MESSAGE_TTL;
	//Add network out event
	pstruEventDetails->dPacketSize = header->PacketLength;
	pstruEventDetails->nApplicationId = 0;
	pstruEventDetails->nEventType = NETWORK_OUT_EVENT;
	pstruEventDetails->nInterfaceId = 0;
	pstruEventDetails->nPacketId = 0;
	pstruEventDetails->nProtocolId = NW_PROTOCOL_IPV4;
	pstruEventDetails->nSegmentId = 0;
	pstruEventDetails->nSubEventType = 0;
	pstruEventDetails->pPacket = packet;
	fnpAddEvent(pstruEventDetails);
	return 0;
}
int fn_NetSim_NDP_ReceiveHello()
{
	fn_NetSim_OLSR_PopulateLinkSet();
	fn_NetSim_OLSR_PopulateNeighborSet();
	fn_NetSim_OLSR_Populate2HopNeighbor();
	fn_NetSim_OLSR_PopulateMPRSet();
	fn_NetSim_OLSR_PopulateMPRSelectorSet();
	fn_NetSim_OLSR_UpdateRoutingTable();
	fn_NetSim_Packet_FreePacket(pstruEventDetails->pPacket);
	pstruEventDetails->pPacket = NULL;
	return 0;
}

unsigned short int fn_NetSim_Fill_Link_To_Hello(NODE_OLSR* olsr,OLSR_HELLO_PACKET* hello,double dCurrentTime)
{
	unsigned short int count=0;
	OLSR_NEIGHBOR_SET* neighbor;
	OLSR_HELLO_LINK* h_link,*temp=NULL;
	OLSR_LINK_SET* linkSet=olsr->linkSet;
		
	while(linkSet)
	{
		if(!IP_COMPARE(linkSet->L_local_iface_addr,olsr->mainAddress))
		{
			h_link=(OLSR_HELLO_LINK*)calloc(1,sizeof* h_link);
			if(linkSet->L_SYM_time >= dCurrentTime)
				h_link->LinkCode.linkType=SYM_LINK;
			else if(linkSet->L_ASYM_time >= dCurrentTime)
				h_link->LinkCode.linkType = ASYM_LINK;
			else
				h_link->LinkCode.linkType = LOST_LINK;

			neighbor=olsrFindNeighborSet(olsr->neighborSet,linkSet->L_neighbor_iface_addr);
			if(neighbor)
			{
				if(neighbor->N_status == MPR_NEIGH)
					h_link->LinkCode.neighTypes=MPR_NEIGH;
				else if(neighbor->N_status == SYM_NEIGH)
					h_link->LinkCode.neighTypes = SYM_NEIGH;
				else
					h_link->LinkCode.neighTypes = NOT_NEIGH;
			}
			h_link->LinkMessageSize=HELLO_LINK_SIZE;
			h_link->NeighborInterfaceAddress=IP_COPY(linkSet->L_neighbor_iface_addr);
			if(!hello->link)
				hello->link=h_link;
			else
				temp->next=h_link;
			temp=h_link;
			count++;
		}
		linkSet=(OLSR_LINK_SET*)LIST_NEXT(linkSet);
	}
	neighbor = olsr->neighborSet;
	while(neighbor)
	{
		h_link = olsrFinkLinkInfoFromHello(hello->link,neighbor->N_neighbor_main_addr);
		if(!h_link)
		{
			h_link = (OLSR_HELLO_LINK*)calloc(1,sizeof* h_link);
			h_link->LinkCode.linkType=UNSPEC_LINK;
			if(neighbor->N_status == MPR_NEIGH)
				h_link->LinkCode.neighTypes=MPR_NEIGH;
			else if(neighbor->N_status == SYM_NEIGH)
				h_link->LinkCode.neighTypes = SYM_NEIGH;
			else
				h_link->LinkCode.neighTypes = NOT_NEIGH;

			h_link->LinkMessageSize=HELLO_LINK_SIZE;
			h_link->NeighborInterfaceAddress=IP_COPY(neighbor->N_neighbor_main_addr);
			if(!hello->link)
				hello->link=h_link;
			else
				temp->next=h_link;
			temp=h_link;
			count++;
		}
		neighbor=(OLSR_NEIGHBOR_SET*)LIST_NEXT(neighbor);
	}
	return count;
}

OLSR_HELLO_LINK* olsrFinkLinkInfoFromHello(OLSR_HELLO_LINK* link,NETSIM_IPAddress ip)
{
	while(link)
	{
		if(!IP_COMPARE(link->NeighborInterfaceAddress,ip))
			return link;
		link=link->next;
	}
	return NULL;
}

int fn_NetSim_NDP_FreeHelloMessage(OLSR_HEADER_MESSAGE* message)
{
	OLSR_HELLO_PACKET* hello=(OLSR_HELLO_PACKET*)message->MESSAGE;
	OLSR_HELLO_LINK* link=hello->link;
	while(link)
	{
		hello->link = link->next;
		IP_FREE(link->NeighborInterfaceAddress);
		free(link);
		link=hello->link;
	}
	free(hello);
	return 0;
}
int fn_NetSim_NDP_CopyHelloMessage(OLSR_HEADER_MESSAGE* dest,OLSR_HEADER_MESSAGE* src)
{
	OLSR_HELLO_LINK* temp,*temp1,*temp2=NULL;
	OLSR_HELLO_PACKET* srcHello=(OLSR_HELLO_PACKET*)src->MESSAGE;
	OLSR_HELLO_PACKET* destHello=(OLSR_HELLO_PACKET*)calloc(1,sizeof* destHello);
	memcpy(destHello,srcHello,sizeof* destHello);
	dest->MESSAGE=destHello;

	//Copy link
	temp=srcHello->link;
	destHello->link=NULL;
	while(temp)
	{
		temp1=(OLSR_HELLO_LINK*)calloc(1,sizeof* temp1);
		memcpy(temp1,temp,sizeof* temp1);
		temp1->NeighborInterfaceAddress = IP_COPY(temp->NeighborInterfaceAddress);
		if(!destHello->link)
			destHello->link=temp1;
		else
			temp2->next=temp1;
		temp2=temp1;
		temp=temp->next;
	}
	return 0;
}