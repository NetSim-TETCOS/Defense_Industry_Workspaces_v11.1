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
static void send_syn(PNETSIM_SOCKET s);

void tcp_active_open(PNETSIM_SOCKET s)
{
	print_tcp_log("Creating TCB");
	create_TCB(s);
	print_tcp_log("Sending Syn");
	send_syn(s);
}

void tcp_passive_open(PNETSIM_SOCKET s, PNETSIM_SOCKET ls)
{
	print_tcp_log("Creating TCB");
	create_TCB(s);
	memcpy(&s->tcb->SEG, &ls->tcb->SEG, sizeof s->tcb->SEG);
	tcp_change_state(s, TCPCONNECTION_LISTEN);
}

static void send_syn(PNETSIM_SOCKET s)
{
	NetSim_PACKET* syn = create_syn(s,pstruEventDetails->dEventTime);

	s->tcb->SND.UNA = s->tcb->ISS;
	s->tcb->SND.NXT = s->tcb->ISS + 1;
	tcp_change_state(s, TCPCONNECTION_SYN_SENT);

	s->tcb->synRetries++;

	s->tcpMetrics->synSent++;

	send_to_network(syn, s);
	add_timeout_event(s,syn);
}

void resend_syn(PNETSIM_SOCKET s)
{
	PTCP_DEV_VAR tcp = GET_TCP_DEV_VAR(pstruEventDetails->nDeviceId);

	if (s->tcb->tcp_state != TCPCONNECTION_SYN_SENT &&
		s->tcb->tcp_state != TCPCONNECTION_SYN_RECEIVED)
	{
		print_tcp_log("ERROR: %s is called for different TCP state %s. it must be %s or %s",
					  __FUNCTION__,
					  state_to_str(s->tcb->tcp_state),
					  state_to_str(TCPCONNECTION_SYN_SENT),
					  state_to_str(TCPCONNECTION_SYN_RECEIVED));
		assert(false);
	}

	PTCP_SEGMENT_HDR hdr = TCP_GET_SEGMENT_HDR(pstruEventDetails->pPacket);

	NetSim_PACKET* syn = get_segment_from_queue(&s->tcb->retransmissionQueue, hdr->SeqNum);

	fn_NetSim_Packet_FreePacket(pstruEventDetails->pPacket);
	pstruEventDetails->pPacket = NULL;

	if (s->tcb->synRetries < tcp->MaxSynRetries)
	{
		print_tcp_log("Resending syn/syn-ack packet.");
		s->tcb->SND.UNA = s->tcb->ISS;
		s->tcb->SND.NXT = s->tcb->ISS + 1;

		send_to_network(syn, s);
		add_timeout_event(s, syn);

		if(syn->nControlDataType == TCPPACKET_SYN)
			s->tcpMetrics->synSent++;
		else
			s->tcpMetrics->synAckSent++;

		s->tcb->synRetries++;
	}
	else
	{
		print_tcp_log("Connection fails for local addr %s:%d, Remote addr %s:%d",
					  s->localAddr->ip->str_ip,
					  s->localAddr->port,
					  s->remoteAddr->ip->str_ip,
					  s->remoteAddr->port);
		tcp_close_socket(s,pstruEventDetails->nDeviceId);
	}
}

void process_syn(PNETSIM_SOCKET s,NetSim_PACKET* p)
{
	if (!s->localAddr) 
	{
		s->listen_callback(s, p);
	}
	else
	{
		if ((s->tcb->IRS && s->tcb->IRS != s->tcb->SEG.SEQ) ||
			(s->tcb->RCV.NXT && s->tcb->RCV.NXT != s->tcb->SEG.SEQ +1))
		{
			//send_rst(s, p, 1); //RFC 793
			//Correction made by RFC 1122. Page93
			send_rst(s, p, 2);
		}
		else
		{
			s->listen_callback(s, p);
		}
	}
	fn_NetSim_Packet_FreePacket(p);
}

