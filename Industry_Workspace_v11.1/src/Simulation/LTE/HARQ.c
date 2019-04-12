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
#include "RLC.h"
#define NO_HARQ_PROCESS
#define LTE_ACK_SIZE 1 //bytes
__inline unsigned int fn_NetSim_LTE_GetHARQIndex(double dTime)
{
	return (unsigned int)(((unsigned int)(dTime/1000))%MAX_HARQ_PROCESS);
}

NetSim_PACKET*** alloc_HARQ()
{
	NetSim_PACKET*** p;
	int i;
	p=(NetSim_PACKET***)calloc(MAX_HARQ_PROCESS,sizeof* p);
	for (i=0;i<MAX_HARQ_PROCESS;i++)
	{
		p[i]=(NetSim_PACKET**)calloc(MAX_CA_COUNT,sizeof* p[i]);
	}
	return p;
}
void free_HARQ(NetSim_PACKET*** p)
{
	int i;
	for(i=0;i<MAX_HARQ_PROCESS;i++)
	{
		int j;
		for(j=0;j<MAX_CA_COUNT;j++)
		{
			NetSim_PACKET* t=p[i][j];
			NetSim_PACKET* t1;
			while(t)
			{
				t1=t->pstruNextPacket;
				fn_NetSim_Packet_FreePacket(t);
				t=t1;
			}
		}
		free(p[i]);
	}
	free(p);
}
int fn_NetSim_LTE_AddToHARQBuffer(NetSim_PACKET*** HARQBuffer,NetSim_PACKET* packet,double dTime,unsigned int carrier_id)
{
#ifndef NO_HARQ_PROCESS
	unsigned int index=fn_NetSim_LTE_GetHARQIndex(dTime);
	NetSim_PACKET* list=HARQBuffer[index][carrier_id];
	if(!HARQBuffer[index][carrier_id])
	{
		HARQBuffer[index][carrier_id]=fn_NetSim_Packet_CopyPacket(packet);
	}
	else
	{
		while(list->pstruNextPacket)
			list=list->pstruNextPacket;

		list->pstruNextPacket=fn_NetSim_Packet_CopyPacket(packet);
	}
#endif
	return 0;
}
int fn_NetSim_LTE_SendAck(NetSim_PACKET* rlcSDU,NetSim_EVENTDETAILS* eventDetail)
{
	NetSim_EVENTDETAILS pevent;
	//Generate LTE Ack
	NetSim_PACKET* packet=fn_NetSim_LTE_CreateCtrlPacket(eventDetail->dEventTime+4000,
		LTEPacket_ACK,
		rlcSDU->nTransmitterId,
		eventDetail->nDeviceId,
		rlcSDU->nTransmitterId,
		LTE_ACK_SIZE);
	LTE_ACK* ack=(LTE_ACK*)calloc(1,sizeof* ack);
	packet->pstruMacData->Packet_MACProtocol=ack;
	ack->SN=((RLC_PDU*)rlcSDU->pstruMacData->Packet_MACProtocol)->header->SN;
	packet->nPacketId=ack->SN;
	//Add physical out event
	memcpy(&pevent,eventDetail,sizeof pevent);
	pevent.dEventTime+=4000;
	pevent.dPacketSize=1;
	pevent.nApplicationId=0;
	pevent.nEventType=PHYSICAL_OUT_EVENT;
	pevent.nPacketId=ack->SN;
	pevent.nProtocolId=MAC_PROTOCOL_LTE;
	pevent.nSegmentId=0;
	pevent.nSubEventType=0;
	pevent.pPacket=packet;
	set_carrier_index(packet,get_carrier_index(rlcSDU));
	pevent.szOtherDetails=NULL;
	fnpAddEvent(&pevent);
	return 1;
}

