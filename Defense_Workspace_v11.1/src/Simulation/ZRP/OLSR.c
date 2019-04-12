/************************************************************************************
 * Copyright (C) 2014                                                               *
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
#include "ZRP.h"
#include "ZRP_Enum.h"


int fn_NetSim_OLSR_PopulateLinkSet()
{
	bool expiry=false;
	bool flag=false;
	NODE_OLSR* olsr=GetOLSRData(pstruEventDetails->nDeviceId);
	NetSim_PACKET* packet=pstruEventDetails->pPacket;
	OLSR_HEADER* header=(OLSR_HEADER*)packet->pstruNetworkData->Packet_RoutingProtocol;
	OLSR_HELLO_PACKET* hello=(OLSR_HELLO_PACKET*)header->message->MESSAGE;
	OLSR_HELLO_LINK* linkInfo=hello->link;
	
	NETSIM_IPAddress src=packet->pstruNetworkData->szSourceIP;
	OLSR_LINK_SET* link = olsrFindLinkSet(olsr,src);
	double vTime = olsrConvertMEToDouble(header->message->Vtime)*SECOND;

	if(!link)
	{
		link = LINK_SET_ALLOC();
		link->L_neighbor_iface_addr = IP_COPY(src);
		link->L_local_iface_addr = IP_COPY(DEVICE_NWADDRESS(pstruEventDetails->nDeviceId,1));
		link->L_SYM_time = pstruEventDetails->dEventTime -1;
		link->L_time = pstruEventDetails->dEventTime + vTime;
		LIST_ADD_LAST((void**)&olsr->linkSet,link);
		olsr->bRoutingTableUpdate=true;
		expiry=true;
	}
	link->L_ASYM_time = pstruEventDetails->dEventTime + vTime;
	while(linkInfo)
	{
		if(!IP_COMPARE(linkInfo->NeighborInterfaceAddress,DEVICE_NWADDRESS(pstruEventDetails->nDeviceId,1)))
		{
			flag=true;
			break;
		}
		linkInfo = linkInfo->next;
	}
	if(flag)
	{
		if(linkInfo->LinkCode.linkType == LOST_LINK)
			link->L_SYM_time = pstruEventDetails->dEventTime-1;
		else if(linkInfo->LinkCode.linkType == ASYM_LINK || linkInfo->LinkCode.linkType == SYM_LINK)
		{
			link->L_SYM_time = pstruEventDetails->dEventTime + vTime;
			link->L_time = pstruEventDetails->dEventTime + NEIGHB_HOLD_TIME;
		}
	}
	link->L_time = max(link->L_time,link->L_ASYM_time);
	if(expiry)
		olsrAddLinktuplesExpiryEvent(link);
	return 0;
}

int fn_NetSim_OLSR_PopulateNeighborSet()
{
	//Section 8.1 RFC 3626
	NODE_OLSR* olsr=GetOLSRData(pstruEventDetails->nDeviceId);
	OLSR_LINK_SET* linkSet=olsr->linkSet;
	OLSR_NEIGHBOR_SET* neighborSet=olsr->neighborSet;
	NetSim_PACKET* packet=pstruEventDetails->pPacket;
	OLSR_HEADER* header=(OLSR_HEADER*)packet->pstruNetworkData->Packet_RoutingProtocol;
	OLSR_HELLO_PACKET* hello=(OLSR_HELLO_PACKET*)header->message->MESSAGE;
	OLSR_HELLO_LINK* linkInfo=hello->link;
	while(linkSet)
	{
		OLSR_NEIGHBOR_SET* neighbor;
		neighbor = olsrFindNeighborSet(olsr->neighborSet,linkSet->L_neighbor_iface_addr);
		if(!neighbor)
		{
//Create Neighbor set
			neighbor=(OLSR_NEIGHBOR_SET*)NEIGHBOR_TUPLES_ALLOC();
			neighbor->N_willingness=WILL_DEFAULT;
			neighbor->N_neighbor_main_addr = IP_COPY(linkSet->L_neighbor_iface_addr);
			LIST_ADD_LAST((void**)&olsr->neighborSet,neighbor);
			olsr->bRoutingTableUpdate=true;
		}
//Update Neighbor Set
		if(linkSet->L_SYM_time >= pstruEventDetails->dEventTime)
		{
			neighbor->N_status = SYM;
			olsr->bRoutingTableUpdate=true;
		}
		else
			neighbor->N_status = NOT_SYM;
		linkSet=(OLSR_LINK_SET*)LIST_NEXT(linkSet);
	}
	neighborSet=olsr->neighborSet;
//Remove neighbor set
	while(neighborSet)
	{
		OLSR_LINK_SET* link = olsrFindLinkSet(olsr,neighborSet->N_neighbor_main_addr);
		if(!link)
		{
			//Free the neighbor
			LIST_FREE((void**)olsr->neighborSet,neighborSet);
		}
		neighborSet = (OLSR_NEIGHBOR_SET*)LIST_NEXT(neighborSet);
	}
//Section 8.1.1 RFC 3626
	while(linkInfo)
	{
		OLSR_NEIGHBOR_SET* neighbor=olsrFindNeighborSet(olsr->neighborSet,linkInfo->NeighborInterfaceAddress);
		if(neighbor)
		{
			neighbor->N_willingness = hello->Willingness;
		}
		linkInfo = linkInfo->next;
	}
	return 0;
}

int fn_NetSim_OLSR_Populate2HopNeighbor()
{
	//Section 8.2 RFC 3626
	NODE_OLSR* olsr=GetOLSRData(pstruEventDetails->nDeviceId);
	OLSR_LINK_SET* linkSet;
	NetSim_PACKET* packet=pstruEventDetails->pPacket;
	OLSR_HEADER* header=(OLSR_HEADER*)packet->pstruNetworkData->Packet_RoutingProtocol;
	OLSR_HELLO_PACKET* hello=(OLSR_HELLO_PACKET*)header->message->MESSAGE;
	OLSR_HELLO_LINK* linkInfo=hello->link;
	double validityTime = olsrConvertMEToDouble(header->message->Vtime)*SECOND;
	linkSet=olsrFindLinkSet(olsr,header->message->OriginatorAddress);
	if(linkSet && linkSet->L_SYM_time >= pstruEventDetails->dEventTime)
	{
		while(linkInfo)
		{
			if(linkInfo->LinkCode.neighTypes == SYM_NEIGH ||
				linkInfo->LinkCode.neighTypes == MPR_NEIGH)
			{
				if(!IP_COMPARE(linkInfo->NeighborInterfaceAddress,olsr->mainAddress))
				{
					//Discard
				}
				else
				{
					bool flag=false;
					OLSR_2HOP_NEIGHBOR_SET* neighbor=olsrFind2HopNeighbor(olsr,header->message->OriginatorAddress,linkInfo->NeighborInterfaceAddress);
					if(!neighbor)
					{
						neighbor=(OLSR_2HOP_NEIGHBOR_SET*)OLSR_2HOPTUPLES_ALLOC();
						LIST_ADD_LAST((void**)&olsr->twoHopNeighborSet,neighbor);
						olsr->bRoutingTableUpdate=true;
						flag=true;
					}
					neighbor->N_neighbor_main_addr = IP_COPY(header->message->OriginatorAddress);
					neighbor->N_2hop_addr = IP_COPY(linkInfo->NeighborInterfaceAddress);
					neighbor->N_time = pstruEventDetails->dEventTime + validityTime;
				}
			}
			else if(linkInfo->LinkCode.neighTypes == NOT_NEIGH)
			{
				OLSR_2HOP_NEIGHBOR_SET* neighbor=olsrFind2HopNeighbor(olsr,header->message->OriginatorAddress,linkInfo->NeighborInterfaceAddress);
				if(neighbor)
				{
					LIST_FREE((void**)&olsr->twoHopNeighborSet,neighbor);
				}
			}
			else
			{
				fnNetSimError("Unknown neighbor type %d in populate 2 hop neighbor set.\n",linkInfo->LinkCode.neighTypes);
			}
			linkInfo=linkInfo->next;
		}
	}
	return 0;
}
struct stru_Degree
{
	OLSR_NEIGHBOR_SET* y;
	unsigned int degree;
	struct stru_Degree* next;
};

unsigned int getD(struct stru_Degree* D,OLSR_NEIGHBOR_SET* y)
{
	while(D)
	{
		if(D->y==y)
			return D->degree;
		D=D->next;
	}
	return 0;
}
struct stru_Degree* fnCalculateDegree(OLSR_NEIGHBOR_SET* N,OLSR_2HOP_NEIGHBOR_SET* N2)
{
	struct stru_Degree* D=NULL,*temp,*ret=NULL;
	OLSR_NEIGHBOR_SET* i;
	for(i=N;i;i=(OLSR_NEIGHBOR_SET*)LIST_NEXT(i))
	{
		OLSR_2HOP_NEIGHBOR_SET* j;
		temp=(struct stru_Degree*)calloc(1,sizeof* temp);
		temp->y = i;
		for(j=N2;j;j=(OLSR_2HOP_NEIGHBOR_SET*)LIST_NEXT(j))
		{
			if(!IP_COMPARE(i->N_neighbor_main_addr,j->N_neighbor_main_addr))
				temp->degree++;
		}
		if(D)
			D->next=temp;
		else
			ret=temp;
		D=temp;
	}
	return ret;
}
int fnFlushN2(OLSR_2HOP_NEIGHBOR_SET** N2,NETSIM_IPAddress two_hop_ip)
{
	OLSR_2HOP_NEIGHBOR_SET* i;
	i=*N2;
	while(i)
	{
		if(!IP_COMPARE(i->N_2hop_addr,two_hop_ip))
		{
			LIST_FREE((void**)N2,i);
			i=*N2;
			continue;
		}
		i=(OLSR_2HOP_NEIGHBOR_SET*)LIST_NEXT(i);
	}
	return 0;
}
int fnFlush2HopSetBasedonMPR(OLSR_2HOP_NEIGHBOR_SET** N2,OLSR_MPR_SET* mprSet)
{
	OLSR_2HOP_NEIGHBOR_SET* i;
	OLSR_MPR_SET* j;
	for(j=mprSet;j;j=(OLSR_MPR_SET*)LIST_NEXT(j))
	{
		i=*N2;
		while(i)
		{
			if(!IP_COMPARE(j->neighborAddress,i->N_neighbor_main_addr))
			{
				fnFlushN2(N2,i->N_2hop_addr);
				i=*N2;
				continue;
			}
			i=(OLSR_2HOP_NEIGHBOR_SET*)LIST_NEXT(i);
		}
	}
	return 0;
}


/**
MPR set of an interface

			  a (sub)set of the neighbors of an interface with a
			  willingness different from WILL_NEVER, selected such that
			  through these selected nodes, all strict 2-hop neighbors
			  reachable from that interface are reachable.

	   N:
			  N is the subset of neighbors of the node, which are
			  neighbor of the interface I.

	   N2:
			  The set of 2-hop neighbors reachable from the interface
			  I, excluding:

			   (i)   the nodes only reachable by members of N with
					 willingness WILL_NEVER

			   (ii)  the node performing the computation

			   (iii) all the symmetric neighbors: the nodes for which
					 there exists a symmetric link to this node on some
					 interface.

	D(y):
			  The degree of a 1-hop neighbor node y (where y is a
			  member of N), is defined as the number of symmetric
			  neighbors of node y, EXCLUDING all the members of N and
			  EXCLUDING the node performing the computation.

   The proposed heuristic is as follows:

	 1    Start with an MPR set made of all members of N with
		  N_willingness equal to WILL_ALWAYS

	 2    Calculate D(y), where y is a member of N, for all nodes in N.

	 3    Add to the MPR set those nodes in N, which are the *only*
		  nodes to provide reachability to a node in N2.  For example,
		  if node b in N2 can be reached only through a symmetric link
		  to node a in N, then add node a to the MPR set.  Remove the
		  nodes from N2 which are now covered by a node in the MPR set.

	 4    While there exist nodes in N2 which are not covered by at
		  least one node in the MPR set:

		  4.1  For each node in N, calculate the reachability, i.e., the
			   number of nodes in N2 which are not yet covered by at
			   least one node in the MPR set, and which are reachable
			   through this 1-hop neighbor;

		  4.2  Select as a MPR the node with highest N_willingness among
			   the nodes in N with non-zero reachability.  In case of
			   multiple choice select the node which provides
			   reachability to the maximum number of nodes in N2.  In
			   case of multiple nodes providing the same amount of
			   reachability, select the node as MPR whose D(y) is
			   greater.  Remove the nodes from N2 which are now covered
			   by a node in the MPR set.

	 5    A node's MPR set is generated from the union of the MPR sets
		  for each interface.  As an optimization, process each node, y,
		  in the MPR set in increasing order of N_willingness.  If all
		  nodes in N2 are still covered by at least one node in the MPR
		  set excluding node y, and if N_willingness of node y is
		  smaller than WILL_ALWAYS, then node y MAY be removed from the
		  MPR set.
*/
int fn_NetSim_OLSR_PopulateMPRSet()
 {
	//Section 8.3 RFC 3626
	NODE_OLSR* olsr=GetOLSRData(pstruEventDetails->nDeviceId);
	OLSR_NEIGHBOR_SET* N = olsrFillNeighbor(olsr);
	OLSR_MPR_SET* mprSet=NULL;
	OLSR_2HOP_NEIGHBOR_SET* N2 = olsrFill2HopNeighbor(olsr);
	struct stru_Degree *D=fnCalculateDegree(N,N2);
	OLSR_2HOP_NEIGHBOR_SET* i=N2;
	while(i)
	{
		OLSR_2HOP_NEIGHBOR_SET* j;
		bool flag=true;
		for(j=N2;j;j=(OLSR_2HOP_NEIGHBOR_SET*)LIST_NEXT(j))
		{
			if(i!=j)
			{
				if(!IP_COMPARE(i->N_2hop_addr,j->N_2hop_addr))
				{
					flag=false;
					break;
				}
			}
		}
		if(flag)
		{
			olsrAddtoMPRSet(&mprSet,N,i);
			fnFlush2HopSetBasedonMPR(&N2,mprSet);
			i=N2;
			olsr->bRoutingTableUpdate=true;
			continue;
		}
		i=(OLSR_2HOP_NEIGHBOR_SET*)LIST_NEXT(i);
	}
	if(N2)
	{
		i=N2;
		while(i)
		{
			OLSR_2HOP_NEIGHBOR_SET* j;
			OLSR_NEIGHBOR_SET* current=olsrFindNeighborSet(N,i->N_neighbor_main_addr);
			OLSR_NEIGHBOR_SET* temp;
			unsigned int degree = getD(D,current);

			for(j=N2;j;j=(OLSR_2HOP_NEIGHBOR_SET*)LIST_NEXT(j))
			{
				if(i!=j)
				{
					if(!IP_COMPARE(i->N_2hop_addr,j->N_2hop_addr))
					{
						temp=olsrFindNeighborSet(N,j->N_neighbor_main_addr);
						if(temp->N_willingness > current->N_willingness)
						{
							current=temp;
							degree=getD(D,current);
						}
						else if(temp->N_willingness == current->N_willingness)
						{
							if(getD(D,temp) > degree)
							{
								current = temp;
								degree = getD(D,current);
							}
						}
						else
						{
							//do nothing
						}
					}
				}
			}
			olsrAddtoMPRSet(&mprSet,N,i);
			fnFlush2HopSetBasedonMPR(&N2,mprSet);
			i=N2;
			olsr->bRoutingTableUpdate=true;
		}
	}
	while(olsr->mprSet)
		LIST_FREE((void**)&olsr->mprSet,olsr->mprSet);
	olsr->mprSet=mprSet;
	olsrMarkMPR(olsr);

	if(mprSet)
		olsrPrintMPR(mprSet);
	while(N)
		LIST_FREE((void**)&N,N);
	while(D)
	{
		struct stru_Degree* degree=D;
		D=D->next;
		free(degree);
	}
	return 0;
}

