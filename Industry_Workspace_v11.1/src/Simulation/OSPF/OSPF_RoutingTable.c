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
#include "../IP/IP.h"
#include "OSPF.h"
#include "OSPF_enum.h"
#include "OSPF_Msg.h"
#include "OSPF_Neighbor.h"
#include "OSPF_Interface.h"
#include "OSPF_RoutingTable.h"
#include "OSPF_List.h"

static bool ospf_rtTable_compareDestType(OSPFDESTTYPE destType1,
										 OSPFDESTTYPE destType2)
{
	if (destType1 == destType2)
		return true;

	else if (destType1 == OSPFDESTTYPE_ABR_ASBR &&
		(destType2 == OSPFDESTTYPE_ABR ||
		 destType2 == OSPFDESTTYPE_ASBR))
		return true;

	else if (destType2 == OSPFDESTTYPE_ABR_ASBR &&
		(destType1 == OSPFDESTTYPE_ABR ||
		 destType1 == OSPFDESTTYPE_ASBR))
		return true;

	return false;
}

static ptrOSPFROUTINGTABLEROW ospf_rtTable_getIntraAreaRoute(ptrOSPF_PDS ospf,
															 NETSIM_IPAddress destAddr,
															 OSPFDESTTYPE destType,
															 OSPFID areaId)
{
	UINT i;
	ptrOSPFROUTINGTABLE rtTable = ospf->routingTable;
	ptrOSPFROUTINGTABLEROW* rowPtr = rtTable->rows;
	ptrOSPFROUTINGTABLEROW longestMatchingEntry = NULL;

	for (i = 0; i < rtTable->numRow; i++)
	{
		if (ospf->isAdvertSelfIntf)
		{
			if (!IP_COMPARE(rowPtr[i]->destAddr, destAddr) &&
				ospf_rtTable_compareDestType(rowPtr[i]->destType, destType) &&
				rowPtr[i]->pathType == OSPFPATHTYPE_INTRA_AREA &&
				!OSPFID_COMPARE(rowPtr[i]->areaId, areaId))
			{
				if (!longestMatchingEntry ||
					rowPtr[i]->addrMask->int_ip[0] >
					longestMatchingEntry->addrMask->int_ip[0])
					longestMatchingEntry = rowPtr[i];
			}
		}
		else
		{
			if (((rowPtr[i]->destAddr->int_ip[0] & rowPtr[i]->addrMask->int_ip[0]) ==
				(destAddr->int_ip[0] & rowPtr[i]->addrMask->int_ip[0])) &&
				ospf_rtTable_compareDestType(rowPtr[i]->destType, destType) &&
				rowPtr[i]->pathType == OSPFPATHTYPE_INTRA_AREA &&
				!OSPFID_COMPARE(rowPtr[i]->areaId, areaId))
			{
				if (!longestMatchingEntry ||
					rowPtr[i]->addrMask->int_ip[0] >
					longestMatchingEntry->addrMask->int_ip[0])
					longestMatchingEntry = rowPtr[i];
			}
		}
	}
	return longestMatchingEntry;
}

static ptrOSPFROUTINGTABLEROW ospf_rtTable_getRoute(ptrOSPF_PDS ospf,
													NETSIM_IPAddress destAddr,
													OSPFDESTTYPE destType)
{
	UINT i;
	ptrOSPFROUTINGTABLE rtTable = ospf->routingTable;
	ptrOSPFROUTINGTABLEROW* rowPtr = rtTable->rows;
	ptrOSPFROUTINGTABLEROW longestMatchingEntry = NULL;

	for (i = 0; i < rtTable->numRow; i++)
	{
		if (ospf->isAdvertSelfIntf)
		{
			if (rowPtr[i]->destAddr->int_ip[0] == destAddr->int_ip[0] &&
				ospf_rtTable_compareDestType(rowPtr[i]->destType, destType))
				return rowPtr[i];
		}
		else
		{
			if ((rowPtr[i]->destAddr->int_ip[0] & rowPtr[i]->addrMask->int_ip[0]) ==
				(destAddr->int_ip[0] & rowPtr[i]->addrMask->int_ip[0]) &&
				ospf_rtTable_compareDestType(rowPtr[i]->destType, destType))
			{
				if (!longestMatchingEntry ||
					rowPtr[i]->addrMask->int_ip[0] > longestMatchingEntry->addrMask->int_ip[0])
				{
					longestMatchingEntry = rowPtr[i];
				}
			}
		}
	}
	return longestMatchingEntry;
}

