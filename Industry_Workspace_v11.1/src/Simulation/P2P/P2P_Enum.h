/************************************************************************************
* Copyright (C) 2017
* TETCOS, Bangalore. India															*

* Tetcos owns the intellectual property rights in the Product and its content.     *
* The copying, redistribution, reselling or publication of any or all of the       *
* Product or its content without express prior written consent of Tetcos is        *
* prohibited. Ownership and / or any other right relating to the software and all  *
* intellectual property rights therein shall remain at all times with Tetcos.      *
* Author:	Shashi Kant Suman														*
* ---------------------------------------------------------------------------------*/
#include "EnumString.h"

BEGIN_ENUM(P2P_Subevent)
{
	//Events that changes interface state
	DECL_ENUM_ELEMENT_WITH_VAL(P2P_LINK_UP, MAC_PROTOCOL_P2P * 100),
		DECL_ENUM_ELEMENT(P2P_LINK_DOWN),
}
#pragma warning(disable:4028)
END_ENUM(P2P_Subevent);
#pragma warning(default:4028)

