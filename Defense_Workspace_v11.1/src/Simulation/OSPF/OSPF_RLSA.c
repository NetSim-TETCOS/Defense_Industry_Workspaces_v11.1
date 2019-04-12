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

void ospf_lsa_printRLSA(char* logid,
						ptrOSPFRLSA rlsa)
{
	if (!rlsa)
		return;
	print_ospf_Dlog(logid,
					"Link count = %d",
					rlsa->linksCount);
	UINT i;
	for (i = 0; i < rlsa->linksCount; i++)
	{
		ptrOSPFRLSALINK link = rlsa->rlsaLink[i];
		print_ospf_Dlog(logid,
						"\tLink Type = %s\n"
						"\tLink data = %s\n"
						"\tLink Id = %s\n"
						"\tMetric = %d\n",
						strOSPFLINKTYPE[link->type],
						link->linkData->str_ip,
						link->linkId->str_ip,
						link->metric);
	}
}

void OSPFLSAINFO_FREE_RLSA(ptrOSPFRLSA rlsa)
{
	UINT i;
	if (rlsa)
	{
		for (i = 0; i < rlsa->linksCount; i++)
			free(rlsa->rlsaLink[i]);
#pragma message(__LOC__"If TOS is implemented. Add free function for TOS")
		free(rlsa->rlsaLink);
		free(rlsa);
	}
}

ptrOSPFRLSA OSPFLSAINFO_COPY_RLSA(ptrOSPFRLSA rlsa)
{
	if (rlsa)
	{
		ptrOSPFRLSA r = calloc(1, sizeof* r);
		memcpy(r, rlsa, sizeof* r);
		UINT i;
		if (rlsa->linksCount)
		{
			r->rlsaLink = calloc(rlsa->linksCount, sizeof* r->rlsaLink);
			for (i = 0; i < r->linksCount; i++)
			{
				r->rlsaLink[i] = calloc(1, sizeof* r->rlsaLink[i]);
				memcpy(r->rlsaLink[i], rlsa->rlsaLink[i], sizeof* r->rlsaLink[i]);
#pragma message(__LOC__"If TOS is implemented. Add TOS copy")
			}
		}
		return r;
	}
	return NULL;
}

static void ospf_rlsa_setABR(ptrOSPFLSAHDR hdr, bool abr)
{
	ptrOSPFRLSA rlsa = ospf_list_get_headptr(hdr->lsaInfo);
	UCHAR* l = &rlsa->VEB;
	UCHAR x = (UCHAR)abr;
	x = x & maskChar(8, 8);

	*l = *l & (~(maskChar(8, 8)));

	*l = *l | LSHIFTCHAR(x, 8);
}

static void ospf_rlsa_add_link(ptrOSPFRLSA rlsa,
							   ptrOSPFRLSALINK link)
{
	if (rlsa->linksCount)
		rlsa->rlsaLink = realloc(rlsa->rlsaLink, (rlsa->linksCount + 1)*(sizeof* rlsa->rlsaLink));
	else
		rlsa->rlsaLink = calloc(1, sizeof* rlsa->rlsaLink);
	rlsa->rlsaLink[rlsa->linksCount] = link;
	rlsa->linksCount++;
}

static ptrOSPFRLSALINK ospf_rlsa_add_type1link(NETSIM_ID d,
											   NETSIM_ID in,
											   ptrOSPF_IF ospf,
											   ptrOSPF_NEIGHBOR neigh)
{
	ptrOSPFRLSALINK link = calloc(1, sizeof* link);
	link->type = OSPFLINKTYPE_POINT_TO_POINT;
	link->linkId = neigh->neighborId;
	link->linkData = DEVICE_NWADDRESS(d, in);
	link->metric = (UINT16)ospf->interfaceOutputCost;
	return link;
}

static ptrOSPFRLSALINK ospf_rlsa_add_type2link(NETSIM_ID d,
											   NETSIM_ID in,
											   ptrOSPF_IF thisInterface)
{
	ptrOSPFRLSALINK link = calloc(1, sizeof* link);
	link->type = OSPFLINKTYPE_TRANSIT;
	link->linkId = thisInterface->designaterRouter;
	link->linkData = DEVICE_SUBNETMASK(d, in);
	link->metric = (UINT16)thisInterface->interfaceOutputCost;
	return link;
}

