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
#include "IEEE1609_Interface.h"

static PIEEE1609_BUFFER get_ieee1609_buffer(NETSIM_ID nDeviceId,
	NETSIM_ID nInterfaceId,
	IEEE1609_CHANNEL_TYPE type)
{
	PIEEE1609_MAC_VAR mac = GET_IEEE1609_MAC_VAR(nDeviceId, nInterfaceId);
	switch (type)
	{
	case IEEE1609_ChannelType_CCH:
		return &(mac->buffer[0]); //CCH
		break;
	case IEEE1609_ChannelType_SCH:
		return &(mac->buffer[1]); //SCH
		break;
	case IEEE1609_ChannelType_GUARD:
		return NULL;
		break;
	default:
		fnNetSimError("Unknown channel type %d\n", type);
		break;
	}
	return NULL;
}

PIEEE802_11_MAC_VAR IEEE802_11_MAC(NETSIM_ID ndeviceId, NETSIM_ID nInterfaceId)
{
	switch (DEVICE_MACLAYER(ndeviceId, nInterfaceId)->nMacProtocolId)
	{
	case MAC_PROTOCOL_IEEE1609:
		return ((PIEEE1609_MAC_VAR)DEVICE_MACVAR(ndeviceId, nInterfaceId))->secondary_protocol_var;
		break;
	case MAC_PROTOCOL_IEEE802_11:
		return ((PIEEE802_11_MAC_VAR)DEVICE_MACVAR(ndeviceId, nInterfaceId));
		break;
	default:
		fnNetSimError("Unknown mac protocol in %s\n", __FUNCTION__);
		return NULL;
		break;
	}
}

static IEEE1609_CHANNEL_TYPE get_curr_channel_type(NETSIM_ID nDeviceId,NETSIM_ID nInterfaceId)
{
	return GET_IEEE1609_MAC_VAR(nDeviceId, nInterfaceId)->CHANNEL_INFO.channel_type;
}

static IEEE1609_CHANNEL_TYPE get_channel_type_of_packet(NetSim_PACKET* packet)
{
	switch (packet->nPacketType)
	{
	case PacketType_BSM:
		return IEEE1609_ChannelType_CCH;
	default:
		return IEEE1609_ChannelType_SCH;
	}
}

PIEEE802_11_PHY_VAR IEEE802_11_PHY(NETSIM_ID ndeviceId, NETSIM_ID nInterfaceId)
{
	switch (DEVICE_MACLAYER(ndeviceId, nInterfaceId)->nMacProtocolId)
	{
	case MAC_PROTOCOL_IEEE1609:
		return ((PIEEE1609_PHY_VAR)DEVICE_PHYVAR(ndeviceId, nInterfaceId))->secondary_protocol_var;;
		break;
	case MAC_PROTOCOL_IEEE802_11:
		return ((PIEEE802_11_PHY_VAR)DEVICE_PHYVAR(ndeviceId, nInterfaceId));
		break;
	default:
		fnNetSimError("Unknown phy protocol in %s\n", __FUNCTION__);
		return NULL;
		break;
	}
}

void SET_IEEE802_11_MAC(NETSIM_ID ndeviceId, NETSIM_ID nInterfaceId,PIEEE802_11_MAC_VAR mac)
{
	switch (DEVICE_MACLAYER(ndeviceId, nInterfaceId)->nMacProtocolId)
	{
	case MAC_PROTOCOL_IEEE1609:
		((PIEEE1609_MAC_VAR)DEVICE_MACVAR(ndeviceId, nInterfaceId))->secondary_protocol_var = mac;
		break;
	case MAC_PROTOCOL_IEEE802_11:
		DEVICE_MACVAR(ndeviceId, nInterfaceId) = mac;
		break;
	default:
		fnNetSimError("Unknown mac protocol in %s\n", __FUNCTION__);
		break;
	}
}
void SET_IEEE802_11_PHY(NETSIM_ID ndeviceId, NETSIM_ID nInterfaceId,PIEEE802_11_PHY_VAR phy)
{
	switch (DEVICE_MACLAYER(ndeviceId, nInterfaceId)->nMacProtocolId)
	{
	case MAC_PROTOCOL_IEEE1609:
		((PIEEE1609_PHY_VAR)DEVICE_PHYVAR(ndeviceId, nInterfaceId))->secondary_protocol_var = phy;
		break;
	case MAC_PROTOCOL_IEEE802_11:
		DEVICE_PHYVAR(ndeviceId, nInterfaceId) = phy;
		break;
	default:
		fnNetSimError("Unknown mac protocol in %s\n", __FUNCTION__);
		break;
	}
}

