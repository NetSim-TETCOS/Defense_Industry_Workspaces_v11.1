#include "main.h"
#include "802_22.h"
#include "SpectrumManager.h"
/** This function is used  set the channel number and assign the channel characteristics for all channels */
int fn_NetSim_CR_FormChannelSet(BS_PHY* pstruBSPhy)
{
	struct stru_802_22_Channel* pstruFirstChannel;
	int nStartFrequency;
	int nEndFrequency;
	int nChannelCount;
	int nChannelBandwidth;
	int nLoop;
	unsigned int nChannelNumber = 1;
	nStartFrequency = pstruBSPhy->nMINFrequency;
	nEndFrequency = pstruBSPhy->nMAXFrequency;
	nChannelBandwidth = pstruBSPhy->nChannelBandwidth;
	nChannelCount = (nEndFrequency-nStartFrequency)/nChannelBandwidth;
	if(nChannelCount)
	{
		pstruFirstChannel = (struct stru_802_22_Channel*)fnpAllocateMemory(nChannelCount+1,sizeof *pstruFirstChannel);
		pstruBSPhy->pstruChannelSet = pstruFirstChannel;
	}
	else
	{
		fnNetSimError("Channel count is 0.\nStart frequency=%d\nEnd Frequency =%d\nBandwidth=%d\nChannel Count=%d\n",nStartFrequency,nEndFrequency,nChannelBandwidth,0);
		return -1;
	}
	for(nLoop=0;nLoop<nChannelCount;nLoop++)
	{
		pstruFirstChannel->dChannelBandwidth = nChannelBandwidth;
		pstruFirstChannel->dLowerFrequency = nStartFrequency;
		pstruFirstChannel->dUpperFrequency = nStartFrequency + nChannelBandwidth;
		pstruFirstChannel->nChannelState = ChannelState_UNCLASSIFIED;
		pstruFirstChannel->pstruNextChannel = pstruFirstChannel+1;
		pstruFirstChannel->nChannelNumber = nChannelNumber++;
		nStartFrequency += nChannelBandwidth;
		pstruFirstChannel++;
	}
	pstruFirstChannel--;
	pstruFirstChannel->pstruNextChannel = NULL;
	return 0;
}
/** This function is used to perform CPE network entry & initialization process */
int fn_NetSim_CR_AssociateCPE(NETSIM_ID nDeviceId,NETSIM_ID nInterfaceId)
{
	unsigned int nSID=0x001;
	NETSIM_ID nLoop;
	BS_MAC* pstruBSMac = (BS_MAC*)DEVICE_MACVAR(nDeviceId,nInterfaceId);
	BS_PHY* pstruBSPhy = (BS_PHY*)DEVICE_PHYVAR(nDeviceId,nInterfaceId);
	NetSim_LINKS* pstruLink;
	CPE_MAC* pstruCPEMAC;
	CPE_PHY* pstruCPEPhy;
	NETSIM_ID nCPEID=0;
	NETSIM_ID nCPEInterfaceId=0;
	SYMBOL_PARAMETER* pstruSymbol = pstruBSPhy->pstruSymbolParameter;
	struct stru_802_22_Channel* pstruChannelSet = pstruBSPhy->pstruChannelSet;
	struct stru_802_22_Channel* pstruOperatingChannel = pstruBSPhy->pstruOpratingChannel;
	char* szCountryCode = pstruBSMac->szISOCountryCode;
	pstruLink = DEVICE_PHYLAYER(nDeviceId,nInterfaceId)->pstruNetSimLinks;
	if(!pstruLink)
		fnNetSimError("No link found from Device %d and interface %d\n",DEVICE(nDeviceId)->nConfigDeviceId,nInterfaceId);
	switch(pstruLink->nLinkType)
	{
	case LinkType_P2P:
		if(nDeviceId != pstruLink->puniDevList.pstruP2P.nFirstDeviceId)
		{
			nCPEID = pstruLink->puniDevList.pstruP2P.nFirstDeviceId;
			nCPEInterfaceId = pstruLink->puniDevList.pstruP2P.nFirstInterfaceId;
			pstruBSMac->anSIDFromDevId[pstruLink->puniDevList.pstruP2P.nFirstDeviceId] = nSID++;
		}
		if(nDeviceId != pstruLink->puniDevList.pstruP2P.nSecondDeviceId)
		{
			nCPEID = pstruLink->puniDevList.pstruP2P.nSecondDeviceId;
			nCPEInterfaceId = pstruLink->puniDevList.pstruP2P.nSecondInterfaceId;
			pstruBSMac->anSIDFromDevId[pstruLink->puniDevList.pstruP2P.nSecondDeviceId] = nSID++;
		}
		pstruCPEMAC = (CPE_MAC*)DEVICE_MACVAR(nCPEID,nCPEInterfaceId);
		pstruCPEPhy = (CPE_PHY*)DEVICE_PHYVAR(nCPEID,nCPEInterfaceId);
		pstruCPEMAC->nSID = nSID-1;
		pstruCPEMAC->nBSID = nDeviceId;
		pstruCPEMAC->nBSInterface = nInterfaceId;
		pstruCPEPhy->pstruSymbol = pstruSymbol;
		pstruCPEPhy->pstruChannelSet = pstruChannelSet;
		pstruCPEPhy->pstruOperatingChannel = pstruOperatingChannel;
		strcpy(pstruCPEMAC->szCountryCode,szCountryCode);
		break;
	case LinkType_P2MP:
		if(nDeviceId != pstruLink->puniDevList.pstrup2MP.nCenterDeviceId)
		{
			nCPEID = pstruLink->puniDevList.pstrup2MP.nCenterDeviceId;
			nCPEInterfaceId = pstruLink->puniDevList.pstrup2MP.nCenterInterfaceId;
			pstruBSMac->anSIDFromDevId[pstruLink->puniDevList.pstrup2MP.nCenterDeviceId] = nSID++;
			pstruCPEMAC = (CPE_MAC*)DEVICE_MACVAR(nCPEID,nCPEInterfaceId);
			pstruCPEPhy = (CPE_PHY*)DEVICE_PHYVAR(nCPEID,nCPEInterfaceId);
			pstruCPEMAC->nSID = nSID-1;
			pstruCPEMAC->nBSID = nDeviceId;
			pstruCPEMAC->nBSInterface = nInterfaceId;
			pstruCPEPhy->pstruSymbol = pstruSymbol;
			pstruCPEPhy->pstruChannelSet = pstruChannelSet;
			pstruCPEPhy->pstruOperatingChannel = pstruOperatingChannel;
			strcpy(pstruCPEMAC->szCountryCode,szCountryCode);
		}
		for(nLoop=0;nLoop<pstruLink->puniDevList.pstrup2MP.nConnectedDeviceCount-1;nLoop++)
		{
			if(nDeviceId != pstruLink->puniDevList.pstrup2MP.anDevIds[nLoop])
			{
				nCPEID = pstruLink->puniDevList.pstrup2MP.anDevIds[nLoop];
				nCPEInterfaceId = pstruLink->puniDevList.pstrup2MP.anDevInterfaceIds[nLoop];
				pstruBSMac->anSIDFromDevId[pstruLink->puniDevList.pstrup2MP.anDevIds[nLoop]] = nSID++;
				pstruCPEMAC = (CPE_MAC*)DEVICE_MACVAR(nCPEID,nCPEInterfaceId);
				pstruCPEPhy = (CPE_PHY*)DEVICE_PHYVAR(nCPEID,nCPEInterfaceId);
				pstruCPEMAC->nSID = nSID-1;
				pstruCPEMAC->nBSID = nDeviceId;
				pstruCPEMAC->nBSInterface = nInterfaceId;
				pstruCPEPhy->pstruSymbol = pstruSymbol;
				pstruCPEPhy->pstruChannelSet = pstruChannelSet;
				pstruCPEPhy->pstruOperatingChannel = pstruOperatingChannel;
				strcpy(pstruCPEMAC->szCountryCode,szCountryCode);
			}
		}
		break;
	case LinkType_MP2MP:
		for(nLoop=0;nLoop<pstruLink->puniDevList.pstruMP2MP.nConnectedDeviceCount;nLoop++)
		{
			if(nDeviceId != pstruLink->puniDevList.pstruMP2MP.anDevIds[nLoop])
			{
				nCPEID = pstruLink->puniDevList.pstruMP2MP.anDevIds[nLoop];
				nCPEInterfaceId = pstruLink->puniDevList.pstruMP2MP.anDevInterfaceIds[nLoop];
				pstruBSMac->anSIDFromDevId[pstruLink->puniDevList.pstruMP2MP.anDevIds[nLoop]] = nSID++;
				pstruCPEMAC = (CPE_MAC*)DEVICE_MACVAR(nCPEID,nCPEInterfaceId);
				pstruCPEPhy = (CPE_PHY*)DEVICE_PHYVAR(nCPEID,nCPEInterfaceId);
				pstruCPEMAC->nSID = nSID-1;
				pstruCPEMAC->nBSID = nDeviceId;
				pstruCPEMAC->nBSInterface = nInterfaceId;
				pstruCPEPhy->pstruSymbol = pstruSymbol;
				pstruCPEPhy->pstruChannelSet = pstruChannelSet;
				pstruCPEPhy->pstruOperatingChannel = pstruOperatingChannel;
				strcpy(pstruCPEMAC->szCountryCode,szCountryCode);
			}
		}
		break;
	}
	return 1;
}
/** SSA initialization processes shall exist at the BS and CPE. In the case
of the BS SSA initialization, if there is at least one available channel (N0), 
a selection shall be made and a second round of spectrum sensing shall then take place on the
adjacent channels of the selected channel. 
If a WRAN signal is detected, RF signal sensing and signal classification shall then be carried
out on the channel (N0+1 or N0–1 or both) through the sensing path during the identified quiet
periods to verify the presence and the identity of the incumbent service underneath the WRAN
operation. In such case, a new available channel will need to be selected. */
int fn_NetSim_CR_SSA_Initialization(BS_MAC* pstruBSMAC,BS_PHY* pstruBSPHY)
{
	//Initial scan the channel
	fn_NetSim_CR_IniScanChannel(pstruBSMAC,pstruBSPHY);
	//Update the operating channel
	fn_NetSim_CR_UpdateOperatingChannel(pstruBSPHY);
	return 1;
}
/** The mechanism for quiet period management consists of the following two sensing stages, 
which are realized through the use of network-wide quiet periods, but which have different 
time scales: 
 a) Intra-frame sensing
 b) Inter-frame sensing
 
 For more Information,refer IEEE 802.22-2011 Document Section 7.21.1*/
