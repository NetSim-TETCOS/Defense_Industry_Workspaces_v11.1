/************************************************************************************
 * Copyright (C) 2015                                                               *
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
#include "DTDMA_enum.h"
#include "DTDMA.h"

int fn_NetSim_DTDMA_Mobility(NETSIM_ID nNodeId);
/**
DTDMA Init function initializes the DTDMA parameters.
*/
_declspec (dllexport) int fn_NetSim_DTDMA_Init(struct stru_NetSim_Network *NETWORK_Formal,
	NetSim_EVENTDETAILS *pstruEventDetails_Formal,
	char *pszAppPath_Formal,
	char *pszWritePath_Formal,
	int nVersion_Type,
	void **fnPointer)
{
	NETSIM_ID i,j;

	fn_NetSim_DTDMA_NodeInit();
	fn_NetSim_DTDMA_CalulateReceivedPower();
	init_dtdma_session();
	init_slot_formation();
	fn_NetSim_DTDMA_InitFrequencyHopping();
	fnMobilityRegisterCallBackFunction(fn_NetSim_DTDMA_Mobility);
	fnNodeJoinRegisterCallBackFunction(fnDTDMANodeJoinCallBack);
	return 0;
}

void update_receiver(NetSim_PACKET* packet)
{
	if (packet->nReceiverId)
		return; // Routing protocol will update
	if (isBroadcastPacket(packet))
		return;
	if (isMulticastPacket(packet))
		return;
	NETSIM_ID in;
	packet->nReceiverId = fn_NetSim_Stack_GetDeviceId_asIP(
		packet->pstruNetworkData->szNextHopIp, &in);
}

/**
	This function is called by NetworkStack.dll, whenever the event gets triggered		
	inside the NetworkStack.dll for the TDMA protocol									
*/
_declspec (dllexport) int fn_NetSim_DTDMA_Run()
{
	NETSIM_ID d = pstruEventDetails->nDeviceId;
	NETSIM_ID in = pstruEventDetails->nInterfaceId;

	switch(pstruEventDetails->nEventType)
	{
	case MAC_OUT_EVENT:
		{
			//Set the arrival time
			NetSim_BUFFER* buffer= DEVICE_MAC_NW_INTERFACE(pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId)->pstruAccessBuffer;
			NetSim_PACKET* packet = fn_NetSim_Packet_GetPacketFromBuffer(buffer,0);
			if (packet)
			{
				packet->pstruMacData->dArrivalTime = pstruEventDetails->dEventTime;
				update_receiver(packet);
			}
		}
		break;
	case MAC_IN_EVENT:
		if(pstruEventDetails->pPacket->nControlDataType/100 != MAC_PROTOCOL_DTDMA)
		{
			reassemble_packet();
			if(pstruEventDetails->pPacket)
			{
				memset(pstruEventDetails->pPacket->pstruMacData,0,sizeof* pstruEventDetails->pPacket->pstruMacData);
				memset(pstruEventDetails->pPacket->pstruPhyData,0,sizeof* pstruEventDetails->pPacket->pstruPhyData);
		
				pstruEventDetails->nProtocolId = fn_NetSim_Stack_GetNWProtocol(pstruEventDetails->nDeviceId);
				pstruEventDetails->nEventType=NETWORK_IN_EVENT;
				fnpAddEvent(pstruEventDetails);
			}
		}
		else
		{
			//Free the DTDMA Frame
			fn_NetSim_Packet_FreePacket(pstruEventDetails->pPacket);
			pstruEventDetails->pPacket = NULL;
		}
		break;
	case PHYSICAL_OUT_EVENT:
		{
			fn_NetSim_DTDMA_PhysicalOut();
		}
		break;
	case PHYSICAL_IN_EVENT:
		pstruEventDetails->pPacket->pstruPhyData->nPacketErrorFlag =
			fn_NetSim_DTDMA_CalculatePacketError(d, in, pstruEventDetails->pPacket);
		pstruEventDetails->pPacket->nPacketStatus = pstruEventDetails->pPacket->pstruPhyData->nPacketErrorFlag;
		fn_NetSim_Metrics_Add(pstruEventDetails->pPacket);
		fn_NetSim_WritePacketTrace(pstruEventDetails->pPacket);
		if(pstruEventDetails->pPacket->nPacketStatus == PacketStatus_NoError)
		{
			pstruEventDetails->nEventType=MAC_IN_EVENT;
			fnpAddEvent(pstruEventDetails);
		}
		else
		{
			fn_NetSim_Packet_FreePacket(pstruEventDetails->pPacket);
		}
		break;
	case TIMER_EVENT:
		{
			switch(pstruEventDetails->nSubEventType)
			{
			case DTDMA_SCHEDULE_Transmission:
				fn_NetSim_DTDMA_FrequencyHopping();
				fn_NetSim_DTDMA_SendToPhy();
				break;
			case DTDMA_FORM_SLOT:
				fn_NetSim_DTDMA_FormSlot();
				break;
			default:
				fnNetSimError("Unknown subevent %d for Timer event in DTDMA protocol\n",pstruEventDetails->nSubEventType);
				break;
			}
		}
		break;
	default:
		fnNetSimError("Unknown event %d for DTDMA Mac protocol",pstruEventDetails->nEventType);
		break;
	}
	return 0;
}

