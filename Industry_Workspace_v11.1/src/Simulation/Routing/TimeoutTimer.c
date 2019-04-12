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
#include "main.h"
#include "Routing.h"
/**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	The timeout is initialized when a route is established, and any time 
	an update message is received for the route. If 180 seconds elapse from
	the last time the timeout was initialized, the route is considered to 
	have expired
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ 
 */
int fn_NetSim_RIP_Timeout_Timer(struct stru_NetSim_Network *pstruNETWORK,NetSim_EVENTDETAILS *pstruEventDetails)
{
	RIP_ROUTING_DATABASE *pstruTempTable;
	unsigned int nRIP_UpdateVar=0;
	NETWORK=pstruNETWORK;
	NETSIM_ID d = pstruEventDetails->nDeviceId;
	DEVICE_ROUTER* rip = get_RIP_var(d);
	pstruTempTable=NETWORK->ppstruDeviceList[pstruEventDetails->nDeviceId-1]->pstruNetworkLayer->RoutingVar;
	while(pstruTempTable!=NULL)
	{
		//if there is no update for one entry, then assign the metric as Expired(16 i,e Infinity)
		if(nRIP_UpdateVar==rip->uniInteriorRouting.struRIP.nStatus)
		{
			pstruTempTable->nMetric=EXPIRED_ROUTE;
			pstruEventDetails->dPacketSize=RIP_PACKET_SIZE_WITH_HEADER;
			pstruEventDetails->nApplicationId=0;
			//Add the garbage collection timer value to trigger garbage collection timer event
			pstruEventDetails->dEventTime=pstruEventDetails->dEventTime+rip->uniInteriorRouting.struRIP.n_garbage_collection_timer;
			pstruEventDetails->nEventType=TIMER_EVENT;
			pstruEventDetails->nProtocolId=APP_PROTOCOL_RIP;
			pstruEventDetails->nSubEventType=RIP_GARBAGE_COLLECTION;
			fnpAddEvent(pstruEventDetails);		
		}
		pstruTempTable=pstruTempTable->pstru_Router_NextEntry;
	}
	return 1;
}

