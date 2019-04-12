/************************************************************************************
 * Copyright (C) 2014                                                               *
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
#include "TDMA_enum.h"
#include "TDMA.h"

int fn_NetSim_TDMA_Mobility(NETSIM_ID nNodeId);

void TDMA_gettxinfo(NETSIM_ID nTxId,
						 NETSIM_ID nTxInterface,
						 NETSIM_ID nRxId,
						 NETSIM_ID nRxInterface,
						 PTX_INFO Txinfo)
{
	TDMA_NODE_PHY* txphy = (TDMA_NODE_PHY*)DEVICE_PHYVAR(nTxId, nTxInterface);
	TDMA_NODE_PHY* rxphy = (TDMA_NODE_PHY*)DEVICE_PHYVAR(nRxId, nRxInterface);
	
	Txinfo->dCentralFrequency = txphy->dFrequency;
	Txinfo->dRxAntennaHeight = rxphy->dAntennaHeight;
	Txinfo->dRxGain = rxphy->dAntennaGain;
	Txinfo->dTxAntennaHeight = txphy->dAntennaHeight;
	Txinfo->dTxGain = txphy->dAntennaGain;
	Txinfo->dTxPower_mw = (double)txphy->dTXPower;
	Txinfo->dTxPower_dbm = MW_TO_DBM(Txinfo->dTxPower_mw);
	Txinfo->dTx_Rx_Distance = DEVICE_DISTANCE(nTxId, nRxId);
}

bool CheckFrequencyInterfrence(double dFrequency1, double dFrequency2, double bandwidth)
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

bool check_interference(NETSIM_ID t, NETSIM_ID ti, NETSIM_ID r, NETSIM_ID ri)
{
	TDMA_NODE_PHY* tp = (TDMA_NODE_PHY*)DEVICE_PHYVAR(t, ti);
	TDMA_NODE_PHY* rp = (TDMA_NODE_PHY*)DEVICE_PHYVAR(r, ri);
	return CheckFrequencyInterfrence(tp->dFrequency, rp->dFrequency, tp->dChannelBandwidth);
}

/**
TDMA Init function initializes the TDMA parameters.
*/
_declspec (dllexport) int fn_NetSim_TDMA_Init(struct stru_NetSim_Network *NETWORK_Formal,
	NetSim_EVENTDETAILS *pstruEventDetails_Formal,
	char *pszAppPath_Formal,
	char *pszWritePath_Formal,
	int nVersion_Type,
	void **fnPointer)
{
	NETSIM_ID loop;
	
	propagationHandle = propagation_init(MAC_PROTOCOL_TDMA, NULL, TDMA_gettxinfo, check_interference);

	fn_NetSim_TDMA_NodeInit();
	fn_NetSim_TDMA_CreateAllocationFile();
	fn_NetSim_TDMA_ReadAllocationFile();
	fn_NetSim_TDMA_CalulateReceivedPower();
	for(loop=0;loop<NETWORK->nDeviceCount;loop++)
		fn_NetSim_TDMA_ScheduleDataTransmission(loop+1,0);
	fnMobilityRegisterCallBackFunction(fn_NetSim_TDMA_Mobility);
	return 0;
}

