/************************************************************************************
 * Copyright (C) 2016
 * TETCOS, Bangalore. India															*

 * Tetcos owns the intellectual property rights in the Product and its content.     *
 * The copying, redistribution, reselling or publication of any or all of the       *
 * Product or its content without express prior written consent of Tetcos is        *
 * prohibited. Ownership and / or any other right relating to the software and all  *
 * intellectual property rights therein shall remain at all times with Tetcos.      *
 * Author:	Shashi Kant Suman														*
 * ---------------------------------------------------------------------------------*/

#include "main.h"
#include "LTE.h"

typedef struct stru_D2DInfo
{
	bool isConnected;
	bool isRangeExtender;
	NETSIM_ID ue_id;
	NETSIM_ID connected_ue_id;

	//Transmission Info
	double dSNR;
	UINT nCQIIndex;
	UINT nMCSIndex;
	PHY_MODULATION modulation;
	UINT nTBSIndex;
	UINT Rank;
}D2D_INFO,*PD2D_INFO;

static PD2D_INFO** d2d_info;
#define D2D_UPDATE_TIME 50*MILLISECOND
#define CQI_INDEX_THRESHOLD 7 //Minimum 16-QAM

static NETSIM_ID get_first_enb()
{
	NETSIM_ID i;
	for(i=1;i<=NETWORK->nDeviceCount;i++)
		if(DEVICE_TYPE(i)==eNB && DEVICE_MACLAYER(i,1)->nMacProtocolId==MAC_PROTOCOL_LTE)
			return i;
	fnNetSimError("No eNB found in whole network\n");
	return 1;
}

void d2d_init()
{
	NETSIM_ID i,j;
	d2d_info = (PD2D_INFO**)calloc(NETWORK->nDeviceCount+1,sizeof* d2d_info);
	for(i=0;i<=NETWORK->nDeviceCount;i++)
	{
		d2d_info[i] = (PD2D_INFO*)calloc(NETWORK->nDeviceCount+1,sizeof* d2d_info[i]);
		for(j=0;j<=NETWORK->nDeviceCount;j++)
			d2d_info[i][j]=(PD2D_INFO)calloc(1,sizeof* d2d_info[i][j]);
	}

	memset(pstruEventDetails,0,sizeof* pstruEventDetails);
	pstruEventDetails->nDeviceId = get_first_enb();
	pstruEventDetails->nDeviceType = eNB;
	pstruEventDetails->nEventType=TIMER_EVENT;
	pstruEventDetails->nInterfaceId=1;
	pstruEventDetails->nProtocolId=MAC_PROTOCOL_LTE;
	pstruEventDetails->nSubEventType=UPDATE_D2D_PEERS;
	fnpAddEvent(pstruEventDetails);
}

void d2d_free()
{
	NETSIM_ID i,j;
	for(i=0;i<=NETWORK->nDeviceCount;i++)
	{
		for (j=0;j<=NETWORK->nDeviceCount;j++)
			free(d2d_info[i][j]);
		free(d2d_info[i]);
	}
	free(d2d_info);
}

static PD2D_INFO get_d2d_info(NETSIM_ID ue_id,NETSIM_ID connected_ue_id)
{
	return d2d_info[ue_id][connected_ue_id];
}

static PD2D_INFO* get_all_d2d_info(NETSIM_ID ue_id)
{
	return d2d_info[ue_id];
}

static PD2D_INFO connect_d2d(NETSIM_ID ue_id,NETSIM_ID connected_ue_id)
{
	PD2D_INFO info = get_d2d_info(ue_id,connected_ue_id);
	info->isConnected=true;
	info->connected_ue_id=connected_ue_id;
	info->ue_id=ue_id;
	return info;
}

static void disconnect_d2d(NETSIM_ID ue_id,NETSIM_ID connected_ue_id)
{
	PD2D_INFO info = get_d2d_info(ue_id,connected_ue_id);
	memset(info,0,sizeof* info);
}

static bool is_ue_in_range(NETSIM_ID ue_id,NETSIM_ID connected_ue_id,int* cqi,double* dSNR)
{
	LTE_UE_MAC* umac = get_ue_mac(ue_id,1);
	LTE_UE_PHY* uphy = get_ue_phy(ue_id,1);
	LTE_ENB_PHY* phy = get_enb_phy(umac->nENBId,umac->nENBInterface);
	double power = get_received_power_between_ue(ue_id,connected_ue_id,phy);
	double snr = get_snr_between_ue(power,phy);
	int cqiIndex=0;
	int i;

	for(i=0;i<CQI_SNR_TABLE_LEN;i++)  
	{
		if(snr<=CQI_SNR_TABLE[uphy->nTransmissionMode][i])
		{
			cqiIndex=i;
			break;
		}
	}
	if(cqiIndex>=CQI_INDEX_THRESHOLD)
	{
		*dSNR=snr;
		*cqi = cqiIndex;
		return true;
	}
	else
	{
		*dSNR=0;
		*cqi=0;
		return false;
	}
}