int fn_Netsim_CR_SM_ScheduleQuietPeriod(BS_PHY* pstruBSPhy,SCH* pstruSCH)
{
	pstruSCH->n_Current_Intra_frame_Quiet_Period_Cycle_Length = pstruBSPhy->nIntraFrameQuietPeriodCycleLength;
	pstruSCH->n_Current_Intra_frame_Quiet_Period_Cycle_Offset = 0;
	if(pstruBSPhy->nIFQPOffset == pstruBSPhy->nIntraFrameQuietPeriodCycleLength)
		pstruBSPhy->nIFQPOffset = 0;
	if(pstruBSPhy->nIntraFrameQuietPeriodCycleLength && !pstruBSPhy->nIFQPOffset)
	{
		pstruSCH->n_Current_Intra_frame_Quiet_Period_Cycle_Offset = pstruBSPhy->nIFQPOffset;
		pstruBSPhy->nIFQPOffset++;
		strcpy(pstruSCH->sz_Current_Intra_frame_Quiet_period_Cycle_Frame_Bitmap,pstruBSPhy->szIntraframeQuietPeiordBitmap);
		pstruSCH->n_Current_Intra_frame_Quiet_Period_Duration = pstruBSPhy->nIntraFrameQuietPeriodDuration;
	}
	else
	{
		pstruSCH->n_Current_Intra_frame_Quiet_Period_Cycle_Offset = pstruBSPhy->nIFQPOffset;
		pstruBSPhy->nIFQPOffset++;
		strcpy(pstruSCH->sz_Current_Intra_frame_Quiet_period_Cycle_Frame_Bitmap,"0000000000000000");
	}
	pstruSCH->n_Claimed_Intra_frame_Quiet_period_Cycle_Frame_Bitmap = 0;
	pstruSCH->n_Claimed_Intra_frame_Quiet_Period_Cycle_Length = 0;
	pstruSCH->n_Claimed_Intra_frame_Quiet_Period_Cycle_Offset = 0;
	pstruSCH->n_Claimed_Intra_frame_Quiet_Period_Duration = 0;
	pstruSCH->n_Synchronization_Counter_for_Intra_frame_Quiet_Period_Rate = 0;
	pstruSCH->n_Synchronization_Counter_for_Intra_frame_Quiet_Period_Duration = 0;
	pstruSCH->n_Inter_frame_Quiet_Period_Duration = 0;
	pstruSCH->n_Inter_frame_Quiet_Period_Offset = 0;
	return 1;
}
/** In order to meet the Channel Detection Time for detecting the presence of incumbents 
in the operating channel, an IEEE 802.22 network shall schedule network-wide quiet periods
for sensing. During these quiet periods, all network traffic is suspended and base stations 
and CPEs shall perform in-band sensing. This process is coordinated by the BS, which is 
responsible for scheduling the quiet periods.

Note: For further details, please Refer IEEE 802.22-2011 Document Section 7.21 */
int fn_NetSim_CR_QuietPeriod()
{
	CPE_MAC* pstruCPEMAC = (CPE_MAC*)DEVICE_MACVAR(pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId);
	CPE_PHY* pstruCPEPHY = (CPE_PHY*)DEVICE_PHYVAR(pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId);
	BS_MAC* pstruBSMAC = (BS_MAC*)DEVICE_MACVAR(pstruCPEMAC->nBSID,pstruCPEMAC->nBSInterface);
	struct stru_802_22_SSFInput* input;
	struct stru_802_22_SSFOutput* output;
	input = (struct stru_802_22_SSFInput*)fnpAllocateMemory(1,sizeof* input);
	input->nChannelBandwidth = pstruCPEPHY->pstruSymbol->nChannelBandwidth;
	input->nChannelNumber = pstruCPEPHY->pstruOperatingChannel->nChannelNumber;
	strcpy(input->szCountryCode,pstruCPEMAC->szCountryCode);
	input->pstruChannelList = pstruCPEPHY->pstruChannelSet;
	input->nMaxProbabilityOfFalseAlram = pstruBSMAC->nFalseAlramProbability;
	input->nSensingMode = 0;
	input->SensingWindowSpecificationArray.NumSensingPeriods = pstruBSMAC->nNumSensingPeriod;
	input->SensingWindowSpecificationArray.SensingPeriodDuration = (int)pstruBSMAC->dSensingPeriodDuration;
	input->SensingWindowSpecificationArray.SensingPeriodInterval = (int)pstruBSMAC->dSensingPeriodInterval;
	input->nIncumbentCount = pstruBSMAC->nIncumbentCount;
	input->pstruIncumbent = pstruBSMAC->pstruIncumbent;
	//Call SSF function
	output = fn_NetSim_CR_CPE_SSF(input,pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId);
	if(output->nSignalPresentVector == 0xFF)
	{
		//Generate the UCS notification packet
		NetSim_PACKET* pstruPacket;
		NetSim_PACKET* pstruTempPacket;
		GMH* pstruGMH = (GMH*)fnpAllocateMemory(1,sizeof *pstruGMH);
		pstruPacket = fn_NetSim_Packet_CreatePacket(MAC_LAYER);
		pstruPacket->nControlDataType = CR_CONTROL_PACKET(MMM_UCS_NOTIFICATION);
		add_dest_to_packet(pstruPacket, pstruCPEMAC->nBSID);
		pstruPacket->nPacketPriority = Priority_High;
		pstruPacket->nPacketType = PacketType_Control;
		pstruPacket->nReceiverId = pstruCPEMAC->nBSID;
		pstruPacket->nSourceId = pstruEventDetails->nDeviceId;
		pstruPacket->nTransmitterId = pstruEventDetails->nDeviceId;
		pstruPacket->pstruMacData->dArrivalTime = pstruEventDetails->dEventTime;
		pstruPacket->pstruMacData->dOverhead = GMH_SIZE;
		pstruPacket->pstruMacData->dPacketSize = GMH_SIZE;
		pstruPacket->pstruMacData->nMACProtocol = MAC_PROTOCOL_IEEE802_22;
		pstruPacket->pstruMacData->Packet_MACProtocol = pstruGMH;
		pstruPacket->pstruMacData->szDestMac = DEVICE_HWADDRESS(pstruCPEMAC->nBSID,pstruCPEMAC->nBSInterface);
		pstruPacket->pstruMacData->szNextHopMac = DEVICE_HWADDRESS(pstruCPEMAC->nBSID,pstruCPEMAC->nBSInterface);
		pstruPacket->pstruMacData->szSourceMac = DEVICE_HWADDRESS(pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId);
		pstruGMH->n_UCS = 1;

		//Add the packet to mac queue
		pstruTempPacket = pstruCPEMAC->pstruQueuedPacketList[0];
		if(!pstruTempPacket)
		{
			pstruCPEMAC->pstruQueuedPacketList[0] = pstruPacket;
		}
		else
		{
			while(pstruTempPacket->pstruNextPacket)
				pstruTempPacket = pstruTempPacket->pstruNextPacket;
			pstruTempPacket->pstruNextPacket = pstruPacket;
		}
		//Add UCS to CR metrics
		pstruCPEMAC->struCPEMetrics.nUCSSent++;
	}
	fnpFreeMemory(input);
	fnpFreeMemory(output);
	return 1;
}
/** The Spectrum Sensing Function observes the RF spectrum of a television channel for
a set of signal types and reports the results of this observation. The spectrum sensing 
function is implemented in both the base station and the CPEs. There are MAC management 
frames that allow the base station to control the operation of the spectrum sensing function within each of the
CPEs. The inputs to the spectrum sensing function come from the SM via SSA. The use of any
specific sensing technique is optional, as long as the inputs, outputs and behavior meet 
the specification of this sub clause.

Note: For further details, please Refer IEEE 802.22-2011 Document Section 10.4.1 */
#define PD 0.9
struct stru_802_22_SSFOutput* fn_NetSim_CR_CPE_SSF(struct stru_802_22_SSFInput* input,NETSIM_ID nDevId,NETSIM_ID nInterfaceId)
{
	struct stru_802_22_SSFOutput* output = (struct stru_802_22_SSFOutput*)fnpAllocateMemory(1,sizeof *output);
	unsigned int nLoop;
	double dDistance;
	int nflag = 0;

