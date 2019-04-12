#include "main.h"
#include "802_22.h"
/** The MAC data elements that are contained in upstream bursts shall be mapped to the 
US sub frame in a different order. They are first mapped horizontally, OFDM symbol by
OFDM symbol, in the same logical sub channel. 

Once a logical sub channel has been filled to the end of the upstream sub frame, 
the balance of the MAC data elements shall be mapped to the next logical sub channel, 
in an increasing sub channel order. This process continues until all of the sub channels 
and symbols allocated to the burst are filled. If the quantity of MAC data elements is 
insufficient to fill an upstream burst so that an integer number of OFDMA slots is
occupied once encoded, zero padding shall be inserted at the end.

Alternatively, the horizontal laying of the MAC data elements may fill one sub channel 
with at least 7 OFDM symbols at a time and continue on the following sub channels. 
However, when all logical sub channels have been filled, the next MAC data elements 
shall be placed in the first available logical sub channel in the following burst. 
The width of the last vertical burst will be between 7 and 13 symbols depending on the 
total number of symbols in the upstream sub frame. */
int fn_NetSim_CR_FormUSBurst()
{
	double t=pstruEventDetails->dEventTime;
	int nLoop;
	double dSize[MAX_FID];
	double dTSize=0;
	NetSim_PACKET* pstruPacket,*pstruTempPacket;
	NETSIM_ID nDeviceId = pstruEventDetails->nDeviceId;
	NETSIM_ID nInterfaceId = pstruEventDetails->nInterfaceId;
	CPE_MAC* pstruCPEMAC = (CPE_MAC*)DEVICE_MACVAR(nDeviceId,nInterfaceId);
	for(nLoop=1;nLoop<MAX_FID;nLoop++)
	{
		dSize[nLoop] =0;
		pstruPacket = pstruCPEMAC->pstruQueuedPacketList[nLoop];
		if(pstruCPEMAC->pstruServiceParameter->SFID[nLoop]==0 && nLoop > 2)
			continue;
		while(pstruPacket)
		{
			dSize[nLoop] += fnGetPacketSize(pstruPacket);
			pstruPacket = pstruPacket->pstruNextPacket;
		}
		dTSize += dSize[nLoop];
	}
	if(dTSize > pstruCPEMAC->BWRequestInfo->dBytesAllocated)
	{
		double dAlloc = pstruCPEMAC->BWRequestInfo->dBytesAllocated;
		for(nLoop=1;nLoop<MAX_FID;nLoop++)
		{
			double d;
			BW_REQUEST* pstruBWRequest;
			d = dSize[nLoop];
			dSize[nLoop] -= dAlloc;
			if(dSize[nLoop] <= 0)
			{
				dAlloc -= d;
				continue;
			}
			//BW request needs to be sent
			pstruPacket = fn_NetSim_Packet_CreatePacket(MAC_LAYER);
			pstruPacket->nControlDataType = CR_CONTROL_PACKET(MMM_BW_REQUEST);
			add_dest_to_packet(pstruPacket, pstruCPEMAC->nBSID);
			pstruPacket->nPacketPriority = Priority_High;
			pstruPacket->nPacketType = PacketType_Control;
			pstruPacket->nReceiverId = pstruCPEMAC->nBSID;
			pstruPacket->nSourceId = nDeviceId;
			pstruPacket->nTransmitterId = nDeviceId;
			pstruPacket->nQOS = pstruCPEMAC->pstruQueuedPacketList[nLoop]->nQOS;
			pstruPacket->pstruMacData->nMACProtocol = MAC_PROTOCOL_IEEE802_22;
			pstruPacket->pstruMacData->dArrivalTime = pstruEventDetails->dEventTime;
			pstruPacket->pstruMacData->dPacketSize = 3;
			pstruPacket->pstruMacData->dOverhead = 3;
			pstruBWRequest = (BW_REQUEST*)fnpAllocateMemory(1,sizeof *pstruBWRequest);
			pstruBWRequest->nType = 0;
			pstruBWRequest->nBR = (int)(d);
			pstruPacket->pstruMacData->Packet_MACProtocol = pstruBWRequest;

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
			dAlloc = 0;
		}
	}
	if(pstruCPEMAC->BWRequestInfo->dBytesAllocated)
	{
		double dByte = pstruCPEMAC->BWRequestInfo->dBytesAllocated;
		int nId = 1;
		while(dByte && nId < 8)
		{
			NetSim_PACKET* pPacket;
			pPacket = pstruCPEMAC->pstruQueuedPacketList[nId];
			if(pPacket)
			{
				double size;
				size = fnGetPacketSize(pPacket);
				if(size <= dByte)
					dByte -= size;
				else
					break;
				pstruCPEMAC->pstruQueuedPacketList[nId] = pstruCPEMAC->pstruQueuedPacketList[nId]->pstruNextPacket;
				pPacket->pstruNextPacket = NULL;
				fn_NetSim_AddPacketToList(&(pstruCPEMAC->pstruUSBurst),pPacket);
			}
			else
				nId++;
		}
		//Add an event to transmit US-Burst
		pstruEventDetails->dEventTime = pstruCPEMAC->BWRequestInfo->dStartTime;
		pstruEventDetails->dPacketSize = 0;
		pstruEventDetails->nApplicationId = 0;
		pstruEventDetails->nEventType = TIMER_EVENT;
		pstruEventDetails->nPacketId = 0;
		pstruEventDetails->nProtocolId = MAC_PROTOCOL_IEEE802_22;
		pstruEventDetails->nSubEventType = TRANSMIT_US_BURST;
		pstruEventDetails->pPacket = NULL;
		fnpAddEvent(pstruEventDetails);

		
	}
	while(pstruCPEMAC->pstruQueuedPacketList[0])
	{
		NetSim_PACKET* p = pstruCPEMAC->pstruQueuedPacketList[0];
		pstruCPEMAC->pstruQueuedPacketList[0] = pstruCPEMAC->pstruQueuedPacketList[0]->pstruNextPacket;
		p->pstruNextPacket=NULL;
		//Generate physical out event
		pstruEventDetails->dEventTime = t;
		pstruEventDetails->dPacketSize = p->pstruMacData->dPacketSize;
		pstruEventDetails->nApplicationId = 0;
		pstruEventDetails->nEventType = PHYSICAL_OUT_EVENT;
		pstruEventDetails->nPacketId = 0;
		pstruEventDetails->nProtocolId = MAC_PROTOCOL_IEEE802_22;
		pstruEventDetails->nSubEventType = TRANSMIT_US_BURST_CONTROL;
		pstruEventDetails->pPacket = p;
		fnpAddEvent(pstruEventDetails);
	}
	return 0;
}
/** The MAC data elements that are contained in upstream bursts shall be mapped to the 
US sub frame in a different order. They are first mapped horizontally, OFDM symbol by
OFDM symbol, in the same logical sub channel. 

Once a logical sub channel has been filled to the end of the upstream sub frame, 
the balance of the MAC data elements shall be mapped to the next logical sub channel, 
in an increasing sub channel order. This process continues until all of the sub channels 
and symbols allocated to the burst are filled. If the quantity of MAC data elements is 
insufficient to fill an upstream burst so that an integer number of OFDMA slots is
occupied once encoded, zero padding shall be inserted at the end.

Alternatively, the horizontal laying of the MAC data elements may fill one sub channel 
with at least 7 OFDM symbols at a time and continue on the following sub channels. 
However, when all logical sub channels have been filled, the next MAC data elements 
shall be placed in the first available logical sub channel in the following burst. 
The width of the last vertical burst will be between 7 and 13 symbols depending on the 
total number of symbols in the upstream sub frame. */
int fn_NetSim_CR_TransmitUSBurst()
{
	NETSIM_ID nDeviceId = pstruEventDetails->nDeviceId;
	NETSIM_ID nInterfaceId = pstruEventDetails->nInterfaceId;
	CPE_MAC* pstruCPEMac = (CPE_MAC*)DEVICE_MACVAR(nDeviceId,nInterfaceId);
	NetSim_PACKET* pstruPacket = pstruCPEMac->pstruUSBurst;
	if(!pstruPacket)
		return 1; //No more US burst
	pstruCPEMac->pstruUSBurst = pstruCPEMac->pstruUSBurst->pstruNextPacket;
	pstruPacket->pstruNextPacket = NULL;
	//Update the MAC data
	pstruPacket->pstruMacData->dEndTime = pstruEventDetails->dEventTime;
	pstruPacket->pstruMacData->dStartTime = pstruEventDetails->dEventTime;
	//Create phy out event
	pstruEventDetails->dPacketSize = fnGetPacketSize(pstruPacket);
	if (pstruPacket->pstruAppData)
	{
		pstruEventDetails->nApplicationId = pstruPacket->pstruAppData->nApplicationId;
		pstruEventDetails->nSegmentId = pstruPacket->pstruAppData->nSegmentId;
	}
	else
	{
		pstruEventDetails->nApplicationId = 0;
		pstruEventDetails->nSegmentId = 0;
	}
	pstruEventDetails->nEventType = PHYSICAL_OUT_EVENT;
	pstruEventDetails->nPacketId = pstruPacket->nPacketId;
	pstruEventDetails->nSubEventType = 0;
	pstruEventDetails->pPacket = pstruPacket;
	fnpAddEvent(pstruEventDetails);
	/* Check for Application end flag */
	if(pstruPacket->pstruAppData && pstruPacket->pstruAppData->nAppEndFlag == 1)
	{
		//Application is ending
		fn_NetSim_TerminateServiceFlow(nDeviceId,nInterfaceId,pstruPacket);
	}
	return 0;
}
