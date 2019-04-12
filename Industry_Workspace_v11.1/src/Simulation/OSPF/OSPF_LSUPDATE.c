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

void OSPF_LSUPDATE_MSG_NEW(ptrOSPFPACKETHDR hdr)
{
	ptrOSPFLSUPDATE lsu = calloc(1, sizeof* lsu);
	OSPF_HDR_SET_MSG(hdr,
					 OSPFMSG_LSUPDATE,
					 lsu,
					 OSPFLSUPDATE_LEN_FIXED);
}

ptrOSPFLSUPDATE OSPF_LSUPDATE_MSG_COPY(ptrOSPFLSUPDATE lsu)
{
	ptrOSPFLSUPDATE u = calloc(1, sizeof* lsu);
	memcpy(u, lsu, sizeof* u);
	UINT i;
	if (lsu->LSAs)
		u->LSAs = calloc(lsu->LSAsCount, sizeof* u->LSAs);
	for (i = 0; i < u->LSAsCount; i++)
		u->LSAs[i] = OSPF_LSA_MSG_COPY(lsu->LSAs[i]);
	return u;
}

void OSPF_LSUPDATE_MSG_FREE(ptrOSPFLSUPDATE lsu)
{
	UINT i;
	for (i = 0; i < lsu->LSAsCount; i++)
		OSPF_LSA_MSG_FREE(lsu->LSAs[i]);
	free(lsu->LSAs);
	free(lsu);
}

static void ospf_lsupdate_sendLSUPacket(UINT count,
										ptrOSPFLSAHDR* lsas,
										NETSIM_IPAddress dest)
{
	OSPFMSG type = OSPFMSG_LSUPDATE;
	NetSim_PACKET* packet = OSPF_PACKET_NEW(pstruEventDetails->dEventTime,
											type,
											pstruEventDetails->nDeviceId,
											pstruEventDetails->nInterfaceId);
	ptrOSPFPACKETHDR hdr = OSPF_PACKET_GET_HDR(packet);
	ptrOSPFLSUPDATE lsu = OSPF_HDR_GET_MSG(hdr);
	lsu->LSAsCount = count;
	lsu->LSAs = lsas;
	UINT i;
	for (i = 0; i < count; i++)
		OSPF_HDR_INCREASE_LEN(packet, lsas[i]->length);

	add_dest_to_packet(packet, 0);
	packet->pstruNetworkData->szDestIP = dest;
	packet->pstruNetworkData->szNextHopIp = ANY_DEST;
	packet->pstruNetworkData->nTTL = 2;

	OSPF_SEND_PACKET(packet);
}

static NETSIM_IPAddress ospf_lsu_getDstAddr(ptrOSPF_IF thisInterface)
{
	if (thisInterface->State == OSPFIFSTATE_DR ||
		thisInterface->State == OSPFIFSTATE_BACKUP)
		return AllSPFRouters;
	else if (thisInterface->Type == OSPFIFTYPE_BROADCAST)
		return AllDRouters;
	else if (thisInterface->Type == OSPFIFTYPE_P2P)
		return AllSPFRouters;
	else
		return ANY_DEST;
}

static UINT16 ospf_lsupdate_getMaxPayloadSize()
{
	return (INTERFACE_MTU_DEFAULT - IP_HDR_LEN -OSPFPACKETHDR_LEN-OSPFLSUPDATE_LEN_FIXED);
}

