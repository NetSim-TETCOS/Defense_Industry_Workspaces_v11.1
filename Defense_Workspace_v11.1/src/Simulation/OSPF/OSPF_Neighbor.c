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
#include "OSPF.h"
#include "OSPF_Msg.h"
#include "OSPF_Neighbor.h"
#include "OSPF_Enum.h"
#include "OSPF_Interface.h"
#include "OSPF_List.h"

#define is_neighbor_DR(ospf,neigh) (!OSPFID_COMPARE(ospf->designaterRouter,neigh->neighborId))
#define is_neighbor_backupDR(ospf, neigh) (!OSPFID_COMPARE(ospf->backupDesignaterRouter,neigh->neighborId))

static void ospf_neighbor_attempt_adjacency(ptrOSPF_NEIGHBOR neigh)
{
	if (neigh->DDSeqNo)
	{
		neigh->DDSeqNo++;
		print_ospf_log(OSPF_LOG, "New DD seq no is %d", neigh->DDSeqNo);
	}
	else
	{
		print_ospf_log(OSPF_LOG, "First time adjacency is attempted. Declaring itself as master.");
		neigh->isMaster = true;
		neigh->DDSeqNo = (UINT)(pstruEventDetails->dEventTime / MILLISECOND);
		print_ospf_log(OSPF_LOG, "New DD seq no is %d", neigh->DDSeqNo);
		print_ospf_log(OSPF_LOG, "Start sending DD msg");
		start_sending_dd_msg();
	}
}

static void ospf_neighbor_change_state(ptrOSPF_NEIGHBOR neigh,
									   OSPFNEIGHSTATE state)
{
	OSPFNEIGHSTATE oldState = neigh->state;
	print_ospf_log(OSPF_LOG, "Neighbor(%s) state changed to %s from %s",
				   neigh->neighborId->str_ip,
				   strNeighborState[state],
				   strNeighborState[neigh->state]);
	neigh->state = state;

	// Attempt to form adjacency if new state is S_ExStart
	if ((oldState != state) && (state == OSPFNEIGHSTATE_ExStart))
	{
		{
			ospf_neighbor_attempt_adjacency(neigh);
		}
	}

	if ((oldState == OSPFNEIGHSTATE_Full &&
		 neigh->state != OSPFNEIGHSTATE_Full) ||
		 (oldState != OSPFNEIGHSTATE_Full &&
		  neigh->state == OSPFNEIGHSTATE_Full))
	{
		ptrOSPF_PDS ospf = OSPF_PDS_CURRENT();
		ptrOSPF_IF thisInterface = OSPF_IF_CURRENT();
		ptrOSPFAREA_DS area = OSPF_AREA_GET_IN(ospf, pstruEventDetails->nInterfaceId);
		ospf_lsa_schedule_routerLSA(ospf, area, false);
		if (thisInterface->State == OSPFIFSTATE_DR)
		{
			ospf_lsa_scheduleNWLSA(ospf,
								   thisInterface,
								   area,
								   false);
		}
	}

	if ((oldState < OSPFNEIGHSTATE_2Way &&
		state >= OSPFNEIGHSTATE_2Way) ||
		 (oldState >= OSPFNEIGHSTATE_2Way &&
		state < OSPFNEIGHSTATE_2Way))
	{
		ospf_event_add(pstruEventDetails->dEventTime,
					   pstruEventDetails->nDeviceId,
					   pstruEventDetails->nInterfaceId,
					   OSPF_NEIGHBORCHANGE,
					   NULL,
					   neigh);
	}
}

ptrOSPF_NEIGHBOR OSPF_NEIGHBOR_FIND(ptrOSPF_IF ospf, OSPFID id)
{
	UINT i;
	for (i = 0; i < ospf->neighborRouterCount; i++)
	{
		ptrOSPF_NEIGHBOR neigh = ospf->neighborRouterList[i];
		if (!OSPFID_COMPARE(neigh->neighborId, id) ||
			!OSPFID_COMPARE(neigh->neighborIPAddr,id))
			return neigh;
	}
	return NULL;
}