static bool ospf_rtTable_isRouteMatch(ptrOSPFROUTINGTABLEROW newRoute,
									  ptrOSPFROUTINGTABLEROW oldRoute)
{
	if (newRoute->metric == oldRoute->metric &&
		!IP_COMPARE(newRoute->nextHop, oldRoute->nextHop))
		return true;
	else
		return false;
}

static void ospf_rtTable_addRowToTable(ptrOSPF_PDS ospf,
									   ptrOSPFROUTINGTABLE table,
									   ptrOSPFROUTINGTABLEROW row)
{
	if (table->numRow)
		table->rows = realloc(table->rows, (table->numRow + 1)*(sizeof* table->rows));
	else
		table->rows = calloc(1, sizeof* table->rows);
	table->rows[table->numRow] = calloc(1,sizeof* table->rows[0]);
	memcpy(table->rows[table->numRow], row, sizeof* row);
	table->numRow++;
}

void ospf_rtTable_addRoute(ptrOSPF_PDS ospf,
						   ptrOSPFROUTINGTABLEROW newRoute)
{
	ptrOSPFROUTINGTABLE rtTable = ospf->routingTable;
	ptrOSPFROUTINGTABLEROW rowPtr;

	// Get old route if any ..
	if (newRoute->destType != OSPFDESTTYPE_NETWORK)
	{
		rowPtr = ospf_rtTable_getIntraAreaRoute(ospf,
												newRoute->destAddr,
												newRoute->destType,
												newRoute->areaId);
	}
	else
	{
		rowPtr = ospf_rtTable_getRoute(ospf,
									   newRoute->destAddr,
									   newRoute->destType);
	}

	if (rowPtr)
	{
		if (ospf_rtTable_isRouteMatch(newRoute, rowPtr))
			newRoute->flag = OSPFROUTEFLAG_NO_CHANGE;
		else
			newRoute->flag = OSPFROUTEFLAG_CHANGED;

		memcpy(rowPtr, newRoute, sizeof* newRoute);
	}
	else
	{
		newRoute->flag = OSPFROUTEFLAG_NEW;
		ospf_rtTable_addRowToTable(ospf, rtTable, newRoute);
	}
}

ptrOSPFROUTINGTABLEROW ospf_rtTable_getValidHostRoute(ptrOSPF_PDS ospf,
													  NETSIM_IPAddress destAddr,
													  OSPFDESTTYPE destType)
{

	UINT i;
	ptrOSPFROUTINGTABLE rtTable = ospf->routingTable;
	ptrOSPFROUTINGTABLEROW rowPtr;

	for (i = 0; i < rtTable->numRow; i++)
	{
		rowPtr = rtTable->rows[i];
		if (rowPtr->destAddr->int_ip[0] == destAddr->int_ip[0] &&
			ospf_rtTable_compareDestType(rowPtr->destType, destType) &&
			rowPtr->flag != OSPFROUTEFLAG_INVALID)
			return rowPtr;
	}
	return NULL;
}

ptrOSPFROUTINGTABLEROW ospf_rtTable_getValidRoute(ptrOSPF_PDS ospf,
												  NETSIM_IPAddress destAddr,
												  OSPFDESTTYPE destType)
{

	UINT i;
	ptrOSPFROUTINGTABLE rtTable = ospf->routingTable;
	ptrOSPFROUTINGTABLEROW rowPtr;
	ptrOSPFROUTINGTABLEROW longestMatchingEntry = NULL;

	for (i = 0; i < rtTable->numRow; i++)
	{
		rowPtr = rtTable->rows[i];

		if (ospf->isAdvertSelfIntf)
		{
			if (rowPtr->destAddr->int_ip[0] == destAddr->int_ip[0] &&
				ospf_rtTable_compareDestType(rowPtr->destType, destType) &&
				rowPtr->flag != OSPFROUTEFLAG_INVALID)
				return rowPtr;
		}
		else
		{
			if ((rowPtr->destAddr->int_ip[0] & rowPtr->addrMask->int_ip[0]) ==
				(destAddr->int_ip[0] & rowPtr->addrMask->int_ip[0]) &&
				ospf_rtTable_compareDestType(rowPtr->destType, destType) &&
				rowPtr->flag != OSPFROUTEFLAG_INVALID)
			{
				if (!longestMatchingEntry ||
					rowPtr->addrMask->int_ip[0] > longestMatchingEntry->addrMask->int_ip[0])
					longestMatchingEntry = rowPtr;
			}
		}
	}
	return longestMatchingEntry;
}