void rcv_SYN(PNETSIM_SOCKET s, NetSim_PACKET* syn)
{
	s->tcb->RCV.NXT = s->tcb->SEG.SEQ + 1;
	s->tcb->IRS = s->tcb->SEG.SEQ;

	//Store the Window scale option based on peer device
	PWsopt wsopt = get_tcp_option(TCP_GET_SEGMENT_HDR(syn),
								  TCP_OPTION_WINDOW_SCALE);

	set_window_scaling(s, wsopt);

	//Store MSS based on peer device
	PMSS_OPTION opt = get_tcp_option(TCP_GET_SEGMENT_HDR(syn),TCP_OPTION_MSS);
	if (opt)
		s->tcb->set_MSS(s, opt->MSSData);

	//Store the timestamp option based on peer device
	PTSopt tsopt = get_tcp_option(TCP_GET_SEGMENT_HDR(syn),
								  TCP_OPTION_TIMESTAMP);
	set_timestamp_value(s, TCP_GET_SEGMENT_HDR(syn), tsopt);

	if (s->tcb->isSackPermitted)
	{
		//Store Sack option based on peer device
		PSACKPERMITTED_OPTION sackp = get_tcp_option(TCP_GET_SEGMENT_HDR(syn), TCP_OPTION_SACK_PERMITTED);
		if (sackp)
			s->tcb->isSackPermitted = true;
		else
			s->tcb->isSackPermitted = false;
	}	

	NetSim_PACKET* synAck = create_synAck(s, pstruEventDetails->dEventTime);
	tcp_change_state(s, TCPCONNECTION_SYN_RECEIVED);
	
	s->tcb->SND.UNA = s->tcb->ISS;
	s->tcb->SND.NXT = s->tcb->ISS + 1;

	s->tcb->synRetries++;

	s->tcpMetrics->synAckSent++;

	send_to_network(synAck, s);
	add_timeout_event(s, synAck);
}

void send_rst(PNETSIM_SOCKET s, NetSim_PACKET* p, UINT c)
{
	print_tcp_log("Sending RST packet");
	NetSim_PACKET* rst = create_rst(p, pstruEventDetails->dEventTime, c);
	send_to_network(rst, s);
}

void send_ack(PNETSIM_SOCKET s, double time, UINT32 seqNo, UINT32 ackNo)
{
	print_tcp_log("Sending ack with seq=%d, Ack=%d.", seqNo, ackNo);
	NetSim_PACKET* ack = create_ack(s, time, seqNo, ackNo);
	s->tcpMetrics->ackSent++;
	send_to_network(ack, s);
}

void send_fin_ack(PNETSIM_SOCKET s, double time, UINT32 seqNo, UINT32 ackNo)
{
	print_tcp_log("Sending fin-ack with seq=%d, Ack=%d.", seqNo, ackNo);
	NetSim_PACKET* ack = create_ack(s, time, seqNo, ackNo);
	PTCP_SEGMENT_HDR hdr = TCP_GET_SEGMENT_HDR(ack);
	hdr->Fin = 1;
	s->tcpMetrics->ackSent++;
	send_to_network(ack, s);
}

void packet_arrive_at_closed_state(PNETSIM_SOCKET s, NetSim_PACKET* p)
{
	print_tcp_log("TCP_STATE=CLOSED");
	if (isTCPControl(p))
	{
		PTCP_SEGMENT_HDR hdr = TCP_GET_SEGMENT_HDR(p);
		if (hdr->Rst)
			goto DISCARD_PACKET;
		else
		{
			send_rst(s, p, 0);
			goto FREE_SEGMENT;
		}
	}
	else
		goto DISCARD_PACKET;

DISCARD_PACKET:
	//Discard packet
	print_tcp_log("Discarding segment");
FREE_SEGMENT:
	fn_NetSim_Packet_FreePacket(p);
	pstruEventDetails->pPacket = NULL;
}

