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
#include "main.h"
#include "P2P.h"
#include "P2P_Enum.h"

//Function prototype
static void add_event_link_up();
static int fn_NetSim_P2P_CalulateReceivedPower();
int P2P_MacOut_Handler();
int P2P_MacIn_Handler();
int P2P_PhyOut_Handler();
int P2P_PhyIn_Handler();

/**
This function is called by NetworkStack.dll, whenever the event gets triggered
inside the NetworkStack.dll for the Mac/Phy layer P2P protocol
It includes MAC_OUT, MAC_IN, PHY_OUT, PHY_IN and TIMER_EVENT.
*/
_declspec (dllexport) int fn_NetSim_P2P_Run()
{
	switch (pstruEventDetails->nEventType)
	{
		case MAC_OUT_EVENT:
			P2P_MacOut_Handler();
			break;
		case MAC_IN_EVENT:
			P2P_MacIn_Handler();
			break;
		case PHYSICAL_OUT_EVENT:
			P2P_PhyOut_Handler();
			break;
		case PHYSICAL_IN_EVENT:
			P2P_PhyIn_Handler();
			break;
		case TIMER_EVENT:
		{
			switch (pstruEventDetails->nSubEventType)
			{
				case P2P_LINK_UP:
					notify_interface_up(pstruEventDetails->nDeviceId, pstruEventDetails->nInterfaceId);
					break;
				case P2P_LINK_DOWN:
					notify_interface_down(pstruEventDetails->nDeviceId, pstruEventDetails->nInterfaceId);
					break;
			}
		}
		break;
	}
	return 0;
}

PHY_MODULATION getModulationFromString(char* val)
{
	if (!_stricmp(val, "QPSK"))
		return Modulation_QPSK;
	else if (!_stricmp(val, "BPSK"))
		return Modulation_BPSK;
	else if (!_stricmp(val, "16QAM"))
		return Modulation_16_QAM;
	else if (!_stricmp(val, "64QAM"))
		return Modulation_64_QAM;
	else if (!_stricmp(val, "GMSK"))
		return Modulation_GMSK;
	else
	{
		fnNetSimError("Unknown modulation %s for P2P protocol. Assigning QPSK\n", val);
		return Modulation_QPSK;
	}
}

static void configure_wireless_P2P(NETSIM_ID d, NETSIM_ID in, void* xmlNetSimNode)
{
	char* szVal;
	ptrP2P_NODE_PHY phy = P2P_PHY(d, in);
	if (!phy)
	{
		phy = (ptrP2P_NODE_PHY)calloc(1, sizeof* phy);
		DEVICE_PHYVAR(d, in) = phy;
	}
	phy->iswireless = true;
	//Get the CENTRAL_FREQUENCY
	getXmlVar(&phy->dCenteralFrequency, CENTRAL_FREQUENCY, xmlNetSimNode, 1, _DOUBLE, P2P);
	//Get the BANDWIDTH
	getXmlVar(&phy->dBandwidth, BANDWIDTH, xmlNetSimNode, 1, _DOUBLE, P2P);
	phy->dBandwidth /= 1000;
	//Get the TX_POWER
	getXmlVar(&phy->dTXPower, TX_POWER, xmlNetSimNode, 1, _DOUBLE, P2P);
	phy->dTXPower *= 1000;
	//Get the TX_POWER
	getXmlVar(&phy->dDataRate, DATA_RATE, xmlNetSimNode, 1, _DOUBLE, P2P);
	//Get the RECEIVER_SENSITIVITY_DBM
	getXmlVar(&phy->dReceiverSensitivity, RECEIVER_SENSITIVITY_DBM, xmlNetSimNode, 1, _DOUBLE, P2P);
	//Get the modulation
	getXmlVar(&szVal, MODULATION_TECHNIQUE, xmlNetSimNode, 1, _STRING, P2P);
	phy->modulation = getModulationFromString(szVal);

	getXmlVar(&phy->dAntennaHeight, ANTENNA_HEIGHT, xmlNetSimNode, 1, _DOUBLE, P2P);
	getXmlVar(&phy->dAntennaGain, ANTENNA_GAIN, xmlNetSimNode, 1, _DOUBLE, P2P);
	getXmlVar(&phy->d0, D0, xmlNetSimNode, 1, _DOUBLE, P2P);
	getXmlVar(&phy->pld0, PL_D0, xmlNetSimNode, 1, _DOUBLE, P2P);
}

