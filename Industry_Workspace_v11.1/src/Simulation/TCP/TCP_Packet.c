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

bool isSynPacket(NetSim_PACKET* packet)
{
	PTCP_SEGMENT_HDR hdr = TCP_GET_SEGMENT_HDR(packet);
	if (hdr->Syn &&
		!hdr->Ack)
		return true;
	else
		return false;
}

bool isSynbitSet(NetSim_PACKET* packet)
{
	PTCP_SEGMENT_HDR hdr = TCP_GET_SEGMENT_HDR(packet);
	return hdr->Syn ? true : false;
}


void set_tcp_option(PTCP_SEGMENT_HDR hdr, void* option, TCP_OPTION type, UINT32 size)
{
	PTCPOPTION opt = (PTCPOPTION)calloc(1, sizeof* opt);
	opt->type = type;
	opt->option = option;
	opt->size = size;
	
	hdr->HdrLength += size;

	if (hdr->option)
	{
		PTCPOPTION o = hdr->option;
		while (o->next)
			o = o->next;
		o->next = opt;
	}
	else
		hdr->option = opt;
}

void* get_tcp_option(PTCP_SEGMENT_HDR hdr, TCP_OPTION type)
{
	PTCPOPTION o = hdr->option;
	while (o)
	{
		if (o->type == type)
			return o->option;
		o = o->next;
	}
	return NULL;
}

static void add_tcp_extra_option(PTCP_SEGMENT_HDR hdr, UINT32 l)
{
	UINT32 i;
	for (i = 0; i < 4-l; i++)
	{
		if (i == 4 - l - 1)
		{
			PEXTRA_OPTION end = calloc(1, sizeof* end);
			end->type = TCP_OPTION_END;
			set_tcp_option(hdr, end, TCP_OPTION_END, EXTRA_OPTION_LEN);
		}
		else
		{
			PEXTRA_OPTION nop = calloc(1, sizeof* nop);
			nop->type = TCP_OPTION_NOOPERATION;
			set_tcp_option(hdr, nop, TCP_OPTION_NOOPERATION, EXTRA_OPTION_LEN);
		}
	}
}

static void set_tcp_packet_size(NetSim_PACKET* p)
{
	PTCP_SEGMENT_HDR hdr = TCP_GET_SEGMENT_HDR(p);
	if (hdr->HdrLength % 4)
	{
		add_tcp_extra_option(hdr, hdr->HdrLength % 4);
	}
	p->pstruTransportData->dOverhead = hdr->HdrLength;
	p->pstruTransportData->dPacketSize = p->pstruTransportData->dPayload +
		p->pstruTransportData->dOverhead;
}

static void add_tcp_mss_option(PNETSIM_SOCKET s,
							   PTCP_SEGMENT_HDR hdr)
{
	PMSS_OPTION opt = (PMSS_OPTION)calloc(1, sizeof* opt);
	opt->type = TCP_OPTION_MSS;
	opt->len = MSS_OPTION_LEN;
	opt->MSSData = s->tcb->get_MSS(s);
	set_tcp_option(hdr, opt, TCP_OPTION_MSS, MSS_OPTION_LEN);
}

static void add_tcp_sack_permitted_option(PNETSIM_SOCKET s,
										  PTCP_SEGMENT_HDR hdr)
{
	PSACKPERMITTED_OPTION opt = (PSACKPERMITTED_OPTION)calloc(1, sizeof* opt);
	opt->type = TCP_OPTION_SACK_PERMITTED;
	opt->len = SACKPERMITTED_OPTION_LEN;
	set_tcp_option(hdr, opt, TCP_OPTION_SACK_PERMITTED, SACKPERMITTED_OPTION_LEN);
}

