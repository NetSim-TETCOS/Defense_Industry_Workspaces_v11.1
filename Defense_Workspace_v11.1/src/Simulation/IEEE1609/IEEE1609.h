#pragma once
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
#ifndef _NETSIM_IEEE1609_H_
#define _NETSIM_IEEE1609_H_
#ifdef  __cplusplus
extern "C" {
#endif
#ifndef _NO_DEFAULT_LINKER_
	//For MSVC compiler. For GCC link via Linker command 
#pragma comment(lib,"IEEE1609.lib")
#pragma comment(lib,"Metrics.lib")
#pragma comment(lib,"NetworkStack.lib")
#pragma comment(lib,"Mobility.lib")
#endif

#include "IEEE1609_Enum.h"
#include "IEEE1609_Interface.h"

	//Default config parameter
#define IEEE1609_SECONDARY_PROTOCOL_DEFAULT		_strdup("IEEE802.11")
#define IEEE1609_STANDARD_CHANNEL_SCH_DEFAULT	_strdup("36_5180")
#define IEEE1609_STANDARD_CHANNEL_CCH_DEFAULT	_strdup("38_5190")
#define IEEE1609_SCH_TIME_DEFAULT				10000 //Microsec
#define IEEE1609_CCH_TIME_DEFAULT				10000 //Microsec
#define IEEE1609_GUARD_TIME_DEFAULT				10 //Microsec

	//Function Prototype
	bool isIEEE1609_Configure(NETSIM_ID nDeviceId, NETSIM_ID nInterfaceId);

	//WSMP
	void fn_NetSim_WSNP_SendWSMPacket();
	int fn_NetSim_WSNP_ReceiveWSMPacket();

	//Channel
	void fn_NetSim_IEEE1609_ChannelSwitch_Start();
	void fn_NetSim_IEEE1609_ChannelSwitch_End();
	void Init_Channel_Switching(NETSIM_ID ndeviceId, NETSIM_ID nInterfaceId);
	void restart_transmission();

#ifdef  __cplusplus
}
#endif
#endif //_NETSIM_IEEE1609_H_