static ptrOSPFRLSALINK ospf_rlsa_add_type3link(NETSIM_ID d,
											   NETSIM_ID in,
											   ptrOSPF_IF thisInterface)
{
	ptrOSPFRLSALINK link = calloc(1, sizeof* link);
	link->type = OSPFLINKTYPE_STUB;
	switch (thisInterface->Type)
	{
		case OSPFIFTYPE_NBMA:
		case OSPFIFTYPE_BROADCAST:
		case OSPFIFTYPE_P2P:
			link->linkId = DEVICE_NWADDRESS(d, in);
			link->linkData = DEVICE_SUBNETMASK(d, in);
			link->metric = (UINT16)thisInterface->interfaceOutputCost;
			break;
		case OSPFIFTYPE_P2MP:
			link->linkId = DEVICE_NWADDRESS(d, in);
			link->linkData = GET_BROADCAST_IP(4);
			link->metric = 0;
			break;
		default:
			fnNetSimError("Unknown if type %d\n", thisInterface->Type);
			break;
	}
	return link;
}

static ptrOSPFRLSALINK ospf_rlsa_add_selflink(ptrOSPF_IF thisInterface)
{
	ptrOSPFRLSALINK link = calloc(1, sizeof* link);
	link->type = OSPFLINKTYPE_STUB;
	link->linkId = thisInterface->IPIfAddr;
	link->linkData = GET_BROADCAST_IP(4);
	link->metric = 0;
	return link;
}

static ptrOSPFRLSALINK ospf_rlsa_add_hostRoute(NETSIM_ID d,
											   NETSIM_ID in,
											   ptrOSPF_IF thisInterface,
											   ptrOSPF_NEIGHBOR neigh)
{
	ptrOSPFRLSALINK link = calloc(1, sizeof* link);
	link->type = OSPFLINKTYPE_STUB;
	link->linkId = neigh->neighborIPAddr;
	link->linkData = GET_BROADCAST_IP(4);
	link->metric = (UINT16)thisInterface->interfaceOutputCost;
	return link;
}

