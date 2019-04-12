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

#ifndef _NETSIM_TCB_H_
#define _NETSIM_TCB_H_
#ifdef  __cplusplus
extern "C" {
#endif

	typedef struct stru_tcp_queue_info
	{
		NetSim_PACKET* packet;
		double time;
		bool isSacked;
		struct stru_tcp_queue_info* next;
	}Queueinfo, *PQueueInfo;
	typedef struct stru_tcp_queue
	{
		UINT size;
		PQueueInfo queue;
	}TCP_QUEUE, *PTCP_QUEUE;

	typedef struct stru_tcp_Transmission_Control_Block
	{
		TCP_CONNECTION_STATE tcp_state; ///< Present State of the TCP Connection.
		TCP_CONNECTION_STATE tcp_prev_state; ///< Present State of the Connection.

		TCPVARIANT variant;


		/*Send Sequence Variables as per RFC 793 page number 18

		SND.UNA - send unacknowledged
		SND.NXT - send next
		SND.WND - send window
		SND.UP  - send urgent pointer
		SND.WL1 - segment sequence number used for last window update
		SND.WL2 - segment acknowledgment number used for last window
		update
		ISS     - initial send sequence number
		*/
		struct stru_tcb_send_seq_var
		{
			UINT32 UNA;
			UINT32 NXT;
			UINT32 WND;
			UINT32 UP;
			UINT32 WL1;
			UINT32 WL2;
		}SND;
		UINT32 ISS;

		/* Receive Sequence Variables

		RCV.NXT - receive next
		RCV.WND - receive window
		RCV.UP  - receive urgent pointer
		IRS     - initial receive sequence number
		*/
		struct stru_tcb_recv_seq_var
		{
			UINT32 NXT;
			UINT32 WND;
			UINT32 UP;
		}RCV;
		UINT32 IRS;

		/*Current Segment Variables

		SEG.SEQ - segment sequence number
		SEG.ACK - segment acknowledgment number
		SEG.LEN - segment length
		SEG.WND - segment window
		SEG.UP - segment urgent pointer
		SEG.PRC - segment precedence value
		*/
		struct stru_tcb_curr_seg_var
		{
			UINT SEQ;
			UINT ACK;
			UINT LEN;
			UINT WND;
			UINT UP;
			UINT PRC;
		}SEG;

		TCP_QUEUE retransmissionQueue;
		TCP_QUEUE outOfOrderSegment;

		struct stru_tcp_timer
		{
			double RTO;
			double SRTT;
			double RTT_VAR;
		}TCP_TIMER;
#define TCP_RTO(tcb)	(tcb->TCP_TIMER.RTO)
#define TCP_SRTT(tcb)	(tcb->TCP_TIMER.SRTT)
#define TCP_RTTVAR(tcb)	(tcb->TCP_TIMER.RTT_VAR)

		UINT synRetries;

		//Congestion algo data
		void* congestionData;

		//Congestion Algo callback
		void(*init_congestionalgo)(PNETSIM_SOCKET);
		void(*ack_received)(PNETSIM_SOCKET);
		void(*rto_expired)(PNETSIM_SOCKET);
		UINT16(*get_MSS)(PNETSIM_SOCKET);
		UINT16(*get_WND)(PNETSIM_SOCKET);
		void(*set_MSS)(PNETSIM_SOCKET s, UINT16);

		//Delayed Ack
		TCPACKTYPE ackType;
		double delayedAckTime;
		double lastAckSendTime;
		bool isAckSentLastTime;

		//Time wait timer
		double timeWaitTimer;
		bool istimewaittimerrestarted;
		bool isOtherTimerCancel;

		//SACK
		bool isSackPermitted;
		struct stru_sack_scoreboard
		{
			UINT leftEdge[4];
			UINT rightEdge[4];
		}scoreboard,prevScoreboard;
		UINT pipe;
		UINT32 HighRxt;
		UINT32 RescueRxt;
		bool isSackOption; //Check whether current ack has sack option
		UINT32 recoveryPoint;

		//Send Variable
		struct stru_tcp_snd
		{
			UINT8 Wind_Shift;	//RFC 7323
			bool TS_OK;			//RFC 7323
		}Snd;

		//Receive Variable
		struct stru_tcp_rcv
		{
			UINT8 Wind_Shift; //RFC 7323
		}Rcv;

		//Window Scaling
		bool isWindowScaling;

		//Timestamp option
		bool isTSopt;
		UINT32 TSVal; //Received TSval

	}TCB, *PTCB;

#ifdef  __cplusplus
}
#endif
#endif //_NETSIM_TCB_H_
