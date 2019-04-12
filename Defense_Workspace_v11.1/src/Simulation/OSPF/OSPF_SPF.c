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
#include "OSPF_Neighbor.h"
#include "OSPF_Msg.h"
#include "OSPF_Enum.h"
#include "OSPF_Interface.h"
#include "OSPF_List.h"
#include "OSPF_RoutingTable.h"

static ptrOSPFVERTEX OSPF_SPF_COPY_VERTEX(ptrOSPFVERTEX vertex)
{
	ptrOSPFVERTEX v = calloc(1, sizeof* v);
	memcpy(v, vertex, sizeof* v);
	v->nextHopList = ospf_list_copyAll(vertex->nextHopList);
	return v;
}

static void OSPF_SPF_FREE_VERTEX(ptrOSPFVERTEX vertex)
{
	ospf_list_delete_all(vertex->nextHopList);
	free(vertex);
}

//Print function
static void ospf_spf_printVertexList(ptrOSPFLIST shortestPathList)
{
	ptrOSPFVERTEX entry;
	void* pass = ospf_list_newIterator();
	while ((entry = ospf_list_iterate_mem(shortestPathList,pass)) != NULL)
	{
		print_ospf_log(OSPF_SPF_LOG, "\tVertex Id = %s", entry->vertexId->str_ip);
		print_ospf_log(OSPF_SPF_LOG, "\tVertex Type = %s",strOSPFVERTEXTYPE[entry->vertexType]);
		print_ospf_log(OSPF_SPF_LOG, "\tMetric = %d", entry->distance);

		ptrOSPFNEXTHOPLISTITEM nextHopItem;
		void* npass = ospf_list_newIterator();
		UINT i = 0;
		while ((nextHopItem = ospf_list_iterate_mem(entry->nextHopList, npass)) != NULL)
		{
			if(nextHopItem->nextHop)
				print_ospf_log(OSPF_SPF_LOG, "\tNext Hop[%d] = %s",
							   i,
							   nextHopItem->nextHop->str_ip);
			else
				print_ospf_log(OSPF_SPF_LOG, "\tNext Hop[%d] = Directly connected",
							   i);
			i++;
		}
		ospf_list_deleteIterator(npass);
	}
	ospf_list_deleteIterator(pass);
}

static void ospf_spf_printShortestpath(ptrOSPF_PDS ospf,
									   ptrOSPFAREA_DS area)
{
	ptrOSPFLIST shortestPathList = area->shortestPathList;
	print_ospf_log(OSPF_SPF_LOG, "Shortest path list for router %u, area %s",
				   ospf->myId,
				   area->areaId->str_ip);
	print_ospf_log(OSPF_SPF_LOG,"    size = %d", ospf_list_get_size(shortestPathList));

	ospf_spf_printVertexList(shortestPathList);
}

void ospf_spf_scheduleCalculation(ptrOSPF_PDS ospf)
{
	if (ospf->SPFTimer > pstruEventDetails->dEventTime)
		return;
	double t = pstruEventDetails->dEventTime;
	t += ospf->spfCalcDelay*NETSIM_RAND_01();

	ospf_event_add(t,
				   pstruEventDetails->nDeviceId,
				   0,
				   OSPF_CALCULATESPF,
				   NULL,
				   NULL);
}

static void ospf_spf_invalidateRoutingTable(ptrOSPF_PDS ospf)
{
	ptrOSPFROUTINGTABLEROW* rowPtr = ospf->routingTable->rows;
	UINT i;
	for (i = 0; i < ospf->routingTable->numRow; i++)
	{
		rowPtr[i]->flag = OSPFROUTEFLAG_INVALID;
	}
}

static void ospf_spf_updateCandidateListUsingNWLSA(ptrOSPF_PDS ospf,
												   ptrOSPFAREA_DS area,
												   ptrOSPFLIST candidateList,
												   ptrOSPFVERTEX vertex)
{
	fnNetSimError("Implement after NWLSA implementation");
#pragma message(__LOC__"Implement after NWLSA implementation")
}