static void add_tcp_window_scale_option(PNETSIM_SOCKET s,
										  PTCP_SEGMENT_HDR hdr)
{
	PWsopt opt = (PWsopt)calloc(1, sizeof* opt);
	opt->type = TCP_OPTION_WINDOW_SCALE;
	opt->len = WSOPT_LEN;
	opt->Shift_cnt = s->tcb->Snd.Wind_Shift;
	set_tcp_option(hdr, opt, TCP_OPTION_WINDOW_SCALE, WSOPT_LEN);

	PEXTRA_OPTION ex = (PEXTRA_OPTION)calloc(1, sizeof* ex);
	ex->type = TCP_OPTION_NOOPERATION;
	set_tcp_option(hdr, ex, TCP_OPTION_NOOPERATION, EXTRA_OPTION_LEN);
}

static void add_tcp_timestamp_option(PNETSIM_SOCKET s,
									 PTCP_SEGMENT_HDR hdr)
{
	PTSopt opt = (PTSopt)calloc(1, sizeof* opt);
	opt->type = TCP_OPTION_TIMESTAMP;
	opt->len = TSOPT_LEN;
	
	opt->TSval = (UINT)pstruEventDetails->dEventTime;
	
	if (!hdr->Ack)
		opt->TSecr = 0;
	else
		opt->TSecr = s->tcb->TSVal;

	set_tcp_option(hdr, opt, TCP_OPTION_TIMESTAMP, TSOPT_LEN);

	PEXTRA_OPTION ex = (PEXTRA_OPTION)calloc(1, sizeof* ex);
	ex->type = TCP_OPTION_NOOPERATION;
	set_tcp_option(hdr, ex, TCP_OPTION_NOOPERATION, EXTRA_OPTION_LEN);
}

NetSim_PACKET* create_tcp_ctrl_packet(PNETSIM_SOCKET s,
									  TCPPACKET type,
									  double time)
{
	PTCP_SEGMENT_HDR hdr = (PTCP_SEGMENT_HDR)calloc(1, sizeof* hdr);
	hdr->HdrLength = TCP_HDR_SIZE;
	hdr->SrcPort = s->localAddr->port;
	hdr->DstPort = s->remoteAddr->port;

	NetSim_PACKET* p = fn_NetSim_Packet_CreatePacket(TRANSPORT_LAYER);
	p->nControlDataType = type;
	add_dest_to_packet(p, s->remoteDeviceId);
	p->nPacketType = PacketType_Control;
	p->nSourceId = s->localDeviceId;
	p->pstruTransportData->dArrivalTime = time;
	p->pstruTransportData->dOverhead = TCP_HDR_SIZE;
	p->pstruTransportData->dPayload = 0;
	p->pstruTransportData->dPacketSize = TCP_HDR_SIZE;
	p->pstruTransportData->nDestinationPort = s->remoteAddr->port;
	p->pstruTransportData->nSourcePort = s->localAddr->port;
	p->pstruTransportData->nTransportProtocol = TX_PROTOCOL_TCP;
	p->pstruTransportData->Packet_TransportProtocol = hdr;

	p->pstruNetworkData->szSourceIP = s->localAddr->ip;
	p->pstruNetworkData->szDestIP = s->remoteAddr->ip;
	p->pstruNetworkData->nTTL = MAX_TTL;
	p->pstruNetworkData->IPProtocol = IPPROTOCOL_TCP;

	return p;
}

NetSim_PACKET* create_syn(PNETSIM_SOCKET s,
						  double time)
{
	NetSim_PACKET* p = create_tcp_ctrl_packet(s,
											  TCPPACKET_SYN,
											  time);

	PTCP_SEGMENT_HDR hdr = TCP_GET_SEGMENT_HDR(p);

	hdr->Syn = 1;
	hdr->SeqNum = s->tcb->ISS;
	hdr->Window = s->tcb->get_WND(s);

	add_tcp_mss_option(s, hdr);

	if (s->tcb->isSackPermitted)
		add_tcp_sack_permitted_option(s, hdr);

	if (s->tcb->isTSopt)
		add_tcp_timestamp_option(s, hdr);

	if (s->tcb->isWindowScaling)
		add_tcp_window_scale_option(s, hdr);

	set_tcp_packet_size(p);

	return p;
}

