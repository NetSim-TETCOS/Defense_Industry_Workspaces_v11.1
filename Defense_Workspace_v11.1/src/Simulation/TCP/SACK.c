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

static UINT get_max_sack_data(PTCP_SEGMENT_HDR hdr)
{
	return (MAX_TCP_HDR_SIZE - hdr->HdrLength - SACK_OPTION_FIX_LEN) / SACKDATA_LEN;
}

UINT32 get_highAck(PNETSIM_SOCKET s)
{
	return s->tcb->SND.UNA;
}

static UINT32 get_highData(PNETSIM_SOCKET s)
{
	return s->tcb->SND.NXT;
}

static UINT32 get_highRxt(PNETSIM_SOCKET s)
{
	return s->tcb->HighRxt;
}

void set_highRxt(PNETSIM_SOCKET s, UINT32 seq)
{
	s->tcb->HighRxt = max(s->tcb->HighRxt, seq);
}

static UINT32 get_rescueRxt(PNETSIM_SOCKET s)
{
	return s->tcb->RescueRxt;
}

void set_rescueRxt(PNETSIM_SOCKET s, UINT32 seq)
{
	s->tcb->RescueRxt = max(s->tcb->RescueRxt, seq);
}

void set_recoveryPoint(PNETSIM_SOCKET s, UINT32 val)
{
	s->tcb->recoveryPoint = val;
}

UINT32 get_recoveryPoint(PNETSIM_SOCKET s)
{
	return s->tcb->recoveryPoint;
}

UINT32 get_pipe(PNETSIM_SOCKET s)
{
	return s->tcb->pipe;
}

void set_sack_option(PNETSIM_SOCKET s, PTCP_SEGMENT_HDR hdr, double time)
{
	UINT maxData;
	UINT i=0;

	if (!s->tcb->isSackPermitted)
		return; // Sack is not permitted

	if (!s->tcb->outOfOrderSegment.size)
		return; // Sack option is not required as no out of order segment.

	maxData = get_max_sack_data(hdr);

	if (!maxData)
		return; // No space is left in TCP header

	PSACK_OPTION sack = (PSACK_OPTION)calloc(1, sizeof* sack);
	sack->type = TCP_OPTION_SACK;

	PQueueInfo queue = s->tcb->outOfOrderSegment.queue;

	PSACKDATA* sackData = calloc(1, sizeof *sackData);;
	UINT len = 0;
	UINT32 lastSeq = 0;
	while (queue)
	{
		PTCP_SEGMENT_HDR h = TCP_GET_SEGMENT_HDR(queue->packet);
		if (lastSeq)
		{
			if (h->SeqNum == lastSeq + len)
			{
				len = get_seg_len(queue->packet);
				sackData[i]->rightEdge = h->SeqNum + len;
				lastSeq = h->SeqNum;
			}
			else if (i < maxData - 1)
			{
				i++;
				len = get_seg_len(queue->packet);
				sackData = (PSACKDATA*)realloc(sackData, (i + 1) * sizeof* sackData);
				sackData[i] = (PSACKDATA)calloc(1, sizeof* sackData[i]);
				sackData[i]->leftEdge = h->SeqNum;
				sackData[i]->rightEdge = h->SeqNum + len;
				lastSeq = h->SeqNum;
			}
			else
			{
				break; // Reached Max Sack Data count
			}
		}
		else if (i < maxData)
		{
			len = get_seg_len(queue->packet);
			sackData[i] = (PSACKDATA)calloc(1, sizeof* sackData[i]);
			sackData[i]->leftEdge = h->SeqNum;
			sackData[i]->rightEdge = h->SeqNum + len;
			lastSeq = h->SeqNum;
		}
		else
		{
			break; // Reached Max Sack Data count
		}

		queue = queue->next;
	}

	sack->sackData = sackData;
	sack->len = SACK_OPTION_LEN(i + 1);

	set_tcp_option(hdr, sack, TCP_OPTION_SACK, sack->len);
}

static void update_sack_flag(PNETSIM_SOCKET s, UINT32 left, UINT32 right)
{
	PQueueInfo queue = s->tcb->retransmissionQueue.queue;
	while (queue)
	{
		PTCP_SEGMENT_HDR hdr = TCP_GET_SEGMENT_HDR(queue->packet);
		if (hdr->SeqNum >= left && hdr->SeqNum <= right)
			queue->isSacked = true;
		queue = queue->next;
	}
}

static void update_scoreboard(PNETSIM_SOCKET s, UINT left, UINT right, UINT index)
{
	//Store current to prev
	if (!index)
		memcpy(&s->tcb->prevScoreboard, &s->tcb->scoreboard, sizeof s->tcb->prevScoreboard);

	s->tcb->scoreboard.leftEdge[index] = left;
	s->tcb->scoreboard.rightEdge[index] = right;
}

