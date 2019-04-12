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
#include "OSPF_Interface.h"
#include "OSPF_Neighbor.h"
#include "OSPF_List.h"

void OSPF_DD_MSG_NEW(ptrOSPFPACKETHDR hdr)
{
	ptrOSPFDD dd = calloc(1, sizeof* dd);
	OSPF_HDR_SET_MSG(hdr,
					 OSPFMSG_DD,
					 dd,
					 OSPFDD_LEN_FIXED);
}

ptrOSPFDD OSPF_DD_MSG_COPY(ptrOSPFDD dd)
{
	ptrOSPFDD d = calloc(1, sizeof* d);
	memcpy(d, dd, sizeof* d);
	if (dd->numLSA)
		d->LSAHeader = calloc(dd->numLSA, sizeof* d->LSAHeader);
	UINT i;
	for (i = 0; i < dd->numLSA; i++)
		d->LSAHeader[i] = OSPF_LSA_HDR_COPY(dd->LSAHeader[i]);
	return d;
}

void OSPF_DD_MSG_FREE(ptrOSPFDD dd)
{
	UINT i;
	for (i = 0; i < dd->numLSA; i++)
		OSPF_LSA_HDR_FREE(dd->LSAHeader[i]);
	free(dd->LSAHeader);
	free(dd);
}

static void set_DD_param_in_exstart(ptrOSPFPACKETHDR hdr,
									ptrOSPF_IF ospf,
									ptrOSPF_NEIGHBOR neigh)
{
	ptrOSPFDD dd = OSPF_HDR_GET_MSG(hdr);
	if (ospf->Type == OSPFIFTYPE_VIRTUALLINK)
		dd->InterfaceMTU = 0;
	else
		dd->InterfaceMTU = INTERFACE_MTU_DEFAULT;
	dd->DDSequenceNumber = neigh->DDSeqNo;
	DD_SET_INIT(dd);
	DD_SET_MORE(dd);
	DD_SET_MASTER(dd);
	neigh->IMMS = dd->IMMS;
}

static ptrOSPFLSAHDR* ospf_DD_getTopLSAFromList(ptrOSPFLIST list,
												UINT16* count,
												UINT maxCount)
{
	ptrOSPFLSAHDR* ret = NULL;
	UINT16 c = 0;
	ptrOSPFLSAHDR lsa;
	while ((lsa = ospf_list_get_mem(list)) != NULL)
	{
		if (ret)
			ret = realloc(ret, (c + 1) * sizeof* ret);
		else
			ret = calloc(1, sizeof* ret);
		ret[c] = lsa;
		c++;
		if (c >= maxCount)
			break;
	}
	*count = c;
	return ret;
}

static void set_DD_param_in_exchange(NetSim_PACKET* packet,
									ptrOSPF_IF ospf,
									ptrOSPF_NEIGHBOR neigh)
{
	ptrOSPFPACKETHDR hdr = OSPF_PACKET_GET_HDR(packet);
	ptrOSPFDD dd = OSPF_HDR_GET_MSG(hdr);
	if (ospf->Type == OSPFIFTYPE_VIRTUALLINK)
		dd->InterfaceMTU = 0;
	else
		dd->InterfaceMTU = INTERFACE_MTU_DEFAULT;
	dd->DDSequenceNumber = neigh->DDSeqNo;

	UINT maxLSAHdrCount = OSPF_DD_MAX_LSA_COUNT();
	ptrOSPFLSAHDR* lsahdr = ospf_DD_getTopLSAFromList(neigh->neighDBSummaryList,
													  &dd->numLSA,
													  maxLSAHdrCount);

	dd->LSAHeader = lsahdr;
	if (!ospf_list_is_empty(neigh->neighDBSummaryList))
		DD_SET_MORE(dd);

	if (neigh->isMaster)
		DD_SET_MASTER(dd);
	neigh->IMMS = dd->IMMS;
	
	OSPF_HDR_INCREASE_LEN(packet, dd->numLSA*OSPFLSAHDR_LEN);
}

static void ospf_DD_update_dst(NetSim_PACKET* packet,
							   ptrOSPF_NEIGHBOR neigh)
{
	add_dest_to_packet(packet, neigh->devId);
	packet->pstruNetworkData->szDestIP = neigh->neighborIPAddr;
	packet->pstruNetworkData->nTTL = 2;
}

