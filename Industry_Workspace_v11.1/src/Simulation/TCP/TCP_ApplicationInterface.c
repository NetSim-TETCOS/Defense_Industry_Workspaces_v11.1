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
* Author:    Shashi Kant Suman                                                     *
*                                                                                  *
* ---------------------------------------------------------------------------------*/
#include "main.h"
#include "TCP.h"
#include "List.h"

//Function prototype
void fnConnectCallback(PNETSIM_SOCKET s);
void fnListenCallback(PNETSIM_SOCKET s, NetSim_PACKET*);

PSOCKETADDRESS anySocketAddr = NULL;

void init_anysockaddr()
{
	anySocketAddr = (PSOCKETADDRESS)calloc(1, sizeof* anySocketAddr);
	anySocketAddr->ip = STR_TO_IP("0.0.0.0", 4);
	anySocketAddr->port = 0;
}

PSOCKETADDRESS get_anysockaddr()
{
	return anySocketAddr;
}

void tcp_init(NETSIM_ID d)
{
	if (!get_anysockaddr())
		init_anysockaddr();

	print_tcp_log("\nDevice %d, Time 0.0: Create socket for listen mode with remote addr 0.0.0.0:0", d);

	PNETSIM_SOCKET listenSocket = tcp_create_socket();

	add_to_socket_list(d, listenSocket);

	PSOCKETADDRESS localsocketAddr = (PSOCKETADDRESS)calloc(1, sizeof* localsocketAddr);
	localsocketAddr->ip = DEVICE_NWADDRESS(d,1);
	localsocketAddr->port = 0;

	listenSocket->localAddr = localsocketAddr;

	listenSocket->localDeviceId = d;

	print_tcp_log("Binding listen socket", d);

	tcp_bind(listenSocket, get_anysockaddr());

	print_tcp_log("Listening...", d);
	tcp_listen(listenSocket, fnListenCallback);

	tcp_create_metrics(listenSocket, false);

}

int packet_arrive_from_application_layer()
{
	NetSim_PACKET* packet = GET_PACKET_FROM_APP(false);

	ptrSOCKETINTERFACE sId = pstruEventDetails->szOtherDetails;

	PNETSIM_SOCKET s = find_socket_at_source(packet);
	
	if (!s)
	{
		
		PNETSIM_SOCKET localSocket = tcp_create_socket();

		add_to_socket_list(pstruEventDetails->nDeviceId, localSocket);

		PSOCKETADDRESS remotesocketAddr = (PSOCKETADDRESS)calloc(1, sizeof* remotesocketAddr);
		remotesocketAddr->ip = IP_COPY(packet->pstruNetworkData->szDestIP);
		remotesocketAddr->port = packet->pstruTransportData->nDestinationPort;
		
		PSOCKETADDRESS localsocketAddr = (PSOCKETADDRESS)calloc(1, sizeof* localsocketAddr);
		localsocketAddr->ip = IP_COPY(packet->pstruNetworkData->szSourceIP);
		localsocketAddr->port = packet->pstruTransportData->nSourcePort;

		localSocket->localDeviceId = packet->nSourceId;
		localSocket->remoteDeviceId = get_first_dest_from_packet(packet);
		localSocket->sId = sId;
		localSocket->appId = pstruEventDetails->nApplicationId;

		print_tcp_log("\nDevice %d, Time %0.2lf: Creating socket with local addr %s:%d, Remote addr %s:%d",
					  packet->nSourceId,
					  pstruEventDetails->dEventTime,
					  localsocketAddr->ip->str_ip,
					  localsocketAddr->port,
					  remotesocketAddr->ip->str_ip,
					  remotesocketAddr->port);
		print_tcp_log("Connecting socket...", packet->nSourceId);
		tcp_connect(localSocket,localsocketAddr,remotesocketAddr);

	}
	else if (s->waitFromApp)
	{
		print_tcp_log("\nDevice %d, Time %0.2lf:",
					  packet->nSourceId,
					  pstruEventDetails->dEventTime);

		if (!s->sId)
		{
			s->appId = pstruEventDetails->nApplicationId;
			s->sId = sId;
		}
		else if (s->sId != sId)
		{
			fnNetSimError("Socket id in TCP (%d) and in application (%d) is mismatched.\n",
						  s->sId, sId);
		}
		send_segment(s);
		s->waitFromApp = false;
	}
	else
	{
		//do nothing. Wait for TCP to complete previous operation
	}
	return 0;
}

void fnListenCallback(PNETSIM_SOCKET s, NetSim_PACKET* p)
{
	print_tcp_log("Accepting Connection...");
	PNETSIM_SOCKET localSocket = tcp_accept(s, p);
}

void send_to_application(PNETSIM_SOCKET s, NetSim_PACKET* p)
{
	NetSim_EVENTDETAILS pevent;
	NetSim_PACKET* pr;
	while(true) 
	{
		pr = check_for_other_segment_to_send_from_queue(s);
		if (!pr)
			break;

		if(!pr->pstruAppData)
			continue;

		memcpy(&pevent, pstruEventDetails, sizeof pevent);
		pevent.dPacketSize = pr->pstruAppData->dPacketSize;
		pevent.nApplicationId = pr->pstruAppData->nApplicationId;
		pevent.nEventType = APPLICATION_IN_EVENT;
		pevent.nPacketId = pr->nPacketId;
		pevent.nProtocolId = PROTOCOL_APPLICATION;
		pevent.nSegmentId = pr->pstruAppData->nSegmentId;
		pevent.nSubEventType = 0;
		pevent.pPacket = pr;
		pevent.szOtherDetails = NULL;
		fnpAddEvent(&pevent);

	}
	if (p->pstruAppData) //Send only app layer packet
	{
		memcpy(&pevent, pstruEventDetails, sizeof pevent);
		pevent.dPacketSize = p->pstruAppData->dPacketSize;
		pevent.nApplicationId = p->pstruAppData->nApplicationId;
		pevent.nEventType = APPLICATION_IN_EVENT;
		pevent.nPacketId = p->nPacketId;
		pevent.nProtocolId = PROTOCOL_APPLICATION;
		pevent.nSegmentId = p->pstruAppData->nSegmentId;
		pevent.nSubEventType = 0;
		pevent.pPacket = p;
		pevent.szOtherDetails = NULL;
		fnpAddEvent(&pevent);
	}
}