int fn_NetSim_LTE_ENB_ProcessAck()
{
	NetSim_PACKET* list,*temp=NULL;
	unsigned int index=fn_NetSim_LTE_GetHARQIndex(pstruEventDetails->dEventTime-4000);
	NetSim_PACKET* packet=pstruEventDetails->pPacket;
	unsigned int carrier_id = get_carrier_index(packet);
	LTE_ASSOCIATEUE_INFO* info=fn_NetSim_LTE_FindInfo(get_current_enb_mac(),packet->nSourceId);
	if(info==NULL)
	{
		assert(false);
		return -1;
	}
	list=info->HARQBuffer[index][carrier_id];
	while(list)
	{
		RLC_PDU* rlc=list->pstruMacData->Packet_MACProtocol;
		LTE_ACK* ack=packet->pstruMacData->Packet_MACProtocol;
		if(rlc->header->SN==ack->SN)
		{
			//remove packet
			if(temp==NULL)
			{
				info->HARQBuffer[index][carrier_id]=list->pstruNextPacket;
			}
			else
				temp->pstruNextPacket=list->pstruNextPacket;
			list->pstruNextPacket=NULL;
			fn_NetSim_Packet_FreePacket(list);
			break;
		}
		temp=list;
		list=list->pstruNextPacket;
	}
	//Free ack packet
	fn_NetSim_Packet_FreePacket(packet);
	return 0;
}
int fn_NetSim_LTE_UE_ProcessAck()
{
	NetSim_PACKET* list,*temp=NULL;
	unsigned int index=fn_NetSim_LTE_GetHARQIndex(pstruEventDetails->dEventTime-4000);
	NetSim_PACKET* packet=pstruEventDetails->pPacket;
	unsigned int carrier_id = get_carrier_index(packet);
	LTE_UE_MAC* mac=get_current_ue_mac();
	if(mac==NULL)
	{
		assert(false);
		return -1;
	}
	list=mac->HARQBuffer[index][carrier_id];
	while(list)
	{
		RLC_PDU* rlc=list->pstruMacData->Packet_MACProtocol;
		LTE_ACK* ack=packet->pstruMacData->Packet_MACProtocol;
		if(rlc->header->SN==ack->SN)
		{
			//remove packet
			if(temp==NULL)
			{
				mac->HARQBuffer[index][carrier_id]=list->pstruNextPacket;
			}
			else
				temp->pstruNextPacket=list->pstruNextPacket;
			list->pstruNextPacket=NULL;
			fn_NetSim_Packet_FreePacket(list);
			break;
		}
		temp=list;
		list=list->pstruNextPacket;
	}
	//Free ack packet
	fn_NetSim_Packet_FreePacket(packet);
	return 0;
}
int fn_NetSim_LTE_HARQ_RetransmitHARQBuffer_DL(LTE_ASSOCIATEUE_INFO* info,LTE_ENB_MAC* enbMac,NETSIM_ID enbId,unsigned int carrier_id)
{
	unsigned int n=fn_NetSim_LTE_GetHARQIndex(pstruEventDetails->dEventTime);
	NetSim_PACKET* buffer=info->HARQBuffer[n][carrier_id];
	if(info->carrier_id==-1 || info->carrier_id==(int)carrier_id)
	{
		while(buffer)
		{
			unsigned int index=0;
			unsigned int size=(unsigned int)buffer->pstruMacData->dPayload;
			unsigned int nTBSIndex_DL=info->DLInfo[carrier_id].TBSIndex;
			unsigned int nRBGcount,flag=0;
			size=size*8;
			//calculate the RB count required
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
			enbMac->ca_mac[carrier_id].nAllocatedRBG+=nRBGcount;

			info->carrier_id=carrier_id;
			//Add physical out event
			pstruEventDetails->dPacketSize=fnGetPacketSize(buffer);
			pstruEventDetails->nApplicationId=0;
			pstruEventDetails->nDeviceId=enbId;
			pstruEventDetails->nDeviceType=DEVICE_TYPE(enbId);
			pstruEventDetails->nEventType=PHYSICAL_OUT_EVENT;
			pstruEventDetails->nInterfaceId=1;
			pstruEventDetails->nPacketId=buffer->nPacketId;
			pstruEventDetails->nProtocolId=MAC_PROTOCOL_LTE;
			pstruEventDetails->nSegmentId=0;
			pstruEventDetails->nSubEventType=0;
			pstruEventDetails->pPacket=fn_NetSim_Packet_CopyPacket(buffer);
			pstruEventDetails->pPacket->pstruNextPacket=NULL;
			fnpAddEvent(pstruEventDetails);
			buffer=buffer->pstruNextPacket;
		}
	}
	return 1;
}
int fn_NetSim_LTE_HARQ_RetransmitHARQBuffer_UL(LTE_ASSOCIATEUE_INFO* info,LTE_ENB_MAC* enbMac,NETSIM_ID enbId,unsigned int carrier_id)
{
	NETSIM_ID ueId=info->nUEId;
	NETSIM_ID ueInterface=info->nUEInterface;
	LTE_UE_MAC* mac=(LTE_UE_MAC*)DEVICE_MACVAR(ueId,ueInterface);
	unsigned int n=fn_NetSim_LTE_GetHARQIndex(pstruEventDetails->dEventTime);
	NetSim_PACKET* buffer=mac->HARQBuffer[n][carrier_id];
	while(buffer)
	{
		unsigned int index=0;
		unsigned int size=(unsigned int)buffer->pstruMacData->dPayload;
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
		enbMac->ca_mac[carrier_id].nAllocatedRBG+=nRBGcount;

		//Add physical out event
		pstruEventDetails->dPacketSize=fnGetPacketSize(buffer);
		pstruEventDetails->nApplicationId=0;
		pstruEventDetails->nDeviceId=enbId;
		pstruEventDetails->nDeviceType=DEVICE_TYPE(enbId);
		pstruEventDetails->nEventType=PHYSICAL_OUT_EVENT;
		pstruEventDetails->nInterfaceId=1;
		pstruEventDetails->nPacketId=buffer->nPacketId;
		pstruEventDetails->nProtocolId=MAC_PROTOCOL_LTE;
		pstruEventDetails->nSegmentId=0;
		pstruEventDetails->nSubEventType=0;
		pstruEventDetails->pPacket=fn_NetSim_Packet_CopyPacket(buffer);
		pstruEventDetails->pPacket->pstruNextPacket=NULL;
		set_carrier_index(pstruEventDetails->pPacket,carrier_id);
		fnpAddEvent(pstruEventDetails);
		buffer=buffer->pstruNextPacket;
	}
	return 1;
}

