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

typedef struct stru_socket_list
{
	PNETSIM_SOCKET s;
	_ele* ele;
}SOCKET_LIST, *PSOCKET_LIST;
#define SOCKET_LIST_ALLOC() (PSOCKET_LIST)list_alloc(sizeof(SOCKET_LIST),offsetof(SOCKET_LIST,ele))
#define SOCKET_LIST_NEXT(sl) (PSOCKET_LIST)LIST_NEXT(sl)

static UINT socket_id = 0;

static bool compare_socketaddr(PSOCKETADDRESS sa1, PSOCKETADDRESS sa2)
{
	return !IP_COMPARE(sa1->ip, sa2->ip) &&
		sa1->port == sa2->port;
}

static bool compare_socket(PNETSIM_SOCKET s1, PNETSIM_SOCKET s2)
{
	bool l = false;
	bool r = false;
	if (s1->localAddr)
	{
		if (s2->localAddr)
			l = compare_socketaddr(s1->localAddr, s2->localAddr);
		else
			return false;
	}
	else
	{
		if (s2->localAddr)
			return false;
		else
			l = true;
	}

	if (s1->remoteAddr)
	{
		if (s2->remoteAddr)
			r = compare_socketaddr(s1->remoteAddr, s2->remoteAddr);
		else
			return false;
	}
	else
	{
		if (s2->remoteAddr)
			return false;
		else
			r = true;
	}

	return l && r;
}

static PSOCKET_LIST find_socket_list(NETSIM_ID devid, PNETSIM_SOCKET s)
{
	PTCP_DEV_VAR tcp = GET_TCP_DEV_VAR(devid);
	PSOCKET_LIST sl = tcp->socket_list;
	while (sl)
	{
		if (compare_socket(sl->s, s))
			return sl;
		sl = SOCKET_LIST_NEXT(sl);
	}
	return NULL;
}

void add_to_socket_list(NETSIM_ID devId, PNETSIM_SOCKET s)
{
	PTCP_DEV_VAR tcp = GET_TCP_DEV_VAR(devId);

	PSOCKET_LIST sl = find_socket_list(devId, s);
	if (!sl)
	{
		sl = SOCKET_LIST_ALLOC();
		sl->s = s;
		LIST_ADD_LAST(&tcp->socket_list, sl);
	}
}

void remove_from_socket_list(NETSIM_ID devId, PNETSIM_SOCKET s)
{
	PTCP_DEV_VAR tcp = GET_TCP_DEV_VAR(devId);
	PSOCKET_LIST sl = find_socket_list(devId, s);
	if (sl)
	{
		LIST_FREE(&tcp->socket_list, sl);
	}
}

PNETSIM_SOCKET find_socket(NETSIM_ID devId,
						   NETSIM_IPAddress srcIP,
						   NETSIM_IPAddress destIP,
						   UINT16 srcPort,
						   UINT16 destPort)
{
	NETSIM_SOCKET s;

	SOCKETADDRESS ssa;
	ssa.ip = srcIP;
	ssa.port = srcPort;

	SOCKETADDRESS dsa;
	dsa.ip = destIP;
	dsa.port = destPort;

	s.localAddr = &ssa;
	s.remoteAddr = &dsa;

	PSOCKET_LIST psl = find_socket_list(devId, &s);
	return psl ? psl->s : NULL;
}

PNETSIM_SOCKET get_Remotesocket(NETSIM_ID d, PSOCKETADDRESS addr)
{
	SOCKETADDRESS localAddr;
	NETSIM_SOCKET s;
	memset(&s, 0, sizeof s);
	s.remoteAddr = addr;
	
	localAddr.ip = DEVICE_NWADDRESS(d, 1);
	localAddr.port = 0;
	s.localAddr = &localAddr;

	PSOCKET_LIST psl = find_socket_list(d, &s);
	return psl ? psl->s : NULL;
}

PNETSIM_SOCKET find_socket_at_source(NetSim_PACKET* packet)
{
	return find_socket(packet->nSourceId,
					   packet->pstruNetworkData->szSourceIP,
					   packet->pstruNetworkData->szDestIP,
					   packet->pstruTransportData->nSourcePort,
					   packet->pstruTransportData->nDestinationPort);
}

