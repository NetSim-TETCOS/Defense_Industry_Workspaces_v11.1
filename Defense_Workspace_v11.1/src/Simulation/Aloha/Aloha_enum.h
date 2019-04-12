/************************************************************************************
* Copyright (C) 2018
* TETCOS, Bangalore. India															*

* Tetcos owns the intellectual property rights in the Product and its content.     *
* The copying, redistribution, reselling or publication of any or all of the       *
* Product or its content without express prior written consent of Tetcos is        *
* prohibited. Ownership and / or any other right relating to the software and all  *
* intellectual property rights therein shall remain at all times with Tetcos.      *
* Author:	Shashi Kant Suman														*
* ---------------------------------------------------------------------------------*/
#ifndef _NETSIM_ALOHA_ENUM_H_
#define _NETSIM_ALOHA_ENUM_H_
#include "main.h"
#include "EnumString.h"

BEGIN_ENUM(ALOHA_Subevent)
{
	DECL_ENUM_ELEMENT_WITH_VAL(ALOHA_PACKET_ERROR,MAC_PROTOCOL_ALOHA*100),
		DECL_ENUM_ELEMENT(ALOHA_PACKET_SUCCESS),
}
END_ENUM(ALOHA_Subevent);
#endif

