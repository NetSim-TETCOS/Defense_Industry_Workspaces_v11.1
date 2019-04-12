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
#include "OSPF_Typedef.h"
#include "OSPF.h"
#include "OSPF_List.h"
#include "OSPF_Interface.h"
#include "OSPF_Msg.h"

ptrOSPFAREA_DS OSPF_AREA_GET(ptrOSPF_PDS ospf,
							 NETSIM_ID in,
							 OSPFID areaId,
							 NETSIM_IPAddress interfaceIP)
{
	UINT i;
	for (i = 0; i < ospf->areaCount; i++)
	{
		if (areaId && !OSPFID_COMPARE(ospf->areaDS[i]->areaId, areaId))
			return ospf->areaDS[i];
		UINT j;
		for (j = 0; j < ospf->areaDS[i]->assocInterfaceCount; j++)
		{
			if (in && ospf->areaDS[i]->assocRouterInterfaceId[j] == in)
				return ospf->areaDS[i];
			if (interfaceIP && !IP_COMPARE(ospf->areaDS[i]->assocRouterInterface[j], interfaceIP))
				return ospf->areaDS[i];
		}
	}
	return NULL;
}

void OSPF_AREA_SET(ptrOSPF_PDS ospf,
				   ptrOSPFAREA_DS area)
{
	if (ospf->areaCount)
		ospf->areaDS = realloc(ospf->areaDS, (ospf->areaCount + 1)*(sizeof* ospf->areaDS));
	else
		ospf->areaDS = calloc(1, sizeof* ospf->areaDS);
	ospf->areaDS[ospf->areaCount] = area;
	ospf->areaCount++;
}

static void ADDR_RN_FREE(ptrADDR_RN rn)
{
	free(rn);
}

static void ospf_area_addressRange_init(NETSIM_ID d,
										ptrOSPFAREA_DS area)
{
	area->addressRangeList = ospf_list_init(ADDR_RN_FREE,NULL);
	NETSIM_ID i;
	for (i = 0; i < DEVICE(d)->nNumOfInterface; i++)
	{
		ptrADDR_RN rn = calloc(1, sizeof* rn);
		NetSIm_DEVICEINTERFACE* inter = DEVICE_INTERFACE(d, i + 1);
		rn->address = IP_NETWORK_ADDRESS(inter->szAddress,
										 inter->szSubnetMask,
										 inter->prefix_len);
		rn->mask = inter->szSubnetMask;
		rn->status = ADDRRNSTATUS_ADVERTISE;
		ospf_list_add_mem(area->addressRangeList, rn);
	}
}

static void add_interface_to_area(ptrOSPFAREA_DS area,
								  NETSIM_ID d,
								  NETSIM_ID in)
{
	if (area->assocInterfaceCount)
	{
		area->assocRouterInterface = realloc(area->assocRouterInterface,
			(area->assocInterfaceCount + 1)*
											 (sizeof* area->assocRouterInterface));

		area->assocRouterInterfaceId = realloc(area->assocRouterInterfaceId,
			(area->assocInterfaceCount + 1)*
											 (sizeof* area->assocRouterInterfaceId));
	}
	else
	{
		area->assocRouterInterface = calloc(1, sizeof* area->assocRouterInterface);
		area->assocRouterInterfaceId = calloc(1, sizeof* area->assocRouterInterfaceId);
	}
	area->assocRouterInterface[area->assocInterfaceCount] = DEVICE_NWADDRESS(d, in);
	area->assocRouterInterfaceId[area->assocInterfaceCount] = in;
	area->assocInterfaceCount++;
}

void ospf_area_init(NETSIM_ID d, NETSIM_ID in)
{
	ptrOSPF_PDS pds = OSPF_PDS_GET(d);
	ptrOSPF_IF ospf = OSPF_IF_GET(pds, in);
	ptrOSPFAREA_DS area = OSPF_AREA_GET_ID(pds, ospf->areaId);
	if (area)
	{
		add_interface_to_area(area, d, in);
		return;
	}

	area = calloc(1, sizeof* area);
	OSPF_AREA_SET(pds, area);
	area->areaId = ospf->areaId;
	add_interface_to_area(area, d, in);
	ospf_area_addressRange_init(d, area);
	area->extRoutingCapability = ospf->extRoutingCapability;

	area->nwLSAList = ospf_list_init(OSPF_LSA_MSG_FREE, OSPF_LSA_MSG_COPY);
	area->routerLSAList = ospf_list_init(OSPF_LSA_MSG_FREE, OSPF_LSA_MSG_COPY);
	area->routerSummaryLSAList = ospf_list_init(OSPF_LSA_MSG_FREE, OSPF_LSA_MSG_COPY);
	area->nwSummaryLSAList = ospf_list_init(OSPF_LSA_MSG_FREE, OSPF_LSA_MSG_COPY);
	area->maxAgeList = ospf_list_init(OSPF_LSA_MSG_FREE, OSPF_LSA_MSG_COPY);
}

void ospf_area_handleABRTask(ptrOSPF_PDS ospf)
{
	fnNetSimError("Implement %s after area implementation", __FUNCTION__);
}
