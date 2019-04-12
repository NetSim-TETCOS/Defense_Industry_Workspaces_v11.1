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
#include "IEEE802_11_MAC_Frame.h"
#include "IEEE802_11_Phy.h"
#include "NetSim_utility.h"

static UINT get_fragment_number(NetSim_PACKET* packet);

double getAckSize(PIEEE802_11_PHY_VAR phy)
{
	switch(phy->PhyProtocol)
	{
	case IEEE_802_11a:
	case IEEE_802_11b:
	case IEEE_802_11g:
	case IEEE_802_11p:
		return ACK_SIZE;
	case IEEE_802_11n:
		return BlockACK_SIZE_802_11n;
	case IEEE_802_11ac:
		return BlockACK_SIZE_802_11ac;
	default:
		fnNetSimError("Unknown phy protocol %d in %s.",phy->PhyProtocol,__FUNCTION__);
		return 0;
	}
}

double getCTSSize()
{
	return CTS_SIZE;
}

double getRTSSize()
{
	return RTS_SIZE;
}

double getMacOverhead(PIEEE802_11_PHY_VAR phy,double size)
{
	double overhead = 0;
	if (phy->PhyProtocol == IEEE_802_11n || phy->PhyProtocol == IEEE_802_11ac)
	{
		overhead = MAC_OVERHEAD_802_11n;
		//This is for the 4bytes of padding
		overhead += (4 - ((int)size % 4)) % 4;
	}
	else
		overhead = MAC_OVERHEAD;
	return overhead;
}

static double get_block_ack_size(NETSIM_ID dev,NETSIM_ID ifid)
{
	switch(IEEE802_11_PHY(dev,ifid)->PhyProtocol)
	{
	case IEEE_802_11n: return BlockACK_SIZE_802_11n;
	case IEEE_802_11ac: return BlockACK_SIZE_802_11ac;
	default: fnNetSimError("Unknown phy protocol %d in %s.",IEEE802_11_PHY(dev,ifid)->PhyProtocol,__FUNCTION__);
		return 0;
	}
}
/**
This function adds MAC header to the packet before sending to Physical layer.
*/
void fn_NetSim_IEEE802_11_Add_MAC_Header(NETSIM_ID nDeviceId,NETSIM_ID nInterfaceId, NetSim_PACKET *pstruPacket,unsigned int i)
{
	PIEEE802_11_PHY_VAR phy = IEEE802_11_PHY(nDeviceId,nInterfaceId);
	PIEEE802_11_MAC_VAR mac = IEEE802_11_MAC(nDeviceId,nInterfaceId);
	
	PIEEE802_11_MAC_HEADER hdr;
	hdr = (PIEEE802_11_MAC_HEADER)calloc(1, sizeof *hdr);

	hdr->FrameControl.ProtocolVerSion = 0;
	hdr->FrameControl.Type = DATA;
	hdr->FrameControl.SubType = QoS_Data;

	if(mac->BSSType==INFRASTRUCTURE)
	{
		if(!MAC_COMPARE(mac->BSSId,DEVICE_HWADDRESS(nDeviceId,nInterfaceId)))
		{
			hdr->FrameControl.ToDS = 0;
			hdr->FrameControl.FromDS = 1;
			hdr->Address1=pstruPacket->pstruMacData->szDestMac;
			hdr->Address2=mac->BSSId;
			hdr->Address3=pstruPacket->pstruMacData->szSourceMac;
		}
		else
		{
			hdr->FrameControl.ToDS = 1;
			hdr->FrameControl.FromDS = 0;
			hdr->Address1=mac->BSSId;
			hdr->Address2=pstruPacket->pstruMacData->szSourceMac;
			hdr->Address3=pstruPacket->pstruMacData->szDestMac;
		}
	}
	else if(mac->BSSType==MESH)
	{
		hdr->FrameControl.ToDS = 1;
		hdr->FrameControl.FromDS = 1;
		hdr->Address1=pstruPacket->pstruMacData->szDestMac;
		hdr->Address2=pstruPacket->pstruMacData->szSourceMac;
		hdr->Address3= pstruPacket->pstruMacData->szDestMac;
		hdr->Address4= pstruPacket->pstruMacData->szSourceMac;
	}
	else
	{
		fnNetSimError("%d BSS Type is not implemented for IEEE802.11 protocol",mac->BSSType);
	}

	if(pstruPacket->pstruNextPacket)
		hdr->FrameControl.MoreFragment = 1;
	else
		hdr->FrameControl.MoreFragment = 0;
	
	if(mac->nRetryCount)
		hdr->FrameControl.Retry = 1;
	else
		hdr->FrameControl.Retry = 0;

	hdr->FrameControl.PowerManagement = 0;

	hdr->FrameControl.MoreData = 0;

	hdr->FrameControl.ProtectedFrame = 0;

	hdr->FrameControl.Order = 0;

	hdr->snDurationID = 0;

	hdr->SequenceControl.SequenceNumber = i;
	hdr->SequenceControl.FragmentNumber = 0;

	memset(&hdr->QOSControl,0,sizeof(hdr->QOSControl));

	hdr->HTControl = 0;

	pstruPacket->pstruMacData->dOverhead = getMacOverhead(phy, pstruPacket->pstruNetworkData->dPacketSize);

	pstruPacket->pstruMacData->dPayload = pstruPacket->pstruNetworkData->dPacketSize;
	pstruPacket->pstruMacData->dPacketSize = pstruPacket->pstruMacData->dPayload \
		+ pstruPacket->pstruMacData->dOverhead ;	
	pstruPacket->pstruMacData->dEndTime = pstruEventDetails->dEventTime;

	if (pstruPacket->pstruMacData->Packet_MACProtocol)
	{
		//Free old header
		fn_NetSim_Packet_FreeMacProtocolData(pstruPacket);
	}
	pstruPacket->pstruMacData->nMACProtocol = MAC_PROTOCOL_IEEE802_11;
	pstruPacket->pstruMacData->Packet_MACProtocol = hdr;	
}

