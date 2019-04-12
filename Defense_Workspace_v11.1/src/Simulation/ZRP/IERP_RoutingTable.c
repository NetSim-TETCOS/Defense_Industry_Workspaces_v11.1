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

int flushIerpTable(NODE_IERP* ierp)
{
	IERP_ROUTE_TABLE* table= ierp->routeTable;
	while(table)
	{
		if(!table->flag)
		{
			LIST_FREE((void**)&ierp->routeTable,table);
			table= ierp->routeTable;
			continue;
		}
		table=(IERP_ROUTE_TABLE*)LIST_NEXT(table);
	}
	return 0;
}
ptrIP_ROUTINGTABLE findInIarpTable(ptrIP_ROUTINGTABLE table, NETSIM_IPAddress ip)
{
	while (table)
	{
		if (!IP_COMPARE(table->networkDestination, ip))
		{
			return table;
		}
		table = (ptrIP_ROUTINGTABLE)LIST_NEXT(table);
	}
	return NULL;
}

int addToIERPTableFromIARP(NODE_IERP* ierp,ptrIP_ROUTINGTABLE iarpTable)
{
	ptrIP_ROUTINGTABLE table= iarpTable;
	while(iarpTable)
	{
		ptrIP_ROUTINGTABLE temp = iarpTable;
		unsigned int i;
		IERP_ROUTE_TABLE* entry = IERP_ROUTE_TABLE_ALLOC();
		entry->DestAddr = IP_COPY(iarpTable->networkDestination);
		entry->SubnetMask=IP_COPY(iarpTable->netMask);
		entry->count = iarpTable->Metric-1;
		if(entry->count)
			entry->Route = (NETSIM_IPAddress*)calloc(entry->count,sizeof* entry->Route);
		for(i=0;i<entry->count;i++)
		{
			entry->Route[i]=IP_COPY(iarpTable->gateway);
			iarpTable=findInIarpTable(table,iarpTable->gateway);
		}
		LIST_ADD_LAST((void**)&ierp->routeTable,entry);
		iarpTable=(ptrIP_ROUTINGTABLE)LIST_NEXT(temp);
	}
	return 0;
}

int extractRouteFromreply(IERP_PACKET* reply)
{
	unsigned int i;
	unsigned int count=0;
	NETSIM_IPAddress current = DEVICE_NWADDRESS(pstruEventDetails->nDeviceId,1);
	NETSIM_IPAddress dest = reply->RouteSourceAddress;
	NODE_ZRP* zrp=(NODE_ZRP*)DEVICE_NWROUTINGVAR(pstruEventDetails->nDeviceId);
	NODE_IERP* ierp=(NODE_IERP*)zrp->ierp;
	IERP_ROUTE_TABLE* table = ierp->routeTable;
	IERP_ROUTE_TABLE* entry;
	while(table)
	{
		if(!IP_COMPARE(table->DestAddr,dest))
			return 1;//entry already present
		table=(IERP_ROUTE_TABLE*)LIST_NEXT(table);
	}
	entry=IERP_ROUTE_TABLE_ALLOC();
	entry->DestAddr=IP_COPY(dest);
	entry->flag=true;
	entry->SubnetMask=IP_COPY(DEVICE_INTERFACE(pstruEventDetails->nDeviceId,1)->szSubnetMask);
	for(i=0;i<reply->IntermediateNodeCount;i++)
	{
		if(!IP_COMPARE(reply->IntermediateNodeAddress[i],current))
		{
			break;
		}
		count++;
	}
	entry->count=count;
	entry->Route=(NETSIM_IPAddress*)calloc(max(count,1),sizeof* entry->Route);
	for(i=0;i<count;i++)
		entry->Route[i]=IP_COPY(reply->IntermediateNodeAddress[count-i-1]);
	LIST_ADD_LAST((void**)&ierp->routeTable,entry);
	return 0;
}

int flushIERPTableFromIARP(NODE_IERP* ierp,ptrIP_ROUTINGTABLE iarpTable)
{
	IERP_ROUTE_TABLE* table = ierp->routeTable;
	ptrIP_ROUTINGTABLE temp;
	while(table)
	{
		if(table->flag)
		{
			NETSIM_IPAddress nextHop=table->Route[0];
			temp=iarpTable;
			while(temp)
			{
				if(!IP_COMPARE(temp->networkDestination,nextHop) && temp->Metric==1)
					break;
				temp=(ptrIP_ROUTINGTABLE)LIST_NEXT(temp);
			}
			if(!temp)
			{
				//IARP entry deleted. IERP entry is invalid
				LIST_FREE((void**)&ierp->routeTable,table);
				table=ierp->routeTable;
				continue;
			}
		}
		table=(IERP_ROUTE_TABLE*)LIST_NEXT(table);
	}
	return 0;
}