static bool ospf_spf_isInShortestPathList(ptrOSPF_PDS ospf,
										  ptrOSPFLIST shortestPathList,
										  OSPFVERTEXTYPE vertexType,
										  NETSIM_IPAddress vertexId)
{
	ptrOSPFVERTEX vertex;
	void* pass = ospf_list_newIterator();
	while ((vertex = ospf_list_iterate_mem(shortestPathList, pass)) != NULL)
	{
		// Found it.
		if (vertex->vertexType == vertexType &&
			!IP_COMPARE(vertex->vertexId, vertexId))
		{
			ospf_list_deleteIterator(pass);
			return true;
		}
	}
	ospf_list_deleteIterator(pass);
	return false;
}

static ptrOSPFVERTEX ospf_spf_findCandidate(ptrOSPF_PDS ospf,
											ptrOSPFLIST candidateList,
											OSPFVERTEXTYPE vertexType,
											NETSIM_IPAddress vertexId)
{
	ptrOSPFVERTEX vertex;
	void* pass = ospf_list_newIterator();
	while ((vertex = ospf_list_iterate_mem(candidateList, pass)) != NULL)
	{
		if (vertex->vertexType == vertexType &&
			!IP_COMPARE(vertex->vertexId, vertexId))
		{
			ospf_list_deleteIterator(pass);
			return vertex;
		}
	}
	ospf_list_deleteIterator(pass);
	return NULL;
}

static NETSIM_IPAddress ospf_spf_getLinkDataForThisVertex(ptrOSPF_PDS ospf,
														  ptrOSPFVERTEX vertex,
														  ptrOSPFVERTEX parent)
{
	ptrOSPFLSAHDR lsa = parent->lsa;
	ptrOSPFRLSA rtrLSA = lsa->lsaInfo;
	UINT i;
	
	for (i = 0; i < rtrLSA->linksCount; i++)
	{
		ptrOSPFRLSALINK link = rtrLSA->rlsaLink[i];
		if (link->type != OSPFLINKTYPE_STUB &&
			!IP_COMPARE(link->linkId, vertex->vertexId))
			return link->linkData;
	}
	// Shouldn't get here
	fnNetSimError("Not get link data from the associated link "
				   "for this vertex %s\n",
				   vertex->vertexId->str_ip);
	return NULL;
}

static bool ospf_spf_haveLinkWithNetworkVertex(ptrOSPF_PDS ospf,
											   ptrOSPFVERTEX vertex)
{
	NETSIM_ID d = ospf->myId;
	UINT i;
	for (i = 0; i < ospf->ifCount; i++)
	{
		ptrOSPF_IF thisInterface = ospf->ospfIf[i];
		NETSIM_ID in = thisInterface->id;
		NETSIM_IPAddress subnetMask = DEVICE_SUBNETMASK(d, in);
		NETSIM_IPAddress netAddr = DEVICE_NWADDRESS(d, in);

		if (IP_IS_IN_SAME_NETWORK_IPV4(vertex->vertexId,
									   netAddr,
									   subnetMask) &&
			thisInterface->State != OSPFIFSTATE_DOWN)
			return true;
	}
	return false;
}

static bool ospf_spf_isPresentinNextHopList(ptrOSPFLIST nextHopList,
											ptrOSPFNEXTHOPLISTITEM item)
{
	ptrOSPFNEXTHOPLISTITEM i;
	void* pass = ospf_list_newIterator();
	while ((i = ospf_list_iterate_mem(nextHopList,pass)) != NULL)
	{
		if (i == item)
		{
			ospf_list_deleteIterator(pass);
			return true;
		}
	}
	ospf_list_deleteIterator(pass);
	return false;
}