/**
This function is called by NetworkStack.dll, while configuring the device
Mac/Phy layer for P2P protocol.
*/
_declspec(dllexport) int fn_NetSim_P2P_Configure(void** var)
{
	char* tag;
	void* xmlNetSimNode;
	NETSIM_ID nDeviceId;
	NETSIM_ID nInterfaceId;
	LAYER_TYPE nLayerType;
	char* szVal;

	tag = (char*)var[0];
	xmlNetSimNode = var[2];
	if (!strcmp(tag, "PROTOCOL_PROPERTY"))
	{
		NETWORK = (struct stru_NetSim_Network*)var[1];
		nDeviceId = *((NETSIM_ID*)var[3]);
		nInterfaceId = *((NETSIM_ID*)var[4]);
		nLayerType = *((LAYER_TYPE*)var[5]);
		switch (nLayerType)
		{
			case MAC_LAYER:
				//Get the MAC address
				szVal = fn_NetSim_xmlConfig_GetVal(xmlNetSimNode, "MAC_ADDRESS", 1);
				if (szVal)
				{
					DEVICE_MACLAYER(nDeviceId, nInterfaceId)->szMacAddress = STR_TO_MAC(szVal);
					free(szVal);
				}
				break;
			case PHYSICAL_LAYER:
			{
				getXmlVar(&szVal, CONNECTION_MEDIUM, xmlNetSimNode, 1, _STRING, P2P);
				if (!_stricmp(szVal, "wireless"))
					configure_wireless_P2P(nDeviceId, nInterfaceId, xmlNetSimNode);
			}
			break;
			default:
				fnNetSimError("%d layer is not implemented for P2P protocol\n", nLayerType);
				break;
		}
	}
	return 0;
}

/**
This function initializes the P2P parameters.
*/
_declspec (dllexport) int fn_NetSim_P2P_Init(struct stru_NetSim_Network *NETWORK_Formal,
											 NetSim_EVENTDETAILS *pstruEventDetails_Formal,
											 char *pszAppPath_Formal,
											 char *pszWritePath_Formal,
											 int nVersion_Type,
											 void **fnPointer)
{
	add_event_link_up();
	fn_NetSim_P2P_CalulateReceivedPower();
	return 0;
}

/**
This function is called by NetworkStack.dll, once simulation end to free the
allocated memory for the network.
*/
_declspec(dllexport) int fn_NetSim_P2P_Finish()
{
	return 0;
}

/**
This function is called by NetworkStack.dll, while writing the event trace
to get the sub event as a string.
*/
_declspec (dllexport) char *fn_NetSim_P2P_Trace(int nSubEvent)
{
	return GetStringP2P_Subevent(nSubEvent);
}

/**
This function is called by NetworkStack.dll, to free the P2P protocol
*/
_declspec(dllexport) int fn_NetSim_P2P_FreePacket(NetSim_PACKET* pstruPacket)
{
	return 0;
}

/**
This function is called by NetworkStack.dll, to copy the P2P protocol from source packet to destination.
*/
_declspec(dllexport) int fn_NetSim_P2P_CopyPacket(NetSim_PACKET* pstruDestPacket, NetSim_PACKET* pstruSrcPacket)
{
	return 0;
}

/**
This function write the Metrics in Metrics.txt
*/
_declspec(dllexport) int fn_NetSim_P2P_Metrics(PMETRICSWRITER metricsWriter)
{
	return 0;
}

/**
This function will return the string to write packet trace heading.
*/
_declspec(dllexport) char* fn_NetSim_P2P_ConfigPacketTrace(const void* xmlNetSimNode)
{
	return "";
}

/**
This function will return the string to write packet trace.
*/
_declspec(dllexport) int fn_NetSim_P2P_WritePacketTrace(NetSim_PACKET* pstruPacket, char** ppszTrace)
{
	return 0;
}

static void p2p_gettxinfo(NETSIM_ID nTxId,
							NETSIM_ID nTxInterface,
							NETSIM_ID nRxId,
							NETSIM_ID nRxInterface,
							PTX_INFO Txinfo)
{
	ptrP2P_NODE_PHY txphy = (ptrP2P_NODE_PHY)DEVICE_PHYVAR(nTxId, nTxInterface);
	ptrP2P_NODE_PHY rxphy = (ptrP2P_NODE_PHY)DEVICE_PHYVAR(nRxId, nRxInterface);

	Txinfo->dCentralFrequency = txphy->dCenteralFrequency;
	Txinfo->dRxAntennaHeight = rxphy->dAntennaHeight;
	Txinfo->dRxGain = rxphy->dAntennaGain;
	Txinfo->dTxAntennaHeight = txphy->dAntennaHeight;
	Txinfo->dTxGain = txphy->dAntennaGain;
	Txinfo->dTxPower_mw = (double)txphy->dTXPower;
	Txinfo->dTxPower_dbm = MW_TO_DBM(Txinfo->dTxPower_mw);
	Txinfo->dTx_Rx_Distance = DEVICE_DISTANCE(nTxId, nRxId);
	Txinfo->d0 = txphy->d0;
}

static bool CheckFrequencyInterfrence(double dFrequency1, double dFrequency2, double bandwidth)
{
	if (dFrequency1 > dFrequency2)
	{
		if ((dFrequency1 - dFrequency2) >= bandwidth)
			return false; // no interference
		else
			return true; // interference
	}
	else
	{
		if ((dFrequency2 - dFrequency1) >= bandwidth)
			return false; // no interference
		else
			return true; // interference
	}
}

