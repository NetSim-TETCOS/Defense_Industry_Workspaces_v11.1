#pragma once
/************************************************************************************
* Copyright (C) 2015
* TETCOS, Bangalore. India															*

* Tetcos owns the intellectual property rights in the Product and its content.     *
* The copying, redistribution, reselling or publication of any or all of the       *
* Product or its content without express prior written consent of Tetcos is        *
* prohibited. Ownership and / or any other right relating to the software and all  *
* intellectual property rights therein shall remain at all times with Tetcos.      *
* Author:	Shashi Kant Suman														*
* ---------------------------------------------------------------------------------*/
#ifndef _NETSIM_P2P_H_
#define _NETSIM_P2P_H_
//For MSVC compiler. For GCC link via Linker command 
#pragma comment(lib,"Metrics.lib")
#pragma comment(lib,"NetworkStack.lib")
#pragma comment(lib,"Mobility.lib")
#pragma comment(lib,"PropagationModel.lib")

#include "List.h"
#include "ErrorModel.h"

#ifdef  __cplusplus
extern "C" {
#endif

	/***** Default Config Parameter ****/
#define P2P_CONNECTION_MEDIUM_DEFAULT			_strdup("wired")
#define P2P_BANDWIDTH_DEFAULT					20		//MHz
#define P2P_CENTRAL_FREQUENCY_DEFAULT			30		//MHz
#define P2P_TX_POWER_DEFAULT					20		//W
#define P2P_DATA_RATE_DEFAULT					10		//mbps
#define P2P_RECEIVER_SENSITIVITY_DBM_DEFAULT	-101	//dBm
#define P2P_MODULATION_TECHNIQUE_DEFAULT		_strdup("QPSK")
#define P2P_ANTENNA_HEIGHT_DEFAULT				1
#define P2P_ANTENNA_GAIN_DEFAULT				1
#define P2P_D0_DEFAULT							1
#define P2P_PL_D0_DEFAULT						-30

	typedef struct stru_P2P_NODE_PHY
	{
		bool iswireless;
		double dBandwidth;
		double dCenteralFrequency;
		double dTXPower;
		double dReceiverSensitivity;
		PHY_MODULATION modulation;
		double dDataRate;
		double dAntennaHeight;
		double dAntennaGain;
		double d0;
		double pld0;
	}P2P_NODE_PHY, *ptrP2P_NODE_PHY;
#define P2P_PHY(devid,ifid) ((ptrP2P_NODE_PHY)DEVICE_PHYVAR(devid,ifid))

#define isP2PConfigured(d,i) (DEVICE_MACLAYER(d,i)->nMacProtocolId == MAC_PROTOCOL_P2P)
#define isP2PWireless(d,i) (isP2PConfigured(d,i)?(P2P_PHY(d,i)?P2P_PHY(d,i)->iswireless:false):false)

	PROPAGATION_HANDLE propagationHandle;

#ifdef  __cplusplus
}
#endif
#endif /* _NETSIM_P2P_H_ */