ptrOSPF_NEIGHBOR OSPF_NEIGHBOR_FIND_BY_IP(ptrOSPF_IF thisInterface, NETSIM_IPAddress ip)
{
	UINT i;
	for (i = 0; i < thisInterface->neighborRouterCount; i++)
	{
		ptrOSPF_NEIGHBOR neigh = thisInterface->neighborRouterList[i];
		if (!IP_COMPARE(neigh->neighborIPAddr, ip))
			return neigh;
	}
	return NULL;
}

ptrOSPF_NEIGHBOR ospf_neighbor_new(NETSIM_IPAddress ip,
								   OSPFID rid)
{
	ptrOSPF_NEIGHBOR neigh;
	neigh = calloc(1, sizeof* neigh);
	neigh->devId = fn_NetSim_Stack_GetDeviceId_asIP(rid, &neigh->devInterface);
	neigh->neighborId = rid;
	neigh->neighborIPAddr = ip;

	ospf_neighbor_change_state(neigh, OSPFNEIGHSTATE_DOWN);

	neigh->neighLSReqList = ospf_lsreq_initList();
	neigh->neighLSRxtList = ospf_list_init(OSPF_LSA_MSG_FREE, OSPF_LSA_MSG_COPY);
	neigh->neighDBSummaryList = ospf_list_init(OSPF_LSA_MSG_FREE, OSPF_LSA_MSG_COPY);
	neigh->linkStateSendList = ospf_list_init(OSPF_LSA_MSG_FREE, OSPF_LSA_MSG_COPY);

	return neigh;
}

void ospf_neighbor_add(ptrOSPF_IF ospf, ptrOSPF_NEIGHBOR neigh)
{
	if (ospf->neighborRouterCount)
		ospf->neighborRouterList = realloc(ospf->neighborRouterList,
		(ospf->neighborRouterCount + 1) * (sizeof* ospf->neighborRouterList));
	else
		ospf->neighborRouterList = calloc(1, sizeof* ospf->neighborRouterList);
	ospf->neighborRouterList[ospf->neighborRouterCount] = neigh;
	ospf->neighborRouterCount++;
}

void ospf_neighbor_remove(ptrOSPF_IF thisInterface, ptrOSPF_NEIGHBOR neigh)
{
	UINT n;
	bool flag = false;
	for (n = 0; n < thisInterface->neighborRouterCount; n++)
	{
		if (thisInterface->neighborRouterList[n] == neigh)
			flag = true;
		if (flag)
			thisInterface->neighborRouterList[n] = thisInterface->neighborRouterList[n + 1];
	}
	if (flag)
		thisInterface->neighborRouterCount--;

	ospf_list_delete_all(neigh->linkStateSendList);
	ospf_list_destroy(neigh->linkStateSendList);
	
	ospf_list_delete_all(neigh->neighDBSummaryList);
	ospf_list_destroy(neigh->neighDBSummaryList);

	ospf_list_delete_all(neigh->neighLSReqList);
	ospf_list_destroy(neigh->neighLSReqList);

	ospf_list_delete_all(neigh->neighLSRxtList);
	ospf_list_destroy(neigh->neighLSRxtList);

	OSPF_HDR_FREE(neigh->lastrecvDDPacket);
	OSPF_HDR_FREE(neigh->lastSentDDPacket);

	fnDeleteEvent(neigh->inactivityTimerId);
	neigh->inactivityTimerId = false;
	free(neigh);
}

void ospf_neighbor_handle_1way_event()
{
	ptrOSPF_NEIGHBOR neigh = pstruEventDetails->szOtherDetails;

	print_ospf_log(OSPF_LOG, "Time %0.4lf, Router %d, Interface %d (%s) 1-way event is triggered for neighbor %s",
				   pstruEventDetails->dEventTime/MILLISECOND,
				   pstruEventDetails->nDeviceId,
				   pstruEventDetails->nInterfaceId,
				   DEVICE_MYIP()->str_ip,
				   neigh->neighborId->str_ip);

	if (neigh->state == OSPFNEIGHSTATE_2Way)
		ospf_neighbor_change_state(neigh, OSPFNEIGHSTATE_Init);

	print_ospf_log(OSPF_LOG, "\n");
}

