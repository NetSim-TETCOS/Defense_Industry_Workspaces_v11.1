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

void send_to_network(NetSim_PACKET* packet, PNETSIM_SOCKET s)
{
	PTCP_SEGMENT_HDR hdr = TCP_GET_SEGMENT_HDR(packet);
	print_tcp_log("Sending segment from outgoing tcp, local addr:%s:%d, Remote addr %s:%d, SEQ=%d, ACK=%d,SYN=%d,ACK=%d,RST=%d,URG=%d,FIN=%d,LEN=%d",
				  s->localAddr == NULL ? "0.0.0.0" : s->localAddr->ip->str_ip,
				  s->localAddr == NULL ? 0 : s->localAddr->port,
				  s->remoteAddr == NULL ? "0.0.0.0" : s->remoteAddr->ip->str_ip,
				  s->remoteAddr == NULL ? 0 : s->remoteAddr->port,
				  hdr->SeqNum,
				  hdr->AckNum,
				  hdr->Syn,
				  hdr->Ack,
				  hdr->Rst,
				  hdr->Urg,
				  hdr->Fin,
				  get_seg_len(packet));

	packet->pstruTransportData->dStartTime = pstruEventDetails->dEventTime;
	packet->pstruTransportData->dEndTime = pstruEventDetails->dEventTime;

	NetSim_EVENTDETAILS pevent;
	memcpy(&pevent, pstruEventDetails, sizeof pevent);
	pevent.dPacketSize = packet->pstruTransportData->dPacketSize;
	if (packet->pstruAppData)
	{
		pevent.nApplicationId = packet->pstruAppData->nApplicationId;
		pevent.nSegmentId = packet->pstruAppData->nSegmentId;
	}
	else
	{
		pevent.nSegmentId = 0;
	}

	pevent.nEventType = NETWORK_OUT_EVENT;
	pevent.nPacketId = packet->nPacketId;
	pevent.nProtocolId = fn_NetSim_Stack_GetNWProtocol(pevent.nDeviceId);
	pevent.nSubEventType = 0;
	pevent.pPacket = packet;
	pevent.szOtherDetails = NULL;
	fnpAddEvent(&pevent);
}

void packet_arrive_from_network_layer()
{
#ifdef _TEST_TCP_
	if (!pass_test())
	{
		//Packet is dropped by test suite
		fn_NetSim_Packet_FreePacket(pstruEventDetails->pPacket);
		return;
	}
#endif
	NetSim_PACKET* packet = pstruEventDetails->pPacket;
	PNETSIM_SOCKET s = find_socket_at_dest(packet);
	PTCP_SEGMENT_HDR hdr = TCP_GET_SEGMENT_HDR(packet);
	if (s && s->tcb)
	{
		print_tcp_log("\nDevice %d, Time %0.2lf: segment arrives at incoming tcp, local addr:%s:%d, Remote addr %s:%d, SEQ=%d, ACK=%d,SYN=%d,ACK=%d,RST=%d,URG=%d,FIN=%d,LEN=%d",
					  pstruEventDetails->nDeviceId,
					  pstruEventDetails->dEventTime,
					  s->localAddr == NULL ? "0.0.0.0" : s->localAddr->ip->str_ip,
					  s->localAddr == NULL ? 0 : s->localAddr->port,
					  s->remoteAddr == NULL ? "0.0.0.0" : s->remoteAddr->ip->str_ip,
					  s->remoteAddr == NULL ? 0 : s->remoteAddr->port,
					  hdr->SeqNum,
					  hdr->AckNum,
					  hdr->Syn,
					  hdr->Ack,
					  hdr->Rst,
					  hdr->Urg,
					  hdr->Fin,
					  get_seg_len(packet));

		update_seq_num_on_receiving(s, packet);

		if (s->tcb->tcp_state == TCPCONNECTION_CLOSED)
			packet_arrive_at_closed_state(s, packet);

		else if (s->tcb->tcp_state == TCPCONNECTION_LISTEN)
			packet_arrives_at_listen_state(s, packet);
		
		else if (s->tcb->tcp_state == TCPCONNECTION_SYN_SENT)
			packet_arrives_at_synsent_state(s, packet);

		else
			packet_arrives_at_incoming_tcp(s, packet);
	}
	else
	{
		print_tcp_log("Packet arrive to TCP for device %d for which there is no connection. Discarding..",
					  pstruEventDetails->nDeviceId);
		fn_NetSim_Packet_FreePacket(pstruEventDetails->pPacket);
		pstruEventDetails->pPacket = NULL;
	}
}