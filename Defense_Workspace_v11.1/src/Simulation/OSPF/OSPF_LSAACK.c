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

ptrOSPFLSACK OSPF_LSACK_COPY(ptrOSPFLSACK ack)
{
	UINT i;
	ptrOSPFLSACK a = calloc(1, sizeof* a);
	memcpy(a, ack, sizeof* a);
	if (ack->count)
	{
		a->LSAHeader = calloc(ack->count, sizeof* a->LSAHeader);
		for (i = 0; i < a->count; i++)
			a->LSAHeader[i] = OSPF_LSA_MSG_COPY(ack->LSAHeader[i]);
	}
	return a;
}

void OSPF_LSACK_FREE(ptrOSPFLSACK ack)
{
	UINT i;
	for (i = 0; i < ack->count; i++)
		OSPF_LSA_MSG_FREE(ack->LSAHeader[i]);
	free(ack);
}

void OSPF_LSACK_NEW(ptrOSPFPACKETHDR hdr)
{
	ptrOSPFLSACK ack = calloc(1, sizeof* ack);
	OSPF_HDR_SET_MSG(hdr,
					 OSPFMSG_LSACK,
					 ack,
					 OSPFLSACK_LEN_FIXED);
}

static void ospf_lsack_updateDst(NetSim_PACKET* packet,
								 NETSIM_IPAddress destAddr)
{
	NETSIM_IPAddress nextHop;
	NETSIM_ID nextHopId;

	if (!IP_COMPARE(destAddr, AllSPFRouters) ||
		!IP_COMPARE(destAddr, AllDRouters))
	{
		nextHop = ANY_DEST;
		nextHopId = 0;
	}
	else
	{
		NETSIM_ID in;
		nextHop = destAddr;
		nextHopId = fn_NetSim_Stack_GetDeviceId_asIP(nextHop, &in);
	}

	add_dest_to_packet(packet, nextHopId);
	packet->pstruNetworkData->szDestIP = destAddr;
	packet->pstruNetworkData->szNextHopIp = nextHop;
	packet->pstruNetworkData->nTTL = 2;
}

static void ospf_lsaacK_send(ptrOSPF_PDS ospf,
							 NETSIM_ID interfaceId,
							 ptrOSPFLSAHDR* lsa,
							 UINT16 count,
							 NETSIM_IPAddress destAddr)
{
	if (!count)
		return;

	OSPFMSG type = OSPFMSG_LSACK;
	NetSim_PACKET* packet = OSPF_PACKET_NEW(pstruEventDetails->dEventTime,
											type,
											ospf->myId,
											interfaceId);
	ptrOSPFPACKETHDR hdr = OSPF_PACKET_GET_HDR(packet);
	ptrOSPFLSACK ack = OSPF_HDR_GET_MSG(hdr);
	OSPF_HDR_INCREASE_LEN(packet, count*OSPFLSAHDR_LEN);

	ospf_lsack_updateDst(packet, destAddr);
	
	ack->count = count;
	ack->LSAHeader = lsa;

	OSPF_SEND_PACKET(packet);
}

void ospf_lsaAck_sendDelayedAck(ptrOSPF_PDS ospf,
								ptrOSPF_IF thisInterface,
								ptrOSPFLSAHDR lsa)
{
	ptrOSPFLSAHDR LSHeader = OSPF_LSA_MSG_COPY(lsa);
	ospf_list_add_mem(thisInterface->delayedAckList, LSHeader);

	if (!thisInterface->delayedAckTimer)
	{
		thisInterface->delayedAckTimer = true;
		double delay = NETSIM_RAND_01()*(thisInterface->RxmtInterval / 2);
		ospf_event_add(OSPF_CURR_TIME() + delay,
					   ospf->myId,
					   thisInterface->id,
					   OSPF_SENDDELAYEDACK,
					   NULL,
					   NULL);;
	}
}

void ospf_lsaAck_sendDirectAck(ptrOSPF_PDS ospf,
							   NETSIM_ID interfaceId,
							   ptrARRAYLIST ackList,
							   NETSIM_IPAddress nbrAddr)
{
	UINT maxCount = INTERFACE_MTU_DEFAULT - IP_HDR_LEN - OSPFPACKETHDR_LEN;
	maxCount = maxCount / OSPFLSAHDR_LEN;
	UINT16 count = 0;
	ptrOSPFLSAHDR* ackLSA = NULL;
	UINT i;
	for (i = 0; i < ackList->count; i++)
	{
		ptrOSPFLSAHDR lsa = ackList->mem[i];
		if (count == maxCount)
		{
			ospf_lsaacK_send(ospf, interfaceId, ackLSA, count, nbrAddr);
			ackLSA = NULL;
			count = 0;
			i--;
			continue;
		}

		if (count)
			ackLSA = realloc(ackLSA, (count + 1) * sizeof* ackLSA);
		else
			ackLSA = calloc(1, sizeof* ackLSA);

		ackLSA[count] = lsa;
		count++;
	}
	ospf_lsaacK_send(ospf, interfaceId, ackLSA, count, nbrAddr);
}

