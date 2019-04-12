#pragma once
/************************************************************************************
* Copyright (C) 2017                                                               *
* TETCOS, Bangalore. India                                                         *
*                                                                                  *
* Tetcos owns the intellectual property rights in the Product and its content.     *
* The copying, redistribution, reselling or publication of any or all of the       *
* Product or its content without express prior written consent of Tetcos is        *
* prohibited. Ownership and / or any other right relating to the software and all  *
* intellectual property rights therein shall remain at all times with Tetcos.      *
*                                                                                  *
* Author:    Shashi Kant Suman                                                     *
*                                                                                  *
* ---------------------------------------------------------------------------------*/

#ifndef _NETSIM_OSPF_NEIGHBOR_H_
#define _NETSIM_OSPF_NEIGHBOR_H_
#ifdef  __cplusplus
extern "C" {
#endif

	enum enum_ospf_neighbor_state
	{
		OSPFNEIGHSTATE_DOWN,
		OSPFNEIGHSTATE_Init,
		OSPFNEIGHSTATE_Attempt,
		OSPFNEIGHSTATE_ExStart,
		OSPFNEIGHSTATE_2Way,
		OSPFNEIGHSTATE_Exchange,
		OSPFNEIGHSTATE_Full,
		OSPFNEIGHSTATE_Loading,
	};
	static char strNeighborState[][50] = { "DOWN",
	"Init",
	"Attempt",
	"ExStart",
	"2-Way",
	"Exchange",
	"Full",
	"Loading" };

	struct stru_ospf_neighbor
	{
		NETSIM_ID devId;
		NETSIM_ID devInterface;

		OSPFNEIGHSTATE state;
		
		bool isInactivityTimerAdded;
		UINT64 inactivityTimerId;

		bool isMaster;
		UINT DDSeqNo;
		ptrOSPFPACKETHDR lastrecvDDPacket;
		ptrOSPFPACKETHDR lastSentDDPacket;
		OSPFID neighborId;
		UINT8 neighborPriority;
		NETSIM_IPAddress neighborIPAddr;
		UINT8 neighborOption;
		UINT8 IMMS;
		OSPFID neighborDesignateRouter;
		OSPFID neighborDesignateBackupRouter;
		double lastDDMsgSentTime;

		ptrOSPFLIST neighLSReqList; // Link state request list
		ptrOSPFLIST neighDBSummaryList; // Database summary list
		ptrOSPFLIST neighLSRxtList; // Link state retransmission list
		ptrOSPFLIST linkStateSendList;
		bool LSRxtTimer;
		UINT LSRxtSeqNum;
		UINT LSReqTimerSequenceNumber;
		double lastHelloRecvTime;
	};

	typedef struct stru_lsrxtTimerDetails
	{
		NETSIM_IPAddress neighborIP;
		UINT rxmtSeqNum;
		OSPFID advertisingRouter;
		OSPFMSG msgType;
	}LSRXTTIMERDETAILS, *ptrLSRXTTIMERDETAILS;

#ifdef  __cplusplus
}
#endif
#endif //_NETSIM_OSPF_NEIGHBOR_H_
