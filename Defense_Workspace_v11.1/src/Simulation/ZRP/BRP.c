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

//#define DEBUG_BRP_MSG
#ifdef DEBUG_BRP_MSG
static FILE* fpDebugBRP;
#endif

int fn_NetSim_BRP_Init()
{
	NETSIM_ID i;

#ifdef DEBUG_BRP_MSG
	char s[BUFSIZ];
	sprintf(s, "%s/%s",
			pszIOPath,
			"DebugBRP.csv");
	fpDebugBRP = fopen(s, "w");
	if (!fpDebugBRP)
	{
		fnSystemError("Unable to open DebugBRP.csv file\n");
		perror(s);
	}
	else
	{
		fprintf(fpDebugBRP, "%s,%s,%s,%s,\n",
				"Type",
				"EventId",
				"BRPHeader",
				"EncapPacket");
	}
#endif

	for(i=0;i<NETWORK->nDeviceCount;i++)
	{
		if(DEVICE_NWLAYER(i+1)->nRoutingProtocolId==NW_PROTOCOL_BRP)
		{
			NODE_ZRP* zrp=(NODE_ZRP*)DEVICE_NWROUTINGVAR(i+1);
			zrp->brp=calloc(1,sizeof(NODE_BRP));
		}
	}
	return 0;
}

int fn_NetSim_BRP_Update(ptrIP_ROUTINGTABLE iarpTable)
{
	NODE_ZRP* zrp=(NODE_ZRP*)DEVICE_NWROUTINGVAR(pstruEventDetails->nDeviceId);
	NODE_BRP* brp=(NODE_BRP*)zrp->brp;
	flushBodercastTree(brp);
	flushZone(zrp);
	flushIerpTable((NODE_IERP*)zrp->ierp);
	addToIERPTableFromIARP((NODE_IERP*)zrp->ierp,iarpTable);
	flushIERPTableFromIARP((NODE_IERP*)zrp->ierp,iarpTable);
	while(iarpTable)
	{
		if(iarpTable->Metric == zrp->nZoneRadius)
			addToBodercastTree(brp,iarpTable->networkDestination,iarpTable->gateway);
		if(iarpTable->Metric <= zrp->nZoneRadius)
		{
			addToZoneList(zrp,iarpTable->networkDestination);
		}
		iarpTable=(ptrIP_ROUTINGTABLE)LIST_NEXT(iarpTable);
	}
	return 0;
}

int addToBodercastTree(NODE_BRP* brp,
	NETSIM_IPAddress boderIP,
	NETSIM_IPAddress nextHop)
{
	BRP_BODERCAST_TREE* bodercastTree=BRP_BODERCAST_TREE_ALLOC();
	bodercastTree->boderId = fn_NetSim_Stack_GetDeviceId_asIP(boderIP,NULL);
	bodercastTree->borderIP=IP_COPY(boderIP);
	bodercastTree->nexthop=IP_COPY(nextHop);
	LIST_ADD_LAST((void**)&brp->bodercastTree,bodercastTree);
	return 0;
}

