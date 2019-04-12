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
#include "IEEE802_11.h"
#include "IEEE802_11_Phy.h"

//Function prototype
int fn_NetSim_IEEE802_11_Configure_F(void** var);
int fn_NetSim_IEEE802_11_Init_F(struct stru_NetSim_Network *NETWORK_Formal,
	NetSim_EVENTDETAILS *pstruEventDetails_Formal,
	char *pszAppPath_Formal,
	char *pszWritePath_Formal,
	int nVersion_Type);
int fn_NetSim_IEEE802_11_Metrics_F(PMETRICSWRITER metricsWriter);
void print_minstrel_table(PMETRICSWRITER metricsWriter);

_declspec(dllexport) int fn_NetSim_IEEE802_11_Configure(void** var)
{
	return fn_NetSim_IEEE802_11_Configure_F(var);
}

_declspec(dllexport) int fn_NetSim_IEEE802_11_Init(struct stru_NetSim_Network *NETWORK_Formal,
	NetSim_EVENTDETAILS *pstruEventDetails_Formal,
	char *pszAppPath_Formal,
	char *pszWritePath_Formal,
	int nVersion_Type)
{
	return fn_NetSim_IEEE802_11_Init_F(NETWORK_Formal,
		pstruEventDetails_Formal,pszAppPath_Formal,
		pszWritePath_Formal,
		nVersion_Type);
}

_declspec(dllexport) int fn_NetSim_IEEE802_11_Run()
{
	switch(pstruEventDetails->nEventType)
	{
	case MAC_OUT_EVENT:
		fn_NetSim_IEEE802_11_MacOut();
		break;
	case MAC_IN_EVENT:
		fn_NetSim_IEEE802_11_MacIn();
		break;
	case PHYSICAL_OUT_EVENT:
		fn_NetSim_IEEE802_11_PhyOut();
		break;
	case PHYSICAL_IN_EVENT:
		fn_NetSim_IEEE802_11_PhyIn();
		break;
	case TIMER_EVENT:
		fn_NetSim_IEEE802_11_Timer();
		break;
	default:
		fnNetSimError("Unknown event type %d for IEEE802.11 protocol",pstruEventDetails->nEventType);
		break;
	}
	return 0;
}

/**
This function is called by NetworkStack.dll, while writing the event trace 
to get the sub event as a string.
*/
_declspec(dllexport) char* fn_NetSim_IEEE802_11_Trace(int nSubEvent)
{
	return GetStringIEEE802_11_Subevent(nSubEvent);
}

/**
This function is called by NetworkStack.dll, to free the WLAN protocol
pstruMacData->Packet_MACProtocol.
*/
_declspec(dllexport) int fn_NetSim_IEEE802_11_FreePacket(NetSim_PACKET* pstruPacket)
{
	free_ieee802_11_mac_header(pstruPacket);
	free_ieee802_11_phy_header(pstruPacket);
	return 0;
}

/**
This function is called by NetworkStack.dll, to copy the WLAN protocol
pstruMacData->Packet_MACProtocol from source packet to destination.
*/
_declspec(dllexport) int fn_NetSim_IEEE802_11_CopyPacket(NetSim_PACKET* pstruDestPacket,NetSim_PACKET* pstruSrcPacket)
{
	copy_ieee802_11_mac_header(pstruDestPacket,pstruSrcPacket);
	copy_ieee802_11_phy_header(pstruDestPacket, pstruSrcPacket);
	return 0;
}

/**
This function call WLAN Metrics function in lib file to write the Metrics in Metrics.txt	
*/
_declspec(dllexport) int fn_NetSim_IEEE802_11_Metrics(PMETRICSWRITER metricsWriter)
{
	fn_NetSim_IEEE802_11_Metrics_F(metricsWriter);	                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  
	print_minstrel_table(metricsWriter);
	return 0;
}

/**
This function is to configure the WLAN protocol packet trace parameter. 
This function return a string which has the parameters separated by comma.
*/
_declspec(dllexport) char* fn_NetSim_IEEE802_11_ConfigPacketTrace(const void* xmlNetSimNode)
{
	return "";
}

/**
This function is called while writing the Packet trace for WLAN protocol. 
This function is called for every packet while writing the packet trace.
*/
_declspec(dllexport) int fn_NetSim_IEEE802_11_WritePacketTrace(NetSim_PACKET* pstruPacket, char** ppszTrace)
{
	return 1;
}