	double p = fn_NetSim_Utilities_GenerateRandomNo(&DEVICE(nDevId)->ulSeed[0],
		&DEVICE(nDevId)->ulSeed[1])/NETSIM_RAND_MAX;
	if(p<(double)input->nMaxProbabilityOfFalseAlram/100.0)
		nflag = 1;

	for(nLoop=0;nLoop<input->nIncumbentCount;nLoop++)
	{
		if(input->pstruIncumbent[nLoop]->nIncumbentStatus == IncumbentStatus_OPERATIOAL)
		{
			dDistance = fn_NetSim_Utilities_CalculateDistance(DEVICE_POSITION(nDevId),input->pstruIncumbent[nLoop]->position);
			if(dDistance <= input->pstruIncumbent[nLoop]->dKeepOutDistance)
			{
				//Incumbent detected
				//check for possible interference
				if(fn_NetSim_Check_Interference(input->nChannelNumber,
					input->pstruChannelList,
					input->pstruIncumbent[nLoop]->nStartFrequeny,
					input->pstruIncumbent[nLoop]->nEndFrequency))
				{
					double p = fn_NetSim_Utilities_GenerateRandomNo(&DEVICE(nDevId)->ulSeed[0],
						&DEVICE(nDevId)->ulSeed[1])/NETSIM_RAND_MAX;
					if(p<(double)PD)
						nflag = 1;
				}
			}
		}
	}
	output->nSensingMode = input->nSensingMode;
	if(nflag)
	{
		output->nSignalPresentVector = 0xFF;
	}
	else
		output->nSignalPresentVector = 0x00;
	return output;
}
/** Urgent Coexistence Situation Used by the CPE to alert the BS about an UCS with 
incumbents in the channel currently being used by the BS or either of its adjacent channels:

	0: no incumbent (default) 
	1: incumbent detected  */
