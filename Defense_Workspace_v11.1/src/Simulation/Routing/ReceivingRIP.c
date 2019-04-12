/************************************************************************************
* Copyright (C) 2012     
*
* TETCOS, Bangalore. India                                                         *

* Tetcos owns the intellectual property rights in the Product and its content.     *
* The copying, redistribution, reselling or publication of any or all of the       *
* Product or its content without express prior written consent of Tetcos is        *
* prohibited. Ownership and / or any other right relating to the software and all  *
* intellectual property rights therein shall remain at all times with Tetcos.      *

* Author:  Thangarasu.K                                                       *
* ---------------------------------------------------------------------------------*/
#define _CRT_SECURE_NO_DEPRECATE
#include "main.h"
#include "Routing.h"
/**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	The RIP message from the neighbor routers are received in this function, After receiving
	the RIP message trigger the timeout timer event
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ 
 */
_declspec(dllexport) int fn_NetSim_RIP_ReceivingOf_RIP_Message(struct stru_NetSim_Network *pstruNETWORK,NetSim_EVENTDETAILS *pstruEventDetails)
{
	NETSIM_ID nDeviceId;
	double dEventTime;
	RIP_ENTRY *pstruEntry;
	// NetSim packet To store the RIP packet
	NetSim_PACKET *pstruControlPacket = NULL;
	// RIP PACKET to store RIP packet data 	
	RIP_PACKET *pstruPacketRip = NULL;	
	pstruControlPacket=pstruEventDetails->pPacket;
	pstruPacketRip=pstruControlPacket->pstruAppData->Packet_AppProtocol;
	pstruEntry=pstruPacketRip->pstruRIPEntry;
	NETWORK=pstruNETWORK;
	nDeviceId=pstruEventDetails->nDeviceId;
	dEventTime=pstruEventDetails->dEventTime;
	get_RIP_var(nDeviceId)->uniInteriorRouting.struRIP.nStatus++;
	pstruEventDetails->nEventType=APPLICATION_IN_EVENT;
	pstruEventDetails->nProtocolId=APP_PROTOCOL_RIP;
	pstruEventDetails->nSubEventType=ROUTING_TABLE_UPDATION;
	fnpAddEvent(pstruEventDetails);
	pstruEventDetails->pPacket = NULL;
	while(pstruEntry!=NULL)
	{
		pstruEventDetails->nApplicationId=0;
		//Assign the packet size as 512
		pstruEventDetails->dPacketSize=RIP_PACKET_SIZE_WITH_HEADER;
		pstruEventDetails->nEventType=TIMER_EVENT;
		pstruEventDetails->nProtocolId=APP_PROTOCOL_RIP;
		pstruEventDetails->nSubEventType=RIP_TIMEOUT;
		// Add the timeout timer with the event time to trigger the timeout event
		pstruEventDetails->dEventTime=dEventTime+get_RIP_var(nDeviceId)->uniInteriorRouting.struRIP.n_timeout_timer;
		fnpAddEvent(pstruEventDetails);
		pstruEntry=pstruEntry->pstru_RIP_NextEntry;
	}		
	return 0;
}