void ospf_lsupdate_send()
{
	ptrOSPF_PDS ospf = OSPF_PDS_CURRENT();
	ptrOSPF_IF thisInterface = OSPF_IF_CURRENT();

	if (ospf_list_is_empty(thisInterface->updateLSAList))
		return;


	char name[100];
	sprintf(name, "ospf_lsupdate_send LSU_%d", thisInterface->id);
	ospf_lsa_printList(form_dlogId("LSULIST", pstruEventDetails->nDeviceId),
					   thisInterface->updateLSAList, name);
	print_ospf_Dlog(form_dlogId("LSULIST", pstruEventDetails->nDeviceId),
					"******************************************************************");

	NETSIM_IPAddress destAddr = ospf_lsu_getDstAddr(thisInterface);

	UINT16 maxPayloadSize = ospf_lsupdate_getMaxPayloadSize();

	ptrOSPFLIST list = thisInterface->updateLSAList;
	void* pass = ospf_list_newIterator();
	ptrOSPFLSAHDR lsa = NULL;

	ptrOSPFLSAHDR* lsas = NULL;
	UINT count = 0;
	UINT16 payloadSize = 0;
	while ((lsa = ospf_list_iterate_mem(list, pass)) != NULL)
	{
		lsa->LSAge += (UINT16)(thisInterface->InfTransDelay / SECOND);
		if (ospf_lsa_maskDoNotAge(ospf, lsa->LSAge) > ospf->LSAMaxAge)
			ospf_lsa_assignNewLSAge(ospf, &lsa->LSAge, ospf->LSAMaxAge);
	LOOP_LSA_AGAIN:
		if (payloadSize == 0 &&
			lsa->length > maxPayloadSize)
		{
			//Send single lsa
			lsas = calloc(1, sizeof* lsas);
			lsas[0] = lsa;
			ospf_list_remove_mem(list, lsa, pass);
			ospf_lsupdate_sendLSUPacket(count, lsas, destAddr);
			lsas = NULL;
			continue;
		}
		if (payloadSize + lsa->length > maxPayloadSize)
		{
			ospf_lsupdate_sendLSUPacket(count, lsas, destAddr);
			lsas = NULL;
			payloadSize = 0;
			count = 0;
			goto LOOP_LSA_AGAIN;
		}

		if (count)
			lsas = realloc(lsas, (count + 1) * sizeof* lsas);
		else
			lsas = calloc(1, sizeof* lsas);

		lsas[count] = lsa;
		ospf_list_remove_mem(list, lsa, pass);
		count++;
		payloadSize += lsa->length;
	}
	ospf_list_deleteIterator(pass);
	if (count)
		ospf_lsupdate_sendLSUPacket(count, lsas, destAddr);
}

static bool ospf_lsupdate_print_neighborError(ptrOSPF_NEIGHBOR neigh,
											  NETSIM_IPAddress src)
{
	if (!neigh)
	{
		print_ospf_log(OSPF_LOG, "Router %d received LSU packet from unknown neighbor %s\n"
					   "Discarding LSU packet",
					   pstruEventDetails->nDeviceId,
					   src->str_ip);
		return false;
	}

	if (neigh->state < OSPFNEIGHSTATE_Exchange)
	{
		print_ospf_log(OSPF_LOG, "Neighbor %s, state is less than exchange.\n"
					   "Discarding LSU packet",
					   src->str_ip);
		return false;
	}
	return true;
}

static bool ospf_lsupdate_validate_lsa(ptrOSPFLSAHDR lsa,
									   ptrOSPFAREA_DS area)
{
	//Section 13(2)
	if (lsa->LSType < LSTYPE_ROUTERLSA ||
		lsa->LSType > LSTYPE_UNDEFINED)
	{
		print_ospf_log(OSPF_LOG, "Received bad LSA (Type = %d). Discard",
					   lsa->LSType);
		return false;
	}

	//Section 13(3)
	if (lsa->LSType == LSTYPE_ASEXTERNALLSA &&
		area &&
		!area->extRoutingCapability)
	{
		print_ospf_log(OSPF_LOG, "Received interface belong to stub area. Discarding ASExternal LSA");
		return false;
	}

	return true;
}