static bool ospf_spf_setNextHopForThisVertex(ptrOSPF_PDS ospf,
											 ptrOSPFVERTEX vertex,
											 ptrOSPFVERTEX parent)
{
	ptrOSPFNEXTHOPLISTITEM nextHopItem;
	print_ospf_log(OSPF_SPF_LOG, "Parent = %s, Vertex = %s",
				   parent->vertexId->str_ip,
				   vertex->vertexId->str_ip);
	if (parent->vertexType == OSPFVERTEXTYPE_ROUTER &&
		!IP_COMPARE(parent->vertexId, ospf->routerId))
	{
		NETSIM_IPAddress linkData = NULL;
		NETSIM_ID interfaceId;

		print_ospf_log(OSPF_SPF_LOG, "Parent is ROOT");
		linkData = ospf_spf_getLinkDataForThisVertex(ospf, vertex, parent);
		interfaceId = fn_NetSim_Stack_GetInterfaceIdFromIP(ospf->myId, linkData);

		ptrOSPF_IF thisInterface = OSPF_IF_GET(ospf, interfaceId);
		if (thisInterface->State == OSPFIFSTATE_DOWN)
			return false;

		nextHopItem = calloc(1, sizeof* nextHopItem);
		nextHopItem->outIntf = interfaceId;

		if (vertex->vertexType == OSPFVERTEXTYPE_ROUTER)
		{
			int in;
			ptrOSPF_NEIGHBOR neigh;
			NETSIM_IPAddress nbrIPAddress = NULL;

			in = fn_NetSim_Stack_GetInterfaceIdFromIP(ospf->myId, linkData);
			ptrOSPF_IF cif = OSPF_IF_GET(ospf, in);
			neigh = OSPF_NEIGHBOR_FIND(cif, vertex->vertexId);
			if (!neigh &&
				!nbrIPAddress)
			{
				free(nextHopItem);
				return false;
			}
			else if (neigh)
			{
				nbrIPAddress = neigh->neighborIPAddr;
			}
			nextHopItem->nextHop = nbrIPAddress;
		}
		else
		{
			// Vertex is directly connected network
			nextHopItem->nextHop = NULL;
		}

		if (!ospf_spf_isPresentinNextHopList(vertex->nextHopList, nextHopItem))
		{
			ospf_list_add_mem(vertex->nextHopList, nextHopItem);
		}
		else
		{
			free(nextHopItem);
			return false;
		}
	}
	else if (parent->vertexType == OSPFVERTEXTYPE_NETWORK &&
		ospf_spf_haveLinkWithNetworkVertex(ospf, parent))
	{
		// If parent is root, vertex is either directly connected router
		// or network
		// Else if parent is network that directly connects with root

		NETSIM_IPAddress linkData = NULL;
		NETSIM_ID interfaceId;

		linkData = ospf_spf_getLinkDataForThisVertex(ospf, vertex, parent);
		interfaceId = fn_NetSim_Stack_GetInterfaceIdFromIP(ospf->myId,
														linkData);

		if (!interfaceId)
			return false;

		ptrOSPF_IF thisInterface = OSPF_IF_GET(ospf, interfaceId);
		if (thisInterface->State == OSPFIFSTATE_DOWN)
			return false;

		nextHopItem = calloc(1, sizeof* nextHopItem);
		nextHopItem->nextHop = linkData;
		nextHopItem->outIntf = interfaceId;

		if (!ospf_spf_isPresentinNextHopList(vertex->nextHopList, nextHopItem))
		{
			ospf_list_add_mem(vertex->nextHopList, nextHopItem);
		}
		else
		{
			free(nextHopItem);
			return false;
		}
	}
	else
	{
		// There should be an intervening router. So inherits the set
		// of next hops from parent
		bool inserted = false;

		ptrOSPFNEXTHOPLISTITEM listItem;
		void* pass = ospf_list_newIterator();
		while ((listItem=ospf_list_iterate_mem(parent->nextHopList,pass))!=NULL)
		{
			nextHopItem = calloc(1, sizeof* nextHopItem);
			memcpy(nextHopItem, listItem, sizeof* nextHopItem);

			if (!ospf_spf_isPresentinNextHopList(vertex->nextHopList,
											  nextHopItem))
			{
				ospf_list_add_mem(vertex->nextHopList, nextHopItem);
				inserted = true;
			}
			else
			{
				free(nextHopItem);
			}
		}
		ospf_list_deleteIterator(pass);
		return inserted;
	}
	return true;
}

