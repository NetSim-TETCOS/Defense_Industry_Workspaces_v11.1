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
#include "LTE.h"
#pragma comment(lib,"List.lib")

//Function prototype
static int fn_NetSim_LTE_FormDownlinkFrame_RoundRobin(NETSIM_ID enbId,NETSIM_ID enbInterface,unsigned int carrier_id,unsigned int nSSId);


int fn_NetSim_LTE_InitQueue(NETSIM_ID nDeviceId,NETSIM_ID nInterfaceId)
{
	switch(NETWORK->ppstruDeviceList[nDeviceId-1]->nDeviceType)
	{
	case UE:
	case RELAY:
		{
			LTE_UE_MAC* UEMac=(LTE_UE_MAC*)DEVICE_MACVAR(nDeviceId,nInterfaceId);
			if(!UEMac)
			{
				UEMac=(LTE_UE_MAC*)calloc(1,sizeof* UEMac);
				DEVICE_MACVAR(nDeviceId,nInterfaceId)=UEMac;
			}
			UEMac->GBRQueue=(LTE_QUEUE*)calloc(1,sizeof* UEMac->GBRQueue);
			UEMac->nonGBRQueue=(LTE_QUEUE*)calloc(1,sizeof* UEMac->nonGBRQueue);

			UEMac->HARQBuffer = alloc_HARQ();
		}
		break;
	default:
		fnNetSimError("Unknown device type for LTE-InitQueue");
		break;
	}
	return 1;
}
int fn_NetSim_LTE_AddInQueue()
{
	while(fn_NetSim_GetBufferStatus(DEVICE_INTERFACE(pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId)->pstruAccessInterface->pstruAccessBuffer))
	{
		NetSim_PACKET* packet=fn_NetSim_Packet_GetPacketFromBuffer(DEVICE_INTERFACE(pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId)->pstruAccessInterface->pstruAccessBuffer,1);
		LTE_QUEUE* queue;
		LTE_UE_MAC* UEMac=(LTE_UE_MAC*)DEVICE_MACVAR(pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId);

		//RRC protocol
		if(UEMac->rrcState==RRC_IDLE)
			fn_NetSim_LTE_InitRRCConnection();

		if(packet->pstruNetworkData)
			//Call pdcp protocol
			fn_NetSim_LTE_PDCP_init(packet);

		if(packet->nQOS==QOS_UGS)
			queue=UEMac->GBRQueue;
		else
			queue=UEMac->nonGBRQueue;
		queue->nSize+=(unsigned int)fnGetPacketSize(packet);
		if(queue->head)
		{
			queue->tail->pstruNextPacket=packet;
			queue->tail=packet;
		}
		else
		{
			queue->head=packet;
			queue->tail=packet;
		}
	}
	return 1;
}
int fn_NetSim_LTE_eNB_AddInQueue()
{
	while(fn_NetSim_GetBufferStatus(DEVICE_INTERFACE(pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId)->pstruAccessInterface->pstruAccessBuffer))
	{
		NetSim_PACKET* packet=fn_NetSim_Packet_GetPacketFromBuffer(DEVICE_INTERFACE(pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId)->pstruAccessInterface->pstruAccessBuffer,1);
		LTE_QUEUE* queue;
		LTE_ENB_MAC* enbMac=(LTE_ENB_MAC*)DEVICE_MACVAR(pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId);
		LTE_ASSOCIATEUE_INFO* info = fn_NetSim_LTE_FindInfo(enbMac, get_first_dest_from_packet(packet));
		packet->pstruPhyData->dPayload=0;
		packet->pstruPhyData->dOverhead=0;
		packet->pstruPhyData->dPacketSize=0;

		if(isBroadcastPacket(packet) ||
		   isMulticastPacket(packet))
			return fn_NetSim_Packet_FreePacket(packet);

		if(!info)
		{
			fnNetSimError("unknown destined packet reaches to mac out of LTE-eNB");
			fn_NetSim_Packet_FreePacket(packet);
			return 0;
		}
		//RRC protocol
		if(info->rrcState==RRC_IDLE)
			fn_NetSim_Init_PagingRequest(pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId,info);

		//Call pdcp protocol
		fn_NetSim_LTE_PDCP_init(packet);

		if(packet->nQOS==QOS_UGS)
			queue=info->GBRQueue;
		else
			queue=info->nonGBRQueue;
		queue->nSize+=(unsigned int)fnGetPacketSize(packet);
		if(queue->head)
		{
			queue->tail->pstruNextPacket=packet;
			queue->tail=packet;
		}
		else
		{
			queue->head=packet;
			queue->tail=packet;
		}
	}
	return 1;
}
int fn_NetSim_LTE_FormNextFrame()
{
	NETSIM_ID enbId=pstruEventDetails->nDeviceId;
	NETSIM_ID enbInterface=pstruEventDetails->nInterfaceId;
	LTE_ENB_PHY* enbPhy=(LTE_ENB_PHY*)DEVICE_PHYVAR(enbId,enbInterface);
	//Add next event
	pstruEventDetails->dEventTime+=enbPhy->dSubFrameDuration*MILLISECOND;
	fnpAddEvent(pstruEventDetails);
	pstruEventDetails->dEventTime-=enbPhy->dSubFrameDuration*MILLISECOND;

	fn_NetSim_LTE_A_AllocateSS(enbId,enbInterface);
	return 1;
}
int fn_NetSim_LTE_FormUPlinkFrame(NETSIM_ID enbId,NETSIM_ID enbInterface,unsigned int carrier_id,unsigned int nSSId)
{
	LTE_ENB_MAC* enbMac=(LTE_ENB_MAC*)DEVICE_MACVAR(enbId,enbInterface);
	LTE_ASSOCIATEUE_INFO* info=enbMac->associatedUEInfo;
	//Form up link queue
	//Form HARQ queue
	while(info)
	{
		if(info->nSSId==nSSId)
			fn_NetSim_LTE_HARQ_RetransmitHARQBuffer_UL(info,enbMac,enbId,carrier_id);
		info=(LTE_ASSOCIATEUE_INFO*)LIST_NEXT(info);
	}
	info=enbMac->associatedUEInfo;

	while(info)
	{
		if(info->nSSId==nSSId)
			fn_NetSim_D2D_Transmit_UL(info,enbMac,enbId,carrier_id,nSSId);
		info=(LTE_ASSOCIATEUE_INFO*)LIST_NEXT(info);
	}
	info=enbMac->associatedUEInfo;

	//From GBR Queue
	while(info)
	{
		NETSIM_ID ueId=info->nUEId;
		NETSIM_ID ueInterface=info->nUEInterface;
		LTE_UE_MAC* ueMac=(LTE_UE_MAC*)DEVICE_MACVAR(ueId,ueInterface);
		if((ueMac->carrier_id==-1 || ueMac->carrier_id==(int)carrier_id) &&
			ueMac->nSSId == nSSId)
		{
			if(ueMac->rrcState==RRC_CONNECTED && ueMac->GBRQueue->nSize)
			{
				unsigned int index=0;
				unsigned int size=ueMac->GBRQueue->nSize;

				unsigned int nTBSIndex_UL=info->ULInfo[carrier_id].TBSIndex;
				unsigned int nRBGcount,flag=0;
				size=size*8;
				//calculate the RB count required
				while(index<LTE_NRB_MAX)
				{
					if ((MODE_INDEX_MAPPING[info->TransmissionIndex.UL].TRANSMISSION_MODE == SingleUser_MIMO_Spatial_Multiplexing
						&& info->ULInfo[carrier_id].modulation== Modulation_64_QAM)) 
					{
						switch (info->ULInfo[carrier_id].Rank)
						{
						case 2:
							if(size<TransportBlockSize2[nTBSIndex_UL][index])
								flag=1;
							break;
						case 3:
							if(size<TransportBlockSize3[nTBSIndex_UL][index])
								flag=1;
							break;
						case 4:
							if(size<TransportBlockSize4[nTBSIndex_UL][index])
								flag=1;
							break;
						}
					}
					else if(size<TransportBlockSize1[nTBSIndex_UL][index])
					{
						flag=1;
					}
					if(flag==1)
						break;
					index++;
				}
				if(index>=LTE_NRB_MAX)
					index=LTE_NRB_MAX-1;
				nRBGcount=(int)ceil((double)(index+1)/enbMac->ca_mac[carrier_id].nRBCountInGroup);
				if(nRBGcount>enbMac->ca_mac[carrier_id].nRBGCount-enbMac->ca_mac[carrier_id].nAllocatedRBG)
					nRBGcount=enbMac->ca_mac[carrier_id].nRBGCount-enbMac->ca_mac[carrier_id].nAllocatedRBG;
				ueMac->GBRQueue->nRBStart=enbMac->ca_mac[carrier_id].nAllocatedRBG;
				ueMac->GBRQueue->nRBLength=nRBGcount*enbMac->ca_mac[carrier_id].nRBCountInGroup;
				if ((MODE_INDEX_MAPPING[info->TransmissionIndex.UL].TRANSMISSION_MODE == SingleUser_MIMO_Spatial_Multiplexing
					&& info->ULInfo[carrier_id].modulation== Modulation_64_QAM)) 
				{
					switch (info->ULInfo[carrier_id].Rank)
					{
					case 2:
						ueMac->GBRQueue->nBitcount=TransportBlockSize2[nTBSIndex_UL][ueMac->GBRQueue->nRBLength-1];
						break;
					case 3:
						ueMac->GBRQueue->nBitcount=TransportBlockSize3[nTBSIndex_UL][ueMac->GBRQueue->nRBLength-1];
						break;
					case 4:
						ueMac->GBRQueue->nBitcount=TransportBlockSize4[nTBSIndex_UL][ueMac->GBRQueue->nRBLength-1];
						break;
					}
				}
				else
					ueMac->GBRQueue->nBitcount=TransportBlockSize1[nTBSIndex_UL][ueMac->GBRQueue->nRBLength-1];

				enbMac->ca_mac[carrier_id].nAllocatedRBG+=nRBGcount;

				//Form RLC SDU
				fn_NetSim_LTE_RLC_FormRLCSDU(ueMac->GBRQueue,ueId,ueMac->nENBId,ueMac->HARQBuffer,carrier_id,nSSId);
				if(ueMac->GBRQueue->nRBLength)
					ueMac->carrier_id=carrier_id;
			}
		}
		info=(LTE_ASSOCIATEUE_INFO*)LIST_NEXT(info);
	}        
	//Form non GBR queue
	info=enbMac->associatedUEInfo;
	while(info)
	{
		NETSIM_ID ueId=info->nUEId;
		NETSIM_ID ueInterface=info->nUEInterface;
		LTE_UE_MAC* ueMac=(LTE_UE_MAC*)DEVICE_MACVAR(ueId,ueInterface);

		if((ueMac->carrier_id==-1 || ueMac->carrier_id==(int)carrier_id) &&
			ueMac->nSSId == nSSId)
		{
			if(ueMac->rrcState==RRC_CONNECTED && ueMac->nonGBRQueue->nSize)
			{
				unsigned int index=0;
				unsigned int size=ueMac->nonGBRQueue->nSize;
				unsigned int nTBSIndex_UL=info->ULInfo[carrier_id].TBSIndex;
				unsigned int nRBGcount,flag=0;

				LTE_ASSOCIATEUE_INFO newInfo;
				NETSIM_ID peer_id;
				fn_NetSim_D2D_GetInfo_UL(ueId,carrier_id,ueMac->nENBId,
					ueMac->nENBInterface,info,&newInfo,&peer_id);

				nTBSIndex_UL=newInfo.ULInfo[carrier_id].TBSIndex;

				size=size*8;

				while(index<LTE_NRB_MAX)
				{
					if ((MODE_INDEX_MAPPING[info->TransmissionIndex.UL].TRANSMISSION_MODE == SingleUser_MIMO_Spatial_Multiplexing
						&& info->ULInfo[carrier_id].modulation== Modulation_64_QAM))
					{
						switch (info->ULInfo[carrier_id].Rank)
						{
						case 2:
							if(size<TransportBlockSize2[nTBSIndex_UL][index])
								flag=1;
							break;
						case 3:
							if(size<TransportBlockSize3[nTBSIndex_UL][index])
								flag=1;
							break;
						case 4:
							if(size<TransportBlockSize4[nTBSIndex_UL][index])
								flag=1;
							break;
						}
					}
					else if(size<TransportBlockSize1[nTBSIndex_UL][index])
						flag=1;

					if(flag==1)
						break;
					index++;
				}
				if(index>=LTE_NRB_MAX)
					index=LTE_NRB_MAX-1;
				nRBGcount=(int)ceil((double)(index+1)/enbMac->ca_mac[carrier_id].nRBCountInGroup);
				if(nRBGcount>enbMac->ca_mac[carrier_id].nRBGCount-enbMac->ca_mac[carrier_id].nAllocatedRBG)
					nRBGcount=enbMac->ca_mac[carrier_id].nRBGCount-enbMac->ca_mac[carrier_id].nAllocatedRBG;
				ueMac->nonGBRQueue->nRBStart=enbMac->ca_mac[carrier_id].nAllocatedRBG;
				ueMac->nonGBRQueue->nRBLength=nRBGcount*enbMac->ca_mac[carrier_id].nRBCountInGroup;
				if(ueMac->nonGBRQueue->nRBLength)
				{
					if ((MODE_INDEX_MAPPING[info->TransmissionIndex.UL].TRANSMISSION_MODE == SingleUser_MIMO_Spatial_Multiplexing &&
						info->ULInfo[carrier_id].modulation== Modulation_64_QAM))
					{
						switch (info->ULInfo[carrier_id].Rank)
						{
						case 2: ueMac->nonGBRQueue->nBitcount=TransportBlockSize2[nTBSIndex_UL][ueMac->nonGBRQueue->nRBLength-1];
							break;
						case 3: ueMac->nonGBRQueue->nBitcount=TransportBlockSize3[nTBSIndex_UL][ueMac->nonGBRQueue->nRBLength-1];
							break;
						case 4: ueMac->nonGBRQueue->nBitcount=TransportBlockSize4[nTBSIndex_UL][ueMac->nonGBRQueue->nRBLength-1];
							break;
						}
					}
					else 
						ueMac->nonGBRQueue->nBitcount=TransportBlockSize1[nTBSIndex_UL][ueMac->nonGBRQueue->nRBLength-1];
					enbMac->ca_mac[carrier_id].nAllocatedRBG+=nRBGcount;

					//Form RLC SDU
					fn_NetSim_LTE_RLC_FormRLCSDU(ueMac->nonGBRQueue,ueId,peer_id,ueMac->HARQBuffer,carrier_id,nSSId);
					ueMac->carrier_id=carrier_id;
				}
			}
		}
		info=(LTE_ASSOCIATEUE_INFO*)LIST_NEXT(info);
	}
	return 1;
}