int fn_NetSim_OLSR_PopulateMPRSelectorSet()
{
	//Section 8.4.1 RFC 3626
	NODE_OLSR* olsr=GetOLSRData(pstruEventDetails->nDeviceId);
	NetSim_PACKET* packet=pstruEventDetails->pPacket;
	OLSR_HEADER* header=(OLSR_HEADER*)packet->pstruNetworkData->Packet_RoutingProtocol;
	OLSR_HELLO_PACKET* hello=(OLSR_HELLO_PACKET*)header->message->MESSAGE;
	OLSR_HELLO_LINK* linkInfo=hello->link;
	double validityTime=olsrConvertMEToDouble(header->message->Vtime);
	while(linkInfo)
	{
		if(linkInfo->LinkCode.neighTypes==MPR_NEIGH)
		{
			OLSR_MPR_SELECTION_SET* set = olsrFindMPRSelectorSet(olsr->mprSelectionSet,header->message->OriginatorAddress);
			if(!set)
			{
				set=MPR_SELECTION_SET_ALLOC();
				LIST_ADD_LAST((void**)&olsr->mprSelectionSet,set);
				set->MS_main_addr = IP_COPY(header->message->OriginatorAddress);
			}
			set->MS_time = pstruEventDetails->dEventTime + validityTime;
		}
		linkInfo = linkInfo->next;
	}
	return 0;
}