static void ospf_spf_updateCandidateListUsingRouterLSA(ptrOSPF_PDS ospf,
													   ptrOSPFAREA_DS area,
													   ptrOSPFLIST candidateList,
													   ptrOSPFVERTEX vertex)
{
	ptrOSPFRLSA vlsa = vertex->lsa->lsaInfo;
	ptrOSPFLSAHDR wlsa;

	print_ospf_log(OSPF_SPF_LOG, "\tLooking for vertex %s from router LSA",
				   vertex->vertexId->str_ip);

	UINT i;
	for (i = 0; i < vlsa->linksCount; i++)
	{
		OSPFVERTEXTYPE newVertexType;
		NETSIM_IPAddress newVertexId;
		UINT newVertexDistance;
		ptrOSPFVERTEX candidateListItem = NULL;
		ptrOSPFRLSALINK link = vlsa->rlsaLink[i];
		print_ospf_log(OSPF_SPF_LOG, "\tExamine link %s, link type %s",
					   link->linkId->str_ip, strOSPFLINKTYPE[link->type]);

		if (link->type == OSPFLINKTYPE_POINT_TO_POINT)
		{
			wlsa = ospf_lsdb_lookup_lsaList(area->routerLSAList,
											link->linkId,
											link->linkId);

			newVertexType = OSPFVERTEXTYPE_ROUTER;
		}
		else if (link->type == OSPFLINKTYPE_TRANSIT)
		{
			wlsa = ospf_lsdb_lookup_lsaListByID(area->nwLSAList, link->linkId);
			newVertexType = OSPFVERTEXTYPE_NETWORK;
		}
		else
		{
			continue;
		}

		// RFC2328, Sec-16.1 (2.b & 2.c)
		if (wlsa == NULL ||
			ospf_lsa_hasMaxAge(ospf, wlsa) ||
			/*!ospf_rlsa_hasLink(ospf, wlsa, vertex->lsa) ||*/ // don't understand why this is not working here!! :(
			ospf_spf_isInShortestPathList(ospf,
										  area->shortestPathList,
										  newVertexType,
										  link->linkId))
		{
			if (wlsa == NULL)
				print_ospf_log(OSPF_SPF_LOG, "\twlsa is NULL");
			else if (ospf_lsa_hasMaxAge(ospf, wlsa))
				print_ospf_log(OSPF_SPF_LOG, "\tWLSA has MAX AGE");
			else if (!ospf_rlsa_hasLink(ospf, wlsa, vertex->lsa))
				print_ospf_log(OSPF_SPF_LOG, "\tNo Link between wLSA and vLSA");
			continue;
		}

		newVertexId = wlsa->LinkStateID;
		newVertexDistance = vertex->distance + link->metric;

		candidateListItem = ospf_spf_findCandidate(ospf,
												   candidateList,
												   newVertexType,
												   newVertexId);
		if (!candidateListItem)
		{
			// Insert new candidate
			candidateListItem = calloc(1, sizeof* candidateListItem);

			candidateListItem->vertexId = newVertexId;
			candidateListItem->vertexType = newVertexType;
			candidateListItem->lsa = wlsa;
			candidateListItem->distance = newVertexDistance;
			candidateListItem->nextHopList = ospf_list_init(NULL, NULL);
			print_ospf_log(OSPF_SPF_LOG, "\n\tPossible new candidate is %s", newVertexId->str_ip);
			if (ospf_spf_setNextHopForThisVertex(ospf,candidateListItem,vertex))
			{
				print_ospf_log(OSPF_SPF_LOG, "\tInserting new vertex %s\n", newVertexId->str_ip);
				ospf_list_add_mem(candidateList, candidateListItem);
			}
			else
			{
				print_ospf_log(OSPF_SPF_LOG, "\tSet next hop is FALSE for candidate %s\n", newVertexId->str_ip);
				ospf_list_delete_all(candidateListItem->nextHopList);
				free(candidateListItem);
			}
		}
		else if (candidateListItem->distance > newVertexDistance)
		{
			// update
			candidateListItem->distance = newVertexDistance;

			// Free old next hop items
			ospf_list_delete_all(candidateListItem->nextHopList);
			ospf_spf_setNextHopForThisVertex(ospf, candidateListItem, vertex);
		}
		else if (candidateListItem->distance == newVertexDistance)
		{
			// Add new set of next hop values
			ospf_spf_setNextHopForThisVertex(ospf, candidateListItem, vertex);
		}
		else
		{
			// Examine next link
			continue;
		}
	}
}