int CompareGBR(void* info1,void* info2)
{
	LTE_ASSOCIATEUE_INFO* if1 = (LTE_ASSOCIATEUE_INFO*)info1;
	LTE_ASSOCIATEUE_INFO* if2 = (LTE_ASSOCIATEUE_INFO*)info2;
	if(if1->GBRQueue->dRatio < if2->GBRQueue->dRatio)
		return 1;
	else
		return 0;
}


int CompareNonGBR(void* info1,void* info2)
{
	LTE_ASSOCIATEUE_INFO* if1 = (LTE_ASSOCIATEUE_INFO*)info1;
	LTE_ASSOCIATEUE_INFO* if2 = (LTE_ASSOCIATEUE_INFO*)info2;
	if(if1->nonGBRQueue->dRatio < if2->nonGBRQueue->dRatio)
		return 1;
	else
		return 0;
}

static int fn_NetSim_LTE_FormDownlinkFrame_RoundRobin(NETSIM_ID enbId,NETSIM_ID enbInterface,unsigned int carrier_id,unsigned int nSSId)
{
	LTE_ENB_MAC* enbMac=(LTE_ENB_MAC*)DEVICE_MACVAR(enbId,enbInterface);
	LTE_ASSOCIATEUE_INFO* info=enbMac->associatedUEInfo;

	unsigned int Required_Rate;
	unsigned int temp_AllocatedRBG;
	unsigned int nRBGcount;

	enbMac->ca_mac[carrier_id].nAllocatedRBG=0;
	//Send HARQ buffer
	while(info)
	{
		if(info->nSSId==nSSId)
			fn_NetSim_LTE_HARQ_RetransmitHARQBuffer_DL(info,enbMac,enbId,carrier_id);
		UEINFO_NEXT(info);
	}
	temp_AllocatedRBG = enbMac->ca_mac[carrier_id].nAllocatedRBG;
	enbMac->ca_mac[carrier_id].nAllocatedRBG = 0;
	info=enbMac->associatedUEInfo;

	//GBR queue
	while(info)
	{
		if((info->carrier_id==-1 || info->carrier_id==(int)carrier_id) &&
			info->nSSId == nSSId)
		{
			if(info->rrcState==RRC_CONNECTED && info->GBRQueue->nSize)
			{
				unsigned int index=0;
				unsigned int size=info->GBRQueue->nSize;
				unsigned int nTBSIndex_DL=info->DLInfo[carrier_id].TBSIndex;
				unsigned int flag=0;
				size = size*8;
				while(index<LTE_NRB_MAX)
				{
					if ((MODE_INDEX_MAPPING[info->TransmissionIndex.DL].TRANSMISSION_MODE == SingleUser_MIMO_Spatial_Multiplexing &&
						info->DLInfo[carrier_id].modulation== Modulation_64_QAM)) 
						switch (info->DLInfo[carrier_id].Rank)
					{
						case 2:if(size<TransportBlockSize2[nTBSIndex_DL][index])
								   flag=1;
							break;
						case 3:if(size<TransportBlockSize3[nTBSIndex_DL][index])
								   flag=1;
							break;
						case 4:if(size<TransportBlockSize4[nTBSIndex_DL][index])
								   flag=1;
							break;
					}
					else if(size<TransportBlockSize1[nTBSIndex_DL][index])
					{
						flag=1;
						break;
					}
					if(flag==1)
						break;
					index++;
				}
				if(index>=LTE_NRB_MAX)
					index=LTE_NRB_MAX-1;
				nRBGcount=(int)ceil((double)(index+1)/enbMac->ca_mac[carrier_id].nRBCountInGroup);
				if(nRBGcount>enbMac->ca_mac[carrier_id].nRBGCount-enbMac->ca_mac[carrier_id].nAllocatedRBG)
					nRBGcount=enbMac->ca_mac[carrier_id].nRBGCount-enbMac->ca_mac[carrier_id].nAllocatedRBG;
				info->GBRQueue->nRBLength=nRBGcount*enbMac->ca_mac[carrier_id].nRBCountInGroup;
				if ((MODE_INDEX_MAPPING[info->TransmissionIndex.DL].TRANSMISSION_MODE == SingleUser_MIMO_Spatial_Multiplexing && info->DLInfo[carrier_id].modulation== Modulation_64_QAM)) 
					switch (info->DLInfo[carrier_id].Rank)
				{
					case 2:Required_Rate= TransportBlockSize2[nTBSIndex_DL][info->GBRQueue->nRBLength-1];
						break;
					case 3:Required_Rate= TransportBlockSize3[nTBSIndex_DL][info->GBRQueue->nRBLength-1];
						break;
					case 4:Required_Rate= TransportBlockSize4[nTBSIndex_DL][info->GBRQueue->nRBLength-1];
						break;
				}
				else Required_Rate= TransportBlockSize2[nTBSIndex_DL][info->GBRQueue->nRBLength-1];	

				if(info->GBRQueue->dPrev_throughput == 0)
					info->GBRQueue->dPrev_throughput = 1;
				info->GBRQueue->dRatio = 1/(double)info->GBRQueue->dPrev_throughput;
			}
		}
		info = (LTE_ASSOCIATEUE_INFO*)LIST_NEXT(info);	
	}
	info = enbMac->associatedUEInfo;

	if(info)
		list_sort((void**)&enbMac->associatedUEInfo,enbMac->associatedUEInfo->ele->offset,CompareGBR);

	info = enbMac->associatedUEInfo;
	while(info)
	{
		if((info->carrier_id==-1 || info->carrier_id == (int)carrier_id) &&
			info->nSSId == nSSId)
		{
			if(info->rrcState==RRC_CONNECTED && info->GBRQueue->nSize)
			{
				unsigned int index=0;
				unsigned int size=info->GBRQueue->nSize;
				unsigned int nTBSIndex_DL=info->DLInfo[carrier_id].TBSIndex;
				unsigned int flag=0;
				size = size*8;
				while(index<LTE_NRB_MAX)
				{
					if ((MODE_INDEX_MAPPING[info->TransmissionIndex.DL].TRANSMISSION_MODE == SingleUser_MIMO_Spatial_Multiplexing 
						&& info->DLInfo[carrier_id].modulation== Modulation_64_QAM)) 
						switch (info->DLInfo[carrier_id].Rank)
					{
						case 2:if(size<TransportBlockSize2[nTBSIndex_DL][index])
								   flag=1;
							break;
						case 3:if(size<TransportBlockSize3[nTBSIndex_DL][index])
								   flag=1;
							break;
						case 4:if(size<TransportBlockSize4[nTBSIndex_DL][index])
								   flag=1;
							break;
					}
					else if(size<TransportBlockSize1[nTBSIndex_DL][index])
					{
						flag=1;
						break;
					}
					if(flag==1)
						break;
					index++;
				}
				if(index>=LTE_NRB_MAX)
					index=LTE_NRB_MAX-1;
				nRBGcount=(int)ceil((double)(index+1)/enbMac->ca_mac[carrier_id].nRBCountInGroup);
				if(nRBGcount>enbMac->ca_mac[carrier_id].nRBGCount-enbMac->ca_mac[carrier_id].nAllocatedRBG)
					nRBGcount=enbMac->ca_mac[carrier_id].nRBGCount-enbMac->ca_mac[carrier_id].nAllocatedRBG;
				info->GBRQueue->nRBStart=enbMac->ca_mac[carrier_id].nAllocatedRBG;
				info->GBRQueue->nRBLength=nRBGcount*enbMac->ca_mac[carrier_id].nRBCountInGroup;
				if ((MODE_INDEX_MAPPING[info->TransmissionIndex.DL].TRANSMISSION_MODE == SingleUser_MIMO_Spatial_Multiplexing
					&& info->DLInfo[carrier_id].modulation== Modulation_64_QAM)) 
					switch (info->DLInfo[carrier_id].Rank)
				{
					case 2: info->GBRQueue->nBitcount=TransportBlockSize2[nTBSIndex_DL][info->GBRQueue->nRBLength-1];
						break;
					case 3: info->GBRQueue->nBitcount=TransportBlockSize3[nTBSIndex_DL][info->GBRQueue->nRBLength-1];
						break;
					case 4: info->GBRQueue->nBitcount=TransportBlockSize4[nTBSIndex_DL][info->GBRQueue->nRBLength-1];
						break;
				}
				else  info->GBRQueue->nBitcount=TransportBlockSize1[nTBSIndex_DL][info->GBRQueue->nRBLength-1];
				enbMac->ca_mac[carrier_id].nAllocatedRBG+=nRBGcount;
				if(info->GBRQueue->nRBLength)
					info->GBRQueue->dPrev_throughput = 0.99*info->GBRQueue->nBitcount + 0.01*info->GBRQueue->dPrev_throughput;
				else 
					info->GBRQueue->dPrev_throughput = 1 + 0.01*info->GBRQueue->dPrev_throughput;
				
				//Form RLC SDU
				fn_NetSim_LTE_RLC_FormRLCSDU(info->GBRQueue,enbId,info->nUEId,info->HARQBuffer,carrier_id,nSSId);
				if(nRBGcount)
					info->carrier_id=carrier_id;
			}
		}
		UEINFO_NEXT(info);
	}
	temp_AllocatedRBG = enbMac->ca_mac[carrier_id].nAllocatedRBG;
	enbMac->ca_mac[carrier_id].nAllocatedRBG = 0;
	//non GBR queue
	info=enbMac->associatedUEInfo;

	while(info)
	{
		if((info->carrier_id==-1 || info->carrier_id == (int)carrier_id) &&
			info->nSSId == nSSId)
		{
			if(info->rrcState==RRC_CONNECTED && info->nonGBRQueue->nSize)
			{
				unsigned int index=0;
				unsigned int size=info->nonGBRQueue->nSize;
				unsigned int nTBSIndex_DL=info->DLInfo[carrier_id].TBSIndex;
				unsigned int flag=0;

				LTE_ASSOCIATEUE_INFO peerInfo;
				NETSIM_ID peer_id;
				fn_NetSim_D2D_GetInfo_DL(info->nUEId,
					carrier_id,
					enbId,
					enbInterface,
					info,
					&peerInfo,
					&peer_id);

				nTBSIndex_DL = peerInfo.DLInfo[carrier_id].TBSIndex;

				size = size*8;
				while(index<LTE_NRB_MAX)
				{
					if ((MODE_INDEX_MAPPING[peerInfo.TransmissionIndex.DL].TRANSMISSION_MODE == SingleUser_MIMO_Spatial_Multiplexing
						&& peerInfo.DLInfo[carrier_id].modulation== Modulation_64_QAM)) 
						switch (peerInfo.DLInfo[carrier_id].Rank)
					{
						case 2:if(size<TransportBlockSize2[nTBSIndex_DL][index])
								   flag=1;
							break;
						case 3:if(size<TransportBlockSize3[nTBSIndex_DL][index])
								   flag=1;
							break;
						case 4:if(size<TransportBlockSize4[nTBSIndex_DL][index])
								   flag=1;
							break;
					}
					else if(size<TransportBlockSize1[nTBSIndex_DL][index])
					{
						flag=1;
						break;
					}
					if(flag==1)
						break;
					index++;
				}
				if(index>=LTE_NRB_MAX)
					index=LTE_NRB_MAX-1;
				nRBGcount=(int)ceil((double)(index+1)/enbMac->ca_mac[carrier_id].nRBCountInGroup);
				if(nRBGcount>enbMac->ca_mac[carrier_id].nRBGCount-enbMac->ca_mac[carrier_id].nAllocatedRBG)
					nRBGcount=enbMac->ca_mac[carrier_id].nRBGCount-enbMac->ca_mac[carrier_id].nAllocatedRBG;
				info->nonGBRQueue->nRBLength=nRBGcount*enbMac->ca_mac[carrier_id].nRBCountInGroup;
				if ((MODE_INDEX_MAPPING[peerInfo.TransmissionIndex.DL].TRANSMISSION_MODE == SingleUser_MIMO_Spatial_Multiplexing
					&& peerInfo.DLInfo[carrier_id].modulation== Modulation_64_QAM)) 
					switch (peerInfo.DLInfo[carrier_id].Rank)
				{
					case 2:Required_Rate= TransportBlockSize2[nTBSIndex_DL][info->nonGBRQueue->nRBLength-1];
						break;
					case 3:Required_Rate= TransportBlockSize3[nTBSIndex_DL][info->nonGBRQueue->nRBLength-1];
						break;
					case 4:Required_Rate= TransportBlockSize4[nTBSIndex_DL][info->nonGBRQueue->nRBLength-1];
						break;
				}
				else Required_Rate= TransportBlockSize1[nTBSIndex_DL][info->nonGBRQueue->nRBLength-1];	
				if(info->nonGBRQueue->dPrev_throughput == 0)
					info->nonGBRQueue->dPrev_throughput = 1;
				info->nonGBRQueue->dRatio = 1/info->nonGBRQueue->dPrev_throughput;
			}
		}
		UEINFO_NEXT(info);
	}
	info = enbMac->associatedUEInfo;
	if(info)
		list_sort((void**)&enbMac->associatedUEInfo,enbMac->associatedUEInfo->ele->offset,CompareNonGBR);
	info=enbMac->associatedUEInfo;
	enbMac->ca_mac[carrier_id].nAllocatedRBG = temp_AllocatedRBG;
	while(info)
	{
		if((info->carrier_id==-1 || info->carrier_id == (int)carrier_id) &&
			info->nSSId == nSSId)
		{
			if(info->rrcState==RRC_CONNECTED && info->nonGBRQueue->nSize)
			{
				unsigned int index=0;
				unsigned int size=info->nonGBRQueue->nSize;
				unsigned int nTBSIndex_DL=info->DLInfo[carrier_id].TBSIndex;
				unsigned int flag=0;

				LTE_ASSOCIATEUE_INFO peerInfo;
				NETSIM_ID peer_id;
				fn_NetSim_D2D_GetInfo_DL(info->nUEId,
					carrier_id,
					enbId,
					enbInterface,
					info,
					&peerInfo,
					&peer_id);

				nTBSIndex_DL = peerInfo.DLInfo[carrier_id].TBSIndex;

				size = size*8;
				while(index<LTE_NRB_MAX)
				{
					if ((MODE_INDEX_MAPPING[peerInfo.TransmissionIndex.DL].TRANSMISSION_MODE == SingleUser_MIMO_Spatial_Multiplexing 
						&& peerInfo.DLInfo[carrier_id].modulation== Modulation_64_QAM)) 
						switch (peerInfo.DLInfo[carrier_id].Rank)
					{
						case 2:if(size<TransportBlockSize2[nTBSIndex_DL][index])
								   flag=1;
							break;
						case 3:if(size<TransportBlockSize3[nTBSIndex_DL][index])
								   flag=1;
							break;
						case 4:if(size<TransportBlockSize4[nTBSIndex_DL][index])
								   flag=1;
							break;
					}
					else if(size<TransportBlockSize1[nTBSIndex_DL][index])
					{
						flag=1;
						break;
					}
					if(flag==1)
						break;
					index++;
				}
				if(index>=LTE_NRB_MAX)
					index=LTE_NRB_MAX-1;
				nRBGcount=(int)ceil((double)(index+1)/enbMac->ca_mac[carrier_id].nRBCountInGroup);
				if(nRBGcount>enbMac->ca_mac[carrier_id].nRBGCount-enbMac->ca_mac[carrier_id].nAllocatedRBG)
					nRBGcount=enbMac->ca_mac[carrier_id].nRBGCount-enbMac->ca_mac[carrier_id].nAllocatedRBG;
				info->nonGBRQueue->nRBStart=enbMac->ca_mac[carrier_id].nAllocatedRBG;
				info->nonGBRQueue->nRBLength=nRBGcount*enbMac->ca_mac[carrier_id].nRBCountInGroup;
				if(info->nonGBRQueue->nRBLength)
				{
					if ((MODE_INDEX_MAPPING[peerInfo.TransmissionIndex.DL].TRANSMISSION_MODE == SingleUser_MIMO_Spatial_Multiplexing 
						&& peerInfo.DLInfo[carrier_id].modulation== Modulation_64_QAM)) 
						switch (peerInfo.DLInfo[carrier_id].Rank)
					{
						case 2: info->nonGBRQueue->nBitcount=TransportBlockSize2[nTBSIndex_DL][info->nonGBRQueue->nRBLength-1];
							break;
						case 3: info->nonGBRQueue->nBitcount=TransportBlockSize3[nTBSIndex_DL][info->nonGBRQueue->nRBLength-1];
							break;
						case 4: info->nonGBRQueue->nBitcount=TransportBlockSize4[nTBSIndex_DL][info->nonGBRQueue->nRBLength-1];
							break;
					}
					else  info->nonGBRQueue->nBitcount=TransportBlockSize1[nTBSIndex_DL][info->nonGBRQueue->nRBLength-1];

					enbMac->ca_mac[carrier_id].nAllocatedRBG+=nRBGcount;
					info->nonGBRQueue->dPrev_throughput = 0.99*info->nonGBRQueue->nBitcount + 0.01*info->nonGBRQueue->dPrev_throughput;
					if(nRBGcount)
						info->carrier_id=carrier_id;
					//Form RLC SDU
					fn_NetSim_LTE_RLC_FormRLCSDU(info->nonGBRQueue,enbId,peer_id,peerInfo.HARQBuffer,carrier_id,nSSId);	

				}
				else
					info->nonGBRQueue->dPrev_throughput = 1 + 0.01*info->nonGBRQueue->dPrev_throughput;
			}
		}
		UEINFO_NEXT(info);
	}
	return 1;
}

