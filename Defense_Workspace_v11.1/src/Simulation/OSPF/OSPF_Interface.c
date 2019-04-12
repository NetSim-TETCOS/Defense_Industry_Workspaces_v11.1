/************************************************************************************
* Copyright (C) 2017                                                               *
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
#include "OSPF.h"
#include "OSPF_enum.h"
#include "OSPF_Interface.h"
#include "OSPF_List.h"
#include "OSPF_Msg.h"

OSPFIFTYPE OSPFIFTYPE_FROM_STR(char* val)
{
	UINT i;
	UINT len = sizeof(strOSPFIFTYPE) / sizeof(strOSPFIFTYPE[0]);
	for (i = 0; i < len; i++)
	{
		if (!_stricmp(strOSPFIFTYPE[i], val))
			return (OSPFIFTYPE)i;
	}
	return (OSPFIFTYPE)0;
}

ptrOSPF_IF OSPF_IF_GET(ptrOSPF_PDS ospf, NETSIM_ID ifId)
{
	UINT i;
	for (i = 0; i < ospf->ifCount; i++)
	{
		if (ospf->ospfIf[i]->id == ifId)
			return ospf->ospfIf[i];
	}
	return NULL;
}

static void ospf_interface_change_state(ptrOSPF_PDS ospf,
										ptrOSPF_IF thisInterface,
										OSPFIFSTATE newState)
{
	OSPFIFSTATE oldState = thisInterface->State;
	print_ospf_log(OSPF_LOG, "Interface state changed to %s from %s",
				   strOSPFIFSTATE[newState],
				   strOSPFIFSTATE[oldState]);
	thisInterface->State = newState;

	ptrOSPFAREA_DS area = OSPF_AREA_GET_ID(ospf, thisInterface->areaId);
	ospf_lsa_schedule_routerLSA(ospf, area, false);

	if (oldState != OSPFIFSTATE_DR &&
		newState == OSPFIFSTATE_DR)
	{
		print_ospf_log(OSPF_LOG, "I am new DR. Producing network LSA for this network");
		ospf_lsa_scheduleNWLSA(ospf, thisInterface, area, false);
	}
	else if (oldState == OSPFIFSTATE_DR &&
			 newState != OSPFIFSTATE_DR)
	{
		print_ospf_log(OSPF_LOG, "I am no longer DR. Flush previously originated network LSA");
		ospf_lsa_scheduleNWLSA(ospf, thisInterface, area, true);
	}
}

void ospf_handle_interfaceUp_event()
{
	ptrOSPF_PDS ospf = OSPF_PDS_CURRENT();
	ptrOSPF_IF thisInterface = OSPF_IF_CURRENT();

	if (thisInterface->State != OSPFIFSTATE_DOWN)
		return;

	// Start Hello timer & enabling periodic sending of Hello packet
	if (thisInterface->Type == OSPFIFTYPE_P2P ||
		thisInterface->Type == OSPFIFTYPE_P2MP ||
		thisInterface->Type == OSPFIFTYPE_VIRTUALLINK)
		ospf_interface_change_state(ospf, thisInterface, OSPFIFSTATE_P2P);
	else if (thisInterface->RouterPriority == 0)
		ospf_interface_change_state(ospf, thisInterface, OSPFIFSTATE_DR);
	else
	{
		ospf_interface_change_state(ospf, thisInterface, OSPFIFSTATE_WAITING);
		ospf_event_add(OSPF_CURR_TIME() +
					   thisInterface->routerDeadInterval*SECOND,
					   pstruEventDetails->nDeviceId,
					   pstruEventDetails->nInterfaceId,
					   OSPF_WAITTIMER,
					   NULL,
					   NULL);
	}
	start_interval_hello_timer();
}

void ospf_interface_init(ptrOSPF_PDS ospf, ptrOSPF_IF thisInterface)
{
	thisInterface->updateLSAList = ospf_list_init(OSPF_LSA_MSG_FREE, OSPF_LSA_MSG_COPY);
	thisInterface->delayedAckList = ospf_list_init(OSPF_LSA_MSG_FREE, OSPF_LSA_MSG_COPY);
}

void ospf_interface_handleMultipleInterfaceEvent()
{
	OSPF_Subevent e = pstruEventDetails->nSubEventType;
	ptrOSPF_PDS ospf = OSPF_PDS_CURRENT();
	ptrOSPF_IF thisInterface = OSPF_IF_CURRENT();
	OSPFIFSTATE oldState = thisInterface->State;
	OSPFIFSTATE newState = OSPFIFSTATE_DOWN;

	if(((e == OSPF_BACKUPSEEN || e == OSPF_WAITTIMER) &&
		thisInterface->State != OSPFIFSTATE_WAITING) ||
		(e == OSPF_NEIGHBORCHANGE) &&
	   (thisInterface->State != OSPFIFSTATE_DROther &&
		thisInterface->State != OSPFIFSTATE_BACKUP &&
		thisInterface->State != OSPFIFSTATE_DR))
	{
		newState = oldState;
	}
	else
	{
		newState = ospf_DR_election(ospf, thisInterface);
	}

	if (newState != oldState)
		ospf_interface_change_state(ospf, thisInterface, newState);
}

void ospf_handle_interfaceDown_event()
{
	ptrOSPF_PDS ospf = OSPF_PDS_CURRENT();
	ptrOSPF_IF thisInterface = OSPF_IF_CURRENT();
	UINT n;
	for (n = 0; n < thisInterface->neighborRouterCount; n++)
	{
		ptrOSPF_NEIGHBOR neigh = thisInterface->neighborRouterList[n];
		ospf_event_set_and_call(OSPF_KillNbr,
								neigh);
	}

	thisInterface->designaterRouter = NULL;
	thisInterface->designaterRouterAddr = NULL;
	thisInterface->backupDesignaterRouter = NULL;
	thisInterface->backupDesignaterRouterAddr = NULL;

	thisInterface->isFloodTimerSet = false;
	thisInterface->delayedAckTimer = false;
	
	ospf_list_delete_all(thisInterface->updateLSAList);
	ospf_list_delete_all(thisInterface->delayedAckList);

	ospf_interface_change_state(ospf, thisInterface, OSPFIFSTATE_DOWN);
}