/**
	This function is called by NetworkStack.dll, whenever the event gets triggered		
	inside the NetworkStack.dll for the TDMA protocol									
*/
_declspec (dllexport) int fn_NetSim_TDMA_Run()
{
	switch(pstruEventDetails->nEventType)
	{
	case MAC_OUT_EVENT:
		{
			//Set the arrival time
			NetSim_BUFFER* buffer= DEVICE_MAC_NW_INTERFACE(pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId)->pstruAccessBuffer;
			NetSim_PACKET* packet = fn_NetSim_Packet_GetPacketFromBuffer(buffer,0);
			packet->pstruMacData->dArrivalTime=pstruEventDetails->dEventTime;
		}
		break;
	case MAC_IN_EVENT:
		memset(pstruEventDetails->pPacket->pstruMacData,0,sizeof* pstruEventDetails->pPacket->pstruMacData);
		memset(pstruEventDetails->pPacket->pstruPhyData,0,sizeof* pstruEventDetails->pPacket->pstruPhyData);
		
		pstruEventDetails->nProtocolId = fn_NetSim_Stack_GetNWProtocol(pstruEventDetails->nDeviceId);
		pstruEventDetails->nEventType=NETWORK_IN_EVENT;
		fnpAddEvent(pstruEventDetails);
		break;
	case PHYSICAL_OUT_EVENT:
		{
			fn_NetSim_TDMA_PhysicalOut();
		}
		break;
	case PHYSICAL_IN_EVENT:
		fn_NetSim_Metrics_Add(pstruEventDetails->pPacket);
		fn_NetSim_WritePacketTrace(pstruEventDetails->pPacket);
		pstruEventDetails->nEventType=MAC_IN_EVENT;
		fnpAddEvent(pstruEventDetails);
		break;
	case TIMER_EVENT:
		{
			switch(pstruEventDetails->nSubEventType)
			{
			case TDMA_SCHEDULE_Transmission:
				fn_NetSim_TDMA_SendToPhy();
				fn_NetSim_TDMA_RemoveCurrentInfo();
				fn_NetSim_TDMA_ScheduleDataTransmission(pstruEventDetails->nDeviceId,pstruEventDetails->dEventTime);
				break;
			default:
				fnNetSimError("Unnown subevent %d for Timer event in TDMA protocol\n",pstruEventDetails->nSubEventType);
				break;
			}
		}
		break;
	default:
		fnNetSimError("Unknown event %d for TDMA Mac protocol",pstruEventDetails->nEventType);
		break;
	}
	return 0;
}

/**
	This function is called by NetworkStack.dll, once simulation end to free the 
	allocated memory for the network.	
*/
_declspec(dllexport) int fn_NetSim_TDMA_Finish()
{
	NETSIM_ID i;
	if(nTDMAFileFlag)
	{
		char fileName[BUFSIZ];
		sprintf(fileName,"%s/TDMA_ALLOCATION.txt",pszIOPath);
		/*if(remove(fileName))
			fnSystemError("Unable to delete %s file",fileName);*/
	}
	for(i=0;i<NETWORK->nDeviceCount;i++)
	{
		if(NETWORK->ppstruDeviceList[i]->ppstruInterfaceList[0]->pstruMACLayer->nMacProtocolId == MAC_PROTOCOL_TDMA)
		{
			TDMA_NODE_MAC* mac=(TDMA_NODE_MAC*)DEVICE_MACVAR(i+1,1);
			TDMA_NODE_PHY* phy=(TDMA_NODE_PHY*)DEVICE_PHYVAR(i+1,1);
			while(mac->allocationInfo)
				LIST_FREE((void**)&mac->allocationInfo,mac->allocationInfo);
			free(mac);
			free(phy);
		}
	}

	propagation_finish(propagationHandle);
	return 0;
}

/**
	This function is called by NetworkStack.dll, while writing the event trace 
	to get the sub event as a string.
*/
_declspec (dllexport) char* fn_NetSim_TDMA_Trace(int nSubEvent)
{
	return GetStringTDMA_Subevent(nSubEvent);
}

