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

BEGIN_ENUM(ZRP_Subevent)
{
	DECL_ENUM_ELEMENT_WITH_VAL(NDP_ScheduleHelloTransmission,NW_PROTOCOL_NDP*100),
	DECL_ENUM_ELEMENT(OLSR_ScheduleTCTransmission),
	DECL_ENUM_ELEMENT(OLSR_LINK_TUPLE_Expire),
}
#pragma warning(disable:4028)
END_ENUM(ZRP_Subevent);
#pragma warning(default:4028)
