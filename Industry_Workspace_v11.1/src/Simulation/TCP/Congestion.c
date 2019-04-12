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

//#define _TEST_CONGESTION_

#ifdef _TEST_CONGESTION_
static FILE* fp;
#endif
void cubic_ack_received(PNETSIM_SOCKET s);
void init_cubic(PNETSIM_SOCKET s);

#define BICTCP_BETA_SCALE 1024	/* Scale factor beta calculation
								* max_cwnd = snd_cwnd * beta
								*/

#define BICTCP_B 4	/*
					* In binary search,
					* go to point (max+min)/N
					*/

typedef struct stru_bic
{
	UINT32 low_window;
	UINT32 max_increment;
	bool fast_convergence;
	double beta;
	UINT32 smooth_part;

	UINT32 cnt;            /* increase cwnd by 1 after ACKs */
	UINT32 last_max_cwnd;  /* last maximum snd_cwnd */
	UINT32 loss_cwnd;      /* congestion window at last loss */
	UINT32 last_cwnd;      /* the last snd_cwnd */
	UINT32 last_time;      /* time when updated last_cwnd */
	UINT32 epoch_start;    /* beginning of an epoch */
#define ACK_RATIO_SHIFT 4
	UINT32 delayed_ack;    /* estimate the ratio of Packets/ACKs << 4 */

	UINT32 cwndcnt;
}BIC,*PBIC;

typedef struct stru_congestion_var
{
	UINT16 SMSS;		///< To store the size of the largest segment that the sender can transmit.
	UINT16 RMSS;		///< To store the size of the largest segment that the receiver is willing to accept.
	UINT16 IW;			///< Size of the sender's congestion window after the three-way handshake is completed.
	UINT16 LW;
	UINT16 ssthresh;	///< To store slow start threshold value
	UINT16 cwnd;		///< To store Congestion window size value.
	UINT16 rwnd;		///< To store the most recently advertised receiver window size value.
	double lastWinUpdateTime;
	
	//Fast Retransmit
	bool isFastRetransmit;
	UINT dupAckCount;

	//Fast Recovery
	bool isFastRecovery;

	//new reno modification for fast recovery
	UINT recover;

	//SACK
	bool isLossRecoveryPhase;

	//BIC
	BIC bic;
}CONGESTIONVAR,*PCONGESTIONVAR;

static inline bool isReno(PNETSIM_SOCKET s)
{
	return s->tcb->variant == TCPVariant_RENO ||
		s->tcb->variant == TCPVariant_NEWRENO;
}

static inline bool isBIC(PNETSIM_SOCKET s)
{
	return s->tcb->variant == TCPVariant_BIC;
}

static inline bool isNewReno(PNETSIM_SOCKET s)
{
	return s->tcb->variant == TCPVariant_NEWRENO;
}

static inline bool isFastRetransmit(PNETSIM_SOCKET s)
{
	return s->tcb->variant == TCPVariant_TAHOE ||
		s->tcb->variant == TCPVariant_RENO ||
		s->tcb->variant == TCPVariant_NEWRENO;
}

static inline bool isFastRecovery(PNETSIM_SOCKET s)
{
	return s->tcb->variant == TCPVariant_RENO ||
		s->tcb->variant == TCPVariant_NEWRENO ||
		s->tcb->variant == TCPVariant_BIC;
}

static inline PCONGESTIONVAR get_congestionvar(PNETSIM_SOCKET s)
{
	return (PCONGESTIONVAR)s->tcb->congestionData;
}

static inline void set_congestionvar(PNETSIM_SOCKET s, PCONGESTIONVAR data)
{
	s->tcb->congestionData = data;
}

static inline UINT16 congestion_get_MSS(PNETSIM_SOCKET s)
{
	return get_congestionvar(s)->SMSS;
}

static inline UINT16 congestion_get_WND(PNETSIM_SOCKET s)
{
	return get_congestionvar(s)->cwnd;
}

static void set_ssthres(PNETSIM_SOCKET s, UINT32 newssthres)
{
	PCONGESTIONVAR var = get_congestionvar(s);
	if (s->tcb->isWindowScaling)
	{
		UINT32 c = (UINT32)newssthres;
		var->ssthresh = (UINT16)(c >> s->tcb->Snd.Wind_Shift);
	}
	else
	{
		var->ssthresh = (UINT16)newssthres;
	}
}

