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

int fn_NetSim_IERP_Init()
{
	NETSIM_ID i;
	for(i=0;i<NETWORK->nDeviceCount;i++)
	{
		if(DEVICE_NWLAYER(i+1)->nRoutingProtocolId==NW_PROTOCOL_ZRP)
		{
			NODE_ZRP* zrp=DEVICE_NWROUTINGVAR(i+1);
			zrp->ierp=calloc(1,sizeof(NODE_IERP));
		}
	}
	return 0;
}
int fn_NetSim_IERP_RoutePacket()
{
	NODE_ZRP* zrp=(NODE_ZRP*)DEVICE_NWROUTINGVAR(pstruEventDetails->nDeviceId);
	NODE_IERP* ierp=(NODE_IERP*)zrp->ierp;
	NODE_BRP* brp= (NODE_BRP*)zrp->brp;
	NetSim_PACKET* packet = pstruEventDetails->pPacket;
	IERP_ROUTE_TABLE* route = checkDestInRouteTable(ierp->routeTable,packet->pstruNetworkData->szDestIP);
	if(route)
	{
		NETSIM_ID i;
		//Route found
		packet->pstruNetworkData->szGatewayIP = IP_COPY(DEVICE_NWADDRESS(pstruEventDetails->nDeviceId,1));
		if(route->Route)
			packet->pstruNetworkData->szNextHopIp = IP_COPY(route->Route[0]);
		else
			packet->pstruNetworkData->szNextHopIp = IP_COPY(route->DestAddr);
		pstruEventDetails->nInterfaceId=1;
		packet->pstruNetworkData->nTTL=255;
		packet->nTransmitterId = pstruEventDetails->nDeviceId;
		packet->nReceiverId = fn_NetSim_Stack_GetDeviceId_asIP(packet->pstruNetworkData->szNextHopIp,&i);
		return 2;
	}
	else
	{
		//Initiate route discovery
		//Add packet to buffer
		NetSim_PACKET* buffer=ierp->buffer;
		while(buffer && buffer->pstruNextPacket)
			buffer=buffer->pstruNextPacket;
		if(buffer)
			buffer->pstruNextPacket=packet;
		else
			ierp->buffer=packet;
		pstruEventDetails->pPacket=NULL;

		if(!brp->bodercastTree)
		{
			//IARP link state routing is not completed. wait...
			return 1;
		}
		else
		{
			fn_NetSim_IERP_GenerateRouteRequest(ierp,packet);
		}
	}
	return 0;
}

