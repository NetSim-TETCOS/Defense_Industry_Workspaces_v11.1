/************************************************************************************
* Copyright (C) 2016                                                               *
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
#ifndef _NETSIM_CSMACD_H_
#define _NETSIM_CSMACD_H_
#ifdef  __cplusplus
extern "C" {
#endif

	///For MSVC compiler. For GCC link via Linker command 
#pragma comment(lib,"CSMACDlib.lib")
#pragma comment(lib,"Metrics.lib")
#pragma comment(lib,"NetworkStack.lib")

#include "CSMACD_enum.h"

#define CSMACD_MAC_OVERHEAD 26 //Bytes
#define CSMACD_PHY_OVERHEAD 0

	FILE *file_collision,*file_contention;

	typedef enum
	{
		LinkState_DOWN,
		LinkState_UP,
	}LINK_STATE;

	typedef enum
	{
		Idle,
		Busy,
	}MAC_STATE;

	typedef struct stru_CSMACD_MacVar
	{
		double dPersistance;
		double dSlotTime;
		unsigned int nRetryLimit;

		MAC_STATE macState;
		unsigned int nRetryCount;
		NetSim_PACKET* currentPacket;
		double waittime;
		bool iswaited;
	}CSMACD_MACVAR,*PCSMACD_MACVAR;
#define CSMACD_MAC(devid,ifid) ((PCSMACD_MACVAR)DEVICE_MACVAR(devid,ifid))
#define CSMACD_CURRMAC (CSMACD_MAC(pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId))

	typedef struct struc_CSMACD_PhyVar
	{
		LINK_STATE link_state;
		int collision_count;
	}CSMACD_PHYVAR,*PCSMACD_PHYVAR;
#define CSMACD_PHY(devid,ifid) (PCSMACD_PHYVAR)DEVICE_PHYVAR(devid,ifid)
#define CSMACD_CURRPHY CSMACD_PHY(pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId)


	bool isMediumIdle();
	void fn_NetSim_CSMACD_MacOut();
	void fn_NetSim_CSMACD_MacIn();
	void fn_NetSim_CSMACD_PhyOut();
	void fn_NetSim_CSMACD_PhyIn();
	void fn_NetSim_CSMACS_PersistanceWait();

#ifdef  __cplusplus
}
#endif
#endif //_NETSIM_CSMACD_H_
