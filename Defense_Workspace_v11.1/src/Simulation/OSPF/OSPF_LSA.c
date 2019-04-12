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
#include "OSPF_RoutingTable.h"
#include "OSPF_List.h"

static void ospf_lsa_printHdr(char* logid,
							  ptrOSPFLSAHDR LSHeader)
{
	print_ospf_Dlog(logid, "Age = %d\tType = %s",
					LSHeader->LSAge,
					strLSTYPE[LSHeader->LSType]);
	print_ospf_Dlog(logid, "Link state id = %s",
					LSHeader->LinkStateID->str_ip);
	print_ospf_Dlog(logid, "Advertising router = %s",
					LSHeader->AdvertisingRouter->str_ip);
	print_ospf_Dlog(logid, "seq number = %d",
					LSHeader->LSSequenceNumber);
}

void ospf_lsa_print(char* logid,
					ptrOSPFLSAHDR LSHeader,
					char* msg)
{
	if (!ospf_dlog_isEnable(logid))
		return;

	print_ospf_Dlog(logid, "\n.....Time %lf.........\n",
					OSPF_CURR_TIME());
	print_ospf_Dlog(logid, msg);
	ospf_lsa_printHdr(logid, LSHeader);
	switch (LSHeader->LSType)
	{
		case LSTYPE_ROUTERLSA:
			ospf_lsa_printRLSA(logid, LSHeader->lsaInfo);
			break;
		default:
			fnNetSimError("Unknown LS Type %s in %s",
						  strLSTYPE[LSHeader->LSType], __FUNCTION__);
			break;
	}
}

void ospf_lsa_printList(char* logid,
						ptrOSPFLIST list,
						char* name)
{
	if (!ospf_dlog_isEnable(logid))
		return;

	print_ospf_Dlog(logid, "\n.......Time = %lf, %s..........",
					OSPF_CURR_TIME(),
					name);
	print_ospf_Dlog(logid, "Item Count = %d", ospf_list_get_size(list));
	ptrOSPFLSAHDR lsa = NULL;
	void* pass = ospf_list_newIterator();
	while ((lsa=ospf_list_iterate_mem(list,pass))!=NULL)
	{
		ospf_lsa_print(logid, lsa, name);
	}
	ospf_list_deleteIterator(pass);
}

void ospf_lsa_schedule_routerLSA(ptrOSPF_PDS ospf,
								 ptrOSPFAREA_DS area,
								 bool isFlush)
{
	double delay = 0;

	if (area->isRouterLSTimer)
		return; // LS Timer is already schedule

	area->isRouterLSTimer = true;

	//Calculate delay
	if (!area->lastLSAOriginateTime)
		delay = NETSIM_RAND_01() * OSPF_BROADCAST_JITTER + 1;
	else if (OSPF_CURR_TIME() - area->lastLSAOriginateTime >=
			 ospf->minLSInterval)
		delay = NETSIM_RAND_01() * OSPF_BROADCAST_JITTER;
	else
		delay = ospf->minLSInterval -
		(OSPF_CURR_TIME() - area->lastLSAOriginateTime);

	ptrEVENTLSDB lsdb = calloc(1, sizeof* lsdb);
	lsdb->lsType = LSTYPE_ROUTERLSA;
	lsdb->area = area;
	lsdb->isFlush = isFlush;
	ospf_event_add(pstruEventDetails->dEventTime + delay,
				   pstruEventDetails->nDeviceId,
				   pstruEventDetails->nInterfaceId,
				   OSPF_SCHEDULELSDB,
				   NULL,
				   lsdb);
}