PNETSIM_SOCKET find_socket_at_dest(NetSim_PACKET* packet)
{
	PNETSIM_SOCKET s;
	s =  find_socket(get_first_dest_from_packet(packet),
					   packet->pstruNetworkData->szDestIP,
					   packet->pstruNetworkData->szSourceIP,
					   packet->pstruTransportData->nDestinationPort,
					   packet->pstruTransportData->nSourcePort);
	if (!s && isSynPacket(packet))
	{
		extern PSOCKETADDRESS anySocketAddr;
		s = get_Remotesocket(get_first_dest_from_packet(packet), anySocketAddr);
	}
	return s;
}

PNETSIM_SOCKET tcp_create_socket()
{
	PNETSIM_SOCKET s = (PNETSIM_SOCKET)calloc(1, sizeof* s);
	s->SocketId = ++socket_id;
	return s;
}

void tcp_close_socket(PNETSIM_SOCKET s, NETSIM_ID devId)
{
	if (s->tcb)
	{
		print_tcp_log("Closing socket with local addr %s:%d, Remote addr %s:%d\n",
					  s->localAddr->ip->str_ip,
					  s->localAddr->port,
					  s->remoteAddr->ip->str_ip,
					  s->remoteAddr->port);
		tcp_change_state(s, TCPCONNECTION_CLOSED);
		free_tcb(s->tcb);
	}
	remove_from_socket_list(devId, s);
	free(s->localAddr);
	if(s->remoteAddr->port)
		free(s->remoteAddr);
	free(s);
}

void tcp_connect(PNETSIM_SOCKET s,
				 PSOCKETADDRESS localAddr,
				 PSOCKETADDRESS remoteAddr)
{
	s->localAddr = localAddr;
	s->remoteAddr = remoteAddr;
	tcp_create_metrics(s,true);
	tcp_active_open(s);
}

void tcp_bind(PNETSIM_SOCKET s,
			  PSOCKETADDRESS addr)
{
	s->remoteAddr = addr;
	create_TCB(s);
	return;
}

void tcp_listen(PNETSIM_SOCKET s,
				void(*listen_callback)(PNETSIM_SOCKET, NetSim_PACKET*))
{
	s->listen_callback = listen_callback;
	tcp_change_state(s, TCPCONNECTION_LISTEN);
}

PNETSIM_SOCKET tcp_accept(PNETSIM_SOCKET s,
						  NetSim_PACKET* p)
{
	PNETSIM_SOCKET localSocket = tcp_create_socket();

	add_to_socket_list(pstruEventDetails->nDeviceId, localSocket);

	PSOCKETADDRESS remotesocketAddr = (PSOCKETADDRESS)calloc(1, sizeof* remotesocketAddr);
	remotesocketAddr->ip = IP_COPY(p->pstruNetworkData->szSourceIP);
	remotesocketAddr->port = p->pstruTransportData->nSourcePort;

	PSOCKETADDRESS localsocketAddr = (PSOCKETADDRESS)calloc(1, sizeof* localsocketAddr);
	localsocketAddr->ip = IP_COPY(p->pstruNetworkData->szDestIP);
	localsocketAddr->port = p->pstruTransportData->nDestinationPort;

	localSocket->localDeviceId = get_first_dest_from_packet(p);
	localSocket->remoteDeviceId = p->nSourceId;

	print_tcp_log("\nDevice %d, Time %0.2lf: creating socket with local addr %s:%d, Remote addr %s:%d",
				  localSocket->localDeviceId,
				  pstruEventDetails->dEventTime,
				  localsocketAddr->ip->str_ip,
				  localsocketAddr->port,
				  remotesocketAddr->ip->str_ip,
				  remotesocketAddr->port);

	localSocket->localAddr = localsocketAddr;
	localSocket->remoteAddr = remotesocketAddr;

	localSocket->sId = fn_NetSim_Socket_GetSocketInterface(localSocket->localDeviceId,
														   0,
														   TX_PROTOCOL_TCP,
														   localSocket->localAddr->port,
														   localSocket->remoteAddr->port);

	tcp_create_metrics(localSocket, false);

	tcp_passive_open(localSocket, s);

	rcv_SYN(localSocket,p);

	return localSocket;
}

void close_all_socket(NETSIM_ID devId)
{
	PTCP_DEV_VAR tcp = GET_TCP_DEV_VAR(devId);
	PSOCKET_LIST sl = tcp->socket_list;
	while (sl)
	{
		tcp_close_socket(sl->s, devId);
		sl = tcp->socket_list;
	}
}

void tcp_close(PNETSIM_SOCKET s)
{
	send_fin(s);
	tcp_change_state(s, TCPCONNECTION_FIN_WAIT_1);
}