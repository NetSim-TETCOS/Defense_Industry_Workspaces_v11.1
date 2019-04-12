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
#include "NetSim_utility.h"
#include "OSPF.h"
#include "OSPF_enum.h"
#include "OSPF_Msg.h"
#include "OSPF_Neighbor.h"
#include "OSPF_Interface.h"
#include "OSPF_List.h"

typedef struct
{
	OSPFID routerId;
	UINT16 routerPriority;
	NETSIM_IPAddress routerAddr;
	UINT routerOption;
	OSPFID DesignatedRouter;
	OSPFID BackupDesignatedRouter;
} drEligibleRouter,*ptrdrEligibleRouter;

static void drEligibleRouter_free(ptrdrEligibleRouter d)
{
	free(d);
}

static void ospf_DR_listEligibleRouter(ptrOSPF_PDS ospf,
									   ptrOSPF_IF thisInterface,
									   ptrOSPFLIST eligibleRouterList)
{
	ptrdrEligibleRouter newRouter;

	if (thisInterface->RouterPriority > 0)
	{
		newRouter = calloc(1, sizeof* newRouter);
		newRouter->routerId = ospf->routerId;
		newRouter->routerPriority = thisInterface->RouterPriority;
		newRouter->routerAddr = thisInterface->IPIfAddr;
		newRouter->DesignatedRouter = thisInterface->designaterRouterAddr;
		newRouter->BackupDesignatedRouter = thisInterface->backupDesignaterRouterAddr;
		ospf_list_add_mem(eligibleRouterList, newRouter);
	}

	ptrOSPF_NEIGHBOR neigh;
	UINT i;
	for (i = 0; i < thisInterface->neighborRouterCount; i++)
	{
		neigh = thisInterface->neighborRouterList[i];
		if (neigh->state >= OSPFNEIGHSTATE_2Way &&
			neigh->neighborPriority > 0)
		{
			newRouter = calloc(1, sizeof* newRouter);
			newRouter->routerId = neigh->neighborId;
			newRouter->routerPriority = neigh->neighborPriority;
			newRouter->routerAddr = neigh->neighborIPAddr;
			newRouter->DesignatedRouter = neigh->neighborDesignateRouter;
			newRouter->BackupDesignatedRouter = neigh->neighborDesignateBackupRouter;
			ospf_list_add_mem(eligibleRouterList, newRouter);
		}
	}
}

static void ospf_DR_electBDR(ptrOSPF_PDS ospf,
							 ptrOSPF_IF thisInterface,
							 ptrdrEligibleRouter eligibleRouter)
{
	ptrdrEligibleRouter routerInfo;
	void* pass = ospf_list_newIterator();
	bool flag = false;
	ptrdrEligibleRouter tempBDR = NULL;

	while ((routerInfo = ospf_list_iterate_mem(eligibleRouter, pass)) != NULL)
	{
		// If the router declared itself to be DR
		// it is not eligible to become BDR
		if (!OSPFID_COMPARE(routerInfo->routerId,routerInfo->DesignatedRouter))
			continue;
		// If neighbor declared itself to be BDR
		if (!OSPFID_COMPARE(routerInfo->routerId, routerInfo->BackupDesignatedRouter))
		{
			if (flag &&
				tempBDR &&
				(tempBDR->routerPriority > routerInfo->routerPriority ||
				(tempBDR->routerPriority == routerInfo->routerPriority &&
				 tempBDR->routerId->int_ip > routerInfo->routerId->int_ip)))
			{
				// do nothing
			}
			else
			{
				tempBDR = routerInfo;
				flag = TRUE;
			}
		}
		else if (!flag)
		{
			if (tempBDR &&
				(tempBDR->routerPriority > routerInfo->routerPriority ||
				(tempBDR->routerPriority == routerInfo->routerPriority &&
				 tempBDR->routerId->int_ip > routerInfo->routerId->int_ip)))
			{
				// do nothing
			}
			else
			{
				tempBDR = routerInfo;
			}
		}
	}
	ospf_list_deleteIterator(pass);

	// Set BDR to this interface
	if (tempBDR)
	{
		thisInterface->backupDesignaterRouter = tempBDR->routerId;
		thisInterface->backupDesignaterRouterAddr = tempBDR->routerAddr;
	}
	else
	{
		thisInterface->backupDesignaterRouter = NULL;
		thisInterface->backupDesignaterRouterAddr = NULL;
	}
}