IERP_ROUTE_TABLE* checkDestInRouteTable(IERP_ROUTE_TABLE* table,NETSIM_IPAddress dest)
{
	while(table)
	{
		if(!IP_COMPARE(table->DestAddr,dest))
			return table;
		table=(IERP_ROUTE_TABLE*)LIST_NEXT(table);
	}
	return NULL;
}
int fn_NetSim_IERP_GenerateRouteRequest(NODE_IERP* ierp,NetSim_PACKET* dataPacket)
{
	NetSim_PACKET* packet = fn_NetSim_ZRP_GeneratePacket(pstruEventDetails->dEventTime,
		IERP_ROUTE_REQUEST,
		NW_PROTOCOL_IERP,
		dataPacket->nSourceId,
		get_first_dest_from_packet(dataPacket),
		IERP_ROUTE_REQUEST_SIZE_FIXED);
	IERP_PACKET* request = (IERP_PACKET*)calloc(1,sizeof* request);
	packet->pstruNetworkData->Packet_RoutingProtocol=request;
	packet->pstruNetworkData->nTTL=MAX_TTL;
	request->Type = ROUTE_REQUEST;
	request->Length = IERP_PACKET_LENGTH(IERP_ROUTE_REQUEST_SIZE_FIXED);
	request->NodePtr=0;
	request->QueryID=++ierp->nQueryId;
	request->RouteSourceAddress = IP_COPY(dataPacket->pstruNetworkData->szSourceIP);
	request->RouteDestinationAddress = IP_COPY(dataPacket->pstruNetworkData->szDestIP);
	//Call BRP to send packet
	fn_NetSim_BRP_BodercastPacket(packet);
	return 0;
}
bool fn_NetSim_IERP_ProcessRouteRequest(NetSim_PACKET* packet)
{
	NODE_ZRP* zrp=(NODE_ZRP*)DEVICE_NWROUTINGVAR(pstruEventDetails->nDeviceId);
	ZRP_ZONE* zone = zrp->zone;

	IERP_PACKET* request = (IERP_PACKET*)packet->pstruNetworkData->Packet_RoutingProtocol;
	if(!IP_COMPARE(request->RouteDestinationAddress,
		DEVICE_NWADDRESS(pstruEventDetails->nDeviceId,1)))
	{
		//Generate route reply
		fn_NetSim_IERP_GenerateRouteReply(packet);
		return false;
	}
	while(zone)
	{
		if(!IP_COMPARE(request->RouteDestinationAddress,
			zone->zoneNodeIP))
		{
			//Generate route reply
			fn_NetSim_IERP_GenerateRouteReply(packet);
			return false;
		}
		zone=(ZRP_ZONE*)LIST_NEXT(zone);
	}
	request->IntermediateNodeCount++;
	request->IntermediateNodeAddress = (NETSIM_IPAddress*)realloc(request->IntermediateNodeAddress,
		request->IntermediateNodeCount*sizeof* request->IntermediateNodeAddress);
	request->IntermediateNodeAddress[request->IntermediateNodeCount-1] = IP_COPY(DEVICE_NWADDRESS(pstruEventDetails->nDeviceId,1));
	packet->pstruNetworkData->dOverhead += 4;
	packet->pstruNetworkData->dPacketSize += 4;
	return true;
}
bool fn_NetSim_IERP_ProcessPacket(NetSim_PACKET* packet)
{
	switch(packet->nControlDataType)
	{
	case IERP_ROUTE_REQUEST:
		return fn_NetSim_IERP_ProcessRouteRequest(packet);
		break;
	case IERP_ROUTE_REPLY:
		break;
	}
	return false;
}


int fn_NetSim_IERP_CopyIerpHeader(const NetSim_PACKET* destPacket,const NetSim_PACKET* srcPacket)
{
	unsigned int i;
	IERP_PACKET* dest=(IERP_PACKET*)calloc(1,sizeof* dest);
	IERP_PACKET* src=(IERP_PACKET*)srcPacket->pstruNetworkData->Packet_RoutingProtocol;
	memcpy(dest,src,sizeof* dest);
	destPacket->pstruNetworkData->Packet_RoutingProtocol=dest;

	if (dest->IntermediateNodeCount)
	{
		dest->IntermediateNodeAddress = (NETSIM_IPAddress*)calloc(dest->IntermediateNodeCount, sizeof* dest->IntermediateNodeAddress);
		for (i = 0; i < dest->IntermediateNodeCount; i++)
			dest->IntermediateNodeAddress[i] = IP_COPY(src->IntermediateNodeAddress[i]);
	}
	dest->RouteDestinationAddress = IP_COPY(src->RouteDestinationAddress);
	dest->RouteSourceAddress=IP_COPY(src->RouteSourceAddress);
	return 0;
}
int fn_NetSim_IERP_FreeIerpHeader(NetSim_PACKET* packet)
{
	IERP_PACKET* header = (IERP_PACKET*)packet->pstruNetworkData->Packet_RoutingProtocol;
	unsigned int i;
	for(i=0;i<header->IntermediateNodeCount;i++)
		IP_FREE(header->IntermediateNodeAddress[i]);
	IP_FREE(header->RouteDestinationAddress);
	IP_FREE(header->RouteSourceAddress);
	free(header);
	packet->pstruNetworkData->Packet_RoutingProtocol=NULL;
	return 0;
}

