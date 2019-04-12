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
#include "TCP_Header.h"

//Function prototype
static void free_queue(PTCP_QUEUE queue);


UINT32 calculate_initial_seq_num(double time)
{
	return max(1,(UINT32)(time/4.0));
}

char* state_to_str(TCP_CONNECTION_STATE state)
{
	switch (state)
	{
	case TCPCONNECTION_CLOSED:
		return "Closed";
	case TCPCONNECTION_LISTEN:
		return "Listen";
	case TCPCONNECTION_CLOSE_WAIT:
		return "Close-Wait";
	case TCPCONNECTION_CLOSING:
		return "Closing";
	case TCPCONNECTION_ESTABLISHED:
		return "Established";
	case TCPCONNECTION_FIN_WAIT_1:
		return "Fin-Wait-1";
	case TCPCONNECTION_FIN_WAIT_2:
		return "Fin-wait-2";
	case TCPCONNECTION_LAST_ACK:
		return "Last-Ack";
	case TCPCONNECTION_SYN_RECEIVED:
		return "Syn-received";
	case TCPCONNECTION_TIME_WAIT:
		return "Time-wait";
	case TCPCONNECTION_SYN_SENT:
		return "Syn-sent";
	default:
		return "Unknown";
	}
}

static void set_tcp_variant(PNETSIM_SOCKET s, PTCP_DEV_VAR t)
{
	s->tcb->variant = t->tcpVariant;
	switch (s->tcb->variant)
	{
	case TCPVariant_OLDTAHOE:
	case TCPVariant_TAHOE:
	case TCPVariant_RENO:
	case TCPVariant_NEWRENO:
	case TCPVariant_BIC:
	case TCPVariant_CUBIC:
		congestion_setcallback(s);
		break;
	default:
		fnNetSimError("Unknown tcp variant %d in %s\n",
					  t->tcpVariant,
					  __FUNCTION__);
		break;
	}
}

static void setSackPermitted(PNETSIM_SOCKET s, PTCP_DEV_VAR t)
{
	s->tcb->isSackPermitted = t->isSackPermitted;
}

static void set_timestamp_option(PNETSIM_SOCKET s, PTCP_DEV_VAR t)
{
	s->tcb->isTSopt = t->isTimestampOpt;
}

void create_TCB(PNETSIM_SOCKET s)
{
	PTCP_DEV_VAR tcp = GET_TCP_DEV_VAR(s->localDeviceId);
	PTCB tcb = (PTCB)calloc(1, sizeof* tcb);
	s->tcb = tcb;

	set_tcp_variant(s, tcp);

	set_ack_type(s, tcp);

	setSackPermitted(s, tcp);

	set_window_scaling_option(s, tcp);

	set_timestamp_option(s, tcp);
	
	TCP_RTO(tcb) = calculate_RTO(0,
								 &TCP_SRTT(tcb),
								 &TCP_RTTVAR(tcb));

	print_tcp_log("New RTO = %0.2lf", TCP_RTO(tcb));

	tcb->timeWaitTimer = tcp->timeWaitTimer;

	tcb->ISS = calculate_initial_seq_num(pstruEventDetails->dEventTime);
	tcb->SND.WND = tcb->get_WND(s);
	tcb->RCV.WND = tcb->get_WND(s);
	
	//Start with closed state
	tcb->tcp_state = TCPCONNECTION_CLOSED;
	tcb->tcp_prev_state = TCPCONNECTION_CLOSED;

	//Set initial RMSS and SMSS size
	s->tcb->init_congestionalgo(s);
}

void free_tcb(PTCB tcb)
{
	free_queue(&tcb->retransmissionQueue);
	free(tcb);
}

void delete_tcb(PNETSIM_SOCKET s)
{
	free_tcb(s->tcb);
	s->tcb = NULL;
}

void tcp_change_state(PNETSIM_SOCKET s, TCP_CONNECTION_STATE state)
{
	PTCB tcb = s->tcb;
	if (tcb->tcp_state != state)
	{
		tcb->tcp_prev_state = tcb->tcp_state;
		tcb->tcp_state = state;
		print_tcp_log("TCP state is changed to \"%s\" from \"%s\".",
					  state_to_str(tcb->tcp_state),
					  state_to_str(tcb->tcp_prev_state));
	}
}

static void free_queue(PTCP_QUEUE queue)
{
	PQueueInfo info = queue->queue;
	while (info)
	{
		NetSim_PACKET* p = info->packet;
		PQueueInfo i = info;
		info = info->next;
		free(i);
		fn_NetSim_Packet_FreePacket(p);
	}
}

void add_packet_to_queue(PTCP_QUEUE queue, NetSim_PACKET* packet, double time)
{
	PQueueInfo info = queue->queue;
	
	PQueueInfo ninfo = (PQueueInfo)calloc(1, sizeof* ninfo);
	ninfo->packet = packet;
	ninfo->time = time;

	if (info)
	{
		while (info->next)
			info = info->next;
		info->next = ninfo;
	}
	else
	{
		queue->queue = ninfo;
	}
	queue->size += (UINT)packet->pstruTransportData->dPacketSize;
}

bool isSegmentInQueue(PTCP_QUEUE queue, NetSim_PACKET* packet)
{
	PTCP_SEGMENT_HDR hdr = TCP_GET_SEGMENT_HDR(packet);
	PQueueInfo info = queue->queue;
	while (info)
	{
		NetSim_PACKET* p = info->packet;
		PTCP_SEGMENT_HDR h = TCP_GET_SEGMENT_HDR(p);
		if (hdr->SeqNum == h->SeqNum)
			return true;
		info = info->next;
	}
	return false;
}