/**
This function is called by NetworkStack.dll, once simulation end to free the 
allocated memory for the network.	
*/
_declspec(dllexport) int fn_NetSim_IEEE802_11_Finish()
{
	return fn_NetSim_IEEE802_11_Finish_F();
}

void fn_NetSim_IEEE802_11_Timer()
{
	switch(pstruEventDetails->nSubEventType)
	{
	case ACK_TIMEOUT:
		fn_NetSim_IEEE802_11_CSMA_AckTimeOut();
		break;
	case CTS_TIMEOUT:
		fn_NetSim_IEEE802_11_RTS_CTS_CTSTimeOut();
		break;
	default:
		fnNetSimError("Unknown Timer event-%d subevent for IEEE802.11 protocol in %s\n",
			pstruEventDetails->nSubEventType,
			__FUNCTION__);
	}
}

bool isIEEE802_11_log()
{
	return false;
}

static void add_event_due_to_medium_change(NETSIM_ID d,
										   NETSIM_ID in,
										   EVENT_TYPE eventType,
										   int subevent)
{
	NetSim_EVENTDETAILS pevent;
	memset(&pevent, 0, sizeof pevent);
	pevent.dEventTime = pstruEventDetails->dEventTime;
	pevent.dPacketSize = 0;
	pevent.nDeviceId = d;
	pevent.nDeviceType = DEVICE_TYPE(d);
	pevent.nEventType = eventType;
	pevent.nInterfaceId = in;
	pevent.nProtocolId = MAC_PROTOCOL_IEEE802_11;
	pevent.nSubEventType = subevent;
	fnpAddEvent(&pevent);
}

static void ieee802_11_handle_medium_busy_state(NETSIM_ID d, NETSIM_ID in)
{
	PIEEE802_11_MAC_VAR mac = IEEE802_11_MAC(d, in);
	switch (mac->currMacState)
	{
		case IEEE802_11_MACSTATE_BACKING_OFF:
			add_event_due_to_medium_change(d, in, MAC_OUT_EVENT, IEEE802_11_EVENT_BACKOFF_PAUSED);
			break;
		case IEEE802_11_MACSTATE_Wait_DIFS:
			add_event_due_to_medium_change(d, in, MAC_OUT_EVENT, IEEE802_11_EVENT_DIFS_FAILED);
			break;
		case IEEE802_11_MACSTATE_OFF:
		case IEEE802_11_MACSTATE_MAC_IDLE:
		default:
			//do nothing
			break;
	}
}

static void ieee802_11_handle_medium_idle_state(NETSIM_ID d, NETSIM_ID in)
{
	PIEEE802_11_MAC_VAR mac = IEEE802_11_MAC(d, in);
	switch (mac->currMacState)
	{
		case IEEE802_11_MACSTATE_MAC_IDLE:
			add_event_due_to_medium_change(d, in, MAC_OUT_EVENT, 0);
			break;
		default:
			//do nothing
			break;
	}
	
}

void medium_change_callbackHandler(NETSIM_ID d,
								   NETSIM_ID in,
								   bool status)
{
	if (status)
		ieee802_11_handle_medium_busy_state(d, in);
	else
		ieee802_11_handle_medium_idle_state(d, in);
}

bool medium_isRadioIdleHandler(NETSIM_ID d,
							   NETSIM_ID in)
{
	PIEEE802_11_PHY_VAR phy = IEEE802_11_PHY(d, in);
	PIEEE802_11_MAC_VAR mac = IEEE802_11_MAC(d, in);
	if ((phy->radio.radioState == RX_OFF ||
		phy->radio.radioState == RX_ON_IDLE) &&
		isMacIdle(mac))
		return true;
	return false;
}

bool medium_isTransmitterBusyHandler(NETSIM_ID d,
									 NETSIM_ID in)
{
	PIEEE802_11_PHY_VAR phy = IEEE802_11_PHY(d, in);
	if (phy->radio.radioState == TRX_ON_BUSY)
		return true;
	return false;
}

double medium_getRxPowerHandler(NETSIM_ID tx,
								NETSIM_ID txif,
								NETSIM_ID rx,
								NETSIM_ID rxif,
								double time)
{
	return GET_RX_POWER_mw(tx, txif, rx, rxif, time);
}
