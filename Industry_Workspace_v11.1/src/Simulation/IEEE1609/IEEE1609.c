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
#include "Protocol.h"
#include "IEEE1609.h"

//Function Prototype
void fn_NetSim_IEEE1609_TimerEvent();
int fn_NetSim_IEEE1609_Configure_F(void**);
int fn_NetSim_IEEE1609_Init_F(struct stru_NetSim_Network *NETWORK_Formal,
							NetSim_EVENTDETAILS *pstruEventDetails_Formal,
							char *pszAppPath_Formal,
							char *pszWritePath_Formal,
							int nVersion_Type);
int fn_NetSim_IEEE1609_Finish_F();

_declspec(dllexport) int fn_NetSim_IEEE1609_Configure(void** var)
{
	return fn_NetSim_IEEE1609_Configure_F(var);
}

_declspec(dllexport) int fn_NetSim_IEEE1609_Init(struct stru_NetSim_Network *NETWORK_Formal,
	NetSim_EVENTDETAILS *pstruEventDetails_Formal,
	char *pszAppPath_Formal,
	char *pszWritePath_Formal,
	int nVersion_Type)
{
	return fn_NetSim_IEEE1609_Init_F(NETWORK_Formal,
		pstruEventDetails_Formal, pszAppPath_Formal,
		pszWritePath_Formal,
		nVersion_Type);
}

_declspec(dllexport) int fn_NetSim_IEEE1609_Run()
{
	switch (pstruEventDetails->nEventType)
	{
#pragma region WSMP
	case TRANSPORT_OUT_EVENT:
	case NETWORK_OUT_EVENT:
		fn_NetSim_WSNP_SendWSMPacket();
		break;
	case TRANSPORT_IN_EVENT:
	case NETWORK_IN_EVENT:
		fn_NetSim_WSNP_ReceiveWSMPacket();
		break;
#pragma endregion WSMP

	case MAC_OUT_EVENT:
		pstruEventDetails->nProtocolId = GET_IEEE1609_CURR_MAC_VAR->secondary_protocol;
		fnCallProtocol(GET_IEEE1609_CURR_MAC_VAR->secondary_protocol);
		break;
	case PHYSICAL_OUT_EVENT:
		pstruEventDetails->nProtocolId = GET_IEEE1609_CURR_MAC_VAR->secondary_protocol;
		fnCallProtocol(GET_IEEE1609_CURR_MAC_VAR->secondary_protocol);
		break;
	case PHYSICAL_IN_EVENT:
		pstruEventDetails->nProtocolId = GET_IEEE1609_CURR_MAC_VAR->secondary_protocol;
		fnCallProtocol(GET_IEEE1609_CURR_MAC_VAR->secondary_protocol);
		break;
	case MAC_IN_EVENT:
		pstruEventDetails->nProtocolId = GET_IEEE1609_CURR_MAC_VAR->secondary_protocol;
		fnCallProtocol(GET_IEEE1609_CURR_MAC_VAR->secondary_protocol);
		break;
	case TIMER_EVENT:
		fn_NetSim_IEEE1609_TimerEvent();
		break;
	}
	return 0;
}

/**
This function is called by NetworkStack.dll, while writing the event trace
to get the sub event as a string.
*/
_declspec(dllexport) char* fn_NetSim_IEEE1609_Trace(int nSubEvent)
{
	return GetStringIEEE1609_Subevent(nSubEvent);
}

/**
This function is called by NetworkStack.dll, to free the WLAN protocol
pstruMacData->Packet_MACProtocol.
*/
_declspec(dllexport) int fn_NetSim_IEEE1609_FreePacket(NetSim_PACKET* pstruPacket)
{
	return 0;
}

/**
This function is called by NetworkStack.dll, to copy the WLAN protocol
pstruMacData->Packet_MACProtocol from source packet to destination.
*/
_declspec(dllexport) int fn_NetSim_IEEE1609_CopyPacket(NetSim_PACKET* pstruDestPacket, NetSim_PACKET* pstruSrcPacket)
{
	return 0;
}

/**
This function call WLAN Metrics function in lib file to write the Metrics in Metrics.txt
*/
_declspec(dllexport) int fn_NetSim_IEEE1609_Metrics(PMETRICSWRITER metricsWriter)
{
	return 0;
}

/**
This function is to configure the WLAN protocol packet trace parameter.
This function return a string which has the parameters separated by comma.
*/
_declspec(dllexport) char* fn_NetSim_IEEE1609_ConfigPacketTrace(const void* xmlNetSimNode)
{
	return "";
}

/**
This function is called while writing the Packet trace for WLAN protocol.
This function is called for every packet while writing the packet trace.
*/
_declspec(dllexport) int fn_NetSim_IEEE1609_WritePacketTrace(NetSim_PACKET* pstruPacket, char** ppszTrace)
{
	return 1;
}

/**
This function is called by NetworkStack.dll, once simulation end to free the
allocated memory for the network.
*/
_declspec(dllexport) int fn_NetSim_IEEE1609_Finish()
{
	return fn_NetSim_IEEE1609_Finish_F();
}

void fn_NetSim_IEEE1609_TimerEvent()
{
	switch (pstruEventDetails->nSubEventType)
	{
	case CHANNEL_SWITCH_END:
		fn_NetSim_IEEE1609_ChannelSwitch_End();
		break;
	case CHANNEL_SWITCH_START:
		fn_NetSim_IEEE1609_ChannelSwitch_Start();
		break;
	default:
		fnNetSimError("Unknown timer event %d for IEEE1609 protocol\n", pstruEventDetails->nSubEventType);
		break;
	}
}

void restart_transmission()
{
	pstruEventDetails->nEventType = MAC_OUT_EVENT;
	pstruEventDetails->nSubEventType = 0;
	fnpAddEvent(pstruEventDetails);
}