static int fn_NetSim_LTE_FormDownlinkFrame_MaxCQI(NETSIM_ID enbId,NETSIM_ID enbInterface,unsigned int carrier_id,unsigned int nSSId)
{
	LTE_ENB_MAC* enbMac=(LTE_ENB_MAC*)DEVICE_MACVAR(enbId,enbInterface);
	LTE_ASSOCIATEUE_INFO* info=enbMac->associatedUEInfo;

	unsigned int Required_Rate=0;
	unsigned int temp_AllocatedRBG;
	unsigned int nRBGcount;

	enbMac->ca_mac[carrier_id].nAllocatedRBG=0;
	//Send HARQ buffer
	while(info)
	{
		if(info->nSSId == nSSId)
			fn_NetSim_LTE_HARQ_RetransmitHARQBuffer_DL(info,enbMac,enbId,carrier_id);
		UEINFO_NEXT(info);
	}
	temp_AllocatedRBG = enbMac->ca_mac[carrier_id].nAllocatedRBG;
	enbMac->ca_mac[carrier_id].nAllocatedRBG = 0;
	info=enbMac->associatedUEInfo;

	//GBR queue
	while(info)
	{
		if((info->carrier_id==-1 || info->carrier_id==(int)carrier_id) &&
			info->nSSId == nSSId)
		{
			if(info->rrcState==RRC_CONNECTED && info->GBRQueue->nSize)
			{
				unsigned int index=0;
				unsigned int size=info->GBRQueue->nSize;
				unsigned int nTBSIndex_DL=info->DLInfo[carrier_id].TBSIndex;
				unsigned int flag=0;
				size = size*8;
				while(index<LTE_NRB_MAX)
				{
					if ((MODE_INDEX_MAPPING[info->TransmissionIndex.DL].TRANSMISSION_MODE == SingleUser_MIMO_Spatial_Multiplexing && info->DLInfo[carrier_id].modulation== Modulation_64_QAM)) 
						switch (info->DLInfo[carrier_id].Rank)
					{
						case 2:if(size<TransportBlockSize2[nTBSIndex_DL][index])
								   flag=1;
							break;
						case 3:if(size<TransportBlockSize3[nTBSIndex_DL][index])
								   flag=1;
							break;
						case 4:if(size<TransportBlockSize4[nTBSIndex_DL][index])
								   flag=1;
							break;
					}
					else if(size<TransportBlockSize1[nTBSIndex_DL][index])
					{
						flag=1;
						break;
					}
					if(flag==1)
						break;
					index++;
				}
				if(index>=LTE_NRB_MAX)
					index=LTE_NRB_MAX-1;
				nRBGcount=(int)ceil((double)(index+1)/enbMac->ca_mac[carrier_id].nRBCountInGroup);
				if(nRBGcount>enbMac->ca_mac[carrier_id].nRBGCount-enbMac->ca_mac[carrier_id].nAllocatedRBG)
					nRBGcount=enbMac->ca_mac[carrier_id].nRBGCount-enbMac->ca_mac[carrier_id].nAllocatedRBG;
				info->GBRQueue->nRBLength=nRBGcount*enbMac->ca_mac[carrier_id].nRBCountInGroup;
				if ((MODE_INDEX_MAPPING[info->TransmissionIndex.DL].TRANSMISSION_MODE == SingleUser_MIMO_Spatial_Multiplexing && info->DLInfo[carrier_id].modulation== Modulation_64_QAM)) 
					switch (info->DLInfo[carrier_id].Rank)
				{
					case 2:Required_Rate= TransportBlockSize2[nTBSIndex_DL][info->GBRQueue->nRBLength-1];
						break;
					case 3:Required_Rate= TransportBlockSize3[nTBSIndex_DL][info->GBRQueue->nRBLength-1];
						break;
					case 4:Required_Rate= TransportBlockSize4[nTBSIndex_DL][info->GBRQueue->nRBLength-1];
						break;
				}
				else Required_Rate= TransportBlockSize2[nTBSIndex_DL][info->GBRQueue->nRBLength-1];	

				if(info->GBRQueue->dPrev_throughput == 0)
					info->GBRQueue->dPrev_throughput = 1;
				info->GBRQueue->dRatio = (double)Required_Rate;
			}
		}
		info = (LTE_ASSOCIATEUE_INFO*)LIST_NEXT(info);	
	}
	info = enbMac->associatedUEInfo;
	if(info)
		list_sort((void**)&enbMac->associatedUEInfo,enbMac->associatedUEInfo->ele->offset,CompareGBR);

	info = enbMac->associatedUEInfo;
	while(info)
	{
		if((info->carrier_id==-1 || info->carrier_id == (int)carrier_id) &&
			info->nSSId == nSSId)
		{
			if(info->rrcState==RRC_CONNECTED && info->GBRQueue->nSize)
			{
				unsigned int index=0;
				unsigned int size=info->GBRQueue->nSize;
				unsigned int nTBSIndex_DL=info->DLInfo[carrier_id].TBSIndex;
				unsigned int flag=0;
				size = size*8;
				while(index<LTE_NRB_MAX)
				{
					if ((MODE_INDEX_MAPPING[info->TransmissionIndex.DL].TRANSMISSION_MODE == SingleUser_MIMO_Spatial_Multiplexing && info->DLInfo[carrier_id].modulation== Modulation_64_QAM)) 
						switch (info->DLInfo[carrier_id].Rank)
					{
						case 2:if(size<TransportBlockSize2[nTBSIndex_DL][index])
								   flag=1;
							break;
						case 3:if(size<TransportBlockSize3[nTBSIndex_DL][index])
								   flag=1;
							break;
						case 4:if(size<TransportBlockSize4[nTBSIndex_DL][index])
								   flag=1;
							break;
					}
					else if(size<TransportBlockSize1[nTBSIndex_DL][index])
					{
						flag=1;
						break;
					}
					if(flag==1)
						break;
					index++;
				}
				if(index>=LTE_NRB_MAX)
					index=LTE_NRB_MAX-1;
				nRBGcount=(int)ceil((double)(index+1)/enbMac->ca_mac[carrier_id].nRBCountInGroup);
				if(nRBGcount>enbMac->ca_mac[carrier_id].nRBGCount-enbMac->ca_mac[carrier_id].nAllocatedRBG)
					nRBGcount=enbMac->ca_mac[carrier_id].nRBGCount-enbMac->ca_mac[carrier_id].nAllocatedRBG;
				info->GBRQueue->nRBStart=enbMac->ca_mac[carrier_id].nAllocatedRBG;
				info->GBRQueue->nRBLength=nRBGcount*enbMac->ca_mac[carrier_id].nRBCountInGroup;
				if ((MODE_INDEX_MAPPING[info->TransmissionIndex.DL].TRANSMISSION_MODE == SingleUser_MIMO_Spatial_Multiplexing && info->DLInfo[carrier_id].modulation== Modulation_64_QAM)) 
					switch (info->DLInfo[carrier_id].Rank)
				{
					case 2: info->GBRQueue->nBitcount=TransportBlockSize2[nTBSIndex_DL][info->GBRQueue->nRBLength-1];
						break;
					case 3: info->GBRQueue->nBitcount=TransportBlockSize3[nTBSIndex_DL][info->GBRQueue->nRBLength-1];
						break;
					case 4: info->GBRQueue->nBitcount=TransportBlockSize4[nTBSIndex_DL][info->GBRQueue->nRBLength-1];
						break;
				}
				else  info->GBRQueue->nBitcount=TransportBlockSize1[nTBSIndex_DL][info->GBRQueue->nRBLength-1];
				enbMac->ca_mac[carrier_id].nAllocatedRBG+=nRBGcount;
				if(info->GBRQueue->nRBLength)
					info->GBRQueue->dPrev_throughput = 0.99*info->GBRQueue->nBitcount + 0.01*info->GBRQueue->dPrev_throughput;
				else 
					info->GBRQueue->dPrev_throughput = 1 + 0.01*info->GBRQueue->dPrev_throughput;

				//Form RLC SDU
				fn_NetSim_LTE_RLC_FormRLCSDU(info->GBRQueue,enbId,info->nUEId,info->HARQBuffer,carrier_id,nSSId);	
				if(nRBGcount)
					info->carrier_id=carrier_id;

			}
		}
		UEINFO_NEXT(info);
	}
	temp_AllocatedRBG = enbMac->ca_mac[carrier_id].nAllocatedRBG;
	enbMac->ca_mac[carrier_id].nAllocatedRBG = 0;
	//non GBR queue
	info=enbMac->associatedUEInfo;

	while(info)
	{
		if((info->carrier_id==-1 || info->carrier_id == (int)carrier_id) &&
			info->nSSId == nSSId)
		{
			if(info->rrcState==RRC_CONNECTED && info->nonGBRQueue->nSize)
			{
				unsigned int index=0;
				unsigned int size=info->nonGBRQueue->nSize;
				unsigned int nTBSIndex_DL=info->DLInfo[carrier_id].TBSIndex;
				unsigned int flag=0;
				LTE_ASSOCIATEUE_INFO peerInfo;

				NETSIM_ID peer_id;
				fn_NetSim_D2D_GetInfo_DL(info->nUEId,
					carrier_id,
					enbId,
					enbInterface,
					info,
					&peerInfo,
					&peer_id);

				nTBSIndex_DL = peerInfo.DLInfo[carrier_id].TBSIndex;

				size = size*8;
				while(index<LTE_NRB_MAX)
				{
					if ((MODE_INDEX_MAPPING[peerInfo.TransmissionIndex.DL].TRANSMISSION_MODE == SingleUser_MIMO_Spatial_Multiplexing &&
						peerInfo.DLInfo[carrier_id].modulation== Modulation_64_QAM)) 
						switch (peerInfo.DLInfo[carrier_id].Rank)
					{
						case 2:if(size<TransportBlockSize2[nTBSIndex_DL][index])
								   flag=1;
							break;
						case 3:if(size<TransportBlockSize3[nTBSIndex_DL][index])
								   flag=1;
							break;
						case 4:if(size<TransportBlockSize4[nTBSIndex_DL][index])
								   flag=1;
							break;
					}
					else if(size<TransportBlockSize1[nTBSIndex_DL][index])
					{
						flag=1;
						break;
					}
					if(flag==1)
						break;
					index++;
				}
				if(index>=LTE_NRB_MAX)
					index=LTE_NRB_MAX-1;
				nRBGcount=(int)ceil((double)(index+1)/enbMac->ca_mac[carrier_id].nRBCountInGroup);
				if(nRBGcount>enbMac->ca_mac[carrier_id].nRBGCount-enbMac->ca_mac[carrier_id].nAllocatedRBG)
					nRBGcount=enbMac->ca_mac[carrier_id].nRBGCount-enbMac->ca_mac[carrier_id].nAllocatedRBG;
				info->nonGBRQueue->nRBLength=nRBGcount*enbMac->ca_mac[carrier_id].nRBCountInGroup;
				if ((MODE_INDEX_MAPPING[peerInfo.TransmissionIndex.DL].TRANSMISSION_MODE == SingleUser_MIMO_Spatial_Multiplexing &&
					peerInfo.DLInfo[carrier_id].modulation== Modulation_64_QAM)) 
					switch (peerInfo.DLInfo[carrier_id].Rank)
				{
					case 2:Required_Rate= TransportBlockSize2[nTBSIndex_DL][info->nonGBRQueue->nRBLength-1];
						break;
					case 3:Required_Rate= TransportBlockSize3[nTBSIndex_DL][info->nonGBRQueue->nRBLength-1];
						break;
					case 4:Required_Rate= TransportBlockSize4[nTBSIndex_DL][info->nonGBRQueue->nRBLength-1];
						break;
				}
				else Required_Rate= TransportBlockSize1[nTBSIndex_DL][info->nonGBRQueue->nRBLength-1];	
				if(info->nonGBRQueue->dPrev_throughput == 0)
					info->nonGBRQueue->dPrev_throughput = 1;
				info->nonGBRQueue->dRatio = Required_Rate;
			}
		}
		info = (LTE_ASSOCIATEUE_INFO*)LIST_NEXT(info);
	}
	info = enbMac->associatedUEInfo;
	if(info)
		list_sort((void**)&enbMac->associatedUEInfo,enbMac->associatedUEInfo->ele->offset,CompareNonGBR);
	info=enbMac->associatedUEInfo;
	enbMac->ca_mac[carrier_id].nAllocatedRBG = temp_AllocatedRBG;
	while(info)
	{
		if((info->carrier_id==-1 || info->carrier_id == (int)carrier_id) &&
			info->nSSId == nSSId)
		{
			if(info->rrcState==RRC_CONNECTED && info->nonGBRQueue->nSize)
			{
				unsigned int index=0;
				unsigned int size=info->nonGBRQueue->nSize;
				unsigned int nTBSIndex_DL=info->DLInfo[carrier_id].TBSIndex;
				unsigned int flag=0;
				LTE_ASSOCIATEUE_INFO peerInfo;

				NETSIM_ID peer_id;
				fn_NetSim_D2D_GetInfo_DL(info->nUEId,
					carrier_id,
					enbId,
					enbInterface,
					info,
					&peerInfo,
					&peer_id);

				nTBSIndex_DL = peerInfo.DLInfo[carrier_id].TBSIndex;

				size = size*8;
				while(index<LTE_NRB_MAX)
				{
					if ((MODE_INDEX_MAPPING[peerInfo.TransmissionIndex.DL].TRANSMISSION_MODE == SingleUser_MIMO_Spatial_Multiplexing &&
						peerInfo.DLInfo[carrier_id].modulation== Modulation_64_QAM)) 
						switch (peerInfo.DLInfo[carrier_id].Rank)
					{
						case 2:if(size<TransportBlockSize2[nTBSIndex_DL][index])
								   flag=1;
							break;
						case 3:if(size<TransportBlockSize3[nTBSIndex_DL][index])
								   flag=1;
							break;
						case 4:if(size<TransportBlockSize4[nTBSIndex_DL][index])
								   flag=1;
							break;
					}
					else if(size<TransportBlockSize1[nTBSIndex_DL][index])
					{
						flag=1;
						break;
					}
					if(flag==1)
						break;
					index++;
				}
				if(index>=LTE_NRB_MAX)
					index=LTE_NRB_MAX-1;
				nRBGcount=(int)ceil((double)(index+1)/enbMac->ca_mac[carrier_id].nRBCountInGroup);
				if(nRBGcount>enbMac->ca_mac[carrier_id].nRBGCount-enbMac->ca_mac[carrier_id].nAllocatedRBG)
					nRBGcount=enbMac->ca_mac[carrier_id].nRBGCount-enbMac->ca_mac[carrier_id].nAllocatedRBG;
				info->nonGBRQueue->nRBStart=enbMac->ca_mac[carrier_id].nAllocatedRBG;
				info->nonGBRQueue->nRBLength=nRBGcount*enbMac->ca_mac[carrier_id].nRBCountInGroup;
				if(info->nonGBRQueue->nRBLength)
				{
					if ((MODE_INDEX_MAPPING[peerInfo.TransmissionIndex.DL].TRANSMISSION_MODE == SingleUser_MIMO_Spatial_Multiplexing &&
						peerInfo.DLInfo[carrier_id].modulation== Modulation_64_QAM)) 
						switch (peerInfo.DLInfo[carrier_id].Rank)
					{
						case 2: info->nonGBRQueue->nBitcount=TransportBlockSize2[nTBSIndex_DL][info->nonGBRQueue->nRBLength-1];
							break;
						case 3: info->nonGBRQueue->nBitcount=TransportBlockSize3[nTBSIndex_DL][info->nonGBRQueue->nRBLength-1];
							break;
						case 4: info->nonGBRQueue->nBitcount=TransportBlockSize4[nTBSIndex_DL][info->nonGBRQueue->nRBLength-1];
							break;
					}
					else  info->nonGBRQueue->nBitcount=TransportBlockSize1[nTBSIndex_DL][info->nonGBRQueue->nRBLength-1];

					//	info->nonGBRQueue->nBitcount=TransportBlockSize[nTBSIndex_DL][info->nonGBRQueue->nRBLength-1];
					enbMac->ca_mac[carrier_id].nAllocatedRBG+=nRBGcount;
					info->nonGBRQueue->dPrev_throughput = 0.99*info->nonGBRQueue->nBitcount + 0.01*info->nonGBRQueue->dPrev_throughput;

					//Form RLC SDU
					fn_NetSim_LTE_RLC_FormRLCSDU(info->nonGBRQueue,enbId,peer_id,peerInfo.HARQBuffer,carrier_id,nSSId);
					info->carrier_id=carrier_id;
				}
				else
					info->nonGBRQueue->dPrev_throughput = 1 + 0.01*info->nonGBRQueue->dPrev_throughput;
			}
		}
		UEINFO_NEXT(info);
	}
	return 1;
}

