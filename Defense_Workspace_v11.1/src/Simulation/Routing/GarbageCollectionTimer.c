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
#ifndef _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_DEPRECATE
#endif
#include "main.h"
#include "Routing.h"
/**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	Until the garbage-collection timer expires, the route is included in all 
	updates sent by the router.	When the garbage collection timer expires, the route is deleted 
	from the routing table  
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
int fn_NetSim_RIP_Garbage_Collection_Timer(struct stru_NetSim_Network *pstruNETWORK,NetSim_EVENTDETAILS *pstruEventDetails)
{
	RIP_ROUTING_DATABASE *pstruTmpHeadEntry;
	RIP_ROUTING_DATABASE *pstruTempTableList;
	RIP_ROUTING_DATABASE *pstruNewTable;

	NETWORK=pstruNETWORK;
	pstruTmpHeadEntry=NETWORK->ppstruDeviceList[pstruEventDetails->nDeviceId-1]->pstruNetworkLayer->RoutingVar;
	pstruTempTableList=pstruTmpHeadEntry;

	while(pstruTempTableList)
	{
		//check whether the entry is invalid or not if the entry is invalid remove that entry
		if(pstruTempTableList->nMetric==EXPIRED_ROUTE)
		{
			pstruNewTable = pstruTempTableList;
			pstruTempTableList=pstruTempTableList->pstru_Router_NextEntry;
			pstruNewTable->pstru_Router_NextEntry = NULL;

			if(NETWORK->ppstruDeviceList[pstruEventDetails->nDeviceId-1]->pstruNetworkLayer->RoutingVar == pstruNewTable)
				NETWORK->ppstruDeviceList[pstruEventDetails->nDeviceId-1]->pstruNetworkLayer->RoutingVar = pstruTempTableList;

			if(pstruNewTable->szAddress)
				IP_FREE(pstruNewTable->szAddress);

			if(pstruNewTable->szRouter)
				IP_FREE(pstruNewTable->szRouter);

			fnpFreeMemory(pstruNewTable);
		}
		else
			pstruTempTableList=pstruTempTableList->pstru_Router_NextEntry;
	}
	return 1;
}