NetSim_PACKET* fn_NetSim_IEEE802_11_CreateAckPacket(NETSIM_ID nDeviceId,NETSIM_ID nInterfaceId,NetSim_PACKET* data,double time)
{
	NetSim_PACKET* packet;
	PIEEE802_11_ACK ack;
	PIEEE802_11_MAC_VAR mac=IEEE802_11_MAC(nDeviceId,nInterfaceId);

	// Create ACK Frame	
	packet = fn_NetSim_Packet_CreatePacket(MAC_LAYER);
	packet->nPacketType = PacketType_Control;
	packet->nPacketPriority = Priority_High;
	packet->nControlDataType = WLAN_ACK;
	packet->nReceiverId = data->nTransmitterId;
	packet->nTransmitterId=nDeviceId;
	packet->nSourceId=nDeviceId;
	add_dest_to_packet(packet, data->nTransmitterId);

	ack =  calloc(1, sizeof* ack);

	ack->FrameControl.Type = CONTROL;
	ack->FrameControl.SubType = ACK;
	ack->RA = ((PIEEE802_11_MAC_HEADER)(data->pstruMacData->Packet_MACProtocol))->Address2;

	switch(mac->BSSType)
	{
	case INFRASTRUCTURE:
		if(MAC_COMPARE(DEVICE_HWADDRESS(nDeviceId,nInterfaceId),mac->BSSId))
		{	
			ack->FrameControl.FromDS = 0;// Data is to AP(infra = 0 and ToDS = 1)
			ack->FrameControl.ToDS = 1;		
		}
		else
		{
			ack->FrameControl.FromDS = 1;// Data from AP
			ack->FrameControl.ToDS = 0;		
	
		}
		break;
	case INDEPENDENT:
		ack->FrameControl.FromDS = 0;// Both 0 for IBSS
		ack->FrameControl.ToDS = 0;		
		break;
	default:
		break;
	}

	packet->pstruMacData->Packet_MACProtocol = ack;
	packet->pstruMacData->dArrivalTime = time;
	packet->pstruMacData->dStartTime = time;
	packet->pstruMacData->dEndTime = time;
	packet->pstruMacData->dOverhead  = ACK_SIZE;
	packet->pstruMacData->dPacketSize = packet->pstruMacData->dOverhead;
	packet->pstruMacData->nMACProtocol = MAC_PROTOCOL_IEEE802_11;
	
	return packet;
}