static void ospf_spf_updateCandidateList(ptrOSPF_PDS ospf,
										 ptrOSPFAREA_DS area,
										 ptrOSPFLIST candidateList,
										 ptrOSPFVERTEX vertex)
{
	if (vertex->vertexType == OSPFVERTEXTYPE_NETWORK)
		ospf_spf_updateCandidateListUsingNWLSA(ospf, area, candidateList, vertex);
	else
		ospf_spf_updateCandidateListUsingRouterLSA(ospf, area, candidateList, vertex);
}

static void ospf_spf_updateIntraAreaRoute(ptrOSPF_PDS ospf,
										  ptrOSPFAREA_DS area,
										  ptrOSPFVERTEX vertex)
{
	OSPFROUTINGTABLEROW newRow;
	ptrOSPFNEXTHOPLISTITEM nextHopItem = NULL;
	ptrOSPF_IF thisInterface = NULL;
	if (!ospf_list_is_empty(vertex->nextHopList))
	{
		nextHopItem = ospf_list_get_headptr(vertex->nextHopList);
		thisInterface = OSPF_IF_GET(ospf, nextHopItem->outIntf);
	}

	// If vertex is router
	if (vertex->vertexType == OSPFVERTEXTYPE_ROUTER)
	{
		ptrOSPFLSAHDR lsa = vertex->lsa;
		ptrOSPFRLSA routerLSA = vertex->lsa->lsaInfo;

		if(Ospf_rlsa_getASBRouter(routerLSA->VEB) ||
			Ospf_rlsa_getABRouter(routerLSA->VEB))
		{
			// Add entry of vertex to routing table
			if (Ospf_rlsa_getABRouter(routerLSA->VEB) &&
				Ospf_rlsa_getASBRouter(routerLSA->VEB))
				newRow.destType = OSPFDESTTYPE_ABR_ASBR;
			else if(Ospf_rlsa_getABRouter(routerLSA->VEB))
				newRow.destType = OSPFDESTTYPE_ABR;
			else
				newRow.destType = OSPFDESTTYPE_ASBR;

			newRow.destAddr = vertex->vertexId;
			newRow.addrMask = ANY_DEST;

			newRow.option = lsa->Options;

			newRow.areaId = area->areaId;
			newRow.pathType = OSPFPATHTYPE_INTRA_AREA;
			newRow.metric = vertex->distance;
			newRow.type2Metric = 0;
			newRow.LSOrigin = (void*)vertex->lsa;
			newRow.advertisingRouter = lsa->AdvertisingRouter;
			newRow.nextHop = nextHopItem->nextHop;
			newRow.outInterface = DEVICE_NWADDRESS(ospf->myId, nextHopItem->outIntf);
			newRow.outInterfaceId = nextHopItem->outIntf;

			ospf_rtTable_addRoute(ospf, &newRow);
		}
	}
	// Else it must be a transit network vertex
	else if (nextHopItem != NULL &&
			 thisInterface->includeSubnetRts)
	{
		fnNetSimError("Reserved for future use. This will work after network LSA implementation.\n");
#pragma message(__LOC__"Implement after network LSA implementation")
	}
}