static int fn_NetSim_LTE_FormDownlinkFrame_ProportionalFairScheduling(NETSIM_ID enbId,NETSIM_ID enbInterface,unsigned int carrier_id,unsigned int nSSId)
{
	LTE_ENB_MAC* enbMac=(LTE_ENB_MAC*)DEVICE_MACVAR(enbId,enbInterface);
	LTE_ASSOCIATEUE_INFO* info=enbMac->associatedUEInfo;

	unsigned int Required_Rate=0;
	unsigned int temp_AllocatedRBG;
	unsigned int nRBGcount;

	enbMac->ca_mac[carrier_id].nAllocatedRBG=0;
	//Send HARQ buffer
	while(info)
	{
		if(info->nSSId == nSSId)
			fn_NetSim_LTE_HARQ_RetransmitHARQBuffer_DL(info,enbMac,enbId,carrier_id);
		UEINFO_NEXT(info);
	}
	temp_AllocatedRBG = enbMac->ca_mac[carrier_id].nAllocatedRBG;
	enbMac->ca_mac[carrier_id].nAllocatedRBG = 0;
	info=enbMac->associatedUEInfo;

	//GBR queue
	while(info)
	{
		if((info->carrier_id==-1 || info->carrier_id==(int)carrier_id) &&
			info->nSSId == nSSId)
		{
			if(info->rrcState==RRC_CONNECTED && info->GBRQueue->nSize)
			{
				unsigned int index=0;
				unsigned int size=info->GBRQueue->nSize;
				unsigned int nTBSIndex_DL=info->DLInfo[carrier_id].TBSIndex;
				unsigned int flag=0;
				size = size*8;
				while(index<LTE_NRB_MAX)
				{
					if ((MODE_INDEX_MAPPING[info->TransmissionIndex.DL].TRANSMISSION_MODE == SingleUser_MIMO_Spatial_Multiplexing && info->DLInfo[carrier_id].modulation== Modulation_64_QAM)) 
						switch (info->DLInfo[carrier_id].Rank)
					{
						case 2:if(size<TransportBlockSize2[nTBSIndex_DL][index])
								   flag=1;
							break;
						case 3:if(size<TransportBlockSize3[nTBSIndex_DL][index])
								   flag=1;
							break;
						case 4:if(size<TransportBlockSize4[nTBSIndex_DL][index])
								   flag=1;
							break;
					}
					else if(size<TransportBlockSize1[nTBSIndex_DL][index])
					{
						flag=1;
						break;
					}
					if(flag==1)
						break;
					index++;
				}
				if(index>=LTE_NRB_MAX)
					index=LTE_NRB_MAX-1;
				nRBGcount=(int)ceil((double)(index+1)/enbMac->ca_mac[carrier_id].nRBCountInGroup);
				if(nRBGcount>enbMac->ca_mac[carrier_id].nRBGCount-enbMac->ca_mac[carrier_id].nAllocatedRBG)
					nRBGcount=enbMac->ca_mac[carrier_id].nRBGCount-enbMac->ca_mac[carrier_id].nAllocatedRBG;
				info->GBRQueue->nRBLength=nRBGcount*enbMac->ca_mac[carrier_id].nRBCountInGroup;
				if ((MODE_INDEX_MAPPING[info->TransmissionIndex.DL].TRANSMISSION_MODE == SingleUser_MIMO_Spatial_Multiplexing && info->DLInfo[carrier_id].modulation== Modulation_64_QAM)) 
					switch (info->DLInfo[carrier_id].Rank)
				{
					case 2:Required_Rate= TransportBlockSize2[nTBSIndex_DL][info->GBRQueue->nRBLength-1];
						break;
					case 3:Required_Rate= TransportBlockSize3[nTBSIndex_DL][info->GBRQueue->nRBLength-1];
						break;
					case 4:Required_Rate= TransportBlockSize4[nTBSIndex_DL][info->GBRQueue->nRBLength-1];
						break;
				}
				else Required_Rate= TransportBlockSize2[nTBSIndex_DL][info->GBRQueue->nRBLength-1];	

				if(info->GBRQueue->dPrev_throughput == 0)
					info->GBRQueue->dPrev_throughput = 1;
				info->GBRQueue->dRatio = (double)Required_Rate/(double)info->GBRQueue->dPrev_throughput;
			}
		}
		info = (LTE_ASSOCIATEUE_INFO*)LIST_NEXT(info);	
	}
	info = enbMac->associatedUEInfo;
	if(info)
		list_sort((void**)&enbMac->associatedUEInfo,enbMac->associatedUEInfo->ele->offset,CompareGBR);

	info = enbMac->associatedUEInfo;
	while(info)
	{
		if((info->carrier_id==-1 || info->carrier_id==(int)carrier_id) &&
			info->nSSId == nSSId)
		{
			if(info->rrcState==RRC_CONNECTED && info->GBRQueue->nSize)
			{
				unsigned int index=0;
				unsigned int size=info->GBRQueue->nSize;
				unsigned int nTBSIndex_DL=info->DLInfo[carrier_id].TBSIndex;
				unsigned int flag=0;
				size = size*8;
				while(index<LTE_NRB_MAX)
				{
					if ((MODE_INDEX_MAPPING[info->TransmissionIndex.DL].TRANSMISSION_MODE == SingleUser_MIMO_Spatial_Multiplexing && info->DLInfo[carrier_id].modulation== Modulation_64_QAM)) 
						switch (info->DLInfo[carrier_id].Rank)
					{
						case 2:if(size<TransportBlockSize2[nTBSIndex_DL][index])
								   flag=1;
							break;
						case 3:if(size<TransportBlockSize3[nTBSIndex_DL][index])
								   flag=1;
							break;
						case 4:if(size<TransportBlockSize4[nTBSIndex_DL][index])
								   flag=1;
							break;
					}
					else if(size<TransportBlockSize1[nTBSIndex_DL][index])
					{
						flag=1;
						break;
					}
					if(flag==1)
						break;
					index++;
				}
				if(index>=LTE_NRB_MAX)
					index=LTE_NRB_MAX-1;
				nRBGcount=(int)ceil((double)(index+1)/enbMac->ca_mac[carrier_id].nRBCountInGroup);
				if(nRBGcount>enbMac->ca_mac[carrier_id].nRBGCount-enbMac->ca_mac[carrier_id].nAllocatedRBG)
					nRBGcount=enbMac->ca_mac[carrier_id].nRBGCount-enbMac->ca_mac[carrier_id].nAllocatedRBG;
				info->GBRQueue->nRBStart=enbMac->ca_mac[carrier_id].nAllocatedRBG;
				info->GBRQueue->nRBLength=nRBGcount*enbMac->ca_mac[carrier_id].nRBCountInGroup;
				if ((MODE_INDEX_MAPPING[info->TransmissionIndex.DL].TRANSMISSION_MODE == SingleUser_MIMO_Spatial_Multiplexing && info->DLInfo[carrier_id].modulation== Modulation_64_QAM)) 
					switch (info->DLInfo[carrier_id].Rank)
				{
					case 2: info->GBRQueue->nBitcount=TransportBlockSize2[nTBSIndex_DL][info->GBRQueue->nRBLength-1];
						break;
					case 3: info->GBRQueue->nBitcount=TransportBlockSize3[nTBSIndex_DL][info->GBRQueue->nRBLength-1];
						break;
					case 4: info->GBRQueue->nBitcount=TransportBlockSize4[nTBSIndex_DL][info->GBRQueue->nRBLength-1];
						break;
				}
				else  info->GBRQueue->nBitcount=TransportBlockSize1[nTBSIndex_DL][info->GBRQueue->nRBLength-1];
				enbMac->ca_mac[carrier_id].nAllocatedRBG+=nRBGcount;
				if(info->GBRQueue->nRBLength)
					info->GBRQueue->dPrev_throughput = 0.99*info->GBRQueue->nBitcount + 0.01*info->GBRQueue->dPrev_throughput;
				else 
					info->GBRQueue->dPrev_throughput = 1 + 0.01*info->GBRQueue->dPrev_throughput;

				//Form RLC SDU
				fn_NetSim_LTE_RLC_FormRLCSDU(info->GBRQueue,enbId,info->nUEId,info->HARQBuffer,carrier_id,nSSId);	
				if(nRBGcount)
					info->carrier_id=carrier_id;
			}
		}
		UEINFO_NEXT(info);
	}
	temp_AllocatedRBG = enbMac->ca_mac[carrier_id].nAllocatedRBG;
	enbMac->ca_mac[carrier_id].nAllocatedRBG = 0;
	//non GBR queue
	info=enbMac->associatedUEInfo;

	while(info)
	{
		if((info->carrier_id==-1 || info->carrier_id == (int)carrier_id) &&
			info->nSSId == nSSId)
		{
			if(info->rrcState==RRC_CONNECTED && info->nonGBRQueue->nSize)
			{
				unsigned int index=0;
				unsigned int size=info->nonGBRQueue->nSize;
				unsigned int nTBSIndex_DL=info->DLInfo[carrier_id].TBSIndex;
				unsigned int flag=0;
				LTE_ASSOCIATEUE_INFO peerInfo;
				
				NETSIM_ID peer_id;
				fn_NetSim_D2D_GetInfo_DL(info->nUEId,
					carrier_id,
					enbId,
					enbInterface,
					info,
					&peerInfo,
					&peer_id);

				nTBSIndex_DL = peerInfo.DLInfo[carrier_id].TBSIndex;


				size = size*8;
				while(index<LTE_NRB_MAX)
				{
					if ((MODE_INDEX_MAPPING[peerInfo.TransmissionIndex.DL].TRANSMISSION_MODE == SingleUser_MIMO_Spatial_Multiplexing &&
						peerInfo.DLInfo[carrier_id].modulation== Modulation_64_QAM)) 
						switch (peerInfo.DLInfo[carrier_id].Rank)
					{
						case 2:if(size<TransportBlockSize2[nTBSIndex_DL][index])
								   flag=1;
							break;
						case 3:if(size<TransportBlockSize3[nTBSIndex_DL][index])
								   flag=1;
							break;
						case 4:if(size<TransportBlockSize4[nTBSIndex_DL][index])
								   flag=1;
							break;
					}
					else if(size<TransportBlockSize1[nTBSIndex_DL][index])
					{
						flag=1;
						break;
					}
					if(flag==1)
						break;
					index++;
				}
				if(index>=LTE_NRB_MAX)
					index=LTE_NRB_MAX-1;
				nRBGcount=(int)ceil((double)(index+1)/enbMac->ca_mac[carrier_id].nRBCountInGroup);
				if(nRBGcount>enbMac->ca_mac[carrier_id].nRBGCount-enbMac->ca_mac[carrier_id].nAllocatedRBG)
					nRBGcount=enbMac->ca_mac[carrier_id].nRBGCount-enbMac->ca_mac[carrier_id].nAllocatedRBG;
				info->nonGBRQueue->nRBLength=nRBGcount*enbMac->ca_mac[carrier_id].nRBCountInGroup;
				if ((MODE_INDEX_MAPPING[peerInfo.TransmissionIndex.DL].TRANSMISSION_MODE == SingleUser_MIMO_Spatial_Multiplexing && info->DLInfo[carrier_id].modulation== Modulation_64_QAM)) 
					switch (peerInfo.DLInfo[carrier_id].Rank)
				{
					case 2:Required_Rate= TransportBlockSize2[nTBSIndex_DL][info->nonGBRQueue->nRBLength-1];
						break;
					case 3:Required_Rate= TransportBlockSize3[nTBSIndex_DL][info->nonGBRQueue->nRBLength-1];
						break;
					case 4:Required_Rate= TransportBlockSize4[nTBSIndex_DL][info->nonGBRQueue->nRBLength-1];
						break;
				}
				else Required_Rate= TransportBlockSize1[nTBSIndex_DL][info->nonGBRQueue->nRBLength-1];	
				if(info->nonGBRQueue->dPrev_throughput == 0)
					info->nonGBRQueue->dPrev_throughput = 1;
				info->nonGBRQueue->dRatio = (double)Required_Rate/info->nonGBRQueue->dPrev_throughput;
			}
		}
		info = (LTE_ASSOCIATEUE_INFO*)LIST_NEXT(info);
	}
	info = enbMac->associatedUEInfo;
	if(info)
		list_sort((void**)&enbMac->associatedUEInfo,enbMac->associatedUEInfo->ele->offset,CompareNonGBR);
	info = enbMac->associatedUEInfo;
	enbMac->ca_mac[carrier_id].nAllocatedRBG = temp_AllocatedRBG;
	while(info)
	{
		if((info->carrier_id==-1 || info->carrier_id == (int)carrier_id) &&
			info->nSSId==nSSId)
		{
			if(info->rrcState==RRC_CONNECTED && info->nonGBRQueue->nSize)
			{
				unsigned int index=0;
				unsigned int size=info->nonGBRQueue->nSize;
				unsigned int nTBSIndex_DL=info->DLInfo[carrier_id].TBSIndex;
				unsigned int flag=0;
				LTE_ASSOCIATEUE_INFO peerInfo;

				NETSIM_ID peer_id;
				fn_NetSim_D2D_GetInfo_DL(info->nUEId,
					carrier_id,
					enbId,
					enbInterface,
					info,
					&peerInfo,
					&peer_id);

				nTBSIndex_DL = peerInfo.DLInfo[carrier_id].TBSIndex;
				size = size*8;
				while(index<LTE_NRB_MAX)
				{
					if ((MODE_INDEX_MAPPING[peerInfo.TransmissionIndex.DL].TRANSMISSION_MODE == SingleUser_MIMO_Spatial_Multiplexing &&
						peerInfo.DLInfo[carrier_id].modulation== Modulation_64_QAM)) 
						switch (peerInfo.DLInfo[carrier_id].Rank)
					{
						case 2:if(size<TransportBlockSize2[nTBSIndex_DL][index])
								   flag=1;
							break;
						case 3:if(size<TransportBlockSize3[nTBSIndex_DL][index])
								   flag=1;
							break;
						case 4:if(size<TransportBlockSize4[nTBSIndex_DL][index])
								   flag=1;
							break;
					}
					else if(size<TransportBlockSize1[nTBSIndex_DL][index])
					{
						flag=1;
						break;
					}
					if(flag==1)
						break;
					index++;
				}
				if(index>=LTE_NRB_MAX)
					index=LTE_NRB_MAX-1;
				nRBGcount=(int)ceil((double)(index+1)/enbMac->ca_mac[carrier_id].nRBCountInGroup);
				if(nRBGcount>enbMac->ca_mac[carrier_id].nRBGCount-enbMac->ca_mac[carrier_id].nAllocatedRBG)
					nRBGcount=enbMac->ca_mac[carrier_id].nRBGCount-enbMac->ca_mac[carrier_id].nAllocatedRBG;
				info->nonGBRQueue->nRBStart=enbMac->ca_mac[carrier_id].nAllocatedRBG;
				info->nonGBRQueue->nRBLength=nRBGcount*enbMac->ca_mac[carrier_id].nRBCountInGroup;
				if(info->nonGBRQueue->nRBLength)
				{
					if ((MODE_INDEX_MAPPING[peerInfo.TransmissionIndex.DL].TRANSMISSION_MODE == SingleUser_MIMO_Spatial_Multiplexing &&
						peerInfo.DLInfo[carrier_id].modulation== Modulation_64_QAM)) 
						switch (peerInfo.DLInfo[carrier_id].Rank)
					{
						case 2: info->nonGBRQueue->nBitcount=TransportBlockSize2[nTBSIndex_DL][info->nonGBRQueue->nRBLength-1];
							break;
						case 3: info->nonGBRQueue->nBitcount=TransportBlockSize3[nTBSIndex_DL][info->nonGBRQueue->nRBLength-1];
							break;
						case 4: info->nonGBRQueue->nBitcount=TransportBlockSize4[nTBSIndex_DL][info->nonGBRQueue->nRBLength-1];
							break;
					}
					else  info->nonGBRQueue->nBitcount=TransportBlockSize1[nTBSIndex_DL][info->nonGBRQueue->nRBLength-1];

					enbMac->ca_mac[carrier_id].nAllocatedRBG+=nRBGcount;
					info->nonGBRQueue->dPrev_throughput = 0.9*(double)info->nonGBRQueue->nBitcount + 0.1*info->nonGBRQueue->dPrev_throughput;
					
					//Form RLC SDU
					fn_NetSim_LTE_RLC_FormRLCSDU(info->nonGBRQueue,enbId,peer_id,peerInfo.HARQBuffer,carrier_id,nSSId);	
					info->carrier_id=carrier_id;
				}
				else
					info->nonGBRQueue->dPrev_throughput = 1 + 0.1*info->nonGBRQueue->dPrev_throughput;
			}
		}
		UEINFO_NEXT(info);
	}
	return 0;
}