void ospf_handle_LSUPDATE()
{
	NetSim_PACKET* packet = pstruEventDetails->pPacket;
	NETSIM_IPAddress srcAddr = packet->pstruNetworkData->szSourceIP;
	ptrOSPF_PDS ospf = OSPF_PDS_CURRENT();
	ptrOSPF_IF thisInterface = OSPF_IF_CURRENT();
	ptrOSPFAREA_DS thisArea = OSPF_AREA_GET_IN(ospf, pstruEventDetails->nInterfaceId);
	ptrOSPF_NEIGHBOR neigh = OSPF_NEIGHBOR_FIND(thisInterface, srcAddr);
	bool isLSDBChanged = false;
	assert(neigh);
	if(!neigh)
		neigh = OSPF_NEIGHBOR_FIND(thisInterface, srcAddr);
	ptrARRAYLIST directAckList = ospf_arrayList_init(NULL);

	print_ospf_log(OSPF_LOG, "Router %d, LSU packet received from source %s",
				   pstruEventDetails->nDeviceId,
				   srcAddr->str_ip);

	if (!ospf_lsupdate_print_neighborError(neigh, srcAddr))
		goto DISCARD_LSUPDATE;

	ptrOSPFPACKETHDR hdr = OSPF_PACKET_GET_HDR(packet);
	ptrOSPFLSUPDATE lsu = OSPF_HDR_GET_MSG(hdr);
	print_ospf_log(OSPF_LOG, "%d LSA received in LSU packet", lsu->LSAsCount);

	UINT count;
	for (count = 0; count < lsu->LSAsCount; count++)
	{
		ptrOSPFLSAHDR lsa = lsu->LSAs[count];
		ospf_lsa_print(form_dlogId("RCVLSU", pstruEventDetails->nDeviceId), lsa, "Recv LSUpdate");
		print_ospf_log(OSPF_LOG, "Processing %d LSA of type %s.",
					   count + 1,
					   strLSTYPE[lsa->LSType]);

		if (!ospf_lsupdate_validate_lsa(lsa, thisArea))
			continue;

		ptrOSPFLSAHDR oldLsa = ospf_lsdb_lookup(ospf,
												thisArea,
												lsa->LSType,
												lsa->AdvertisingRouter,
												lsa->LinkStateID);

		// RFC 2328 Sec-13 (4)
		if (ospf_lsa_maskDoNotAge(ospf, lsa->LSAge) == ospf->LSAMaxAge &&
			!oldLsa &&
			!ospf_neighbor_isAnyNeighborInExchangeOrLoadingState(ospf))
		{
			ptrOSPFLSAHDR ackLSHdr;
			ackLSHdr = OSPF_LSA_HDR_COPY(lsa);
			ospf_add_ptr_to_arrayList(directAckList, ackLSHdr);

			print_ospf_log(OSPF_LOG, "LSA's LSAge is equal to MaxAge and there is "
						   "current no instance of the LSA in the LSDB,\n"
						   "and none of the routers neighbor are in state "
						   "Exchange or Loading. Send direct Ack\n");
			continue;
		}

		int isMoreRecent = ospf_lsa_isMoreRecent(ospf, lsa, oldLsa);

		// RFC 2328 Sec-13 (5)
		if (isMoreRecent)
		{
			print_ospf_log(OSPF_LOG, "Received LSA is more recent or no current "
						   "Database copy exist\n");

			if (ospf_lsdb_update(ospf,
								 thisInterface,
								 lsa,
								 thisArea,
								 srcAddr))
			{
				isLSDBChanged = true;
			}
			continue;
		}

		// RFC 2328 Sec-13 (6)
		if (ospf_lsReq_searchFromList(neigh, lsa))
		{
			ospf_event_set_and_call(OSPF_BadLSReq, neigh);
			ospf_arraylist_free(directAckList);
			return;
		}

		// RFC 2328 Sec-13 (7)
		if (!isMoreRecent)
		{
			ptrOSPFLSAHDR rxlsa;
			void* pass = ospf_list_newIterator();

			while ((rxlsa = ospf_list_iterate_mem(neigh->neighLSRxtList, pass)) != NULL)
			{
				if (ospf_lsa_compare(ospf, rxlsa, lsa))
					break;
			}
			ospf_list_deleteIterator(pass);
			if (rxlsa)
			{
				print_ospf_log(OSPF_LOG, "Received LSA treated as implied ack.\n"
							   "Removing LSA from retransmission list.");
				ospf_list_remove_mem(neigh->neighLSRxtList,
									 rxlsa,
									 NULL);
				ospf_lsa_printList(form_dlogId("RXTLIST", ospf->myId), neigh->neighLSRxtList, "remove Rxlist");
				if (ospf_list_is_empty(neigh->neighLSRxtList))
				{
					neigh->LSRxtTimer = false;
					neigh->LSRxtSeqNum++;
				}

				if (thisInterface->State == OSPFIFSTATE_BACKUP &&
					!OSPFID_COMPARE(thisInterface->designaterRouter, srcAddr))
				{
					print_ospf_log(OSPF_LOG, "I am BDR. send delayed ack to DR");
					ospf_lsaAck_sendDelayedAck(ospf,
											   thisInterface,
											   lsa);
				}
			}
			else
			{
				ptrOSPFLSAHDR ackLSA = OSPF_LSA_HDR_COPY(lsa);
				ospf_add_ptr_to_arrayList(directAckList, ackLSA);
			}
			continue;
		}

		// RFC 2328 Sec-13 (8)
		if (isMoreRecent < 0)
		{
			if (ospf_lsa_maskDoNotAge(ospf, oldLsa->LSAge) == ospf->LSAMaxAge &&
				oldLsa->LSSequenceNumber == OSPF_MAX_SEQUENCE_NUMBER)
			{
				print_ospf_log(OSPF_LOG, "Discarding LSA without ack");
			}
			else
			{
				ptrOSPFLSAHDR sendLSA = ospf_neighbor_searchSendList(neigh->linkStateSendList, lsa);

				if (sendLSA &&
					OSPF_CURR_TIME() - sendLSA->time > ospf->minLSInterval)
				{
					print_ospf_log(OSPF_LOG, "Sending new LSA to neighbor. Not adding to retx list");
					ospf_lsupdate_sendLSUPacket(1, &oldLsa, srcAddr);
					sendLSA->time = OSPF_CURR_TIME();
				}
				else if (!sendLSA)
				{
					ospf_lsupdate_sendLSUPacket(1, &oldLsa, srcAddr);
					ospf_neighbor_insertToSendList(neigh->linkStateSendList,
												   lsa,
												   OSPF_CURR_TIME());
				}
			}
		}
	}
	UINT i;
	for (i = 0; i < ospf->ifCount; i++)
	{
		ptrOSPF_IF inter = ospf->ospfIf[i];
		char name[100];
		sprintf(name, "Handle LSU_%d", inter->id);
		ospf_lsa_printList(form_dlogId("LSULIST", pstruEventDetails->nDeviceId),
						   inter->updateLSAList, name);
	}
	print_ospf_Dlog(form_dlogId("LSULIST", pstruEventDetails->nDeviceId),
					"******************************************************************");

	if (directAckList->count)
		ospf_lsaAck_sendDirectAck(ospf,
								  thisInterface->id,
								  directAckList,
								  srcAddr);

	ospf_arraylist_free(directAckList);

	if (isLSDBChanged)
		ospf_spf_scheduleCalculation(ospf);

	if (!ospf_list_is_empty(neigh->neighLSReqList))
	{
		// If we've got the desired LSA(s), send next request
		if (ospf_lsreq_isRequestedLSAReceived(neigh))
		{
			// Send next LS request
			ospf_lsreq_send(ospf,
							thisInterface->id,
							neigh->neighborIPAddr,
							false);
		}
	}
	else
	{
		// Cancel retransmission timer
		neigh->LSReqTimerSequenceNumber += 1;
		ospf_event_set_and_call(OSPF_LoadingDone,
								neigh);
	}

DISCARD_LSUPDATE:
	fn_NetSim_Packet_FreePacket(packet);
	pstruEventDetails->pPacket = NULL;
	print_ospf_log(OSPF_LOG, "");
}