void ospf_rtTable_freeRoute(ptrOSPF_PDS ospf,
							ptrOSPFROUTINGTABLEROW row)
{
	ptrOSPFROUTINGTABLE rtTable = ospf->routingTable;
	UINT i;
	bool isFound = false;
	for (i = 0; i < rtTable->numRow; i++)
	{
		if (rtTable->rows[i] == row)
		{
			isFound = true;
			rtTable->rows[i] = NULL;
		}

		if (isFound)
		{
			if (i != rtTable->numRow - 1)
				rtTable->rows[i] = rtTable->rows[i + 1];
			else
				rtTable->rows[i] = NULL;
		}
	}
	rtTable->numRow--;
}

void ospf_rtTable_freeAllInvalidRoute(ptrOSPF_PDS ospf)
{
	UINT i;
	for (i = 0; i < ospf->routingTable->numRow; i++)
	{
		ptrOSPFROUTINGTABLEROW row = ospf->routingTable->rows[i];
		if (row->flag == OSPFROUTEFLAG_INVALID)
		{
			ospf_rtTable_freeRoute(ospf, row);
			i--;
		}
	}
}

static void ospf_rtTable_deleteAllIPRoute(ptrOSPF_PDS ospf)
{
	UINT i;
	for (i = 0; i < ospf->ipTableCount; i++)
	{
		iptable_delete_by_route(IP_WRAPPER_GET(ospf->myId),
								ospf->ipTable[i]);
	}
	ospf->ipTableCount = 0;
	free(ospf->ipTable);
	ospf->ipTable = NULL;
}

static void ospf_rtTable_addIPRoute(ptrOSPF_PDS ospf,
									void* iproute)
{
	if (ospf->ipTableCount)
		ospf->ipTable = realloc(ospf->ipTable, (ospf->ipTableCount + 1) * sizeof* ospf->ipTable);
	else
		ospf->ipTable = calloc(1, sizeof* ospf->ipTable);
	ospf->ipTable[ospf->ipTableCount] = iproute;
	ospf->ipTableCount++;
}

void ospf_rtTable_updateIPTable(ptrOSPF_PDS ospf)
{
	UINT i;
	ospf_rtTable_deleteAllIPRoute(ospf);
	ptrOSPFROUTINGTABLE rtTable = ospf->routingTable;
	ptrOSPFROUTINGTABLEROW rowPtr;

	print_ospf_Dlog(form_dlogId("OSPFROUTE", ospf->myId),
					"Updating IP table for device %d at time %lf. Number of rows %d.",
					ospf->myId,
					OSPF_CURR_TIME(),
					rtTable->numRow);

	for (i = 0; i < rtTable->numRow; i++)
	{
		rowPtr = rtTable->rows[i];

		// Avoid updating IP forwarding table for route to self and router
		if (ospf_isMyAddr(ospf->myId, rowPtr->destAddr))
			continue;
		if (rowPtr->flag == OSPFROUTEFLAG_INVALID)
			continue;

		print_ospf_Dlog(form_dlogId("OSPFROUTE", ospf->myId),
						"%s,%s,%s,%s,%d,%d,",
						rowPtr->destAddr->str_ip,
						rowPtr->addrMask->str_ip,
						rowPtr->nextHop->str_ip,
						rowPtr->outInterface->str_ip,
						rowPtr->outInterfaceId,
						rowPtr->metric);

		void* iptable = iptable_add(IP_WRAPPER_GET(ospf->myId),
									rowPtr->destAddr,
									rowPtr->addrMask,
									0,
									rowPtr->nextHop,
									1,
									&rowPtr->outInterface,
									&rowPtr->outInterfaceId,
									rowPtr->metric,
									"OSPF");
		ospf_rtTable_addIPRoute(ospf, iptable);
	}
	print_ospf_Dlog(form_dlogId("OSPFROUTE", ospf->myId), "\n");
}