static bool check_interference(NETSIM_ID t, NETSIM_ID ti, NETSIM_ID r, NETSIM_ID ri)
{
	ptrP2P_NODE_PHY tp = (ptrP2P_NODE_PHY)DEVICE_PHYVAR(t, ti);
	ptrP2P_NODE_PHY rp = (ptrP2P_NODE_PHY)DEVICE_PHYVAR(r, ri);
	return CheckFrequencyInterfrence(tp->dCenteralFrequency, rp->dCenteralFrequency, tp->dBandwidth);
}

static int p2p_CalculateReceivedPower(NETSIM_ID tx, NETSIM_ID txi, NETSIM_ID rx, NETSIM_ID rxi)
{
	//Distance will change between Tx and Rx due to mobility.
	// So update the power from both side
	PTX_INFO info = get_tx_info(propagationHandle, tx, txi, rx, rxi);
	info->dTx_Rx_Distance = DEVICE_DISTANCE(tx, rx);
	propagation_calculate_received_power(propagationHandle,
										 tx, txi, rx, rxi, pstruEventDetails->dEventTime);
	return 1;
}

static int fn_NetSim_P2P_CalulateReceivedPower()
{
	NETSIM_ID t, ti, r, ri;

	propagationHandle = propagation_init(MAC_PROTOCOL_P2P, NULL,
										 p2p_gettxinfo, check_interference);

	for (t = 0; t < NETWORK->nDeviceCount; t++)
	{
		for (ti = 0; ti < DEVICE(t + 1)->nNumOfInterface; ti++)
		{
			if (!isP2PConfigured(t + 1, ti + 1))
				continue;
			if (!isP2PWireless(t + 1, ti + 1))
				continue;
			for (r = 0; r < NETWORK->nDeviceCount; r++)
			{
				for (ri = 0; ri < DEVICE(r + 1)->nNumOfInterface; ri++)
				{
					if (!isP2PConfigured(r + 1, ri + 1))
						continue;
					if (!isP2PWireless(r + 1, ri + 1))
						continue;
					p2p_CalculateReceivedPower(t + 1, ti + 1, r + 1, ri + 1);
				}
			}
		}
	}
	return 0;
}

int fn_NetSim_P2P_LinkStateChanged(NETSIM_ID linkId,
								   LINK_STATE newState)
{
	NetSim_LINKS* link = NETWORK->ppstruNetSimLinks[linkId-1];
	NETSIM_ID d1 = link->puniDevList.pstruP2P.nFirstDeviceId;
	NETSIM_ID d2 = link->puniDevList.pstruP2P.nSecondDeviceId;
	NETSIM_ID i1 = link->puniDevList.pstruP2P.nFirstInterfaceId;
	NETSIM_ID i2 = link->puniDevList.pstruP2P.nSecondInterfaceId;
	
	NETSIM_ID subevent;
	if (newState == LINKSTATE_UP)
		subevent = P2P_LINK_UP;
	else
		subevent = P2P_LINK_DOWN;
	NetSim_EVENTDETAILS pevent;
	memset(&pevent, 0, sizeof pevent);

	if (isP2PConfigured(d1, i1))
	{
		pevent.dEventTime = pstruEventDetails->dEventTime;
		pevent.nDeviceId = d1;
		pevent.nDeviceType = DEVICE_TYPE(d1);
		pevent.nEventType = TIMER_EVENT;
		pevent.nInterfaceId = i1;
		pevent.nProtocolId = MAC_PROTOCOL_P2P;
		pevent.nSubEventType = subevent;
		fnpAddEvent(&pevent);
	}

	if (isP2PConfigured(d2, i2))
	{
		pevent.dEventTime = pstruEventDetails->dEventTime;
		pevent.nDeviceId = d2;
		pevent.nDeviceType = DEVICE_TYPE(d2);
		pevent.nEventType = TIMER_EVENT;
		pevent.nInterfaceId = i2;
		pevent.nProtocolId = MAC_PROTOCOL_P2P;
		pevent.nSubEventType = subevent;
		fnpAddEvent(&pevent);
	}
	return 0;
}

static void add_event_link_up()
{
	NETSIM_ID d, i;

	NetSim_EVENTDETAILS pevent;
	memset(&pevent, 0, sizeof pevent);

	for (d = 0; d < NETWORK->nDeviceCount; d++)
	{
		for (i = 0; i < DEVICE(d + 1)->nNumOfInterface; i++)
		{
			if(!isP2PConfigured(d+1,i+1))
				continue;

			//Register link failure model callback
			fn_NetSim_Link_RegisterLinkFailureCallback(DEVICE_PHYLAYER(d + 1, i + 1)->pstruNetSimLinks->nLinkId,
													   fn_NetSim_P2P_LinkStateChanged);
			pevent.nDeviceId = d + 1;
			pevent.nDeviceType = DEVICE_TYPE(d + 1);
			pevent.nEventType = TIMER_EVENT;
			pevent.nInterfaceId = i + 1;
			pevent.nProtocolId = MAC_PROTOCOL_P2P;
			pevent.nSubEventType = P2P_LINK_UP;
			fnpAddEvent(&pevent);
		}
	}
}