/**
	This function is called by NetworkStack.dll, while configuring the device 
	for TDMA protocol.	
*/
_declspec(dllexport) int fn_NetSim_TDMA_Configure(void** var)
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
				TDMA_NODE_MAC* nodeMac=(TDMA_NODE_MAC*)DEVICE_MACVAR(nDeviceId,nInterfaceId);
				if(!nodeMac)
				{
					nodeMac=(TDMA_NODE_MAC*)calloc(1,sizeof* nodeMac);
					DEVICE_MACVAR(nDeviceId,nInterfaceId)=nodeMac;
				}
				//Get the MAC address
				szVal = fn_NetSim_xmlConfig_GetVal(xmlNetSimNode,"MAC_ADDRESS",1);
				if(szVal)
				{
					NETWORK->ppstruDeviceList[nDeviceId-1]->ppstruInterfaceList[nInterfaceId-1]->pstruMACLayer->szMacAddress = STR_TO_MAC(szVal);
					fnpFreeMemory(szVal);
				}
				//Get the SLOT_DURATION
				getXmlVar(&nodeMac->dSlotDuration,SLOT_DURATION,xmlNetSimNode,1,_DOUBLE,TDMA);
				nodeMac->dSlotDuration*=MILLISECOND;
				//Get the EPOCHES_DURATION
				getXmlVar(&nodeMac->dEpochesDuration,EPOCHES_DURATION,xmlNetSimNode,1,_DOUBLE,TDMA);
				nodeMac->dEpochesDuration *= SECOND*60;
				//Get the SET_COUNT
				getXmlVar(&nodeMac->nSetCount,SET_COUNT,xmlNetSimNode,1,_INT,TDMA);
				//Get the FRAME_DURATION
				getXmlVar(&nodeMac->dFrameDuration,FRAME_DURATION,xmlNetSimNode,1,_DOUBLE,TDMA);
				nodeMac->dFrameDuration *= SECOND;
				//Get the TIME_SLOT_BLOCK_SIZE
				getXmlVar(&nodeMac->nTSBSize,TIME_SLOT_BLOCK_SIZE,xmlNetSimNode,1,_INT,TDMA);
				//Get the NET_COUNT
				getXmlVar(&nodeMac->nNetCount,NET_COUNT,xmlNetSimNode,1,_INT,TDMA);
				//Get the NET_ID
				getXmlVar(&nodeMac->nNetId,NET_ID,xmlNetSimNode,1,_INT,TDMA);
				
			}
			break;
		case PHYSICAL_LAYER:
			{
				TDMA_NODE_PHY* phy=(TDMA_NODE_PHY*)DEVICE_PHYVAR(nDeviceId,nInterfaceId);
				if(!phy)
				{
					phy=(TDMA_NODE_PHY*)calloc(1,sizeof* phy);
					DEVICE_PHYVAR(nDeviceId,nInterfaceId)=phy;
				}
				//Get the TX_POWER
				getXmlVar(&phy->dTXPower,TX_POWER,xmlNetSimNode,1,_DOUBLE,TDMA);
				//Get the RECEIVER_SENSITIVITY_DBM
				getXmlVar(&phy->dReceiverSensitivity,RECEIVER_SENSITIVITY_DBM,xmlNetSimNode,1,_DOUBLE,TDMA);
				//Get the FREQUENCY
				getXmlVar(&phy->dFrequency, FREQUENCY, xmlNetSimNode, 1, _DOUBLE, TDMA);
				//Get the ANTENNA_HEIGHT
				getXmlVar(&phy->dAntennaHeight, ANTENNA_HEIGHT, xmlNetSimNode, 1, _DOUBLE, TDMA);
				//Get the ANTENNA_GAIN
				getXmlVar(&phy->dAntennaGain, ANTENNA_GAIN, xmlNetSimNode, 1, _DOUBLE, TDMA);
				//Get the CHANNEL_BANDWIDTH
				getXmlVar(&phy->dChannelBandwidth, CHANNEL_BANDWIDTH, xmlNetSimNode, 1, _DOUBLE, TDMA);
			}
			break;
		default:
			NetSimxmlError("Unknown layer type for TDMA protocol","",xmlNetSimNode);
			break;
		}
	}
	return 0;
}

/**
	This function is called by NetworkStack.dll, to free the TDMA protocol data.
*/
_declspec(dllexport) int fn_NetSim_TDMA_FreePacket(NetSim_PACKET* pstruPacket)
{
	return 0;
}

/**
	This function is called by NetworkStack.dll, to copy the TDMA protocol
	details from source packet to destination.
*/
_declspec(dllexport) int fn_NetSim_TDMA_CopyPacket(NetSim_PACKET* pstruDestPacket,NetSim_PACKET* pstruSrcPacket)
{
	return 0;
}

/**
This function write the Metrics 	
*/
_declspec(dllexport) int fn_NetSim_TDMA_Metrics(PMETRICSWRITER metricsWriter)
{
	return 0;
}

/**
This function will return the string to write packet trace heading.
*/
_declspec(dllexport) char* fn_NetSim_TDMA_ConfigPacketTrace()
{
	return "";
}

/**
 This function will return the string to write packet trace.																									
*/
_declspec(dllexport) char* fn_NetSim_TDMA_WritePacketTrace(NetSim_PACKET* pstruPacket, char** ppszTrace)
{
	return "";
}

int fn_NetSim_TDMA_Mobility(NETSIM_ID nNodeId)
{
	NETSIM_ID i;
	for(i=0;i<NETWORK->nDeviceCount;i++)
	{
		if(nNodeId!=i+1)
		{
			fn_NetSim_CalculateReceivedPower(i+1,nNodeId,pstruEventDetails->dEventTime);
			fn_NetSim_CalculateReceivedPower(nNodeId,i+1, pstruEventDetails->dEventTime);
		}
	}
	return 0;
}