NetSim_PACKET* create_synAck(PNETSIM_SOCKET s,
							 double time)
{
	NetSim_PACKET* p = create_tcp_ctrl_packet(s,
											  TCPPACKET_SYNACK,
											  time);

	PTCP_SEGMENT_HDR hdr = TCP_GET_SEGMENT_HDR(p);

	hdr->Syn = 1;
	hdr->Ack = 1;
	hdr->SeqNum = s->tcb->ISS;
	hdr->AckNum = s->tcb->RCV.NXT;
	hdr->Window = s->tcb->get_WND(s);

	add_tcp_mss_option(s, hdr);

	if (s->tcb->isWindowScaling)
		add_tcp_window_scale_option(s, hdr);

	if (s->tcb->isSackPermitted)
		add_tcp_sack_permitted_option(s, hdr);

	if (s->tcb->isTSopt)
		add_tcp_timestamp_option(s, hdr);

	set_tcp_packet_size(p);

	return p;
}

NetSim_PACKET* create_rst(NetSim_PACKET* p,
						  double time,
						  UINT c)
{
	NETSIM_SOCKET temp;
	SOCKETADDRESS l,r;

	l.ip = p->pstruNetworkData->szDestIP;
	l.port = p->pstruTransportData->nDestinationPort;
	temp.localAddr = &l;
	
	r.ip = p->pstruNetworkData->szSourceIP;
	r.port = p->pstruTransportData->nSourcePort;
	temp.remoteAddr = &r;

	temp.localDeviceId = get_first_dest_from_packet(p);
	temp.remoteDeviceId = p->nSourceId;

	NetSim_PACKET* packet = create_tcp_ctrl_packet(&temp,
											  TCPPACKET_RST,
											  time);

	PTCP_SEGMENT_HDR hdr = TCP_GET_SEGMENT_HDR(packet);
	PTCP_SEGMENT_HDR seg = TCP_GET_SEGMENT_HDR(p);

	if (seg->Ack || c == 1)
	{
		hdr->SeqNum = seg->AckNum;
		hdr->Rst = 1;
	}
	else
	{
		hdr->SeqNum = 0;
		hdr->AckNum = seg->SeqNum + seg->HdrLength;
		hdr->Rst = 1;
		hdr->Ack = 1;
	}

	set_tcp_packet_size(p);

	return packet;
}

NetSim_PACKET* create_ack(PNETSIM_SOCKET s,
						  double time,
						  UINT32 seqno,
						  UINT32 ackno)
{
	NetSim_PACKET* p = create_tcp_ctrl_packet(s,
											  TCPPACKET_ACK,
											  time);

	PTCP_SEGMENT_HDR hdr = TCP_GET_SEGMENT_HDR(p);

	hdr->Ack = 1;
	hdr->SeqNum = seqno;
	hdr->AckNum = ackno;
	hdr->Window = s->tcb->get_WND(s);

	if (s->tcb->isTSopt)
		add_tcp_timestamp_option(s, hdr);

	set_sack_option(s, hdr, time);
	set_tcp_packet_size(p);

	return p;
}

NetSim_PACKET* create_fin(PNETSIM_SOCKET s,
						  double time)
{
	NetSim_PACKET* p = create_tcp_ctrl_packet(s,
											  TCPPACKET_FIN,
											  time);

	PTCP_SEGMENT_HDR hdr = TCP_GET_SEGMENT_HDR(p);

	hdr->Fin = 1;
	hdr->SeqNum = s->tcb->ISS + 1 == s->tcb->SND.NXT ?
		s->tcb->SND.NXT + 1 : s->tcb->SND.NXT;
	hdr->Window = window_scale_get_wnd(s);

	set_tcp_packet_size(p);

	return p;
}

