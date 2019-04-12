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

BEGIN_ENUM(OSPF_Subevent)
{
	//Events that changes interface state
	DECL_ENUM_ELEMENT_WITH_VAL(OSPF_InterfaceUp, APP_PROTOCOL_OSPF * 100),
		DECL_ENUM_ELEMENT(OSPF_WAITTIMER),
		DECL_ENUM_ELEMENT(OSPF_BACKUPSEEN),
		DECL_ENUM_ELEMENT(OSPF_NEIGHBORCHANGE),
		DECL_ENUM_ELEMENT(OSPF_LOOPIND),
		DECL_ENUM_ELEMENT(OSPF_UNLOOPIND),
		DECL_ENUM_ELEMENT(OSPF_InterfaceDown),

		//Events that changes neighbor state
		DECL_ENUM_ELEMENT(OSPF_HelloReceived),
		DECL_ENUM_ELEMENT(OSPF_Start),
		DECL_ENUM_ELEMENT(OSPF_1Way),
		DECL_ENUM_ELEMENT(OSPF_2WayReceived),
		DECL_ENUM_ELEMENT(OSPF_NegotiationDone),
		DECL_ENUM_ELEMENT(OSPF_ExchangeDone),
		DECL_ENUM_ELEMENT(OSPF_BadLSReq),
		DECL_ENUM_ELEMENT(OSPF_LoadingDone),
		DECL_ENUM_ELEMENT(OSPF_Adjok),
		DECL_ENUM_ELEMENT(OSPF_SeqNumberMismatch),
		DECL_ENUM_ELEMENT(OSPF_KillNbr),
		DECL_ENUM_ELEMENT(OSPF_InactivityTimer),
		DECL_ENUM_ELEMENT(OSPF_LLDown),

		DECL_ENUM_ELEMENT(OSPF_HELLO_TIMER),
		DECL_ENUM_ELEMENT(OSPF_DD_RXMT_IN_EXSTART),
		DECL_ENUM_ELEMENT(OSPF_SCHEDULELSDB),
		DECL_ENUM_ELEMENT(OSPF_FLOODTIMER),
		DECL_ENUM_ELEMENT(OSPF_MAXAGEREMOVALTIMER),
		DECL_ENUM_ELEMENT(OSPF_CALCULATESPF),
		DECL_ENUM_ELEMENT(OSPF_RXMTTIMER),
		DECL_ENUM_ELEMENT(OSPF_SENDDELAYEDACK),

		DECL_ENUM_ELEMENT(OSPF_INCREMENTAGE),
}
#pragma warning(disable:4028)
END_ENUM(OSPF_Subevent);
#pragma warning(default:4028)

#ifndef _OSPF_EVENT_FUN_
#define _OSPF_EVENT_FUN_

typedef void(*fnEventCallback)();

static fnEventCallback fnpEventCallback[] = {ospf_handle_interfaceUp_event,
ospf_interface_handleMultipleInterfaceEvent,
ospf_interface_handleMultipleInterfaceEvent,
ospf_interface_handleMultipleInterfaceEvent,
NULL,
NULL,
ospf_handle_interfaceDown_event,
ospf_neighbor_handle_helloReceived_event,
ospf_neighbor_handle_start_event,
ospf_neighbor_handle_1way_event,
ospf_neighbor_handle_2wayReceived_event,
ospf_neighbor_handle_negotiationDone_event,
ospf_neighbor_handle_exchangeDone_event,
NULL,
ospf_neighbor_handle_LoadingDoneEvent,
NULL,
NULL,
ospf_neighbor_handle_KillNbrEvent,
ospf_neighbor_handle_inactivityTimer_event,
NULL,
ospf_handle_helloTimer_event,
start_sending_dd_msg,
ospf_lsa_ScheduleLSDB,
ospf_lsa_handle_floodTimer_event,
ospf_lsdb_handleMaxAgeRemovalTimer,
ospf_spf_handleCalculateSPFEvent,
ospf_lsupdate_handleRxmtTimer,
ospf_lsack_handleDelayedAckTimer,
ospf_LSDB_handle_IncrementAge_event};

#define OSPF_CALL_SUBEVENT(subevent) fnpEventCallback[(subevent)%100]()
#define OSPF_IS_SUBEVENT(subevent) (fnpEventCallback[(subevent)%100] != NULL)

#endif //_OSPF_EVENT_FUN_
