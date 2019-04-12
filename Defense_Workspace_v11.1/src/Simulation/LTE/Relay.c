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
#include "LTE.h"

NETSIM_ID get_rcv_interface_id(NetSim_PACKET* p)
{
	switch (DEVICE_TYPE(p->nTransmitterId))
	{
	case UE:
		{
			switch(DEVICE_TYPE(p->nReceiverId))
			{
				case UE:
					return get_UE_Interface(p->nReceiverId);
				default:
					return get_eNB_Interface(p->nReceiverId);
			}
		}
	case eNB:
		return get_UE_Interface(p->nReceiverId);
	case RELAY:
		{
			switch(DEVICE_TYPE(p->nReceiverId))
			{
			case UE:
				return get_UE_Interface(p->nReceiverId);
			case eNB:
				return get_eNB_Interface(p->nReceiverId);
			}
		}
	}
	return 0;
}

NETSIM_ID get_tx_interface_id(NetSim_PACKET* p)
{
	switch (DEVICE_TYPE(p->nTransmitterId))
	{
	case UE:
		return get_UE_Interface(p->nTransmitterId);
	case eNB:
		return get_eNB_Interface(p->nTransmitterId);
	case RELAY:
		{
			switch(DEVICE_TYPE(p->nReceiverId))
			{
			case UE:
				return get_eNB_Interface(p->nTransmitterId);
			case eNB:
				return get_UE_Interface(p->nTransmitterId);
			}
		}
	}
	return 0;
}

bool find_info_from_relay(NETSIM_ID relayId,NETSIM_ID destId)
{
	NETSIM_ID ifid = get_eNB_Interface(relayId);
	LTE_ASSOCIATEUE_INFO* info=get_enb_mac(relayId,ifid)->associatedUEInfo;
	while(info)
	{
		if(info->nUEId == destId)
			return true;
		info=(LTE_ASSOCIATEUE_INFO*)LIST_NEXT(info);
	}
	return false;
}

void fn_NetSim_LTE_InitRelay(NETSIM_ID relayId)
{
	int i=0;
	NETSIM_ID enbId;
	NETSIM_ID enbInterface;
	LTE_ASSOCIATEUE_INFO *info,*rinfo;
	NETSIM_ID relay_enb_if_id = get_eNB_Interface(relayId);
	NETSIM_ID relay_ue_if_id = get_UE_Interface(relayId);
	LTE_ENB_MAC* relay_enb_mac = get_enb_mac(relayId,relay_enb_if_id);

	LTE_UE_MAC* relay_ue_mac = get_ue_mac(relayId,relay_ue_if_id);
	enbId = relay_ue_mac->nENBId;
	enbInterface = relay_ue_mac->nENBInterface;
	info = fn_NetSim_LTE_FindInfo(get_enb_mac(enbId,enbInterface),relayId);
	info->connectedUECount = relay_enb_mac->nAssociatedUECount;
	info->connectedUEId = (NETSIM_ID *)calloc(info->connectedUECount,sizeof(NETSIM_ID));
	info->connectedUEInterface = (NETSIM_ID *)calloc(info->connectedUECount,sizeof(NETSIM_ID));

	rinfo = relay_enb_mac->associatedUEInfo;
	while(rinfo)
	{
		info->connectedUEId[i]=rinfo->nUEId;
		info->connectedUEInterface[i]=rinfo->nUEInterface;
		i++;
		rinfo=(LTE_ASSOCIATEUE_INFO*)LIST_NEXT(rinfo);
	}
}

int fn_NetSim_LTE_Relay_ForwardPacket(NetSim_PACKET* packet,NETSIM_ID relayId,NETSIM_ID relayInterface)
{
	NETSIM_ID newif=relayInterface==get_eNB_Interface(relayId)?get_UE_Interface(relayId):get_eNB_Interface(relayId);
	NetSim_BUFFER* buf = DEVICE_ACCESSBUFFER(relayId,newif);
	if(!fn_NetSim_GetBufferStatus(buf))
	{
		pstruEventDetails->nInterfaceId=newif;
		pstruEventDetails->dPacketSize=fnGetPacketSize(packet);
		if(packet->pstruAppData)
		{
			pstruEventDetails->nApplicationId=packet->pstruAppData->nApplicationId;
			pstruEventDetails->nSegmentId=packet->pstruAppData->nSegmentId;
		}
		pstruEventDetails->nEventType=MAC_OUT_EVENT;
		pstruEventDetails->nPacketId=packet->nPacketId;
		pstruEventDetails->nSubEventType=0;
		pstruEventDetails->pPacket=packet;
		fnpAddEvent(pstruEventDetails);
	}
	fn_NetSim_Packet_AddPacketToList(buf,packet,0);
	return 0;
}