void packet_arrives_at_listen_state(PNETSIM_SOCKET s, NetSim_PACKET* p)
{
	print_tcp_log("TCP_STATE=LISTEN");
	PTCP_SEGMENT_HDR hdr = TCP_GET_SEGMENT_HDR(p);
	if (hdr->Rst)
		goto DISCARD_PACKET;
	else if (hdr->Ack)
	{
		send_rst(s, p, 0);
		goto FREE_SEGMENT;
	}
	else if (hdr->Syn)
	{
		process_syn(s, p);
		return;
	}
	else
		fnNetSimError("Unknown processing mode in %s",
					  __FUNCTION__);


DISCARD_PACKET:
	print_tcp_log("Discarding Packet");
FREE_SEGMENT:
	fn_NetSim_Packet_FreePacket(p);
	pstruEventDetails->pPacket = NULL;
}

void packet_arrives_at_synsent_state(PNETSIM_SOCKET s, NetSim_PACKET* p)
{
	bool isAckAcceptable = false;
	print_tcp_log("TCP_STATE = SYN-SENT");

	PTCP_SEGMENT_HDR hdr = TCP_GET_SEGMENT_HDR(p);
	if (hdr->Ack)
	{
		if (s->tcb->SEG.ACK <= s->tcb->ISS ||
			s->tcb->SEG.ACK > s->tcb->SND.NXT)
		{
			send_rst(s, p, 0);
			goto FREE_SEGMENT;
		}

		if (s->tcb->SND.UNA <= s->tcb->SEG.ACK &&
			s->tcb->SEG.ACK <= s->tcb->SND.NXT)
			isAckAcceptable = true;
	}
	if (hdr->Rst)
	{
		if (isAckAcceptable)
		{
			print_tcp_log("Error: Connection reset");
			delete_tcb(s);
			tcp_change_state(s, TCPCONNECTION_CLOSED);
			tcp_close_socket(s, s->localDeviceId);
			return;
		}
		else
		{
			goto DISCARD_PACKET;
		}
	}
	if (isAckAcceptable)
	{
		if (hdr->Syn)
		{
			s->tcb->RCV.NXT = s->tcb->SEG.SEQ + 1;
			s->tcb->IRS = s->tcb->SEG.SEQ;
			s->tcb->SND.UNA = s->tcb->SEG.ACK;

			//Store MSS based on peer device
			PMSS_OPTION opt = get_tcp_option(hdr, TCP_OPTION_MSS);
			if (opt)
				s->tcb->set_MSS(s, opt->MSSData);

			if (s->tcb->isTSopt)
			{
				//Store the timestamp option based on peer device
				PTSopt tsopt = get_tcp_option(hdr,
											  TCP_OPTION_TIMESTAMP);
				set_timestamp_value(s, hdr, tsopt);
			}

			delete_segment_from_queue(&s->tcb->retransmissionQueue, s->tcb->SEG.ACK);

			if (s->tcb->SND.UNA > s->tcb->ISS)
			{
				//Correction made by RFC 1122. Page 93.
				s->tcb->SND.WND = s->tcb->SEG.WND;
				s->tcb->SND.WL1 = s->tcb->SEG.SEQ;
				s->tcb->SND.WL2 = s->tcb->SEG.ACK;

				send_ack(s,
						 pstruEventDetails->dEventTime,
						 s->tcb->SND.NXT,
						 s->tcb->RCV.NXT);
				tcp_change_state(s, TCPCONNECTION_ESTABLISHED);
				send_segment(s);
				return;
			}
		}
	}

DISCARD_PACKET:
	print_tcp_log("Discarding Packet");
FREE_SEGMENT:
	fn_NetSim_Packet_FreePacket(p);
	pstruEventDetails->pPacket = NULL;
}

void send_fin(PNETSIM_SOCKET s)
{
	NetSim_PACKET* fin = create_fin(s, pstruEventDetails->dEventTime);

	send_to_network(fin, s);

#ifdef _DEBUG
	fnNetSimError("RTO timeout is not implemented for FIN");
	//add_timeout_event(s, fin);
#endif 
}