NETSIM_IPAddress replyGetNextHop(IERP_PACKET* reply,NETSIM_IPAddress ip)
{
	unsigned int i;
	if(!IP_COMPARE(ip,reply->RouteSourceAddress))
	{
		if(reply->IntermediateNodeAddress)
			return reply->IntermediateNodeAddress[0];
		else
			return reply->RouteDestinationAddress;
	}
	for(i=0;i<reply->IntermediateNodeCount;i++)
	{
		if(!IP_COMPARE(ip,reply->IntermediateNodeAddress[i]))
		{
			if(i==reply->IntermediateNodeCount-1)
				return reply->RouteDestinationAddress;
			else
				return reply->IntermediateNodeAddress[i+1];
		}
	}
	return NULL;
}
int fn_NetSim_IERP_GenerateRouteReply(NetSim_PACKET* requestPacket)
{
	NetSim_EVENTDETAILS pevent;
	unsigned int i,j;
	NODE_ZRP* zrp=(NODE_ZRP*)DEVICE_NWROUTINGVAR(pstruEventDetails->nDeviceId);
	NODE_IERP* ierp=(NODE_IERP*)zrp->ierp;

	IERP_PACKET* request = (IERP_PACKET*)requestPacket->pstruNetworkData->Packet_RoutingProtocol;

	if (pstruEventDetails->nDeviceId == requestPacket->nSourceId)
		return -1;

	IERP_ROUTE_TABLE* route = checkDestInRouteTable(ierp->routeTable,request->RouteDestinationAddress);
	int count = request->IntermediateNodeCount+route->count+1;

	IERP_PACKET* reply =  (IERP_PACKET*)calloc(1,sizeof* reply);
	NetSim_PACKET* packet = fn_NetSim_ZRP_GeneratePacket(pstruEventDetails->dEventTime,
		IERP_ROUTE_REPLY,
		NW_PROTOCOL_IERP,
		pstruEventDetails->nDeviceId,
		requestPacket->nSourceId,
		IERP_ROUTE_REPLY_SIZE_FIXED+4*count);

	packet->pstruNetworkData->Packet_RoutingProtocol=reply;
	packet->pstruNetworkData->szNextHopIp = NULL;
	packet->pstruNetworkData->nTTL=MAX_TTL;

	reply->IntermediateNodeCount = count;
	reply->IntermediateNodeAddress = (NETSIM_IPAddress*)calloc(reply->IntermediateNodeCount,sizeof* reply->IntermediateNodeAddress);
	for(i=0,j=0;i<reply->IntermediateNodeCount;i++)
	{
		if(i<route->count)
		{
			reply->IntermediateNodeAddress[i] = IP_COPY(route->Route[route->count-i-1]);
		}
		else if(j==0)
		{
			reply->IntermediateNodeAddress[i] = IP_COPY(DEVICE_NWADDRESS(pstruEventDetails->nDeviceId,1));
			j++;
		}
		else
		{
			reply->IntermediateNodeAddress[i]=IP_COPY(request->IntermediateNodeAddress[request->IntermediateNodeCount-j]);
			j++;
		}
	}
	reply->Length = (unsigned char)(IERP_PACKET_LENGTH(IERP_ROUTE_REPLY_SIZE_FIXED+4*reply->IntermediateNodeCount));
	reply->QueryID = ++ierp->nQueryId;
	reply->RouteDestinationAddress = IP_COPY(request->RouteSourceAddress);
	reply->RouteSourceAddress = IP_COPY(request->RouteDestinationAddress);
	reply->Type = ROUTE_REPLY;
	reply->NodePtr=0;
	//Set the next hop
	if(reply->IntermediateNodeCount)
	{
		packet->pstruNetworkData->szNextHopIp = IP_COPY(replyGetNextHop(reply,DEVICE_NWADDRESS(pstruEventDetails->nDeviceId,1)));
		packet->nReceiverId = fn_NetSim_Stack_GetDeviceId_asIP(packet->pstruNetworkData->szNextHopIp,NULL);
	}

	//Add network out event to transmit packet
	pevent.dEventTime=pstruEventDetails->dEventTime;
	pevent.dPacketSize=IERP_ROUTE_REPLY_SIZE_FIXED+4*request->IntermediateNodeCount;
	pevent.nApplicationId=0;
	pevent.nDeviceId=pstruEventDetails->nDeviceId;
	pevent.nDeviceType=pstruEventDetails->nDeviceType;
	pevent.nEventType=NETWORK_OUT_EVENT;
	pevent.nInterfaceId=1;
	pevent.nPacketId=0;
	pevent.nProtocolId=NW_PROTOCOL_IPV4;
	pevent.nSegmentId=0;
	pevent.nSubEventType=0;
	pevent.pPacket=packet;
	pevent.szOtherDetails=NULL;
	fnpAddEvent(&pevent);
	return 0;
}

