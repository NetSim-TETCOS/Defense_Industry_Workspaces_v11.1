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

//#define _LOG_INCOMING_

#ifdef _LOG_INCOMING_
static FILE* fp;
#endif

static bool Validate_segment(PNETSIM_SOCKET s)
{
	/*RFC 793 Sept 1981 Page 69
	There are four cases for the acceptability test for an incoming
	segment:

	Segment Receive  Test
	Length  Window
	------- -------  -------------------------------------------

	0       0		SEG.SEQ = RCV.NXT

	0       >0		RCV.NXT =< SEG.SEQ < RCV.NXT+RCV.WND

	>0      0		not acceptable

	>0      >0		RCV.NXT =< SEG.SEQ < RCV.NXT+RCV.WND
					or RCV.NXT =< SEG.SEQ+SEG.LEN-1 < RCV.NXT+RCV.WND
	*/
	PTCB t = s->tcb;
	if (!t->SEG.LEN && !t->RCV.WND)
		return t->SEG.SEQ == t->RCV.NXT;
	else if (!t->SEG.LEN && t->RCV.WND > 0)
		return (t->RCV.NXT <= t->SEG.SEQ) &&
		(t->SEG.SEQ < t->RCV.NXT + t->RCV.WND);
	else if (t->SEG.LEN > 0 && t->RCV.WND == 0)
		return false;
	else if (t->SEG.LEN > 0 && t->RCV.WND > 0)
		return ((t->RCV.NXT <= t->SEG.SEQ) &&
		(t->SEG.SEQ < t->RCV.NXT + t->RCV.WND)) ||
				((t->RCV.NXT <= t->SEG.SEQ + t->SEG.LEN - 1) &&
		(t->SEG.SEQ + t->SEG.LEN - 1 < t->RCV.NXT + t->RCV.WND));

	assert(false); // TCP is now mad. Enjoy debugging....:(
	return false;
}

static void process_rst_packet(PNETSIM_SOCKET s, NetSim_PACKET* p)
{
	switch (s->tcb->tcp_state)
	{
	case TCPCONNECTION_SYN_RECEIVED:
	{
		delete_all_segment_from_queue(&s->tcb->retransmissionQueue);
		if (s->tcb->tcp_prev_state == TCPCONNECTION_LISTEN)
		{
			//Passive open case
			tcp_change_state(s, TCPCONNECTION_LISTEN);
		}
		else
		{
			//Active open case
			print_tcp_log("Connection Refused for local addr %s:%d, remote addr %s:%d",
						  s->localAddr->ip->str_ip,
						  s->localAddr->port,
						  s->remoteAddr->ip->str_ip,
						  s->remoteAddr->port);
			tcp_close_socket(s, s->localDeviceId);
		}
	}
	break;
	case TCPCONNECTION_ESTABLISHED:
	case TCPCONNECTION_FIN_WAIT_1:
	case TCPCONNECTION_FIN_WAIT_2:
	case TCPCONNECTION_CLOSE_WAIT:
	{
		print_tcp_log("Connection Reset for local addr %s:%d, remote addr %s:%d",
					  s->localAddr->ip->str_ip,
					  s->localAddr->port,
					  s->remoteAddr->ip->str_ip,
					  s->remoteAddr->port);
	}
	case TCPCONNECTION_CLOSING:
	case TCPCONNECTION_LAST_ACK:
	case TCPCONNECTION_TIME_WAIT:
		tcp_close_socket(s, s->localDeviceId);
		break;
	}
	fn_NetSim_Packet_FreePacket(p);
	pstruEventDetails->pPacket = NULL;
}

static void process_syn_packet(PNETSIM_SOCKET s, NetSim_PACKET* p)
{
	switch (s->tcb->tcp_state)
	{
	case TCPCONNECTION_SYN_RECEIVED:
		//Correction made by RFC 1122 Page 94
		tcp_change_state(s, TCPCONNECTION_LISTEN);
		break;
	case TCPCONNECTION_ESTABLISHED:
	case TCPCONNECTION_FIN_WAIT_1:
	case TCPCONNECTION_FIN_WAIT_2:
	case TCPCONNECTION_CLOSE_WAIT:
	case TCPCONNECTION_CLOSING:
	case TCPCONNECTION_LAST_ACK:
	case TCPCONNECTION_TIME_WAIT:
		print_tcp_log("Connection Reset for local addr %s:%d, remote addr %s:%d",
					  s->localAddr->ip->str_ip,
					  s->localAddr->port,
					  s->remoteAddr->ip->str_ip,
					  s->remoteAddr->port);
		tcp_close_socket(s, s->localDeviceId);
		break;
	}
	fn_NetSim_Packet_FreePacket(p);
	pstruEventDetails->pPacket = NULL;
}