void ospf_lsu_sendLSUpdateToNeighbor(ptrOSPF_PDS ospf,
									 ptrOSPF_IF thisInterface)
{
	if (ospf_list_is_empty(thisInterface->updateLSAList))
		return;

	// Determine Destination Address
	NETSIM_IPAddress dstAddr = ospf_lsu_getDstAddr(thisInterface);
	UINT16 maxPayloadSize = ospf_lsupdate_getMaxPayloadSize();

	ptrOSPFLIST list = thisInterface->updateLSAList;
	void* pass = ospf_list_newIterator();
	ptrOSPFLSAHDR lsa = NULL;

	ptrOSPFLSAHDR* lsas = NULL;
	UINT count = 0;
	UINT16 payloadSize = 0;
	while ((lsa = ospf_list_iterate_mem(list, pass)) != NULL)
	{
		LOOP_LSA_AGAIN:
		if (payloadSize == 0 &&
			lsa->length > maxPayloadSize)
		{
			//Send single lsa
			lsas = calloc(1, sizeof* lsas);
			lsas[0] = lsa;
			ospf_list_remove_mem(list, lsa, pass);
			ospf_lsupdate_sendLSUPacket(count, lsas, dstAddr);
			lsas = NULL;
			continue;
		}
		if (payloadSize + lsa->length > maxPayloadSize)
		{
			ospf_lsupdate_sendLSUPacket(count, lsas, dstAddr);
			lsas = NULL;
			payloadSize = 0;
			count = 0;
			goto LOOP_LSA_AGAIN;
		}
		// Increment LS age transmission delay (in seconds).
		lsa->LSAge = (UINT16)(lsa->LSAge +
			((thisInterface->InfTransDelay / SECOND)));

		// LS age has a maximum age limit.
		if (ospf_lsa_maskDoNotAge(ospf, lsa->LSAge) >
			ospf->LSAMaxAge)
		{
			ospf_lsa_assignNewLSAge(ospf,
									&lsa->LSAge,
									ospf->LSAMaxAge);
		}
		if (count)
			lsas = realloc(lsas, (count + 1) * sizeof* lsas);
		else
			lsas = calloc(1, sizeof* lsas);

		lsas[count] = lsa;
		count++;
		payloadSize += lsa->length;
		ospf_list_remove_mem(list, lsa, pass);
	}
	ospf_list_deleteIterator(pass);
	if (count)
		ospf_lsupdate_sendLSUPacket(count, lsas, dstAddr);
}