/**
	This function is called by NetworkStack.dll, once simulation end to free the 
	allocated memory for the network.	
*/
_declspec(dllexport) int fn_NetSim_DTDMA_Finish()
{
	NETSIM_ID i, j;
	for (i = 0; i < NETWORK->nDeviceCount; i++)
	{
		for (j = 0; j < DEVICE(i + 1)->nNumOfInterface; j++)
		{
			if (!isDTDMAConfigured(i + 1, j + 1))
				continue;

			if (NETWORK->ppstruDeviceList[i]->ppstruInterfaceList[j]->pstruMACLayer->nMacProtocolId == MAC_PROTOCOL_DTDMA)
			{
				DTDMA_NODE_MAC* mac = DTDMA_MAC(i + 1, j + 1);
				DTDMA_NODE_PHY* phy = DTDMA_PHY(i + 1, j + 1);
				free(mac);
				free(phy);
			}
		}
	}
	propagation_finish(propagationHandle);
	fn_NetSim_DTDMA_FinishFrequencyHopping();
	free_dtdma_session();
	return 0;
}

/**
	This function is called by NetworkStack.dll, while writing the event trace 
	to get the sub event as a string.
*/
_declspec (dllexport) char* fn_NetSim_DTDMA_Trace(int nSubEvent)
{
	return GetStringDTDMA_Subevent(nSubEvent);
}