int fn_NetSim_CR_BS_UCS()
{
	NetSim_PACKET* pstruPacket = pstruEventDetails->pPacket;
	GMH* pstruGMH = (GMH*)pstruPacket->pstruMacData->Packet_MACProtocol;
	BS_MAC* pstruBSMAC = (BS_MAC*)DEVICE_MACVAR(pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId);
	BS_PHY* pstruBSPHY = (BS_PHY*)DEVICE_PHYVAR(pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId);
	struct stru_802_22_Channel* pstruChannelList;
	if(pstruGMH->n_UCS == 1 && !pstruBSMAC->nCHSREQFlag)
	{
		pstruBSMAC->nCHSREQFlag = ONE;
		pstruBSMAC->chsFrameCount = 2;
		pstruBSPHY->pstruOpratingChannel->nChannelState = ChannelState_PROTECTED;
		if(pstruBSPHY->pstruOpratingChannel->pstruNextChannel)
			pstruBSPHY->pstruOpratingChannel->pstruNextChannel->nChannelState = ChannelState_PROTECTED;
		//Find the first back up channel
		pstruChannelList = pstruBSPHY->pstruChannelSet;
		while(pstruChannelList)
		{
			if(pstruChannelList->nChannelState == ChannelState_BACKUP)
			{
				NetSim_PACKET* pstruCHS_REQ;
				//Switch to channel
				pstruBSPHY->pstruOpratingChannel = pstruChannelList;
				//Generate the channel switch message
				pstruCHS_REQ = fn_NetSim_CR_GenerateBroadcastCtrlPacket(pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId,MMM_CHS_REQ);
				//Add CHS-REQ to CR Metrics
				pstruBSMAC->struBSMetrics.nCHS_REQSent++;

				//Add packet to broadcast packet list
				if(pstruBSMAC->pstruBroadcastPDU)
				{
					NetSim_PACKET* pPacket = pstruBSMAC->pstruBroadcastPDU;
					while(pPacket->pstruNextPacket)
						pPacket = pPacket->pstruNextPacket;
					pPacket->pstruNextPacket = pstruCHS_REQ;
				}
				else
				{
					pstruBSMAC->pstruBroadcastPDU = pstruCHS_REQ;
				}
				fn_NetSim_CR_UpdateIncumbentMetrics(pstruBSMAC,pstruBSPHY,pstruEventDetails->dEventTime);
				break;
			}
			pstruChannelList = pstruChannelList->pstruNextChannel;
		}
		if(pstruChannelList == NULL)
		{
			pstruBSPHY->pstruOpratingChannel = NULL;
			fn_NetSim_CR_UpdateIncumbentMetrics(pstruBSMAC,pstruBSPHY,pstruEventDetails->dEventTime);
		}
	}
	return 1;
}
/** When the BS decides to switch channels during normal operation, it shall execute 
the following procedure to determine when to schedule the channel switching operation. 
		The BS selects the first backup channel from its backup/candidate channel list, 
it shall select a waiting time T46 to make sure that all its CPEs are prepared for the 
channel switch. The value of T46 is a configuration parameter that could be set by the 
management interface. The first requirement is the value of T46 shall be smaller or equal to
the maximum allowed channel moving time and the second requirement is that is long enough 
for the CPEs to recover from an incumbent detection.

Note: For further details, please Refer IEEE 802.22-2011 Document Section 7.22.2 */
int fn_NetSim_CR_CPE_SwitchChannel()
{
	CPE_MAC* pstruCPEMAC = (CPE_MAC*)DEVICE_MACVAR(pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId);
	CPE_PHY* pstruCPEPHY = (CPE_PHY*)DEVICE_PHYVAR(pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId);
	BS_PHY* pstruBSPHY = (BS_PHY*)DEVICE_PHYVAR(pstruCPEMAC->nBSID,pstruCPEMAC->nBSInterface);
	pstruCPEPHY->pstruOperatingChannel = pstruBSPHY->pstruOpratingChannel;
	//Add CHS-REQ to CR Metrics
	pstruCPEMAC->struCPEMetrics.nCHS_REQReceived++;
	return 1;
}
/** This function is used to update the operating channel as well as to update the
 event details  */
