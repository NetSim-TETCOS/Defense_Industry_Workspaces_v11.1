/************************************************************************************
* Copyright (C) 2013                                                               *
* TETCOS, Bangalore. India                                                         *
*                                                                                  *
* Tetcos owns the intellectual property rights in the Product and its content.     *
* The copying, redistribution, reselling or publication of any or all of the       *
* Product or its content without express prior written consent of Tetcos is        *
* prohibited. Ownership and / or any other right relating to the software and all *
* intellectual property rights therein shall remain at all times with Tetcos.      *
*                                                                                  *
* Author:    Thangarasu                                                       *
*                                                                                  *
* ---------------------------------------------------------------------------------*/
#ifndef _ROUTING_H_
#define _ROUTING_H_
#ifdef  __cplusplus
extern "C" {
#endif
#include "RIP.h"
typedef struct stru_ROUTINGTABLES ROUTING_TABLES;
int (*fnRouter[10])();
/** Structure for NetSim Router */
struct stru_NetSim_Router
{
	union uni_Interior_Routing
	{
		struct stru_RIP struRIP; //RIP
	}uniInteriorRouting;
	ROUTING_TABLES *pstruRoutingTables;
	APPLICATION_LAYER_PROTOCOL AppIntRoutingProtocol;
	APPLICATION_LAYER_PROTOCOL AppExtRoutingProtocol;
	APPLICATION_LAYER_PROTOCOL *RoutingProtocol;
	ptrSOCKETINTERFACE sock;
};
/** Structure for router buffer */
struct stru_Router_Buffer
{
	NetSim_BUFFER *pstruBuffer;
};
/** Structure to store Routing Tables*/
struct stru_ROUTINGTABLES
{
	RIP_ROUTING_DATABASE *pstruRIP_RoutingTable;
};

_declspec(dllexport)int fn_NetSim_UpdateEntryinRoutingTable(struct stru_NetSim_Network *NETWORK,NetSim_EVENTDETAILS *pstruEventDetails,NETSIM_ID nDeviceId,NETSIM_ID nInterfaceId,unsigned int bgpRemoteAS,NETSIM_IPAddress szDestinationIP,NETSIM_IPAddress szNexthop,NETSIM_IPAddress szSubnetmask,unsigned int prefix_len,int nCost,int nFlag);
_declspec(dllexport)int UpdateInterfaceList();
int fnCheckRoutingPacket();
APPLICATION_LAYER_PROTOCOL fnCheckRoutingProtocol(NETSIM_ID deviceId);

#ifdef  __cplusplus
}
#endif
#endif