/**
	This function is called by NetworkStack.dll, while configuring the device 
	for DTDMA protocol.	
*/
_declspec(dllexport) int fn_NetSim_DTDMA_Configure(void** var)
{
	char* tag;
	void* xmlNetSimNode;
	NETSIM_ID nDeviceId;
	NETSIM_ID nInterfaceId;
	LAYER_TYPE nLayerType;
	char* szVal;

	tag = (char*)var[0];
	xmlNetSimNode=var[2];
	if(!strcmp(tag,"PROTOCOL_PROPERTY"))
	{
		NETWORK=(struct stru_NetSim_Network*)var[1];
		nDeviceId = *((NETSIM_ID*)var[3]);
		nInterfaceId = *((NETSIM_ID*)var[4]);
		nLayerType = *((LAYER_TYPE*)var[5]);
		switch(nLayerType)
		{
		case MAC_LAYER:
			{
				DTDMA_NODE_MAC* nodeMac=DTDMA_MAC(nDeviceId,nInterfaceId);
				if(!nodeMac)
				{
					nodeMac=(DTDMA_NODE_MAC*)calloc(1,sizeof* nodeMac);
					DEVICE_MACVAR(nDeviceId,nInterfaceId)=nodeMac;
				}
				//Get the MAC address
				szVal = fn_NetSim_xmlConfig_GetVal(xmlNetSimNode,"MAC_ADDRESS",1);
				if(szVal)
				{
					NETWORK->ppstruDeviceList[nDeviceId-1]->ppstruInterfaceList[nInterfaceId-1]->pstruMACLayer->szMacAddress = STR_TO_MAC(szVal);
					fnpFreeMemory(szVal);
				}

				getXmlVar(&szVal,NETWORK_TYPE,xmlNetSimNode,1,_STRING,DTDMA);
				if(!_stricmp(szVal,"AD-HOC"))
					nodeMac->network_type=ADHOC;
				else
					nodeMac->network_type=INFRASTRUCTURE;
				free(szVal);

				getXmlVar(&szVal,SLOT_ALLOCATION_TECHNIQUE,xmlNetSimNode,1,_STRING,DTDMA);
				if (!_stricmp(szVal, "ROUNDROBIN"))
					nodeMac->slot_allocation_technique = TECHNIQUE_ROUNDROBIN;
				else if (!_stricmp(szVal, "DEMANDBASED"))
					nodeMac->slot_allocation_technique = TECHNIQUE_DEMANDBASED;
				else if (!_stricmp(szVal, "FILEBASED"))
					nodeMac->slot_allocation_technique = TECHNIQUE_FILE;
				free(szVal);
				if(nodeMac->slot_allocation_technique==TECHNIQUE_DEMANDBASED)
				{
					getXmlVar(&nodeMac->max_slot,MAX_SLOT_PER_DEVICE,xmlNetSimNode,1,_INT,DTDMA);
					getXmlVar(&nodeMac->min_slot,MIN_SLOT_PER_DEVICE,xmlNetSimNode,1,_INT,DTDMA);
				}

				//Get the SLOT_DURATION
				getXmlVar(&nodeMac->dSlotDuration,SLOT_DURATION,xmlNetSimNode,1,_DOUBLE,DTDMA);
				nodeMac->dSlotDuration*=MILLISECOND;
				//Get the FRAME_DURATION
				getXmlVar(&nodeMac->dFrameDuration,FRAME_DURATION,xmlNetSimNode,1,_DOUBLE,DTDMA);
				nodeMac->dFrameDuration *= SECOND;
				//Get the NET_COUNT
				getXmlVar(&nodeMac->nNetCount,NET_COUNT,xmlNetSimNode,1,_INT,DTDMA);
				//Get the NET_ID
				getXmlVar(&nodeMac->nNetId,NET_ID,xmlNetSimNode,1,_INT,DTDMA);
				//Get the guard interval
				getXmlVar(&nodeMac->dGuardInterval,GUARD_INTERVAL,xmlNetSimNode,1,_DOUBLE,DTDMA);
				//Get the BITS_PER_SLOT
				getXmlVar(&nodeMac->bitsPerSlot,BITS_PER_SLOT,xmlNetSimNode,1,_INT,DTDMA);
				//Get the OVERHEADS_PER_SLOT
				getXmlVar(&nodeMac->overheadPerSlot,OVERHEADS_PER_SLOT,xmlNetSimNode,1,_INT,DTDMA);
			}
			break;
		case PHYSICAL_LAYER:
			{
				DTDMA_NODE_PHY* phy=DTDMA_PHY(nDeviceId,nInterfaceId);
				if(!phy)
				{
					phy=(DTDMA_NODE_PHY*)calloc(1,sizeof* phy);
					DEVICE_PHYVAR(nDeviceId,nInterfaceId)=phy;
				}
				//Get the LOWER_FREQUENCY
				getXmlVar(&phy->dLowerFrequency,LOWER_FREQUENCY,xmlNetSimNode,1,_DOUBLE,DTDMA);
				//Get the UPPER_FREQUENCY
				getXmlVar(&phy->dUpperFrequency,UPPER_FREQUENCY,xmlNetSimNode,1,_DOUBLE,DTDMA);
				//Get the BANDWIDTH
				getXmlVar(&phy->dBandwidth,BANDWIDTH,xmlNetSimNode,1,_DOUBLE,DTDMA);
				phy->dBandwidth /= 1000;
				//Get the FREQUENCY_HOPPING
				getXmlVar(&szVal,FREQUENCY_HOPPING,xmlNetSimNode,1,_STRING,DTDMA);
				if(!_stricmp(szVal,"ON"))
					phy->frequency_hopping=true;
				else
					phy->frequency_hopping=false;
				//Get the TX_POWER
				getXmlVar(&phy->dTXPower,TX_POWER,xmlNetSimNode,1,_DOUBLE,DTDMA);
				phy->dTXPower *= 1000;
				//Get the RECEIVER_SENSITIVITY_DBM
				getXmlVar(&phy->dReceiverSensitivity,RECEIVER_SENSITIVITY_DBM,xmlNetSimNode,1,_DOUBLE,DTDMA);
				//Get the modulation
				getXmlVar(&szVal,MODULATION_TECHNIQUE,xmlNetSimNode,1,_STRING,DTDMA);
				phy->modulation = getModulationFromString(szVal);

				getXmlVar(&phy->dAntennaHeight, ANTENNA_HEIGHT, xmlNetSimNode, 1, _DOUBLE, DTDMA);
				getXmlVar(&phy->dAntennaGain, ANTENNA_GAIN, xmlNetSimNode, 1, _DOUBLE, DTDMA);
				getXmlVar(&phy->d0, D0, xmlNetSimNode, 1, _DOUBLE, DTDMA);
				getXmlVar(&phy->pld0, PL_D0, xmlNetSimNode, 1, _DOUBLE, DTDMA);

			}
			break;
		default:
			NetSimxmlError("Unknown layer type for D-TDMA protocol","",xmlNetSimNode);
			break;
		}
	}
	return 0;
}

/**
	This function is called by NetworkStack.dll, to free the TDMA protocol data.
*/
_declspec(dllexport) int fn_NetSim_DTDMA_FreePacket(NetSim_PACKET* pstruPacket)
{
	if(pstruPacket->pstruMacData->Packet_MACProtocol)
	{
		PDTDMA_MAC_PACKET mp = (PDTDMA_MAC_PACKET)pstruPacket->pstruMacData->Packet_MACProtocol;
		free(mp);
	}
	pstruPacket->pstruMacData->Packet_MACProtocol=NULL;
	return 0;
}