int fn_NetSim_LTE_FormDownlinkFrame(NETSIM_ID enbId,NETSIM_ID enbInterface,unsigned int carrier_id,unsigned int nSSId)
{
	LTE_ENB_MAC* enbMac = (LTE_ENB_MAC*)DEVICE_MACVAR(enbId,enbInterface);
	LTE_ASSOCIATEUE_INFO* info;
	switch(enbMac->sScheduling)
	{
	case (RoundRobin):					
		fn_NetSim_LTE_FormDownlinkFrame_RoundRobin(enbId,enbInterface,carrier_id,nSSId);
		break;
	case (ProportionalFairScheduling):
		fn_NetSim_LTE_FormDownlinkFrame_ProportionalFairScheduling(enbId,enbInterface,carrier_id,nSSId);
		break;
	case (MaxCQI):
		fn_NetSim_LTE_FormDownlinkFrame_MaxCQI(enbId,enbInterface,carrier_id,nSSId);
		break;
	default:
		fnNetSimError("Unknown scheduling type %d in fn_NetSim_LTE_FormDownlinkFrame for BS id %d.",enbMac->sScheduling,enbId);
		break;
	}
	//Print to plot
	info=enbMac->associatedUEInfo;
	while(info)
	{
		//add to plot
		if(nLTEPlotFlag && nLTEPlotFlagList[info->nUEId-1])
		{
			double total=0;
			if(info->GBRQueue->nRBLength)
				total += (double)(info->GBRQueue->nBitcount);
			if(info->nonGBRQueue->nRBLength)
				total += (double)(info->nonGBRQueue->nBitcount);
			total=total/1000.0;//mbps
			add_plot_data_formatted(genericLTEPlot[info->nUEId-1],"%d,%0.2lf,",(int)(pstruEventDetails->dEventTime/1000),total);
		}
		info=(LTE_ASSOCIATEUE_INFO*)LIST_NEXT(info);
	}
	return 1;
}
