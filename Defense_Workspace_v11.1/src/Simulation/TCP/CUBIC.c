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
#include <intrin.h>

//#define _TEST_CONGESTION_

#ifdef _TEST_CONGESTION_
static FILE* fp;
#endif

#define clamp(val, lo, hi) min(max(val, lo), hi)

#define tcp_time_stamp ((UINT32)(pstruEventDetails->dEventTime/1000))
#define HZ 1024

#define BICTCP_BETA_SCALE    1024	/* Scale factor beta calculation
* max_cwnd = snd_cwnd * beta
*/
#define	BICTCP_HZ		10	/* BIC HZ 2^10 = 1024 */

/* Two methods of hybrid slow start */
#define HYSTART_ACK_TRAIN	0x1
#define HYSTART_DELAY		0x2

/* Number of delay samples for detecting the increase of delay */
#define HYSTART_MIN_SAMPLES	8
#define HYSTART_DELAY_MIN	(4U<<3)
#define HYSTART_DELAY_MAX	(16U<<3)
#define HYSTART_DELAY_THRESH(x)	clamp(x, HYSTART_DELAY_MIN, HYSTART_DELAY_MAX)

static int fast_convergence = 1;
static int tcp_friendliness = 1;

static int hystart = 1;
static int hystart_detect = HYSTART_ACK_TRAIN | HYSTART_DELAY;


static UINT32 cube_rtt_scale;
static UINT32 beta_scale;
static UINT32 cube_factor;

typedef struct stru_cubic
{
	UINT32 hystart_low_window;
	int hystart_ack_delta;
	int bic_scale;
	int beta; /* = 717/1024 (BICTCP_BETA_SCALE) */
	UINT32 initial_ssthresh;

	UINT32 cnt;				/* increase cwnd by 1 after ACKs */
	UINT32 last_max_cwnd;	/* last maximum snd_cwnd */
	UINT32 loss_cwnd;		/* congestion window at last loss */
	UINT32 last_cwnd;		/* the last snd_cwnd */
	UINT32 last_time;		/* time when updated last_cwnd */
	UINT32 bic_origin_point;/* origin point of bic function */
	UINT32 bic_K;			/* time to origin point
								from the beginning of the current epoch */
	UINT32 delay_min;		/* min delay (msec << 3) */
	UINT32 epoch_start;		/* beginning of an epoch */
	UINT32 ack_cnt;			/* number of acks */
	UINT32 tcp_cwnd;		/* estimated tcp cwnd */
	UINT32 unused;
	UINT8 sample_cnt;		/* number of samples to decide curr_rtt */
	UINT8 found;			/* the exit point is found? */
	UINT32 round_start;		/* beginning of each round */
	UINT32 end_seq;			/* end_seq of the round */
	UINT32 last_ack;		/* last time when the ACK spacing is close */
	UINT32 curr_rtt;		/* the minimum rtt of current round */
	UINT32 cwndcnt;
}CUBIC, *PCUBIC;

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

	//SACK
	bool isLossRecoveryPhase;

	//CUBIC
	CUBIC cubic;
}CONGESTIONVAR, *PCONGESTIONVAR;

static inline PCONGESTIONVAR get_congestionvar(PNETSIM_SOCKET s)
{
	return (PCONGESTIONVAR)s->tcb->congestionData;
}

static inline void set_congestionvar(PNETSIM_SOCKET s, PCONGESTIONVAR data)
{
	s->tcb->congestionData = data;
}

