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
https://tools.ietf.org/html/draft-ietf-manet-zone-ierp-02
************************************************************************************/
#pragma once
#ifndef _NETSIM_IERP_H_
#define _NETSIM_IERP_H_
#ifdef  __cplusplus
extern "C" {
#endif

#define IERP_ROUTE_REQUEST_SIZE_FIXED	16 //Bytes
#define IERP_ROUTE_REPLY_SIZE_FIXED		16 //Bytes

//Typedefs struct
	typedef struct stru_IERP				NODE_IERP;
	typedef struct stru_IERP_Packet			IERP_PACKET;
	typedef struct stru_IERP_RoutingTable	IERP_ROUTE_TABLE;

#define NW_PROTOCOL_IERP NW_PROTOCOL_ZRP	

	/*
	A.  Packet Format
						1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |     Type      |     Length    |    Node Ptr   |    RESERVED   |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |           Query ID            |        R E S E R V E D        |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                  Query/Route Source Address                   |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ -+-
   |                 Intermediate Node (1) Address                 |  |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+  |
   |                 Intermediate Node (2) Address                 |  |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+  |
								  | |                                 |
								  | |                               route
								 \| |/                                |
								  \ /                                 |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-|  |
   |                Intermediate Node (N) Address                  |  |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+  |
   |                Query/Route Destination Address                |  |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ -+-
*/
#define ROUTE_REQUEST	1
#define ROUTE_REPLY		2
#define IERP_PACKET_LENGTH(size) size*8/32
	struct stru_IERP_Packet
	{
		unsigned char Type;
		unsigned char Length;
		unsigned char NodePtr;
		unsigned char RESERVED;
		unsigned short int QueryID;
		unsigned short int RESERVED1;
		NETSIM_IPAddress RouteSourceAddress;
		unsigned int IntermediateNodeCount;
		NETSIM_IPAddress* IntermediateNodeAddress;
		NETSIM_IPAddress RouteDestinationAddress;
	};

/*
	 B.2  IERP Routing Table

			+-----------------------|--------------------------------+
			|   Dest    |  Subnet   |      Route     |     Route     |
			|   Addr    |   Mask    |                |    Metrics    |
			| (node_id) | (node_id) | (node_id list) | (metric list) |
			|-----------+-----------|----------------+---------------|
			|           |           |                |               |
			|-----------+-----------|----------------+---------------|
			|           |           |                |               |
			|-----------+-----------|----------------+---------------|
			|           |           |                |               |
			+-----------------------|--------------------------------+
*/
	struct stru_IERP_RoutingTable
	{
		NETSIM_IPAddress DestAddr;
		NETSIM_IPAddress SubnetMask;
		unsigned int count;
		NETSIM_IPAddress* Route;
		unsigned int* RouteMetrics;
		bool flag;
		_ele* ele;
	};
#define IERP_ROUTE_TABLE_ALLOC() (IERP_ROUTE_TABLE*)list_alloc(sizeof(IERP_ROUTE_TABLE),offsetof(IERP_ROUTE_TABLE,ele))

	struct stru_IERP
	{
		NetSim_PACKET* buffer; //List of packet thats need to be routed
		IERP_ROUTE_TABLE* routeTable;
		unsigned short int nQueryId;
	};


	int fn_NetSim_IERP_Init();
	IERP_ROUTE_TABLE* checkDestInRouteTable(IERP_ROUTE_TABLE* table,NETSIM_IPAddress dest);
	int flushIerpTable(NODE_IERP* ierp);
	int addToIERPTableFromIARP(NODE_IERP* ierp, ptrIP_ROUTINGTABLE iarpTable);
	int extractRouteFromreply(IERP_PACKET* reply);
	int flushIERPTableFromIARP(NODE_IERP* ierp, ptrIP_ROUTINGTABLE iarpTable);
	int fn_NetSim_IERP_CopyIerpHeader(const NetSim_PACKET* destPacket,const NetSim_PACKET* srcPacket);
	int fn_NetSim_IERP_FreeIerpHeader(NetSim_PACKET* packet);
	int fn_NetSim_IERP_GenerateRouteReply(NetSim_PACKET* requestPacket);
	int fn_NetSim_IERP_GenerateRouteRequest(NODE_IERP* ierp,NetSim_PACKET* dataPacket);
	bool fn_NetSim_IERP_ProcessPacket(NetSim_PACKET* packet);
	int fn_NetSim_IERP_ProcessRouteReply();
	int fn_NetSim_IERP_RoutePacket();
	int routePacketFromBuffer();


#ifdef  __cplusplus
}
#endif
#endif