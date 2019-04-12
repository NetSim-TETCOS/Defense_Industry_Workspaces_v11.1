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

ptrOSPFLSAHDR ospf_lsdb_lookup_lsaList(ptrOSPFLIST list,
									   OSPFID adverRouter,
									   OSPFID linkStateId)
{
	ptrOSPFLSAHDR lsa;
	void* pass = ospf_list_newIterator();
	while ((lsa = ospf_list_iterate_mem(list, pass)) != NULL)
	{
		if (!OSPFID_COMPARE(lsa->AdvertisingRouter, adverRouter) &&
			!OSPFID_COMPARE(lsa->LinkStateID, linkStateId))
		{
			ospf_list_deleteIterator(pass);
			return lsa;
		}
	}
	ospf_list_deleteIterator(pass);
	return NULL;
}

ptrOSPFLSAHDR ospf_lsdb_lookup_lsaListByID(ptrOSPFLIST list,
										   OSPFID linkStateId)
{
	ptrOSPFLSAHDR lsa;
	void* pass = ospf_list_newIterator();
	while ((lsa = ospf_list_iterate_mem(list, &pass)) != NULL)
	{
		if (!OSPFID_COMPARE(lsa->LinkStateID, linkStateId))
		{
			ospf_list_deleteIterator(pass);
			return lsa;
		}
	}
	ospf_list_deleteIterator(pass);
	return NULL;
}

ptrOSPFLSAHDR ospf_lsdb_lookup(ptrOSPF_PDS ospf,
							   ptrOSPFAREA_DS area,
							   LSTYPE lsType,
							   OSPFID adveRouter,
							   OSPFID linkStateID)
{
	switch (lsType)
	{
		case LSTYPE_ROUTERLSA:
			return ospf_lsdb_lookup_lsaList(area->routerLSAList,
											adveRouter,
											linkStateID);
			break;
		case LSTYPE_NETWORKLSA:
			return ospf_lsdb_lookup_lsaList(area->nwLSAList,
											adveRouter,
											linkStateID);
			break;
		case LSTYPE_SUMMARYLSA_ROUTER:
			return ospf_lsdb_lookup_lsaList(area->routerSummaryLSAList,
											adveRouter,
											linkStateID);
			break;
		case LSTYPE_SUMMARYLSA_NETWORK:
			return ospf_lsdb_lookup_lsaList(area->nwSummaryLSAList,
											adveRouter,
											linkStateID);
			break;
		default:
			fnNetSimError("Unknwon LSType %d in %s\n",
						  lsType,
						  __FUNCTION__);
			break;
	}
	return NULL;
}

bool ospf_lsdb_install(ptrOSPF_PDS ospf,
					   OSPFID areaId,
					   ptrOSPFLSAHDR lsa,
					   ptrOSPFLIST list)
{
	//RFC 2328 -- section 13.2
	bool ret = false;
	void* pass = ospf_list_newIterator();
	ptrOSPFLSAHDR hdr = NULL;
	ptrOSPFLSAHDR newLSA;
	while ((hdr = ospf_list_iterate_mem(list, pass)) != NULL)
	{
		if (!OSPFID_COMPARE(lsa->AdvertisingRouter, hdr->AdvertisingRouter) &&
			!OSPFID_COMPARE(lsa->LinkStateID, hdr->LinkStateID))
			break;
	}
	ospf_list_deleteIterator(pass);

	// LSA found in list
	if (hdr)
	{
		ret = ospf_lsa_is_content_changed(ospf, lsa, hdr);

		if (ret)
		{
			newLSA = OSPF_LSA_MSG_COPY(lsa);
			newLSA->time = OSPF_CURR_TIME();
			ospf_list_replace_mem(list, hdr, newLSA);

			if (lsa->LSType == LSTYPE_ROUTERLSA ||
				lsa->LSType == LSTYPE_NETWORKLSA)
				ospf_lsa_assignNewLSAIntoLSOrigin(ospf, lsa, newLSA);

			OSPF_LSA_MSG_FREE(hdr);
		}
	}
	else
	{
		newLSA = OSPF_LSA_MSG_COPY(lsa);
		newLSA->time = OSPF_CURR_TIME();
		ospf_list_add_mem(list, newLSA);
		ret = true;
	}

	ospf_lsa_printList(form_dlogId("LSDB", ospf->myId), list, "LSDB install");

	if (ospf_lsa_hasMaxAge(ospf, lsa))
		ospf_lsa_addToMaxAgeLSAList(ospf, areaId, lsa);
	return ret;
}

