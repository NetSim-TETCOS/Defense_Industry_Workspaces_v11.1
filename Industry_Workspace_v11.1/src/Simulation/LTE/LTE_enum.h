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

#include "EnumString.h"

BEGIN_ENUM(LTE_Subevent)
{
	DECL_ENUM_ELEMENT_WITH_VAL(LTE_TXNEXTFRAME,MAC_PROTOCOL_LTE*100),
	DECL_ENUM_ELEMENT(LTE_T300_Expire),//RRC connection retransmit request timer
	DECL_ENUM_ELEMENT(UPDATE_D2D_PEERS),
}
END_ENUM(LTE_Subevent);