NetSim_PACKET* fn_NetSim_IEEE802_11_CreateBlockAckPacket(NETSIM_ID nDeviceId,NETSIM_ID nInterfaceId,NetSim_PACKET* data,double time)
{
	NetSim_PACKET* packet;
	PIEEE802_11_BLOCKACK ack;
	PIEEE802_11_MAC_VAR mac=IEEE802_11_MAC(nDeviceId,nInterfaceId);

	// Create ACK Frame	
	packet = fn_NetSim_Packet_CreatePacket(MAC_LAYER);
	packet->nPacketType = PacketType_Control;
	packet->nPacketPriority = Priority_High;
	packet->nControlDataType = WLAN_BlockACK;
	packet->nReceiverId = data->nTransmitterId;
	packet->nTransmitterId=nDeviceId;
	packet->nSourceId=nDeviceId;
	add_dest_to_packet(packet, data->nTransmitterId);

	ack =  (PIEEE802_11_BLOCKACK)calloc(1, sizeof* ack);

	ack->FrameControl.Type = CONTROL;
	ack->FrameControl.SubType = BlockAck;
	ack->RA = ((PIEEE802_11_MAC_HEADER)(data->pstruMacData->Packet_MACProtocol))->Address2;
	ack->TA = MAC_COPY(DEVICE_HWADDRESS(nDeviceId,nInterfaceId));

	ack->BARControl.compressedBitmap=1;
	ack->BAStartingSequenceControl.StartingSequenceNumber=get_fragment_number(data);

	switch(mac->BSSType)
	{
	case INFRASTRUCTURE:
		if(MAC_COMPARE(DEVICE_HWADDRESS(nDeviceId,nInterfaceId),mac->BSSId))
		{	
			ack->FrameControl.FromDS = 0;// Data is to AP(infra = 0 and ToDS = 1)
			ack->FrameControl.ToDS = 1;		
		}
		else
		{
			ack->FrameControl.FromDS = 1;// Data from AP
			ack->FrameControl.ToDS = 0;		

		}
		break;
	case INDEPENDENT:
		ack->FrameControl.FromDS = 0;// Both 0 for IBSS
		ack->FrameControl.ToDS = 0;		
		break;
	default:
		break;
	}

	packet->pstruMacData->Packet_MACProtocol = ack;
	packet->pstruMacData->dArrivalTime = time;
	packet->pstruMacData->dStartTime = time;
	packet->pstruMacData->dEndTime = time;
	packet->pstruMacData->dOverhead  = get_block_ack_size(nDeviceId,nInterfaceId);;
	packet->pstruMacData->dPacketSize = packet->pstruMacData->dOverhead;
	packet->pstruMacData->nMACProtocol = MAC_PROTOCOL_IEEE802_11;

	return packet;
}

bool isIEEE802_11_CtrlPacket(NetSim_PACKET* packet)
{
	if (packet)
		return (packet->nControlDataType / 100 == MAC_PROTOCOL_IEEE802_11);
	else
		return false;
}

