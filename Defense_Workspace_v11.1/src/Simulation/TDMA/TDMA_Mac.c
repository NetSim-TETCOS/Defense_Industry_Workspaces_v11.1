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


int fn_NetSim_TDMA_NodeInit()
{
	NETSIM_ID loop;
	TDMA_NODE_MAC* mac=(TDMA_NODE_MAC*)DEVICE_MACVAR(1,1);

	for(loop=0;loop<NETWORK->nDeviceCount;loop++)
	{
		TDMA_NODE_MAC* nodeMac=(TDMA_NODE_MAC*)DEVICE_MACVAR(loop+1,1);
		nodeMac->nSlotCountInFrame = (int)(mac->dFrameDuration/mac->dSlotDuration);
		nodeMac->dEpochesCount = NETWORK->pstruSimulationParameter->dVal/mac->dEpochesDuration;
		nodeMac->nFrameCount = (int)ceil(NETWORK->pstruSimulationParameter->dVal/mac->dFrameDuration);
	}
	return 0;
}
int fn_NetSim_TDMA_CreateAllocationFile()
{
	unsigned int i;
	bool flag=true;
	unsigned int nSlotId=1;
	unsigned int nFrameId=1;
	unsigned int nFrameCount;
	double dFrameTime;
	TDMA_NODE_MAC* mac=(TDMA_NODE_MAC*)DEVICE_MACVAR(1,1);
	FILE* fp;
	char fileName[BUFSIZ];
	sprintf(fileName,"%s/TDMA_ALLOCATION.txt",pszIOPath);
	//fp=fopen(fileName,"r");
	//if(fp)
	//{
	//	fclose(fp);
	//	nTDMAFileFlag=0;
	//	return 1; //file already exit
	//}
	nTDMAFileFlag=1;
	printf("Creating TDMA allocation file....\n");
	fp=fopen(fileName,"w");
	fprintf(fp,"NET_ID\tFRAME_ID\tSTART_SLOT_ID\tEND_SLOT_ID\tNODE_ID\n");
	dFrameTime=mac->dFrameDuration;
	nFrameCount=mac->nFrameCount;

	for(i=0;i<MAX_NET;i++)
	{
		bool isFound=false;
		unsigned int nRemainingSlot=mac->nSlotCountInFrame;
		flag=true;
		while(flag)
		{
			NETSIM_ID loop;
			for(loop=0;loop<NETWORK->nDeviceCount;loop++)
			{
				TDMA_NODE_MAC* nodeMac=(TDMA_NODE_MAC*)DEVICE_MACVAR(loop+1,1);
				if(nodeMac->nNetId == i+1)
				{
					isFound=true;
					if(mac->nTSBSize > nRemainingSlot)
					{
						nFrameId++;
						nRemainingSlot=mac->nSlotCountInFrame;
						nSlotId=1;
					}
					if(nFrameId>nFrameCount)
					{
						flag=false;
						break;
					}
					fprintf(fp,"%d\t%d\t%d\t%d\t%d\n",nodeMac->nNetId,
						nFrameId,
						nSlotId,
						nSlotId+mac->nTSBSize-1,
						loop+1);
					nSlotId+=mac->nTSBSize;
					nRemainingSlot -= mac->nTSBSize;
				}
			}
			if(!isFound)
				break;
		}
	}
	fclose(fp);
	return 0;
}
int fn_NetSim_TDMA_ReadAllocationFile()
{
	unsigned int nNetId;
	unsigned int nStartSlot;
	unsigned int nEndSlot;
	unsigned int nFrameId;
	unsigned int nNodeId;

	char Buffer[BUFSIZ];
	FILE* fp;
	char fileName[BUFSIZ];
	sprintf(fileName,"%s/TDMA_ALLOCATION.txt",pszIOPath);
	fp=fopen(fileName,"r");
	fgets(Buffer,BUFSIZ,fp);

	fprintf(stderr,"Reading TDMA allocation file...\n");
	while(fgets(Buffer,BUFSIZ,fp))
	{
		TDMA_NODE_MAC* mac;
		TDMA_ALLOCATION_INFO* info=ALLOCATION_INFO_ALLOC();
		sscanf(Buffer,"%d\t%d\t%d\t%d\t%d",&nNetId,&nFrameId,&nStartSlot,&nEndSlot,&nNodeId);
		info->nFrameId=nFrameId;
		info->nNodeId=(NETSIM_ID)nNodeId;
		info->nEndingSlot=nEndSlot;
		info->nStartingSlotId = nStartSlot;
		mac=(TDMA_NODE_MAC*)DEVICE_MACVAR(nNodeId,1);
		LIST_ADD_LAST((void**)&mac->allocationInfo,info);
		info->dStartTime=mac->dFrameDuration*(nFrameId-1)+(nStartSlot-1)*(mac->dSlotDuration);
		info->dEndTime=info->dStartTime+mac->dSlotDuration*(nEndSlot-nStartSlot+1);
	}
	fprintf(stderr,"TDMA allocation file reading completed.\n");
	fclose(fp);
	return 1;
}
int fn_NetSim_TDMA_ScheduleDataTransmission(NETSIM_ID nNodeId,double dTime)
{
	TDMA_NODE_MAC* mac=(TDMA_NODE_MAC*)DEVICE_MACVAR(nNodeId,1);
	if(mac->allocationInfo==NULL)
		return 1;
	pstruEventDetails->dEventTime=mac->allocationInfo->dStartTime;
	pstruEventDetails->dPacketSize=0;
	pstruEventDetails->nApplicationId=0;
	pstruEventDetails->nDeviceId=nNodeId;
	pstruEventDetails->nDeviceType=DEVICE_TYPE(nNodeId);
	pstruEventDetails->nEventType=TIMER_EVENT;
	pstruEventDetails->nInterfaceId=1;
	pstruEventDetails->nPacketId=0;
	pstruEventDetails->nProtocolId=MAC_PROTOCOL_TDMA;
	pstruEventDetails->nSegmentId=0;
	pstruEventDetails->nSubEventType=TDMA_SCHEDULE_Transmission;
	pstruEventDetails->pPacket=NULL;
	pstruEventDetails->szOtherDetails=NULL;
	fnpAddEvent(pstruEventDetails);
	return 0;
}