OLSR_NEIGHBOR_SET* olsrFindNeighborSet(OLSR_NEIGHBOR_SET* neighborSet,NETSIM_IPAddress ip)
{
	OLSR_NEIGHBOR_SET* set =neighborSet;
	while(set)
	{
		if(!IP_COMPARE(set->N_neighbor_main_addr,ip))
			return set;
		set = (OLSR_NEIGHBOR_SET*)LIST_NEXT(set);
	}
	return NULL;
}
OLSR_LINK_SET* olsrFindLinkSet(NODE_OLSR* olsr,NETSIM_IPAddress ip)
{
	OLSR_LINK_SET* linkSet = olsr->linkSet;
	while(linkSet)
	{
		if(!IP_COMPARE(linkSet->L_neighbor_iface_addr,ip))
			return linkSet;
		linkSet = (OLSR_LINK_SET*)LIST_NEXT(linkSet);
	}
	return NULL;
}
OLSR_2HOP_NEIGHBOR_SET* olsrFind2HopNeighbor(NODE_OLSR* olsr,NETSIM_IPAddress neighborAddress,NETSIM_IPAddress N_2_Hop_Address)
{
	OLSR_2HOP_NEIGHBOR_SET* neighbor = olsr->twoHopNeighborSet;
	while(neighbor)
	{
		if(!IP_COMPARE(neighbor->N_neighbor_main_addr,neighborAddress) &&
			!IP_COMPARE(neighbor->N_2hop_addr,N_2_Hop_Address))
			return neighbor;
		neighbor=(OLSR_2HOP_NEIGHBOR_SET*)LIST_NEXT(neighbor);
	}
	return NULL;
}
OLSR_NEIGHBOR_SET* olsrFillNeighbor(NODE_OLSR* olsr)
{
	OLSR_NEIGHBOR_SET* ret=NULL;
	OLSR_NEIGHBOR_SET* neighbor = olsr->neighborSet;
	while(neighbor)
	{
		if(neighbor->N_willingness != WILL_NEVER)
		{
			OLSR_NEIGHBOR_SET* temp=(OLSR_NEIGHBOR_SET*)NEIGHBOR_TUPLES_ALLOC();
			temp->N_neighbor_main_addr = IP_COPY(neighbor->N_neighbor_main_addr);
			temp->N_status = neighbor->N_status;
			temp->N_willingness = neighbor->N_willingness;
			LIST_ADD_LAST((void**)&ret,temp);
		}
		neighbor = (OLSR_NEIGHBOR_SET*)LIST_NEXT(neighbor);
	}
	return ret;
}
OLSR_2HOP_NEIGHBOR_SET* olsrFill2HopNeighbor(NODE_OLSR* olsr)
{
	OLSR_2HOP_NEIGHBOR_SET* ret=NULL;
	OLSR_2HOP_NEIGHBOR_SET* two_hop_neighbor;
	for(two_hop_neighbor = olsr->twoHopNeighborSet;two_hop_neighbor;two_hop_neighbor = (OLSR_2HOP_NEIGHBOR_SET*)LIST_NEXT(two_hop_neighbor))
	{
		OLSR_NEIGHBOR_SET* neighbor = olsrFindNeighborSet(olsr->neighborSet,two_hop_neighbor->N_neighbor_main_addr);
		if(neighbor->N_willingness != WILL_NEVER) // Condition (i)
		{
			OLSR_NEIGHBOR_SET* temp = olsrFindNeighborSet(olsr->neighborSet,two_hop_neighbor->N_2hop_addr);
			if(temp)
			{
				continue; // Condition (ii)
			}
			else
			{
				OLSR_LINK_SET* link = olsrFindLinkSet(olsr,two_hop_neighbor->N_2hop_addr);
				if(link && link->L_SYM_time >= pstruEventDetails->dEventTime)
				{
					continue; // condition (iii)
				}
				else
				{
					OLSR_2HOP_NEIGHBOR_SET* temp=(OLSR_2HOP_NEIGHBOR_SET*)OLSR_2HOPTUPLES_ALLOC();
					temp->N_2hop_addr = IP_COPY(two_hop_neighbor->N_2hop_addr);
					temp->N_neighbor_main_addr = IP_COPY(two_hop_neighbor->N_neighbor_main_addr);
					temp->N_time = two_hop_neighbor->N_time;
					LIST_ADD_LAST((void**)&ret,temp);
				}
			}
		}
	}
	 return ret;
}
int olsrAddtoMPRSet(OLSR_MPR_SET** mpr,OLSR_NEIGHBOR_SET* N,OLSR_2HOP_NEIGHBOR_SET* N2)
{
	OLSR_NEIGHBOR_SET* i;
	for(i=N;i;i=(OLSR_NEIGHBOR_SET*)LIST_NEXT(i))
	{
		if(!IP_COMPARE(i->N_neighbor_main_addr,N2->N_neighbor_main_addr))
		{
			OLSR_MPR_SET* set=MPRSET_ALLOC();
			set->neighborAddress=i->N_neighbor_main_addr;
			LIST_ADD_LAST((void**)mpr,set);
			break;
		}
	}
	return 0;
}
int olsrMarkMPR(NODE_OLSR* olsr)
{
	OLSR_MPR_SET* mpr = olsr->mprSet;
	OLSR_NEIGHBOR_SET* neighbor = olsr->neighborSet;
	while(mpr)
	{
		OLSR_NEIGHBOR_SET* N=olsrFindNeighborSet(neighbor,mpr->neighborAddress);
		if(N)
			N->N_status=MPR_NEIGH;
		mpr=(OLSR_MPR_SET*)LIST_NEXT(mpr);
	}
	return 0;
}
OLSR_MPR_SELECTION_SET* olsrFindMPRSelectorSet(OLSR_MPR_SELECTION_SET* set,NETSIM_IPAddress ip)
{
	while(set)
	{
		if(!IP_COMPARE(set->MS_main_addr,ip))
			return set;
		set=(OLSR_MPR_SELECTION_SET*)LIST_NEXT(set);
	}
	return NULL;
}
int olsrAddLinktuplesExpiryEvent(OLSR_LINK_SET* link)
{
	NetSim_EVENTDETAILS pevent;
	pevent.dEventTime=link->L_time;
	pevent.dPacketSize=0;
	pevent.nApplicationId=0;
	pevent.nDeviceId=pstruEventDetails->nDeviceId;
	pevent.nDeviceType=pstruEventDetails->nDeviceType;
	pevent.nEventType=TIMER_EVENT;
	pevent.nInterfaceId=pstruEventDetails->nInterfaceId;
	pevent.nPacketId=0;
	pevent.nProtocolId=NW_PROTOCOL_OLSR;
	pevent.nSegmentId=0;
	pevent.nSubEventType=OLSR_LINK_TUPLE_Expire;
	pevent.pPacket=NULL;
	pevent.szOtherDetails=link;
	fnpAddEvent(&pevent);
	return 0;
}
int olsrPrintMPR(OLSR_MPR_SET* mprSet)
{
	printf("\nMpr Set of node %d at time %lf--- ",pstruEventDetails->nDeviceId,pstruEventDetails->dEventTime);
	while(mprSet)
	{
		char ip[_NETSIM_IP_LEN];
		IP_TO_STR(mprSet->neighborAddress,ip);
		fprintf(stdout,"%s, ",ip);
		mprSet=(OLSR_MPR_SET*)LIST_NEXT(mprSet);
	}
	fprintf(stdout,"\n");
	return 0;
}
int fn_NetSim_OLSR_PacketProcessing()
{
	//Section 3.4
	NODE_OLSR* olsr=GetOLSRData(pstruEventDetails->nDeviceId);
	OLSR_HEADER* header=(OLSR_HEADER*)pstruEventDetails->pPacket->pstruNetworkData->Packet_RoutingProtocol;
	OLSR_HEADER_MESSAGE* hdrMessage=header->message;
	if(hdrMessage->MESSAGE==NULL) //Condition 1
	{
		fn_NetSim_Packet_FreePacket(pstruEventDetails->pPacket);
		pstruEventDetails->pPacket=NULL;
		return -1;
	}
	//Condition 2
	if(hdrMessage->TimeToLive == 0)
	{
		fn_NetSim_Packet_FreePacket(pstruEventDetails->pPacket);
		pstruEventDetails->pPacket=NULL;
		return -2;
	}
	if(!IP_COMPARE(hdrMessage->OriginatorAddress,olsr->mainAddress))
	{
		fn_NetSim_Packet_FreePacket(pstruEventDetails->pPacket);
		pstruEventDetails->pPacket=NULL;
		return -2;
	}
	//Condition 3
	if(olsrExistInDuplicateSet(olsr->duplicateSet,header))
	{
		fn_NetSim_Packet_FreePacket(pstruEventDetails->pPacket);
		pstruEventDetails->pPacket=NULL;
		return -2;
	}
	switch(hdrMessage->MessageType)
	{
	case TC_MESSAGE:
		fn_NetSim_OLSR_ReceiveTC();
		fn_NetSim_OLSR_UpdateRoutingTable();
		break;
	default:
		fnNetSimError("Unknown message type %d for OLSR protocol in packet processing function\n",hdrMessage->MessageType);
		break;
	}
	return 0;
}
bool olsrExistInDuplicateSet(OLSR_DUPLICATE_SET* set,OLSR_HEADER* header)
{
	while(set)
	{
		if(!IP_COMPARE(set->D_addr,header->message->OriginatorAddress) &&
			set->D_seq_num == header->message->MessageSequenceNumber)
			return true;
		set=(OLSR_DUPLICATE_SET*)LIST_NEXT(set);
	}
	return false;
}
int fn_NetSim_OLSR_PacketForwarding()
{
	//Section 3.4.1
	bool forwardFlag=false;
	NetSim_PACKET* packet = pstruEventDetails->pPacket;
	OLSR_HEADER* header = (OLSR_HEADER*)packet->pstruNetworkData->Packet_RoutingProtocol;
	NODE_OLSR* olsr=GetOLSRData(pstruEventDetails->nDeviceId);
	OLSR_NEIGHBOR_SET* neighbor;
	OLSR_DUPLICATE_SET* duplicate;
	OLSR_MPR_SELECTION_SET* mpr;
	NetSim_PACKET* retransmitted;
	
	//Condition 1
	neighbor = olsrFindNeighborSet(olsr->neighborSet,packet->pstruNetworkData->szGatewayIP);
	if(neighbor && neighbor->N_status<SYM_NEIGH)
		return -1;

	//Condition 2,3
	duplicate=olsrFindDuplicateSet(olsr->duplicateSet,header->message->OriginatorAddress,header->message->MessageSequenceNumber);
	if(duplicate &&  (duplicate->D_retransmitted==true || 
		!IP_COMPARE(olsr->mainAddress,duplicate->D_iface_list)))
	{
		return -2;
	}

	//condition 4
	mpr=olsrFindMPRSelectorSet(olsr->mprSelectionSet,packet->pstruNetworkData->szGatewayIP);
	if(mpr && header->message->TimeToLive > 1)
		forwardFlag=true;

	//condition 5
	if(duplicate)
	{
		duplicate->D_time=pstruEventDetails->dEventTime+DUP_HOLD_TIME;
		duplicate->D_iface_list=IP_COPY(olsr->mainAddress);
		if(forwardFlag)
			duplicate->D_retransmitted=true;
	}
	else
	{
		duplicate=DUPLICATE_SET_ALLOC();
		duplicate->D_addr=IP_COPY(header->message->OriginatorAddress);
		duplicate->D_seq_num=header->message->MessageSequenceNumber;
		duplicate->D_iface_list=IP_COPY(olsr->mainAddress);
		duplicate->D_time=pstruEventDetails->dEventTime+DUP_HOLD_TIME;
		if(forwardFlag)
			duplicate->D_retransmitted=true;
		LIST_ADD_LAST((void**)&olsr->duplicateSet,duplicate);
	}

	retransmitted=fn_NetSim_Packet_CopyPacket(packet);
	if(forwardFlag)
	{
		OLSR_HEADER* reheader=(OLSR_HEADER*)retransmitted->pstruNetworkData->Packet_RoutingProtocol;
		retransmitted->pstruNetworkData->nTTL--;
		reheader->message->TimeToLive--;
		reheader->message->HopCount++;
	}
	retransmitted->pstruNetworkData->szGatewayIP=IP_COPY(olsr->mainAddress);
	retransmitted->pstruNetworkData->szNextHopIp=NULL;
	pstruEventDetails->dPacketSize=retransmitted->pstruNetworkData->dPacketSize;
	pstruEventDetails->nApplicationId=0;
	pstruEventDetails->nEventType=NETWORK_OUT_EVENT;
	pstruEventDetails->nProtocolId=NW_PROTOCOL_IPV4;
	pstruEventDetails->nSegmentId=0;
	pstruEventDetails->nSubEventType=0;
	pstruEventDetails->pPacket=retransmitted;
	pstruEventDetails->szOtherDetails=NULL;
	fnpAddEvent(pstruEventDetails);

	return 0;
}