static void ospf_lsupdate_retransmitLSA(ptrOSPF_PDS ospf,
										ptrOSPF_IF thisInterface,
										ptrOSPF_NEIGHBOR neigh)
{
	if (ospf_list_is_empty(neigh->neighLSRxtList))
	{
		neigh->LSRxtTimer = false;
		neigh->LSRxtSeqNum++;
		return;
	}

	UINT16 maxPayloadSize = ospf_lsupdate_getMaxPayloadSize();
	UINT16 payloadSize = 0;
	UINT count = 0;
	ptrOSPFLSAHDR* lsas = NULL;
	ptrOSPFLSAHDR LSHeader;
	void* pass = ospf_list_newIterator();
	while ((LSHeader = ospf_list_iterate_mem(neigh->neighLSRxtList, pass)) != NULL)
	{
		// Check if this LSA have been in the list for at least rxmtInterval
		if ((OSPF_CURR_TIME() - LSHeader->time) <
			thisInterface->RxmtInterval)
			continue;

		if (payloadSize + LSHeader->length > maxPayloadSize)
			break;

		ptrOSPFLSAHDR sendListItem = ospf_neighbor_searchSendList(neigh->linkStateSendList,
																  LSHeader);

		if (sendListItem)
		{
			sendListItem->time = OSPF_CURR_TIME();
		}
		else
		{
			ospf_neighbor_insertToSendList(neigh->linkStateSendList,
										   LSHeader,
										   OSPF_CURR_TIME());
		}

		LSHeader->time = OSPF_CURR_TIME();
		// Increment LS age transmission delay (in seconds).
		LSHeader->LSAge = (UINT16)(LSHeader->LSAge + thisInterface->InfTransDelay / SECOND);

		// LS age has a maximum age limit.
		if (ospf_lsa_maskDoNotAge(ospf, LSHeader->LSAge) > ospf->LSAMaxAge)
		{
			ospf_lsa_assignNewLSAge(ospf,
									&LSHeader->LSAge,
									ospf->LSAMaxAge);
		}
		print_ospf_log(OSPF_LOG, "    Router %u Retransmitting LSA to router %s\n"
					   "    Type = %d, ID = %s, AdvRtr = %s, SeqNo = 0x%x",
					   ospf->myId,
					   neigh->neighborIPAddr->str_ip,
					   strLSTYPE[LSHeader->LSType],
					   LSHeader->LinkStateID->str_ip,
					   LSHeader->AdvertisingRouter->str_ip,
					   LSHeader->LSSequenceNumber);

		if (count)
			lsas = realloc(lsas, (count + 1) * sizeof* lsas);
		else
			lsas = calloc(1, sizeof* lsas);

		lsas[count] = LSHeader;
		count++;
		payloadSize += LSHeader->length;
		ospf_list_remove_mem(neigh->neighLSRxtList, LSHeader, pass);
	}
	ospf_list_deleteIterator(pass);
	if (count > 0)
	{
		ospf_lsupdate_sendLSUPacket(count,
									lsas,
									neigh->neighborIPAddr);
	}
	neigh->LSRxtSeqNum++;

	// If LSA still exist in retx list, start timer again
	if (!ospf_list_is_empty(neigh->neighLSRxtList))
	{
		ptrLSRXTTIMERDETAILS detail = calloc(1, sizeof* detail);
		detail->advertisingRouter = LSHeader->AdvertisingRouter;
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
	else
	{
		neigh->LSRxtTimer = false;
	}
}

static void ospf_lsupdate_rxmtTimer(ptrOSPF_PDS ospf,
									ptrOSPF_IF thisInterface,
									ptrOSPF_NEIGHBOR neigh,
									UINT seqNum,
									OSPFMSG type)
{
	bool foundpacket = false;
	switch (type)
	{
		case OSPFMSG_LSUPDATE:
		{
			// Timer expired.
			if (seqNum != neigh->LSRxtSeqNum)
			{
				print_ospf_log(OSPF_LOG, "Old timer. Ignore");
				break;
			}
			ospf_lsupdate_retransmitLSA(ospf, thisInterface, neigh);
		}
		break;
		case OSPFMSG_DD:
			fnNetSimError("Implement retransmission of DD packet");
#pragma message(__LOC__"Implement retransmission of DD packet")
			break;
		case OSPFMSG_LSREQUEST:
			if (neigh->LSReqTimerSequenceNumber != seqNum)
			{
				print_ospf_log(OSPF_LOG, "No packet to retransmit. Ignore");
				break;
			}

			if (ospf_list_is_empty(neigh->neighLSReqList))
			{
				// No packet to retransmit
				print_ospf_log(OSPF_LOG, "dont retransmit LS Req because "
							   "LS Req List is EMPTY!!");
				break;
			}
			ospf_lsreq_retransmit(ospf, thisInterface, neigh, seqNum);
			break;
		default:
			fnNetSimError("Unknown packet type %d in %s", type, __FUNCTION__);
			break;
	}

	if (foundpacket)
	{
		fnNetSimError("implement after DD rxt");
#pragma message(__LOC__"implement after DD rxt")
	}
}

void ospf_lsupdate_handleRxmtTimer()
{
	ptrLSRXTTIMERDETAILS details = pstruEventDetails->szOtherDetails;
	print_ospf_log(OSPF_LOG, "Rxmt Timer is expired");
	print_ospf_log(OSPF_LOG, "Seq number is %d", details->rxmtSeqNum);

	ptrOSPF_NEIGHBOR neigh = OSPF_NEIGHBOR_FIND_BY_IP(OSPF_IF_CURRENT(),
													  details->neighborIP);
	if (!neigh)
	{
		print_ospf_log(OSPF_LOG, "Got rxmt timer is called for neighbor %s"
					   "Neighbor no longer exists", details->neighborIP->str_ip);
		return;
	}
	
	// Retransmit packet to specified neighbor.
	ospf_lsupdate_rxmtTimer(OSPF_PDS_CURRENT(),
							OSPF_IF_CURRENT(),
							neigh,
							details->rxmtSeqNum,
							details->msgType);
	free(details);
}