static int ospf_lsdb_updateLSAList(ptrOSPF_PDS ospf,
									ptrOSPF_IF thisInterface,
									ptrOSPFAREA_DS area,
									ptrOSPFLIST list,
									ptrOSPFLSAHDR lsa,
									NETSIM_IPAddress srcAddr)
{
	bool isFloodBack;
	ptrOSPFLSAHDR hdr = NULL;
	void* pass = ospf_list_newIterator();
	while ((hdr = ospf_list_iterate_mem(list, pass)) != NULL)
	{
		if (!OSPFID_COMPARE(lsa->AdvertisingRouter, hdr->AdvertisingRouter) &&
			!OSPFID_COMPARE(lsa->LinkStateID, hdr->LinkStateID))
		{
			assert(hdr->time); // Time is not updated. Somewhere forgot to update. Check!!
			print_ospf_log(OSPF_LOG, "\tLSA found in LSDB\n"
						   "    advertisingRouter %s linkStateID %s",
						   lsa->AdvertisingRouter->str_ip,
						   lsa->LinkStateID->str_ip);

			// RFC2328, Sec-13 (5.a)
			if(!ospf_lsa_isSelfOriginated(ospf,lsa) &&
			   OSPF_CURR_TIME()-hdr->time < ospf->minLSInterval)

			{
				print_ospf_log(OSPF_LOG, "Received LSA is more recent, but arrives "
							   "before minLSInterval. So don't update LSDB");
				ospf_list_deleteIterator(pass);
				return -1;
			}
			break;
		}
	}
	ospf_list_deleteIterator(pass);
	
	isFloodBack = ospf_lsa_flood(ospf,
								 area->areaId,
								 lsa,
								 srcAddr,
								 thisInterface->id);

	if (!isFloodBack)
	{
		if (thisInterface->State == OSPFIFSTATE_BACKUP)
		{
			if (!IP_COMPARE(thisInterface->designaterRouter, srcAddr))
				ospf_lsaAck_sendDelayedAck(ospf,
										   thisInterface,
										   lsa);
		}
		else
		{
			ospf_lsaAck_sendDelayedAck(ospf,
									   thisInterface,
									   lsa);
		}
	}

	return (int)ospf_lsdb_install(ospf, area->areaId, lsa, list);
}