void fn_NetSim_Process_CtrlPacket()
{
	NetSim_PACKET* packet = pstruEventDetails->pPacket;
	switch(packet->nControlDataType)
	{
	case WLAN_ACK:
		fn_NetSim_IEEE802_11_CSMACA_ProcessAck();
		break;
	case WLAN_RTS:
		fn_NetSim_IEEE802_11_RTS_CTS_ProcessRTS();
		break;
	case WLAN_CTS:
		fn_NetSim_IEEE802_11_RTS_CTS_ProcessCTS();
		break;
	case WLAN_BlockACK:
		fn_NetSim_IEEE802_11_CSMACA_ProcessBlockAck();
		break;
	default:
		fnNetSimError("Unknown ctrl packet %d is %s\n",packet->nControlDataType,__FUNCTION__);
		break;
	}
}


NetSim_PACKET* fn_NetSim_IEEE802_11_CreateRTSPacket(NetSim_PACKET* data, double duration)
{
	PIEEE802_11_MAC_VAR mac=IEEE802_11_CURR_MAC;
	NetSim_PACKET* packet=fn_NetSim_Packet_CreatePacket(MAC_LAYER);
	PIEEE802_11_RTS rts=(PIEEE802_11_RTS)calloc(1,sizeof* rts);
	NETSIM_ID linkid = DEVICE_PHYLAYER(pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId)->nLinkId;

	packet->dEventTime=pstruEventDetails->dEventTime;
	packet->nControlDataType=WLAN_RTS;
	add_dest_to_packet(packet, data->nReceiverId);
	packet->nPacketType=PacketType_Control;
	packet->nReceiverId=data->nReceiverId;
	packet->nSourceId=data->nTransmitterId;
	packet->nTransmitterId=data->nTransmitterId;
	packet->pstruMacData->dArrivalTime=pstruEventDetails->dEventTime;
	packet->pstruMacData->dEndTime=pstruEventDetails->dEventTime;
	packet->pstruMacData->dOverhead=RTS_SIZE;
	packet->pstruMacData->dPacketSize=RTS_SIZE;
	packet->pstruMacData->dStartTime=pstruEventDetails->dEventTime;
	packet->pstruMacData->nMACProtocol=MAC_PROTOCOL_IEEE802_11;
	packet->pstruMacData->szDestMac=MAC_COPY(data->pstruMacData->szNextHopMac);
	packet->pstruMacData->szNextHopMac=MAC_COPY(data->pstruMacData->szNextHopMac);
	packet->pstruMacData->szSourceMac=MAC_COPY(data->pstruMacData->szSourceMac);
	packet->pstruMacData->Packet_MACProtocol=rts;

	rts->FrameControl.Type=CONTROL;
	rts->FrameControl.SubType=RTS;
	rts->Duration = (UINT16)duration;
	rts->TA=MAC_COPY(DEVICE_HWADDRESS(packet->nTransmitterId,
		fn_NetSim_Stack_GetWirelessInterface(linkid,packet->nTransmitterId)));
	
	rts->RA=MAC_COPY(DEVICE_HWADDRESS(packet->nReceiverId,
		fn_NetSim_Stack_GetWirelessInterface(linkid,packet->nReceiverId)));

	switch(mac->BSSType)
	{
	case INFRASTRUCTURE:
		if(MAC_COMPARE(DEVICE_HWADDRESS(pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId),mac->BSSId))
		{	
			rts->FrameControl.FromDS = 0;// Data is to AP(infra = 0 and ToDS = 1)
			rts->FrameControl.ToDS = 1;		
		}
		else
		{
			rts->FrameControl.FromDS = 1;// Data from AP
			rts->FrameControl.ToDS = 0;		

		}
		break;
	case INDEPENDENT:
		rts->FrameControl.FromDS = 0;// Both 0 for IBSS
		rts->FrameControl.ToDS = 0;		
		break;
	default:
		break;
	}

	return packet;
}