void ospf_lsa_scheduleNWLSA(ptrOSPF_PDS ospf,
							ptrOSPF_IF thisInterface,
							ptrOSPFAREA_DS area,
							bool isFlush)
{
	double delay;
	ptrOSPFLSAHDR oldLSA = ospf_lsdb_lookup_lsaList(area->nwLSAList,
													ospf->routerId,
													thisInterface->IPIfAddr);

	bool isHaveAdjNeigh = false;
	isHaveAdjNeigh = ospf_is_dr_router_fulladjacentwithAnother(thisInterface);

	if (isHaveAdjNeigh && !isFlush)
	{
	}
	else if (oldLSA && (!isHaveAdjNeigh || isFlush))
		isFlush = true;
	else
		return;

	if (thisInterface->networkLSTimer && !isFlush)
		return;

	thisInterface->networkLSTimerSeqNumber++;

	if (thisInterface->networkLSAOriginateTime == 0 ||
		OSPF_CURR_TIME() - thisInterface->networkLSAOriginateTime >=
		ospf->minLSInterval)
	{
		delay = NETSIM_RAND_01()*OSPF_BROADCAST_JITTER;
	}
	else
	{
		delay = ospf->minLSInterval -
			(OSPF_CURR_TIME() - thisInterface->networkLSAOriginateTime);
	}

	thisInterface->networkLSTimer = true;
	ptrEVENTLSDB l = calloc(1, sizeof* l);
	l->area = area;
	l->isFlush = isFlush;
	l->lsType = LSTYPE_NETWORKLSA;
	l->nwLSASeqNum = thisInterface->networkLSTimerSeqNumber;

	ospf_event_add(OSPF_CURR_TIME() + delay,
				   ospf->myId,
				   thisInterface->id,
				   OSPF_SCHEDULELSDB,
				   NULL,
				   l);
}

void ospf_lsa_scheduleSummaryLSA(ptrOSPF_PDS ospf,
								 ptrOSPF_IF thisInterface,
								 ptrOSPFAREA_DS area,
								 NETSIM_IPAddress destAddr,
								 NETSIM_IPAddress destMask,
								 OSPFDESTTYPE destType,
								 bool isFlush)
{
	ptrEVENTLSDB l = calloc(1, sizeof* l);
	l->area = area;
	l->destAddr = destAddr;
	l->destMask = destMask;
	l->isFlush = isFlush;
	l->destType = destType;
	if (destType == OSPFDESTTYPE_NETWORK)
		l->lsType = LSTYPE_SUMMARYLSA_NETWORK;
	else if (destType == OSPFDESTTYPE_ROUTER)
		l->lsType = LSTYPE_SUMMARYLSA_ROUTER;

	ospf_event_add(OSPF_CURR_TIME() + ospf->minLSInterval,
				   ospf->myId,
				   thisInterface->id,
				   OSPF_SCHEDULELSDB,
				   NULL,
				   l);
}

void OSPFLSAINFO_FREE(ptrOSPFLSAHDR lsa)
{
	switch (lsa->LSType)
	{
		case LSTYPE_ROUTERLSA:
			OSPFLSAINFO_FREE_RLSA(lsa->lsaInfo);
			break;
		case LSTYPE_NETWORKLSA:
		case LSTYPE_SUMMARYLSA_NETWORK:
		case LSTYPE_SUMMARYLSA_ROUTER:
		default:
			fnNetSimError("Unknown LSTYPE %d in %s",
						  lsa->LSType,
						  __FUNCTION__);
			break;
	}
}

void* OSPFLSAINFO_COPY(ptrOSPFLSAHDR lsa)
{
	switch (lsa->LSType)
	{
		case LSTYPE_ROUTERLSA:
			return OSPFLSAINFO_COPY_RLSA(lsa->lsaInfo);
			break;
		case LSTYPE_NETWORKLSA:
		case LSTYPE_SUMMARYLSA_NETWORK:
		case LSTYPE_SUMMARYLSA_ROUTER:
		default:
			fnNetSimError("Unknown LSTYPE %d in %s",
						  lsa->LSType,
						  __FUNCTION__);
			break;
	}
	return NULL;
}

void OSPF_LSA_MSG_FREE(ptrOSPFLSAHDR hdr)
{
	OSPFLSAINFO_FREE(hdr);
	free(hdr);
}

ptrOSPFLSAHDR OSPF_LSA_MSG_COPY(ptrOSPFLSAHDR hdr)
{
	ptrOSPFLSAHDR lsa = calloc(1, sizeof* lsa);
	memcpy(lsa, hdr, sizeof* lsa);
	lsa->lsaInfo = OSPFLSAINFO_COPY(hdr);
	return lsa;
}

ptrOSPFLSAHDR OSPF_LSA_HDR_COPY(ptrOSPFLSAHDR lsa)
{
	ptrOSPFLSAHDR l = calloc(1, sizeof* l);
	memcpy(l, lsa, sizeof* l);
	l->lsaInfo = NULL;
	return l;
}

void OSPF_LSA_HDR_FREE(ptrOSPFLSAHDR lsa)
{
	free(lsa);
}