static bool is_adjacency_should_established(ptrOSPF_IF ospf,
											ptrOSPF_NEIGHBOR neigh)
{
	if (ospf->Type == OSPFIFTYPE_P2P ||
		ospf->Type == OSPFIFTYPE_P2MP ||
		ospf->Type == OSPFIFTYPE_VIRTUALLINK)
		return true;

	if (ospf->State == OSPFIFSTATE_DR)
		return true;

	if (ospf->State == OSPFIFSTATE_BACKUP)
		return true;

	if (is_neighbor_DR(ospf, neigh))
		return true;
	
	if (is_neighbor_backupDR(ospf, neigh))
		return true;

	return false;
}

static void ospf_handle_2wayReceived_event_in_init_state(ptrOSPF_IF ospf,
														 ptrOSPF_NEIGHBOR neigh)
{
	print_ospf_log(OSPF_LOG, "Neighbor state is Init");

	bool isAdjacency = is_adjacency_should_established(ospf, neigh);
	if (isAdjacency)
	{
		print_ospf_log(OSPF_LOG, "Adjacency is required");
		ospf_neighbor_change_state(neigh, OSPFNEIGHSTATE_ExStart);
	}
	else
	{
		print_ospf_log(OSPF_LOG, "Adjacency is not required");
		ospf_neighbor_change_state(neigh, OSPFNEIGHSTATE_2Way);
	}
}

void ospf_neighbor_handle_2wayReceived_event()
{
	ptrOSPF_IF ospf = OSPF_IF_CURRENT();
	ptrOSPF_NEIGHBOR neigh = pstruEventDetails->szOtherDetails;

	print_ospf_log(OSPF_LOG, "Time %0.4lf: Router %d, interface %d (%s) 2-Way received event triggered for neighbor %s",
				   pstruEventDetails->dEventTime / MILLISECOND,
				   pstruEventDetails->nDeviceId,
				   pstruEventDetails->nInterfaceId,
				   DEVICE_MYIP()->str_ip,
				   neigh->neighborId->str_ip);

	if (neigh->state == OSPFNEIGHSTATE_Init)
		ospf_handle_2wayReceived_event_in_init_state(ospf, neigh);
	print_ospf_log(OSPF_LOG, "\n");
}

void ospf_neighbor_handle_helloReceived_event()
{
	ptrOSPF_NEIGHBOR neigh = pstruEventDetails->szOtherDetails;

	print_ospf_log(OSPF_LOG, "Time %0.4lf, Router %d, interface %d (%s) HelloReceived event triggered for neighbor %s",
				   pstruEventDetails->dEventTime/MILLISECOND,
				   pstruEventDetails->nDeviceId,
				   pstruEventDetails->nInterfaceId,
				   DEVICE_MYIP()->str_ip,
				   neigh->neighborId->str_ip);

	if (neigh->state == OSPFNEIGHSTATE_DOWN)
		ospf_neighbor_change_state(neigh, OSPFNEIGHSTATE_Init);

	neigh->lastHelloRecvTime = OSPF_CURR_TIME();
	if (!neigh->isInactivityTimerAdded)
	{
		ptrOSPF_IF ospf = OSPF_IF_CURRENT();

		double time = pstruEventDetails->dEventTime +
			ospf->routerDeadInterval*SECOND;

		print_ospf_log(OSPF_LOG, "Adding Inactivity timer at time %0.4lf",
					   time/MILLISECOND);

		neigh->inactivityTimerId = ospf_event_add(time,
												  pstruEventDetails->nDeviceId,
												  pstruEventDetails->nInterfaceId,
												  OSPF_InactivityTimer,
												  NULL,
												  neigh);
		neigh->isInactivityTimerAdded = true;
	}
	print_ospf_log(OSPF_LOG, "\n");
}

static void ospf_neighbor_addToRxtList(ptrOSPF_PDS ospf,
									   ptrOSPF_IF thisInterface,
									   ptrOSPF_NEIGHBOR neigh,
									   ptrOSPFLSAHDR lsa)
{
	lsa = OSPF_LSA_HDR_COPY(lsa);

	lsa->time = OSPF_CURR_TIME();
	ospf_list_add_mem(neigh->neighLSRxtList, lsa);
	ospf_lsa_printList(form_dlogId("RXTLIST", ospf->myId), neigh->neighLSRxtList, "add Rxlist");
	ospf_list_add_mem(neigh->linkStateSendList, OSPF_LSA_HDR_COPY(lsa));

	if (!neigh->LSRxtTimer)
	{
		ptrLSRXTTIMERDETAILS detail = calloc(1, sizeof* detail);
		detail->advertisingRouter = lsa->AdvertisingRouter;
		detail->msgType = OSPFMSG_LSUPDATE;
		detail->neighborIP = neigh->neighborIPAddr;
		detail->rxmtSeqNum = neigh->LSRxtSeqNum;
		ospf_event_add(OSPF_CURR_TIME() + thisInterface->RxmtInterval*SECOND,
					   ospf->myId,
					   thisInterface->id,
					   OSPF_RXMTTIMER,
					   NULL,
					   detail);
		neigh->LSRxtTimer = true;
	}
}

