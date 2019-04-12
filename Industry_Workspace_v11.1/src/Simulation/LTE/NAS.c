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
#define MEASUREMENT_REPORT_SIZE 184/8.0
#define HO_REQUEST_SIZE 288/8.0
#define HO_CONFIRM_SIZE 112/8.0
#define HANDOVER_DIFF	3 //db

int fn_NetSim_LTE_InitHandover(NETSIM_ID ueId,NETSIM_ID nENBId)
{
	//Prepare the measurement report
	NetSim_PACKET* packet;
	LTE_MAC_PACKET* macPacket;
	LTE_PHY_PACKET* phyPacket;
	LTE_MEASUREMENT_REPORT* report=NULL;
	NETSIM_ID i;
	for(i=0;i<NETWORK->nDeviceCount;i++)	
	{
		if(DEVICE_TYPE(i+1) == eNB)
		{
			unsigned int j;
			LTE_MEASUREMENT_REPORT* temp=MEASUREMENT_REPORT_ALLOC();
			LTE_ASSOCIATEUE_INFO* info=UEINFO_ALLOC();
			LTE_ENB_PHY* enbPhy=(LTE_ENB_PHY*)DEVICE_PHYVAR(i+1,1);
			info->nUEId=ueId;
			info->nUEInterface=1;
			temp->nENBId=i+1;
			temp->nUEId=ueId;
			temp->carrier_count = enbPhy->ca_count;
			for(j=0;j<enbPhy->ca_count;j++)
			{

				fn_NetSim_LTE_CalculateRxPower(i+1,1,info,j);
				fn_NetSim_LTE_CalculateSNR(i+1,1,info,j);
				fn_NetSim_LTE_GetCQIIndex(i+1,1,info,j);
				fn_NetSim_LTE_GetMCS_TBS_Index(info,j);
				while(info->DLInfo[j].nCQIIndex>1 && info->ULInfo[j].nCQIIndex>1)
				{
					double ber;
					fn_NetSim_LTE_GetMCS_TBS_Index(info,j);
					ber = fn_NetSim_LTE_CalculateBER(0,info->DLInfo[j].MCSIndex,info->DLInfo[j].dSNR);
					if(ber<TARGET_BER)
						break;
					else
					{
						info->DLInfo[j].nCQIIndex--;
						info->ULInfo[j].nCQIIndex--;
					}
				}
				temp->nCQIIndex_DL[j]=info->DLInfo[j].nCQIIndex;
				temp->dSNR_DL[j]=info->DLInfo[j].dSNR;
			}
			LIST_FREE((void**)&info,info);
			LIST_ADD_LAST((void**)&report,temp);
		}
	}
	if(report)
	{
		packet=fn_NetSim_LTE_CreateCtrlPacket(pstruEventDetails->dEventTime,
			LTEPacket_MeasurementReport,
			nENBId,
			ueId,
			nENBId,
			MEASUREMENT_REPORT_SIZE);
		macPacket=calloc(1,sizeof* macPacket);
		macPacket->logicalChannel=LogicalChannel_CCCH;
		macPacket->MessageType=LTEPacket_MeasurementReport;
		macPacket->MessageVar=report;
		macPacket->transportChannel=TransportChannel_RACH;
		phyPacket=PACKET_PHYPROTOCOLDATA(packet);
		phyPacket->physicalChannel=PhysicalChannel_PRACH;

		packet->pstruMacData->Packet_MACProtocol=macPacket;
		packet->pstruPhyData->Packet_PhyData=phyPacket;

		//Add physical out event
		pstruEventDetails->nDeviceId=ueId;
		pstruEventDetails->nDeviceType=UE;
		pstruEventDetails->nInterfaceId=1;
		pstruEventDetails->nProtocolId=MAC_PROTOCOL_LTE;
		pstruEventDetails->dPacketSize=MEASUREMENT_REPORT_SIZE;
		pstruEventDetails->nApplicationId=0;
		pstruEventDetails->nEventType=PHYSICAL_OUT_EVENT;
		pstruEventDetails->nPacketId=0;
		pstruEventDetails->nSegmentId=0;
		pstruEventDetails->nSubEventType=0;
		pstruEventDetails->pPacket=packet;
		pstruEventDetails->szOtherDetails=NULL;
		fnpAddEvent(pstruEventDetails);
	}
	return 0;
}
int fn_NetSim_LTE_MME_RoutePacket()
{
	NetSim_BUFFER* buffer;

	NetSim_PACKET* packet=pstruEventDetails->pPacket;
	LTE_MME* mme=NETWORK->ppstruDeviceList[pstruEventDetails->nDeviceId-1]->deviceVar;
	if(mme)
	{
		LTE_HLR* hlr=mme->HLR;
		NETSIM_ID ueId = get_first_dest_from_packet(pstruEventDetails->pPacket);
		while(hlr)
		{
			if(hlr->nUEId==ueId)
			{
				pstruEventDetails->nInterfaceId=hlr->nMMEInterface;
				break;
			}
			hlr=LIST_NEXT(hlr);
		}
	}
	//Add macout event
	buffer = NETWORK->ppstruDeviceList[pstruEventDetails->nDeviceId-1]->ppstruInterfaceList[pstruEventDetails->nInterfaceId-1]->pstruAccessInterface->pstruAccessBuffer;
		
				
	if(!fn_NetSim_GetBufferStatus(buffer))
	{
		//Add the MAC out event
		pstruEventDetails->dPacketSize = packet->pstruNetworkData->dPacketSize;
		if(packet->pstruAppData)
		{
			pstruEventDetails->nApplicationId = packet->pstruAppData->nApplicationId;
			pstruEventDetails->nSegmentId = packet->pstruAppData->nSegmentId;
		}
		pstruEventDetails->nEventType = MAC_OUT_EVENT;
		pstruEventDetails->nPacketId = packet->nPacketId;
		pstruEventDetails->nProtocolId = fn_NetSim_Stack_GetMacProtocol(pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId);
		pstruEventDetails->nSubEventType = 0;
		pstruEventDetails->pPacket = NULL;
		pstruEventDetails->szOtherDetails = NULL;
		fnpAddEvent(pstruEventDetails);
	}
	fn_NetSim_Packet_AddPacketToList(buffer,packet,0);
	return 0;
}