int fn_NetSim_TDMA_SendToPhy()
{
	NetSim_BUFFER* buffer=DEVICE_MAC_NW_INTERFACE(pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId)->pstruAccessBuffer;
	NetSim_PACKET* packet=fn_NetSim_Packet_GetPacketFromBuffer(buffer,0);
	if(!packet)
		return 1;//no packet in buffer

	fn_NetSim_FormTDMABlock();
	return 0;
}
int fn_NetSim_FormTDMABlock()
{
	TDMA_NODE_MAC* mac=(TDMA_NODE_MAC*)DEVICE_MACVAR(pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId);
	NetSim_BUFFER* buffer=DEVICE_MAC_NW_INTERFACE(pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId)->pstruAccessBuffer;
	
	double dDataRate=DATA_RATE;
	double dSize=dDataRate*(mac->allocationInfo->dEndTime-mac->allocationInfo->dStartTime)/8.0;

	double dTime=mac->allocationInfo->dStartTime;
	double dOverhead=fnGetTDMAOverHead(pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId);

	while(dSize>0.0)
	{
		double size;
		NetSim_PACKET* packet=fn_NetSim_Packet_GetPacketFromBuffer(buffer,0);
		if(!packet)
			break;
		size=fnGetPacketSize(packet);
		if(size+dOverhead>dSize)
			break;
		else
			dSize-=size+dOverhead;

		packet=fn_NetSim_Packet_GetPacketFromBuffer(buffer,1);
		packet->pstruMacData->dStartTime=dTime;
		packet->pstruMacData->dEndTime=dTime;
		packet->pstruMacData->dOverhead=TDMA_MAC_OVERHEAD;
		packet->pstruMacData->dPayload=packet->pstruNetworkData->dPacketSize;
		packet->pstruMacData->dPacketSize=packet->pstruMacData->dPayload+packet->pstruMacData->dOverhead;
		packet->pstruMacData->nMACProtocol=MAC_PROTOCOL_TDMA;
		
		//Add physical out event
		pstruEventDetails->dEventTime=dTime;
		pstruEventDetails->dPacketSize=packet->pstruMacData->dPacketSize;
		if(packet->pstruAppData)
		{
			pstruEventDetails->nApplicationId=packet->pstruAppData->nApplicationId;
			pstruEventDetails->nSegmentId=packet->pstruAppData->nSegmentId;
		}
		pstruEventDetails->nEventType=PHYSICAL_OUT_EVENT;
		pstruEventDetails->nPacketId=packet->nPacketId;
		pstruEventDetails->nProtocolId=MAC_PROTOCOL_TDMA;
		pstruEventDetails->nSubEventType=0;
		pstruEventDetails->pPacket=packet;
		fnpAddEvent(pstruEventDetails);

		dTime+=(size+dOverhead)*8.0/dDataRate;

	}
	return 0;
}
double fnGetTDMAOverHead(NETSIM_ID nNodeId,NETSIM_ID nInterfaceId)
{
	return (double)(TDMA_MAC_OVERHEAD+LINK16_PHY_OVERHEAD);
}
int fn_NetSim_TDMA_RemoveCurrentInfo()
{
	TDMA_NODE_MAC* mac=(TDMA_NODE_MAC*)DEVICE_MACVAR(pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId);
	LIST_FREE((void**)&mac->allocationInfo,mac->allocationInfo);
	return 0;
}