bool isIEEE802_11_Configure(NETSIM_ID ndeviceId, NETSIM_ID nInterfaceId)
{
	if (!DEVICE_MACLAYER(ndeviceId, nInterfaceId))
		return false;

	switch (DEVICE_MACLAYER(ndeviceId, nInterfaceId)->nMacProtocolId)
	{
	case MAC_PROTOCOL_IEEE1609:
		return ((PIEEE1609_MAC_VAR)DEVICE_MACVAR(ndeviceId, nInterfaceId))->secondary_protocol == MAC_PROTOCOL_IEEE802_11;
		break;
	case MAC_PROTOCOL_IEEE802_11:
		return true;
		break;
	default:
		return false;
		break;
	}
}

static void add_to_IEEE1609_queue(NETSIM_ID nDeviceId, NETSIM_ID nInterfaceId, NetSim_PACKET* packet)
{
	IEEE1609_CHANNEL_TYPE type = get_channel_type_of_packet(packet);
	PIEEE1609_BUFFER buf = get_ieee1609_buffer(nDeviceId,nInterfaceId,type);
	if (buf->head)
	{
		buf->tail->pstruNextPacket = packet;
		buf->tail = packet;
	}
	else
	{
		buf->head = packet;
		buf->tail = packet;
	}
}
void add_to_queue(NETSIM_ID nDeviceId, NETSIM_ID nInterfaceId, NetSim_PACKET* packet)
{
	switch (DEVICE_MACLAYER(nDeviceId, nInterfaceId)->nMacProtocolId)
	{
	case MAC_PROTOCOL_IEEE1609:
		add_to_IEEE1609_queue(nDeviceId, nInterfaceId, packet);
		break;
	case MAC_PROTOCOL_IEEE802_11:
		fn_NetSim_IEEE802_11e_AddtoQueue(IEEE802_11_MAC(nDeviceId, nInterfaceId), packet);
		break;
	default:
		fnNetSimError("Unknown Mac protocol %d in %s.",
			DEVICE_MACLAYER(nDeviceId, nInterfaceId)->nMacProtocolId, __FUNCTION__);
		break;
	}
}

static NetSim_PACKET* get_from_IEEE1609_queue(NETSIM_ID nDeviceId, NETSIM_ID nInterfaceId,
	int nPacketRequire,
	int* nPacketCount,
	IEEE1609_CHANNEL_TYPE type)
{
	NetSim_PACKET* p = NULL, *t = NULL;
	int c=0;
	PIEEE1609_BUFFER buf = get_ieee1609_buffer(nDeviceId, nInterfaceId, type);
	
	if (!buf)
	{
		*nPacketCount = 0;
		return NULL; // BUFFER is not initialized for this channel type
	}

	while (buf->head && c<nPacketRequire)
	{
		if (p)
		{
			t->pstruNextPacket = buf->head;
			t = t->pstruNextPacket;
		}
		else
		{
			p = buf->head;
			t = p;
		}
		buf->head = buf->head->pstruNextPacket;
		t->pstruNextPacket = NULL;
		if (!buf->head)
			buf->tail = NULL;
		c++;
	}
	*nPacketCount = c;
	return p;
}

NetSim_PACKET* get_from_queue(NETSIM_ID nDeviceId,
							  NETSIM_ID nInterfaceId,
							  int nPacketRequire,
							  int* nPacketCount)
{
	switch (DEVICE_MACLAYER(nDeviceId, nInterfaceId)->nMacProtocolId)
	{
	case MAC_PROTOCOL_IEEE1609:
	{
		
		return get_from_IEEE1609_queue(nDeviceId,
									   nInterfaceId,
									   nPacketRequire,
									   nPacketCount,
									   get_curr_channel_type(nDeviceId,nInterfaceId));
	}
	break;
	case MAC_PROTOCOL_IEEE802_11:
		return fn_NetSim_IEEE802_11e_GetPacketFromQueue(&(IEEE802_11_MAC(nDeviceId, nInterfaceId)->qosList),
														nPacketRequire,
														nPacketCount);
		break;
	default:
		fnNetSimError("Unknown Mac protocol %d in %s.",
			DEVICE_MACLAYER(nDeviceId, nInterfaceId)->nMacProtocolId, __FUNCTION__);
		return NULL;
		break;
	}
}

