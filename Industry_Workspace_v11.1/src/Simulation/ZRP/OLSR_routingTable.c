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

int fn_NetSim_OLSR_UpdateRoutingTable()
{
	//Section 10
	NODE_OLSR* olsr=GetOLSRData(pstruEventDetails->nDeviceId);
	NETSIM_IPAddress subnet = STR_TO_IP4("255.255.255.255");
	OLSR_NEIGHBOR_SET* neighbor;
	OLSR_2HOP_NEIGHBOR_SET* two_hop_neighbor;
	OLSR_TOPOLOGY_INFORMATION_BASE* topology;
	if(!olsr->bRoutingTableUpdate)
	{
		return 0; //No update
	}
	//Condition 1
	olsrFlushroutingTable(olsr->ipWrapper,pstruEventDetails->nDeviceId);
	olsr->ipWrapper->table = NULL;

	//Condition 2
	neighbor=olsr->neighborSet;
	while(neighbor)
	{
		if(neighbor->N_status>=SYM_NEIGH)
		{
			NETSIM_ID interfaceId = 1;
			iptable_add(olsr->ipWrapper,
						neighbor->N_neighbor_main_addr,
						subnet,
						0,
						neighbor->N_neighbor_main_addr,
						1,
						&olsr->mainAddress,
						&interfaceId,
						1,
						"OLSR");

		}
		neighbor=(OLSR_NEIGHBOR_SET*)LIST_NEXT(neighbor);
	}

	//Condition 3
	two_hop_neighbor=olsr->twoHopNeighborSet;
	while(two_hop_neighbor)
	{
		neighbor=olsrFindNeighborSet(olsr->neighborSet,two_hop_neighbor->N_2hop_addr);
		if(!neighbor && IP_COMPARE(two_hop_neighbor->N_2hop_addr,olsr->mainAddress))
		{
			neighbor=olsrFindNeighborSet(olsr->neighborSet,two_hop_neighbor->N_neighbor_main_addr);
			if(neighbor->N_willingness != WILL_NEVER && neighbor->N_status==MPR_NEIGH)
			{
				NETSIM_ID interfaceId = 1;
				iptable_add(olsr->ipWrapper,
							two_hop_neighbor->N_2hop_addr,
							subnet,
							0,
							two_hop_neighbor->N_neighbor_main_addr,
							1,
							&olsr->mainAddress,
							&interfaceId,
							2,
							"OLSR");
			}
		}
		two_hop_neighbor=(OLSR_2HOP_NEIGHBOR_SET*)LIST_NEXT(two_hop_neighbor);
	}

	//Condition 3
	topology=olsr->topologyInfoBase;
	while(topology)
	{
		ptrIP_ROUTINGTABLE table=iptable_check(&olsr->ipWrapper->table,topology->T_dest_addr,subnet);
		if(!table && IP_COMPARE(topology->T_dest_addr,olsr->mainAddress))
		{
			table=iptable_check(&olsr->ipWrapper->table,topology->T_last_addr,subnet);
			if(table && table->Metric < olsr->nZoneRadius)
			{
				NETSIM_ID interfaceId;
				iptable_add(olsr->ipWrapper,
							topology->T_dest_addr,
							subnet,
							0,
							table->gateway,
							1,
							&olsr->mainAddress,
							&interfaceId,
							table->Metric + 1,
							"OLSR");
				topology=olsr->topologyInfoBase;
				continue;
			}
		}
		topology=(OLSR_TOPOLOGY_INFORMATION_BASE*)LIST_NEXT(topology);
	}
	olsrUpdateIptable(olsr->ipWrapper->table,pstruEventDetails->nDeviceId);
	olsr->bRoutingTableUpdate=false;
	if(DEVICE_NWLAYER(pstruEventDetails->nDeviceId)->nRoutingProtocolId == NW_PROTOCOL_ZRP)
		fn_NetSim_BRP_Update(olsr->ipWrapper->table);
	return 0;
}
int olsrUpdateIptable(ptrIP_ROUTINGTABLE table,NETSIM_ID nNodeId)
{
	while(table)
	{
		iptable_add(IP_WRAPPER_GET(nNodeId),
					table->networkDestination,
					table->netMask,
					table->prefix_len,
					table->gateway,
					1,
					table->Interface,
					table->nInterfaceId,
					table->Metric,
					"OLSR");
		
		table=(ptrIP_ROUTINGTABLE)LIST_NEXT(table);
	}
	return 0;
}
int olsrFlushroutingTable(ptrIP_WRAPPER wrapper,NETSIM_ID nNodeId)
{
	ptrIP_ROUTINGTABLE iptable = wrapper->table;
	while(iptable)
	{
		iptable_delete(IP_WRAPPER_GET(nNodeId), iptable->networkDestination, NULL);
		iptable_delete(wrapper, iptable->networkDestination, NULL);
		iptable = wrapper->table;
	}
	return 0;
}