static bool process_ack_in_establishedState(PNETSIM_SOCKET s, PTCP_SEGMENT_HDR hdr)
{
#ifdef _LOG_INCOMING_
	fprintf(fp, "%lf,,,,,%d,\n",
			pstruEventDetails->dEventTime,
			s->tcb->SEG.ACK);
	fflush(fp);
#endif
	
	PTCB t = s->tcb;

	//Call Sack
	receive_sack_option(s, hdr);

	if (s->tcb->isTSopt)
	{
		//Store the timestamp option based on peer device
		PTSopt tsopt = get_tcp_option(hdr,
									  TCP_OPTION_TIMESTAMP);
		set_timestamp_value(s, hdr, tsopt);
	}

	//Call congestion algorithm
	t->ack_received(s);


	s->tcpMetrics->ackReceived++;

	if (t->SND.UNA < t->SEG.ACK &&
		t->SEG.ACK <= t->SND.NXT) //RFC 793
	{

		//Update the RTO
		TCP_RTO(s->tcb) = calculate_RTO(get_RTT(s->tcb,t->SEG.ACK),
									 &TCP_SRTT(s->tcb),
									 &TCP_RTTVAR(s->tcb));


		t->SND.UNA = t->SEG.ACK;
		delete_segment_from_queue(&t->retransmissionQueue, t->SEG.ACK);
		if (t->SND.WL1 < t->SEG.SEQ ||
			(t->SND.WL1 == t->SEG.SEQ &&
			 t->SND.WL2 <= t->SEG.ACK))
		{
			t->SND.WND = t->SEG.WND;
			t->SND.WL1 = t->SEG.SEQ;
			t->SND.WL2 = t->SEG.ACK;
		}
		return true;
	}
	//else  if (t->SEG.ACK < t->SND.UNA) //RFC 793
	else  if (t->SEG.ACK <= t->SND.UNA) //RFC 1122 Page 94
	{
		//Duplicate ack. Ignore
		s->tcpMetrics->dupAckReceived++;
		return false;
	}
	else if (t->SEG.ACK > t->SND.NXT)
	{
		send_ack(s, pstruEventDetails->dEventTime, t->SEG.SEQ, t->SEG.ACK);
		return false;
	}
	
	return false;
}

