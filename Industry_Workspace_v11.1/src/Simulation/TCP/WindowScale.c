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

void set_window_scaling(PNETSIM_SOCKET s, PWsopt opt)
{
	if (opt && s->tcb->isWindowScaling)
	{
		s->tcb->Snd.Wind_Shift = min(s->tcb->Snd.Wind_Shift, opt->Shift_cnt);
		s->tcb->Rcv.Wind_Shift = min(s->tcb->Rcv.Wind_Shift, opt->Shift_cnt);
	}
	else
	{
		s->tcb->isWindowScaling = false;
		s->tcb->Snd.Wind_Shift = 0;
		s->tcb->Rcv.Wind_Shift = 0;
	}
}

UINT8 get_shift_count(PNETSIM_SOCKET s)
{
	return s->tcb->isWindowScaling ? s->tcb->Snd.Wind_Shift : 0;
}

void set_window_scaling_option(PNETSIM_SOCKET s, PTCP_DEV_VAR tcp)
{
	if (tcp->isWindowScaling)
	{
		s->tcb->isWindowScaling = true;
		s->tcb->Snd.Wind_Shift = tcp->shiftCount;
		s->tcb->Rcv.Wind_Shift = tcp->shiftCount;
	}
}

UINT32 window_scale_get_cwnd(PNETSIM_SOCKET s)
{
	UINT32 c = (UINT32)s->tcb->get_WND(s);
	if (s->tcb->isWindowScaling)
	{
		return (c << s->tcb->Snd.Wind_Shift);
	}
	else
		return (c);
}

UINT16 window_scale_get_wnd(PNETSIM_SOCKET s)
{
	if (s->tcb->isWindowScaling)
		return (UINT16)(s->tcb->SND.WND >> s->tcb->Snd.Wind_Shift);
	else
		return (UINT16)(s->tcb->SND.WND);
}
