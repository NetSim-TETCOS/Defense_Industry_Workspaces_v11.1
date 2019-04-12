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

typedef struct stru_lsreq_item
{
	UINT seqNum;
	double time;
	ptrOSPFLSAHDR lsHdr;
}LSREQITEM, *ptrLSREQITEM;

void OSPF_LSREQ_MSG_NEW(ptrOSPFPACKETHDR hdr)
{
	ptrOSPFLSREQ lsr = calloc(1, sizeof* lsr);
	OSPF_HDR_SET_MSG(hdr,
					 OSPFMSG_LSREQUEST,
					 lsr,
					 0);
}

ptrOSPFLSREQ OSPF_LSREQ_MSG_COPY(ptrOSPFLSREQ lsr)
{
	ptrOSPFLSREQ l;
	l = calloc(1, sizeof* l);
	l->count = lsr->count;
	l->lsrObj = calloc(l->count, sizeof* l->lsrObj);
	UINT i;
	for (i = 0; i < l->count; i++)
	{
		l->lsrObj[i] = calloc(1, sizeof* l->lsrObj[i]);
		memcpy(l->lsrObj[i], lsr->lsrObj[i], sizeof* l->lsrObj[i]);
	}
	return l;
}

void OSPF_LSREQ_MSG_FREE(ptrOSPFLSREQ lsr)
{
	UINT i;
	for (i = 0; i < lsr->count; i++)
		free(lsr->lsrObj[i]);
	free(lsr->lsrObj);
	free(lsr);
}

static void ospf_lsreq_freeItem(ptrLSREQITEM item)
{
	OSPF_LSA_HDR_FREE(item->lsHdr);
	free(item);
}

ptrOSPFLIST ospf_lsreq_initList()
{
	return ospf_list_init(ospf_lsreq_freeItem, NULL);
}

void ospf_lsreq_insertToList(ptrOSPF_NEIGHBOR neigh,
							 ptrOSPFLSAHDR lsHdr,
							 double time)
{
	if (ospf_lsReq_searchFromList(neigh, lsHdr))
	{
		return;
	}

	ptrLSREQITEM item = calloc(1, sizeof* item);
	item->seqNum = neigh->LSReqTimerSequenceNumber;
	item->lsHdr = OSPF_LSA_HDR_COPY(lsHdr);
	item->time = time;
	ospf_list_add_mem(neigh->neighLSReqList, item);
}

void ospf_lsreq_removeFromReqList(ptrOSPFLIST list,
								  ptrOSPFLSAHDR lsa)
{
	ptrLSREQITEM req;
	ptrOSPFLSAHDR l;
	void* pass = ospf_list_newIterator();
	while ((req = ospf_list_iterate_mem(list, pass)) != NULL)
	{
		l = req->lsHdr;
		if (l->LSType == lsa->LSType &&
			!OSPFID_COMPARE(l->AdvertisingRouter, lsa->AdvertisingRouter) &&
			!OSPFID_COMPARE(l->LinkStateID, lsa->LinkStateID))
		{
			ospf_list_delete_mem(list, req, pass);
			break;
		}
	}
	ospf_list_deleteIterator(pass);
}

ptrOSPFLSAHDR ospf_lsReq_searchFromList(ptrOSPF_NEIGHBOR neigh,
										ptrOSPFLSAHDR lsa)
{
	ptrOSPFLIST list = neigh->neighLSReqList;
	void* pass = ospf_list_newIterator();
	ptrOSPFLSAHDR hdr = NULL;
	ptrLSREQITEM item;
	while ((item = ospf_list_iterate_mem(list, pass)) != NULL)
	{
		hdr = item->lsHdr;

		if (hdr->LSType == lsa->LSType &&
			!OSPFID_COMPARE(hdr->LinkStateID, lsa->LinkStateID) &&
			!OSPFID_COMPARE(hdr->AdvertisingRouter, lsa->AdvertisingRouter))
		{
			ospf_list_deleteIterator(pass);
			return hdr;
		}
	}
	ospf_list_deleteIterator(pass);
	return NULL;
}

bool ospf_lsreq_isRequestedLSAReceived(ptrOSPF_NEIGHBOR neigh)
{
	ptrOSPFLSAHDR lsa = ospf_list_get_headptr(neigh->neighLSReqList);
	if (lsa->LSSequenceNumber == neigh->LSReqTimerSequenceNumber)
		return false;
	return true;
}