OLSR_DUPLICATE_SET* olsrFindDuplicateSet(OLSR_DUPLICATE_SET* duplicate,NETSIM_IPAddress ip,unsigned short int seq_no)
{
	while(duplicate)
	{
		if(!IP_COMPARE(duplicate->D_addr,ip) && 
			duplicate->D_seq_num == seq_no)
			return duplicate;
		duplicate=(OLSR_DUPLICATE_SET*)LIST_NEXT(duplicate);
	}
	return NULL;
}
int fn_NetSim_OLSR_LinkTupleExpire()
{
	NODE_OLSR* olsr=GetOLSRData(pstruEventDetails->nDeviceId);
	OLSR_LINK_SET* link=(OLSR_LINK_SET*)pstruEventDetails->szOtherDetails;
	if(link->L_time<=pstruEventDetails->dEventTime)
	{
		//remove neighbor tuples
		fn_NetSim_OLSR_RemoveNeighbortuple(link);
		//Link tuples expires
		LIST_FREE((void**)&olsr->linkSet,link);
		//Update the routing table
		fn_NetSim_OLSR_UpdateRoutingTable();
	}
	else
	{
		pstruEventDetails->dEventTime=link->L_time;
		fnpAddEvent(pstruEventDetails);
	}
	return 0;
}
int fn_NetSim_OLSR_RemoveNeighbortuple(OLSR_LINK_SET* link)
{
	NODE_OLSR* olsr=GetOLSRData(pstruEventDetails->nDeviceId);
	OLSR_NEIGHBOR_SET* neighbor=olsr->neighborSet;
	OLSR_LINK_SET* linkset=olsr->linkSet;
	unsigned int count=0;
	while(linkset)
	{
		if(!IP_COMPARE(linkset->L_neighbor_iface_addr,link->L_neighbor_iface_addr))
			count++;
		linkset=(OLSR_LINK_SET*)LIST_NEXT(linkset);
	}
	if(count>=2)
		return 0; //Other link tuple exists.
	while(neighbor)
	{
		if(!IP_COMPARE(link->L_neighbor_iface_addr,neighbor->N_neighbor_main_addr))
		{
			fn_NetSim_OLSR_Remove2HopTuple(olsr,neighbor);
			LIST_FREE((void**)&olsr->neighborSet,neighbor);
			neighbor=olsr->neighborSet;
			olsr->bRoutingTableUpdate=true;
			continue;
		}
		neighbor=(OLSR_NEIGHBOR_SET*)LIST_NEXT(neighbor);
	}
	return 0;
}