ptrOSPFLSAHDR ospf_lsa_find_old_lsa(ptrOSPFLIST list,
									OSPFID rid,
									OSPFID lid)
{
	void* pass = ospf_list_newIterator();
	ptrOSPFLSAHDR hdr = NULL;
	while ((hdr = ospf_list_iterate_mem(list, pass)) != NULL)
	{
		if (!OSPFID_COMPARE(hdr->AdvertisingRouter, rid) &&
			!OSPFID_COMPARE(hdr->LinkStateID, lid))
		{
			ospf_list_deleteIterator(pass);
			return hdr;
		}
	}
	ospf_list_deleteIterator(pass);
	return NULL;
}

bool ospf_lsa_update_lsahdr(ptrOSPF_PDS ospf,
							ptrOSPFAREA_DS area,
							ptrOSPFLSAHDR lsa,
							ptrOSPFLSAHDR old,
							LSTYPE lstype)
{
	lsa->LSType = lstype;
	lsa->length = OSPFLSAHDR_LEN;
	if (old)
	{
		if (old->LSSequenceNumber == OSPF_MAX_SEQUENCE_NUMBER &&
			ospf_lsa_maskDoNotAge(ospf,old->LSAge) < ospf->LSAMaxAge)
		{
			ospf_lsa_flush(ospf, area, old);
			return false;
		}
		else if (ospf_lsa_maskDoNotAge(ospf,old->LSAge) == ospf->LSAMaxAge)
		{
			// Max Age LSA is still in the Retransmission List.
			// New LSA will be originated after removal from list.
			return false;
		}
		lsa->LSSequenceNumber = old->LSSequenceNumber + 1;
	}
	else
	{
		lsa->LSSequenceNumber = OSPF_INITIAL_SEQUENCE_NUMBER;
	}
	return true;
}

void ospf_lsahdr_add_lsa(ptrOSPFLSAHDR lhdr,
						 void* lsa,
						 UINT16 len)
{
	lhdr->lsaInfo = lsa;
	lhdr->length += len;
}

static void ospf_lsa_removeFromRxtList(ptrOSPF_PDS ospf,
									   OSPFID area,
									   ptrOSPFLSAHDR lsa)
{
	UINT i;
	for (i = 0; i < ospf->ifCount; i++)
	{
		ptrOSPF_IF ospfIf = ospf->ospfIf[i];
		if (OSPFID_COMPARE(area, ospfIf->areaId))
			continue;
		UINT n;
		for (n = 0; n < ospfIf->neighborRouterCount; n++)
		{
			ptrOSPF_NEIGHBOR neigh = ospfIf->neighborRouterList[n];
			ptrOSPFLSAHDR hdr;
			ospf_lsa_printList(form_dlogId("RXTLIST", ospf->myId), neigh->neighLSRxtList, "before Rxlist remove");
			void* pass = ospf_list_newIterator();
			while ((hdr = ospf_list_iterate_mem(neigh->neighLSRxtList, pass)) != NULL)
			{
				if (!OSPFID_COMPARE(lsa->LinkStateID, hdr->LinkStateID) &&
					lsa->LSType == hdr->LSType &&
					!OSPFID_COMPARE(lsa->AdvertisingRouter, hdr->AdvertisingRouter))
				{
					ospf_list_delete_mem(neigh->neighLSRxtList, hdr, pass);
				}
			}
			ospf_list_deleteIterator(pass);
			ospf_lsa_printList(form_dlogId("RXTLIST", ospf->myId), neigh->neighLSRxtList, "after Rxlist remove");
			if (ospf_list_is_empty(neigh->neighLSRxtList) &&
				neigh->LSRxtTimer)
			{
				neigh->LSRxtTimer = false;
				neigh->LSRxtSeqNum++;
			}
		}
	}
}

bool ospf_lsa_checkForDoNotAge(ptrOSPF_PDS ospf,
							   UINT16 routerLSAAge)
{
	if (ospf->supportDC)
	{
		if (routerLSAAge & OSPF_DO_NOT_AGE)
			return true;
		else
			return false;
	}
	else
	{
		return false;
	}
}