static void send_DD_msg(ptrOSPF_IF ospf,
						ptrOSPF_NEIGHBOR neigh)
{
	OSPFMSG type = OSPFMSG_DD;
	NetSim_PACKET* packet = OSPF_PACKET_NEW(pstruEventDetails->dEventTime,
											type,
											pstruEventDetails->nDeviceId,
											pstruEventDetails->nInterfaceId);

	ospf_DD_update_dst(packet, neigh);
	ptrOSPFPACKETHDR hdr = OSPF_PACKET_GET_HDR(packet);

	if (neigh->state == OSPFNEIGHSTATE_ExStart)
		set_DD_param_in_exstart(hdr, ospf, neigh);
	else if (neigh->state == OSPFNEIGHSTATE_Exchange)
		set_DD_param_in_exchange(packet, ospf, neigh);

	OSPF_SEND_PACKET(packet);

	if (neigh->lastSentDDPacket)
		OSPF_HDR_FREE(neigh->lastSentDDPacket);
	neigh->lastSentDDPacket = OSPF_HDR_COPY(hdr);
	neigh->lastDDMsgSentTime = pstruEventDetails->dEventTime;
}

static void copy_DD_param(ptrOSPFPACKETHDR hdr,
						  ptrOSPF_NEIGHBOR neigh)
{
	ptrOSPFDD dd = hdr->ospfMSG;
	ptrOSPFDD old = neigh->lastrecvDDPacket->ospfMSG;
	dd->DDSequenceNumber = old->DDSequenceNumber;
	dd->Option = old->Option;
	dd->InterfaceMTU = old->InterfaceMTU;

	if (old->numLSA)
	{
		dd->numLSA = old->numLSA;
		dd->LSAHeader = calloc(dd->numLSA, sizeof* dd->LSAHeader);
		UINT i;
		for (i = 0; i < old->numLSA; i++)
			dd->LSAHeader[i] = OSPF_LSA_HDR_COPY(old->LSAHeader[i]);
	}

	dd->IMMS = old->IMMS;
}

static void resend_DD_msg(ptrOSPF_IF ospf,
						  ptrOSPF_NEIGHBOR neigh)
{
	OSPFMSG type = OSPFMSG_DD;
	NetSim_PACKET* packet = OSPF_PACKET_NEW(pstruEventDetails->dEventTime,
											type,
											pstruEventDetails->nDeviceId,
											pstruEventDetails->nInterfaceId);

	ospf_DD_update_dst(packet, neigh);
	ptrOSPFPACKETHDR hdr = OSPF_PACKET_GET_HDR(packet);

	copy_DD_param(hdr, neigh);
	OSPF_SEND_PACKET(packet);

	if (neigh->lastSentDDPacket)
		OSPF_HDR_FREE(neigh->lastSentDDPacket);
	neigh->lastSentDDPacket = OSPF_HDR_COPY(hdr);
	neigh->lastDDMsgSentTime = pstruEventDetails->dEventTime;
}

static void add_DD_rxmt_timer_in_exstart_state(ptrOSPF_IF ospf,
											   ptrOSPF_NEIGHBOR neigh)
{
	ospf_event_add(pstruEventDetails->dEventTime + ospf->RxmtInterval*SECOND,
				   pstruEventDetails->nDeviceId,
				   pstruEventDetails->nInterfaceId,
				   OSPF_DD_RXMT_IN_EXSTART,
				   NULL,
				   neigh);
}

void start_sending_dd_msg()
{
	ptrOSPF_IF ospf = OSPF_IF_CURRENT();
	ptrOSPF_NEIGHBOR neigh = pstruEventDetails->szOtherDetails;
	if (neigh->state != OSPFNEIGHSTATE_ExStart)
		return;
	print_ospf_log(OSPF_LOG, "Time %0.4lf, Router %d, interface %d (%s), Sending DD msg to neighbor %s",
				   pstruEventDetails->dEventTime/MILLISECOND,
				   pstruEventDetails->nDeviceId,
				   pstruEventDetails->nInterfaceId,
				   DEVICE_MYIP()->str_ip,
				   neigh->neighborId->str_ip);
	print_ospf_log(OSPF_LOG, "Neighbor state is %s",
				   strNeighborState[neigh->state]);
	send_DD_msg(ospf, neigh);
	add_DD_rxmt_timer_in_exstart_state(ospf, neigh);
	print_ospf_log(OSPF_LOG, "\n");
}