static void process_ack_packet(PNETSIM_SOCKET s, NetSim_PACKET* p)
{
	PTCP_SEGMENT_HDR hdr = TCP_GET_SEGMENT_HDR(p);
	bool isContinueProcessing=false;
	switch (s->tcb->tcp_state)
	{
	case TCPCONNECTION_SYN_RECEIVED:
	{
		if (!(s->tcb->SND.UNA <= s->tcb->SEG.ACK &&
			s->tcb->SEG.ACK <= s->tcb->SND.NXT))
		{
			send_rst(s, p, 1);
			break;
		}
		else
		{
			//Correction made by RFC 1122 page 94
			s->tcb->SND.WND = s->tcb->SEG.WND;
			s->tcb->SND.WL1 = s->tcb->SEG.SEQ;
			s->tcb->SND.WL2 = s->tcb->SEG.ACK;
			
			tcp_change_state(s, TCPCONNECTION_ESTABLISHED);
		}
	}
	case TCPCONNECTION_ESTABLISHED:
	{
		isContinueProcessing = process_ack_in_establishedState(s, hdr);
		if (isContinueProcessing)
			send_segment(s);
	}
	break;
	case TCPCONNECTION_FIN_WAIT_1:
		isContinueProcessing = process_ack_in_establishedState(s, hdr);
		if (isContinueProcessing)
			send_segment(s);
		if(hdr->Fin)
			tcp_change_state(s, TCPCONNECTION_FIN_WAIT_2);
		break;
	case TCPCONNECTION_FIN_WAIT_2:
	case TCPCONNECTION_CLOSE_WAIT:
		isContinueProcessing = process_ack_in_establishedState(s, hdr);
		if (isContinueProcessing)
			send_segment(s);
		break;
	case TCPCONNECTION_CLOSING:
		isContinueProcessing = process_ack_in_establishedState(s, hdr);
		if (isContinueProcessing)
			send_segment(s);
		if (hdr->Fin)
		{
			start_timewait_timer(s);
			tcp_change_state(s, TCPCONNECTION_TIME_WAIT);
		}
		break;
	case TCPCONNECTION_LAST_ACK:
		tcp_change_state(s, TCPCONNECTION_CLOSED);
		delete_tcb(s);
		break;
	case TCPCONNECTION_TIME_WAIT:
		send_fin_ack(s, pstruEventDetails->dEventTime, s->tcb->SEG.SEQ, s->tcb->SEG.ACK);
		start_timewait_timer(s);
		s->tcb->istimewaittimerrestarted = true;
		break;
	default:
		fnNetSimError("Unknown TCP connection state %d in %s",
					  s->tcb->tcp_state,
					  __FUNCTION__);
		break;
	}
	fn_NetSim_Packet_FreePacket(p);
	pstruEventDetails->pPacket = NULL;
}

static void process_fin_packet(PNETSIM_SOCKET s, NetSim_PACKET* p)
{
	switch (s->tcb->tcp_state)
	{
	case TCPCONNECTION_CLOSED:
	case TCPCONNECTION_LISTEN:
	case TCPCONNECTION_SYN_SENT:
		goto DISCARD_SEGMENT;
	}

	switch (s->tcb->tcp_state)
	{
	case TCPCONNECTION_SYN_RECEIVED:
	case TCPCONNECTION_ESTABLISHED:
		send_fin_ack(s, pstruEventDetails->dEventTime, s->tcb->SND.NXT, s->tcb->RCV.NXT);
		tcp_change_state(s, TCPCONNECTION_CLOSE_WAIT);
		//User will call close
		send_fin(s);
		tcp_change_state(s, TCPCONNECTION_LAST_ACK);
		print_tcp_log("Canceling all timer.");
		s->tcb->isOtherTimerCancel = true;
		break;
	case TCPCONNECTION_FIN_WAIT_1:
		tcp_change_state(s, TCPCONNECTION_CLOSING);
		send_fin_ack(s, pstruEventDetails->dEventTime, s->tcb->SND.NXT, s->tcb->RCV.NXT);
		break;
	case TCPCONNECTION_FIN_WAIT_2:
		tcp_change_state(s, TCPCONNECTION_TIME_WAIT);
		start_timewait_timer(s);
		send_ack(s, pstruEventDetails->dEventTime, s->tcb->SND.NXT+1, s->tcb->SEG.SEQ+1);
		break;
	case TCPCONNECTION_CLOSE_WAIT:
	case TCPCONNECTION_CLOSING:
	case TCPCONNECTION_LAST_ACK:
		// No operation
		break;
	case TCPCONNECTION_TIME_WAIT:
		s->tcb->istimewaittimerrestarted = true;
		start_timewait_timer(s);
		break;
	default:
		fnNetSimError("Unknown tcp state %d", s->tcb->tcp_state);
		break;
	}
DISCARD_SEGMENT:
	fn_NetSim_Packet_FreePacket(p);
	pstruEventDetails->pPacket = NULL;
}

static bool check_segment(PNETSIM_SOCKET s, NetSim_PACKET* p)
{
	PTCP_SEGMENT_HDR hdr = TCP_GET_SEGMENT_HDR(p);
	if (s->tcb->RCV.NXT == hdr->SeqNum)
	{
		s->tcb->RCV.NXT += s->tcb->SEG.LEN;
		//Check from queue
		check_segment_in_queue(s);
		return true;
	}
	else if ((s->tcb->RCV.NXT > hdr->SeqNum))
	{
		//Duplicate segment
		s->tcpMetrics->dupSegmentReceived++;
		return false;
	}
	else
	{
		s->tcpMetrics->outOfOrderSegmentReceived++;
		if (isSegmentInQueue(&s->tcb->outOfOrderSegment, p))
			s->tcpMetrics->dupSegmentReceived++;
		else
			add_packet_to_queue(&s->tcb->outOfOrderSegment, p, pstruEventDetails->dEventTime);
		return false;
	}
}