UINT16 ospf_lsa_maskDoNotAge(ptrOSPF_PDS ospf,
							 UINT16 routerLSAAge)
{
	if (ospf_lsa_checkForDoNotAge(ospf, routerLSAAge))
		return routerLSAAge & ~OSPF_DO_NOT_AGE;
	else
		return routerLSAAge;
}

void ospf_lsa_assignNewLSAge(ptrOSPF_PDS ospf,
								UINT16* routerLSAAge,
								UINT16 newLSAAge)
{
	if (ospf_lsa_checkForDoNotAge(ospf, *routerLSAAge))
	{
		*routerLSAAge = OSPF_DO_NOT_AGE | newLSAAge;
	}
	else
	{
		*routerLSAAge = newLSAAge;
	}
}

static INT16 ospf_lsa_DiffOfAge(ptrOSPF_PDS ospf,
								UINT16 a1,
								UINT16 a2)
{
	a1 = ospf_lsa_maskDoNotAge(ospf, a1);
	a2 = ospf_lsa_maskDoNotAge(ospf, a2);

	return (a1 - a2);
}

int ospf_lsa_compare(ptrOSPF_PDS ospf,
					 ptrOSPFLSAHDR oldLS,
					 ptrOSPFLSAHDR newLS)
{
	/// RFC 2328: Section 13.1
	if (newLS->LSSequenceNumber > oldLS->LSSequenceNumber)
		return 1;

	if (oldLS->LSSequenceNumber > newLS->LSSequenceNumber)
		return -1;

	if (ospf_lsa_maskDoNotAge(ospf, newLS->LSAge) >= ospf->LSAMaxAge &&
		ospf_lsa_maskDoNotAge(ospf, oldLS->LSAge) < ospf->LSAMaxAge)
		return 1;

	if (ospf_lsa_maskDoNotAge(ospf, oldLS->LSAge) >= ospf->LSAMaxAge &&
		ospf_lsa_maskDoNotAge(ospf, newLS->LSAge) < ospf->LSAMaxAge)
		return -1;

	if ((abs(ospf_lsa_DiffOfAge(ospf, newLS->LSAge, oldLS->LSAge)) >
		 OSPF_LSA_MAX_AGE_DIFF / SECOND) &&
		 (ospf_lsa_maskDoNotAge(ospf, newLS->LSAge) <
		  ospf_lsa_maskDoNotAge(ospf, oldLS->LSAge)))
		return 1;

	if ((abs(ospf_lsa_DiffOfAge(ospf, newLS->LSAge, oldLS->LSAge)) >
		 OSPF_LSA_MAX_AGE_DIFF / SECOND) &&
		 (ospf_lsa_maskDoNotAge(ospf, newLS->LSAge) >
		  ospf_lsa_maskDoNotAge(ospf, oldLS->LSAge)))
		return -1;

	return 0;
}

bool ospf_lsa_compareToListMem(ptrOSPF_PDS ospf,
							   ptrOSPFLSAHDR oldLS,
							   ptrOSPFLSAHDR newLS)
{
	if (oldLS->LSType == newLS->LSType &&
		!OSPFID_COMPARE(oldLS->AdvertisingRouter, newLS->AdvertisingRouter) &&
		!OSPFID_COMPARE(oldLS->LinkStateID, newLS->LinkStateID) &&
		!ospf_lsa_compare(ospf, oldLS, newLS))
		return true;
	return false;
}

void ospf_lsa_queueToFlood(ptrOSPF_PDS ospf,
						   ptrOSPF_IF thisInterface,
						   ptrOSPFLSAHDR lsa)
{
	void* item = ospf_list_newIterator();
	ptrOSPFLSAHDR hdr = NULL;
	ptrOSPFLSAHDR updateLSA = OSPF_LSA_MSG_COPY(lsa);
	while ((hdr = ospf_list_iterate_mem(thisInterface->updateLSAList, item)) != NULL)
	{
		if (hdr->LSAge == lsa->LSAge &&
			!OSPFID_COMPARE(hdr->LinkStateID, lsa->LinkStateID) &&
			!OSPFID_COMPARE(hdr->AdvertisingRouter, lsa->AdvertisingRouter))
		{
			if (ospf_lsa_compare(ospf, hdr, lsa) > 0)
			{
				ospf_list_delete_mem(thisInterface->updateLSAList, hdr, item);
				ospf_list_add_mem(thisInterface->updateLSAList, updateLSA);
			}
			return;
		}
	}
	ospf_list_deleteIterator(item);
	ospf_list_add_mem(thisInterface->updateLSAList, updateLSA);
}

