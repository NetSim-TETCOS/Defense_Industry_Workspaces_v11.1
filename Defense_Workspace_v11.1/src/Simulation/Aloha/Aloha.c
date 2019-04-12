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
#include "Aloha.h"

void send_to_phy();
void wait_for_random_time();

PHY_MODULATION getModulationFromString(char* val)
{
	if(!_stricmp(val,"QPSK"))
		return Modulation_QPSK;
	else if(!_stricmp(val,"BPSK"))
		return Modulation_BPSK;
	else if(!_stricmp(val,"16QAM"))
		return Modulation_16_QAM;
	else if(!_stricmp(val,"64QAM"))
		return Modulation_64_QAM;
	else
	{
		fnNetSimError("Unknown modulation %s. Assigning QPSK\n",val);
		return Modulation_QPSK;
	}
}

_declspec(dllexport) int fn_NetSim_Aloha_Configure(void** var)
{
	void* xmlNetSimNode;
	NETSIM_ID nDeviceId;
	NETSIM_ID nInterfaceId;
	LAYER_TYPE nLayerType;
	xmlNetSimNode = var[2];
	nDeviceId = *((NETSIM_ID*)var[3]);
	nInterfaceId = *((NETSIM_ID*)var[4]);
	nLayerType = *((LAYER_TYPE*)var[5]);
	switch(nLayerType)
	{	
	case PHYSICAL_LAYER:
		{
			PALOHA_PHY_VAR phy = ALOHA_PHY(nDeviceId,nInterfaceId);
			char* szVal;

			if(!phy)
			{
				phy = (PALOHA_PHY_VAR)calloc(1,sizeof *phy);
				DEVICE_PHYVAR(nDeviceId,nInterfaceId) = phy;
			}

			getXmlVar(&phy->frequency,FREQUENCY,xmlNetSimNode,1,_DOUBLE,ALOHA);
			getXmlVar(&phy->bandwidth,BANDWIDTH,xmlNetSimNode,1,_DOUBLE,ALOHA);
			getXmlVar(&phy->tx_power,TX_POWER,xmlNetSimNode,1, _DOUBLE,ALOHA);
			getXmlVar(&phy->rx_sensitivity,RECEIVER_SENSITIVITY_DBM,xmlNetSimNode,1, _DOUBLE,ALOHA);
			getXmlVar(&szVal,MODULATION,xmlNetSimNode,1,_STRING,ALOHA);
			phy->modulation = getModulationFromString(szVal);
			free(szVal);
			getXmlVar(&phy->data_rate,DATA_RATE,xmlNetSimNode,1, _DOUBLE,ALOHA);
		}
		break;
	case MAC_LAYER:
		{
			PALOHA_MAC_VAR mac = ALOHA_MAC(nDeviceId,nInterfaceId);
			char* szVal;

			if(!mac)
			{
				mac = (PALOHA_MAC_VAR)calloc(1,sizeof *mac);
				DEVICE_MACVAR(nDeviceId,nInterfaceId) = mac;
			}

			//Get the MAC address
			szVal = fn_NetSim_xmlConfig_GetVal(xmlNetSimNode,"MAC_ADDRESS",1);
			if(szVal)
			{
				NETWORK->ppstruDeviceList[nDeviceId-1]->ppstruInterfaceList[nInterfaceId-1]->pstruMACLayer->szMacAddress = STR_TO_MAC(szVal);
				free(szVal);
			}

			getXmlVar(&mac->slotlength,SLOT_LENGTH,xmlNetSimNode,0, _DOUBLE,ALOHA);

			getXmlVar(&mac->retryLimit, RETRY_LIMIT, xmlNetSimNode, 1, _UINT, ALOHA);

			getXmlVar(&mac->isMACBuffer, IS_MAC_BUFFER, xmlNetSimNode, 1, _BOOL, ALOHA);
		}
		break;
	default:
		fnNetSimError("%d layer is not implemented for Aloha protocol\n",nLayerType);
		break;
	}
	return 0;
}

_declspec (dllexport) int fn_NetSim_Aloha_Init(struct stru_NetSim_Network *NETWORK_Formal,
	NetSim_EVENTDETAILS *pstruEventDetails_Formal,
	char *pszAppPath_Formal,
	char *pszWritePath_Formal,
	int nVersion_Type,
	void **fnPointer)
{
	NETSIM_ID i;
	fn_NetSim_Aloha_CalulateReceivedPower();
	for(i=0;i<NETWORK->nDeviceCount;i++)
	{
		PALOHA_MAC_VAR mac=ALOHA_MAC(i+1,1);
		PALOHA_PHY_VAR phy=ALOHA_PHY(i+1,1);
		if (mac->slotlength)
		{
			mac->isSlotted = true;
			double t = get_slot_length(phy);
			mac->slotlength = max(t, mac->slotlength);
		}
		else
		{
			mac->slotlength = get_slot_length(phy);
		}


		phy->transmitter_status = RX_ON_IDLE;
		phy->dInterferencePower = MW_TO_DBM(0);
	}
	return 0;
}

_declspec (dllexport) int fn_NetSim_Aloha_Run()
{
	switch(pstruEventDetails->nEventType)
	{
	case MAC_OUT_EVENT:
		fn_NetSim_Aloha_Handle_MacOut();
		break;
	case MAC_IN_EVENT:
		pstruEventDetails->nEventType = NETWORK_IN_EVENT;
		fnpAddEvent(pstruEventDetails);
		break;
	case PHYSICAL_OUT_EVENT:
		fn_NetSim_Aloha_PhyOut();
		break;
	case PHYSICAL_IN_EVENT:
		fn_NetSim_Aloha_PhyIn();
		break;
	default:
		fnNetSimError("Unknown event type %d for aloha protocol in %s",pstruEventDetails->nEventType,__FUNCTION__);
		break;
	}
	return 0;
}

_declspec(dllexport) int fn_NetSim_Aloha_Finish()
{
	return 0;
}

_declspec (dllexport) char* fn_NetSim_Aloha_Trace(int nSubEvent)
{
	return "";
}
_declspec(dllexport) int fn_NetSim_Aloha_FreePacket(NetSim_PACKET* pstruPacket)
{
	return 0;
}

/**
	This function is called by NetworkStack.dll, to copy the aloha protocol
	details from source packet to destination.
*/
_declspec(dllexport) int fn_NetSim_Aloha_CopyPacket(NetSim_PACKET* pstruDestPacket,NetSim_PACKET* pstruSrcPacket)
{
	return 0;
}

/**
This function write the Metrics 	
*/
_declspec(dllexport) int fn_NetSim_Aloha_Metrics(PMETRICSWRITER metricsWriter)
{
	return 0;
}

/**
This function will return the string to write packet trace heading.
*/
_declspec(dllexport) char* fn_NetSim_Aloha_ConfigPacketTrace()
{
	return "";
}

/**
 This function will return the string to write packet trace.																									
*/
_declspec(dllexport) char* fn_NetSim_Aloha_WritePacketTrace(NetSim_PACKET* pstruPacket, char** ppszTrace)
{
	return "";
}
