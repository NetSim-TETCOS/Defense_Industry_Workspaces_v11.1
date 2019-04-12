/************************************************************************************
* Copyright (C) 2017                                                               *
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
#include "OSPF_enum.h"
#include "OSPF_Msg.h"
#include "OSPF_Neighbor.h"
#include "OSPF_Interface.h"

#define OSPF_HELLO_GET_DR_FROM_IF(i)	(i->designaterRouter?i->designaterRouter:NullID)
#define OSPF_HELLO_GET_BDR_FROM_IF(i)	(i->backupDesignaterRouter?i->backupDesignaterRouter:NullID)

void OSPF_HELLO_MSG_NEW(ptrOSPFPACKETHDR hdr)
{
	ptrOSPFHELLO hello = calloc(1, sizeof* hello);
	OSPF_HDR_SET_MSG(hdr,
					 OSPFMSG_HELLO,
					 hello,
					 OSPFHELLO_LEN_FIXED);
}

ptrOSPFHELLO OSPF_HELLO_MSG_COPY(ptrOSPFHELLO hello)
{
	ptrOSPFHELLO h = calloc(1, sizeof* hello);
	memcpy(h, hello, sizeof* hello);
	UINT i;
	if (hello->neighCount)
		h->Neighbor = calloc(hello->neighCount, sizeof* h->Neighbor);
	for (i = 0; i < hello->neighCount; i++)
		h->Neighbor[i] = IP_COPY(hello->Neighbor[i]);
	return h;
}

void OSPF_HELLO_MSG_FREE(ptrOSPFHELLO hello)
{
	free(hello);
}

void start_interval_hello_timer()
{
	double delay = NETSIM_RAND_01()*OSPF_BROADCAST_JITTER;

	NetSim_EVENTDETAILS pevent;
	memset(&pevent, 0, sizeof pevent);
	pevent.dEventTime = pstruEventDetails->dEventTime + delay;
	pevent.nDeviceId = pstruEventDetails->nDeviceId;
	pevent.nDeviceType = DEVICE_TYPE(pevent.nDeviceId);
	pevent.nEventType = TIMER_EVENT;
	pevent.nInterfaceId = pstruEventDetails->nInterfaceId;
	pevent.nProtocolId = APP_PROTOCOL_OSPF;
	pevent.nSubEventType = OSPF_HELLO_TIMER;
	fnpAddEvent(&pevent);

	if (isOSPFHelloDebug)
		print_ospf_log(OSPF_HELLO_LOG, "Starting interval hello timer for router %d-%d at time %lf",
					   pstruEventDetails->nDeviceId,
					   pstruEventDetails->nInterfaceId,
					   pstruEventDetails->dEventTime);
}

static void add_next_hello_timer()
{
	ptrOSPF_IF ospfIf = OSPF_IF_CURRENT();
	double delay = NETSIM_RAND_01()*OSPF_BROADCAST_JITTER;
	NetSim_EVENTDETAILS pevent;
	memset(&pevent, 0, sizeof pevent);
	pevent.dEventTime = pstruEventDetails->dEventTime +
		ospfIf->helloInterval*SECOND + delay;
	pevent.nDeviceId = pstruEventDetails->nDeviceId;
	pevent.nDeviceType = DEVICE_TYPE(pevent.nDeviceId);
	pevent.nEventType = TIMER_EVENT;
	pevent.nInterfaceId = pstruEventDetails->nInterfaceId;
	pevent.nProtocolId = APP_PROTOCOL_OSPF;
	pevent.nSubEventType = OSPF_HELLO_TIMER;
	fnpAddEvent(&pevent);

	if (isOSPFHelloDebug)
		print_ospf_log(OSPF_HELLO_LOG, "Scheduling next interval hello timer for router %d-%d at time %lf",
					   pstruEventDetails->nDeviceId,
					   pstruEventDetails->nInterfaceId,
					   pstruEventDetails->dEventTime);
}

static UINT16 add_neighborToHello(ptrOSPFHELLO hello, ptrOSPF_IF ospf)
{
	UINT16 c = 0;
	UINT i;
	print_ospf_log(OSPF_HELLO_LOG, "Adding neighbor to hello message");
	for (i = 0; i < ospf->neighborRouterCount; i++)
	{
		if (ospf->neighborRouterList[i]->state >= OSPFNEIGHSTATE_Init)
		{
			if (hello->Neighbor)
				hello->Neighbor = realloc(hello->Neighbor, (c + 1) * sizeof* hello->Neighbor);
			else
				hello->Neighbor = calloc(1, sizeof* hello->Neighbor);
			hello->Neighbor[c] = ospf->neighborRouterList[i]->neighborId;
			print_ospf_log(OSPF_HELLO_LOG, "%s neighbor is added",
						   ospf->neighborRouterList[i]->neighborId->str_ip);
			hello->neighCount++;
			c++;
		}
	}
	print_ospf_log(OSPF_HELLO_LOG, "%d neighbor is added out of %d", c, ospf->neighborRouterCount);
	return c;
}

static void set_hello_param(NetSim_PACKET* packet, ptrOSPFPACKETHDR hdr)
{
	ptrOSPFHELLO hello = OSPF_HDR_GET_MSG(hdr);
	ptrOSPF_IF thisInterface = OSPF_IF_CURRENT();
	hello->NetworkMask = DEVICE_CURR_MASK;
	hello->HelloInterval = OSPFTIME_TO_UINT16(thisInterface->helloInterval);
	hello->RtrPri = thisInterface->RouterPriority;
	hello->RouterDeadInterval = OSPFTIME_TO_UINT16(thisInterface->routerDeadInterval);
	hello->DesignatedRouter = OSPF_HELLO_GET_DR_FROM_IF(thisInterface);
	hello->BackupDesignatedRouter = OSPF_HELLO_GET_BDR_FROM_IF(thisInterface);
	UINT16 count = add_neighborToHello(hello, thisInterface);
	OSPF_HDR_INCREASE_LEN(packet, count * 4);
}

static void ospf_hello_update_dst(NetSim_PACKET* packet)
{
	add_dest_to_packet(packet, 0);
	packet->pstruNetworkData->szDestIP = AllSPFRouters;
	packet->pstruNetworkData->szNextHopIp = AllSPFRouters;
	packet->pstruNetworkData->nTTL = 2;
}

static void send_hello()
{
	OSPFMSG type = OSPFMSG_HELLO;
	NetSim_PACKET* packet = OSPF_PACKET_NEW(pstruEventDetails->dEventTime,
											type,
											pstruEventDetails->nDeviceId,
											pstruEventDetails->nInterfaceId);

	ospf_hello_update_dst(packet);
	ptrOSPFPACKETHDR hdr = OSPF_PACKET_GET_HDR(packet);

	set_hello_param(packet, hdr);

	OSPF_SEND_PACKET(packet);
}

void ospf_handle_helloTimer_event()
{
	ptrOSPF_IF thisInterface = OSPF_IF_CURRENT();
	if (thisInterface->State == OSPFIFSTATE_DOWN)
		return; // No hello is send if interface state is down.

	add_next_hello_timer();

	print_ospf_log(OSPF_HELLO_LOG, "Time %0.4lf, Router %d, interface %d (%s) is sending hello msg",
				   pstruEventDetails->dEventTime / MILLISECOND,
				   pstruEventDetails->nDeviceId,
				   pstruEventDetails->nInterfaceId,
				   DEVICE_MYIP()->str_ip);
	send_hello();
	print_ospf_log(OSPF_HELLO_LOG, "\n");
}

static bool validate_hello_msg()
{
	NetSim_PACKET* packet = pstruEventDetails->pPacket;
	ptrOSPFPACKETHDR hdr = OSPF_PACKET_GET_HDR(packet);
	ptrOSPFHELLO hello = OSPF_HDR_GET_MSG(hdr);
	ptrOSPF_IF thisInterface = OSPF_IF_CURRENT();

	if (thisInterface->Type != OSPFIFTYPE_P2P &&
		thisInterface->Type != OSPFIFTYPE_VIRTUALLINK)
	{
		if (IP_COMPARE(thisInterface->IPIfMask, hello->NetworkMask))
		{
			fnNetSimError("Router %d-%d received hello packet with different net mask than configured\n",
						  pstruEventDetails->nDeviceId,
						  pstruEventDetails->nInterfaceId);
			return false;
		}
	}

	if (thisInterface->helloInterval != hello->HelloInterval)
	{
		fnNetSimError("Router %d-%d received hello packet with different hello interval than configured\n",
					  pstruEventDetails->nDeviceId,
					  pstruEventDetails->nInterfaceId);
		return false;
	}

	if (thisInterface->routerDeadInterval != hello->RouterDeadInterval)
	{
		fnNetSimError("Router %d-%d received hello packet with different router dead interval than configured\n",
					  pstruEventDetails->nDeviceId,
					  pstruEventDetails->nInterfaceId);
		return false;
	}
	return true;
}

static bool is_ip_present_in_hello(ptrOSPFHELLO hello, NETSIM_IPAddress ip)
{
	UINT i;
	for (i = 0; i < hello->neighCount; i++)
	{
		if (!IP_COMPARE(hello->Neighbor[i], ip))
			return true;
	}
	return false;
}

static bool ospf_hello_findNeighbor(ptrOSPF_IF thisInterface,
									NETSIM_IPAddress srcAddr,
									OSPFID rid,
									ptrOSPF_NEIGHBOR* neighbor)
{
	UINT i;
	for (i = 0; i < thisInterface->neighborRouterCount; i++)
	{
		ptrOSPF_NEIGHBOR neigh = thisInterface->neighborRouterList[i];
		if (thisInterface->Type == OSPFIFTYPE_BROADCAST ||
			thisInterface->Type == OSPFIFTYPE_P2MP)
		{
			if (!IP_COMPARE(neigh->neighborIPAddr, srcAddr))
			{
				*neighbor = neigh;
				return true;
			}
		}
		else
		{
			if (!OSPFID_COMPARE(neigh->neighborId, rid))
			{
				if (IP_COMPARE(neigh->neighborIPAddr, srcAddr))
				{
					*neighbor = NULL;
					return false;
				}
				*neighbor = neigh;
				return true;
			}
		}
	}
	*neighbor = NULL;
	return true;
}

void ospf_process_hello()
{
	NETSIM_ID d = pstruEventDetails->nDeviceId;
	NETSIM_ID in = pstruEventDetails->nInterfaceId;
	NetSim_PACKET* packet = pstruEventDetails->pPacket;
	NETSIM_IPAddress src = packet->pstruNetworkData->szSourceIP;
	ptrOSPFPACKETHDR hdr = OSPF_PACKET_GET_HDR(packet);
	NETSIM_IPAddress rid = hdr->RouterId;
	ptrOSPFHELLO hello = OSPF_HDR_GET_MSG(hdr);
	ptrOSPF_PDS ospf = OSPF_PDS_CURRENT();
	ptrOSPF_IF thisIntf = OSPF_IF_CURRENT();
	ptrOSPFAREA_DS thisArea = OSPF_AREA_GET_ID(ospf, thisIntf->areaId);
	ptrOSPF_NEIGHBOR tempNeighborInfo = NULL;
	BOOL neighborFound = false;
	UINT numNeighbor;
	print_ospf_log(OSPF_HELLO_LOG, "Time %0.4lf ms, Router %d interface %d (%s) received hello msg from neighbor %s",
				   pstruEventDetails->dEventTime / MILLISECOND,
				   d,
				   in,
				   DEVICE_NWADDRESS(d, in)->str_ip,
				   hdr->RouterId->str_ip);

	if (!validate_hello_msg())
	{
		print_ospf_log(OSPF_HELLO_LOG, "Hello msg is not validated. Deleting hello msg...");
		goto TERMINATE_PROCESSING_HELLO;
	}

	NETSIM_IPAddress nbrPrevDR = NULL;
	NETSIM_IPAddress nbrPrevBDR = NULL;
	UINT8 nbrPrevPriority = 0;
	bool routerIdInHelloPktBody = false;

	numNeighbor = hello->neighCount;
	print_ospf_log(OSPF_HELLO_LOG, "%d neighbor is present in hello message", numNeighbor);

	if (!ospf_hello_findNeighbor(thisIntf, src, rid, &tempNeighborInfo))
		goto TERMINATE_PROCESSING_HELLO;

	if (tempNeighborInfo)
		neighborFound = true;

	if (!neighborFound)
	{
		print_ospf_log(OSPF_HELLO_LOG, "Neighbor is not found in neighbor list");
		print_ospf_log(OSPF_HELLO_LOG, "Adding new neighbor of RID %s, IP addr %s",
					   rid->str_ip, src->str_ip);
		tempNeighborInfo = ospf_neighbor_new(src, rid);
		ospf_neighbor_add(thisIntf, tempNeighborInfo);
		tempNeighborInfo->neighborDesignateRouter = hello->DesignatedRouter;
		tempNeighborInfo->neighborDesignateBackupRouter = hello->BackupDesignatedRouter;
	}
	nbrPrevDR = tempNeighborInfo->neighborDesignateRouter;
	nbrPrevBDR = tempNeighborInfo->neighborDesignateBackupRouter;

	if (thisIntf->Type == OSPFIFTYPE_BROADCAST ||
		thisIntf->Type == OSPFIFTYPE_P2MP)
	{
		nbrPrevPriority = tempNeighborInfo->neighborPriority;
		tempNeighborInfo->neighborDesignateRouter = hello->DesignatedRouter;
		tempNeighborInfo->neighborDesignateBackupRouter = hello->BackupDesignatedRouter;
		tempNeighborInfo->neighborPriority = hello->RtrPri;
	}
	else if (thisIntf->Type == OSPFIFTYPE_P2P)
	{
		nbrPrevPriority = tempNeighborInfo->neighborPriority;
		tempNeighborInfo->neighborIPAddr = src;
		tempNeighborInfo->neighborPriority = hello->RtrPri;
	}
	else
	{
		nbrPrevPriority = tempNeighborInfo->neighborPriority;
		tempNeighborInfo->neighborPriority = hello->RtrPri;
	}

	// Handle neighbor event
	ospf_event_set_and_call(OSPF_HelloReceived,
							tempNeighborInfo);

	// Check whether this router itself appear in the
	// list of neighbor contained in Hello packet.
	if (is_ip_present_in_hello(hello, ospf->routerId))
	{
		print_ospf_log(OSPF_HELLO_LOG, "My ip is present in neighbor list. Setting 2-way received event");
		ospf_event_set_and_call(OSPF_2WayReceived, tempNeighborInfo);
		routerIdInHelloPktBody = true;
	}
	else
	{
		print_ospf_log(OSPF_HELLO_LOG, "My ip is not present in neighbor list. Set 1-way event & terminating further processing");
		ospf_event_set_and_call(OSPF_1Way, tempNeighborInfo);
		goto TERMINATE_PROCESSING_HELLO;
	}

	// If a change in the neighbor's Router Priority field was noted,
	// the receiving interface's state machine is scheduled with
	// the event NeighborChange.
	if (nbrPrevPriority != tempNeighborInfo->neighborPriority)
	{
		print_ospf_log(OSPF_HELLO_LOG, "Neighbor priority is changed. Adding neighbor change event");
		ospf_event_add(pstruEventDetails->dEventTime,
					   d,
					   in,
					   OSPF_NEIGHBORCHANGE,
					   NULL,
					   tempNeighborInfo);
	}

	// If the neighbor is both declaring itself to be Designated Router and
	// the Backup Designated Router field in the packet is equal to 0.0.0.0
	// and the receiving interface is in state Waiting, the receiving
	// interface's state machine is scheduled with the event BackupSeen.
	// Otherwise, if the neighbor is declaring itself to be Designated Router
	// and it had not previously, or the neighbor is not declaring itself
	// Designated Router where it had previously, the receiving interface's
	// state machine is scheduled with the event NeighborChange.
	if (!IP_COMPARE(hello->DesignatedRouter, tempNeighborInfo->neighborIPAddr) &&
		!hello->BackupDesignatedRouter->int_ip[0] &&
		thisIntf->State == OSPFIFSTATE_WAITING)
	{
		print_ospf_log(OSPF_HELLO_LOG, "Neighbor is declaring itself as DR and BDR is NULL");
		ospf_event_add(pstruEventDetails->dEventTime,
					   d,
					   in,
					   OSPF_BACKUPSEEN,
					   NULL,
					   tempNeighborInfo);
	}
	else if ((!IP_COMPARE(hello->DesignatedRouter, tempNeighborInfo->neighborIPAddr) &&
			  IP_COMPARE(nbrPrevDR, tempNeighborInfo->neighborIPAddr)) ||
			  (IP_COMPARE(hello->DesignatedRouter, tempNeighborInfo->neighborIPAddr) &&
			   !IP_COMPARE(nbrPrevDR, tempNeighborInfo->neighborIPAddr)))
	{
		ospf_event_add(pstruEventDetails->dEventTime,
					   d,
					   in,
					   OSPF_NEIGHBORCHANGE,
					   NULL,
					   tempNeighborInfo);
	}


	// If the neighbor is declaring itself to be Backup Designated Router and
	// the receiving interface is in state Waiting, the receiving interface's
	// state machine is scheduled with the event BackupSeen. Otherwise, if
	// neighbor is declaring itself to be Backup Designated Router and it had
	// not previously, or the neighbor is not declaring itself Backup
	// Designated Router where it had previously, the receiving interface's
	// state machine is scheduled with the event NeighborChange.
	if (hello->BackupDesignatedRouter->int_ip[0] ==
		tempNeighborInfo->neighborIPAddr->int_ip[0] &&
		thisIntf->State == OSPFIFSTATE_WAITING)
	{
		ospf_event_add(pstruEventDetails->dEventTime,
					   d,
					   in,
					   OSPF_BACKUPSEEN,
					   NULL,
					   tempNeighborInfo);
	}
	else if ((hello->BackupDesignatedRouter->int_ip[0] == tempNeighborInfo->neighborIPAddr->int_ip[0] &&
			  nbrPrevBDR->int_ip[0] != tempNeighborInfo->neighborIPAddr->int_ip[0]) ||
			  (hello->BackupDesignatedRouter->int_ip[0] != tempNeighborInfo->neighborIPAddr->int_ip[0] &&
			   nbrPrevBDR->int_ip[0] == tempNeighborInfo->neighborIPAddr->int_ip[0]))
	{
		ospf_event_add(pstruEventDetails->dEventTime,
					   d,
					   in,
					   OSPF_NEIGHBORCHANGE,
					   NULL,
					   tempNeighborInfo);
	}

TERMINATE_PROCESSING_HELLO:
	fn_NetSim_Packet_FreePacket(packet);
	pstruEventDetails->pPacket = NULL;
	print_ospf_log(OSPF_HELLO_LOG, "\n\n");
}