static void ospf_process_DD(ptrOSPF_PDS ospf,
							ptrOSPF_IF thisInterface,
							ptrOSPF_NEIGHBOR neigh,
							ptrOSPFPACKETHDR hdr)
{
	print_ospf_log(OSPF_LOG, "Processing DD msg");
	ptrOSPFDD dd = hdr->ospfMSG;
	ptrOSPFAREA_DS area = OSPF_AREA_GET_ID(ospf, thisInterface->areaId);
	UINT numLSA = dd->numLSA;
	UINT i;
	for (i = 0; i < numLSA; i++)
	{
		ptrOSPFLSAHDR lsa = dd->LSAHeader[i];
		ptrOSPFLSAHDR oldLsa = NULL;

		// Check for the validity of LSA
		if (lsa->LSType < LSTYPE_ROUTERLSA ||
			lsa->LSType >= LSTYPE_UNDEFINED)
		{
			print_ospf_log(OSPF_LOG, "Unknown LSA type %d. Discard DD packet",
						   lsa->LSType);

			ospf_event_set_and_call(OSPF_SeqNumberMismatch, neigh);
			return;
		}

		// Looks up for the LSA in its own Database
		oldLsa = ospf_lsdb_lookup(ospf, area, lsa->LSType,
								  lsa->AdvertisingRouter, lsa->LinkStateID);

		// Add to request list if the LSA doesn't exist or if
		// this instance is more recent one

		if (!oldLsa ||
			ospf_lsa_compare(ospf, oldLsa, lsa) > 0)
		{
			print_ospf_log(OSPF_LOG, "Adding LSA to request list");
			ospf_lsreq_insertToList(neigh, lsa, OSPF_CURR_TIME());
		}
	}

	if (neigh->isMaster)
	{
		neigh->DDSeqNo++;
		if (!isBitSet(neigh->IMMS, DD_MORE_BIT_INDEX) &&
			!DD_IS_MORE(dd))
		{
			print_ospf_log(OSPF_LOG, "Scheduling event Exchange_Done");
			ospf_event_add(pstruEventDetails->dEventTime,
						   pstruEventDetails->nDeviceId,
						   pstruEventDetails->nInterfaceId,
						   OSPF_ExchangeDone,
						   NULL,
						   neigh);
		}
		else
		{
			print_ospf_log(OSPF_LOG, "More bit is set. Sending DD msg");
			send_DD_msg(thisInterface, neigh);
		}
	}
	else
	{
		neigh->DDSeqNo = dd->DDSequenceNumber;
		print_ospf_log(OSPF_LOG, "I am slave. Sending DD msg");
		send_DD_msg(thisInterface, neigh);
		if (!DD_IS_MORE(dd) &&
			ospf_list_is_empty(neigh->neighDBSummaryList))
		{
			print_ospf_log(OSPF_LOG, "Scheduling event Exchange_Done");
			ospf_event_add(pstruEventDetails->dEventTime,
						   pstruEventDetails->nDeviceId,
						   pstruEventDetails->nInterfaceId,
						   OSPF_ExchangeDone,
						   NULL,
						   neigh);
		}
	}
}

static bool validate_dd_msg(ptrOSPF_IF ospf,
							ptrOSPF_NEIGHBOR neigh,
							ptrOSPFDD dd)
{
	if (dd->InterfaceMTU > INTERFACE_MTU_DEFAULT)
		return false;

	if (!neigh)
		return false;

	if (neigh->state == OSPFNEIGHSTATE_DOWN)
		return false;

	if (neigh->state == OSPFNEIGHSTATE_Attempt)
		return false;

	return true;
}

static bool ospf_dd_check_duplicate(ptrOSPF_NEIGHBOR neigh,
									ptrOSPFPACKETHDR hdr)
{
	if (!neigh->lastrecvDDPacket)
		return false;
	ptrOSPFDD n = hdr->ospfMSG;
	ptrOSPFDD o = neigh->lastrecvDDPacket->ospfMSG;

	if (n->DDSequenceNumber == o->DDSequenceNumber &&
		n->IMMS == o->IMMS &&
		n->Option == o->Option)
		return true;

	return false;
}