NetSim_PACKET* fn_NetSim_IEEE802_11_CreateCTSPacket(NetSim_PACKET* data)
{
	PIEEE802_11_MAC_VAR mac=IEEE802_11_CURR_MAC;
	NetSim_PACKET* packet=fn_NetSim_Packet_CreatePacket(MAC_LAYER);
	PIEEE802_11_CTS cts=(PIEEE802_11_CTS)calloc(1,sizeof* cts);
	PIEEE802_11_RTS rts = (PIEEE802_11_RTS)data->pstruMacData->Packet_MACProtocol;

	packet->dEventTime=pstruEventDetails->dEventTime;
	packet->nControlDataType=WLAN_CTS;
	add_dest_to_packet(packet, data->nTransmitterId);
	packet->nPacketType=PacketType_Control;
	packet->nReceiverId=data->nTransmitterId;
	packet->nSourceId=data->nReceiverId;
	packet->nTransmitterId=data->nReceiverId;
	packet->pstruMacData->dArrivalTime=pstruEventDetails->dEventTime;
	packet->pstruMacData->dEndTime=pstruEventDetails->dEventTime;
	packet->pstruMacData->dOverhead=CTS_SIZE;
	packet->pstruMacData->dPacketSize=CTS_SIZE;
	packet->pstruMacData->dStartTime=pstruEventDetails->dEventTime;
	packet->pstruMacData->nMACProtocol=MAC_PROTOCOL_IEEE802_11;
	packet->pstruMacData->szDestMac=MAC_COPY(data->pstruMacData->szSourceMac);
	packet->pstruMacData->szNextHopMac=MAC_COPY(data->pstruMacData->szSourceMac);
	packet->pstruMacData->szSourceMac=MAC_COPY(data->pstruMacData->szDestMac);
	packet->pstruMacData->Packet_MACProtocol=cts;

	cts->FrameControl.Type=CONTROL;
	cts->FrameControl.SubType=CTS;
	cts->Duration = (UINT16)calculate_CTS_duration(pstruEventDetails->nDeviceId,
										   pstruEventDetails->nInterfaceId,
										   rts->Duration);

	cts->RA=MAC_COPY(rts->TA);

	switch(mac->BSSType)
	{
	case INFRASTRUCTURE:
		if(MAC_COMPARE(DEVICE_HWADDRESS(pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId),mac->BSSId))
		{	
			rts->FrameControl.FromDS = 1;// Data is to AP(infra = 0 and ToDS = 1)
			rts->FrameControl.ToDS = 0;		
		}
		else
		{
			rts->FrameControl.FromDS = 0;// Data from AP
			rts->FrameControl.ToDS = 1;		

		}
		break;
	case INDEPENDENT:
		rts->FrameControl.FromDS = 0;// Both 0 for IBSS
		rts->FrameControl.ToDS = 0;		
		break;
	default:
		break;
	}

	return packet;
}

void set_blockack_bitmap(NetSim_PACKET* ackPacket,NetSim_PACKET* packet)
{
	PIEEE802_11_BLOCKACK back = (PIEEE802_11_BLOCKACK)ackPacket->pstruMacData->Packet_MACProtocol;
	UINT i=get_fragment_number(packet);
	SET_BIT_64(back->BitMap,i-1);
}

bool is_more_fragment_coming(NetSim_PACKET* packet)
{
	return ((PIEEE802_11_MAC_HEADER)(packet->pstruMacData->Packet_MACProtocol))->FrameControl.MoreFragment;
}

bool is_first_packet(NetSim_PACKET* packet)
{
	return ((PIEEE802_11_MAC_HEADER)(packet->pstruMacData->Packet_MACProtocol))->SequenceControl.SequenceNumber==1;
}

static UINT get_fragment_number(NetSim_PACKET* packet)
{
	return ((PIEEE802_11_MAC_HEADER)(packet->pstruMacData->Packet_MACProtocol))->SequenceControl.SequenceNumber;
}

static void free_rts_header(NetSim_PACKET* packet)
{
	free(((PIEEE802_11_RTS)packet->pstruMacData->Packet_MACProtocol));
}

