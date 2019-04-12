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


/************************************************************************************
http://tools.ietf.org/html/draft-ietf-manet-zone-zrp-04
************************************************************************************/
#pragma once
#include "OLSR.h"
#include "BRP.h"
#include "IERP.h"
#ifndef _NETSIM_ZRP_H_
#define _NETSIM_ZRP_H_
#ifdef  __cplusplus
extern "C" {
#endif
//Include necessary lib's
#pragma comment(lib,"NetworkStack.lib")

#define ZRP_IARP_PROTOCOL_DEFAULT	_strdup("OLSR");
#define ZRP_ZONE_RADIUS_DEFAULT		2

// API
#define flushZone(zrp)	{while(zrp->zone) LIST_FREE((void**)&zrp->zone,zrp->zone);}

	typedef struct stru_Node_ZRP	NODE_ZRP;
	typedef struct stru_Zone		ZRP_ZONE;

	typedef enum
	{
		IERP_ROUTE_REQUEST=NW_PROTOCOL_ZRP*100,
		IERP_ROUTE_REPLY,

		IERP_ROUTE_REQUEST_WITH_BRP_HEADER,
		IERP_ROUTE_REPLY_WITH_BRP_HEADER,
	}ZRP_CONTROL_PACKET;
#define BRP_DIFF IERP_ROUTE_REQUEST_WITH_BRP_HEADER-IERP_ROUTE_REQUEST
	struct stru_Zone
	{
		NETSIM_IPAddress zoneNodeIP;
		_ele* ele;
	};
#define ZRP_ZONE_ALLOC() (struct stru_Zone*)list_alloc(sizeof(struct stru_Zone),offsetof(struct stru_Zone,ele))

	struct stru_Node_ZRP
	{
		NETWORK_LAYER_PROTOCOL iarpProtocol;
		unsigned int nZoneRadius;
		ZRP_ZONE* zone;
		void* iarp;
		void* ierp;
		void* brp;
	};

	//Function prototype
	NetSim_PACKET* fn_NetSim_ZRP_GeneratePacket(double dTime,
		unsigned int nPacketType,
		NETWORK_LAYER_PROTOCOL protocol,
		NETSIM_ID nSource,
		NETSIM_ID nDestination,
		double dPacketSize);
	int fn_NetSim_ZRP_Configure_F(void** var);
	int fn_NetSim_ZRP_FreePacket_F(NetSim_PACKET* packet);
	int fn_NetSim_ZRP_CopyPacket_F(const NetSim_PACKET* destPacket,const NetSim_PACKET* srcPacket);
	char* fn_NetSim_ZRP_Trace_F(NETSIM_ID id);
	int fn_NetSim_ZRP_Finish_F();
	int fn_NetSim_ZRP_Init_F();
	int addToZoneList(NODE_ZRP* zrp,NETSIM_IPAddress ip);

#ifdef  __cplusplus
}
#endif
#endif
