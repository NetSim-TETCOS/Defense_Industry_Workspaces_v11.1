#include "main.h"
#include "802_22.h"
/** The Superframe Control header decoding is critical, so the SCH shall be transmitted using
the modulation technique. The length of SCH shall be one OFDM symbol. The
SCH shall be decoded by all the CPEs associated with that BS . 
The SCH provides information about the IEEE 802.22 cell, in order to protect incumbents,
support self-coexistence mechanisms, and support the intra-frame and inter-frame mechanisms 
for management of quiet periods for sensing. Transmission of a SCH indicates that 
the WRAN cell is operating in one of the two possible modes: normal mode or self-coexistence mode. */
int fn_NetSim_CR_TransmitSCH()
{
	double dTime = pstruEventDetails->dEventTime;
	BS_MAC* pstruBSMAC;
	BS_PHY* pstruBSPhy;
	SCH* pstruSCH;
	NetSim_PACKET* pstruSCHPacket;
	PNETSIM_MACADDRESS szBSMAC = NETWORK->ppstruDeviceList[pstruEventDetails->nDeviceId-1]->ppstruInterfaceList[pstruEventDetails->nInterfaceId-1]->pstruMACLayer->szMacAddress;
	pstruBSMAC = (BS_MAC*)NETWORK->ppstruDeviceList[pstruEventDetails->nDeviceId-1]->ppstruInterfaceList[pstruEventDetails->nInterfaceId-1]->pstruMACLayer->MacVar;
	pstruBSPhy = (BS_PHY*)NETWORK->ppstruDeviceList[pstruEventDetails->nDeviceId-1]->ppstruInterfaceList[pstruEventDetails->nInterfaceId-1]->pstruPhysicalLayer->phyVar;

	if(pstruBSPhy->pstruOpratingChannel == NULL)
	{
		//Add an event to check for channel
		pstruEventDetails->dEventTime += 0;
		pstruEventDetails->nSubEventType = SM_UPDATECHANNEL;
		fnpAddEvent(pstruEventDetails);
		return 0; //No channel found
	}
	//Increment the superframe number in BS Phy
	pstruBSPhy->nSuperframeNumber++;
	pstruBSPhy->nFrameNumber = 0;
	pstruBSMAC->dSuperframeStartTime = pstruEventDetails->dEventTime;
	pstruBSMAC->dFrameStartTime = pstruEventDetails->dEventTime;
	//Generate the SCH packet
	pstruSCHPacket = fn_NetSim_Packet_CreatePacket(MAC_LAYER);
	pstruSCHPacket->dEventTime = pstruEventDetails->dEventTime;
	pstruSCHPacket->nControlDataType = CR_CONTROL_PACKET(MMM_SCH);
	add_dest_to_packet(pstruSCHPacket, 0); //Broadcast packet
	pstruSCHPacket->nPacketPriority = Priority_High;
	pstruSCHPacket->nPacketType = PacketType_Control;
	pstruSCHPacket->nServiceType = ServiceType_CBR;
	pstruSCHPacket->nTransmitterId = pstruEventDetails->nDeviceId;
	pstruSCHPacket->nSourceId = pstruEventDetails->nDeviceId;
	//Fill Mac layer details
	pstruSCHPacket->pstruMacData->dArrivalTime = dTime + 2*pstruBSPhy->pstruSymbolParameter->dSymbolDuration;
	pstruSCHPacket->pstruMacData->dEndTime = dTime + 2*pstruBSPhy->pstruSymbolParameter->dSymbolDuration;
	pstruSCHPacket->pstruMacData->dOverhead = SCH_SIZE;
	pstruSCHPacket->pstruMacData->dPacketSize = SCH_SIZE;
	pstruSCHPacket->pstruMacData->dStartTime = dTime + 2*pstruBSPhy->pstruSymbolParameter->dSymbolDuration;
	pstruSCHPacket->pstruMacData->nMACProtocol = MAC_PROTOCOL_IEEE802_22;
	pstruSCHPacket->pstruMacData->szDestMac = BROADCAST_MAC;
	pstruSCHPacket->pstruMacData->szSourceMac = szBSMAC;

	//Update SCH header
	pstruSCH = fnpAllocateMemory(1,sizeof *pstruSCH);
	pstruSCH->n_SuperframeNumber = pstruBSPhy->nSuperframeNumber;
	pstruSCH->sz_BS_Id = szBSMAC;
	pstruSCH->n_FrameAllocationMap = B16_1111111111111111;
	pstruSCH->n_CP = pstruBSPhy->nCP;
	pstruSCH->n_FCH_EncodingFlag = B2_11;
	pstruSCH->n_Self_coexistence_Capability_Indicator = B4_0000;
	pstruSCH->n_MAC_version = 0x01;

	//Call Spectrum manager to schedule quiet period
	fn_Netsim_CR_SM_ScheduleQuietPeriod(pstruBSPhy,pstruSCH);

	pstruSCH->n_SCW_Cycle_Frame_Bitmap = 0;
	pstruSCH->n_SCW_Cycle_Length = 0;
	pstruSCH->n_SCW_Cycle_Offset = 0;
	pstruSCH->n_Current_DS_US_Split = 0;
	pstruSCH->n_Claimed_US_DS_Split = 0;
	pstruSCH->n_DS_US_Change_Offset = 0;
	pstruSCH->n_HCS = 0;
	pstruSCH->n_Padding_bits = 0;

	//Add SCH header to SCH packet
	pstruSCHPacket->pstruMacData->Packet_MACProtocol = pstruSCH;
	//Create an event for phy out
	pstruEventDetails->dEventTime = dTime + 2*pstruBSPhy->pstruSymbolParameter->dSymbolDuration;
	pstruEventDetails->dPacketSize = SCH_SIZE;
	pstruEventDetails->nEventType = PHYSICAL_OUT_EVENT;
	pstruEventDetails->nSubEventType = 0;
	pstruEventDetails->pPacket = pstruSCHPacket;
	pstruEventDetails->szOtherDetails = NULL;
	//Add physical out event
	fnpAddEvent(pstruEventDetails);
	//Schedule next superfame event
	pstruEventDetails->dEventTime = dTime+SCH_DURATION;
	pstruEventDetails->dPacketSize = 0;
	pstruEventDetails->nEventType = TIMER_EVENT;
	pstruEventDetails->nSubEventType = TRANSMIT_SCH;
	pstruEventDetails->pPacket = NULL;
	fnpAddEvent(pstruEventDetails);

	//Add to metrics
	pstruBSMAC->struBSMetrics.nSCHSent++;
	return 1;
}
/** The Superframe Control header decoding is critical, so the SCH shall be transmitted using
the modulation technique. The length of SCH shall be one OFDM symbol. The
SCH shall be decoded by all the CPEs associated with that BS . 
The SCH provides information about the IEEE 802.22 cell, in order to protect incumbents,
support self-coexistence mechanisms, and support the intra-frame and inter-frame mechanisms 
for management of quiet periods for sensing. Transmission of a SCH indicates that 
the WRAN cell is operating in one of the two possible modes: normal mode or self-coexistence mode. */
int fn_NetSim_CR_CPE_ProcessSCH(NetSim_PACKET* pstruPacket)
{
	CPE_MAC* pstruCPEMAC = DEVICE_MACVAR(pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId);
	CPE_PHY* pstruCPEPhy = DEVICE_PHYVAR(pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId);
	SCH* pstruSCH = pstruPacket->pstruMacData->Packet_MACProtocol;
	if(pstruSCH->n_Current_Intra_frame_Quiet_Period_Cycle_Length && !pstruSCH->n_Current_Intra_frame_Quiet_Period_Cycle_Offset)
	{
		pstruCPEPhy->nIntraFrameQuietPeriodLength = pstruSCH->n_Current_Intra_frame_Quiet_Period_Cycle_Length;
		strcpy(pstruCPEPhy->szIntraFrameQuietPeriodBitmap,pstruSCH->sz_Current_Intra_frame_Quiet_period_Cycle_Frame_Bitmap);
		pstruCPEPhy->nIntraFrameQuietPeriodDuration = pstruSCH->n_Current_Intra_frame_Quiet_Period_Duration;
	}
	else
	{
		pstruCPEPhy->nIntraFrameQuietPeriodLength = 0;
	}
	pstruCPEPhy->nFrameNumber = 0;
	//Add to metrics
	pstruCPEMAC->struCPEMetrics.nSCHReceived++;
	return 1;
}