bool ospf_lsdb_update(ptrOSPF_PDS ospf,
					  ptrOSPF_IF thisInterface,
					  ptrOSPFLSAHDR lsa,
					  ptrOSPFAREA_DS thisArea,
					  NETSIM_IPAddress srcAddr)
{
	int ret = false;

	switch (lsa->LSType)
	{
		case LSTYPE_ROUTERLSA:
			ret = ospf_lsdb_updateLSAList(ospf,
										  thisInterface,
										  thisArea,
										  thisArea->routerLSAList,
										  lsa,
										  srcAddr);
			break;
		case LSTYPE_NETWORKLSA:
			ret = ospf_lsdb_updateLSAList(ospf,
										  thisInterface,
										  thisArea,
										  thisArea->nwLSAList,
										  lsa,
										  srcAddr);
			break;
		case LSTYPE_SUMMARYLSA_NETWORK:
			ret = ospf_lsdb_updateLSAList(ospf,
										  thisInterface,
										  thisArea,
										  thisArea->nwSummaryLSAList,
										  lsa,
										  srcAddr);
			break;
		case LSTYPE_SUMMARYLSA_ROUTER:
			ret = ospf_lsdb_updateLSAList(ospf,
										  thisInterface,
										  thisArea,
										  thisArea->routerSummaryLSAList,
										  lsa,
										  srcAddr);
			break;
		default:
			fnNetSimError("Unknown LS hdr of type %s in %s",
						  strLSTYPE[lsa->LSType],
						  __FUNCTION__);
			break;
	}

	if (ret < 0)
		return false;

	if (ospf_lsa_isSelfOriginated(ospf, lsa))
	{
		bool isFlush = false;
		switch (lsa->LSType)
		{
			case LSTYPE_NETWORKLSA:
			{
				NETSIM_ID in = fn_NetSim_Stack_GetInterfaceIdFromIP(ospf->myId,
																	lsa->LinkStateID);

				ptrOSPF_IF myInterface; 
				if (in)
				{
					myInterface = OSPF_IF_GET(ospf, in);
					if (myInterface->State != OSPFIFSTATE_DR &&
						OSPFID_COMPARE(lsa->AdvertisingRouter, ospf->routerId))
						isFlush = true;
				}
			}
			break;
			case LSTYPE_SUMMARYLSA_NETWORK:
			{
#pragma message(__LOC__"Implement after routing table implementation")
			}
			break;
			case LSTYPE_SUMMARYLSA_ROUTER:
			{
#pragma message(__LOC__"Implement after routing table implementation")
			}
			break;
		}
		if (isFlush)
		{
			ospf_lsa_flush(ospf, thisArea, lsa);
		}
		else
		{
			ptrOSPFLSAHDR oldLSA = ospf_lsdb_lookup(ospf,
													thisArea,
													lsa->LSType,
													lsa->AdvertisingRouter,
													lsa->LinkStateID);
			
			oldLSA->LSSequenceNumber = lsa->LSSequenceNumber + 1;
			/*ptrEVENTLSDB elsdb = calloc(1,sizeof* elsdb);
			elsdb->area = thisArea;
			elsdb->lsType = LSTYPE_ROUTERLSA;
			elsdb->isFlush = false;
			pstruEventDetails->szOtherDetails = elsdb;
			ospf_lsa_ScheduleLSDB();
			pstruEventDetails->szOtherDetails = NULL;*/
			ospf_lsa_schedule(ospf,
							  thisInterface,
							  thisArea,
							  lsa);
		}
	}

	return ret;
}

void ospf_lsdb_scheduleMaxAgeRemovalTimer(ptrOSPF_PDS ospf)
{
	ospf_event_add(OSPF_CURR_TIME() + ospf->maxAgeRemovalTime,
				   ospf->myId,
				   0,
				   OSPF_MAXAGEREMOVALTIMER,
				   NULL,
				   NULL);
	ospf->isMaxAgeRemovalTimerSet = true;
}

static bool ospf_lsdb_isLSAInNeighborRxtList(ptrOSPF_PDS ospf,
											 OSPFID areaId,
											 ptrOSPFLSAHDR lsa)
{
	UINT i;
	for (i = 0; i < ospf->ifCount; i++)
	{
		ptrOSPF_IF thisInterface = ospf->ospfIf[i];
		if (OSPFID_COMPARE(areaId,invalidAreaId) &&
			OSPFID_COMPARE(areaId,thisInterface->areaId))
			continue;

		UINT n;
		for (n = 0; n < thisInterface->neighborRouterCount; n++)
		{
			ptrOSPF_NEIGHBOR neigh = thisInterface->neighborRouterList[n];

			ptrOSPFLSAHDR l;
			void* pass = ospf_list_newIterator();

			while ((l=ospf_list_iterate_mem(neigh->neighLSRxtList,pass))!=NULL)
			{
				// If LSA exists in retransmission list
				if (ospf_lsa_compareToListMem(ospf, l, lsa))
				{
					ospf_list_deleteIterator(pass);
					return true;
				}
			}
			ospf_list_deleteIterator(pass);
		}
	}
	return false;
}

static void ospf_lsdb_removeLSAFromList(ptrOSPFLIST list,
										ptrOSPFLSAHDR lsa)
{
	if (ospf_list_is_empty(list))
		return;

	ptrOSPFLSAHDR l;
	void* pass = ospf_list_newIterator();
	while ((l = ospf_list_iterate_mem(list, pass)) != NULL)
	{
		if (!OSPFID_COMPARE(l->AdvertisingRouter, lsa->AdvertisingRouter) &&
			!OSPFID_COMPARE(l->LinkStateID, lsa->LinkStateID))
			ospf_list_delete_mem(list, l, pass);
	}
	ospf_list_deleteIterator(pass);
}