void add_tcp_hdr(NetSim_PACKET* p, PNETSIM_SOCKET s)
{
	PTCP_SEGMENT_HDR hdr = (PTCP_SEGMENT_HDR)calloc(1, sizeof* hdr);
	hdr->DstPort = s->remoteAddr->port;
	hdr->HdrLength = TCP_HDR_SIZE;
	hdr->SeqNum = s->tcb->SND.NXT;
	hdr->SrcPort = s->localAddr->port;
	hdr->Window = window_scale_get_wnd(s);

	if(s->tcb->isTSopt)
		add_tcp_timestamp_option(s, hdr);

	p->pstruTransportData->dEndTime = pstruEventDetails->dEventTime;
	p->pstruTransportData->dOverhead = TCP_HDR_SIZE;
	p->pstruTransportData->dPayload = p->pstruAppData->dPacketSize;
	p->pstruTransportData->dStartTime = pstruEventDetails->dEventTime;
	p->pstruTransportData->Packet_TransportProtocol = hdr;
	p->pstruTransportData->nTransportProtocol = TX_PROTOCOL_TCP;

	p->pstruNetworkData->IPProtocol = IPPROTOCOL_TCP;

	set_tcp_packet_size(p);
}

static PTCPOPTION copy_tcp_mss_option(PTCPOPTION opt)
{
	PTCPOPTION o = (PTCPOPTION)calloc(1, sizeof* o);
	memcpy(o, opt, sizeof* o);
	o->next = NULL;
	PMSS_OPTION mss = opt->option;
	PMSS_OPTION m = (PMSS_OPTION)calloc(1, sizeof* m);
	memcpy(m, mss, sizeof* m);
	o->option = m;
	return o;
}

static PTCPOPTION copy_tcp_sack_permitted_option(PTCPOPTION opt)
{
	PTCPOPTION o = (PTCPOPTION)calloc(1, sizeof* o);
	memcpy(o, opt, sizeof* o);
	o->next = NULL;
	PSACKPERMITTED_OPTION sackPermitted = opt->option;
	PSACKPERMITTED_OPTION sp = (PSACKPERMITTED_OPTION)calloc(1, sizeof* sp);
	memcpy(sp, sackPermitted, sizeof* sp);
	o->option = sp;
	return o;
}

static PTCPOPTION copy_tcp_extra_option(PTCPOPTION opt)
{
	PTCPOPTION o = (PTCPOPTION)calloc(1, sizeof* o);
	memcpy(o, opt, sizeof* o);
	o->next = NULL;
	PEXTRA_OPTION ex = opt->option;
	PEXTRA_OPTION dex = (PEXTRA_OPTION)calloc(1, sizeof* dex);
	memcpy(dex, ex, sizeof* dex);
	o->option = dex;
	return o;
}

static PTCPOPTION copy_sack_option(PTCPOPTION t)
{
	PSACK_OPTION sack = t->option;
	PSACK_OPTION s = (PSACK_OPTION)calloc(1, sizeof* s);
	memcpy(s, sack, sizeof* s);
	s->sackData = NULL;
	UINT c = get_sack_data_count(sack);
	s->sackData = (PSACKDATA*)calloc(c, sizeof* s->sackData);
	for (UINT i = 0; i < c; i++)
	{
		s->sackData[i] = (PSACKDATA)calloc(1, sizeof* s->sackData[i]);
		memcpy(s->sackData[i], sack->sackData[i], sizeof* s->sackData[i]);
	}
	PTCPOPTION r = (PTCPOPTION)calloc(1, sizeof* r);
	r->option = s;
	r->size = t->size;
	r->type = t->type;
	return r;
}

static PTCPOPTION copy_window_scale_option(PTCPOPTION opt)
{
	PTCPOPTION o = (PTCPOPTION)calloc(1, sizeof* o);
	memcpy(o, opt, sizeof* o);
	o->next = NULL;
	PWsopt ex = opt->option;
	PWsopt dex = (PWsopt)calloc(1, sizeof* dex);
	memcpy(dex, ex, sizeof* dex);
	o->option = dex;
	return o;
}

