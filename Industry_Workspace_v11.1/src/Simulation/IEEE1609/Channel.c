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
#include "main.h"
#include "IEEE1609.h"

void Init_Channel_Switching(NETSIM_ID ndeviceId, NETSIM_ID nInterfaceId)
{
	memset(pstruEventDetails, 0, sizeof* pstruEventDetails);
	pstruEventDetails->nDeviceId = ndeviceId;
	pstruEventDetails->nDeviceType = DEVICE_TYPE(ndeviceId);
	pstruEventDetails->nEventType = TIMER_EVENT;
	pstruEventDetails->nInterfaceId = nInterfaceId;
	pstruEventDetails->nProtocolId = MAC_PROTOCOL_IEEE1609;
	pstruEventDetails->nSubEventType = CHANNEL_SWITCH_END;
	fnpAddEvent(pstruEventDetails);
	GET_IEEE1609_CURR_MAC_VAR->CHANNEL_INFO.prevChannel = IEEE1609_ChannelType_SCH;
}

void fn_NetSim_IEEE1609_ChannelSwitch_End()
{
	double t;
	NetSim_EVENTDETAILS pevent;
	memcpy(&pevent, pstruEventDetails, sizeof pevent);

	switch (GET_IEEE1609_CURR_MAC_VAR->CHANNEL_INFO.prevChannel)
	{
	case IEEE1609_ChannelType_SCH:
		t = GET_IEEE1609_CURR_MAC_VAR->dCCH_Time;
		GET_IEEE1609_CURR_MAC_VAR->CHANNEL_INFO.channel_type = IEEE1609_ChannelType_CCH;
		break;
	case IEEE1609_ChannelType_CCH:
		t = GET_IEEE1609_CURR_MAC_VAR->dSCH_Time;
		GET_IEEE1609_CURR_MAC_VAR->CHANNEL_INFO.channel_type = IEEE1609_ChannelType_SCH;
		break;
	default:
		fnNetSimError("Unknown channel type %d for IEEE1609 protocol\n",
			GET_IEEE1609_CURR_MAC_VAR->CHANNEL_INFO.prevChannel);
		break;
	}
	GET_IEEE1609_CURR_MAC_VAR->CHANNEL_INFO.dStartTime = pstruEventDetails->dEventTime;
	pstruEventDetails->dEventTime += t;
	pstruEventDetails->nSubEventType = CHANNEL_SWITCH_START;
	fnpAddEvent(pstruEventDetails);
	GET_IEEE1609_CURR_MAC_VAR->CHANNEL_INFO.dEndTime = pstruEventDetails->dEventTime;
	
	//restore
	memcpy(pstruEventDetails, &pevent, sizeof* pstruEventDetails);
	restart_transmission();
}

void fn_NetSim_IEEE1609_ChannelSwitch_Start()
{
	pstruEventDetails->dEventTime += GET_IEEE1609_CURR_MAC_VAR->dGuard_Time;
	pstruEventDetails->nSubEventType = CHANNEL_SWITCH_END;
	fnpAddEvent(pstruEventDetails);
	GET_IEEE1609_CURR_MAC_VAR->CHANNEL_INFO.prevChannel =
		GET_IEEE1609_CURR_MAC_VAR->CHANNEL_INFO.channel_type;
	GET_IEEE1609_CURR_MAC_VAR->CHANNEL_INFO.channel_type = IEEE1609_ChannelType_GUARD;
}
