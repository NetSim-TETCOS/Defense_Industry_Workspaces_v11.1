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
#ifndef _NETSIM_ALOHA_H_
#define _NETSIM_ALOHA_H_
#ifdef  __cplusplus
extern "C" {
#endif

///For MSVC compiler. For GCC link via Linker command 
#pragma comment(lib,"Metrics.lib")
#pragma comment(lib,"NetworkStack.lib")
#pragma comment(lib,"Mobility.lib")
#pragma comment(lib,"PropagationModel.lib")
#include "ErrorModel.h"

#define ALOHA_SLOT_LENGTH_DEFAULT				0 //Microsec
#define ALOHA_RETRY_LIMIT_DEFAULT				0
#define ALOHA_IS_MAC_BUFFER_DEFAULT				false
#define ALOHA_FREQUENCY_DEFAULT					900 //MHz
#define ALOHA_BANDWIDTH_DEFAULT					5 //MHz
#define ALOHA_TX_POWER_DEFAULT					100 //mW
#define ALOHA_RECEIVER_SENSITIVITY_DBM_DEFAULT	-85 //dBm
#define ALOHA_MODULATION_DEFAULT				_strdup("QPSK");
#define ALOHA_DATA_RATE_DEFAULT					10 //mbps

#define ALOHA_MAC_OVERHEAD 0 //Bytes
#define ALOHA_PHY_OVERHEAD 0 //Bytes

	typedef struct stru_aloha_mac_var
	{
		//Config parameter
		bool isSlotted;
		double slotlength;
		UINT retryLimit;
		bool isMACBuffer;

		double rateLimit;
		UINT backoffCounter;
		UINT retryCount;
		double waitTime;
		NetSim_PACKET* currPacket;
	}ALOHA_MAC_VAR,*PALOHA_MAC_VAR;
#define ALOHA_MAC(devid,ifid) ((PALOHA_MAC_VAR)DEVICE_MACVAR(devid,ifid))
#define ALOHA_CURR_MAC (ALOHA_MAC(pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId))

	typedef struct stru_aloha_phy_var
	{
		PHY_MODULATION modulation;
		double frequency;
		double bandwidth;
		double tx_power;
		double rx_sensitivity;
		double data_rate;
		double dAntennaHeight;
		double dAntennaGain;

		PHY_TX_STATUS transmitter_status;
		double dInterferencePower;
	}ALOHA_PHY_VAR,*PALOHA_PHY_VAR;
#define ALOHA_PHY(devid,ifid) ((PALOHA_PHY_VAR)DEVICE_PHYVAR(devid,ifid))
#define ALOHA_CURR_PHY (ALOHA_PHY(pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId))

#define get_slot_length(phy) ((1500*8)/(phy->data_rate))

	//Propagation Model
	double get_received_power(NETSIM_ID TX,NETSIM_ID RX, double time);
	int fn_NetSim_Aloha_CalulateReceivedPower();

	int fn_NetSim_Aloha_Handle_MacOut();

	int fn_NetSim_Aloha_PhyOut();
	void fn_NetSim_Aloha_PhyIn();

#ifdef  __cplusplus
}
#endif
#endif //_NETSIM_ALOHA_H_