static bool check_range_extender(NETSIM_ID ue1,NETSIM_ID ue2)
{
	return get_ue_mac(ue1,1)->nENBId==get_ue_mac(ue2,1)->nENBId;
}

static void d2d_scan_for_peers(NETSIM_ID ue_id,NETSIM_ID ue_if)
{
	NETSIM_ID i;
	LTE_UE_MAC* mac = get_ue_mac(ue_id,ue_if);
	if(!mac->isD2D)
		return; // Not D2D enable UE

	for(i=1;i<=NETWORK->nDeviceCount;i++)
	{
		if(DEVICE_TYPE(i)==UE &&
			DEVICE_MACLAYER(i,1)->nMacProtocolId==MAC_PROTOCOL_LTE)
		{
			int cqi;
			double snr;
			LTE_UE_MAC* m = get_ue_mac(i,1);
			if(!m->isD2D || ue_id==i)
				continue; // Not D2D enable peers
			if(is_ue_in_range(ue_id,i,&cqi,&snr))
			{
				PD2D_INFO info = connect_d2d(ue_id,i);
				info->isRangeExtender = check_range_extender(ue_id,i);
				
				info->dSNR = snr;
				info->nCQIIndex = (UINT)cqi;
				info->nMCSIndex = CQI_MCS_MAPPING[cqi-1].MCS_Index;
				info->modulation= CQI_MCS_MAPPING[cqi-1].modulation;
				info->nTBSIndex = MCS_TBS_MAPPING[info->nMCSIndex].TBS_Index;
			}
			else
			{
				disconnect_d2d(ue_id,i);
			}
		}
	}
}


static void print_d2d(double time)
{
	NETSIM_ID i,j;
	static FILE* fp=NULL;
	if(!fp)
	{
		char fpath[BUFSIZ];
		sprintf(fpath,"%s/%s",pszIOPath,"D2D_Peers.txt");
		fp = fopen(fpath,"w");
		fprintf(fp,"UE Id\t\tPeers list\t\tRange Extender list\n");
	}
	fprintf(fp,"Time = %lf\n",time);
	for(i=1;i<=NETWORK->nDeviceCount;i++)
	{
		if(DEVICE_TYPE(i)==UE && 
			DEVICE_MACLAYER(i,1)->nMacProtocolId==MAC_PROTOCOL_LTE)
		{
			PD2D_INFO* info;
			LTE_UE_MAC* mac = get_ue_mac(i,1);
			if(!mac->isD2D)
				continue;
			info=get_all_d2d_info(i);
			fprintf(fp,"%d\t\t",i);
			for(j=1;j<=NETWORK->nDeviceCount;j++)
			{
				if(info[j]->isConnected)
					fprintf(fp,"%d,",j);
			}
			fprintf(fp,"\t\t");
			for(j=1;j<=NETWORK->nDeviceCount;j++)
			{
				if(info[j]->isRangeExtender)
					fprintf(fp,"%d,",j);
			}
			fprintf(fp,"\n");
		}
	}
	fprintf(fp,"\n\n");
	fflush(fp);
}

void fn_NetSim_D2D_UpdatePeers()
{
	NETSIM_ID i;
	
	//Add next event
	pstruEventDetails->dEventTime += D2D_UPDATE_TIME;
	fnpAddEvent(pstruEventDetails);

	//Call each UE to update their peers list
	for(i=0;i<NETWORK->nDeviceCount;i++)
	{
		if(DEVICE_TYPE(i+1)==UE &&
			DEVICE_MACLAYER(i+1,1)->nMacProtocolId==MAC_PROTOCOL_LTE)
			d2d_scan_for_peers(i+1,1);
	}

	print_d2d(pstruEventDetails->dEventTime-D2D_UPDATE_TIME);
}

static bool is_in_peers_list(NETSIM_ID ue_id,NETSIM_ID peer_id)
{
	return get_d2d_info(ue_id,peer_id)->isConnected;
}

static UINT get_tbs_index(NETSIM_ID ue_id,NETSIM_ID peer_id)
{
	return get_d2d_info(ue_id,peer_id)->nTBSIndex;
}

void fn_NetSim_D2D_GetTransmissionInfo(NETSIM_ID ue_id,NETSIM_ID peer_id,
	UINT* MCSIndex,double* dSNR)
{
	PD2D_INFO info = get_d2d_info(ue_id,peer_id);
	*MCSIndex = info->nMCSIndex;
	*dSNR = info->dSNR;
}

NetSim_PACKET* remove_packet_from_list(NetSim_PACKET* p, LTE_QUEUE* queue)
{
	NetSim_PACKET* h,*t=NULL;
	h=queue->head;
	while(h)
	{
		if(h==p)
		{
			if(t)
				t->pstruNextPacket=h->pstruNextPacket;
			else
				queue->head=h->pstruNextPacket;

			if(queue->tail==p)
			{
				if(t)
					queue->tail=t;
				else
					queue->tail=NULL;
			}
			queue->nSize -= (UINT)fnGetPacketSize(p);
			break;
		}
		t=h;
		h=h->pstruNextPacket;
	}
	return p;
}

