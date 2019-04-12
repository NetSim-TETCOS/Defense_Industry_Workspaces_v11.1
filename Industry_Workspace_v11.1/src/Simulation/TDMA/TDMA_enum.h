/************************************************************************************
 * Copyright (C) 2014
 * TETCOS, Bangalore. India															*

 * Tetcos owns the intellectual property rights in the Product and its content.     *
 * The copying, redistribution, reselling or publication of any or all of the       *
 * Product or its content without express prior written consent of Tetcos is        *
 * prohibited. Ownership and / or any other right relating to the software and all  *
 * intellectual property rights therein shall remain at all times with Tetcos.      *
 * Author:	Shashi Kant Suman														*
 * ---------------------------------------------------------------------------------*/
#ifndef _NETSIM_TDMA_ENUM_H_
#define _NETSIM_TDMA_ENUM_H_
#include "main.h"
#include "EnumString.h"

BEGIN_ENUM(TDMA_Subevent)
{
	DECL_ENUM_ELEMENT_WITH_VAL(TDMA_SCHEDULE_Transmission,MAC_PROTOCOL_TDMA*100),
}
END_ENUM(TDMA_Subevent);
#endif