static ptrOSPFVERTEX ospf_spf_updateShortestPathList(ptrOSPF_PDS ospf,
													 ptrOSPFAREA_DS area,
													 ptrOSPFLIST candidateList)
{
	UINT lowestMetric = OSPF_LS_INFINITY;
	ptrOSPFVERTEX candidateEntry;
	ptrOSPFVERTEX closestCandidate = NULL;
	void* pass = ospf_list_newIterator();

	// Get the vertex with the smallest metric from the candidate list...
	while ((candidateEntry = ospf_list_iterate_mem(candidateList, pass)) != NULL)
	{
		if (candidateEntry->distance < lowestMetric)
		{
			lowestMetric = candidateEntry->distance;

			// Keep track of closest vertex
			closestCandidate = candidateEntry;
		}
		
		else if (candidateEntry->distance == lowestMetric &&
				 closestCandidate &&
				 closestCandidate->vertexType == OSPFVERTEXTYPE_ROUTER &&
				 candidateEntry->vertexType == OSPFVERTEXTYPE_NETWORK)
		{
			// Network vertex get preference over router vertex
			// Keep track of closest vertex
			closestCandidate = candidateEntry;
		}
	}
	ospf_list_deleteIterator(pass);

	ptrOSPFVERTEX shortestPathListItem = closestCandidate;

	// Now insert it into the shortest path list...
	ospf_list_add_mem(area->shortestPathList, shortestPathListItem);

	// And remove it from the candidate list since no longer available.
	ospf_list_remove_mem(candidateList, closestCandidate, NULL);

	// Update my routing table to reflect the new shortest path list.
	ospf_spf_updateIntraAreaRoute(ospf, area, shortestPathListItem);

	return shortestPathListItem;
}

static bool ospf_spf_removeLSAFromShortestPathList(ptrOSPF_PDS ospf,
												   ptrOSPFLIST shortestPathList,
												   ptrOSPFLSAHDR lsa)
{
	ptrOSPFVERTEX vertex = NULL;
	void* pass = ospf_list_newIterator();
	while ((vertex = ospf_list_iterate_mem(shortestPathList, pass)) != NULL)
	{
		if (lsa == vertex->lsa)
		{
			if (vertex->nextHopList)
				ospf_list_delete_all(vertex->nextHopList);
			ospf_list_remove_mem(shortestPathList, lsa, pass);
			ospf_list_deleteIterator(pass);
			return true;
		}
	}
	ospf_list_deleteIterator(pass);
	return false;
}