static UINT16 ospf_rlsa_add_foreach_interface(ptrOSPF_PDS ospf,
											ptrOSPFAREA_DS area,
											ptrOSPFLSAHDR lsa)
{
	NETSIM_ID d = pstruEventDetails->nDeviceId;
	ptrOSPFRLSALINK link;
	ptrOSPFRLSA rlsa = calloc(1, sizeof* rlsa);
	UINT16 size = OSPFRLSA_LEN_FIXED;
	UINT i;
	for (i = 0; i < ospf->ifCount; i++)
	{
		ptrOSPF_IF ospfIf = ospf->ospfIf[i];
		if (ospfIf->State == OSPFIFSTATE_DOWN)
			continue;
		if (OSPFID_COMPARE(ospfIf->areaId, area->areaId))
			continue;

		switch (ospfIf->Type)
		{
			case OSPFIFTYPE_BROADCAST:
			{
				if (ospfIf->State == OSPFIFSTATE_WAITING)
				{
					link = ospf_rlsa_add_type3link(d,
												   ospfIf->id,
												   ospfIf);
					ospf_rlsa_add_link(rlsa, link);
					size += OSPFRLSALINK_LEN_FIXED;
				}
				else if (((ospfIf->State != OSPFIFSTATE_DR) &&
						  ospf_is_router_fullAdjacentWithDR(ospfIf)) ||
						  (ospfIf->State == OSPFIFSTATE_DR &&
						   ospf_is_dr_router_fulladjacentwithAnother(ospfIf)))
				{
					link = ospf_rlsa_add_type2link(d,
												   ospfIf->id,
												   ospfIf);
					ospf_rlsa_add_link(rlsa, link);
					size += OSPFRLSALINK_LEN_FIXED;
				}
				else
				{
					link = ospf_rlsa_add_type3link(d,
												   ospfIf->id,
												   ospfIf);
					ospf_rlsa_add_link(rlsa, link);
					size += OSPFRLSALINK_LEN_FIXED;
				}
			}
			break;
			case OSPFIFTYPE_P2MP:
			{
				link = ospf_rlsa_add_selflink(ospfIf);
				ospf_rlsa_add_link(rlsa, link);
				size += OSPFRLSALINK_LEN_FIXED;

				UINT n;
				for (n = 0; n < ospfIf->neighborRouterCount; n++)
				{
					ptrOSPF_NEIGHBOR neigh = ospfIf->neighborRouterList[n];
					if (neigh->state == OSPFNEIGHSTATE_Full)
					{
						link = ospf_rlsa_add_type1link(d,
													   ospfIf->id,
													   ospfIf,
													   neigh);
						ospf_rlsa_add_link(rlsa, link);
						size += OSPFRLSALINK_LEN_FIXED;
					}
				}
			}
			break;
			case OSPFIFTYPE_P2P:
			{
				NETSIM_ID in = ospfIf->id;
				if (!ospfIf->neighborRouterCount &&
					ospfIf->State == OSPFIFSTATE_P2P)
				{
					if (DEVICE_INTERFACE(d, in)->nInterfaceType != INTERFACE_VIRTUAL &&
						DEVICE_MACLAYER(d, in)->nMacProtocolId == MAC_PROTOCOL_P2P)
					{
						link = ospf_rlsa_add_type3link(d,
													   in,
													   ospfIf);
						ospf_rlsa_add_link(rlsa, link);
						size += OSPFRLSALINK_LEN_FIXED;
					}
				}
				UINT n;
				for (n = 0; n < ospfIf->neighborRouterCount; n++)
				{
					ptrOSPF_NEIGHBOR neigh = ospfIf->neighborRouterList[n];
					if (neigh->state == OSPFNEIGHSTATE_Full)/* ||
						neigh->state == OSPFNEIGHSTATE_Loading)
#pragma message(__LOC__"Check above again --- 15540")*/
					{
						link = ospf_rlsa_add_type1link(d,
													   in,
													   ospfIf,
													   neigh);
						ospf_rlsa_add_link(rlsa, link);
						size += OSPFRLSALINK_LEN_FIXED;
					}
					if (ospfIf->State == OSPFIFSTATE_P2P)
					{
						if (DEVICE_INTERFACE(d, in)->nInterfaceType != INTERFACE_VIRTUAL &&
							DEVICE_MACLAYER(d, in)->nMacProtocolId == MAC_PROTOCOL_P2P)
						{
							link = ospf_rlsa_add_type3link(d,
														   in,
														   ospfIf);
							ospf_rlsa_add_link(rlsa, link);
							size += OSPFRLSALINK_LEN_FIXED;
						}
						else
						{
							link = ospf_rlsa_add_hostRoute(d,
														   in,
														   ospfIf,
														   neigh);
							ospf_rlsa_add_link(rlsa, link);
							size += OSPFRLSALINK_LEN_FIXED;
						}
					}
				}
			}
			break;
			default:
				fnNetSimError("Unknown if type %d for interface %d, router %s\n",
							  ospfIf->Type,
							  ospfIf->id,
							  ospf->routerId->str_ip);
				break;
		}
	}
	ospf_lsahdr_add_lsa(lsa, rlsa, size);
	return size;
}

bool ospf_rlsa_isBodyChanged(ptrOSPFLSAHDR newLSA,
							 ptrOSPFLSAHDR oldLSA)
{
	UINT16 i;
	UINT16 j;
	bool* sameLinkInfo = NULL;

	ptrOSPFRLSA newRLSA = newLSA->lsaInfo;
	ptrOSPFRLSA oldRLSA = oldLSA->lsaInfo;

	if (newRLSA->linksCount != oldRLSA->linksCount ||
		newRLSA->VEB != oldRLSA->VEB)
		return true;

	ptrOSPFRLSALINK* newLink = newRLSA->rlsaLink;
	ptrOSPFRLSALINK* oldLink = oldRLSA->rlsaLink;

	UINT16 size = newRLSA->linksCount;

	sameLinkInfo = (bool*)calloc(size, sizeof* sameLinkInfo);

	for (i = 0; i < size; i++)
	{
		for (j = 0; j < size; j++)
		{
			if (!sameLinkInfo[j])
			{
				if (newLink[i]->linkId == oldLink[j]->linkId &&
					newLink[i]->type == oldLink[j]->type &&
					!OSPFID_COMPARE(newLink[i]->linkData, oldLink[j]->linkData) &&
					newLink[i]->metric == oldLink[j]->metric)
				{
					sameLinkInfo[j] = true;
					break;
				}
			}
		}
	}

	for (j = 0; j < size; j++)
	{
		if (!sameLinkInfo[j])
		{
			free(sameLinkInfo);
			return true;
		}
	}

	free(sameLinkInfo);
	return false;
}

