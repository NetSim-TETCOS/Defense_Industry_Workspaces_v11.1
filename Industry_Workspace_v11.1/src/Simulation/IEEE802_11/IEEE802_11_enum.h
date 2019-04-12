/************************************************************************************
 * Copyright (C) 2016
 * TETCOS, Bangalore. India															*

 * Tetcos owns the intellectual property rights in the Product and its content.     *
 * The copying, redistribution, reselling or publication of any or all of the       *
 * Product or its content without express prior written consent of Tetcos is        *
 * prohibited. Ownership and / or any other right relating to the software and all  *
 * intellectual property rights therein shall remain at all times with Tetcos.      *
 * Author:	Shashi Kant Suman														*
 * ---------------------------------------------------------------------------------*/

#include "EnumString.h"
#include "main.h"


BEGIN_ENUM(IEEE802_11_Subevent)
{
	DECL_ENUM_ELEMENT_WITH_VAL(CS, MAC_PROTOCOL_IEEE802_11 * 100),
		DECL_ENUM_ELEMENT(IEEE802_11_EVENT_DIFS_END),
		DECL_ENUM_ELEMENT(IEEE802_11_EVENT_DIFS_FAILED),
		DECL_ENUM_ELEMENT(IEEE802_11_EVENT_BACKOFF),
		DECL_ENUM_ELEMENT(IEEE802_11_EVENT_BACKOFF_PAUSED),
		DECL_ENUM_ELEMENT(SEND_ACK),
		DECL_ENUM_ELEMENT(SEND_CTS),
		DECL_ENUM_ELEMENT(SEND_MPDU),
		DECL_ENUM_ELEMENT(CTS_TIMEOUT),
		DECL_ENUM_ELEMENT(ACK_TIMEOUT),
		DECL_ENUM_ELEMENT(RECEIVE_ACK),
		DECL_ENUM_ELEMENT(RECEIVE_BLOCK_ACK),
		DECL_ENUM_ELEMENT(RECEIVE_RTS),
		DECL_ENUM_ELEMENT(RECEIVE_CTS),
		DECL_ENUM_ELEMENT(RECEIVE_MPDU),
		DECL_ENUM_ELEMENT(UPDATE_DEVICE_STATUS),
}
END_ENUM(IEEE802_11_Subevent);