static void free_cts_header(NetSim_PACKET* packet)
{
	free(((PIEEE802_11_CTS)packet->pstruMacData->Packet_MACProtocol));
}

static void free_ack_header(NetSim_PACKET* packet)
{
	free(((PIEEE802_11_ACK)packet->pstruMacData->Packet_MACProtocol));
}

static void free_blockack_header(NetSim_PACKET* packet)
{
	free(((PIEEE802_11_BLOCKACK)packet->pstruMacData->Packet_MACProtocol));
}

static void free_mpdu_header(NetSim_PACKET* packet)
{
	free(((PIEEE802_11_MAC_HEADER)packet->pstruMacData->Packet_MACProtocol));
}

void free_ieee802_11_mac_header(NetSim_PACKET* packet)
{
	switch(packet->nControlDataType)
	{
	case WLAN_RTS:
		free_rts_header(packet);
		break;
	case WLAN_ACK:
		free_ack_header(packet);
		break;
	case WLAN_BlockACK:
		free_blockack_header(packet);
		break;
	case WLAN_CTS:
		free_cts_header(packet);
		break;
	default:
		free_mpdu_header(packet);
		break;
	}
	packet->pstruMacData->Packet_MACProtocol=NULL;
}

static void copy_rts_header(NetSim_PACKET* dest,NetSim_PACKET* src)
{
	dest->pstruMacData->Packet_MACProtocol = calloc(1,sizeof(IEEE802_11_RTS));
	memcpy(dest->pstruMacData->Packet_MACProtocol,src->pstruMacData->Packet_MACProtocol,sizeof(IEEE802_11_RTS));
}

static void copy_cts_header(NetSim_PACKET* dest,NetSim_PACKET* src)
{
	dest->pstruMacData->Packet_MACProtocol = calloc(1,sizeof(IEEE802_11_CTS));
	memcpy(dest->pstruMacData->Packet_MACProtocol,src->pstruMacData->Packet_MACProtocol,sizeof(IEEE802_11_CTS));
}

static void copy_ack_header(NetSim_PACKET* dest,NetSim_PACKET* src)
{
	dest->pstruMacData->Packet_MACProtocol = calloc(1,sizeof(IEEE802_11_ACK));
	memcpy(dest->pstruMacData->Packet_MACProtocol,src->pstruMacData->Packet_MACProtocol,sizeof(IEEE802_11_ACK));
}

static void copy_blockack_header(NetSim_PACKET* dest,NetSim_PACKET* src)
{
	dest->pstruMacData->Packet_MACProtocol = calloc(1,sizeof(IEEE802_11_BLOCKACK));
	memcpy(dest->pstruMacData->Packet_MACProtocol,src->pstruMacData->Packet_MACProtocol,sizeof(IEEE802_11_BLOCKACK));
}

static void copy_mpdu_header(NetSim_PACKET* dest,NetSim_PACKET* src)
{
	if(src->pstruMacData->Packet_MACProtocol)
	{
		dest->pstruMacData->Packet_MACProtocol = calloc(1,sizeof(IEEE802_11_MAC_HEADER));
		memcpy(dest->pstruMacData->Packet_MACProtocol,src->pstruMacData->Packet_MACProtocol,sizeof(IEEE802_11_MAC_HEADER));
	}
}

void copy_ieee802_11_mac_header(NetSim_PACKET* dest,NetSim_PACKET* src)
{
	if(!src->pstruMacData->Packet_MACProtocol)
		return;
	switch(src->nControlDataType)
	{
	case WLAN_RTS:
		copy_rts_header(dest,src);
		break;
	case WLAN_ACK:
		copy_ack_header(dest,src);
		break;
	case WLAN_BlockACK:
		copy_blockack_header(dest,src);
		break;
	case WLAN_CTS:
		copy_cts_header(dest,src);
		break;
	default:
		copy_mpdu_header(dest,src);
		break;
	}
}