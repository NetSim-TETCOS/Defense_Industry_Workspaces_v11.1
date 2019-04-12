#include "main.h"
#include "802_22.h"
int fn_NetSim_CR_CPE_MACIN();
int fn_NetSim_CR_BS_PhysicalOut();
int fn_NetSim_CR_PhysicalIn();
int fn_NetSim_CR_BS_MacOut();
/**  This function Initializes the CR parameters.*/
_declspec(dllexport) int fn_NetSim_CR_Init(struct stru_NetSim_Network *NETWORK_Formal,\
 NetSim_EVENTDETAILS *pstruEventDetails_Formal,char *pszAppPath_Formal,\
 char *pszWritePath_Formal,int nVersion_Type,void **fnPointer)
{
	fn_NetSim_CR_Init_F(NETWORK_Formal,pstruEventDetails_Formal,pszAppPath_Formal,pszWritePath_Formal,nVersion_Type,fnPointer);
	return 1;
}
/** This function is called by NetworkStack.dll, whenever the event gets triggered
		inside the NetworkStack.dll for the MAC layer in Cognitive Radio.
		It includes MAC_OUT,MAC_IN,PHY_OUT,PHY_IN and TIMER_EVENT. */
_declspec(dllexport) int fn_NetSim_CR_Run()
{
	switch(pstruEventDetails->nEventType)
	{
	case MAC_OUT_EVENT:
		{
			switch(pstruEventDetails->nDeviceType)
			{
			case BASESTATION:
				{
					fn_NetSim_CR_BS_MacOut();
				}
				break;
			default:	//CPE, node or any other device
				{
					switch(pstruEventDetails->nSubEventType)
					{
					case 0:
						{
							fn_NetSim_CR_PacketArrive();
						}
						break;
					}
				}
				break;
			}
		}
		break;
	case MAC_IN_EVENT:
		switch(pstruEventDetails->nDeviceType)
			{
			case BASESTATION:
				{
					fn_NetSim_CR_BS_MACIN();
				}
				break;
			default:	//CPE, node or any other device
				fn_NetSim_CR_CPE_MACIN();
				break;
			}
		break;
	case PHYSICAL_OUT_EVENT:
		{
			switch(pstruEventDetails->nDeviceType)
			{
			case BASESTATION:
				switch(pstruEventDetails->nSubEventType)
				{
				default:
					fn_NetSim_CR_BS_PhysicalOut();
					break;
				}
				break;
			default:	//CPE, Node or and other device 
				fn_NetSim_CR_CPE_PhysicalOut();
				break;
			}
		}
		break;
	case PHYSICAL_IN_EVENT:
		fn_NetSim_CR_PhysicalIn();
		break;
	case TIMER_EVENT:
		switch(pstruEventDetails->nSubEventType)
		{
		case INCUMBENT_OPERATION_START:
			fn_NetSim_CR_IncumbentStart();
			break;
		case INCUMBENT_OPERATION_END:
			fn_NetSim_CR_IncumbentEnd();
			break;
		case TRANSMIT_SCH:
			fn_NetSim_CR_TransmitSCH();
			break;
		case FORM_DS_BURST:
			fn_NetSim_CR_TransmitFCH();
			break;
		case TRANSMIT_DS_BURST:
			fn_NetSim_CR_TransmitDSBurst();
			break;
		case FORM_US_BURST:
			fn_NetSim_CR_FormUSBurst();
			break;
		case TRANSMIT_US_BURST:
			fn_NetSim_CR_TransmitUSBurst();
			break;
		case QUIET_PERIOD:
			fn_NetSim_CR_QuietPeriod();
			break;
		case SM_UPDATECHANNEL:
			fn_NetSim_CR_UpdateChannel();
			break;
		case DSA_RSP_TIMEOUT:
			//Not implemented
			break;
		case DSA_RVD_TIMEOUT:
			//Not implemented
			break;
		default:
			printf("CR--- Unknown sub event type %d for CR Network.\n",pstruEventDetails->nSubEventType);
			break;
		}
		break;
	default:
		printf("CR--- Unknown event type %d for CR Network.\n",pstruEventDetails->nEventType);
	}
	return 1;
}
_declspec(dllexport) char* fn_NetSim_CR_Trace(int nSubEvent)
{
	return fn_NetSim_CR_Trace_F(nSubEvent);
}
/** This function is called by NetworkStack.dll, to copy the CR destination packet to source packet. */
_declspec(dllexport) int fn_NetSim_CR_CopyPacket(const NetSim_PACKET* pstruSrcPacket,const NetSim_PACKET* pstruDestPacket)
{
	return fn_NetSim_CR_CopyPacket_F(pstruSrcPacket,pstruDestPacket);
}
/** This function is called by NetworkStack.dll,to free the packet. */
_declspec(dllexport) int fn_NetSim_CR_FreePacket(const NetSim_PACKET* pstruPacket)
{
	return fn_NetSim_CR_FreePacket_F(pstruPacket);
}
/** This function write the metrics in metrics.txt */
_declspec(dllexport) int fn_NetSim_CR_Metrics(PMETRICSWRITER metricsWriter)
{
	return fn_NetSim_CR_Metrics_F(metricsWriter);
}
/** This function is called by NetworkStack.dll, once the simulation end, this function free
	the allocated memory for the network.	 */
