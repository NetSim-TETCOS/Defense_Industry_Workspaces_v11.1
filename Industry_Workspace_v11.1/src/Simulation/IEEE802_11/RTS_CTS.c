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

static double get_RTS_CTS_Time(PIEEE802_11_MAC_VAR mac, PIEEE802_11_PHY_VAR phy)
{
	return 2*phy->plmeCharacteristics.aSIFSTime
		+ get_preamble_time(phy)
		+ ((getCTSSize() * 8) / phy->dControlFrameDataRate);
}

double calculate_RTS_duration(NETSIM_ID txId,
							  NETSIM_ID txIf,
							  NetSim_PACKET* packet)
{
	NETSIM_ID rxId = packet->nReceiverId;
	NETSIM_ID rxIf = fn_NetSim_Stack_GetConnectedInterface(txId, txIf, rxId);

	PIEEE802_11_PHY_VAR phy = IEEE802_11_PHY(txId, txIf);
	fn_NetSim_IEEE802_11_SetDataRate(txId, txIf, rxId, rxIf, packet, pstruEventDetails->dEventTime);
	double packetTime = fn_NetSim_IEEE802_11_CalculateTransmissionTime(packet->pstruMacData->dPayload +
																	   getMacOverhead(phy,
																					  packet->pstruMacData->dPayload),
																	   txId,
																	   txIf);
	double ctsTime = (getCTSSize() * 8.0) / phy->dControlFrameDataRate;
	double ackTime = (getAckSize(phy) * 8.0) / phy->dControlFrameDataRate;
	double rtsTime = (getRTSSize()*8.0) / phy->dBroadcastFrameDataRate;

	return phy->plmeCharacteristics.aSIFSTime +
		get_preamble_time(phy) +
		ctsTime +
		phy->plmeCharacteristics.aSIFSTime +
		get_preamble_time(phy) +
		packetTime +
		phy->plmeCharacteristics.aSIFSTime +
		get_preamble_time(phy) +
		ackTime;
}

double calculate_CTS_duration(NETSIM_ID d,
							  NETSIM_ID i,
							  double rtsduration)
{
	PIEEE802_11_PHY_VAR phy = IEEE802_11_PHY(d, i);
	double ctsTime = (getCTSSize() * 8.0) / phy->dControlFrameDataRate;
	return rtsduration -
		(phy->plmeCharacteristics.aSIFSTime +
		 get_preamble_time(phy) +
		 ctsTime);
}

static bool decide_RTS_CTS_Mechanism()
{
	double d=0;
	NetSim_PACKET* p=IEEE802_11_CURR_MAC->currentProcessingPacket;
	
	if (isBroadcastPacket(p) || isMulticastPacket(p))
		return false; //Broadcast or multicast packet

	if (isIEEE802_11_CtrlPacket(p))
		return false; //No RTS CTS for IEEE802.11 control packet
	while(p)
	{
		d+=p->pstruMacData->dPayload;
		p=p->pstruNextPacket;
	}
	return (d >= IEEE802_11_CURR_MAC->nRTSThreshold);
}

void fn_NetSim_IEEE802_11_RTS_CTS_Init()
{
	PIEEE802_11_MAC_VAR mac = IEEE802_11_CURR_MAC;

	if(!decide_RTS_CTS_Mechanism())
		return; //NO RTS-CTS

	mac->waitingforCTS=mac->currentProcessingPacket;
	double  d = calculate_RTS_duration(pstruEventDetails->nDeviceId,
									   pstruEventDetails->nInterfaceId,
									   mac->currentProcessingPacket);
	mac->currentProcessingPacket = fn_NetSim_IEEE802_11_CreateRTSPacket(mac->waitingforCTS, d);

	mac->metrics.nRTSSentCount++;

	//Update the packet processing time
	mac->dPacketProcessingEndTime += get_RTS_CTS_Time(mac,IEEE802_11_CURR_PHY);

}

void fn_NetSim_IEEE802_11_RTS_CTS_ProcessRTS()
{
	pstruEventDetails->dEventTime += IEEE802_11_CURR_PHY->SIFS;
	pstruEventDetails->nSubEventType = SEND_CTS;
	pstruEventDetails->nEventType = MAC_OUT_EVENT;
	fnpAddEvent(pstruEventDetails);
	IEEE802_11_Change_Mac_State(IEEE802_11_CURR_MAC,IEEE802_11_MACSTATE_TXing_CTS);

	IEEE802_11_CURR_MAC->metrics.nRTSReceivedCount++;
}