static UINT32 get_ssthres(PNETSIM_SOCKET s)
{
	PCONGESTIONVAR var = get_congestionvar(s);
	UINT32 c = (UINT32)var->ssthresh;
	if (s->tcb->isWindowScaling)
	{
		return c << s->tcb->Snd.Wind_Shift;
	}
	else
	{
		return c;
	}
}

static void set_cwnd(PNETSIM_SOCKET s, UINT32 newcwnd)
{
	PCONGESTIONVAR var = get_congestionvar(s);
	if (s->tcb->isWindowScaling)
	{
		UINT32 c = (UINT32)newcwnd;
		var->cwnd = (UINT16)(c >> s->tcb->Snd.Wind_Shift);
		assert(var->cwnd < 65535);
		if (!var->cwnd)
			var->cwnd = 1;
	}
	else
	{
		var->cwnd = (UINT16)newcwnd;
	}
}

static UINT32 get_cwnd(PNETSIM_SOCKET s)
{
	PCONGESTIONVAR var = get_congestionvar(s);
	UINT32 c = (UINT32)var->cwnd;
	if (s->tcb->isWindowScaling)
	{
		return c << s->tcb->Snd.Wind_Shift;
	}
	else
	{
		return c;
	}
}

static UINT32 get_cwnd_print(PNETSIM_SOCKET s)
{
	PCONGESTIONVAR var = get_congestionvar(s);
	if (s->tcb->isWindowScaling)
	{
		UINT32 c = (UINT32)var->cwnd;
		return (UINT32)(c << s->tcb->Snd.Wind_Shift);
	}
	else
	{
		return (UINT32)var->cwnd;
	}
}

static inline bool tcp_in_slow_start(PNETSIM_SOCKET s)
{
	return (get_cwnd(s) <= get_ssthres(s));
}

static inline void bictcp_reset(PBIC ca)
{
	ca->cnt = 0;
	ca->last_max_cwnd = 0;
	ca->last_cwnd = 0;
	ca->last_time = 0;
	ca->epoch_start = 0;
	ca->delayed_ack = 2 << ACK_RATIO_SHIFT;
}

static void bictcp_init(PNETSIM_SOCKET sk, PTCP_DEV_VAR tcp)
{
	PBIC bic = &get_congestionvar(sk)->bic;

	bictcp_reset(bic);
	bic->loss_cwnd = 0;

	bic->low_window = tcp->low_window;
	bic->max_increment = tcp->max_increment;
	bic->fast_convergence = true;
	bic->beta = tcp->beta;
	bic->smooth_part = tcp->smooth_part;
}

static void init_congestion(PNETSIM_SOCKET s)
{
	PTCP_DEV_VAR tcp = GET_TCP_DEV_VAR(s->localDeviceId);
	PCONGESTIONVAR var = get_congestionvar(s);
	if (!var)
	{
		var = (PCONGESTIONVAR)calloc(1, sizeof* var);
		set_congestionvar(s, var);
	}
	var->SMSS = tcp->MSS;
	var->RMSS = tcp->MSS;
	set_ssthres(s, tcp->initSSThresh);
	var->cwnd = var->ssthresh;
	var->rwnd = var->ssthresh;

	var->isFastRetransmit = isFastRetransmit(s);

	if(isNewReno(s))
		var->recover = s->tcb->ISS;

	if (isBIC(s))
		bictcp_init(s, tcp);

#ifdef _TEST_CONGESTION_
	fp = fopen("Congestion.csv","w");
	fprintf(fp, "Called For,Time,CWND,ssThres,Flight Size,Ackno,UNA,\n");
	fflush(fp);
#endif
}

