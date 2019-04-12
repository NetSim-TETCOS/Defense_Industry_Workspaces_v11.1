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
#include "TCP_Enum.h"
#include "TCP_Header.h"

//RFC 6298
#define alpha	(1/8.0)
#define beta	(1/4.0)
#define G		(0.5*SECOND) /* Min RTO value is set to 0.5 s.
							  * The RFC states this should be 1 s
							  *	Linux seems to be currently using 200 ms.
							  */
#define K		(4)

double calculate_RTO(double R,
					 double* srtt,
					 double* rtt_var)
{
	double rto;
	if (!R) //2.1
		rto = 1 * SECOND;
	else if (!*srtt) //2.2
	{
		*srtt = R;
		*rtt_var = R / 2.0;
		rto = *srtt + max(G, K*(*rtt_var));
	}
	else //2.3
	{
		*rtt_var = (1 - beta) * (*rtt_var) + beta *  fabs(*srtt - R);
		*srtt = (1 - alpha) * (*srtt) + alpha * R;
		rto = *srtt + max(G, K*(*rtt_var));
	}
	return min(max(rto, G), (60 * SECOND)); //2.4 and 2.5
}

static void Backoff_RTO(double* rto)
{
	*rto = min(max((*rto*2), G), (60 * SECOND));
	print_tcp_log("New RTO = %0.2lf", *rto);
}

void add_timeout_event(PNETSIM_SOCKET s,
					   NetSim_PACKET* packet)
{
	NetSim_PACKET* p = fn_NetSim_Packet_CopyPacket(packet);
	add_packet_to_queue(&s->tcb->retransmissionQueue, p, pstruEventDetails->dEventTime);
	NetSim_EVENTDETAILS pevent;
	memcpy(&pevent, pstruEventDetails, sizeof pevent);
	pevent.dEventTime += TCP_RTO(s->tcb);
	pevent.dPacketSize = packet->pstruTransportData->dPacketSize;
	pevent.nEventType = TIMER_EVENT;
	pevent.nPacketId = packet->nPacketId;
	if (packet->pstruAppData)
	{
		pevent.nApplicationId = packet->pstruAppData->nApplicationId;
		pevent.nSegmentId = packet->pstruAppData->nSegmentId;
	}
	else
		pevent.nSegmentId = 0;
	pevent.nProtocolId = TX_PROTOCOL_TCP;
	pevent.pPacket = fn_NetSim_Packet_CopyPacket(p);
	pevent.szOtherDetails = NULL;
	pevent.nSubEventType = TCP_RTO_TIMEOUT;
	fnpAddEvent(&pevent);
	print_tcp_log("Adding RTO Timer at %0.1lf", pevent.dEventTime);
}

static void handle_rto_timer_for_ctrl(PNETSIM_SOCKET s)
{
	if (isSynbitSet(pstruEventDetails->pPacket))
		resend_syn(s);
	else
		fnNetSimError("Unknown packet %d arrives at %s\n",
					  pstruEventDetails->pPacket->nControlDataType,
					  __FUNCTION__);
}
void handle_rto_timer()
{
	PNETSIM_SOCKET s = find_socket_at_source(pstruEventDetails->pPacket);
	if (!s)
	{
		fnNetSimError("%s is called without proper socket.\n" \
					  "This may also happen due to\n"\
					  "1. Multiple syn error\n"\
					  "2. RST packet received\n"\
					  "Ignore this error if happen due to above. Check tcp log for more details.\n",
					  __FUNCTION__);
		return;
	}
	if (!s->tcb || s->tcb->isOtherTimerCancel)
		return;

	bool isPresent = isSegmentInQueue(&s->tcb->retransmissionQueue, pstruEventDetails->pPacket);
	if (isPresent)
	{
		print_tcp_log("\nDevice %d, Time %0.2lf: RTO Time expire.",
					  pstruEventDetails->nDeviceId,
					  pstruEventDetails->dEventTime);
		
		s->tcpMetrics->timesRTOExpired++;

		Backoff_RTO(&TCP_RTO(s->tcb));

		if (isTCPControl(pstruEventDetails->pPacket))
		{
			handle_rto_timer_for_ctrl(s);
		}
		else
		{
			//Call congestion algorithm
			s->tcb->rto_expired(s);

			resend_segment(s, pstruEventDetails->pPacket);
		}
	}
	else
	{
		//Ignore this event
		fn_NetSim_Packet_FreePacket(pstruEventDetails->pPacket);
	}
}