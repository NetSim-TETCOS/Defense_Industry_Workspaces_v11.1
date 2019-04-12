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
https://tools.ietf.org/html/draft-ietf-manet-zone-brp-02
************************************************************************************/
#pragma once
#ifndef _NETSIM_BRP_H_
#define _NETSIM_BRP_H_
#ifdef  __cplusplus
extern "C" {
#endif

#define NW_PROTOCOL_BRP NW_PROTOCOL_ZRP
#define BRP_PACKET_SIZE		32 //Bytes

//API
#define flushBodercastTree(brp) { while(brp->bodercastTree) LIST_FREE((void**)&brp->bodercastTree,brp->bodercastTree);}

	typedef struct stru_BRP_Header		BRP_HEADER;
	typedef struct stru_BodercastTree	BRP_BODERCAST_TREE;
	typedef struct stru_BRP				NODE_BRP;


	/*
	A.  Packet Format

						1                   2                   3
	0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                       Query Source Address                    |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                    Query Destination Address                  |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|           Query ID            |Query Extension|    RESERVED   |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                    Prev Bordercast Address                    |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                                                               |
	|            E N C A P S U L A T E D     P A C K E T            |
	|                                                               |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	*/
	struct stru_BRP_Header
	{
		NETSIM_IPAddress QuerySourceAddress;
		NETSIM_IPAddress QueryDestinationAddress;
		unsigned short int QueryID;
		unsigned char QueryExtension;
		unsigned char RESERVED;
		NETSIM_IPAddress PrevBordercastAddress;
		NetSim_PACKET* EncapsulatedPacket;
	};

	struct stru_BodercastTree
	{
		NETSIM_IPAddress borderIP;
		NETSIM_ID boderId;
		NETSIM_IPAddress nexthop;
		_ele* ele;
	};
#define BRP_BODERCAST_TREE_ALLOC() (struct stru_BodercastTree*)list_alloc(sizeof(struct stru_BodercastTree),offsetof(struct stru_BodercastTree,ele))

	struct stru_BRP
	{
		BRP_BODERCAST_TREE* bodercastTree;
		unsigned short int nQueryId;
	};
	
	//Function prototype
	int fn_NetSim_BRP_Init();
	int fn_NetSim_BRP_CopyBRPHeader(const NetSim_PACKET* destPacket,const NetSim_PACKET* srcPacket);
	int fn_NetSim_BRP_FreeBRPHeader(NetSim_PACKET* packet);
	int addToBodercastTree(NODE_BRP* brp,
	NETSIM_IPAddress boderIP,
	NETSIM_IPAddress nextHop);
	int fn_NetSim_BRP_BodercastPacket(NetSim_PACKET* dataPacket);
	int fn_NetSim_BRP_ProcessPacket();
	int fn_NetSim_BRP_Update(ptrIP_ROUTINGTABLE iarpTable);
#ifdef  __cplusplus
}
#endif
#endif