static void increase_cwnd(PNETSIM_SOCKET s, UINT16 increase)
{
	PCONGESTIONVAR var = get_congestionvar(s);
	if (s->tcb->isWindowScaling)
	{
		UINT32 c = window_scale_get_cwnd(s);
		c += increase;
		UINT32 cwnd = c >> s->tcb->Snd.Wind_Shift;
		if (cwnd >= 65535)
			var->cwnd = 65535; // Don't want to do this. But no option.
		else
			var->cwnd = (UINT16)cwnd;
		assert(var->cwnd <= 65535);
	}
	else
	{
		UINT32 c = var->cwnd + increase;
		if (c >= 65535)
			var->cwnd = 65535; // Don't want to do this. But no option.
		else
			var->cwnd = var->cwnd + increase;
	}
}

//RFC 3390
static void congestion_set_IW(PNETSIM_SOCKET s)
{
	PCONGESTIONVAR var = get_congestionvar(s);
	PTCP_DEV_VAR t = GET_TCP_DEV_VAR(s->localDeviceId);

	if (var->SMSS > 2190)
		var->IW = max(2 * var->SMSS, 2 * t->MSS);
	else if (var->SMSS > 1095)
		var->IW = max(3 * var->SMSS, 3 * t->MSS);
	else if (var->SMSS <= 1095)
		var->IW = max(4 * var->SMSS, 4 * t->MSS);
}

static void congestion_set_MSS(PNETSIM_SOCKET s, UINT16 mss)
{
	PCONGESTIONVAR var = get_congestionvar(s);
	var->SMSS = min(var->SMSS, mss);
	var->RMSS = min(var->RMSS, mss);

	congestion_set_IW(s);

	var->LW = var->SMSS;

	set_cwnd(s, var->IW);
	var->rwnd = var->IW;
}

static bool isDupAck(PNETSIM_SOCKET s, PCONGESTIONVAR var)
{
	return (s->tcb->SEG.ACK <= s->tcb->SND.UNA);
}

static bool isFullAck(PNETSIM_SOCKET s, PCONGESTIONVAR var)
{
	return s->tcb->SEG.ACK >= var->recover;
}

static void slowStart(PNETSIM_SOCKET s, PCONGESTIONVAR var)
{
	//Slow start mode
	UINT16 N; /* N is the number of previously unacknowledged
			* bytes acknowledged in the incoming ACK.
			*/
	N = (UINT16)(s->tcb->SEG.ACK - s->tcb->SND.UNA);
	increase_cwnd(s, min(N, var->SMSS));
}

static void CongestionAvoidance(PNETSIM_SOCKET s, PCONGESTIONVAR var)
{
	//Congestion avoidance mode
	double RTT = get_RTT(s->tcb, s->tcb->SEG.ACK);
	if (var->lastWinUpdateTime +
		RTT < pstruEventDetails->dEventTime)
	{
		var->lastWinUpdateTime = pstruEventDetails->dEventTime;
		UINT16 N = (UINT16)(s->tcb->SEG.ACK - s->tcb->SND.UNA);
		increase_cwnd(s, min(N, var->SMSS));
	}
}

static void FastRetransmit(PNETSIM_SOCKET s, PCONGESTIONVAR var)
{
	/* RFC 5681 page 9*/
	if (var->dupAckCount == TCP_DupThresh)
	{
		/* 
		2.  When the third duplicate ACK is received, a TCP MUST set ssthresh
			to no more than the value given in equation (4)
			 ssthresh = max (FlightSize / 2, 2*SMSS)            (4)
	   */
		UINT16 FlightSize = (UINT16)(s->tcb->SND.NXT - s->tcb->SND.UNA);
		set_ssthres(s, max(FlightSize / 2, 2 * var->SMSS));

		/*
		3.  The lost segment starting at SND.UNA MUST be retransmitted and
			cwnd set to ssthresh plus 3*SMSS.  This artificially "inflates"
			the congestion window by the number of segments (three) that have
			left the network and which the receiver has buffered.
		*/
		set_cwnd(s, get_ssthres(s) + TCP_DupThresh * var->SMSS);
		resend_segment_without_timeout(s, s->tcb->SEG.ACK);

		var->isFastRecovery = isFastRecovery(s);

		if (s->tcb->isSackOption)
		{
			tcp_sack_fastRetransmit(s);
			var->isLossRecoveryPhase = true;
		}
	}
	else
	{
		/*
		4.  For each additional duplicate ACK received(after the third),
			cwnd MUST be incremented by SMSS.This artificially inflates the
			congestion window in order to reflect the additional segment that
			has left the network.
		*/
		increase_cwnd(s, var->SMSS);

		if (s->tcb->isSackOption &&
			var->isLossRecoveryPhase)
		{
			var->isLossRecoveryPhase = tcp_sack_lossRecoveryPhase(s);
		}
	}
}