int fn_NetSim_BRP_BodercastPacket(NetSim_PACKET* dataPacket)
{
	NODE_ZRP* zrp=(NODE_ZRP*)DEVICE_NWROUTINGVAR(pstruEventDetails->nDeviceId);
	NODE_BRP* brp=(NODE_BRP*)zrp->brp;
	BRP_BODERCAST_TREE* tree=brp->bodercastTree;
	while(tree)
	{
		NetSim_EVENTDETAILS pevent;
		NetSim_PACKET* packet = fn_NetSim_ZRP_GeneratePacket(pstruEventDetails->dEventTime,
		dataPacket->nControlDataType+BRP_DIFF,
		NW_PROTOCOL_BRP,
		pstruEventDetails->nDeviceId,
		tree->boderId,
		fnGetPacketSize(dataPacket)+BRP_PACKET_SIZE);
		BRP_HEADER* header = (BRP_HEADER*)calloc(1,sizeof* header);
		packet->pstruNetworkData->Packet_RoutingProtocol=header;
		packet->pstruNetworkData->nTTL = dataPacket->pstruNetworkData->nTTL;
		packet->pstruNetworkData->szNextHopIp = NULL;
		header->EncapsulatedPacket = fn_NetSim_Packet_CopyPacket(dataPacket);

#ifdef DEBUG_BRP_MSG
		if (fpDebugBRP)
		{
			fprintf(fpDebugBRP, "%s,%lld,0x%p,0x%p,\n",
					"Create",
					pstruEventDetails->nEventId,
					(void*)header,
					(void*)header->EncapsulatedPacket);
			fflush(fpDebugBRP);
		}
#endif
		header->QuerySourceAddress = IP_COPY(DEVICE_NWADDRESS(pstruEventDetails->nDeviceId,1));
		header->QueryDestinationAddress = IP_COPY(tree->borderIP);
		header->QueryID = ++brp->nQueryId;
		header->QueryExtension = 1;
		header->PrevBordercastAddress = IP_COPY(DEVICE_NWADDRESS(pstruEventDetails->nDeviceId,1));

		//write network out for transmit packet
		pevent.dEventTime=pstruEventDetails->dEventTime;
		pevent.dPacketSize=fnGetPacketSize(packet);
		pevent.nApplicationId=0;
		pevent.nDeviceId=pstruEventDetails->nDeviceId;
		pevent.nDeviceType=pstruEventDetails->nDeviceType;
		pevent.nEventType=NETWORK_OUT_EVENT;
		pevent.nInterfaceId=0;
		pevent.nPacketId=0;
		pevent.nProtocolId=NW_PROTOCOL_IPV4;
		pevent.nSegmentId=0;
		pevent.nSubEventType=0;
		pevent.pPacket=packet;
		pevent.szOtherDetails=NULL;
		fnpAddEvent(&pevent);

		tree=(BRP_BODERCAST_TREE*)LIST_NEXT(tree);
	}
	return 0;
}
int fn_NetSim_BRP_ProcessPacket()
{
	bool bodercastFlag=false;
	BRP_HEADER* header = (BRP_HEADER*)pstruEventDetails->pPacket->pstruNetworkData->Packet_RoutingProtocol;
	bodercastFlag = fn_NetSim_IERP_ProcessPacket(header->EncapsulatedPacket);
	if(bodercastFlag && !IP_COMPARE(pstruEventDetails->pPacket->pstruNetworkData->szDestIP,
		DEVICE_NWADDRESS(pstruEventDetails->nDeviceId,1)))
	{
		NODE_ZRP* zrp=(NODE_ZRP*)DEVICE_NWROUTINGVAR(pstruEventDetails->nDeviceId);
		NODE_BRP* brp=(NODE_BRP*)zrp->brp;
		BRP_BODERCAST_TREE* tree=brp->bodercastTree;
		header->QueryID = ++brp->nQueryId;
		while(tree)
		{
			if(IP_COMPARE(tree->borderIP,header->PrevBordercastAddress))
			{
				NetSim_EVENTDETAILS pevent;
				NetSim_PACKET* packet = fn_NetSim_Packet_CopyPacket(pstruEventDetails->pPacket);
				BRP_HEADER* temp = (BRP_HEADER*)packet->pstruNetworkData->Packet_RoutingProtocol;

				temp->PrevBordercastAddress = IP_COPY(DEVICE_NWADDRESS(pstruEventDetails->nDeviceId,1));

				packet->pstruNetworkData->szNextHopIp = NULL;
				packet->pstruNetworkData->szDestIP = IP_COPY(tree->borderIP);
				packet->pstruNetworkData->szSourceIP = IP_COPY(DEVICE_NWADDRESS(pstruEventDetails->nDeviceId,1));
				packet->pstruNetworkData->szGatewayIP = IP_COPY(DEVICE_NWADDRESS(pstruEventDetails->nDeviceId,1));
				packet->pstruNetworkData->Packet_RoutingProtocol = temp;
				packet->nSourceId = pstruEventDetails->nDeviceId;
				remove_dest_from_packet(packet, get_first_dest_from_packet(packet));
				add_dest_to_packet(packet, tree->boderId);
				packet->nTransmitterId = pstruEventDetails->nDeviceId;
				packet->nReceiverId = tree->boderId;
				packet->pstruNetworkData->dOverhead = fnGetPacketSize(temp->EncapsulatedPacket)+BRP_PACKET_SIZE;
				packet->pstruNetworkData->dPacketSize = fnGetPacketSize(temp->EncapsulatedPacket)+BRP_PACKET_SIZE;

				//write network out for transmit packet
				pevent.dEventTime=pstruEventDetails->dEventTime;
				pevent.dPacketSize=fnGetPacketSize(packet);
				pevent.nApplicationId=0;
				pevent.nDeviceId=pstruEventDetails->nDeviceId;
				pevent.nDeviceType=pstruEventDetails->nDeviceType;
				pevent.nEventType=NETWORK_OUT_EVENT;
				pevent.nInterfaceId=0;
				pevent.nPacketId=0;
				pevent.nProtocolId=NW_PROTOCOL_IPV4;
				pevent.nSegmentId=0;
				pevent.nSubEventType=0;
				pevent.pPacket=packet;
				pevent.szOtherDetails=NULL;
				fnpAddEvent(&pevent);
			}
			tree=(BRP_BODERCAST_TREE*)LIST_NEXT(tree);
		}
		pstruEventDetails->pPacket=NULL;
	}
	else if(!bodercastFlag)
		pstruEventDetails->pPacket = NULL;
	return 0;
}