static bool ospf_lsreq_createLSReqObject(ptrOSPF_PDS ospf,
										 ptrOSPF_NEIGHBOR neigh,
										 ptrOSPFLSREQOBJ lsr,
										 bool retx)
{
	ptrLSREQITEM item;
	ptrOSPFLSAHDR lsHdr = NULL;
	void* pass = ospf_list_newIterator();
	if (retx)
	{
		while ((item = ospf_list_iterate_mem(neigh->neighLSReqList, pass)) != NULL)
		{
			if (item->seqNum == neigh->LSReqTimerSequenceNumber)
			{
				item->seqNum = neigh->LSReqTimerSequenceNumber + 1;
				lsHdr = item->lsHdr;
				break;
			}
		}
	}
	else
	{
		while ((item = ospf_list_iterate_mem(neigh->neighLSReqList, pass)) != NULL)
		{
			if (item->seqNum == neigh->LSReqTimerSequenceNumber)
			{
				item->seqNum = neigh->LSReqTimerSequenceNumber + 1;
				lsHdr = item->lsHdr;
				break;
			}
			else if (item->seqNum == 0)
			{
				item->seqNum = neigh->LSReqTimerSequenceNumber + 1;
				lsHdr = item->lsHdr;
				break;
			}
		}
	}
	ospf_list_deleteIterator(pass);
	if (!lsHdr)
		return false;

	lsr->LSType = lsHdr->LSType;
	lsr->LinkStateId = lsHdr->LinkStateID;
	lsr->AdvertisingRouter = lsHdr->AdvertisingRouter;
	return true;
}

void ospf_lsreq_send(ptrOSPF_PDS ospf,
					 NETSIM_ID interfaceId,
					 NETSIM_IPAddress nbrAddr,
					 bool retx)
{
	UINT16 maxLSReqObject = INTERFACE_MTU_DEFAULT - IP_HDR_LEN;
	maxLSReqObject = (maxLSReqObject - OSPFPACKETHDR_LEN) / OSPFLSREQ_LEN_SINGLE;
	UINT16 numLSReqObject = 0;
	ptrOSPF_IF thisInterface = OSPF_IF_GET(ospf, interfaceId);
	ptrOSPF_NEIGHBOR neigh = OSPF_NEIGHBOR_FIND_BY_IP(thisInterface, nbrAddr);

	if (!neigh)
		fnNetSimError("Neighbor %s is nor found in neighbor list. %s must not be called.",
					  nbrAddr->str_ip, __FUNCTION__);

	ptrOSPFLSREQOBJ* lsReqObj = NULL;
	OSPFLSREQOBJ reqObj;
	// Prepare LS Request packet payload
	while (numLSReqObject < maxLSReqObject &&
		   ospf_lsreq_createLSReqObject(ospf,
										neigh,
										&reqObj,
										retx))
	{
		if (numLSReqObject)
			lsReqObj = realloc(lsReqObj, (numLSReqObject + 1) * sizeof* lsReqObj);
		else
			lsReqObj = calloc(1, sizeof* lsReqObj);
		lsReqObj[numLSReqObject] = calloc(1, sizeof* lsReqObj[0]);
		memcpy(lsReqObj[numLSReqObject], &reqObj, sizeof reqObj);
		numLSReqObject++;
	}

	if (!numLSReqObject)
		print_ospf_log(OSPF_LOG, "Sending empty LSREQ msg");

	NetSim_PACKET* packet = OSPF_PACKET_NEW(OSPF_CURR_TIME(),
											OSPFMSG_LSREQUEST,
											ospf->myId,
											interfaceId);
	ptrOSPFPACKETHDR hdr = OSPF_PACKET_GET_HDR(packet);
	ptrOSPFLSREQ lsReq = OSPF_HDR_GET_MSG(hdr);
	OSPF_HDR_INCREASE_LEN(packet, numLSReqObject*OSPFLSREQ_LEN_SINGLE);
	lsReq->count = numLSReqObject;
	lsReq->lsrObj = lsReqObj;

	NETSIM_IPAddress dstAddr;
	NETSIM_ID dstId;
	NETSIM_IPAddress nxtHop;
	if (thisInterface->Type == OSPFIFSTATE_P2P)
	{
		dstAddr = AllSPFRouters;
		dstId = 0;
		nxtHop = ANY_DEST;
	}
	else
	{
		dstAddr = nbrAddr;
		dstId = neigh->devId;
		nxtHop = nbrAddr;
	}
	add_dest_to_packet(packet, dstId);
	packet->pstruNetworkData->szDestIP = dstAddr;
	packet->pstruNetworkData->szNextHopIp = nxtHop;
	packet->pstruNetworkData->nTTL = 2;

	//Set the retransmission timer
	ptrLSRXTTIMERDETAILS rxtInfo = calloc(1, sizeof* rxtInfo);
	rxtInfo->neighborIP = nbrAddr;
	rxtInfo->msgType = OSPFMSG_LSREQUEST;
	rxtInfo->rxmtSeqNum = ++neigh->LSReqTimerSequenceNumber;
	ospf_event_add(OSPF_CURR_TIME() + thisInterface->RxmtInterval,
				   ospf->myId,
				   interfaceId,
				   OSPF_RXMTTIMER,
				   NULL,
				   rxtInfo);
	OSPF_SEND_PACKET(packet);
}