static void ospf_neighbor_update_DBSummaryList(ptrOSPF_PDS ospf,
											   ptrOSPF_IF thisInterface,
											   ptrOSPF_NEIGHBOR neigh,
											   ptrOSPFLIST list)
{
	ptrOSPFLSAHDR lsa = NULL;
	void* item = ospf_list_newIterator();
	while ((lsa = ospf_list_iterate_mem(list, item)) != NULL)
	{
		// Add to retransmission list if LSAge is MaxAge
		if (ospf_lsa_maskDoNotAge(ospf, lsa->LSAge) == ospf->LSAMaxAge)
		{
			ospf_neighbor_addToRxtList(ospf,
									   thisInterface,
									   neigh,
									   lsa);
			continue;
		}

		ptrOSPFLSAHDR cpy = OSPF_LSA_HDR_COPY(lsa);
		ospf_list_add_mem(neigh->neighDBSummaryList, cpy);
	}
	ospf_list_deleteIterator(item);
}

static void ospf_neighbor_create_DBSummaryList(ptrOSPF_PDS pds,
											   ptrOSPF_IF ospf,
											   ptrOSPF_NEIGHBOR neigh)
{
	ptrOSPFAREA_DS area = OSPF_AREA_GET_IN(pds, ospf->id);
	ospf_neighbor_update_DBSummaryList(pds, ospf, neigh, area->routerLSAList);
	ospf_neighbor_update_DBSummaryList(pds, ospf, neigh, area->nwLSAList);
	ospf_neighbor_update_DBSummaryList(pds, ospf, neigh, area->routerSummaryLSAList);
	ospf_neighbor_update_DBSummaryList(pds, ospf, neigh, area->nwSummaryLSAList);
}

void ospf_neighbor_handle_negotiationDone_event()
{
	ptrOSPF_NEIGHBOR neigh = pstruEventDetails->szOtherDetails;
	print_ospf_log(OSPF_LOG, "Time %0.4lf, Router %d, Interface %d (%s), %s event is triggered for neighbor %s",
				   pstruEventDetails->dEventTime / MILLISECOND,
				   pstruEventDetails->nDeviceId,
				   pstruEventDetails->nInterfaceId,
				   DEVICE_MYIP()->str_ip,
				   GetStringOSPF_Subevent(pstruEventDetails->nSubEventType),
				   neigh->neighborId->str_ip);
	print_ospf_log(OSPF_LOG, "Neighbor state is %s",
				   strNeighborState[neigh->state]);
	if(neigh->state != OSPFNEIGHSTATE_ExStart)
		return;

	ospf_neighbor_change_state(neigh, OSPFNEIGHSTATE_Exchange);
	print_ospf_log(OSPF_LOG, "Creating DB summary list");
	ospf_neighbor_create_DBSummaryList(OSPF_PDS_CURRENT(),
									   OSPF_IF_CURRENT(),
									   neigh);
	print_ospf_log(OSPF_LOG, "");
}