bool ospf_lsa_flood(ptrOSPF_PDS pds,
					OSPFID area,
					ptrOSPFLSAHDR lsa,
					NETSIM_IPAddress srcAddr,
					NETSIM_ID in)
{
	bool floodBack = false;
	ptrOSPFLIST list = ospf_list_init(NULL, NULL);

	ospf_lsa_removeFromRxtList(pds, area, lsa);
	UINT i;
	for (i = 0; i < pds->ifCount; i++)
	{
		bool isAddedToRxtList = false;
		ptrOSPF_IF ospf = pds->ospfIf[i];

		if (ospf->State == OSPFIFSTATE_DOWN)
			continue;

		if (OSPFID_COMPARE(ospf->areaId, area))
			continue;

		UINT n;
		for (n = 0; n < ospf->neighborRouterCount; n++)
		{
			ptrOSPF_NEIGHBOR neigh = ospf->neighborRouterList[n];
			if(neigh->state <OSPFNEIGHSTATE_Exchange)
				continue;

			if (neigh->state != OSPFNEIGHSTATE_Full)
			{
				ptrOSPFLSAHDR lsHdr = ospf_lsReq_searchFromList(neigh, lsa);

				if (lsHdr)
				{
					int isRecent = ospf_lsa_compare(pds, lsHdr, lsa);
					if (isRecent < 0)
						continue;
					if (isRecent == 0)
					{
						ospf_lsreq_removeFromReqList(neigh->neighLSReqList, lsHdr);

						if (ospf_list_is_empty(neigh->neighLSReqList) &&
							 neigh->state == OSPFNEIGHSTATE_Loading)
						{
							// Cancel retransmission timer
							neigh->LSRxtSeqNum += 1;
							ospf_event_set_and_call(OSPF_LoadingDone,
													neigh);
							continue;
						}
					}
					else
					{
						ospf_lsreq_removeFromReqList(neigh->neighLSReqList, lsHdr);

						if (ospf_list_is_empty(neigh->neighLSReqList) &&
							neigh->state == OSPFNEIGHSTATE_Loading)
						{
							// Cancel retransmission timer
							neigh->LSRxtSeqNum += 1;
							ospf_list_add_mem(list, neigh);
						}
					}
				}
			}

			if (!srcAddr || !IP_COMPARE(srcAddr, neigh->neighborIPAddr))
				continue;

			ptrOSPFLSAHDR rxtlsa = OSPF_LSA_MSG_COPY(lsa);
			ospf_list_add_mem(neigh->neighLSRxtList, rxtlsa);
			ospf_lsa_printList(form_dlogId("RXTLIST", pds->myId), neigh->neighLSRxtList, "add Rxlist");
			isAddedToRxtList = true;
		}
		if (!isAddedToRxtList)
			continue;

		if (ospf->Type == OSPFIFTYPE_BROADCAST &&
			IP_COMPARE(srcAddr, ANY_DEST) &&
			ospf->id == in)
		{
			if (!IP_COMPARE(srcAddr, ospf->designaterRouter) ||
				!IP_COMPARE(srcAddr, ospf->backupDesignaterRouter))
				continue;

			if (ospf->State == OSPFIFSTATE_BACKUP)
				continue;
		}

		if (IP_COMPARE(srcAddr, ANY_DEST) &&
			ospf->id == in)
			floodBack = true;

		ospf_lsa_queueToFlood(pds, ospf, lsa);

		if (!ospf->isFloodTimerSet)
		{
			double t = pstruEventDetails->dEventTime;
			t = max(t, ospf->lastFloodTimer + pds->minLSInterval);
			t += pds->dFloodTimer*NETSIM_RAND_01();
			ospf_event_add(t,
						   pstruEventDetails->nDeviceId,
						   ospf->id,
						   OSPF_FLOODTIMER,
						   NULL,
						   NULL);
			ospf->lastFloodTimer = t;
			ospf->isFloodTimerSet = true;
		}
	}

	void* pass = ospf_list_newIterator();
	ptrOSPF_NEIGHBOR neigh;
	while ((neigh = ospf_list_iterate_mem(list, pass)) != NULL)
		ospf_event_set_and_call(OSPF_LoadingDone, neigh);
	ospf_list_deleteIterator(pass);
	ospf_list_destroy(list);
	return floodBack;
}

