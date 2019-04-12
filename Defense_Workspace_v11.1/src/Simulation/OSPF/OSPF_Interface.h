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

#ifndef _NETSIM_OSPF_INTERFACE_H_
#define _NETSIM_OSPF_INTERFACE_H_
#ifdef  __cplusplus
extern "C" {
#endif

	typedef enum enum_ospf_if_type
	{
		OSPFIFTYPE_P2P,
		OSPFIFTYPE_BROADCAST,
		OSPFIFTYPE_NBMA,
		OSPFIFTYPE_P2MP,
		OSPFIFTYPE_VIRTUALLINK,
	}OSPFIFTYPE;
	static char strOSPFIFTYPE[][50] = { "POINT_TO_POINT","BROADCAST",
	"NBMA","POINT_TO_MULTIPOINT","VITUALLINK" };

	enum enum_if_state
	{
		OSPFIFSTATE_DOWN,
		OSPFIFSTATE_LOOPBACK,
		OSPFIFSTATE_WAITING,
		OSPFIFSTATE_P2P,
		OSPFIFSTATE_DROther,
		OSPFIFSTATE_BACKUP,
		OSPFIFSTATE_DR,
	};
	static char strOSPFIFSTATE[][50] = { "DOWN",
		"LoopBack",
		"Waiting",
		"P2P",
		"DR-Other",
		"BackUp",
		"DR" };

	struct stru_ospf_if
	{
		char* name;
		NETSIM_ID configId;
		NETSIM_ID id;
		bool includeSubnetRts;

		OSPFIFTYPE Type;
		OSPFIFSTATE State;
		NETSIM_IPAddress IPIfAddr;
		NETSIM_IPAddress IPIfMask;
		OSPFID areaId;
		OSPFTIME helloInterval;
		OSPFTIME routerDeadInterval;
		OSPFTIME InfTransDelay;
		UINT8 RouterPriority;
		OSPFTIME waitTimer;
		UINT neighborRouterCount;
		ptrOSPF_NEIGHBOR* neighborRouterList;

		OSPFID designaterRouter;
		NETSIM_IPAddress designaterRouterAddr;
		
		OSPFID backupDesignaterRouter;
		NETSIM_IPAddress backupDesignaterRouterAddr;
		
		UINT16 interfaceOutputCost;
		OSPFTIME RxmtInterval;
		UINT auType;
		UINT auKey;
		bool extRoutingCapability;
		double lastFloodTimer;
		bool isFloodTimerSet;
		ptrOSPFLIST updateLSAList;
		bool networkLSTimer;
		UINT networkLSTimerSeqNumber;
		double networkLSAOriginateTime;

		//Delayed Ack
		bool delayedAckTimer;
		ptrOSPFLIST delayedAckList;
	};

	void ospf_interface_init(ptrOSPF_PDS ospf, ptrOSPF_IF thisInterface);
	OSPFIFTYPE OSPFIFTYPE_FROM_STR(char* val);

#ifdef  __cplusplus
}
#endif
#endif //_NETSIM_OSPF_NEIGHBOR_H_