static void FastRecovery(PNETSIM_SOCKET s, PCONGESTIONVAR var)
{
	var->isFastRecovery = false;
	var->dupAckCount = 0;
	set_cwnd(s, get_ssthres(s));
}

static void newReno_FastRecovery(PNETSIM_SOCKET s, PCONGESTIONVAR var)
{
	var->dupAckCount = 0;
	if (isFullAck(s, var))
	{
		//Full Ack
		UINT16 FlightSize = (UINT16)(s->tcb->SND.NXT - s->tcb->SND.UNA);
		set_cwnd(s,
				 min(get_ssthres(s),
					 max(FlightSize, var->SMSS) + var->SMSS)); //---(1)

		//var->cwnd = var->ssthresh; //----(2)
		var->isFastRecovery = false;
	}
	else
	{
		//Partial Ack
		increase_cwnd (s, (UINT16)(s->tcb->SEG.ACK - var->recover));
		resend_segment_without_timeout(s, s->tcb->SEG.ACK);
	}
}

/*
*	behave like Reno until low_window is reached,
*	then increase congestion window slowly
*/
static UINT32 bictcp_recalc_ssthresh(PNETSIM_SOCKET sk)
{
	PBIC ca = &get_congestionvar(sk)->bic;

	UINT32 segCwnd = (UINT32)get_cwnd(sk)/sk->tcb->get_MSS(sk);

	ca->epoch_start = 0;	/* end of epoch */

	/* Wmax and fast convergence */
	if (segCwnd < ca->last_max_cwnd && ca->fast_convergence)
	{
		ca->last_max_cwnd = (UINT32)(segCwnd * ca->beta);
	}
	else
	{
		ca->last_max_cwnd = segCwnd;
	}
	ca->loss_cwnd = segCwnd;
	UINT16 FlightSize = (UINT16)(sk->tcb->SND.NXT - sk->tcb->SND.UNA);

	if (segCwnd <= ca->low_window)
		return max(2 * sk->tcb->get_MSS(sk), FlightSize / 2);
	else
		return (UINT32)(max(segCwnd * ca->beta, 2.0) * sk->tcb->get_MSS(sk));
}

/*
* Compute congestion window to use.
*/
static inline void bictcp_update(PNETSIM_SOCKET sk)
{
	PCONGESTIONVAR c = get_congestionvar(sk);
	PBIC ca = &c->bic;
	UINT32 cwnd = (UINT32)get_cwnd(sk)/sk->tcb->get_MSS(sk);

	ca->last_cwnd = cwnd;
	ca->last_time = (UINT32)pstruEventDetails->dEventTime;

	if (ca->epoch_start == 0) /* record the beginning of an epoch */
		ca->epoch_start = (UINT32)pstruEventDetails->dEventTime;

	/* start off normal */
	if (cwnd <= ca->low_window)
	{
		ca->cnt = cwnd;
		return;
	}

	/* binary increase */
	if (cwnd < ca->last_max_cwnd)
	{
		UINT32 dist = (ca->last_max_cwnd - cwnd) / BICTCP_B;

		if (dist > ca->max_increment)
		{
			/* linear increase */
			ca->cnt = cwnd / ca->max_increment;
		}
		else if (dist <= 1U)
		{
			/* smoothed binary search increase: when our window is really
			 * close to the last maximum, we parameterize in m_smoothPart the number
			 * of RTT needed to reach that window.
			 */
			ca->cnt = (cwnd * ca->smooth_part) / BICTCP_B;
		}
		else
		{
			/* binary search increase */
			ca->cnt = cwnd / dist;
		}
	}
	else
	{
		/* slow start AMD linear increase */
		if (cwnd < ca->last_max_cwnd + BICTCP_B)
		{
			/* slow start */
			ca->cnt = (cwnd * ca->smooth_part) / BICTCP_B;
		}
		else if (cwnd < ca->last_max_cwnd + ca->max_increment*(BICTCP_B - 1))
		{
			/* slow start */
			ca->cnt = (cwnd * (BICTCP_B - 1))
				/ (cwnd - ca->last_max_cwnd);
		}
		else
		{
			/* linear increase */
			ca->cnt = cwnd / ca->max_increment;
		}
	}
	/* if in slow start or link utilization is very low */
	if (ca->last_max_cwnd == 0)
	{
		if (ca->cnt > 20) /* increase cwnd 5% per RTT */
			ca->cnt = 20;
	}

	ca->cnt = (ca->cnt << ACK_RATIO_SHIFT) / ca->delayed_ack;
	if (ca->cnt == 0)			/* cannot be zero */
		ca->cnt = 1;
	
}

