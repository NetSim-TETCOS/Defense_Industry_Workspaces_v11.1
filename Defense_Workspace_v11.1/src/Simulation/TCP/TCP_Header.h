#pragma once
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

#ifndef _NETSIM_TCP_HEADER_H_
#define _NETSIM_TCP_HEADER_H_
#ifdef  __cplusplus
extern "C" {
#endif

	//If TCP.h file is not included in .c file.
#ifndef _NETSIM_TCP_H_
#define PNETSIM_SOCKET void*
#endif

	typedef enum enum_tcp_packet
	{
		TCPPACKET_SYN = TX_PROTOCOL_TCP * 100 + 1,
		TCPPACKET_SYNACK,
		TCPPACKET_FIN,
		TCPPACKET_RST,
		TCPPACKET_ACK,
	}TCPPACKET;

	typedef enum enum_tcp_option
	{
		TCP_OPTION_END,
		TCP_OPTION_NOOPERATION,
		TCP_OPTION_MSS,
		TCP_OPTION_WINDOW_SCALE,
		TCP_OPTION_SACK_PERMITTED,
		TCP_OPTION_SACK,
		TCP_OPTION_ECHO,		//obsoleted by option 8
		TCP_OPTION_ECHOREPLY,	//obsoleted by option 8
		TCP_OPTION_TIMESTAMP,
	}TCP_OPTION;

	typedef struct stru_tcp_option
	{
		TCP_OPTION type;
		void* option;
		UINT32 size;
		struct stru_tcp_option* next;
	}TCPOPTION,*PTCPOPTION;

/*
	TCP Header Format


	0                   1                   2                   3
	0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |          Source Port          |       Destination Port        |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                        Sequence Number                        |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                    Acknowledgment Number                      |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |  Data |           |U|A|P|R|S|F|                               |
   | Offset| Reserved  |R|C|S|S|Y|I|            Window             |
   |       |           |G|K|H|T|N|N|                               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |           Checksum            |         Urgent Pointer        |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                    Options                    |    Padding    |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                             data                              |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

							TCP Header Format
 */
	//Looks little different than RFC as below is used in Linux or windows.
	typedef struct stru_tcp_header
	{
		UINT16 SrcPort;
		UINT16 DstPort;
		UINT32 SeqNum;
		UINT32 AckNum;
		UINT32 HdrLength : 8;
		UINT32 Fin : 1;
		UINT32 Syn : 1;
		UINT32 Rst : 1;
		UINT32 Psh : 1;
		UINT32 Ack : 1;
		UINT32 Urg : 1;
		UINT32 Reserved : 2;
		UINT16 Window;
		UINT16 Checksum;
		UINT16 UrgPtr;
		PTCPOPTION option;
	}TCP_SEGMENT_HDR,*PTCP_SEGMENT_HDR;
#define TCP_HDR_SIZE		20 //Bytes
#define MAX_TCP_OPT_LEN		40 //Bytes
#define MAX_TCP_HDR_SIZE	(TCP_HDR_SIZE+MAX_TCP_OPT_LEN)

	static PTCP_SEGMENT_HDR TCP_GET_SEGMENT_HDR(NetSim_PACKET* packet)
	{
		if (packet && 
			packet->pstruTransportData &&
			packet->pstruTransportData->nTransportProtocol == TX_PROTOCOL_TCP)
			return packet->pstruTransportData->Packet_TransportProtocol;
		return NULL;
	}

	typedef struct stru_mss_option
	{
		UINT8 type;
		UINT8 len;
		UINT16 MSSData;
	}MSS_OPTION, *PMSS_OPTION;
#define MSS_OPTION_LEN	4 //Bytes

	typedef struct stru_sack_permitted_option
	{
		UINT8 type;
		UINT8 len;
	}SACKPERMITTED_OPTION, *PSACKPERMITTED_OPTION;
#define SACKPERMITTED_OPTION_LEN 2 //Bytes

	typedef struct stru_sack_data
	{
		UINT32 leftEdge;
		UINT32 rightEdge;
	}SACKDATA, *PSACKDATA;
#define SACKDATA_LEN	8 //Bytes

	typedef struct stru_sack_option
	{
		UINT8 type;
		UINT8 len;
		PSACKDATA* sackData;
	}SACK_OPTION, *PSACK_OPTION;
#define SACK_OPTION_FIX_LEN		2 //Bytes
#define SACK_OPTION_LEN(n) ((UINT8)(SACK_OPTION_FIX_LEN+(n)*SACKDATA_LEN)) //Bytes 
#define get_sack_data_count(sack) ((sack->len-SACK_OPTION_FIX_LEN)/SACKDATA_LEN)

	typedef struct stru_tcp_extra_option
	{
		UINT8 type;
	}EXTRA_OPTION, *PEXTRA_OPTION;
#define EXTRA_OPTION_LEN	1 //Bytes

	typedef struct stru_tcp_window_scale_option
	{
		UINT8 type;
		UINT8 len;
		UINT8 Shift_cnt;
	}Wsopt, *PWsopt;
#define WSOPT_LEN		3 //Bytes

	typedef struct stru_tcp_timestamp_option
	{
		UINT8 type;
		UINT8 len;
		UINT32 TSval;
		UINT32 TSecr;
	}TSopt, *PTSopt;
#define TSOPT_LEN		10 //Bytes

	//Function prototype
	PTCP_SEGMENT_HDR copy_tcp_hdr(PTCP_SEGMENT_HDR hdr);
	void free_tcp_hdr(PTCP_SEGMENT_HDR hdr);
	void* get_tcp_option(PTCP_SEGMENT_HDR hdr, TCP_OPTION type);
	void set_tcp_option(PTCP_SEGMENT_HDR hdr, void* option, TCP_OPTION type, UINT32 size);

	//SACK
	void set_sack_option(PNETSIM_SOCKET s, PTCP_SEGMENT_HDR hdr, double time);
	void receive_sack_option(PNETSIM_SOCKET s, PTCP_SEGMENT_HDR hdr);

	//Window Scaling
	void set_window_scaling(PNETSIM_SOCKET s, PWsopt opt);

	//TimeStamp option
	void set_timestamp_value(PNETSIM_SOCKET s,
							 PTCP_SEGMENT_HDR hdr,
							 PTSopt opt);

#ifdef  __cplusplus
}
#endif
#endif //_NETSIM_TCP_HEADER_H_