/************************************************************************************
 * Copyright (C) 2015
 * TETCOS, Bangalore. India															*

 * Tetcos owns the intellectual property rights in the Product and its content.     *
 * The copying, redistribution, reselling or publication of any or all of the       *
 * Product or its content without express prior written consent of Tetcos is        *
 * prohibited. Ownership and / or any other right relating to the software and all  *
 * intellectual property rights therein shall remain at all times with Tetcos.      *
 * Author:	Shashi Kant Suman														*
 * ---------------------------------------------------------------------------------*/
#ifndef _NETSIM_DTDMA_ENUM_H_
#define _NETSIM_DTDMA_ENUM_H_
#include "main.h"
#include "EnumString.h"

BEGIN_ENUM(DTDMA_Subevent)
{
	DECL_ENUM_ELEMENT_WITH_VAL(DTDMA_SCHEDULE_Transmission,MAC_PROTOCOL_DTDMA*100),
	DECL_ENUM_ELEMENT(DTDMA_FORM_SLOT),
}
END_ENUM(DTDMA_Subevent);
#endif