static void bictcp_cong_avoid(PNETSIM_SOCKET sk, UINT32 segmentAcked)
{
	PCONGESTIONVAR var = get_congestionvar(sk);

	if (tcp_in_slow_start(sk))
	{
		slowStart(sk, get_congestionvar(sk));
		segmentAcked--;
	}
	
	if(!tcp_in_slow_start(sk) && segmentAcked > 0)
	{
		PBIC bic = &var->bic;

		bic->cwndcnt += segmentAcked;
		bictcp_update(sk);
		UINT32 cnt = bic->cnt;
		/* According to the BIC paper and RFC 6356 even once the new cwnd is
		 * calculated you must compare this to the number of ACKs received since
		 * the last cwnd update. If not enough ACKs have been received then cwnd
		 * cannot be updated.
		*/
		if (bic->cwndcnt > cnt)
		{
			increase_cwnd(sk, var->SMSS);
			bic->cwndcnt = 0;
		}
		else
		{
			//Not enough segments have been ACKed to increment cwnd
		}
	}
}

static void bictcp_fastretransmit(PNETSIM_SOCKET s)
{
	PCONGESTIONVAR var = get_congestionvar(s);

	if (var->dupAckCount == TCP_DupThresh)
	{
		UINT32 ssThresh = bictcp_recalc_ssthresh(s);
		set_ssthres(s, (UINT16)ssThresh);

		/*
		3.  The lost segment starting at SND.UNA MUST be retransmitted and
		cwnd set to ssthresh plus 3*SMSS.  This artificially "inflates"
		the congestion window by the number of segments (three) that have
		left the network and which the receiver has buffered.
		*/
		set_cwnd(s, get_ssthres(s) + TCP_DupThresh * var->SMSS);
		resend_segment_without_timeout(s, s->tcb->SEG.ACK);

		var->isFastRecovery = isFastRecovery(s);

		if (s->tcb->isSackOption)
		{
			tcp_sack_fastRetransmit(s);
			var->isLossRecoveryPhase = true;
		}
	}
	else
	{
		/*
		4.  For each additional duplicate ACK received(after the third),
		cwnd MUST be incremented by SMSS.This artificially inflates the
		congestion window in order to reflect the additional segment that
		has left the network.
		*/
		increase_cwnd(s, var->SMSS);

		if (s->tcb->isSackOption &&
			var->isLossRecoveryPhase)
		{
			var->isLossRecoveryPhase = tcp_sack_lossRecoveryPhase(s);
		}
	}
}

/* Track delayed acknowledgment ratio using sliding window
* ratio = (15*ratio + sample) / 16
*/
static void bictcp_acked(PNETSIM_SOCKET sk)
{
	BIC ca = get_congestionvar(sk)->bic;

	UINT32 pkts_acked = (sk->tcb->SEG.ACK - sk->tcb->SND.UNA) /
		sk->tcb->get_MSS(sk);

	ca.delayed_ack += pkts_acked -
		(ca.delayed_ack >> ACK_RATIO_SHIFT);
}

