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
#include "TCP_Header.h"

void send_segment(PNETSIM_SOCKET s)
{
	bool closeApp=false;
	PTCB t = s->tcb;
	UINT32 seq = t->SND.NXT;
	if (!s->sId)
		fnNetSimError("SOCKETINTERFACE is NULL for TCP connection %s:%d %s:%d in function %s\n",
					  s->localAddr->ip->str_ip,
					  s->localAddr->port,
					  s->remoteAddr->ip->str_ip,
					  s->remoteAddr->port,
					  __FUNCTION__);
	NetSim_PACKET* p = fn_NetSim_Socket_GetPacketFromInterface(s->sId, 0);
	if (!p)
		s->waitFromApp = true; //Enable flag to signal application waiting.

	t->SND.WND = window_scale_get_cwnd(s);
	assert(t->SND.WND > t->get_MSS(s));
	while (p)
	{
		UINT32 l = (UINT32)p->pstruAppData->dPacketSize;
		if (seq + l <= t->SND.WND + t->SND.UNA)
		{
			NetSim_PACKET* pt = fn_NetSim_Socket_GetPacketFromInterface(s->sId, 1);
			if (pt->pstruAppData->nAppEndFlag)
				closeApp = true; //Enable flag to signal application closing

			add_tcp_hdr(pt, s);
			t->SND.NXT += l;
			seq += l;

			s->tcpMetrics->segmentSent++;

			send_to_network(pt, s);
			add_timeout_event(s, pt);
		}
		else
			break; //No more segment allowed to send

		p = fn_NetSim_Socket_GetPacketFromInterface(s->sId, 0);
	}

	if (closeApp)
		tcp_close(s);
}

void resend_segment(PNETSIM_SOCKET s, NetSim_PACKET* expired)
{
	PTCP_SEGMENT_HDR hdr = TCP_GET_SEGMENT_HDR(expired);
	PTCP_QUEUE q = &s->tcb->retransmissionQueue;

	NetSim_PACKET* p = get_segment_from_queue(q, hdr->SeqNum);
	
	if (p)
	{	
		s->tcpMetrics->segmentRetransmitted++;
		send_to_network(p, s);
		add_timeout_event(s, p);
	}
}

void resend_segment_without_timeout(PNETSIM_SOCKET s, UINT seq)
{
	bool isSacked = false;
	PTCP_QUEUE q = &s->tcb->retransmissionQueue;

	NetSim_PACKET* p = get_copy_segment_from_queue(q, seq, &isSacked);

	if (s->tcb->isSackPermitted &&
		isSacked)
	{
		//Don't retransmit
		fn_NetSim_Packet_FreePacket(p);
		p = NULL;
	}

	if (p)
	{
		set_highRxt(s, seq);
		set_rescueRxt(s, seq);
		s->tcpMetrics->segmentRetransmitted++;
		send_to_network(p, s);
	}
}