static void readd_to_IEEE1609_queue(NETSIM_ID nDeviceId, NETSIM_ID nInterfaceId, NetSim_PACKET* packet)
{
	IEEE1609_CHANNEL_TYPE type = get_channel_type_of_packet(packet);
	PIEEE1609_BUFFER buf = get_ieee1609_buffer(nDeviceId, nInterfaceId, type);
	if (buf->head)
	{
		packet->pstruNextPacket = buf->head;
		buf->head = packet;
	}
	else
	{
		buf->head = packet;
		buf->tail = packet;
	}
}

void readd_to_queue(NETSIM_ID nDeviceId, NETSIM_ID nInterfaceId, NetSim_PACKET* packet)
{
	switch (DEVICE_MACLAYER(nDeviceId, nInterfaceId)->nMacProtocolId)
	{
	case MAC_PROTOCOL_IEEE1609:
		readd_to_IEEE1609_queue(nDeviceId, nInterfaceId, packet);
		break;
	case MAC_PROTOCOL_IEEE802_11:
		fnNetSimError("Mustnot call this function %s for protocol %d.", __FUNCTION__,
			MAC_PROTOCOL_IEEE802_11);
		assert(false);
		break;
	default:
		fnNetSimError("Unknown Mac protocol %d in %s.",
			DEVICE_MACLAYER(nDeviceId, nInterfaceId)->nMacProtocolId, __FUNCTION__);
		break;
	}
}

static bool is_time_in_same_channel(double time, NETSIM_ID nDeviceId, NETSIM_ID nInterfaceId)
{
	PIEEE1609_MAC_VAR mac = GET_IEEE1609_MAC_VAR(nDeviceId, nInterfaceId);
	if (mac->CHANNEL_INFO.dEndTime >= time)
		return true;
	else
		return false;
}

static void revert_packet(NETSIM_ID nDeviceId, NETSIM_ID nInterfaceId)
{
	PIEEE802_11_MAC_VAR mac = IEEE802_11_MAC(nDeviceId, nInterfaceId);
	if (!mac->currentProcessingPacket)
		return; //nothing to revert

	if (isIEEE802_11_CtrlPacket(mac->currentProcessingPacket))
	{
		readd_to_IEEE1609_queue(nDeviceId, nInterfaceId,
			mac->waitingforCTS);
		mac->waitingforCTS = NULL;
		fn_NetSim_Packet_FreePacket(mac->currentProcessingPacket);
		mac->currentProcessingPacket = NULL;
	}
	else
	{
		readd_to_IEEE1609_queue(nDeviceId, nInterfaceId,
			mac->currentProcessingPacket);
		mac->currentProcessingPacket = NULL;
	}
}

static void make_idle(NETSIM_ID nDeviceId, NETSIM_ID nInterfaceId)
{
	PIEEE802_11_MAC_VAR mac = IEEE802_11_MAC(nDeviceId, nInterfaceId);
	PIEEE802_11_PHY_VAR phy = IEEE802_11_PHY(nDeviceId, nInterfaceId);

	IEEE802_11_Change_Mac_State(mac, IEEE802_11_MACSTATE_MAC_IDLE);
	phy->radio.radioState = RX_ON_IDLE;
}

bool validate_processing_time(double time, NETSIM_ID nDeviceId, NETSIM_ID nInterfaceId)
{
	switch (DEVICE_MACLAYER(nDeviceId, nInterfaceId)->nMacProtocolId)
	{
	case MAC_PROTOCOL_IEEE1609:
	{
		if (is_time_in_same_channel(time, nDeviceId, nInterfaceId))
			return true;
		revert_packet(nDeviceId,nInterfaceId);
		make_idle(nDeviceId, nInterfaceId);
		return false;
	}
	break;
	case MAC_PROTOCOL_IEEE802_11:
		return true;
		break;
	default:
		fnNetSimError("Unknown MAC protocol %d in %s\n",
			DEVICE_MACLAYER(nDeviceId, nInterfaceId)->nMacProtocolId,
			__FUNCTION__);
		return false;
		break;
	}
}