static void oldtahoe_ack_received(PNETSIM_SOCKET s)
{
	PCONGESTIONVAR var = get_congestionvar(s);
	
	if (get_cwnd(s) <= get_ssthres(s))
		slowStart(s, var);
	else
		CongestionAvoidance(s, var);

	write_congestion_plot(s, pstruEventDetails->dEventTime / 1000.0, get_cwnd(s));

#ifdef _TEST_CONGESTION_
	fprintf(fp, "Ack,%lf,%d,%d,%d,%d,%d\n",
			pstruEventDetails->dEventTime,
			get_cwnd_print(s),
			get_ssthres(s),
			s->tcb->SND.NXT - s->tcb->SND.UNA,
			s->tcb->SEG.ACK,s->tcb->SND.UNA);
	fflush(fp);
#endif
}

static void tahoe_ack_received(PNETSIM_SOCKET s)
{
	PCONGESTIONVAR var = get_congestionvar(s);
	bool isdup = isDupAck(s, var);
	if (isdup)
	{
		var->dupAckCount++;
		if (var->dupAckCount < TCP_DupThresh)
		{
			if (s->tcb->isSackOption &&
				tcp_sack_isLost(s, get_highAck(s) + 1))
				FastRetransmit(s, var);
		}
		else
		{
			FastRetransmit(s, var);
		}
	}
	else if (var->dupAckCount)
	{
		var->dupAckCount = 0;
		set_cwnd(s, var->LW);
	}
	else
	{
		if (get_cwnd(s) <= get_ssthres(s))
			slowStart(s, var);
		else
			CongestionAvoidance(s, var);
	}

	write_congestion_plot(s, pstruEventDetails->dEventTime / 1000.0, get_cwnd(s));

#ifdef _TEST_CONGESTION_
	fprintf(fp, "Ack,%lf,%d,%d,%d,%d,%d\n",
			pstruEventDetails->dEventTime,
			get_cwnd_print(s),
			get_ssthres(s),
			s->tcb->SND.NXT - s->tcb->SND.UNA,
			s->tcb->SEG.ACK,s->tcb->SND.UNA);
	fflush(fp);
#endif
}

static void reno_ack_received(PNETSIM_SOCKET s)
{
	PCONGESTIONVAR var = get_congestionvar(s);
	bool isdup = isDupAck(s, var);
	if (isdup)
	{
		var->dupAckCount++;
		if (var->dupAckCount < TCP_DupThresh)
		{
			if (s->tcb->isSackOption &&
				tcp_sack_isLost(s, get_highAck(s) + 1))
				FastRetransmit(s, var);
		}
		else
		{
			FastRetransmit(s, var);
		}
	}
	else if (var->isFastRecovery)
	{
		FastRecovery(s, var);
	}
	else
	{
		if (get_cwnd(s) <= get_ssthres(s))
			slowStart(s, var);
		else
			CongestionAvoidance(s, var);
	}

	write_congestion_plot(s, pstruEventDetails->dEventTime / 1000.0, get_cwnd(s));

#ifdef _TEST_CONGESTION_
	fprintf(fp, "Ack,%lf,%d,%d,%d,%d,%d\n",
			pstruEventDetails->dEventTime,
			get_cwnd_print(s),
			get_ssthres(s),
			s->tcb->SND.NXT - s->tcb->SND.UNA,
			s->tcb->SEG.ACK, s->tcb->SND.UNA);
	fflush(fp);
#endif
}

static void newreno_ack_received(PNETSIM_SOCKET s)
{
	PCONGESTIONVAR var = get_congestionvar(s);
	bool isdup = isDupAck(s, var);
	if (isdup)
	{
		var->dupAckCount++;
		if (var->dupAckCount < TCP_DupThresh)
		{
			if (s->tcb->isSackOption &&
				tcp_sack_isLost(s, get_highAck(s) + 1))
				FastRetransmit(s, var);
		}
		else
		{
			if (var->recover < s->tcb->SEG.ACK - 1)
			{
				var->recover = s->tcb->SND.NXT;

				FastRetransmit(s, var);
			}
		}
	}
	else if (var->isFastRecovery)
	{
		newReno_FastRecovery(s, var);
	}
	else
	{
		if (get_cwnd(s) <= get_ssthres(s))
			slowStart(s, var);
		else
			CongestionAvoidance(s, var);
	}

	write_congestion_plot(s, pstruEventDetails->dEventTime / 1000.0, get_cwnd(s));

#ifdef _TEST_CONGESTION_
	fprintf(fp, "Ack,%lf,%d,%d,%d,%d,%d\n",
			pstruEventDetails->dEventTime,
			get_cwnd_print(s),
			get_ssthres(s),
			s->tcb->SND.NXT - s->tcb->SND.UNA,
			s->tcb->SEG.ACK, s->tcb->SND.UNA);
	fflush(fp);
#endif
}