int fn_NetSim_D2D_Transmit_UL(LTE_ASSOCIATEUE_INFO* info,LTE_ENB_MAC* enbMac,NETSIM_ID enbId,unsigned int carrier_id,UINT nSSId)
{
	NETSIM_ID ueId=info->nUEId;
	NETSIM_ID ueInterface=info->nUEInterface;
	LTE_UE_MAC* mac=(LTE_UE_MAC*)DEVICE_MACVAR(ueId,ueInterface);
	NetSim_PACKET* buffer=mac->nonGBRQueue->head;

	while(buffer)
	{
		unsigned int index=0;
		unsigned int size=(unsigned int)buffer->pstruMacData->dPayload;
		unsigned int nRBGcount,flag=0;
		unsigned int nTBSIndex_UL;
		NetSim_PACKET* p = buffer;
		buffer = buffer->pstruNextPacket;

		if(!is_in_peers_list(ueId,get_first_dest_from_packet(p)))
			continue;

		nTBSIndex_UL=get_tbs_index(ueId, get_first_dest_from_packet(p));
		
		size=size*8;
		
		//calculate the RB count required
		while(index<LTE_NRB_MAX)
		{
			if(size<TransportBlockSize1[nTBSIndex_UL][index])
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
			break; // Not enough resource.

		p=remove_packet_from_list(p,mac->nonGBRQueue);
		
		enbMac->ca_mac[carrier_id].nAllocatedRBG+=nRBGcount;

		//Add physical out event
		pstruEventDetails->dPacketSize=fnGetPacketSize(p);
		pstruEventDetails->nApplicationId=0;
		pstruEventDetails->nDeviceId=ueId;
		pstruEventDetails->nDeviceType=DEVICE_TYPE(ueId);
		pstruEventDetails->nEventType=PHYSICAL_OUT_EVENT;
		pstruEventDetails->nInterfaceId=ueInterface;
		pstruEventDetails->nPacketId=p->nPacketId;
		pstruEventDetails->nProtocolId=MAC_PROTOCOL_LTE;
		pstruEventDetails->nSegmentId=0;
		pstruEventDetails->nSubEventType=0;
		pstruEventDetails->pPacket=create_rlc_sdu(p,carrier_id,nSSId);
		pstruEventDetails->pPacket->pstruNextPacket=NULL;
		set_carrier_index(pstruEventDetails->pPacket,carrier_id);
		fnpAddEvent(pstruEventDetails);
	}
	return 1;
}

void fn_NetSim_D2D_GetInfo_UL(NETSIM_ID ue_id,
	UINT carrier_id,
	NETSIM_ID enbId,
	NETSIM_ID enbInterface,
	LTE_ASSOCIATEUE_INFO* info,
	LTE_ASSOCIATEUE_INFO* newInfo,
	NETSIM_ID* peer_id)
{
	if(info->ULInfo->nCQIIndex<=CQI_INDEX_THRESHOLD)
	{
		NETSIM_ID i;
		PD2D_INFO* d = get_all_d2d_info(ue_id);
		for(i=1;i<=NETWORK->nDeviceCount;i++)
		{
			if(d[i]->isConnected && d[i]->isRangeExtender)
			{
				newInfo->ULInfo[carrier_id].dSNR = d[i]->dSNR;
				newInfo->ULInfo[carrier_id].MCSIndex = d[i]->nMCSIndex;
				newInfo->ULInfo[carrier_id].modulation = d[i]->modulation;
				newInfo->ULInfo[carrier_id].nCQIIndex = d[i]->nCQIIndex;
				newInfo->ULInfo[carrier_id].Rank = d[i]->Rank;
				newInfo->ULInfo[carrier_id].TBSIndex = d[i]->nTBSIndex;
				*peer_id = i;
				return;
			}
		}
	}
	//No D2D peers found
	memcpy(newInfo,info,sizeof* newInfo);
	*peer_id = enbId;
}

void fn_NetSim_D2D_GetInfo_DL(NETSIM_ID ue_id,
	UINT carrier_id,
	NETSIM_ID enbId,
	NETSIM_ID enbInterface,
	LTE_ASSOCIATEUE_INFO* info,
	LTE_ASSOCIATEUE_INFO* newInfo,
	NETSIM_ID* peer_id)
{
	if(info->ULInfo->nCQIIndex<=CQI_INDEX_THRESHOLD)
	{
		NETSIM_ID i;
		PD2D_INFO* d = get_all_d2d_info(ue_id);
		for(i=1;i<=NETWORK->nDeviceCount;i++)
		{
			if(d[i]->isConnected && d[i]->isRangeExtender)
			{
				LTE_ASSOCIATEUE_INFO* pinfo = fn_NetSim_LTE_FindInfo(get_enb_mac(enbId,enbInterface),i);
				memcpy(newInfo,pinfo,sizeof* newInfo);
				*peer_id = i;
				return;
			}
		}
	}
	//No D2D peers found
	memcpy(newInfo,info,sizeof* newInfo);
	*peer_id = ue_id;
}