void ospf_handle_LSRequest()
{
	NETSIM_ID d = pstruEventDetails->nDeviceId;
	NETSIM_ID in = pstruEventDetails->nInterfaceId;
	NetSim_PACKET* packet = pstruEventDetails->pPacket;
	NETSIM_IPAddress srcAddr = packet->pstruNetworkData->szSourceIP;
	ptrOSPF_PDS ospf = OSPF_PDS_CURRENT();
	ptrOSPF_IF thisInterface = OSPF_IF_CURRENT();
	ptrOSPFAREA_DS thisArea = OSPF_AREA_GET_ID(ospf, thisInterface->areaId);
	ptrOSPF_NEIGHBOR neigh = OSPF_NEIGHBOR_FIND_BY_IP(thisInterface, srcAddr);
	
	if (!neigh)
	{
		print_ospf_log(OSPF_LOG, "Router %d receive LS request from unknown"
					   " neighbor %s. Discarding LS request.",
					   d, srcAddr->str_ip);
		goto DISCARD_LSREQ;
	}

	if (neigh->state <OSPFNEIGHSTATE_Exchange)
	{
		print_ospf_log(OSPF_LOG, "Router %d receive LS request from"
					   " neighbor %s whose state is below exchange. Discarding LS request.",
					   d, srcAddr->str_ip);
		goto DISCARD_LSREQ;
	}

	UINT numLSReqObject;
	ptrOSPFPACKETHDR hdr = OSPF_PACKET_GET_HDR(packet);
	ptrOSPFLSREQ reqPkt = OSPF_HDR_GET_MSG(hdr);
	numLSReqObject = reqPkt->count;
	ptrOSPFLSREQOBJ* lsReqObjA = reqPkt->lsrObj;

	UINT i;
	for (i = 0; i < numLSReqObject; i++)
	{
		ptrOSPFLSAHDR LSHeader;
		ptrOSPFLSREQOBJ LSReqObj = lsReqObjA[i];

		// Stop processing packet if requested LSA type is not identified
		if (LSReqObj->LSType < LSTYPE_ROUTERLSA ||
			 LSReqObj->LSType >= LSTYPE_UNDEFINED)
		{
			print_ospf_log(OSPF_LOG, "Receive bad LS Request from"
						   " neighbor (%s). Discard LS Request packet", srcAddr->str_ip);
			ospf_event_set_and_call(OSPF_BadLSReq, srcAddr);
			goto DISCARD_LSREQ;
		}

		// Find LSA in my own LSDB
		LSHeader = ospf_lsdb_lookup(ospf,
									thisArea,
									LSReqObj->LSType,
									LSReqObj->AdvertisingRouter,
									LSReqObj->LinkStateId);

		// Stop processing packet if LSA is not found in my database
		if (!LSHeader)
		{
			print_ospf_log(OSPF_LOG, "Receive bad LS Request from"
						   " neighbor (%s). LSA not in LSDB."
						   " Discard LS Request packet",
						   srcAddr->str_ip);

			// Handle neighbor event : Bad LS Request
			ospf_event_set_and_call(OSPF_BadLSReq, srcAddr);
			goto DISCARD_LSREQ;
		}
		
		ospf_lsa_queueToFlood(ospf,
							  thisInterface,
							  LSHeader);

		if (ospf->isSendDelayedUpdate)
		{
			if (!thisInterface->isFloodTimerSet)
			{
				double t = pstruEventDetails->dEventTime;
				t = max(t, thisInterface->lastFloodTimer + ospf->minLSInterval);
				t += ospf->dFloodTimer*NETSIM_RAND_01();
				ospf_event_add(t,
							   ospf->myId,
							   thisInterface->id,
							   OSPF_FLOODTIMER,
							   NULL,
							   NULL);
				thisInterface->lastFloodTimer = t;
				thisInterface->isFloodTimerSet = true;
			}
		}
	}

	

	if (!ospf->isSendDelayedUpdate)
	{
		ospf_lsu_sendLSUpdateToNeighbor(ospf,
										thisInterface);
	}

DISCARD_LSREQ:
	fn_NetSim_Packet_FreePacket(packet);
	pstruEventDetails->pPacket = NULL;
}

void ospf_lsreq_retransmit(ptrOSPF_PDS ospf,
						   ptrOSPF_IF thisInterface,
						   ptrOSPF_NEIGHBOR neigh,
						   UINT seqNum)
{
	ptrLSREQITEM lsreqItem = NULL;
	void* pass = ospf_list_newIterator();
	while ((lsreqItem = ospf_list_iterate_mem(neigh->neighLSReqList, pass)) != NULL)
	{
		if (lsreqItem->seqNum == seqNum)
		{
			ospf_lsreq_send(ospf,
							thisInterface->id,
							neigh->neighborIPAddr,
							true);
			break;
		}
	}
	ospf_list_deleteIterator(pass);
}