static void ospf_spf_addStubRouteToshortestPath(ptrOSPF_PDS ospf,
												ptrOSPFAREA_DS thisArea)
{
	UINT i;
	ptrOSPFVERTEX tempV = NULL;
	UINT distance;

	void* pass = ospf_list_newIterator();
	while ((tempV = ospf_list_iterate_mem(thisArea->shortestPathList, pass)) != NULL)
	{
		// Examine router vertex only
		if (tempV->vertexType != OSPFVERTEXTYPE_ROUTER)
			continue;

		ptrOSPFLSAHDR lsaHdr = tempV->lsa;
		ptrOSPFRLSA rlsa = lsaHdr->lsaInfo;

		// Skip LSA if max age is reached and examine the next LSA.
		if (ospf_lsa_maskDoNotAge(ospf, lsaHdr->LSAge) >= ospf->LSAMaxAge)
			continue;

		for (i = 0; i < rlsa->linksCount; i++)
		{
			ptrOSPFRLSALINK link = rlsa->rlsaLink[i];

			// Examine stub network only
			if (link->type != OSPFLINKTYPE_STUB)
				continue;

			// Don't process if this indicates to my address
			if (ospf_isMyAddr(ospf->myId, link->linkId))
			{
				print_ospf_log(OSPF_SPF_LOG, "%s is my address. Not processing this link", link->linkId->str_ip);
				continue;
			}

			// Calculate distance from root.
			distance = link->metric + tempV->distance;

			ptrOSPFROUTINGTABLEROW rowPtr;
			// Get this route from routing table
			// handling for host route
			if (!IP_COMPARE(link->linkData, ANY_DEST))
				rowPtr = ospf_rtTable_getValidHostRoute(ospf,
														link->linkId,
														OSPFDESTTYPE_NETWORK);
			else
				rowPtr = ospf_rtTable_getValidRoute(ospf,
													link->linkId,
													OSPFDESTTYPE_NETWORK);

			OSPFROUTINGTABLEROW newRow;

			if (rowPtr)
			{
				// If the calculated distance is larger than previous,
				// examine next stub link

				if (distance >= rowPtr->metric)
					continue;

				ospf_rtTable_freeRoute(ospf, rowPtr);
			}

			// Add new route
			newRow.destType = OSPFDESTTYPE_NETWORK;
			newRow.destAddr = link->linkId;
			newRow.addrMask = link->linkData;
			newRow.option = 0;
			newRow.areaId = thisArea->areaId;
			newRow.pathType = OSPFPATHTYPE_INTRA_AREA;
			newRow.metric = distance;
			newRow.type2Metric = 0;
			newRow.LSOrigin = (void*)tempV->lsa;
			newRow.advertisingRouter = lsaHdr->AdvertisingRouter;

			// If stub network is part of the node's interfaces
			if (!OSPFID_COMPARE(tempV->vertexId, ospf->routerId))
			{
				NETSIM_ID intfId;

				// handling for host route
				if (link->linkData->int_ip[0] != 0xffffffff)
				{
					intfId = ospf_neighbor_getInterfaceIdforThisNeighbor(ospf,
																		 link->linkId);
				}
				else
				{
					intfId = ospf_getInterfaceIdForNextHop(ospf->myId,
														   link->linkId);
				}

				if (!intfId)
				{
					ptrOSPFLSAHDR LSHeader = NULL;

					LSHeader = ospf_lsdb_lookup_lsaList(thisArea->routerLSAList,
														link->linkId,
														link->linkId);

					if (LSHeader)
					{
						ospf_lsdb_removeLSA(ospf,
											thisArea,
											LSHeader);

						ospf_spf_removeLSAFromShortestPathList(ospf,
															   thisArea->shortestPathList,
															   LSHeader);

					}
					continue;
				}

				newRow.outInterface = DEVICE_NWADDRESS(ospf->myId, intfId);
				newRow.outInterfaceId = intfId;
				newRow.nextHop = NULL;
			}
			else
			{
				if (!tempV->nextHopList)
					continue;

				if (ospf_list_is_empty(tempV->nextHopList))
					continue;

				ptrOSPFNEXTHOPLISTITEM nextHopItem = ospf_list_get_headptr(tempV->nextHopList);
				newRow.nextHop = nextHopItem->nextHop;
				newRow.outInterface = DEVICE_NWADDRESS(ospf->myId, nextHopItem->outIntf);
				newRow.outInterfaceId = nextHopItem->outIntf;

			}

			ospf_rtTable_addRoute(ospf, &newRow);
		}
	}
	ospf_list_deleteIterator(pass);
}

static void ospf_spf_printCandidateList(ptrOSPF_PDS ospf,
										ptrOSPFLIST candidateList)
{
	print_ospf_log(OSPF_SPF_LOG, "Candidate list for router %d", ospf->myId);
	print_ospf_log(OSPF_SPF_LOG, "    size = %d", ospf_list_get_size(candidateList));

	ospf_spf_printVertexList(candidateList);
}