static void ospf_process_dd_in_exstart_state(ptrOSPF_PDS ospf,
											 ptrOSPF_IF ospfif,
											 ptrOSPF_NEIGHBOR neigh,
											 ptrOSPFPACKETHDR hdr)
{
	ptrOSPFDD dd = hdr->ospfMSG;
	if (DD_IS_INIT(dd) &&
		DD_IS_MORE(dd) &&
		DD_IS_MASTER(dd) &&
		!dd->LSAHeader &&
		OSPFID_ISGREATER(hdr->RouterId,ospf->routerId))
	{
		neigh->DDSeqNo = dd->DDSequenceNumber;
		neigh->isMaster = false;
		print_ospf_log(OSPF_LOG, "I am now slave.");
	}
	else if (!DD_IS_INIT(dd) &&
			 !DD_IS_MASTER(dd) &&
			 dd->DDSequenceNumber == neigh->DDSeqNo &&
			 OSPFID_ISSMALLER(hdr->RouterId,ospf->routerId))
	{
		neigh->isMaster = true;
		print_ospf_log(OSPF_LOG, "I am now master.");
	}
	else
	{
		print_ospf_log(OSPF_LOG, "Neither master nor slave. Who i am? Ignoring DD msg.");
		return;
	}

	ospf_event_set_and_call(OSPF_NegotiationDone, neigh);
	neigh->neighborOption = dd->Option;
	ospf_process_DD(ospf, ospfif, neigh, hdr);
}

static void ospf_process_dd_in_init_state(ptrOSPF_NEIGHBOR neigh,
										  ptrOSPFPACKETHDR hdr)
{
	ospf_event_set_and_call(OSPF_2WayReceived, neigh);
}

static void ospf_process_dd_in_exchange_state_for_duplicate(ptrOSPF_IF ospf,
															ptrOSPF_NEIGHBOR neigh,
															ptrOSPFPACKETHDR hdr)
{
	if (neigh->isMaster)
		print_ospf_log(OSPF_LOG, "I am master. Discarding duplicate DD");
	else
		resend_DD_msg(ospf, neigh);
}

static void ospf_process_dd_in_exchange_state(ptrOSPF_PDS ospf,
											  ptrOSPF_IF thisInterface,
											  ptrOSPF_NEIGHBOR neigh,
											  ptrOSPFPACKETHDR hdr,
											  bool isDuplicate)
{
	if (isDuplicate)
	{
		ospf_process_dd_in_exchange_state_for_duplicate(thisInterface, neigh, hdr);
		return;
	}

	ptrOSPFDD dd = hdr->ospfMSG;

	/*
	If the state of the MS-bit is inconsistent with the
	master/slave state of the connection, generate the
	neighbor event SeqNumberMismatch and stop processing the
	packet.
	*/
	if ((DD_IS_MASTER(dd) && neigh->isMaster) ||
		(!DD_IS_MASTER(dd) && !neigh->isMaster))
	{
		print_ospf_log(OSPF_LOG, "MS-bit is inconsistent with the "
					   "master / slave state of the connection");
		ospf_event_set_and_call(OSPF_SeqNumberMismatch, neigh);
		return;
	}

	/*
	If the initialize(I) bit is set, generate the neighbor
	event SeqNumberMismatch and stop processing the packet
	*/
	if (DD_IS_INIT(dd))
	{
		print_ospf_log(OSPF_LOG, "Initialize(I) bit is set");
		ospf_event_set_and_call(OSPF_SeqNumberMismatch, neigh);
		return;
	}

	/*
	If the packet's Options field indicates a different set
	of optional OSPF capabilities than were previously
	received from the neighbor (recorded in the Neighbor
	Options field of the neighbor structure), generate the
	neighbor event SeqNumberMismatch and stop processing the
	packet
	*/
	if (dd->Option != neigh->neighborOption)
	{
		print_ospf_log(OSPF_LOG, "packet's Options field indicates a different set "
					   "of optional OSPF capabilities than were previously "
					   "received from the neighbor(recorded in the Neighbor "
					   "Options field of the neighbor structure)");
		ospf_event_set_and_call(OSPF_SeqNumberMismatch, neigh);
		return;
	}
	
	/*
	Database Description packets must be processed in
	sequence, as indicated by the packets' DD sequence
	numbers. If the router is master, the next packet
	received should have DD sequence number equal to the DD
	sequence number in the neighbor data structure. If the
	router is slave, the next packet received should have DD
	sequence number equal to one more than the DD sequence
	number stored in the neighbor data structure. In either
	case, if the packet is the next in sequence it should be
	accepted and its contents processed.
	*/
	if ((neigh->isMaster &&
		 neigh->DDSeqNo != dd->DDSequenceNumber) ||
		 (!neigh->isMaster &&
		  dd->DDSequenceNumber != neigh->DDSeqNo + 1))
	{
		print_ospf_log(OSPF_LOG, "DD seq number mismatch");
		ospf_event_set_and_call(OSPF_SeqNumberMismatch, neigh);
		return;
	}

	ospf_process_DD(ospf, thisInterface, neigh, hdr);
}