static void ospf_lsdb_removeFromSendList(ptrOSPF_PDS ospf,
										 ptrOSPFLSAHDR lsa,
										 OSPFID areaId)
{
	UINT  i;
	for (i = 0; i < ospf->ifCount; i++)
	{
		ptrOSPF_IF thisInterface = ospf->ospfIf[i];

		// Skip the interface if it doesn't belong to specified area
		if (OSPFID_COMPARE(thisInterface->areaId, areaId) &&
			OSPFID_COMPARE(areaId, invalidAreaId))
			continue;

		UINT n;
		for (n = 0; n < thisInterface->neighborRouterCount; n++)
		{
			ptrOSPF_NEIGHBOR neigh = thisInterface->neighborRouterList[n];

			ptrOSPFLSAHDR s;
			void* pass = ospf_list_newIterator();
			while ((s = ospf_list_iterate_mem(neigh->linkStateSendList, pass)) != NULL)
			{
				if (s->LSType == lsa->LSType &&
					!OSPFID_COMPARE(s->AdvertisingRouter, lsa->AdvertisingRouter) &&
					!OSPFID_COMPARE(s->LinkStateID, lsa->LinkStateID))
				{
					ospf_list_delete_mem(neigh->linkStateSendList, s, pass);
				}
			}
			ospf_list_deleteIterator(pass);
		}
	}
}

void ospf_lsdb_removeLSA(ptrOSPF_PDS ospf,
						 ptrOSPFAREA_DS area,
						 ptrOSPFLSAHDR lsa)
{
	bool removeFromSend = true;
	switch (lsa->LSType)
	{
		case LSTYPE_ROUTERLSA:
			ospf_lsdb_removeLSAFromList(area->routerLSAList, lsa);
			break;
		case LSTYPE_NETWORKLSA:
			ospf_lsdb_removeLSAFromList(area->nwLSAList, lsa);
			break;
		case LSTYPE_SUMMARYLSA_NETWORK:
			ospf_lsdb_removeLSAFromList(area->nwSummaryLSAList, lsa);
			break;
		case LSTYPE_SUMMARYLSA_ROUTER:
			ospf_lsdb_removeLSAFromList(area->routerSummaryLSAList, lsa);
			break;
		default:
			removeFromSend = false;
			fnNetSimError("Unknown LSTYPE %d in %s.",
						  lsa->LSType, __FUNCTION__);
			break;
	}

	if (removeFromSend)
	{
		ospf_lsdb_removeFromSendList(ospf,
									 lsa,
									 area->areaId);
	}
}

void ospf_lsdb_handleMaxAgeRemovalTimer()
{
	ptrOSPF_PDS ospf = OSPF_PDS_CURRENT();
	bool scheduledTimer = false;

	if (ospf_neighbor_isAnyNeighborInExchangeOrLoadingState(ospf))
	{
		ospf_lsdb_scheduleMaxAgeRemovalTimer(ospf);
		return;
	}
	else
	{
		ospf->isMaxAgeRemovalTimerSet = false;
		
		for (UINT i = 0; i < ospf->ifCount; i++)
		{
			ptrOSPF_IF thisInterface = ospf->ospfIf[i];
			ptrOSPFAREA_DS area = OSPF_AREA_GET_ID(ospf, thisInterface->areaId);

			if (!area)
				continue;

			ptrOSPFLSAHDR lsa;
			void* pass = ospf_list_newIterator();
			while ((lsa = ospf_list_iterate_mem(area->maxAgeList, pass)) != NULL)
			{
				OSPFID areaId;
				areaId = thisInterface->areaId;
				if (ospf_lsdb_isLSAInNeighborRxtList(ospf, areaId, lsa))
				{
					if (!scheduledTimer)
					{
						ospf_lsdb_scheduleMaxAgeRemovalTimer(ospf);
						scheduledTimer = true;
					}
					continue;
				}

				ospf_lsdb_removeLSA(ospf, area, lsa);
				ospf_list_delete_mem(area->maxAgeList, lsa, pass);
			}
			ospf_list_deleteIterator(pass);
			
			if (!ospf_list_is_empty(area->maxAgeList) &&
				!ospf->isMaxAgeRemovalTimerSet &&
				!scheduledTimer)
			{
				ospf_lsdb_scheduleMaxAgeRemovalTimer(ospf);
				scheduledTimer = true;
			}
		}
	}
}

