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
#ifndef _NETSIM_RRC_H_
#define _NETSIM_RRC_H_
#include "List.h"
#ifdef  __cplusplus
extern "C" {
#endif

	//RRC message size
#define RRC_CONNECTION_REQUEST_SIZE			80/8.0		//RLC-0,	RRC-80,		NAS-0
#define RRC_CONNECTION_SETUP_SIZE			1112/8.0	//RLC-80,	RRC-1032,	NAS-0
#define RRC_CONNECTION_SETUP_COMPLETE_SIZE	208/8.0		//RLC-56,	RRC-152,	NAS-0
#define RRC_CONNECTION_REJECT_SIZE			1

	typedef enum enum_Establishmentcause
	{
		emergency,
		highPriorityAccess,
		mt_Access,
		mo_Signalling,
		mo_Data,
		delayTolerantAccess_v1020,
		spare2,
		spare1,
	}Establishment_Cause;

	struct stru_LTE_RRC_Connection_Request
	{
		LTE_CONTROL_PACKET MessageType;
		NETSIM_ID InitialUEIdentity;
		unsigned int SelectedPLMNIdentity;
		Establishment_Cause EstablishmentCause;
	};

	struct stru_LTE_RRC_Connection_Setup
	{
		LTE_CONTROL_PACKET MessageType;
		unsigned int TransactionIdentifier;
		NETSIM_ID InitialUEIdentity;
		void* RadioResourceConfiguration;
	};

	struct stru_LTE_RRC_Connection_Setup_Complete
	{
		LTE_CONTROL_PACKET MessageType;
		unsigned int TransactionIdetifier;
		void* IE;
	};

#ifdef  __cplusplus
}
#endif
#endif /* _NETSIM_RRC_H_ */