static OSPFIFSTATE ospf_DR_electDR(ptrOSPF_PDS ospf,
								   ptrOSPF_IF thisInterface,
								   ptrdrEligibleRouter eligibleRouter)
{
	ptrdrEligibleRouter routerInfo;
	void* pass = ospf_list_newIterator();
	ptrdrEligibleRouter tempDR = NULL;

	while ((routerInfo = ospf_list_iterate_mem(eligibleRouter, pass)) != NULL)
	{
		if (!OSPFID_COMPARE(routerInfo->routerAddr, routerInfo->DesignatedRouter))
		{
			if (tempDR &&
				(tempDR->routerPriority > routerInfo->routerPriority ||
				(tempDR->routerPriority == routerInfo->routerPriority &&
				 tempDR->routerId->int_ip > routerInfo->routerId->int_ip)))
			{
				// do nothing
			}
			else
			{
				tempDR = routerInfo;
			}
		}
	}
	ospf_list_deleteIterator(pass);

	// Set DR to this interface
	if (tempDR)
	{
		thisInterface->designaterRouter = tempDR->routerId;
		thisInterface->designaterRouterAddr = tempDR->routerAddr;
	}
	else
	{
		thisInterface->designaterRouter = thisInterface->backupDesignaterRouter;
		thisInterface->designaterRouterAddr = thisInterface->backupDesignaterRouterAddr;
	}

	// Return new interface state
	if (!OSPFID_COMPARE(thisInterface->designaterRouter, ospf->routerId))
	{
		return OSPFIFSTATE_DR;
	}
	else if (!OSPFID_COMPARE(thisInterface->backupDesignaterRouter, ospf->routerId))
	{
		return OSPFIFSTATE_BACKUP;
	}
	else
	{
		return OSPFIFSTATE_DROther;
	}
}

OSPFIFSTATE ospf_DR_election(ptrOSPF_PDS ospf,
							 ptrOSPF_IF thisInterface)
{
	ptrOSPFLIST eligibleRoutersList = ospf_list_init(drEligibleRouter_free, NULL);
	OSPFID oldDR;
	OSPFID oldBDR;
	OSPFIFSTATE oldState;
	OSPFIFSTATE newState;

	ospf_DR_listEligibleRouter(ospf,
							   thisInterface,
							   eligibleRoutersList);

	// RFC-2328, Section: 9.4.1
	oldDR = thisInterface->designaterRouter;
	oldBDR = thisInterface->backupDesignaterRouter;
	oldState = thisInterface->State;

	// RFC-2328, Section: 9.4.2 & 9.4.3
	// First election of DR and BDR
	ospf_DR_electBDR(ospf, thisInterface, eligibleRoutersList);
	newState = ospf_DR_electDR(ospf, thisInterface, eligibleRoutersList);
	ospf_list_delete_all(eligibleRoutersList);

	// RFC-2328, Section: 9.4.4
	if (newState != oldState &&
		(newState != OSPFIFSTATE_DROther || oldState > OSPFIFSTATE_DROther))
	{
		eligibleRoutersList = ospf_list_init(drEligibleRouter_free, NULL);
		ospf_DR_listEligibleRouter(ospf,
								   thisInterface,
								   eligibleRoutersList);

		ospf_DR_electBDR(ospf, thisInterface, eligibleRoutersList);
		newState = ospf_DR_electDR(ospf, thisInterface, eligibleRoutersList);
		ospf_list_delete_all(eligibleRoutersList);
	}

	print_ospf_log(OSPF_LOG, "Router %d declare DR = %s and BDR = %s\n"
				   "on interface %d at time %0.3lf",
				   ospf->myId,
				   thisInterface->designaterRouter->str_ip,
				   thisInterface->backupDesignaterRouter->str_ip,
				   thisInterface->id,
				   OSPF_CURR_TIME() / MILLISECOND);


	// RFC-2328, Section: 9.4.7
	if (OSPFID_COMPARE(oldDR, thisInterface->designaterRouter) ||
		OSPFID_COMPARE(oldBDR, thisInterface->backupDesignaterRouter))
	{
		UINT i;
		for (i = 0; i < thisInterface->neighborRouterCount; i++)
		{
			ptrOSPF_NEIGHBOR neigh = thisInterface->neighborRouterList[i];
			if (neigh->state == OSPFNEIGHSTATE_2Way)
			{
				ospf_event_add(OSPF_CURR_TIME(),
							   ospf->myId,
							   thisInterface->id,
							   OSPF_Adjok,
							   NULL,
							   neigh);
			}
		}
	}
	return newState;
}