static void set_cwnd(PNETSIM_SOCKET s, UINT32 newcwnd)
{
	PCONGESTIONVAR var = get_congestionvar(s);
	if (s->tcb->isWindowScaling)
	{
		UINT32 c = (UINT32)newcwnd;
		var->cwnd = (UINT16)(c >> s->tcb->Snd.Wind_Shift);
		assert(var->cwnd < 65535);
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

static void increase_cwnd(PNETSIM_SOCKET s, UINT16 increase)
{
	PCONGESTIONVAR var = get_congestionvar(s);
	if (s->tcb->isWindowScaling)
	{
		UINT32 c = window_scale_get_cwnd(s);
		c += increase;
		var->cwnd = (UINT16)(c >> s->tcb->Snd.Wind_Shift);
		assert(var->cwnd < 65535);
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

static inline bool tcp_in_slow_start(PNETSIM_SOCKET s)
{
	return (get_cwnd(s) <= get_ssthres(s));
}

static bool isDupAck(PNETSIM_SOCKET s, PCONGESTIONVAR var)
{
	return (s->tcb->SEG.ACK <= s->tcb->SND.UNA);
}

/**
* div64_u64 - unsigned 64bit divide with 64bit divisor
*/
static inline UINT64 div64_u64(UINT64 dividend, UINT64 divisor)
{
	return dividend / divisor;
}

#define do_div(x,y) (x=x/y)

static inline bool before(UINT32 seq1, UINT32 seq2)
{
	return (int)(seq1 - seq2) < 0;
}
#define after(seq2, seq1) 	before(seq1, seq2)

static UINT count_bit(UINT64 n)
{
	char a[100];
	_i64toa(n, a, 2);
	int i;
	int len = strlen(a);
	for (i = 0; i < len; i++)
		if (a[i] == '1')
			return i;
	return 0;
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

static inline void cubictcp_reset(PCUBIC ca)
{
	ca->cnt = 0;
	ca->last_max_cwnd = 0;
	ca->last_cwnd = 0;
	ca->last_time = 0;
	ca->bic_origin_point = 0;
	ca->bic_K = 0;
	ca->delay_min = 0;
	ca->epoch_start = 0;
	ca->ack_cnt = 0;
	ca->tcp_cwnd = 0;
	ca->found = 0;
}

static inline UINT32 cubictcp_clock(void)
{
	return (UINT32)(pstruEventDetails->dEventTime / 1000);
}

static inline void cubictcp_hystart_reset(PNETSIM_SOCKET sk)
{
	PCUBIC ca = &get_congestionvar(sk)->cubic;

	ca->round_start = ca->last_ack = cubictcp_clock();
	ca->end_seq = sk->tcb->SND.NXT;
	ca->curr_rtt = 0;
	ca->sample_cnt = 0;
}

static void cubictcp_init(PNETSIM_SOCKET sk, PTCP_DEV_VAR tcp)
{
	PCUBIC ca = &get_congestionvar(sk)->cubic;

	cubictcp_reset(ca);
	ca->loss_cwnd = 0;

	ca->hystart_ack_delta = tcp->hystart_ack_delta;
	ca->hystart_low_window = tcp->hystart_low_window;
	ca->beta = (int)tcp->beta;
	ca->bic_scale = tcp->bic_scale;
	ca->initial_ssthresh = tcp->initSSThresh;

	if (hystart)
		cubictcp_hystart_reset(sk);
}

void init_cubic(PNETSIM_SOCKET s)
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

	var->isFastRetransmit = true;

	cubictcp_init(s,tcp);
	PCUBIC cubic = &var->cubic;

	beta_scale = 8 * (BICTCP_BETA_SCALE + cubic->beta) / 3
		/ (BICTCP_BETA_SCALE - cubic->beta);

	cube_rtt_scale = (cubic->bic_scale * 10);	/* 1024*c/rtt */

										/* calculate the "K" for (wmax-cwnd) = c/rtt * K^3
										*  so K = cubic_root( (wmax-cwnd)*rtt/c )
										* the unit of K is bictcp_HZ=2^10, not HZ
										*
										*  c = bic_scale >> 10
										*  rtt = 100ms
										*
										* the following code has been designed and tested for
										* cwnd < 1 million packets
										* RTT < 100 seconds
										* HZ < 1,000,00  (corresponding to 10 nano-second)
										*/

										/* 1/c * 2^2*bictcp_HZ * srtt */

#pragma warning(disable:4310)
	cube_factor = (UINT32)(1ull << (10 + 3 * BICTCP_HZ)); /* 2^40 */
#pragma warning(default:4310)

												/* divide by bic_scale and by constant Srtt (100ms) */
	do_div(cube_factor, cubic->bic_scale * 10);


#ifdef _TEST_CONGESTION_
	fp = fopen("Congestion.csv", "w");
	fprintf(fp, "Called For,Time,CWND,ssThres,Flight Size,Ackno,UNA,\n");
	fflush(fp);
#endif
}

/* calculate the cubic root of x using a table lookup followed by one
* Newton-Raphson iteration.
* Avg err ~= 0.195%
*/
static UINT32 cubic_root(UINT64 a)
{
	UINT32 x, b, shift;
	/*
	* cbrt(x) MSB values for x MSB values in [0..63].
	* Precomputed then refined by hand - Willy Tarreau
	*
	* For x in [0..63],
	*   v = cbrt(x << 18) - 1
	*   cbrt(x) = (v[x] + 10) >> 6
	*/
	static const UINT8 v[] = {
		/* 0x00 */    0,   54,   54,   54,  118,  118,  118,  118,
		/* 0x08 */  123,  129,  134,  138,  143,  147,  151,  156,
		/* 0x10 */  157,  161,  164,  168,  170,  173,  176,  179,
		/* 0x18 */  181,  185,  187,  190,  192,  194,  197,  199,
		/* 0x20 */  200,  202,  204,  206,  209,  211,  213,  215,
		/* 0x28 */  217,  219,  221,  222,  224,  225,  227,  229,
		/* 0x30 */  231,  232,  234,  236,  237,  239,  240,  242,
		/* 0x38 */  244,  245,  246,  248,  250,  251,  252,  254,
	};
	
	b = count_bit(a);
	if (b < 7) {
		/* a in [0..63] */
		return ((UINT32)v[(UINT32)a] + 35) >> 6;
	}

	b = ((b * 84) >> 8) - 1;
	shift = (UINT32)(a >> (b * 3));

	x = ((UINT32)(((UINT32)v[shift] + 10) << b)) >> 6;

	/*
	* Newton-Raphson iteration
	*                         2
	* x    = ( 2 * x  +  a / x  ) / 3
	*  k+1          k         k
	*/
	x = (2 * x + (UINT32)div64_u64(a, (UINT64)x * (UINT64)(x - 1)));
	x = ((x * 341) >> 10);
	return x;
}

static UINT32 cubictcp_recalc_ssthresh(PNETSIM_SOCKET sk)
{
	PCUBIC ca = &get_congestionvar(sk)->cubic;

	UINT32 segCwnd = (UINT32)get_cwnd(sk) / sk->tcb->get_MSS(sk);

	ca->epoch_start = 0;	/* end of epoch */

							/* Wmax and fast convergence */
	if (segCwnd < ca->last_max_cwnd && fast_convergence)
		ca->last_max_cwnd = (segCwnd * (BICTCP_BETA_SCALE + ca->beta))
		/ (2 * BICTCP_BETA_SCALE);
	else
		ca->last_max_cwnd = segCwnd;

	ca->loss_cwnd = segCwnd;

	return max((segCwnd * ca->beta) / BICTCP_BETA_SCALE, 2U);
}

/*
* Compute congestion window to use.
*/
static inline void cubictcp_update(PNETSIM_SOCKET sk, UINT32 acked)
{
	PCONGESTIONVAR c = get_congestionvar(sk);
	PCUBIC ca = &c->cubic;
	UINT32 cwnd = (UINT32)get_cwnd(sk) / sk->tcb->get_MSS(sk);

	UINT32 delta, bic_target, max_cnt;
	UINT64 offs, t;

	ca->ack_cnt += acked;	/* count the number of ACKed packets */

	if (ca->last_cwnd == cwnd &&
		(int)(tcp_time_stamp - ca->last_time) <= HZ / 32)
		return;

	/* The CUBIC function can update ca->cnt at most once per jiffy.
	* On all cwnd reduction events, ca->epoch_start is set to 0,
	* which will force a recalculation of ca->cnt.
	*/
	if (ca->epoch_start && tcp_time_stamp == ca->last_time)
		goto tcp_friendliness;

	ca->last_cwnd = cwnd;
	ca->last_time = tcp_time_stamp;

	if (ca->epoch_start == 0)
	{
		ca->epoch_start = tcp_time_stamp;	/* record beginning */
		ca->ack_cnt = acked;			/* start counting */
		ca->tcp_cwnd = cwnd;			/* syn with cubic */

		if (ca->last_max_cwnd <= cwnd)
		{
			ca->bic_K = 0;
			ca->bic_origin_point = cwnd;
		}
		else
		{
			/* Compute new K based on
			* (wmax-cwnd) * (srtt>>3 / HZ) / c * 2^(3*bictcp_HZ)
			*/
			ca->bic_K = cubic_root(cube_factor
				* (ca->last_max_cwnd - cwnd));
			ca->bic_origin_point = ca->last_max_cwnd;
		}
	}

	/* cubic function - calc*/
	/* calculate c * time^3 / rtt,
	*  while considering overflow in calculation of time^3
	* (so time^3 is done by using 64 bit)
	* and without the support of division of 64bit numbers
	* (so all divisions are done by using 32 bit)
	*  also NOTE the unit of those veriables
	*	  time  = (t - K) / 2^bictcp_HZ
	*	  c = bic_scale >> 10
	* rtt  = (srtt >> 3) / HZ
	* !!! The following code does not have overflow problems,
	* if the cwnd < 1 million packets !!!
	*/

	t = (int)(tcp_time_stamp - ca->epoch_start);
	t += (ca->delay_min >> 3);
	/* change the unit from HZ to bictcp_HZ */
	t <<= BICTCP_HZ;
	do_div(t, HZ);

	if (t < ca->bic_K)		/* t - K */
		offs = ca->bic_K - t;
	else
		offs = t - ca->bic_K;

	/* c/rtt * (t-K)^3 */
	delta = (cube_rtt_scale * offs * offs * offs) >> (10 + 3 * BICTCP_HZ);
	if (t < ca->bic_K)                            /* below origin*/
		bic_target = ca->bic_origin_point - delta;
	else                                          /* above origin*/
		bic_target = ca->bic_origin_point + delta;

	/* cubic function - calc bictcp_cnt*/
	if (bic_target > cwnd)
	{
		ca->cnt = cwnd / (bic_target - cwnd);
	}
	else
	{
		ca->cnt = 100 * cwnd;              /* very small increment*/
	}

	/*
	* The initial growth of cubic function may be too conservative
	* when the available bandwidth is still unknown.
	*/
	if (ca->last_max_cwnd == 0 && ca->cnt > 20)
		ca->cnt = 20;	/* increase cwnd 5% per RTT */

tcp_friendliness:
	/* TCP Friendly */
	if (tcp_friendliness)
	{
		UINT32 scale = beta_scale;

		delta = (cwnd * scale) >> 3;
		while (delta && ca->ack_cnt > delta)
		{		/* update tcp cwnd */
			ca->ack_cnt -= delta;
			ca->tcp_cwnd++;
		}

		if (ca->tcp_cwnd > cwnd)
		{	/* if bic is slower than tcp */
			delta = ca->tcp_cwnd - cwnd;
			max_cnt = cwnd / delta;
			if (ca->cnt > max_cnt)
				ca->cnt = max_cnt;
		}
	}

	/* The maximum rate of cwnd increase CUBIC allows is 1 packet per
	* 2 packets ACKed, meaning cwnd grows at 1.5x per RTT.
	*/
	ca->cnt = max(ca->cnt, 2U);

}

static void cubictcp_cong_avoid(PNETSIM_SOCKET sk, UINT32 segmentAcked)
{
	PCONGESTIONVAR var = get_congestionvar(sk);
	PCUBIC ca = &var->cubic;
	UINT32 ack = sk->tcb->SEG.ACK;

	if (tcp_in_slow_start(sk))
	{
		if (hystart && after(ack, ca->end_seq))
			cubictcp_hystart_reset(sk);
		slowStart(sk, get_congestionvar(sk));
		segmentAcked--;
	}

	if (!tcp_in_slow_start(sk) && segmentAcked > 0)
	{

		ca->cwndcnt += segmentAcked;
		cubictcp_update(sk,segmentAcked);
		UINT32 cnt = ca->cnt;
		/* According to the BIC paper and RFC 6356 even once the new cwnd is
		* calculated you must compare this to the number of ACKs received since
		* the last cwnd update. If not enough ACKs have been received then cwnd
		* cannot be updated.
		*/
		if (ca->cwndcnt > cnt)
		{
			increase_cwnd(sk, var->SMSS);
			ca->cwndcnt = 0;
		}
		else
		{
			//Not enough segments have been ACKed to increment cwnd
		}
	}
}

static void hystart_update(PNETSIM_SOCKET sk, UINT32 delay)
{
	PCONGESTIONVAR var = get_congestionvar(sk);
	PCUBIC ca = &var->cubic;

	if (ca->found & hystart_detect)
		return;

	if (hystart_detect & HYSTART_ACK_TRAIN) {
		UINT32 now = cubictcp_clock();

		/* first detection parameter - ack-train detection */
		if ((int)(now - ca->last_ack) <= ca->hystart_ack_delta)
		{
			ca->last_ack = now;
			if ((int)(now - ca->round_start) > ca->delay_min >> 4)
			{
				ca->found |= HYSTART_ACK_TRAIN;
				if(get_ssthres(sk) != ca->initial_ssthresh)
					set_cwnd(sk, get_ssthres(sk));
			}
		}
	}

	if (hystart_detect & HYSTART_DELAY)
	{
		/* obtain the minimum delay of more than sampling packets */
		if (ca->sample_cnt < HYSTART_MIN_SAMPLES)
		{
			if (ca->curr_rtt == 0 || ca->curr_rtt > delay)
				ca->curr_rtt = delay;

			ca->sample_cnt++;
		}
		else
		{
			if (ca->curr_rtt > ca->delay_min +
				HYSTART_DELAY_THRESH(ca->delay_min >> 3)) 
			{
				ca->found |= HYSTART_DELAY;
				set_ssthres(sk, get_cwnd(sk));
			}
		}
	}
}

/* Track delayed acknowledgment ratio using sliding window
* ratio = (15*ratio + sample) / 16
*/
static void cubictcp_acked(PNETSIM_SOCKET sk)
{
	PCUBIC ca = &get_congestionvar(sk)->cubic;
	UINT32 delay;
	UINT32 cwnd = (UINT32)get_cwnd(sk) / sk->tcb->get_MSS(sk);
	delay = (((UINT32)get_RTT(sk->tcb, sk->tcb->SEG.ACK)) << 3) / MILLISECOND;
	if (!delay)
		delay = 1;

	/* hystart triggers when cwnd is larger than some threshold */
	if (hystart && tcp_in_slow_start(sk) &&
		cwnd >= ca->hystart_low_window)
		hystart_update(sk, delay);
}

static void cubictcp_fastretransmit(PNETSIM_SOCKET s)
{
	PCONGESTIONVAR var = get_congestionvar(s);

	if (var->dupAckCount == TCP_DupThresh)
	{
		UINT32 ssThresh = cubictcp_recalc_ssthresh(s);
		set_ssthres(s, ssThresh*s->tcb->get_MSS(s));

		/*
		3.  The lost segment starting at SND.UNA MUST be retransmitted and
		cwnd set to ssthresh plus 3*SMSS.  This artificially "inflates"
		the congestion window by the number of segments (three) that have
		left the network and which the receiver has buffered.
		*/
		set_cwnd(s, get_ssthres(s) + TCP_DupThresh * var->SMSS);
		resend_segment_without_timeout(s, s->tcb->SEG.ACK);

		var->isFastRecovery = true;

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

void cubic_ack_received(PNETSIM_SOCKET s)
 {
	PCONGESTIONVAR var = get_congestionvar(s);
	UINT32 segmentAcked = (s->tcb->SEG.ACK - s->tcb->SND.UNA) /
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
				cubictcp_fastretransmit(s);
			}
		}
		else
		{
			cubictcp_fastretransmit(s);
		}
	}
	else if (var->isFastRecovery)
	{
		FastRecovery(s, var);
	}
	else
	{
		cubictcp_acked(s);
		cubictcp_cong_avoid(s, segmentAcked);
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
