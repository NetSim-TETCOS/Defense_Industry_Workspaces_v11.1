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
#include "CSMACD.h"

#define check_for_retry_limit(mac) (mac->nRetryLimit>mac->nRetryCount)

void wait_for_random_time()
{
	double ran = fn_NetSim_Utilities_GenerateRandomNo(DEVICE_SEED(pstruEventDetails->nDeviceId))/NETSIM_RAND_MAX;

	ran *= 16;
	if(file_contention != NULL)
	{
		fprintf(file_contention,"%d\t%d\t%f\n",pstruEventDetails->nDeviceId,(int)ran,pstruEventDetails->dEventTime);
	}
	pstruEventDetails->dEventTime+= (int)ran*CSMACD_CURRMAC->dSlotTime;
}

void send_to_phy()
{
	NetSim_PACKET* packet = fn_NetSim_Packet_CopyPacket(CSMACD_CURRMAC->currentPacket);

	packet->pstruMacData->dArrivalTime = packet->pstruNetworkData->dEndTime;
	packet->pstruMacData->dStartTime = pstruEventDetails->dEventTime;
	packet->pstruMacData->dEndTime = pstruEventDetails->dEventTime;

	packet->pstruMacData->dPayload=packet->pstruNetworkData->dPacketSize;
	packet->pstruMacData->dOverhead=CSMACD_MAC_OVERHEAD;
	packet->pstruMacData->dPacketSize = 
		packet->pstruMacData->dPayload+
		packet->pstruMacData->dOverhead;

	pstruEventDetails->dPacketSize=fnGetPacketSize(packet);
	if(packet->pstruAppData)
	{
		pstruEventDetails->nApplicationId=packet->pstruAppData->nApplicationId;
		pstruEventDetails->nSegmentId=packet->pstruAppData->nSegmentId;
	}
	else
	{
		pstruEventDetails->nApplicationId=0;
		pstruEventDetails->nSegmentId=0;
	}
	pstruEventDetails->nEventType=PHYSICAL_OUT_EVENT;
	pstruEventDetails->nPacketId=packet->nPacketId;
	pstruEventDetails->nProtocolId=MAC_PROTOCOL_CSMACD;
	pstruEventDetails->nSubEventType=0;
	pstruEventDetails->pPacket=packet;
	pstruEventDetails->szOtherDetails=NULL;
	fnpAddEvent(pstruEventDetails);
}

void fn_NetSim_CSMACD_MacOut()
{
	NetSim_BUFFER* buffer= DEVICE_MAC_NW_INTERFACE(pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId)->pstruAccessBuffer;
	PCSMACD_MACVAR mac = CSMACD_CURRMAC;
	NetSim_PACKET* packet;

	if(mac->macState || !isMediumIdle())
		return;

	if(mac->currentPacket && !mac->iswaited)
	{
		if(check_for_retry_limit(mac))
		{
			mac->macState=Busy;
			wait_for_random_time();
			pstruEventDetails->nEventType = TIMER_EVENT;
			pstruEventDetails->nSubEventType = WAIT_FOR_RANDOM_TIME;
			fnpAddEvent(pstruEventDetails);
			return;
		}
		else
		{
			fn_NetSim_Packet_FreePacket(mac->currentPacket);
			mac->currentPacket=NULL;
			mac->nRetryCount=0;
			goto FIRST_TRANSMISSION;
		}
	}
FIRST_TRANSMISSION:
	mac->iswaited = false;

	packet = mac->currentPacket;

	if(fn_NetSim_GetBufferStatus(buffer) || packet)
	{
		if(isMediumIdle())
		{
			double ran;
			mac->macState=Busy;
			ran = fn_NetSim_Utilities_GenerateRandomNo(DEVICE_SEED(pstruEventDetails->nDeviceId))/NETSIM_RAND_MAX;
			if(ran<=mac->dPersistance)
			{
				if(!packet) //Take new packet from buffer
					packet = fn_NetSim_Packet_GetPacketFromBuffer(buffer,1);
				mac->currentPacket = packet;
				send_to_phy();
			}
			else
			{
				pstruEventDetails->dEventTime +=mac->dSlotTime;
				pstruEventDetails->nEventType = TIMER_EVENT;
				pstruEventDetails->nSubEventType = PERSISTANCE_WAIT;
				fnpAddEvent(pstruEventDetails);
			}
		}
	}
}

void fn_NetSim_CSMACD_MacIn()
{
	pstruEventDetails->nEventType = NETWORK_IN_EVENT;
	fnpAddEvent(pstruEventDetails);
}

void fn_NetSim_CSMACS_PersistanceWait()
{
	double ran;
	PCSMACD_MACVAR mac=CSMACD_CURRMAC;
	NetSim_PACKET* packet;
	NetSim_BUFFER* buffer= DEVICE_MAC_NW_INTERFACE(pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId)->pstruAccessBuffer;

	if(!isMediumIdle())
	{
		mac->macState=Idle;
		mac->iswaited = true;
		return;
	}

	ran = fn_NetSim_Utilities_GenerateRandomNo(DEVICE_SEED(pstruEventDetails->nDeviceId))/NETSIM_RAND_MAX;
	if(ran<=mac->dPersistance)
	{
		if(!mac->currentPacket)
		{
			packet = fn_NetSim_Packet_GetPacketFromBuffer(buffer,1);
			mac->currentPacket = packet;
		}
		send_to_phy();
	}
	else
	{
		pstruEventDetails->dEventTime +=mac->dSlotTime;
		pstruEventDetails->nEventType = TIMER_EVENT;
		pstruEventDetails->nSubEventType = PERSISTANCE_WAIT;
		fnpAddEvent(pstruEventDetails);
	}
}