/**
	This function is called by NetworkStack.dll, to copy the TDMA protocol
	details from source packet to destination.
*/
_declspec(dllexport) int fn_NetSim_DTDMA_CopyPacket(NetSim_PACKET* pstruDestPacket,NetSim_PACKET* pstruSrcPacket)
{
	if(pstruSrcPacket->pstruMacData->Packet_MACProtocol)
	{
		PDTDMA_MAC_PACKET mp = (PDTDMA_MAC_PACKET)pstruSrcPacket->pstruMacData->Packet_MACProtocol;
		pstruDestPacket->pstruMacData->Packet_MACProtocol = calloc(1,sizeof* mp);
		memcpy(pstruDestPacket->pstruMacData->Packet_MACProtocol,mp,sizeof* mp);
	}
	return 0;
}

/**
This function write the Metrics 	
*/
_declspec(dllexport) int fn_NetSim_DTDMA_Metrics(PMETRICSWRITER metricsWriter)
{
	return 0;
}

/**
This function will return the string to write packet trace heading.
*/
_declspec(dllexport) char* fn_NetSim_DTDMA_ConfigPacketTrace()
{
	return "";
}

/**
 This function will return the string to write packet trace.																									
*/
_declspec(dllexport) char* fn_NetSim_DTDMA_WritePacketTrace(NetSim_PACKET* pstruPacket, char** ppszTrace)
{
	return "";
}

int fn_NetSim_DTDMA_Mobility(NETSIM_ID nNodeId)
{
	NETSIM_ID t, ti, i;
	for (i = 0; i < DEVICE(nNodeId)->nNumOfInterface; i++)
	{
		if (!isDTDMAConfigured(nNodeId, i + 1))
			continue;

		for (t = 0; t < NETWORK->nDeviceCount; t++)
		{
			if (nNodeId == t + 1)
				continue;

			for (ti = 0; ti < DEVICE(t + 1)->nNumOfInterface; ti++)
			{
				if (!isDTDMAConfigured(t + 1, ti + 1))
					continue;

				dtdma_CalculateReceivedPower(t + 1, ti + 1, nNodeId, i + 1);
				dtdma_CalculateReceivedPower(nNodeId, i + 1, t + 1, ti + 1);
			}
		}
	}
	return 0;
}

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
	else if (!_stricmp(val, "GMSK"))
		return Modulation_GMSK;
	else
	{
		fnNetSimError("Unknown modulation %s for D-TDMA protocol. Assigning QPSK\n",val);
		return Modulation_QPSK;
	}
}

int fnDTDMANodeJoinCallBack(NETSIM_ID nDeviceId, double time, NODE_ACTION action)
{
	NETSIM_ID i;
	for (i = 0; i < DEVICE(nDeviceId)->nNumOfInterface; i++)
	{
		if (!isDTDMAConfigured(nDeviceId, i + 1))
			continue;
		PDTDMA_NODE_MAC mac = DTDMA_MAC(nDeviceId, i + 1);
		if (action == JOIN)
			mac->nodeInfo->nodeAction = CONNECTED;
		else if (action == LEAVE)
			mac->nodeInfo->nodeAction = NOT_CONNECTED;
		else
			fnNetSimError("Unknown action %d in function fnDTDMANodeJoinCallBack\n", action);
	}
	return 0;
}

int fn_NetSim_DTDMA_NodeInit()
{
	NETSIM_ID i, j;
	for (i = 0; i < NETWORK->nDeviceCount; i++)
	{
		for (j = 0; j < DEVICE(i + 1)->nNumOfInterface; j++)
		{

			if (!isDTDMAConfigured(i + 1, j + 1))
				continue;

			NetSim_BUFFER* buffer = DEVICE_ACCESSBUFFER(i + 1, j + 1);
			DTDMA_NODE_MAC* mac = DTDMA_MAC(i + 1, j + 1);
			DTDMA_NODE_PHY* phy = DTDMA_PHY(i + 1, j + 1);

			mac->nSlotCountInFrame = (unsigned int)(mac->dFrameDuration / (mac->dSlotDuration + mac->dGuardInterval));
			mac->nFrameCount = (unsigned int)ceil(NETWORK->pstruSimulationParameter->dVal / (mac->dFrameDuration + DTDMA_INTER_FRAME_GAP));
			phy->dDataRate = mac->bitsPerSlot / mac->dSlotDuration;

			mac->reassembly = (NetSim_PACKET**)calloc(NETWORK->nDeviceCount, sizeof* mac->reassembly);

			if (buffer->nSchedulingType == SCHEDULING_NONE)
				buffer->nSchedulingType = SCHEDULING_FIFO;
		}
	}
	return 0;
}