/*
This routine returns whether the given sequence number is
considered to be lost.  The routine returns true when either
DupThresh discontiguous SACKed sequences have arrived above
'SeqNum' or more than (DupThresh - 1) * SMSS bytes with sequence
numbers greater than 'SeqNum' have been SACKed.  Otherwise, the
routine returns false.
*/
bool tcp_sack_isLost(PNETSIM_SOCKET s, UINT seqNum)
{
	UINT i;
	UINT SMSS = s->tcb->get_MSS(s);
	UINT check = seqNum + SMSS;

	if (s->tcb->scoreboard.leftEdge[0] >= check)
	{
		UINT c = 0;
		//Discontinuous
		for (i = 0; i < 4; i++)
		{
			c += (s->tcb->scoreboard.rightEdge[i] - s->tcb->scoreboard.leftEdge[i]) / SMSS;
			if (c >= TCP_DupThresh)
				return true;
			if (s->tcb->scoreboard.rightEdge[i] > (TCP_DupThresh - 1)*SMSS + seqNum)
				return true;
		}
	}
	return false;
}

void tcp_sack_setPipe(PNETSIM_SOCKET s)
{
	UINT32 highAck = get_highAck(s);
	UINT32 highData = get_highData(s);
	UINT32 highRxt = get_highRxt(s);
	s->tcb->pipe = 0;
	UINT SMSS = s->tcb->get_MSS(s);
	UINT32 s1;

	for (s1 = highAck; s1 < highData; s1 += SMSS)
	{
		if (!tcp_sack_isLost(s, s1))
			s->tcb->pipe += SMSS;

		if (s1 <= highRxt)
			s->tcb->pipe += SMSS;
	}
}

UINT32 tcp_sack_nextSeg(PNETSIM_SOCKET s)
{
	NetSim_PACKET* p;
	UINT32 s2;
	UINT32 highRxt = get_highRxt(s);
	PQueueInfo queue = s->tcb->retransmissionQueue.queue;
	while (queue)
	{
		p = queue->packet;

		//Rule 1
		if (!queue->isSacked)
		{
			PTCP_SEGMENT_HDR hdr = TCP_GET_SEGMENT_HDR(p);
			s2 = hdr->SeqNum;
			if (s2 > highRxt &&
				tcp_sack_isLost(s, s2))
			{
				PQueueInfo q = queue->next;
				while (q)
				{
					if (q->isSacked)
						return s2;
					q = q->next;
				}
			}
		}
		queue = queue->next;
	}

	//Rule 2
	UINT32 seq = s->tcb->SND.NXT;
	p = fn_NetSim_Socket_GetPacketFromInterface(s->sId, 0);
	s->tcb->SND.WND = s->tcb->get_WND(s);
	if (p)
	{
		UINT32 l = (UINT32)p->pstruAppData->dPacketSize;
		if (seq + l <= s->tcb->SND.WND + s->tcb->SND.UNA)
			return s->tcb->SND.NXT;
	}

	//Rule 3
	queue = s->tcb->retransmissionQueue.queue;
	while (queue)
	{
		p = queue->packet;

		//Rule 1
		if (!queue->isSacked)
		{
			PTCP_SEGMENT_HDR hdr = TCP_GET_SEGMENT_HDR(p);
			s2 = hdr->SeqNum;
			if (s2 > highRxt)
			{
				PQueueInfo q = queue->next;
				while (q)
				{
					if (q->isSacked)
						return s2;
					q = q->next;
				}
			}
		}
		queue = queue->next;
	}

	return 0;
}

void receive_sack_option(PNETSIM_SOCKET s, PTCP_SEGMENT_HDR hdr)
{
	s->tcb->isSackOption = false;

	if (!s->tcb->isSackPermitted)
		return; // Sack is not permitted

	PSACK_OPTION sack = get_tcp_option(hdr, TCP_OPTION_SACK);
	if (!sack)
		return; // No sack option is present

	UINT c = get_sack_data_count(sack);

	if (!c)
		return; //No sack data present

	s->tcb->isSackOption = true;

	UINT i;
	for (i = 0; i < c; i++)
	{
		PSACKDATA data = sack->sackData[i];
		UINT32 left = data->leftEdge;
		UINT32 right = data->rightEdge;
		update_sack_flag(s, left, right);
		update_scoreboard(s, left, right, i);
	}

	for(;i<4;i++)
		update_scoreboard(s, 0, 0, i);
}

void tcp_sack_fastRetransmit(PNETSIM_SOCKET s)
{
	if (s->tcb->isSackOption)
	{
		set_recoveryPoint(s, get_highData(s));
		tcp_sack_setPipe(s);
	}
}

bool tcp_sack_lossRecoveryPhase(PNETSIM_SOCKET s)
{
	if (s->tcb->SEG.ACK > s->tcb->recoveryPoint)
		return false; //End loss recovery phase
	else
	{
		tcp_sack_setPipe(s);

		UINT32 wnd = s->tcb->get_WND(s);
		UINT32 pipe = get_pipe(s);
		UINT32 mss = s->tcb->get_MSS(s);

		while (wnd >= mss + pipe)
		{
			UINT32 nextSeg = tcp_sack_nextSeg(s);
			if (nextSeg)
			{
				UINT32 highData = get_highData(s);
				if (nextSeg < highData)
				{
					resend_segment_without_timeout(s, nextSeg);
					set_highRxt(s, nextSeg);
				}
				else
				{
					UINT32 len;
					UINT32 prev = s->tcb->SND.NXT;
					send_segment(s);
					len = s->tcb->SND.NXT - prev;
					pipe += len;
					s->tcb->pipe += len;
				}
			}
			else
			{
				return true; 
			}
		}
		return true; //Continue in Loss recovery phase
	}
}