int fn_NetSim_BRP_FreeBRPHeader(NetSim_PACKET* packet)
{
	BRP_HEADER* header = (BRP_HEADER*)packet->pstruNetworkData->Packet_RoutingProtocol;
#ifdef DEBUG_BRP_MSG
	if (fpDebugBRP)
	{
		fprintf(fpDebugBRP, "%s,%lld,0x%p,0x%p,\n",
				"Free",
				pstruEventDetails->nEventId,
				(void*)header,
				(void*)header->EncapsulatedPacket);
		fflush(fpDebugBRP);
	}
#endif
	fn_NetSim_Packet_FreePacket(header->EncapsulatedPacket);
	IP_FREE(header->PrevBordercastAddress);
	IP_FREE(header->QueryDestinationAddress);
	IP_FREE(header->QuerySourceAddress);
	free(header);
	return 0;
}

int fn_NetSim_BRP_CopyBRPHeader(const NetSim_PACKET* destPacket,const NetSim_PACKET* srcPacket)
{
	BRP_HEADER* dest= (BRP_HEADER*)calloc(1,sizeof* dest);
	BRP_HEADER* src= (BRP_HEADER*)srcPacket->pstruNetworkData->Packet_RoutingProtocol;
	destPacket->pstruNetworkData->Packet_RoutingProtocol = dest;
	memcpy(dest,src,sizeof* dest);
#ifdef DEBUG_BRP_MSG
	if (fpDebugBRP)
	{
		fprintf(fpDebugBRP, "%s,%lld,0x%p,0x%p,0x%p,\n",
				"After Copy",
				pstruEventDetails->nEventId,
				(void*)src,
				(void*)dest->EncapsulatedPacket,
				(void*)dest);
		fflush(fpDebugBRP);
	}
#endif
	dest->EncapsulatedPacket = fn_NetSim_Packet_CopyPacket(src->EncapsulatedPacket);
#ifdef DEBUG_BRP_MSG
	if (fpDebugBRP)
	{
		fprintf(fpDebugBRP, "%s,%lld,0x%p,0x%p,0x%p,\n",
				"After Copy",
				pstruEventDetails->nEventId,
				(void*)src,
				(void*)dest->EncapsulatedPacket,
				(void*)dest);
		fflush(fpDebugBRP);
	}
#endif
	dest->PrevBordercastAddress = IP_COPY(src->PrevBordercastAddress);
	dest->QueryDestinationAddress = IP_COPY(src->QueryDestinationAddress);
	dest->QuerySourceAddress = IP_COPY(src->QuerySourceAddress);
	return 0;
}


