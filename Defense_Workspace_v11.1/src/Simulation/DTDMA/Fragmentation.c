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
#include "DTDMA_enum.h"
#include "DTDMA.h"

NetSim_PACKET* get_packet_from_buffer(NETSIM_ID dev_id,NETSIM_ID if_id,int flag)
{
	PDTDMA_NODE_MAC mac = DTDMA_MAC(dev_id,if_id);
	NetSim_PACKET* packet;
	NetSim_BUFFER* buffer=DEVICE_MAC_NW_INTERFACE(dev_id,if_id)->pstruAccessBuffer;
	if(mac->fragment)
	{
		packet = mac->fragment;
		if(flag)
			mac->fragment=NULL;
	}
	else
	{
		packet = fn_NetSim_Packet_GetPacketFromBuffer(buffer,flag);
		if(packet)
		{
			packet->pstruMacData->dOverhead=0;
			packet->pstruMacData->dPayload=packet->pstruNetworkData->dPacketSize;
			packet->pstruMacData->dPacketSize=packet->pstruMacData->dPayload+packet->pstruMacData->dOverhead;
			packet->pstruMacData->nMACProtocol=MAC_PROTOCOL_DTDMA;
		}
	}
	return packet;
}

NetSim_PACKET* get_packet_from_buffer_with_size(NETSIM_ID dev_id,NETSIM_ID if_id,double size)
{
	int g;
	PDTDMA_MAC_PACKET mp;
	PDTDMA_NODE_MAC mac = DTDMA_MAC(dev_id,if_id);
	NetSim_PACKET* packet = get_packet_from_buffer(dev_id,if_id,1);
	NetSim_PACKET* ret;
	double siz = fnGetPacketSize(packet);
	if(siz<=size)
	{
		g=1;
		ret = packet;
	}
	else
	{
		g=0;
		ret = fn_NetSim_Packet_CopyPacket(packet);
		ret->pstruMacData->dPacketSize=size;
		packet->pstruMacData->dPacketSize-=size;
		mac->fragment=packet;
	}
	mp=(PDTDMA_MAC_PACKET)ret->pstruMacData->Packet_MACProtocol;
	if(mp)
	{
		if(g)
			mp->frag=LAST_FRAGMENT;
		else
			mp->frag=CONTINUE_FRAGMENT;
		mp->fragment_id = mac->fragment_id;
		mp->segment_id = ++mac->segment_id;
	}
	else if(!g)
	{
		mp=(PDTDMA_MAC_PACKET)calloc(1,sizeof* mp);
		ret->pstruMacData->Packet_MACProtocol=mp;
		mp->frag = FIRST_FRAGMENT;
		mp->fragment_id = ++mac->fragment_id;
		mp->segment_id=1;
		mac->segment_id=1;
		packet->pstruMacData->Packet_MACProtocol = calloc(1,sizeof* mp);
		memcpy(packet->pstruMacData->Packet_MACProtocol,mp,sizeof* mp);
	}
	return ret;
}

void reassemble_packet()
{
	PDTDMA_NODE_MAC mac = DTDMA_MAC(pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId);
	NetSim_PACKET* packet = pstruEventDetails->pPacket;
	PDTDMA_MAC_PACKET mp = (PDTDMA_MAC_PACKET)packet->pstruMacData->Packet_MACProtocol;
	NETSIM_ID tx = packet->nTransmitterId;
	NetSim_PACKET* resamble = mac->reassembly[tx-1];
	if(!mp)
		goto RETURN; //NO Fragmentation
	if(!resamble)
	{
		if(mp->frag==FIRST_FRAGMENT)
		{
			mac->reassembly[tx-1]=packet;
			goto WAIT_FOR_NEXT;
		}
		else
			goto DISCARD_PACKET;
	}
	else
	{
		PDTDMA_MAC_PACKET mpr = (PDTDMA_MAC_PACKET)resamble->pstruMacData->Packet_MACProtocol;
		if(mpr->fragment_id!=mp->fragment_id ||
			mpr->segment_id+1!=mp->segment_id)
		{
			fn_NetSim_Packet_FreePacket(resamble);
			mac->reassembly[tx-1]=NULL;
			goto DISCARD_PACKET;
		}
		else
		{
			FRAGMENT frag = mp->frag;
			mpr->segment_id++;
			resamble->pstruMacData->dPacketSize+=packet->pstruMacData->dPacketSize;
			fn_NetSim_Packet_FreePacket(packet);
			if(frag==LAST_FRAGMENT)
			{
				pstruEventDetails->pPacket=resamble;
				mac->reassembly[tx-1]=NULL;
				goto RETURN;
			}
			else
				goto WAIT_FOR_NEXT;
		}
	}
DISCARD_PACKET:
	fn_NetSim_Packet_FreePacket(packet);
WAIT_FOR_NEXT:
	pstruEventDetails->pPacket=NULL;
RETURN:
	return;
}