void ospf_neighbor_handle_exchangeDone_event()
{
	ptrOSPF_PDS ospf = OSPF_PDS_CURRENT();
	ptrOSPF_NEIGHBOR neigh = pstruEventDetails->szOtherDetails;
	print_ospf_log(OSPF_LOG, "Time %0.4lf, Router %d, Interface %d (%s), %s event is triggered for neighbor %s",
				   pstruEventDetails->dEventTime / MILLISECOND,
				   pstruEventDetails->nDeviceId,
				   pstruEventDetails->nInterfaceId,
				   DEVICE_MYIP()->str_ip,
				   GetStringOSPF_Subevent(pstruEventDetails->nSubEventType),
				   neigh->neighborId->str_ip);
	print_ospf_log(OSPF_LOG, "Neighbor state is %s",
				   strNeighborState[neigh->state]);

	if (neigh->state != OSPFNEIGHSTATE_Exchange)
		return;

	if (ospf_list_is_empty(neigh->neighLSReqList))
	{
		print_ospf_log(OSPF_LOG, "Neighbor LSR list is empty");
		ospf_neighbor_change_state(neigh, OSPFNEIGHSTATE_Full);
	}
	else
	{
		print_ospf_log(OSPF_LOG, "Neighbor LSR is not empty");
		ospf_neighbor_change_state(neigh, OSPFNEIGHSTATE_Loading);
		ospf_lsreq_send(ospf,
						pstruEventDetails->nInterfaceId,
						neigh->neighborIPAddr,
						false);
	}
	print_ospf_log(OSPF_LOG, "");
}

void ospf_neighbor_handle_start_event()
{
	//For NBMA network type
	//Ignore
}

static void restart_inactivity_timer(ptrOSPF_IF ospf,
									 ptrOSPF_NEIGHBOR neigh)
{
	double time = neigh->lastHelloRecvTime +
		ospf->routerDeadInterval*SECOND;

	print_ospf_log(OSPF_LOG, "Adding Inactivity timer at time %0.4lf",
				   time / MILLISECOND);

	ospf_event_add(time,
				   pstruEventDetails->nDeviceId,
				   pstruEventDetails->nInterfaceId,
				   OSPF_InactivityTimer,
				   NULL,
				   neigh);
}

void ospf_neighbor_handle_inactivityTimer_event()
{
	ptrOSPF_IF ospf = OSPF_IF_CURRENT();
	ptrOSPF_NEIGHBOR neigh = pstruEventDetails->szOtherDetails;

	print_ospf_log(OSPF_LOG, "Time %0.4lf, Router %d, Interface %d (%s), "
				   "%s event is triggered for neighbor %s.",
				   pstruEventDetails->dEventTime / MILLISECOND,
				   pstruEventDetails->nDeviceId,
				   pstruEventDetails->nInterfaceId,
				   DEVICE_MYIP()->str_ip,
				   GetStringOSPF_Subevent(pstruEventDetails->nSubEventType),
				   neigh->neighborId->str_ip);

	double lt = neigh->lastHelloRecvTime;
	if (lt + ospf->routerDeadInterval*SECOND <=
		pstruEventDetails->dEventTime)
	{
		print_ospf_log(OSPF_LOG, "Neighbor is inactive");
		ospf_neighbor_change_state(neigh, OSPFNEIGHSTATE_DOWN);
		
		ospf_list_delete_all(neigh->neighLSReqList);
		ospf_list_destroy(neigh->neighLSReqList);
		neigh->neighLSReqList = NULL;
		
		ospf_list_delete_all(neigh->neighDBSummaryList);
		ospf_list_destroy(neigh->neighDBSummaryList);
		neigh->neighDBSummaryList = NULL;

		ospf_list_delete_all(neigh->neighLSRxtList);
		ospf_list_destroy(neigh->neighLSRxtList);
		neigh->neighLSRxtList = NULL;

		ospf_list_delete_all(neigh->linkStateSendList);
		ospf_list_destroy(neigh->linkStateSendList);
		neigh->linkStateSendList = NULL;

		neigh->isInactivityTimerAdded = false;
		
		OSPF_HDR_FREE(neigh->lastrecvDDPacket);
		neigh->lastrecvDDPacket = NULL;
		
		OSPF_HDR_FREE(neigh->lastSentDDPacket);
		neigh->lastSentDDPacket = NULL;

		neigh->neighborDesignateBackupRouter = NULL;
		neigh->neighborDesignateRouter = NULL;

	}
	else
	{
		print_ospf_log(OSPF_LOG, "Neighbor is active");
		restart_inactivity_timer(ospf, neigh); //reduce number of event for faster execution
	}
	print_ospf_log(OSPF_LOG, "");
}