static void ospf_lsdb_refreshLSA(ptrOSPF_PDS ospf,
								 ptrOSPFLSAHDR LSHeader,
								 OSPFID areaId)
{
	ptrOSPFAREA_DS thisArea = OSPF_AREA_GET_ID(ospf, areaId);
	if (LSHeader->LSSequenceNumber == OSPF_MAX_SEQUENCE_NUMBER)
	{
		// Sequence number reaches the maximum value. We need to
		// flush this instance first before originating any instance.
		ospf_lsa_flush(ospf, thisArea, LSHeader);
		ospf_lsa_schedule(ospf, 0, thisArea, LSHeader);
	}
	else
	{
		LSHeader->LSSequenceNumber += 1;
		LSHeader->LSAge = 0;
		LSHeader->time = OSPF_CURR_TIME();

		ospf_lsa_flood(ospf, areaId, LSHeader, NULL, 0);
	}
}

static void ospf_LSDB_IncrementLSAgeInLSAList(ptrOSPF_PDS ospf,
											  ptrOSPFLIST list,
											  OSPFID areaId)
{
	ptrOSPFLSAHDR LSHeader = NULL;
	void* pass = ospf_list_newIterator();

	while ((LSHeader = ospf_list_iterate_mem(list, pass)) != NULL)
	{
		UINT16 tempAge;
		if (ospf_lsa_checkForDoNotAge(ospf, LSHeader->LSAge))
			tempAge = ospf_lsa_maskDoNotAge(ospf, LSHeader->LSAge);
		else
			tempAge = ospf_lsa_maskDoNotAge(ospf, LSHeader->LSAge) +
			((UINT16)(ospf->incrementTime / SECOND));

		if (tempAge > ospf->LSAMaxAge)
			tempAge = ospf->LSAMaxAge;

		if (ospf_lsa_maskDoNotAge(ospf, LSHeader->LSAge) ==
			ospf->LSAMaxAge)
			continue;

		// Increment LS age.
		ospf_lsa_assignNewLSAge(ospf,
								&LSHeader->LSAge,
								tempAge);

		// LS Age field of Self originated LSA reaches LSRefreshTime
		if (!OSPFID_COMPARE(LSHeader->AdvertisingRouter, ospf->routerId) &&
			ospf_lsa_maskDoNotAge(ospf, tempAge) == ospf->LSRefreshTime / SECOND)
		{
			ospf_lsdb_refreshLSA(ospf, LSHeader, areaId);
		}
		// Expired, so remove from LSDB and flood.
		else if (ospf_lsa_maskDoNotAge(ospf, tempAge) == ospf->LSAMaxAge)
		{
			ospf_lsa_flood(ospf,
						   areaId,
						   LSHeader,
						   ANY_DEST,
						   0);
			ospf_lsa_addToMaxAgeLSAList(ospf, areaId, LSHeader);
			ospf_spf_scheduleCalculation(ospf);
		}
	}
	ospf_list_deleteIterator(pass);
}