static void ospf_process_dd_in_loading_or_full_state(ptrOSPF_IF ospf,
													 ptrOSPF_NEIGHBOR neigh,
													 ptrOSPFPACKETHDR hdr,
													 bool isDuplicate)
{
	if (isDuplicate)
	{
		if (neigh->isMaster)
		{
			print_ospf_log(OSPF_LOG, "Duplicate DD msg received. I am master. Discarding packet");
			return;
		}
		else
		{
			if (pstruEventDetails->dEventTime - neigh->lastDDMsgSentTime <
				ospf->routerDeadInterval*SECOND)
			{
				resend_DD_msg(ospf, neigh);
				return;
			}
		}
	}

	print_ospf_log(OSPF_LOG, "DD seq number mismatch");
	ospf_event_set_and_call(OSPF_SeqNumberMismatch, neigh);
}

void ospf_handle_DD()
{
	NETSIM_ID d = pstruEventDetails->nDeviceId;
	NETSIM_ID in = pstruEventDetails->nInterfaceId;
	NetSim_PACKET* packet = pstruEventDetails->pPacket;
	ptrOSPFPACKETHDR hdr = OSPF_PACKET_GET_HDR(packet);
	ptrOSPFDD dd = OSPF_HDR_GET_MSG(hdr);
	ptrOSPF_PDS ospf = OSPF_PDS_CURRENT();
	ptrOSPF_IF thisInterface = OSPF_IF_CURRENT();
	OSPFID rid = hdr->RouterId;
	ptrOSPF_NEIGHBOR neigh = OSPF_NEIGHBOR_FIND(thisInterface, rid);

	print_ospf_log(OSPF_LOG, "Time %0.4lf ms, Router %d interface %d (%s) received DD msg from neighbor %s",
				   pstruEventDetails->dEventTime / MILLISECOND,
				   d,
				   in,
				   DEVICE_NWADDRESS(d, in)->str_ip,
				   hdr->RouterId->str_ip);

	if (!validate_dd_msg(thisInterface, neigh, dd))
	{
		print_ospf_log(OSPF_LOG, "DD msg is not validated. Deleting DD msg...");
		goto TERMINATE_PROCESSING_DD;
	}

	bool isDuplicate = ospf_dd_check_duplicate(neigh, hdr);

	OSPF_HDR_FREE(neigh->lastrecvDDPacket);
	neigh->lastrecvDDPacket = OSPF_HDR_COPY(hdr);

	print_ospf_log(OSPF_LOG, "Neighbor state is %s", strNeighborState[neigh->state]);

	switch (neigh->state)
	{
		case OSPFNEIGHSTATE_DOWN:
		case OSPFNEIGHSTATE_Attempt:
		case OSPFNEIGHSTATE_2Way:
			print_ospf_log(OSPF_LOG, "Ignoring DD msg");
			break;
		case OSPFNEIGHSTATE_Init:
			ospf_process_dd_in_init_state(neigh, hdr);
			if (neigh->state != OSPFNEIGHSTATE_ExStart)
				break;
		case OSPFNEIGHSTATE_ExStart:
			ospf_process_dd_in_exstart_state(ospf,
											 thisInterface,
											 neigh,
											 hdr);
			break;
		case OSPFNEIGHSTATE_Exchange:
			ospf_process_dd_in_exchange_state(ospf, thisInterface, neigh, hdr, isDuplicate);
			break;
		case OSPFNEIGHSTATE_Loading:
		case OSPFNEIGHSTATE_Full:
			ospf_process_dd_in_loading_or_full_state(thisInterface, neigh, hdr, isDuplicate);
			break;
		default:
			fnNetSimError("Unknown neighbor state %s in %s.",
						  strNeighborState[neigh->state],
						  __FUNCTION__);
			break;
	}

TERMINATE_PROCESSING_DD:
	fn_NetSim_Packet_FreePacket(packet);
	pstruEventDetails->pPacket = NULL;
	print_ospf_log(OSPF_LOG, "\n");
}