static PTCPOPTION copy_timestamp_option(PTCPOPTION opt)
{
	PTCPOPTION o = (PTCPOPTION)calloc(1, sizeof* o);
	memcpy(o, opt, sizeof* o);
	o->next = NULL;
	PTSopt ex = opt->option;
	PTSopt dex = (PTSopt)calloc(1, sizeof* dex);
	memcpy(dex, ex, sizeof* dex);
	o->option = dex;
	return o;
}

static void copy_tcp_option(PTCP_SEGMENT_HDR dhdr, PTCP_SEGMENT_HDR hdr)
{
	PTCPOPTION opt = hdr->option;
	dhdr->option = NULL;
	while (opt)
	{
		PTCPOPTION dopt = NULL;
		switch (opt->type)
		{
		case TCP_OPTION_END:
		case TCP_OPTION_NOOPERATION:
			dopt = copy_tcp_extra_option(opt);
			break;
		case TCP_OPTION_MSS:
			dopt = copy_tcp_mss_option(opt);
			break;
		case TCP_OPTION_SACK_PERMITTED:
			dopt = copy_tcp_sack_permitted_option(opt);
			break;
		case TCP_OPTION_SACK:
			dopt = copy_sack_option(opt);
			break;
		case TCP_OPTION_WINDOW_SCALE:
			dopt = copy_window_scale_option(opt);
			break;
		case TCP_OPTION_TIMESTAMP:
			dopt = copy_timestamp_option(opt);
			break;
		default:
			fnNetSimError("Unknown TCP option %d\n", opt->type);
			break;
		}
		if (dhdr->option)
		{
			PTCPOPTION o = dhdr->option;
			while (o->next)
				o = o->next;
			o->next = dopt;
		}
		else
		{
			dhdr->option = dopt;
		}
		opt = opt->next;
	}
}

PTCP_SEGMENT_HDR copy_tcp_hdr(PTCP_SEGMENT_HDR hdr)
{
	PTCP_SEGMENT_HDR dhdr = (PTCP_SEGMENT_HDR)calloc(1, sizeof* dhdr);
	memcpy(dhdr, hdr, sizeof* dhdr);
	copy_tcp_option(dhdr, hdr);
	return dhdr;
}

static void free_tcp_mss_option(PMSS_OPTION mss)
{
	free(mss);
}

static void free_tcp_sack_permitted_option(PSACKPERMITTED_OPTION sackPermitted)
{
	free(sackPermitted);
}

static void free_tcp_extra_option(PEXTRA_OPTION ex)
{
	free(ex);
}

static void free_tcp_sack_option(PSACK_OPTION sack)
{
	UINT c = get_sack_data_count(sack);
	for (UINT i = 0; i < c; i++)
	{
		free(sack->sackData[i]);
	}
	free(sack->sackData);
	free(sack);
}

static void free_tcp_window_scale_option(PWsopt opt)
{
	free(opt);
}

static void free_tcp_timestamp_option(PTSopt opt)
{
	free(opt);
}

static void free_tcp_option(PTCPOPTION opt)
{
	while (opt)
	{
		PTCPOPTION o = opt->next;
		switch (opt->type)
		{
		case TCP_OPTION_END:
		case TCP_OPTION_NOOPERATION:
			free_tcp_extra_option(opt->option);
			break;
		case TCP_OPTION_MSS:
			free_tcp_mss_option(opt->option);
			break;
		case TCP_OPTION_SACK_PERMITTED:
			free_tcp_sack_permitted_option(opt->option);
			break;
		case TCP_OPTION_SACK:
			free_tcp_sack_option(opt->option);
			break;
		case TCP_OPTION_WINDOW_SCALE:
			free_tcp_window_scale_option(opt->option);
			break;
		case TCP_OPTION_TIMESTAMP:
			free_tcp_timestamp_option(opt->option);
			break;
		default:
			fnNetSimError("Unknown TCP option %d\n", opt->type);
			break;
		}
		free(opt);
		opt = o;
	}
}

void free_tcp_hdr(PTCP_SEGMENT_HDR hdr)
{
	free_tcp_option(hdr->option);
	free(hdr);
}