static bool ospf_lsa_isBodyChanged(ptrOSPFLSAHDR newLSA,
								   ptrOSPFLSAHDR oldLSA)
{
	switch (newLSA->LSType)
	{
		case LSTYPE_ROUTERLSA:
			return ospf_rlsa_isBodyChanged(newLSA, oldLSA);
		case LSTYPE_NETWORKLSA:
		case LSTYPE_SUMMARYLSA_ROUTER:
		case LSTYPE_SUMMARYLSA_NETWORK:
		case LSTYPE_ASEXTERNALLSA:
			fnNetSimError("Implement lsa_isbodychange function");
#pragma message(__LOC__"Implement lsa_isbodychange function")
			break;
		default:
			fnNetSimError("Unknown LS Type %d in %s\n",
						  newLSA->LSType, __FUNCTION__);
			return false;
	}
	return false;
}

bool ospf_lsa_is_content_changed(ptrOSPF_PDS ospf,
								 ptrOSPFLSAHDR newLSA,
								 ptrOSPFLSAHDR oldLSA)
{
	if (newLSA->Options != oldLSA->Options)
		return true;

	if ((ospf_lsa_maskDoNotAge(ospf, newLSA->LSAge) == ospf->LSAMaxAge &&
		 ospf_lsa_maskDoNotAge(ospf, oldLSA->LSAge) != ospf->LSAMaxAge) ||
		 (ospf_lsa_maskDoNotAge(ospf, newLSA->LSAge) != ospf->LSAMaxAge &&
		  ospf_lsa_maskDoNotAge(ospf, oldLSA->LSAge) == ospf->LSAMaxAge))
		return true;

	if (newLSA->length != oldLSA->length)
		return true;

	if (ospf_lsa_isBodyChanged(newLSA, oldLSA))
		return true;

	return false;
}

void ospf_lsa_assignNewLSAIntoLSOrigin(ptrOSPF_PDS ospf,
									   ptrOSPFLSAHDR LSA,
									   ptrOSPFLSAHDR newLSA)
{
	UINT i;
	ptrOSPFROUTINGTABLE rtTable = ospf->routingTable;
	
	for (i = 0; i < rtTable->numRow; i++)
	{
		ptrOSPFROUTINGTABLEROW row = rtTable->rows[i];
		if (row->LSOrigin == LSA)
			row->LSOrigin = newLSA;
	}
}

bool ospf_lsa_hasMaxAge(ptrOSPF_PDS ospf,
						ptrOSPFLSAHDR lsa)
{
	return ospf_lsa_maskDoNotAge(ospf, lsa->LSAge) == ospf->LSAMaxAge;
}

static bool ospf_lsa_isPresentInMaxAgeList(ptrOSPF_PDS ospf,
										   ptrOSPFLIST list,
										   ptrOSPFLSAHDR lsa)
{
	ptrOSPFLSAHDR hdr;
	void* pass = ospf_list_newIterator();
	while ((hdr = ospf_list_iterate_mem(list, pass)) != NULL)
	{
		if (hdr->LSType == lsa->LSType &&
			!OSPFID_COMPARE(hdr->LinkStateID, lsa->LinkStateID) &&
			!OSPFID_COMPARE(hdr->AdvertisingRouter, lsa->AdvertisingRouter) &&
			!ospf_lsa_compare(ospf, lsa, hdr))
		{
			ospf_list_deleteIterator(pass);
			return true;
		}
	}
	ospf_list_deleteIterator(pass);
	return false;
}

void ospf_lsa_addToMaxAgeLSAList(ptrOSPF_PDS ospf,
								 OSPFID areaId,
								 ptrOSPFLSAHDR lsa)
{
	ptrOSPFAREA_DS area = OSPF_AREA_GET_ID(ospf, areaId);

	if (!ospf_lsa_isPresentInMaxAgeList(ospf, area->maxAgeList, lsa))
	{
		ptrOSPFLSAHDR newLSA = OSPF_LSA_MSG_COPY(lsa);
		ospf_list_add_mem(area->maxAgeList, newLSA);

		if (!ospf->isMaxAgeRemovalTimerSet)
		{
			ospf_lsdb_scheduleMaxAgeRemovalTimer(ospf);
		}
	}
}