static void ospf_spf_findShrotestPathForThisArea(ptrOSPF_PDS ospf,
												 ptrOSPFAREA_DS area)
{
	ptrOSPFLIST candidateList = ospf_list_init(OSPF_SPF_FREE_VERTEX, OSPF_SPF_COPY_VERTEX);
	ptrOSPFVERTEX tempV = calloc(1, sizeof* tempV);

	area->shortestPathList = ospf_list_init(OSPF_SPF_FREE_VERTEX, OSPF_SPF_COPY_VERTEX);

	print_ospf_log(OSPF_SPF_LOG, "Calculating shortest path for area %s",
				   area->areaId->str_ip);

	// The shortest path starts with myself as the root.
	tempV->vertexId = ospf->routerId;
	tempV->vertexType = OSPFVERTEXTYPE_ROUTER;
	tempV->lsa = ospf_lsdb_lookup_lsaList(area->routerLSAList,
										  ospf->routerId,
										  ospf->routerId);

	if (!tempV->lsa ||
		ospf_lsa_hasMaxAge(ospf, tempV->lsa))
	{
		free(tempV);
		ospf_list_delete_all(candidateList);
		ospf_list_delete_all(area->shortestPathList);
		return;
	}
	print_ospf_log(OSPF_SPF_LOG, "Examine LSA of type %s, Id %s, seq num %u",
				   strLSTYPE[tempV->lsa->LSType],
				   tempV->lsa->LinkStateID->str_ip,
				   tempV->lsa->LSSequenceNumber - OSPF_INITIAL_SEQUENCE_NUMBER);

	tempV->nextHopList = ospf_list_init(NULL, NULL);

	// Insert myself (root) to the shortest path list.
	ospf_list_add_mem(area->shortestPathList, tempV);

	// Find candidates to be considered for the shortest path list.
	ospf_spf_updateCandidateList(ospf, area, candidateList, tempV);
	
	ospf_spf_printCandidateList(ospf, candidateList);

	// Keep calculating shortest path until the candidate list is empty.
	while (!ospf_list_is_empty(candidateList))
	{
		// Select the next best node in the candidate list into
		// the shortest path list.  That node is tempV.
		tempV = ospf_spf_updateShortestPathList(ospf, area, candidateList);

		ospf_spf_printShortestpath(ospf, area);

		// Find more candidates to be considered for the shortest path list.
		ospf_spf_updateCandidateList(ospf, area, candidateList, tempV);
		
		ospf_spf_printCandidateList(ospf, candidateList);
	}

	// Add stub routes to the shortest path list.
	ospf_spf_addStubRouteToshortestPath(ospf, area);

	ospf_spf_printShortestpath(ospf, area);

	ospf_list_delete_all(area->shortestPathList);
	ospf_list_delete_all(candidateList);
}

static void ospf_spf_findInterAreaPath(ptrOSPF_PDS ospf)
{
	fnNetSimError("Implement %s after area implementation -- 22645", __FUNCTION__);
#pragma message("Implement %s after area implementation -- 22645")
}

static void ospf_spf_findShortestPath(ptrOSPF_PDS ospf)
{
	ospf_spf_invalidateRoutingTable(ospf);

	UINT i;
	for (i = 0; i < ospf->areaCount; i++)
	{
		ptrOSPFAREA_DS area = ospf->areaDS[i];
		ospf_spf_findShrotestPathForThisArea(ospf, area);
	}

	// Calculate Inter Area routes
	if (ospf->isPartitionedIntoArea)
		ospf_spf_findInterAreaPath(ospf);


	if (ospf->isPartitionedIntoArea == TRUE &&
		ospf->routerType == OSPFRTYPE_ABR)
		ospf_area_handleABRTask(ospf);

	ospf_rtTable_freeAllInvalidRoute(ospf);

	// Update IP forwarding table using new shortest path.
	ospf_rtTable_updateIPTable(ospf);
}

void ospf_spf_handleCalculateSPFEvent()
{
	ptrOSPF_PDS ospf = OSPF_PDS_CURRENT();
	ospf->SPFTimer = OSPF_CURR_TIME();
	print_ospf_log(OSPF_SPF_LOG, "Calculating shortest path for router %d at time %0.3lf",
				   ospf->myId, ospf->SPFTimer / MILLISECOND);
	ospf_spf_findShortestPath(ospf);
	print_ospf_log(OSPF_SPF_LOG, "");
}