NetSim_PACKET* get_segment_from_queue(PTCP_QUEUE queue, UINT32 seqNo)
{
	PQueueInfo info = queue->queue;
	PQueueInfo pr=NULL;
	while (info)
	{
		NetSim_PACKET* p = info->packet;
		PTCP_SEGMENT_HDR hdr = TCP_GET_SEGMENT_HDR(p);
		if (hdr->SeqNum == seqNo)
		{
			queue->size -= (UINT)p->pstruTransportData->dPacketSize;
			if (pr)
			{
				pr->next = info->next;
				free(info);
			}
			else
			{
				queue->queue = info->next;
				free(info);
			}
			return p;
		}
		pr = info;
		info = info->next;
	}
	return NULL;
}

NetSim_PACKET* get_copy_segment_from_queue(PTCP_QUEUE queue, UINT32 seqNo, bool* isSacked)
{
	PQueueInfo info = queue->queue;
	PQueueInfo pr = NULL;
	while (info)
	{
		NetSim_PACKET* p = info->packet;
		PTCP_SEGMENT_HDR hdr = TCP_GET_SEGMENT_HDR(p);
		if (hdr->SeqNum == seqNo)
		{
			NetSim_PACKET* pret = fn_NetSim_Packet_CopyPacket(p);
			pret->pstruNextPacket = NULL;
			*isSacked = info->isSacked;
			return pret;
		}
		pr = info;
		info = info->next;
	}
	return NULL;
}


UINT32 get_seg_len(NetSim_PACKET* p)
{
	PTCP_SEGMENT_HDR hdr = TCP_GET_SEGMENT_HDR(p);
	if (hdr->Syn)
		return 1;
	else
		return (UINT32)(p->pstruTransportData->dPayload);
}

void delete_segment_from_queue(PTCP_QUEUE queue, UINT32 ackNo)
{
	PQueueInfo info = queue->queue;
	PQueueInfo pr = NULL;
	while (info)
	{
		NetSim_PACKET* p = info->packet;
		PTCP_SEGMENT_HDR hdr = TCP_GET_SEGMENT_HDR(p);
		if (hdr->SeqNum + get_seg_len(p) <= ackNo)
		{
			//Delete
			queue->size -= (UINT)p->pstruTransportData->dPacketSize;
			if (pr)
			{
				pr->next = info->next;
				free(info);
				info = pr->next;
			}
			else
			{
				queue->queue = info->next;
				free(info);
				info = queue->queue;
			}
			fn_NetSim_Packet_FreePacket(p);
			continue;
		}
		pr = info;
		info = info->next;
	}
}

bool isAnySegmentInQueue(PTCP_QUEUE queue)
{
	return queue->size > 0;
}

void delete_all_segment_from_queue(PTCP_QUEUE queue)
{
	free_queue(queue);
}

void update_seq_num_on_receiving(PNETSIM_SOCKET s,
								 NetSim_PACKET* p)
{
	PTCP_SEGMENT_HDR hdr = TCP_GET_SEGMENT_HDR(p);
	PTCB t = s->tcb;
	if (t)
	{
		t->SEG.ACK = hdr->AckNum;
		t->SEG.LEN = get_seg_len(p);
		t->SEG.SEQ = hdr->SeqNum;
		t->SEG.WND = hdr->Window;
	}
}

double get_RTT(PTCB tcb, UINT ackNo)
{
	PQueueInfo info = tcb->retransmissionQueue.queue;
	while (info)
	{
		NetSim_PACKET* p = info->packet;
		PTCP_SEGMENT_HDR hdr = TCP_GET_SEGMENT_HDR(p);
		if (hdr->SeqNum + get_seg_len(p) == ackNo)
		{
			return pstruEventDetails->dEventTime - info->time;
		}
		info = info->next;
	}
	return 0;
}

NetSim_PACKET* check_for_other_segment_to_send_from_queue(PNETSIM_SOCKET s)
{
	PTCP_QUEUE queue = &s->tcb->outOfOrderSegment;
	PQueueInfo info = queue->queue;
	NetSim_PACKET* pr = NULL;
	if (info)
	{
		pr = info->packet;
		PTCP_SEGMENT_HDR h = TCP_GET_SEGMENT_HDR(pr);
		if (h->SeqNum < s->tcb->RCV.NXT)
		{
			//Delete
			queue->size -= (UINT)pr->pstruTransportData->dPacketSize;
			queue->queue = info->next;
			free(info);
		}
		else
			pr = NULL;
	}
	return pr;
}

void check_segment_in_queue(PNETSIM_SOCKET s)
{
	PTCP_QUEUE queue = &s->tcb->outOfOrderSegment;
	PQueueInfo info = queue->queue;
	while (info)
	{
		NetSim_PACKET* pr = NULL;
		pr = info->packet;
		PTCP_SEGMENT_HDR h = TCP_GET_SEGMENT_HDR(pr);
		if (h->SeqNum == s->tcb->RCV.NXT)
		{
			s->tcb->RCV.NXT += get_seg_len(pr);
		}
		info = info->next;
	}
}

void set_timestamp_value(PNETSIM_SOCKET s,
						 PTCP_SEGMENT_HDR hdr,
						 PTSopt opt)
{
	if (hdr->Syn)
	{
		if (opt && s->tcb->isTSopt)
			s->tcb->isTSopt = true;
		else
			s->tcb->isTSopt = false;
	}

	if (opt && s->tcb->isTSopt)
	{
		s->tcb->TSVal = opt->TSval;
	}

}