double min_elementd(double* p,unsigned int c)
{
	unsigned int i;
	double m=p[0];
	for(i=0;i<c;i++)
		m=min(m,p[i]);
	return m;
}

int fn_NetSim_LTE_DecideHandover()
{
	double dSNR=0;
	double dTargetSNR=0;
	NETSIM_ID target=0;
	NETSIM_ID ueId=0;
	LTE_MEASUREMENT_REPORT* report=(LTE_MEASUREMENT_REPORT*)((LTE_MAC_PACKET*)pstruEventDetails->pPacket->pstruMacData->Packet_MACProtocol)->MessageVar;
	while(report)
	{
		if(report->nENBId==pstruEventDetails->nDeviceId)
		{
			dSNR= min_elementd(report->dSNR_DL,report->carrier_count);
			ueId=report->nUEId;
			break;
		}
		report=LIST_NEXT(report);
	}
	report=(LTE_MEASUREMENT_REPORT*)((LTE_MAC_PACKET*)pstruEventDetails->pPacket->pstruMacData->Packet_MACProtocol)->MessageVar;
	while(report)
	{
		if(report->nENBId!=pstruEventDetails->nDeviceId)
		{
			double d = min_elementd(report->dSNR_DL,report->carrier_count);
			if( d >= dSNR+HANDOVER_DIFF &&
				d>dTargetSNR &&
				isPartofSameMME(pstruEventDetails->nDeviceId,
				report->nENBId))
			{
				target=report->nENBId;
				dTargetSNR=d;
			}
		}
		report=LIST_NEXT(report);
	}
	//Free measurement report packet
	fn_NetSim_Packet_FreePacket(pstruEventDetails->pPacket);
	if(target && pstruEventDetails->nDeviceType!=RELAY && DEVICE_TYPE(target)!=RELAY)
	{
		NetSim_BUFFER* buffer;
		NetSim_PACKET* packet;
		LTE_MAC_PACKET* macPacket=(LTE_MAC_PACKET*)calloc(1,sizeof* macPacket);
		LTE_HO_INFO* request=(LTE_HO_INFO*)calloc(1,sizeof* request);
		packet=fn_NetSim_LTE_CreateCtrlPacket(pstruEventDetails->dEventTime,
			LTEPacket_HandoverRequest,
			target,
			pstruEventDetails->nDeviceId,
			target,
			HO_REQUEST_SIZE);
		macPacket->logicalChannel=LogicalChannel_CCCH;
		macPacket->MessageType=LTEPacket_HandoverRequest;
		packet->pstruMacData->Packet_MACProtocol=macPacket;
		packet->pstruMacData->dontFree = true;
		macPacket->MessageVar=request;
		request->nSourceENB=pstruEventDetails->nDeviceId;
		request->nTargetENB=target;
		request->nUEId=ueId;
		request->info=fn_NetSim_LTE_FindInfo(DEVICE_MACVAR(pstruEventDetails->nDeviceId,1),ueId);
		((LTE_HO_INFO*)macPacket->MessageVar)->sourceMac=DEVICE_MACVAR(pstruEventDetails->nDeviceId,1);

		//Add macout event
		buffer = NETWORK->ppstruDeviceList[pstruEventDetails->nDeviceId-1]->ppstruInterfaceList[1]->pstruAccessInterface->pstruAccessBuffer;
		
				
		if(!fn_NetSim_GetBufferStatus(buffer))
		{
			//Add the MAC out event
			pstruEventDetails->dPacketSize = HO_REQUEST_SIZE;
			pstruEventDetails->nEventType = MAC_OUT_EVENT;
			pstruEventDetails->nPacketId = packet->nPacketId;
			pstruEventDetails->nProtocolId = fn_NetSim_Stack_GetMacProtocol(pstruEventDetails->nDeviceId,2);
			pstruEventDetails->nSubEventType = 0;
			pstruEventDetails->pPacket = NULL;
			pstruEventDetails->szOtherDetails = NULL;
			pstruEventDetails->nApplicationId=0;
			pstruEventDetails->nInterfaceId=2;
			pstruEventDetails->nSegmentId=0;
			fnpAddEvent(pstruEventDetails);
		}
		fn_NetSim_Packet_AddPacketToList(buffer,packet,0);
	}
	return 1;
}
int fn_NetSim_LTE_MME_RouteHOPacket()
{
	NetSim_EVENTDETAILS pevent;
	NetSim_BUFFER* buffer;
	NetSim_PACKET* packet=pstruEventDetails->pPacket;
	LTE_MME* mme=NETWORK->ppstruDeviceList[pstruEventDetails->nDeviceId-1]->deviceVar;
	memcpy(&pevent,pstruEventDetails,sizeof pevent);
	if(mme)
	{
		NETSIM_ID enbId= get_first_dest_from_packet(pevent.pPacket);
		NETSIM_ID i;
		NETSIM_ID j,k;
		for(i=0;i<NETWORK->ppstruDeviceList[pstruEventDetails->nDeviceId-1]->nNumOfInterface;i++)
		{
			fn_NetSim_Stack_GetConnectedDevice(pstruEventDetails->nDeviceId,i+1,&j,&k);
			if(j==enbId)
			{
				pevent.nInterfaceId = i+1;
				break;
			}
		}
	}
	//Add macout event
	buffer = NETWORK->ppstruDeviceList[pevent.nDeviceId-1]->ppstruInterfaceList[pevent.nInterfaceId-1]->pstruAccessInterface->pstruAccessBuffer;
		
				
	if(!fn_NetSim_GetBufferStatus(buffer))
	{
		//Add the MAC out event
		pevent.nEventType = MAC_OUT_EVENT;
		pevent.nPacketId = packet->nPacketId;
		pevent.nProtocolId = fn_NetSim_Stack_GetMacProtocol(pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId);
		pevent.nSubEventType = 0;
		pevent.pPacket = NULL;
		pevent.szOtherDetails = NULL;
		fnpAddEvent(&pevent);
	}
	packet->nTransmitterId=pevent.nDeviceId;
	fn_NetSim_Packet_AddPacketToList(buffer,packet,0);
	pstruEventDetails->pPacket=NULL;
	return 0;
}
int fn_NetSim_LTE_ProcessHORequest()
{
	NetSim_PACKET* packet;
	LTE_MAC_PACKET* macPacket=calloc(1,sizeof* macPacket);
	NetSim_BUFFER* buffer=DEVICE_MAC_NW_INTERFACE(pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId)->pstruAccessBuffer;
	NetSim_PACKET* request=fn_NetSim_Packet_GetPacketFromBuffer(buffer,1);
					
	packet=fn_NetSim_LTE_CreateCtrlPacket(pstruEventDetails->dEventTime,
		LTEPacket_HandoverRequestAck,
		request->nSourceId,
		pstruEventDetails->nDeviceId,
		request->nSourceId,
		HO_CONFIRM_SIZE);
	macPacket->logicalChannel=LogicalChannel_CCCH;
	macPacket->MessageType=LTEPacket_HandoverRequestAck;
	packet->pstruMacData->Packet_MACProtocol=macPacket;
	packet->pstruMacData->dontFree = true;
	macPacket->MessageVar=((LTE_MAC_PACKET*)request->pstruMacData->Packet_MACProtocol)->MessageVar;
	((LTE_HO_INFO*)macPacket->MessageVar)->targetMac=DEVICE_MACVAR(pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId);
	//Add macout event
	buffer = NETWORK->ppstruDeviceList[pstruEventDetails->nDeviceId-1]->ppstruInterfaceList[1]->pstruAccessInterface->pstruAccessBuffer;


	if(!fn_NetSim_GetBufferStatus(buffer))
	{
		//Add the MAC out event
		pstruEventDetails->dPacketSize = HO_CONFIRM_SIZE;
		pstruEventDetails->nEventType = MAC_OUT_EVENT;
		pstruEventDetails->nPacketId = packet->nPacketId;
		pstruEventDetails->nProtocolId = fn_NetSim_Stack_GetMacProtocol(pstruEventDetails->nDeviceId,2);
		pstruEventDetails->nSubEventType = 0;
		pstruEventDetails->pPacket = NULL;
		pstruEventDetails->szOtherDetails = NULL;
		pstruEventDetails->nApplicationId=0;
		pstruEventDetails->nInterfaceId=2;
		pstruEventDetails->nSegmentId=0;
		fnpAddEvent(pstruEventDetails);
	}
	fn_NetSim_Packet_AddPacketToList(buffer,packet,0);
	return 1;
}
int fn_NetSim_LTE_ProcessHORequestAck()
{
	LTE_UE_MAC* ueMac;
	NETSIM_ID nMMEId,nMMEInterface;
	LTE_HLR* hlr;
	NetSim_BUFFER* buffer=DEVICE_MAC_NW_INTERFACE(pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId)->pstruAccessBuffer;
	NetSim_PACKET* ack=fn_NetSim_Packet_GetPacketFromBuffer(buffer,1);
	LTE_HO_INFO* info=(LTE_HO_INFO*)((LTE_MAC_PACKET*)ack->pstruMacData->Packet_MACProtocol)->MessageVar;
	
	//Remove from source enb
	LIST_REMOVE((void**)&info->sourceMac->associatedUEInfo,info->info);
	info->sourceMac->nAssociatedUECount--;
	
	//Add to target enb
	LIST_ADD_LAST((void**)&info->targetMac->associatedUEInfo,info->info);
	info->targetMac->nAssociatedUECount++;
	
	//Link correction
	fn_NetSim_Stack_RemoveDeviceFromlink(info->nUEId,1,DEVICE_PHYLAYER(info->nSourceENB,1)->nLinkId);
	fn_NetSim_Stack_AddDeviceTolink(info->nUEId,1,DEVICE_PHYLAYER(info->nTargetENB,1)->nLinkId);
	//Path switch
	fn_NetSim_Stack_GetConnectedDevice(info->nSourceENB,2,&nMMEId,&nMMEInterface);
	hlr=((LTE_MME*)NETWORK->ppstruDeviceList[nMMEId-1]->deviceVar)->HLR;
	while(hlr)
	{
		if(hlr->nUEId==info->nUEId)
		{
			hlr->nENBId=info->nTargetENB;
			hlr->nENBInterface=2;
			fn_NetSim_Stack_GetConnectedDevice(hlr->nENBId,hlr->nENBInterface,&hlr->nMMEID,&hlr->nMMEInterface);
			break;
		}
		hlr=(LTE_HLR*)LIST_NEXT(hlr);
	}

	//Change the UE association
	ueMac=(LTE_UE_MAC*)DEVICE_MACVAR(info->nUEId,1);
	ueMac->nENBId=info->nTargetENB;
	ueMac->nENBInterface=1;
	//Update metrics
	ueMac->metrics.nHandoverCount++;
	fn_NetSim_Packet_FreePacket(ack);
	return 1;
}