bool ospf_rlsa_hasLink(ptrOSPF_PDS ospf,
					   ptrOSPFLSAHDR wlsa,
					   ptrOSPFLSAHDR vlsa)
{
	ptrOSPFRLSA rlsa = wlsa->lsaInfo;

	UINT i;
	for (i = 0; i < rlsa->linksCount; i++)
	{
		ptrOSPFRLSALINK link = rlsa->rlsaLink[i];
		if (link->type == OSPFLINKTYPE_POINT_TO_POINT)
		{
			if (vlsa->LSType == LSTYPE_ROUTERLSA &&
				!OSPFID_COMPARE(link->linkId, vlsa->AdvertisingRouter))
				return true;
		}
		else if (link->type == OSPFLINKTYPE_TRANSIT)
		{
			if (vlsa->LSType == LSTYPE_NETWORKLSA &&
				!OSPFID_COMPARE(link->linkId, vlsa->LinkStateID))
				return true;
		}
	}
	return false;
}

bool Ospf_rlsa_getASBRouter(UINT8 VEB)
{
	UINT8 asbr = VEB;

	//clears all the bits except seventh bit
	asbr = asbr & maskChar(7, 7);

	//Right shifts so that last bit represents ASBoundaryRouter
	asbr = RSHIFTCHAR(asbr, 7);

	return (bool)asbr;
}

bool Ospf_rlsa_getABRouter(UINT8 VEB)
{
	UINT8 abr = VEB;

	//clears all the bits except eighth bit
	abr = abr & maskChar(8, 8);

	//Right shifts so that last bit represents areaBorderRouter
	abr = RSHIFTCHAR(abr, 8);

	return (bool)abr;
}

void ospf_rlsa_originateRouterLSA(ptrOSPFAREA_DS area,
								  bool isFlush)
{
	UINT16 rlsaSize;
	ptrOSPF_PDS ospf = OSPF_PDS_CURRENT();
	ptrOSPF_IF thisInterface = OSPF_IF_CURRENT();

	ptrOSPFLSAHDR lsa = calloc(1, sizeof* lsa);
	ptrOSPFLSAHDR old = ospf_lsa_find_old_lsa(area->routerLSAList, ospf->routerId, ospf->routerId);

	rlsaSize = ospf_rlsa_add_foreach_interface(ospf, area, lsa);

	if (!ospf_lsa_update_lsahdr(ospf, area, lsa, old, LSTYPE_ROUTERLSA))
	{
		OSPF_LSA_MSG_FREE(lsa);
		ospf_lsa_schedule_routerLSA(ospf, area, false);
		return;
	}
	if (isFlush)
		lsa->LSAge = ospf->LSAMaxAge;

	lsa->LinkStateID = ospf->routerId;
	lsa->AdvertisingRouter = ospf->routerId;
	lsa->length = OSPFLSAHDR_LEN + rlsaSize;

	if (ospf->routerType == OSPFRTYPE_ABR)
		ospf_rlsa_setABR(lsa, true);

	area->lastLSAOriginateTime = pstruEventDetails->dEventTime;
	area->isRouterLSTimer = false;

	ospf_lsa_print(form_dlogId("RLSALOG", ospf->myId), lsa, "Originate RLSA");

	ospf_lsa_flood(ospf,
				   area->areaId,
				   lsa,
				   DEVICE_MYIP(),
				   pstruEventDetails->nInterfaceId);

	if (ospf_lsdb_install(ospf, area->areaId, lsa, area->routerLSAList))
		ospf_spf_scheduleCalculation(ospf);

	OSPF_LSA_MSG_FREE(lsa);
}