void ospf_lsack_handleDelayedAckTimer()
{
	ptrOSPF_PDS ospf = OSPF_PDS_CURRENT();
	ptrOSPF_IF thisInterface = OSPF_IF_CURRENT();

	if (!thisInterface->delayedAckTimer)
		return;

	NETSIM_IPAddress destAddr;
	if (thisInterface->State == OSPFIFSTATE_DR ||
		thisInterface->State == OSPFIFSTATE_BACKUP)
		destAddr = AllSPFRouters;
	else if (thisInterface->Type == OSPFIFTYPE_BROADCAST)
		destAddr = AllDRouters;
	else
		destAddr = AllSPFRouters;

	UINT maxCount = INTERFACE_MTU_DEFAULT - IP_HDR_LEN;
	maxCount -= OSPFLSACK_LEN_FIXED;
	maxCount -= OSPFPACKETHDR_LEN;
	maxCount /= OSPFLSAHDR_LEN;

	UINT16 count = 0;

	ptrOSPFLSAHDR listItem;
	void* pass = ospf_list_newIterator();
	ptrOSPFLSAHDR* ackList = NULL;
	while ((listItem = ospf_list_iterate_mem(thisInterface->delayedAckList, pass)) != NULL)
	{
		if (count)
			ackList = realloc(ackList, (count + 1) * sizeof* ackList);
		else
			ackList = calloc(1, sizeof* ackList);
		ackList[count] = listItem;
		listItem->lsaInfo = NULL;
		ospf_list_remove_mem(thisInterface->delayedAckList, listItem, pass);
		count++;

		if (count == maxCount)
		{
			ospf_lsaacK_send(ospf,
							 thisInterface->id,
							 ackList,
							 count,
							 destAddr);
			// Reset variables
			count = 0;
			ackList = NULL;
		}
	}
	ospf_list_deleteIterator(pass);
	ospf_lsaacK_send(ospf,
					 thisInterface->id,
					 ackList,
					 count,
					 destAddr);
	thisInterface->delayedAckTimer = false;
}

void ospf_handle_LSAck()
{
	NETSIM_ID d = pstruEventDetails->nDeviceId;
	ptrOSPF_PDS ospf = OSPF_PDS_CURRENT();
	ptrOSPF_IF thisInterface = OSPF_IF_CURRENT();
	NetSim_PACKET* packet = pstruEventDetails->pPacket;
	NETSIM_IPAddress sourceAddr = packet->pstruNetworkData->szSourceIP;
	ptrOSPF_NEIGHBOR neighbor = OSPF_NEIGHBOR_FIND_BY_IP(thisInterface, sourceAddr);

	print_ospf_log(OSPF_LOG, "Router %d, Time %0.3lf: Receive LSACK from neighbor %s",
				   d,
				   pstruEventDetails->dEventTime / MILLISECOND,
				   sourceAddr->str_ip);
	if (!neighbor)
	{
		print_ospf_log(OSPF_LOG, "Neighbor no longer exists. So ignore ack packet");
		goto DISCARD_LSACK;
	}

	if (neighbor->state < OSPFNEIGHSTATE_Exchange)
	{
		print_ospf_log(OSPF_LOG, "Neighbor state is less than exchange. So ignore ack packet");
		goto DISCARD_LSACK;
	}
	ptrOSPFPACKETHDR hdr = OSPF_PACKET_GET_HDR(packet);
	ptrOSPFLSACK ackPkt = OSPF_HDR_GET_MSG(hdr);
	UINT numAck = ackPkt->count;
	UINT count;
	ptrOSPFLSAHDR LSHeader;
	for (count = 0; count < numAck; count++)
	{
		LSHeader = ackPkt->LSAHeader[count];
		ptrOSPFLSAHDR listItem;
		void* pass = ospf_list_newIterator();
		ospf_lsa_printList(form_dlogId("RXTLIST", ospf->myId), neighbor->neighLSRxtList, "before Rxlist remove");
		while ((listItem = ospf_list_iterate_mem(neighbor->neighLSRxtList, pass)) != NULL)
		{
			// If LSA exists in retransmission list
			if (ospf_lsa_compareToListMem(ospf,
										  LSHeader,
										  listItem))
			{
				ospf_list_delete_mem(neighbor->neighLSRxtList, listItem, pass);
				print_ospf_log(OSPF_LOG, "Router %d: Removing LSA with seqno 0x%x from "
							   "rext list of nbr %s\n"
							   "        Type = %s, ID = %s,"
							   " advRtr = %s, age = %d",
							   d,
							   LSHeader->LSSequenceNumber,
							   neighbor->neighborId,
							   strLSTYPE[LSHeader->LSType],
							   LSHeader->LinkStateID->str_ip,
							   LSHeader->AdvertisingRouter->str_ip,
							   LSHeader->LSAge);
				break;
			}
		}
		ospf_list_deleteIterator(pass);
		ospf_lsa_printList(form_dlogId("RXTLIST", ospf->myId), neighbor->neighLSRxtList, "after Rxlist remove");
	}

	if (ospf_list_is_empty(neighbor->neighLSRxtList) &&
		neighbor->LSRxtTimer)
	{
		neighbor->LSRxtTimer = false;
		neighbor->LSRxtSeqNum++;
	}

DISCARD_LSACK:
	fn_NetSim_Packet_FreePacket(packet);
	pstruEventDetails->pPacket = NULL;
}