int fn_NetSim_OLSR_Remove2HopTuple(NODE_OLSR* olsr,OLSR_NEIGHBOR_SET* neighbor)
{
	OLSR_2HOP_NEIGHBOR_SET* two_hop_neighbor=olsr->twoHopNeighborSet;
	while(two_hop_neighbor)
	{
		if(!IP_COMPARE(two_hop_neighbor->N_neighbor_main_addr,neighbor->N_neighbor_main_addr))
		{
			LIST_FREE(&olsr->twoHopNeighborSet,two_hop_neighbor);
			two_hop_neighbor=olsr->twoHopNeighborSet;
			continue;
		}
		two_hop_neighbor=(OLSR_2HOP_NEIGHBOR_SET*)LIST_NEXT(two_hop_neighbor);
	}
	return 0;
}

int fn_NetSim_OLSR_2HopTupleExpire()
{
	NODE_OLSR* olsr=GetOLSRData(pstruEventDetails->nDeviceId);
	OLSR_2HOP_NEIGHBOR_SET* neighbor=pstruEventDetails->szOtherDetails;
	if(neighbor->N_time<=pstruEventDetails->dEventTime)
	{
		//Link tuples expires
		LIST_FREE(&olsr->twoHopNeighborSet,neighbor);
		olsr->bRoutingTableUpdate=true;
		//Update the routing table
		fn_NetSim_OLSR_UpdateRoutingTable();
	}
	else
	{
		pstruEventDetails->dEventTime=neighbor->N_time;
		fnpAddEvent(pstruEventDetails);
	}
	return 0;
}