int fn_NetSim_CR_UpdateChannel()
{
	BS_MAC* pstruBSMAC = (BS_MAC*)DEVICE_MACVAR(pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId);
	BS_PHY* pstruBSPHY = (BS_PHY*)DEVICE_PHYVAR(pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId);
	//Initial scan the channel
	fn_NetSim_CR_ScanChannel(pstruBSMAC,pstruBSPHY);
	//Update the operating channel
	fn_NetSim_CR_UpdateOperatingChannel(pstruBSPHY);
	if(pstruBSPHY->pstruOpratingChannel)
	{
		fn_NetSim_CR_AssociateCPE(pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId);
		pstruEventDetails->dPacketSize = 0;
		pstruEventDetails->nApplicationId = 0;
		pstruEventDetails->nDeviceType = BASESTATION;
		pstruEventDetails->nEventType = TIMER_EVENT;
		pstruEventDetails->nPacketId = 0;
		pstruEventDetails->nProtocolId = MAC_PROTOCOL_IEEE802_22;
		pstruEventDetails->nSubEventType = TRANSMIT_SCH;
		pstruEventDetails->pPacket = NULL;
		pstruEventDetails->szOtherDetails = NULL;
		fnpAddEvent(pstruEventDetails);
	}
	else
	{
		pstruEventDetails->dEventTime += 1000000;
		fnpAddEvent(pstruEventDetails);
	}
	return 1;
}
