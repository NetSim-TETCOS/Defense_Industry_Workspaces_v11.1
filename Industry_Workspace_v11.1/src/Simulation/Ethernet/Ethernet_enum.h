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

BEGIN_ENUM(Ethernet_Subevent)
{
	DECL_ENUM_ELEMENT_WITH_VAL(SEND_CONFIG_BPDU, MAC_PROTOCOL_IEEE802_3 * 100),
	DECL_ENUM_ELEMENT(ETHERNET_TIMEOUT2),
	DECL_ENUM_ELEMENT(ETH_IF_UP),
}
#pragma warning(disable:4028)
END_ENUM(Ethernet_Subevent);
#pragma warning(default:4028)

