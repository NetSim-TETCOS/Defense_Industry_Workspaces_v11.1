/************************************************************************************
* Copyright (C) 2018                                                               *
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
#include "Aloha.h"
#include "Aloha_enum.h"

static void aloha_mac_reset(PALOHA_MAC_VAR mac)
{
	mac->backoffCounter = 0;
	if (mac->currPacket)
		fn_NetSim_Packet_FreePacket(mac->currPacket);
	mac->currPacket = NULL;
	mac->retryCount = 0;
	mac->waitTime = 0;
}

static void move_time_to_slot(PALOHA_MAC_VAR mac, double* time)
{
	if (!mac->isSlotted)
		return;

	*time = ceil(*time / mac->slotlength)*mac->slotlength;
}

static void send_to_phy()
{
	PALOHA_MAC_VAR mac = ALOHA_CURR_MAC;
	PALOHA_PHY_VAR phy = ALOHA_CURR_PHY;

	NetSim_PACKET* packet = fn_NetSim_Packet_CopyPacket(mac->currPacket);

	double time = pstruEventDetails->dEventTime;
	move_time_to_slot(mac, &time);

	packet->pstruMacData->dArrivalTime = packet->pstruNetworkData->dEndTime;
	packet->pstruMacData->dStartTime = time;
	packet->pstruMacData->dEndTime = time;

	packet->pstruMacData->dPayload = packet->pstruNetworkData->dPacketSize;
	packet->pstruMacData->dOverhead = ALOHA_MAC_OVERHEAD;
	packet->pstruMacData->dPacketSize =
		packet->pstruMacData->dPayload +
		packet->pstruMacData->dOverhead;

	pstruEventDetails->dEventTime = time;
	pstruEventDetails->dPacketSize = fnGetPacketSize(packet);
	if (packet->pstruAppData)
	{
		pstruEventDetails->nApplicationId = packet->pstruAppData->nApplicationId;
		pstruEventDetails->nSegmentId = packet->pstruAppData->nSegmentId;
	}
	else
	{
		pstruEventDetails->nApplicationId = 0;
		pstruEventDetails->nSegmentId = 0;
	}
	pstruEventDetails->nEventType = PHYSICAL_OUT_EVENT;
	pstruEventDetails->nPacketId = packet->nPacketId;
	pstruEventDetails->nProtocolId = MAC_PROTOCOL_ALOHA;
	pstruEventDetails->nSubEventType = 0;
	pstruEventDetails->pPacket = packet;
	pstruEventDetails->szOtherDetails = NULL;
	fnpAddEvent(pstruEventDetails);
}

static void update_receiver(NetSim_PACKET* packet)
{
	if (packet->nReceiverId)
		return;

	if (isBroadcastPacket(packet))
		return;

	packet->nReceiverId = get_first_dest_from_packet(packet);
}

static void aloha_packet_arrive_from_upper_layer()
{
	PALOHA_MAC_VAR mac = ALOHA_CURR_MAC;
	if (mac->currPacket)
	{
		if (!mac->isMACBuffer)
		{
			NetSim_BUFFER* buf = DEVICE_MYACCESSBUFFER();
			if (fn_NetSim_GetBufferStatus(buf))
			{
				NetSim_PACKET* p = fn_NetSim_Packet_GetPacketFromBuffer(buf, 1);
				if (p)
					fn_NetSim_Packet_FreePacket(p);
			}
		}
		return;
	}

	static double val = 0;
	aloha_mac_reset(mac);

	NetSim_BUFFER* buf = DEVICE_MYACCESSBUFFER();
	if (fn_NetSim_GetBufferStatus(buf))
	{
		mac->currPacket = fn_NetSim_Packet_GetPacketFromBuffer(buf, 1);
		update_receiver(mac->currPacket);
		double waitTime = val;
		pstruEventDetails->dEventTime += waitTime;

		send_to_phy();
	}
}

static void aloha_resend_packet()
{
	PALOHA_MAC_VAR mac = ALOHA_CURR_MAC;
	
	mac->retryCount++;
	if (mac->retryCount > mac->retryLimit)
	{
		aloha_mac_reset(ALOHA_CURR_MAC);
		aloha_packet_arrive_from_upper_layer();
		return;
	}

	int window = (int)pow(2, mac->retryCount) - 1;
	int backoffCounter = (int)((window*rand()*1.0) / RAND_MAX);
	double waitTime = mac->slotlength;
	waitTime *= backoffCounter;
	assert(waitTime >= 0.0);
	pstruEventDetails->dEventTime += waitTime;

	send_to_phy();
}

int fn_NetSim_Aloha_Handle_MacOut()
{
	switch (pstruEventDetails->nSubEventType)
	{
		case 0:
			aloha_packet_arrive_from_upper_layer();
			break;
		case ALOHA_PACKET_ERROR:
			aloha_resend_packet();
			break;
		case ALOHA_PACKET_SUCCESS:
			aloha_mac_reset(ALOHA_CURR_MAC);
			aloha_packet_arrive_from_upper_layer();
			break;
		default:
			fnNetSimError("Unknow subevent %d for aloha mac.\n",
						  pstruEventDetails->nSubEventType);
			break;
	}
	return 0;
}