static bool process_segment_text(PNETSIM_SOCKET s, NetSim_PACKET* p)
{
	PTCP_SEGMENT_HDR hdr = TCP_GET_SEGMENT_HDR(p);

#ifdef _LOG_INCOMING_
	fprintf(fp, "%lf,%d,,,,,\n",
			pstruEventDetails->dEventTime,
			s->tcb->SEG.SEQ);
	fflush(fp);
#endif

	s->tcpMetrics->segmentReceived++;

	if (s->tcb->isTSopt)
	{
		//Store the timestamp option based on peer device
		PTSopt tsopt = get_tcp_option(hdr,
									  TCP_OPTION_TIMESTAMP);
		set_timestamp_value(s, hdr, tsopt);
	}

	if (s->tcb->tcp_state == TCPCONNECTION_ESTABLISHED ||
		s->tcb->tcp_state == TCPCONNECTION_FIN_WAIT_1 ||
		s->tcb->tcp_state == TCPCONNECTION_FIN_WAIT_2)
	{
		bool isinorder = check_segment(s, p);
		
		if((!isinorder || send_ack_or_not(s)) && !hdr->Fin)
			send_ack(s, pstruEventDetails->dEventTime, s->tcb->SND.NXT, s->tcb->RCV.NXT);
		
		if(isinorder)
			send_to_application(s, p);
		
		return true;
	}
	else if (hdr->Fin)
	{
		return true; //FIN Packet
	}
	else if (s->tcb->tcp_state == TCPCONNECTION_CLOSE_WAIT ||
			 s->tcb->tcp_state == TCPCONNECTION_CLOSING ||
			 s->tcb->tcp_state == TCPCONNECTION_LAST_ACK ||
			 s->tcb->tcp_state == TCPCONNECTION_TIME_WAIT)
	{
		fn_NetSim_Packet_FreePacket(p);
		pstruEventDetails->pPacket = NULL;
		return false;
	}
	else if (s->tcb->tcp_state == TCPCONNECTION_SYN_RECEIVED)
	{
		// May be ack got error. Ignore segment
		fn_NetSim_Packet_FreePacket(p);
		pstruEventDetails->pPacket = NULL;
		return false;
	}
	else
	{
		fnNetSimError("Unknown TCP connection state %s in %s",
					  state_to_str(s->tcb->tcp_state),
					  __FUNCTION__);
		fn_NetSim_Packet_FreePacket(p);
		pstruEventDetails->pPacket = NULL;
		return false;
	}
}

void packet_arrives_at_incoming_tcp(PNETSIM_SOCKET s, NetSim_PACKET* p)
{
	bool isContinue;

#ifdef _LOG_INCOMING_
	if (!fp)
	{
		fp = fopen("IncomingTCP.csv", "w");
		fprintf(fp, "Time,Seq,,,Ack\n");
		fflush(fp);
	}
#endif

	PTCP_SEGMENT_HDR hdr = TCP_GET_SEGMENT_HDR(p);

	bool isValidSeg = Validate_segment(s);

	if (isValidSeg)
	{
		if (hdr->Rst)
			process_rst_packet(s, p);
		else if (hdr->Syn)
			process_syn_packet(s, p);
		else if (hdr->Ack)
			process_ack_packet(s, p);
		else
		{
			isContinue = process_segment_text(s, p);

			if (isContinue && hdr->Fin)
				process_fin_packet(s, p);
		}
	}
	else
	{
		print_tcp_log("unacceptable segment");

		if (!hdr->Rst)
			send_ack(s,
					 pstruEventDetails->dEventTime,
					 s->tcb->SND.NXT,
					 s->tcb->RCV.NXT);

		fn_NetSim_Packet_FreePacket(p);
		pstruEventDetails->pPacket = NULL;
	}
}