void fn_NetSim_IEEE802_11_RTS_CTS_SendCTS()
{
	NetSim_PACKET* packet=fn_NetSim_IEEE802_11_CreateCTSPacket(pstruEventDetails->pPacket);
	//Free the RTS packet
	fn_NetSim_Packet_FreePacket(pstruEventDetails->pPacket);

	//Send CTS to phy
	pstruEventDetails->dPacketSize = packet->pstruMacData->dPacketSize;
	pstruEventDetails->nSubEventType = 0;
	pstruEventDetails->nEventType = PHYSICAL_OUT_EVENT;
	pstruEventDetails->pPacket = packet;
	fnpAddEvent(pstruEventDetails);	

	IEEE802_11_CURR_MAC->metrics.nCTSSentCount++;
}

void fn_NetSim_IEEE802_11_RTS_CTS_AddCTSTimeOut(NetSim_PACKET* packet,NETSIM_ID devId,NETSIM_ID devIf)
{
	NetSim_EVENTDETAILS pevent;
	PIEEE802_11_PHY_VAR phy = IEEE802_11_PHY(devId,devIf);
	double ctstime;

	ctstime = ceil(packet->pstruPhyData->dStartTime
		+ phy->plmeCharacteristics.aSIFSTime
		+ get_preamble_time(phy)
		+ ((getCTSSize() * 8)/phy->dControlFrameDataRate));

	pevent.dEventTime=ctstime+2;
	pevent.dPacketSize=0;
	pevent.nDeviceId=devId;
	pevent.nDeviceType=DEVICE_TYPE(devId);
	pevent.nEventType=TIMER_EVENT;
	pevent.nInterfaceId=devIf;
	pevent.nPacketId=packet->nPacketId;
	pevent.nProtocolId=MAC_PROTOCOL_IEEE802_11;
	if(packet->pstruAppData)
	{
		pevent.nSegmentId=packet->pstruAppData->nSegmentId;
		pevent.nApplicationId=packet->pstruAppData->nApplicationId;
	}
	else
	{
		pevent.nSegmentId=0;
		pevent.nApplicationId=0;
	}
	pevent.nSubEventType=CTS_TIMEOUT;
	pevent.pPacket=NULL;
	pevent.szOtherDetails=NULL;
	fnpAddEvent(&pevent);
}

void fn_NetSim_IEEE802_11_RTS_CTS_CTSTimeOut()
{
	PIEEE802_11_MAC_VAR mac = IEEE802_11_CURR_MAC;
	if(mac->currMacState == IEEE802_11_MACSTATE_Wait_CTS)
	{
		IEEE802_11_Change_Mac_State(mac,IEEE802_11_MACSTATE_MAC_IDLE);
		if(fn_NetSim_IEEE802_11_CSMACA_CheckRetryLimit(mac))
		{
			mac->nRetryCount++;
			fn_NetSim_IEEE802_11_CSMACA_IncreaseCW(mac);
			fn_NetSim_IEEE802_11_CSMACA_Init();
		}
		else
		{
			if(mac->waitingforCTS)
			{
				NetSim_PACKET* pstruPacket = mac->waitingforCTS;

				if(pstruPacket->DropNotification)
					pstruPacket->DropNotification(pstruPacket);
				fn_NetSim_Packet_FreePacket(mac->currentProcessingPacket);
				fn_NetSim_Packet_FreePacket(mac->waitingforCTS);
				mac->currentProcessingPacket=NULL;
				mac->waitingforCTS=NULL;
			}
			mac->nRetryCount = 0;
			fn_NetSim_IEEE802_11_CSMACA_Init();
		}
	}
}

void fn_NetSim_IEEE802_11_RTS_CTS_ProcessCTS()
{
	PIEEE802_11_MAC_VAR mac=IEEE802_11_CURR_MAC;
	fn_NetSim_Packet_FreePacket(mac->currentProcessingPacket);
	mac->currentProcessingPacket=mac->waitingforCTS;
	mac->waitingforCTS=NULL;
	mac->metrics.nCTSReceivedCount++;
	//Free CTS packet
	fn_NetSim_Packet_FreePacket(pstruEventDetails->pPacket);
	pstruEventDetails->pPacket=NULL;
	
	pstruEventDetails->dEventTime += IEEE802_11_CURR_PHY->SIFS;
	pstruEventDetails->nSubEventType = SEND_MPDU;
	pstruEventDetails->nEventType = MAC_OUT_EVENT;
	if(mac->currentProcessingPacket->pstruAppData)
	{
		pstruEventDetails->nApplicationId = mac->currentProcessingPacket->pstruAppData->nApplicationId;
		pstruEventDetails->nSegmentId = mac->currentProcessingPacket->pstruAppData->nSegmentId;
	}
	else
	{
		pstruEventDetails->nApplicationId = 0;
		pstruEventDetails->nSegmentId = 0;
	}
	pstruEventDetails->nPacketId = mac->currentProcessingPacket->nPacketId;
	fnpAddEvent(pstruEventDetails);
	IEEE802_11_Change_Mac_State(IEEE802_11_CURR_MAC,IEEE802_11_MACSTATE_TXing_MPDU);
}
