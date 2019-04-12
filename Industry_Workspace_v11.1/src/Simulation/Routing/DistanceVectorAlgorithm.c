/************************************************************************************
* Copyright (C) 2012     
*
* TETCOS, Bangalore. India                                                         *

* Tetcos owns the intellectual property rights in the Product and its content.     *
* The copying, redistribution, reselling or publication of any or all of the       *
* Product or its content without express prior written consent of Tetcos is        *
* prohibited. Ownership and / or any other right relating to the software and all  *
* intellectual property rights therein shall remain at all times with Tetcos.      *

* Author:  Thangarasu.K                                                       *
* ---------------------------------------------------------------------------------*/
#include "main.h"
#include "Routing.h"
#include "List.h"
#include "../IP/IP.h"
/**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This function runs the distance vector routing algorithm for optimized routing of packets from 
source to destination. The optimization is done per a particular metric, i.e the shortest distance.
This algorithm runs inside a single Autonomous System

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/
int fn_NetSim_RIP_DistanceVectorAlgorithm(struct stru_NetSim_Network *NETWORK,NetSim_EVENTDETAILS *pstruEventDetails)
{
	NETSIM_ID nDeviceId,i;
	RIP_ROUTING_DATABASE *pstru_Router_RIP_TempTable=NULL;
	RIP_ROUTING_DATABASE *pstru_Router_RIP_TempTraverse=NULL,*pstru_Router_RIP_InsertTable=NULL;
	// NetSim packet To store the RIP packet
	NetSim_PACKET *pstruControlPacket = NULL;
	RIP_ENTRY *pstru_Router_RIP_RIPEntry=NULL;
	// RIP PACKET to store RIP packet data 	
	RIP_PACKET *pstruPacketRIP = NULL;	
	pstruControlPacket=pstruEventDetails->pPacket;
	nDeviceId=pstruEventDetails->nDeviceId;
	pstruPacketRIP=pstruControlPacket->pstruAppData->Packet_AppProtocol;
	pstru_Router_RIP_RIPEntry=pstruPacketRIP->pstruRIPEntry;
	//Get the Routing database of the router
	pstru_Router_RIP_TempTable = ((DEVICE_ROUTER*)DEVICE_APPVAR(nDeviceId, APP_PROTOCOL_RIP))->pstruRoutingTables->pstruRIP_RoutingTable;
	fn_NetSim_Stack_GetDeviceId_asIP(pstruControlPacket->pstruNetworkData->szNextHopIp,&i);
	while(pstru_Router_RIP_RIPEntry)
	{
		pstru_Router_RIP_TempTraverse=pstru_Router_RIP_TempTable;

		while(pstru_Router_RIP_TempTraverse)
		{
			if(!IP_COMPARE(pstru_Router_RIP_TempTraverse->szAddress,pstru_Router_RIP_RIPEntry->szIPv4_address)&& !IP_COMPARE(pstru_Router_RIP_TempTraverse->szSubnetmask,pstru_Router_RIP_RIPEntry->szSubnet_Mask) )
			{
				if((pstru_Router_RIP_TempTraverse->nMetric  > pstru_Router_RIP_RIPEntry->nMetric+1))
				{
					IP_FREE(pstru_Router_RIP_TempTraverse->szAddress);
					IP_FREE(pstru_Router_RIP_TempTraverse->szRouter);
					pstru_Router_RIP_TempTraverse->szAddress = IP_COPY(pstru_Router_RIP_RIPEntry->szIPv4_address);
					pstru_Router_RIP_TempTraverse->szRouter = IP_COPY(pstruControlPacket->pstruNetworkData->szGatewayIP);
					pstru_Router_RIP_TempTraverse->nMetric = pstru_Router_RIP_RIPEntry->nMetric + 1;
					pstru_Router_RIP_TempTraverse->nInterface = i;
					pstru_Router_RIP_TempTraverse->dTimer = pstruEventDetails->dEventTime;
					{
						iptable_add(IP_WRAPPER_GET(nDeviceId),
									pstru_Router_RIP_RIPEntry->szIPv4_address,
									pstru_Router_RIP_RIPEntry->szSubnet_Mask,
									0,
									pstruControlPacket->pstruNetworkData->szGatewayIP,
									1,
									&pstruControlPacket->pstruNetworkData->szNextHopIp,
									&i,
									pstru_Router_RIP_RIPEntry->nMetric + 1,
									"RIP");
					}
				}
				break;
			}
			pstru_Router_RIP_TempTraverse = pstru_Router_RIP_TempTraverse->pstru_Router_NextEntry;
		}
		// If any new entry update that entry in the routing database
		if(!pstru_Router_RIP_TempTraverse)
		{

				pstru_Router_RIP_InsertTable=calloc(1,sizeof(RIP_ROUTING_DATABASE));
				pstru_Router_RIP_InsertTable->szAddress=pstru_Router_RIP_RIPEntry->szIPv4_address;
				pstru_Router_RIP_InsertTable->szSubnetmask = IP_COPY(pstru_Router_RIP_RIPEntry->szSubnet_Mask);
				pstru_Router_RIP_InsertTable->szRouter= IP_COPY(pstruControlPacket->pstruNetworkData->szGatewayIP);
				pstru_Router_RIP_InsertTable->nInterface=i;
				pstru_Router_RIP_InsertTable->nMetric=pstru_Router_RIP_RIPEntry->nMetric+1;
				pstru_Router_RIP_InsertTable->dTimer=pstruEventDetails->dEventTime;
				fn_NetSim_UpdateEntryinRoutingTable(NETWORK,pstruEventDetails,nDeviceId,pstru_Router_RIP_InsertTable->nInterface,0,pstru_Router_RIP_InsertTable->szAddress,pstru_Router_RIP_InsertTable->szRouter,pstru_Router_RIP_InsertTable->szSubnetmask,0,pstru_Router_RIP_InsertTable->nMetric,1);
				((DEVICE_ROUTER*)(DEVICE_APPVAR(nDeviceId,APP_PROTOCOL_RIP)))->uniInteriorRouting.struRIP.nRIP_Update++;

				//Adding the entries in IP table
				{
					iptable_add(IP_WRAPPER_GET(nDeviceId),
								pstru_Router_RIP_RIPEntry->szIPv4_address,
								pstru_Router_RIP_RIPEntry->szSubnet_Mask,
								0,
								pstruControlPacket->pstruNetworkData->szGatewayIP,
								1,
								&pstruControlPacket->pstruNetworkData->szNextHopIp,
								&i,
								pstru_Router_RIP_RIPEntry->nMetric + 1,
								"RIP");
				}
				fnpFreeMemory(pstru_Router_RIP_InsertTable);

		}
		pstru_Router_RIP_RIPEntry=pstru_Router_RIP_RIPEntry->pstru_RIP_NextEntry;
	}
	if(pstruControlPacket)
	{
		fn_NetSim_Packet_FreePacket(pstruControlPacket);
		pstruEventDetails->pPacket=NULL;
	}
	return 0;
}