static void ospf_LSDB_removeStaleDoNotAgeLSA(ptrOSPF_PDS ospf)
{
	ptrOSPFAREA_DS thisArea = NULL;
	ptrOSPFLSAHDR LSHeader = NULL;
	void* pass = NULL;

	UINT i;
	for (i = 0; i < ospf->areaCount; i++)
	{
		thisArea = ospf->areaDS[i];
		pass = ospf_list_newIterator();
		while ((LSHeader = ospf_list_iterate_mem(thisArea->nwLSAList, pass)) != NULL)
		{
			//Check for DoNotAge LSA
			if (ospf_lsa_checkForDoNotAge(ospf, LSHeader->LSAge))
			{
				//Check for at least MaxAge Seconds
				if ((OSPF_CURR_TIME() - LSHeader->time) >= ospf->LSAMaxAge)
				{
					//Flush this LSA
					ospf_lsa_flush(ospf, thisArea, LSHeader);
					ospf_lsdb_removeLSA(ospf, thisArea, LSHeader);
				}
			}
		}
		ospf_list_deleteIterator(pass);
		pass = ospf_list_newIterator();
		while ((LSHeader = ospf_list_iterate_mem(thisArea->routerLSAList, pass)) != NULL)
		{
			//Check for DoNotAge LSA
			if (ospf_lsa_checkForDoNotAge(ospf, LSHeader->LSAge))
			{
				//Check for at least MaxAge Seconds
				if ((OSPF_CURR_TIME() - LSHeader->time) >= ospf->LSAMaxAge)
				{
					//Flush this LSA
					ospf_lsa_flush(ospf, thisArea, LSHeader);
					ospf_lsdb_removeLSA(ospf, thisArea, LSHeader);
				}
			}
		}
		ospf_list_deleteIterator(pass);
		pass = ospf_list_newIterator();
		while ((LSHeader = ospf_list_iterate_mem(thisArea->nwSummaryLSAList, pass)) != NULL)
		{
			//Check for DoNotAge LSA
			if (ospf_lsa_checkForDoNotAge(ospf, LSHeader->LSAge))
			{
				//Check for at least MaxAge Seconds
				if ((OSPF_CURR_TIME() - LSHeader->time) >= ospf->LSAMaxAge)
				{
					//Flush this LSA
					ospf_lsa_flush(ospf, thisArea, LSHeader);
					ospf_lsdb_removeLSA(ospf, thisArea, LSHeader);
				}
			}
		}
		ospf_list_deleteIterator(pass);
		pass = ospf_list_newIterator();
		while ((LSHeader = ospf_list_iterate_mem(thisArea->routerSummaryLSAList, pass)) != NULL)
		{
			//Check for DoNotAge LSA
			if (ospf_lsa_checkForDoNotAge(ospf, LSHeader->LSAge))
			{
				//Check for at least MaxAge Seconds
				if ((OSPF_CURR_TIME() - LSHeader->time) >= ospf->LSAMaxAge)
				{
					//Flush this LSA
					ospf_lsa_flush(ospf, thisArea, LSHeader);
					ospf_lsdb_removeLSA(ospf, thisArea, LSHeader);
				}
			}
		}
		ospf_list_deleteIterator(pass);
	}
}

static void ospf_lsdb_incrementLSAge(ptrOSPF_PDS ospf)
{
	UINT i;
	for (i = 0; i < ospf->areaCount; i++)
	{
		ptrOSPFAREA_DS thisArea = ospf->areaDS[i];
		ospf_LSDB_IncrementLSAgeInLSAList(ospf,
										  thisArea->nwLSAList,
										  thisArea->areaId);
		ospf_LSDB_IncrementLSAgeInLSAList(ospf,
										  thisArea->routerLSAList,
										  thisArea->areaId);
		ospf_LSDB_IncrementLSAgeInLSAList(ospf,
										  thisArea->nwSummaryLSAList,
										  thisArea->areaId);
		ospf_LSDB_IncrementLSAgeInLSAList(ospf,
										  thisArea->routerSummaryLSAList,
										  thisArea->areaId);
	}

	//RFC:1793::Section 2.3
	//As increment LSA is called after periodic intervals so
	//removal of stale DoNotAge LSAs can be done here
	//(1) The LSA has been in the router’s database
	//for at least MaxAge seconds.
	//(2) The originator of the LSA has been unreachable (according to
	//the routing calculations specified by Section 16 of [1]) for
	//at least MaxAge seconds
	if (ospf->supportDC)
		ospf_LSDB_removeStaleDoNotAgeLSA(ospf);
}

void ospf_LSDB_handle_IncrementAge_event()
{
	print_ospf_log(OSPF_LOG, "Router %d, Increment Age event is triggered at time %0.3lf",
				   pstruEventDetails->nDeviceId,
				   pstruEventDetails->dEventTime / MILLISECOND);

	ptrOSPF_PDS ospf = OSPF_PDS_CURRENT();

	ospf_event_add(OSPF_CURR_TIME() + ospf->incrementTime,
				   ospf->myId,
				   pstruEventDetails->nInterfaceId,
				   pstruEventDetails->nSubEventType,
				   NULL,
				   NULL);

	// Increment age of each LSA.
	ospf_lsdb_incrementLSAge(ospf);
}