void ospf_lsa_ScheduleLSDB()
{
	ptrEVENTLSDB lsdb = pstruEventDetails->szOtherDetails;
	LSTYPE lsType = lsdb->lsType;

	if (lsType == LSTYPE_ROUTERLSA)
	{
		ospf_rlsa_originateRouterLSA(lsdb->area, lsdb->isFlush);
	}
	else
	{
		fnNetSimError("write other condition for different LSA type");
#pragma message(__LOC__"write other condition for different LSA type")
	}
	free(lsdb);
}

void ospf_lsa_handle_floodTimer_event()
{
	ptrOSPF_IF ospf = OSPF_IF_CURRENT();
	if (!ospf->isFloodTimerSet)
		return;
	ospf_lsupdate_send();
	ospf->isFloodTimerSet = false;
}

int ospf_lsa_isMoreRecent(ptrOSPF_PDS ospf, ptrOSPFLSAHDR newLSA, ptrOSPFLSAHDR oldLSA)
{
	if (!oldLSA)
		return 1;
	if (!newLSA)
		return 0;
	return ospf_lsa_compare(ospf, oldLSA, newLSA);
}

bool ospf_lsa_isSelfOriginated(ptrOSPF_PDS ospf,
							   ptrOSPFLSAHDR lsa)
{
	if (!OSPFID_COMPARE(lsa->AdvertisingRouter, ospf->routerId) ||
		(ospf_isMyAddr(ospf->myId, lsa->LinkStateID) &&
		 lsa->LSType == LSTYPE_NETWORKLSA))
		return true;
	else
		return false;
}

void ospf_lsa_flush(ptrOSPF_PDS ospf,
					ptrOSPFAREA_DS area,
					ptrOSPFLSAHDR lsa)
{
	if (lsa->LSAge == ospf->LSAMaxAge)
		return;

	lsa->LSAge = ospf->LSAMaxAge;

	ospf_lsa_flood(ospf, area->areaId, lsa, ANY_DEST, 0);
	ospf_lsa_addToMaxAgeLSAList(ospf, area->areaId, lsa);
	ospf_spf_scheduleCalculation(ospf);
}

void ospf_lsa_schedule(ptrOSPF_PDS ospf,
					   ptrOSPF_IF thisInterface,
					   ptrOSPFAREA_DS area,
					   ptrOSPFLSAHDR lsa)
{
	switch (lsa->LSType)
	{
		case LSTYPE_ROUTERLSA:
			ospf_lsa_schedule_routerLSA(ospf,
										area,
										false);
			break;
		case LSTYPE_NETWORKLSA:
			ospf_lsa_scheduleNWLSA(ospf,
								   thisInterface,
								   area,
								   false);
			break;
		case LSTYPE_SUMMARYLSA_NETWORK:
			ospf_lsa_scheduleSummaryLSA(ospf,
										thisInterface,
										area,
										lsa->LinkStateID,
										DEVICE_MYMASK(),
										OSPFDESTTYPE_NETWORK,
										false);
			break;
		case LSTYPE_SUMMARYLSA_ROUTER:
			ospf_lsa_scheduleSummaryLSA(ospf,
										thisInterface,
										area,
										lsa->LinkStateID,
										ANY_DEST,
										OSPFDESTTYPE_ASBR,
										false);
			break;
		default:
			fnNetSimError("Unknown LS type %s in %s",
						  strLSTYPE[lsa->LSType],
						  __FUNCTION__);
	}
}

static bool ospf_nwlsa_hasLink(ptrOSPF_PDS ospf,
							   ptrOSPFLSAHDR wlsa,
							   ptrOSPFLSAHDR vlsa)
{
#pragma message(__LOC__"Implement after NW LSA implementation")
	fnNetSimError("Implement after NW LSA implementation");
	return false;
}

bool ospf_lsa_hasLink(ptrOSPF_PDS ospf,
					  ptrOSPFLSAHDR wlsa,
					  ptrOSPFLSAHDR vlsa)
{
	if (wlsa->LSType == LSTYPE_ROUTERLSA)
		return ospf_rlsa_hasLink(ospf, wlsa, vlsa);
	else if (wlsa->LSType == LSTYPE_NETWORKLSA)
		return ospf_nwlsa_hasLink(ospf, wlsa, vlsa);
	return false;
}