bool ospf_is_router_fullAdjacentWithDR(ptrOSPF_IF ospf)
{
	UINT i;
	for (i = 0; i < ospf->neighborRouterCount; i++)
	{
		ptrOSPF_NEIGHBOR neigh = ospf->neighborRouterList[i];
		if (!OSPFID_COMPARE(neigh->neighborId, ospf->designaterRouter) &&
			neigh->state == OSPFNEIGHSTATE_Full)
			return true;
	}
	return false;
}

bool ospf_is_dr_router_fulladjacentwithAnother(ptrOSPF_IF ospf)
{
	UINT i;
	for (i = 0; i < ospf->neighborRouterCount; i++)
	{
		ptrOSPF_NEIGHBOR neigh = ospf->neighborRouterList[i];
		if (neigh->state == OSPFNEIGHSTATE_Full)
			return true;
	}
	return false;
}

bool ospf_neighbor_isAnyNeighborInExchangeOrLoadingState(ptrOSPF_PDS ospf)
{
	UINT i, n;
	for (i = 0; i < ospf->ifCount; i++)
	{
		ptrOSPF_IF thisInterface = ospf->ospfIf[i];
		ptrOSPF_NEIGHBOR neigh;
		for (n = 0; n < thisInterface->neighborRouterCount; n++)
		{
			neigh = thisInterface->neighborRouterList[n];
			if (neigh->state == OSPFNEIGHSTATE_Exchange ||
				neigh->state == OSPFNEIGHSTATE_Loading)
				return true;
		}
	}
	return false;
}

ptrOSPFLSAHDR ospf_neighbor_searchSendList(ptrOSPFLIST list,
										   ptrOSPFLSAHDR lsa)
{
	ptrOSPFLSAHDR l;
	void* pass = ospf_list_newIterator();
	while ((l = ospf_list_iterate_mem(list, pass)) != NULL)
	{
		if (l->LSType == lsa->LSType &&
			!OSPFID_COMPARE(l->AdvertisingRouter, lsa->AdvertisingRouter) &&
			!OSPFID_COMPARE(l->LinkStateID, lsa->LinkStateID))
		{
			ospf_list_deleteIterator(pass);
			return l;
		}
	}
	ospf_list_deleteIterator(pass);
	return NULL;
}

void ospf_neighbor_insertToSendList(ptrOSPFLIST list,
									ptrOSPFLSAHDR lsa,
									double time)
{
	ptrOSPFLSAHDR s = OSPF_LSA_HDR_COPY(lsa);
	s->time = time;
	ospf_list_add_mem(list, s);
}

NETSIM_ID ospf_neighbor_getInterfaceIdforThisNeighbor(ptrOSPF_PDS ospf,
													  NETSIM_IPAddress neighIPaddr)
{
	UINT i;
	for (i = 0; i < ospf->ifCount; i++)
	{
		ptrOSPF_IF thisInterface = ospf->ospfIf[i];

		UINT n;
		for(n=0;n<thisInterface->neighborRouterCount;n++)
		{
			ptrOSPF_NEIGHBOR neigh = thisInterface->neighborRouterList[n];
			if (!IP_COMPARE(neigh->neighborIPAddr, neighIPaddr))
				return thisInterface->id;
		}
	}
	return 0;
}

void ospf_neighbor_handle_LoadingDoneEvent()
{
	NETSIM_ID d = pstruEventDetails->nDeviceId;
	ptrOSPF_NEIGHBOR tempNeighborInfo = pstruEventDetails->szOtherDetails;

	if (tempNeighborInfo->state != OSPFNEIGHSTATE_Loading)
		return;

	if (tempNeighborInfo->state != OSPFNEIGHSTATE_Full)
		print_ospf_log(OSPF_LOG, "Router %d, LoadingDone event is triggered"
					   "Neighbor (%s) state move up to Full", d, tempNeighborInfo->neighborId);

	ospf_neighbor_change_state(tempNeighborInfo, OSPFNEIGHSTATE_Full);
}

void ospf_neighbor_handle_KillNbrEvent()
{
	ptrOSPF_PDS ospf = OSPF_PDS_CURRENT();
	ptrOSPF_IF thisInterface = OSPF_IF_CURRENT();
	ptrOSPF_NEIGHBOR neighbor = pstruEventDetails->szOtherDetails;

	ospf_neighbor_change_state(neighbor, OSPFNEIGHSTATE_DOWN);
	ospf_neighbor_remove(thisInterface, neighbor);

}