_declspec(dllexport) int fn_NetSim_CR_Finish()
{
	return fn_NetSim_CR_Finish_F();
}
/** This function is called by NetworkStack.dll to configure the CR devices.  */
_declspec(dllexport) int fn_NetSim_CR_Configure(void** var)
{
	fn_NetSim_CR_Configure_F(var);
	return 1;
}
_declspec(dllexport) char* fn_NetSim_CR_ConfigPacketTrace()
{
	return "";
}
_declspec(dllexport) char* fn_NetSim_CR_WritePacketTrace()
{
	return "";
}
/** This function is used to initialize the Physical layer parameters of Cognitive Radio Base station */
int fn_NetSim_CR_BS_PhysicalOut()
{
	NetSim_PACKET* pstruPacket;
	BS_PHY* pstruBSPhy;
	BS_MAC* pstruBSMAC;
	double dDataRate;
	double dTransmissionTime;
	double dTime = pstruEventDetails->dEventTime;
	NETSIM_ID nDeviceId = pstruEventDetails->nDeviceId;
	NETSIM_ID nInterfaceId = pstruEventDetails->nInterfaceId;
	netsimDEVICE_TYPE nDeviceType = pstruEventDetails->nDeviceType;
	pstruPacket = pstruEventDetails->pPacket;
	pstruBSPhy = DEVICE_PHYVAR(nDeviceId,nInterfaceId);
	pstruBSMAC = DEVICE_MACVAR(nDeviceId,nInterfaceId);
	if(pstruPacket->nControlDataType/100 == MAC_PROTOCOL_IEEE802_22)
	{
		switch(pstruPacket->nControlDataType%100)
		{
		case MMM_SCH:
			//Update phy layer details in packet
			pstruPacket->pstruPhyData->dArrivalTime = dTime;
			pstruPacket->pstruPhyData->dPayload = pstruPacket->pstruMacData->dPacketSize;
			pstruPacket->pstruPhyData->dOverhead = 0;
			pstruPacket->pstruPhyData->dPacketSize = pstruPacket->pstruPhyData->dOverhead + pstruPacket->pstruPhyData->dPayload;
			pstruPacket->pstruPhyData->dStartTime = dTime;
			pstruPacket->pstruPhyData->nPacketErrorFlag = PacketStatus_NoError;
			pstruPacket->pstruPhyData->nPhyMedium = PHY_MEDIUM_WIRELESS;
			//Set the phy mode = 2
			dDataRate = struPhyMode[SCH_PHY_MODE].dDataRate;
			//Calculate transmission time
			//SCH packet is one symbol long
			dTransmissionTime = pstruBSPhy->pstruSymbolParameter->dSymbolDuration;
			//Set the end time
			pstruPacket->pstruPhyData->dEndTime = dTime+dTransmissionTime;
			//Broadcast the packet over link
			fn_NetSim_CR_BroadCastPacket(pstruPacket,pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId);

			//Schedule the FCH Packet
			pstruEventDetails->dEventTime = pstruPacket->pstruPhyData->dEndTime;
			pstruEventDetails->nDeviceId = nDeviceId;
			pstruEventDetails->nDeviceType = nDeviceType;
			pstruEventDetails->nInterfaceId = nInterfaceId;
			pstruEventDetails->dPacketSize = 0;
			pstruEventDetails->nApplicationId = 0;
			pstruEventDetails->nEventType = TIMER_EVENT;
			pstruEventDetails->nPacketId = 0;
			pstruEventDetails->nSubEventType = FORM_DS_BURST;
			pstruEventDetails->pPacket = NULL;
			pstruEventDetails->szOtherDetails = NULL;
			fnpAddEvent(pstruEventDetails);
			fn_NetSim_Packet_FreePacket(pstruPacket);
			break;
		case MMM_FCH:
			//Update phy layer details in packet
			pstruPacket->pstruPhyData->dArrivalTime = dTime;
			pstruPacket->pstruPhyData->dPayload = pstruPacket->pstruMacData->dPacketSize;
			pstruPacket->pstruPhyData->dOverhead = 0;
			pstruPacket->pstruPhyData->dPacketSize = pstruPacket->pstruPhyData->dOverhead + pstruPacket->pstruPhyData->dPayload;
			pstruPacket->pstruPhyData->dStartTime = dTime;
			pstruPacket->pstruPhyData->nPacketErrorFlag = PacketStatus_NoError;
			pstruPacket->pstruPhyData->nPhyMedium = PHY_MEDIUM_WIRELESS;
			//Set the phy mode = 2
			dDataRate = struPhyMode[FCH_PHY_MODE].dDataRate;
			//Calculate transmission time
			//FCH packet is one symbol long
			dTransmissionTime = fn_NetSim_CR_CalculateTransmissionTime(pstruPacket->pstruMacData->dPacketSize,pstruBSPhy->pstruSymbolParameter);
			//Set the end time
			pstruPacket->pstruPhyData->dEndTime = dTime+dTransmissionTime;
			//Broadcast the packet over link
			fn_NetSim_CR_BroadCastPacket(pstruPacket,pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId);

			//Schedule the next packet
			pstruEventDetails->dEventTime = pstruPacket->pstruPhyData->dEndTime;
			pstruEventDetails->nDeviceId = nDeviceId;
			pstruEventDetails->nDeviceType = nDeviceType;
			pstruEventDetails->nInterfaceId = nInterfaceId;
			pstruEventDetails->dPacketSize = 0;
			pstruEventDetails->nApplicationId = 0;
			pstruEventDetails->nEventType = TIMER_EVENT;
			pstruEventDetails->nPacketId = 0;
			pstruEventDetails->nSubEventType = TRANSMIT_DS_BURST;
			pstruEventDetails->pPacket = NULL;
			pstruEventDetails->szOtherDetails = NULL;
			fnpAddEvent(pstruEventDetails);
			fn_NetSim_Packet_FreePacket(pstruPacket);
			break;
		default:
			goto TRANSMIT_BS_PACKET;
			break;

		}
	}
	else
	{
TRANSMIT_BS_PACKET:
		//Update phy layer details in packet
		pstruPacket->pstruPhyData->dArrivalTime = dTime;
		pstruPacket->pstruPhyData->dPayload = pstruPacket->pstruMacData->dPacketSize;
		pstruPacket->pstruPhyData->dOverhead = 0;
		pstruPacket->pstruPhyData->dPacketSize = pstruPacket->pstruPhyData->dOverhead + pstruPacket->pstruPhyData->dPayload;
		pstruPacket->pstruPhyData->dStartTime = dTime;
		pstruPacket->pstruPhyData->nPacketErrorFlag = PacketStatus_NoError;
		pstruPacket->pstruPhyData->nPhyMedium = PHY_MEDIUM_WIRELESS;
		//Get the data rate
		dDataRate = pstruBSPhy->pstruSymbolParameter->dDataRate;
		dTransmissionTime = fn_NetSim_CR_CalculateTransmissionTime(pstruPacket->pstruMacData->dPacketSize,pstruBSPhy->pstruSymbolParameter);
		pstruPacket->pstruPhyData->dEndTime = dTime+dTransmissionTime;
		if(isBroadcastPacket(pstruPacket))
			//Broadcast the packet over link
			fn_NetSim_CR_BroadCastPacket(pstruPacket,pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId);
		else if(isMulticastPacket(pstruPacket))
			fn_NetSim_CR_MulticastPacket(pstruPacket, pstruEventDetails->nDeviceId, pstruEventDetails->nInterfaceId);
		else
			fn_NetSim_CR_TransmitP2PPacket(pstruPacket,pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId);
		//Schedule the next packet

		pstruEventDetails->dEventTime = pstruPacket->pstruPhyData->dEndTime;
		if(pstruPacket->nControlDataType/100 == MAC_PROTOCOL_IEEE802_22)
			if(pstruPacket->nControlDataType%100==MMM_DCD)// DCD is the last control packet
				pstruEventDetails->dEventTime =pstruBSMAC->dFrameStartTime+pstruBSPhy->pstruSymbolParameter->dSymbolDuration*((int)ceil((pstruEventDetails->dEventTime-pstruBSMAC->dFrameStartTime)/pstruBSPhy->pstruSymbolParameter->dSymbolDuration));
		
		
		pstruEventDetails->nDeviceId = nDeviceId;
		pstruEventDetails->nDeviceType = nDeviceType;
		pstruEventDetails->nInterfaceId = nInterfaceId;
		pstruEventDetails->dPacketSize = 0;
		pstruEventDetails->nApplicationId = 0;
		pstruEventDetails->nEventType = TIMER_EVENT;
		pstruEventDetails->nPacketId = 0;
		pstruEventDetails->nSubEventType = TRANSMIT_DS_BURST;
		pstruEventDetails->pPacket = NULL;
		pstruEventDetails->szOtherDetails = NULL;
		fnpAddEvent(pstruEventDetails);
		if(isBroadcastPacket(pstruPacket) ||
		   isMulticastPacket(pstruPacket))
			fn_NetSim_Packet_FreePacket(pstruPacket);
	}
	//Add channel usages in CR metrics
	pstruBSPhy->pstruOpratingChannel->struChannelMetrics.dUsage += dTransmissionTime;
	return 0;
}
/** This function is used to initialize the Physical layer parameters of Cognitive Radio Base station */
int fn_NetSim_CR_PhysicalIn()
{
	//Call function for metrics
	fn_NetSim_Metrics_Add(pstruEventDetails->pPacket);
	//Call Trace function to write packet trace
	fn_NetSim_WritePacketTrace(pstruEventDetails->pPacket);
	if(pstruEventDetails->pPacket->nPacketStatus == PacketStatus_NoError)
	{
		//Add the MAC IN event
		pstruEventDetails->nEventType = MAC_IN_EVENT;
		fnpAddEvent(pstruEventDetails);
	}
	return 1;
}
/** This function is used to initialize the MAC layer parameters of CR CPE*/
int fn_NetSim_CR_CPE_MACIN()
{
	NETSIM_ID nDeviceId;
	NETSIM_ID nInterfaceId;
	NetSim_PACKET* pstruPacket;
	nDeviceId = pstruEventDetails->nDeviceId;
	nInterfaceId = pstruEventDetails->nInterfaceId;
	//Get the packet
	pstruPacket = pstruEventDetails->pPacket;
	if(pstruPacket->nControlDataType/100 == MAC_PROTOCOL_IEEE802_22)
	switch(pstruPacket->nControlDataType%100)
	{
	case MMM_SCH:
		fn_NetSim_CR_CPE_ProcessSCH(pstruPacket);
		//Drop the packet
		fn_NetSim_Packet_FreePacket(pstruPacket);
		return 1;
		break;
	case MMM_FCH:
		fn_NetSim_CR_CPE_ProcessFCH(pstruPacket);
		//Drop the packet
		fn_NetSim_Packet_FreePacket(pstruPacket);
		return 1;
		break;
	case MMM_DS_MAP:
		fn_NetSim_CR_CPE_ProcessDSMAP();
		//Drop the packet
		fn_NetSim_Packet_FreePacket(pstruPacket);
		return 1;
		break;
	case MMM_UCD:
		//Drop the packet
		fn_NetSim_Packet_FreePacket(pstruPacket);
		return 1;
		break;
	case MMM_US_MAP:
		fn_NetSim_CR_CPE_ProcessUSMAP();
		fn_NetSim_Packet_FreePacket(pstruPacket);
		return 1;
		break;
	case MMM_DCD:
		fn_NetSim_Packet_FreePacket(pstruPacket);
		return 1;
		break;
	case MMM_DSA_RSP:
		fn_NetSim_CR_CPE_ProcessDSA_RSP();
		fn_NetSim_Packet_FreePacket(pstruPacket);
		return 1;
		break;
	case MMM_DSD_REP:
		fn_NetSim_CR_CPE_ProcessDSD_REP();
		return 1;
		break;
	case MMM_CHS_REQ:
		fn_NetSim_CR_CPE_SwitchChannel();
		fn_NetSim_Packet_FreePacket(pstruPacket);
		return 1;
		break;
	default:
		//Unknown control packet
		printf("CR--- Unknown control packet arrives to CPE mac in\n");
		break;
	}
	else
	{
		fn_NetSim_CR_CPE_PackPacket();
	}
	return 0;
}
/** This function is used to initialize the Physical layer parameters of CR CPE*/
int fn_NetSim_CR_CPE_PhysicalOut()
 {
	double dTransmissionTime;
	CPE_MAC* pstruCPEMAC = DEVICE_MACVAR(pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId);
	CPE_PHY* pstruCPEPhy = DEVICE_PHYVAR(pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId);
	NetSim_PACKET* pstruPacket = pstruEventDetails->pPacket;
	//Update the phy layer
	pstruPacket->pstruPhyData->dArrivalTime = pstruEventDetails->dEventTime;
	pstruPacket->pstruPhyData->dPayload = pstruPacket->pstruMacData->dPacketSize;
	pstruPacket->pstruPhyData->dPacketSize = pstruPacket->pstruPhyData->dPayload;
	pstruPacket->pstruPhyData->dStartTime = pstruEventDetails->dEventTime;
	pstruPacket->pstruPhyData->nPhyMedium = PHY_MEDIUM_WIRELESS;
	dTransmissionTime = fn_NetSim_CR_CalculateTransmissionTime(pstruPacket->pstruPhyData->dPacketSize,pstruCPEPhy->pstruSymbol);
	if(DEVICE_PHYLAYER(pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId)->dLastPacketEndTime <pstruEventDetails->dEventTime)
		DEVICE_PHYLAYER(pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId)->dLastPacketEndTime =pstruEventDetails->dEventTime;
	pstruPacket->pstruPhyData->dEndTime = DEVICE_PHYLAYER(pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId)->dLastPacketEndTime  + dTransmissionTime;
	DEVICE_PHYLAYER(pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId)->dLastPacketEndTime = pstruPacket->pstruPhyData->dEndTime;

	if(pstruEventDetails->nSubEventType != TRANSMIT_US_BURST_CONTROL)
	{
		//Add an event to transmit US-Burst if not BW_Request packet
		pstruEventDetails->dEventTime = pstruPacket->pstruPhyData->dEndTime;
		pstruEventDetails->dPacketSize = 0;
		pstruEventDetails->nApplicationId = 0;
		pstruEventDetails->nEventType = TIMER_EVENT;
		pstruEventDetails->nPacketId = 0;
		pstruEventDetails->nProtocolId = MAC_PROTOCOL_IEEE802_22;
		pstruEventDetails->nSubEventType = TRANSMIT_US_BURST;
		pstruEventDetails->pPacket = NULL;
		fnpAddEvent(pstruEventDetails);
	}
	//Set the receiver as BS-Id
	pstruPacket->nReceiverId = pstruCPEMAC->nBSID;
	fn_NetSim_CR_TransmitP2PPacket(pstruPacket,
		pstruEventDetails->nDeviceId,
		pstruEventDetails->nInterfaceId);

	//Add Channel usages in CR metrics
	pstruCPEPhy->pstruOperatingChannel->struChannelMetrics.dUsage+=dTransmissionTime;
	return 0;
}
/** This function checks the forwarded packet is Data packet or Control packet and call the appropriate functions */
int fn_NetSim_CR_BS_MACIN()
{
	NetSim_PACKET* pstruPacket = pstruEventDetails->pPacket;
	if(pstruPacket->nPacketType != PacketType_Control || pstruPacket->nControlDataType/100 != MAC_PROTOCOL_IEEE802_22)
	{
		//Data packet or other protocol control packet
		fn_NetSim_CR_BS_ForwardDataPacket();
	}
	else
	{
		// CR Control packet
		switch(pstruPacket->nControlDataType%100)
		{
		case MMM_BW_REQUEST:
			{
				fn_NetSim_CR_BS_AllocateBandwidth();
				fn_NetSim_Packet_FreePacket(pstruPacket);
			}
			break;
		case MMM_DSA_REQ:
			{
				fn_NetSim_CR_BS_ProcessDSAReq();
				fn_NetSim_Packet_FreePacket(pstruPacket);
			}
			break;
		case MMM_DSD_REQ:
			{
				fn_NetSim_CR_BS_ProcessDSDReq();
			}
			break;
		case MMM_UCS_NOTIFICATION:
			{
				fn_NetSim_CR_BS_UCS();
				fn_NetSim_Packet_FreePacket(pstruPacket);
			}
			break;
		default:
			printf("CR--- Unknown control packet arrive to BS Mac\n");
			break;
		}
	}
	return 0;
}
/** This function is used to initialize the MAC layer parameters of CR Base station*/
int fn_NetSim_CR_BS_MacOut()
{
	BS_MAC* pstruBSMAC = (BS_MAC*)DEVICE_MACVAR(pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId);
	NetSim_PACKET* pstruPacket = fn_NetSim_Packet_GetPacketFromBuffer(NETWORK->ppstruDeviceList[pstruEventDetails->nDeviceId-1]->ppstruInterfaceList[pstruEventDetails->nInterfaceId-1]->pstruAccessInterface->pstruAccessBuffer,1);
	NetSim_PACKET* pstruTempPacket;
	if(pstruPacket->pstruMacData->nMACProtocol != MAC_PROTOCOL_IEEE802_22)
	{
		pstruPacket->pstruMacData->nMACProtocol = MAC_PROTOCOL_IEEE802_22;
		fn_NetSim_CR_FragmentPacket(pstruPacket,(double)MAX_SDU_SIZE);
	}
	pstruTempPacket = pstruBSMAC->pstruDSPacketList;
	if(!pstruTempPacket)
	{
		pstruBSMAC->pstruDSPacketList = pstruPacket;
		return 0;
	}
	while(pstruTempPacket->pstruNextPacket)
		pstruTempPacket = pstruTempPacket->pstruNextPacket;
	pstruTempPacket->pstruNextPacket = pstruPacket;
	return 0;
}
/** The function is used to allocate the bandwidtrh for the CR base-station */
int fn_NetSim_CR_BS_AllocateBandwidth()
{
	NETSIM_ID nDeviceId = pstruEventDetails->nDeviceId;
	NETSIM_ID nInterfaceId = pstruEventDetails->nInterfaceId;
	BS_MAC* pstruBSMAC = DEVICE_MACVAR(nDeviceId,nInterfaceId);
	NetSim_PACKET* pstruPacket = pstruEventDetails->pPacket;
	BW_REQUEST* pstruBWrequest = pstruPacket->pstruMacData->Packet_MACProtocol;
	SYMBOL_PARAMETER* pstruSymol = ((BS_PHY*)DEVICE_PHYVAR(nDeviceId,nInterfaceId))->pstruSymbolParameter;
	unsigned int nSlot = (unsigned int)ceil(pstruBWrequest->nBR*8.0/pstruSymol->nBitsCountInOneSlot);
	NETSIM_ID nSID = (NETSIM_ID)(pstruBSMAC->anSIDFromDevId[pstruPacket->nTransmitterId]);
	fn_NetSim_CR_AllocBandwidth(nSID,pstruPacket->nQOS,&(pstruBSMAC->uplinkAllocInfo),nSlot,pstruSymol->nUPlinkSymbol*pstruSymol->nOFDMSlots);
	return 0;
}