int fn_NetSim_IERP_ProcessRouteReply()
{
	NetSim_PACKET* packet = pstruEventDetails->pPacket;
	IERP_PACKET* reply = (IERP_PACKET*)packet->pstruNetworkData->Packet_RoutingProtocol;
	extractRouteFromreply(reply);
	routePacketFromBuffer();
	if(!IP_COMPARE(packet->pstruNetworkData->szDestIP,
		DEVICE_NWADDRESS(pstruEventDetails->nDeviceId,1)))
		pstruEventDetails->pPacket=NULL;
	else
	{
		packet->pstruNetworkData->szGatewayIP = IP_COPY(DEVICE_NWADDRESS(pstruEventDetails->nDeviceId,1));
		packet->pstruNetworkData->szNextHopIp = IP_COPY(replyGetNextHop(reply,DEVICE_NWADDRESS(pstruEventDetails->nDeviceId,1)));
		packet->nTransmitterId=pstruEventDetails->nDeviceId;
		packet->nReceiverId=fn_NetSim_Stack_GetDeviceId_asIP(packet->pstruNetworkData->szNextHopIp,NULL);
		pstruEventDetails->nEventType=NETWORK_OUT_EVENT;
		fnpAddEvent(pstruEventDetails);
		pstruEventDetails->pPacket=NULL;
	}
	return 0;
}
int routePacketFromBuffer()
{
	NODE_ZRP* zrp=(NODE_ZRP*)DEVICE_NWROUTINGVAR(pstruEventDetails->nDeviceId);
	NODE_IERP* ierp=(NODE_IERP*)zrp->ierp;
	NetSim_PACKET* buffer = ierp->buffer;
	NetSim_PACKET* prev=NULL;
	while(buffer)
	{
		IERP_ROUTE_TABLE* route=checkDestInRouteTable(ierp->routeTable,buffer->pstruNetworkData->szDestIP);
		if(route)
		{
			NetSim_EVENTDETAILS pevent;
			//Route found
			buffer->pstruNetworkData->szGatewayIP = IP_COPY(DEVICE_NWADDRESS(pstruEventDetails->nDeviceId,1));
			if(route->Route)
				buffer->pstruNetworkData->szNextHopIp = IP_COPY(route->Route[0]);
			else
				buffer->pstruNetworkData->szNextHopIp = IP_COPY(route->DestAddr);
			buffer->pstruNetworkData->nTTL=255;
			buffer->nTransmitterId = pstruEventDetails->nDeviceId;
			buffer->nReceiverId = fn_NetSim_Stack_GetDeviceId_asIP(buffer->pstruNetworkData->szNextHopIp,NULL);

			if(prev)
				prev->pstruNextPacket=buffer->pstruNextPacket;
			else
				ierp->buffer=buffer->pstruNextPacket;
			pevent.pPacket=buffer;
			buffer=buffer->pstruNextPacket;
			pevent.pPacket->pstruNextPacket=NULL;

			pevent.dEventTime=pstruEventDetails->dEventTime;
			pevent.dPacketSize=fnGetPacketSize(pevent.pPacket);
			if(pevent.pPacket->pstruAppData)
			{
				pevent.nApplicationId=pevent.pPacket->pstruAppData->nApplicationId;
				pevent.nSegmentId=pevent.pPacket->pstruAppData->nSegmentId;
			}
			else
			{
				pevent.nApplicationId=0;
				pevent.nSegmentId=0;
			}
			pevent.nDeviceId=pstruEventDetails->nDeviceId;
			pevent.nDeviceType=pstruEventDetails->nDeviceType;
			pevent.nEventType=NETWORK_OUT_EVENT;
			pevent.nInterfaceId=1;
			pevent.nPacketId=pevent.pPacket->nPacketId;
			pevent.nProtocolId=NW_PROTOCOL_IPV4;
			pevent.nSubEventType=0;
			pevent.szOtherDetails=NULL;
			fnpAddEvent(&pevent);
			continue;
			
		}
		prev=buffer;
		buffer=buffer->pstruNextPacket;
	}
	return 0;
}



