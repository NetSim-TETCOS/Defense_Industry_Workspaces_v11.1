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

void set_ack_type(PNETSIM_SOCKET s, PTCP_DEV_VAR tcp)
{
	s->tcb->ackType = tcp->ackType;
	s->tcb->delayedAckTime = tcp->dDelayedAckTime;
	s->tcb->isAckSentLastTime = false;
}

bool send_ack_or_not(PNETSIM_SOCKET s)
{
	if (s->tcb->ackType == TCPACKTYPE_UNDELAYED)
		return true;

	if (s->tcb->lastAckSendTime +
		s->tcb->delayedAckTime <= pstruEventDetails->dEventTime)
	{
		s->tcb->lastAckSendTime = pstruEventDetails->dEventTime;
		s->tcb->isAckSentLastTime = true;
		return true;
	}
	else
	{
		if (s->tcb->isAckSentLastTime)
		{
			s->tcb->isAckSentLastTime = false;
			return false;
		}
		else
		{
			s->tcb->lastAckSendTime = pstruEventDetails->dEventTime;
			s->tcb->isAckSentLastTime = true;
			return true;
		}
	}
}