static void bic_ack_received(PNETSIM_SOCKET s)
{
	PCONGESTIONVAR var = get_congestionvar(s);
	UINT32 segmentAcked = (s->tcb->SEG.ACK - s->tcb->SND.UNA)/
		s->tcb->get_MSS(s);

	bool isdup = isDupAck(s, var);
	if (isdup)
	{
		var->dupAckCount++;
		if (var->dupAckCount < TCP_DupThresh)
		{
			if (s->tcb->isSackOption &&
				tcp_sack_isLost(s, get_highAck(s) + 1))
			{
				bictcp_fastretransmit(s);
			}
		}
		else
		{
			bictcp_fastretransmit(s);
		}
	}
	else if (var->isFastRecovery)
	{
		FastRecovery(s, var);
	}
	else
	{
		bictcp_acked(s);
		bictcp_cong_avoid(s,segmentAcked);
	}

	write_congestion_plot(s, pstruEventDetails->dEventTime / 1000.0, get_cwnd(s));

#ifdef _TEST_CONGESTION_
	fprintf(fp, "Ack,%lf,%d,%d,%d,%d,%d\n",
			pstruEventDetails->dEventTime,
			get_cwnd_print(s),
			get_ssthres(s),
			s->tcb->SND.NXT - s->tcb->SND.UNA,
			s->tcb->SEG.ACK, s->tcb->SND.UNA);
	fflush(fp);
#endif
}

static void congestion_rto_timeout(PNETSIM_SOCKET s)
{
	PCONGESTIONVAR var = get_congestionvar(s);
	UINT16 FlightSize = (UINT16)(s->tcb->SND.NXT - s->tcb->SND.UNA);
	set_ssthres(s, max(FlightSize / 2, 2 * var->SMSS));
	set_cwnd(s, var->LW);

	if (isNewReno(s))
	{
		var->recover = s->tcb->SND.NXT;
		var->isFastRecovery = false;
	}

	write_congestion_plot(s, pstruEventDetails->dEventTime / 1000.0, get_cwnd(s));

#ifdef _TEST_CONGESTION_
	fprintf(fp, "RTO,%lf,%d,%d,%d,%d,%d\n",
			pstruEventDetails->dEventTime,
			get_cwnd_print(s),
			get_ssthres(s),
			s->tcb->SND.NXT - s->tcb->SND.UNA,
			s->tcb->SEG.ACK, s->tcb->SND.UNA);
	fflush(fp);
#endif
}

void congestion_setcallback(PNETSIM_SOCKET s)
{
	s->tcb->init_congestionalgo = init_congestion;

	switch (s->tcb->variant)
	{
	case TCPVariant_OLDTAHOE:
		s->tcb->ack_received = oldtahoe_ack_received;
		break;
	case TCPVariant_TAHOE:
		s->tcb->ack_received = tahoe_ack_received;
		break;
	case TCPVariant_RENO:
		s->tcb->ack_received = reno_ack_received;
		break;
	case TCPVariant_NEWRENO:
		s->tcb->ack_received = newreno_ack_received;
		break;
	case TCPVariant_BIC:
		s->tcb->ack_received = bic_ack_received;
		break;
	case TCPVariant_CUBIC:
		s->tcb->init_congestionalgo = init_cubic;
		s->tcb->ack_received = cubic_ack_received;
		//Other function will work as both structure are same initially. :):)
		//But is it correct to do? Nope.
		break;
	default:
		fnNetSimError("Unknown TCP variant %d in %s\n",
					  s->tcb->variant,
					  __FUNCTION__);
		break;
	}
	
	s->tcb->rto_expired = congestion_rto_timeout;
	s->tcb->get_MSS = congestion_get_MSS;
	s->tcb->get_WND = congestion_get_WND;
	s->tcb->set_MSS = congestion_set_MSS;

	s->tcb->init_congestionalgo(s);
}
