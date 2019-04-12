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
http://tools.ietf.org/html/rfc3626
************************************************************************************/
#pragma once
#include "List.h"
#include "../IP/IP.h"
#ifndef _NETSIM_OSLR_H_
#define _NETSIM_OSLR_H_
#ifdef  __cplusplus
extern "C" {
#endif

#define NW_PROTOCOL_NDP NW_PROTOCOL_OLSR

//Config parameter
#define OLSR_HELLO_INTERVAL_DEFAULT		2 //Second
#define OLSR_REFERESH_INTERVAL_DEFAULT	2 //Second
#define OLSR_TC_INTERVAL_DEFAULT		5 //Second
#define OLSR_ZONE_RADIUS_DEFAULT		INFINITE

//Constant
#define TC_REDUNDANCY	0
#define MPR_COVERAGE	1
#define MAXJITTER		OLSR_HELLO_INTERVAL_DEFAULT/4.0*SECOND

//Holding Time
#define NEIGHB_HOLD_TIME	OLSR_REFERESH_INTERVAL_DEFAULT*3*SECOND
#define TOP_HOLD_TIME		OLSR_TC_INTERVAL_DEFAULT*3*SECOND
#define DUP_HOLD_TIME		30*SECOND

//Packet Size
#define OLSR_HEADER_SIZE		4	//Bytes
#define MESSAGE_HEADER_SIZE		12	//Bytes
#define HELLO_MESSAGE_SIZE		4	//Bytes
#define HELLO_LINK_SIZE			8	//Bytes
#define TC_MESSAGE_SIZE_FIXED	4	//Bytes

//TTL
#define TC_MESSAGE_TTL			255
#define HELLO_MESSAGE_TTL		1

//Scale factor. For converting in format of Exponent and mantissa
#define SCALE_FACTOR 0.0625

//Sequence Number. Section 19 RFC 3626
#define MAXVALUE_SEQUENCE_NUMBER 0xFFFF
#define SEQUENCE_NUMBER_GREATER_THAN(s1,s2) (((s1>s2) && ((s1-s2)<=(MAXVALUE_SEQUENCE_NUMBER/2))) || ((s2>s1) && ((s2-s1)>(MAXVALUE_SEQUENCE_NUMBER/2))))

//Struct typedef's
	typedef struct stru_Node_OLSR					NODE_OLSR;
	typedef struct stru_NetSim_OLSR_Header			OLSR_HEADER;
	typedef struct stru_OLSR_HelloPacket			OLSR_HELLO_PACKET;
	typedef struct stru_OLSR_NeighborTuples			OLSR_NEIGHBOR_SET;
	typedef struct stru_OLSR_2HopTuples				OLSR_2HOP_NEIGHBOR_SET;
	typedef struct stru_OLSR_MPRSet					OLSR_MPR_SET;
	typedef struct stru_OLSR_MPR_Selection_Tuples	OLSR_MPR_SELECTION_SET;
	typedef struct stru_OLSR_TopologyTuple			OLSR_TOPOLOGY_INFORMATION_BASE;
	typedef struct stru_Message						OLSR_HEADER_MESSAGE;
	typedef struct stru_Link						OLSR_HELLO_LINK;
	typedef struct stru_OLSR_LinkTuples				OLSR_LINK_SET;
	typedef struct stru_OLSR_TC_Message				OLSR_TC_MESSAGE;
	typedef struct stru_OLSR_DuplicateTuples		OLSR_DUPLICATE_SET;


/*	
18.4.  Message Types

		  HELLO_MESSAGE         = 1

		  TC_MESSAGE            = 2

		  MID_MESSAGE           = 3

		  HNA_MESSAGE           = 4
*/
	typedef enum
	{
		HELLO_MESSAGE	=1,
		TC_MESSAGE		=2,
		MID_MESSAGE		=3,
		HNA_MESSAGE		=4,
	}MESSAGE_TYPE;

#define OLSR_CONTROL_PACKET(x) NW_PROTOCOL_OLSR*100+x

/*

18.8.  Willingness

		  WILL_NEVER            = 0

		  WILL_LOW              = 1

		  WILL_DEFAULT          = 3

		  WILL_HIGH             = 6

		  WILL_ALWAYS           = 7
*/
	typedef enum
	{
		WILL_NEVER		= 0,
		WILL_LOW		= 1,
		WILL_DEFAULT	= 3,
		WILL_HIGH		= 6,
		WILL_ALWAYS		= 7,
	}WILLINGNESS;

/*	
18.5.  Link Types

		  UNSPEC_LINK           = 0

		  ASYM_LINK             = 1

		  SYM_LINK              = 2

		  LOST_LINK             = 3
*/
	typedef enum
	{
		UNSPEC_LINK		= 0,
		ASYM_LINK		= 1,
		SYM_LINK		= 2,
		LOST_LINK		= 3,
	}OLSR_LINK_TYPE;

/*	
18.6.  Neighbor Types

		  NOT_NEIGH             = 0

		  SYM_NEIGH             = 1

		  MPR_NEIGH             = 2
*/
	typedef enum
	{
		NOT_NEIGH=0,
		NOT_SYM=0,
		SYM_NEIGH=1,
		SYM=1,
		MPR_NEIGH=2,
	}OLSR_NEIGHBOR_TYPES;
/*
Clausen & Jacquet             Experimental                     [Page 13]
 
RFC 3626              Optimized Link State Routing          October 2003


3.3.  Packet Format

   The basic layout of any packet in OLSR is as follows (omitting IP and
   UDP headers):

	   0                   1                   2                   3
	   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
	  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	  |         Packet Length         |    Packet Sequence Number     |
	  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	  |  Message Type |     Vtime     |         Message Size          |
	  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	  |                      Originator Address                       |
	  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	  |  Time To Live |   Hop Count   |    Message Sequence Number    |
	  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	  |                                                               |
	  :                            MESSAGE                            :
	  |                                                               |
*/
	struct stru_Message
	{
		MESSAGE_TYPE MessageType;
		unsigned char Vtime;
		unsigned short int MessageSize;
		NETSIM_IPAddress OriginatorAddress;
		unsigned int TimeToLive:8;
		unsigned int HopCount:8;
		unsigned short int MessageSequenceNumber;
		void* MESSAGE;
		struct stru_Message* next;
	};
	struct stru_NetSim_OLSR_Header
	{
		unsigned short int PacketLength;
		unsigned short int PacketSequenceNumber;
		OLSR_HEADER_MESSAGE* message;
	};

		
/*
Clausen & Jacquet             Experimental                     [Page 27]
 
RFC 3626              Optimized Link State Routing          October 2003


	   0                   1                   2                   3
	   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1

	  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	  |          Reserved             |     Htime     |  Willingness  |
	  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	  |   Link Code   |   Reserved    |       Link Message Size       |
	  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	  |                  Neighbor Interface Address                   |
	  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	  |                  Neighbor Interface Address                   |
	  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	  :                             .  .  .                           :
	  :                                                               :
	  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	  |   Link Code   |   Reserved    |       Link Message Size       |
	  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	  |                  Neighbor Interface Address                   |
	  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	  |                  Neighbor Interface Address                   |
	  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	  :                                                               :
	  :                                       :
   (etc.)
*/
	struct stru_Link
	{
/*
6.1.1.  Link Code as Link Type and Neighbor Type
		7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
	|   0   |   0   |   0   |   0   | Neighbor Type |   Link Type   |
	+-------+-------+-------+-------+-------+-------+-------+-------+
*/
		struct stru_LinkCode
		{
			OLSR_LINK_TYPE linkType;
			OLSR_NEIGHBOR_TYPES neighTypes;
		}LinkCode;
		unsigned int Reserved:8;
		unsigned short int LinkMessageSize;
		NETSIM_IPAddress NeighborInterfaceAddress;
		struct stru_Link* next;
	};
	struct stru_OLSR_HelloPacket
	{
		unsigned short int Reserved;
		unsigned char HTime;
		WILLINGNESS Willingness;
		OLSR_HELLO_LINK* link;
	};

/*
9.1.  TC Message Format

   The proposed format of a TC message is as follows:

	   0                   1                   2                   3
	   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
	  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	  |              ANSN             |           Reserved            |
	  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	  |               Advertised Neighbor Main Address                |
	  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	  |               Advertised Neighbor Main Address                |
	  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	  |                              ...                              |
	  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/
	struct stru_OLSR_TC_Message
	{
		unsigned short int ANSN;
		unsigned short int Reserved;
		unsigned short int AdvertisedNeighborCount;
		NETSIM_IPAddress* AdvertisedNeighborMainAddress;
	};

	struct stru_OLSR_NeighborTuples
	{
		NETSIM_IPAddress N_neighbor_main_addr;
		OLSR_NEIGHBOR_TYPES N_status;
		WILLINGNESS N_willingness;
		_ele* ele;
	};
#define NEIGHBOR_TUPLES_ALLOC() (struct stru_OLSR_NeighborTuples*)list_alloc(sizeof(struct stru_OLSR_NeighborTuples),offsetof(struct stru_OLSR_NeighborTuples,ele))

	struct stru_OLSR_2HopTuples
	{
		NETSIM_IPAddress N_neighbor_main_addr;
		NETSIM_IPAddress N_2hop_addr;
		double N_time;
		_ele* ele;
	};
#define OLSR_2HOPTUPLES_ALLOC() (struct stru_OLSR_2HopTuples*)list_alloc(sizeof(struct stru_OLSR_2HopTuples),offsetof(struct stru_OLSR_2HopTuples,ele))

	struct stru_OLSR_MPRSet
	{
		NETSIM_IPAddress neighborAddress;
		_ele* ele;
	};
#define MPRSET_ALLOC() (struct stru_OLSR_MPRSet*)list_alloc(sizeof(struct stru_OLSR_MPRSet),offsetof(struct stru_OLSR_MPRSet,ele))

	struct stru_OLSR_MPR_Selection_Tuples
	{
		NETSIM_IPAddress MS_main_addr;
		double MS_time;
		_ele* ele;
	};
#define MPR_SELECTION_SET_ALLOC() (struct stru_OLSR_MPR_Selection_Tuples*)list_alloc(sizeof(struct stru_OLSR_MPR_Selection_Tuples),offsetof(struct stru_OLSR_MPR_Selection_Tuples,ele))

	struct stru_OLSR_TopologyTuple
	{
		NETSIM_IPAddress T_dest_addr;
		NETSIM_IPAddress T_last_addr;
		unsigned int T_seq;
		double T_time;
		_ele* ele;
	};
#define TOPOLOGY_INFO_BASE_ALLOC() (struct stru_OLSR_TopologyTuple*)list_alloc(sizeof(struct stru_OLSR_TopologyTuple),offsetof(struct stru_OLSR_TopologyTuple,ele))

	struct stru_OLSR_LinkTuples
	{
		NETSIM_IPAddress L_local_iface_addr;
		NETSIM_IPAddress L_neighbor_iface_addr;
		double L_SYM_time;
		double L_ASYM_time;
		double L_time;
		_ele* ele;
	};
#define LINK_SET_ALLOC() (OLSR_LINK_SET*)list_alloc(sizeof(OLSR_LINK_SET),offsetof(struct stru_OLSR_LinkTuples,ele))

/* Section 3.4
 a node records a "Duplicate Tuple" (D_addr, D_seq_num, D_retransmitted, D_iface_list,D_time), 
 where D_addr is the originator address of the message,
   D_seq_num is the message sequence number of the message,
   D_retransmitted is a boolean indicating whether the message has been
   already retransmitted, D_iface_list is a list of the addresses of the
   interfaces on which the message has been received and D_time
   specifies the time at which a tuple expires and *MUST* be removed
*/
	struct stru_OLSR_DuplicateTuples
	{
		NETSIM_IPAddress D_addr;
		unsigned short int D_seq_num;
		bool D_retransmitted;
		NETSIM_IPAddress D_iface_list;
		double D_time;
		_ele* ele;
	};
#define DUPLICATE_SET_ALLOC() (OLSR_DUPLICATE_SET*)list_alloc(sizeof(OLSR_DUPLICATE_SET),offsetof(struct stru_OLSR_DuplicateTuples,ele))

	struct stru_Node_OLSR
	{
		//Config parameter
		double dHelloInterval;
		double dRefereshInterval;
		double dTCInterval;
		unsigned int nZoneRadius;

		WILLINGNESS willingness;
		NETSIM_IPAddress mainAddress;
				
		//Neighbor table
		OLSR_NEIGHBOR_SET* neighborSet;
		OLSR_2HOP_NEIGHBOR_SET* twoHopNeighborSet;
		OLSR_MPR_SET* mprSet;
		OLSR_MPR_SELECTION_SET* mprSelectionSet;
		OLSR_TOPOLOGY_INFORMATION_BASE* topologyInfoBase;
		OLSR_LINK_SET* linkSet;

		//Sequence number
		unsigned short int nPacketSequenceNumber;
		unsigned short int nMessageSequenceNumber;
		unsigned short int ANSN;

		//Duplicate Set
		OLSR_DUPLICATE_SET* duplicateSet;

		//Routing table update flag
		bool bRoutingTableUpdate;
		ptrIP_WRAPPER ipWrapper;
		bool bTCUpdateFlag;
	};

	//Function prototype
	unsigned char olsrConvertDoubleToME(double val); //Sec 18.3
	double olsrConvertMEToDouble(unsigned char val); //Sec 18.3
	NODE_OLSR* GetOLSRData(NETSIM_ID nDeviceId);
	OLSR_LINK_SET* olsrFindLinkSet(NODE_OLSR* olsr,NETSIM_IPAddress ip);
	int fn_NetSim_NDP_ScheduleHelloTransmission(NETSIM_ID nNodeId,double dEventTime);
	OLSR_NEIGHBOR_SET* olsrFindNeighborSet(OLSR_NEIGHBOR_SET* neighborSet,NETSIM_IPAddress ip);
	OLSR_HELLO_LINK* olsrFinkLinkInfoFromHello(OLSR_HELLO_LINK* link,NETSIM_IPAddress ip);
	OLSR_2HOP_NEIGHBOR_SET* olsrFind2HopNeighbor(NODE_OLSR* olsr,NETSIM_IPAddress neighborAddress,NETSIM_IPAddress N_2_Hop_Address);
	OLSR_2HOP_NEIGHBOR_SET* olsrFill2HopNeighbor(NODE_OLSR* olsr);
	OLSR_NEIGHBOR_SET* olsrFillNeighbor(NODE_OLSR* olsr);
	int olsrAddtoMPRSet(OLSR_MPR_SET** mpr,OLSR_NEIGHBOR_SET* N,OLSR_2HOP_NEIGHBOR_SET* N2);
	int fn_NetSim_OLSR_PopulateLinkSet();
	int fn_NetSim_NDP_CopyHelloMessage(OLSR_HEADER_MESSAGE* dest,OLSR_HEADER_MESSAGE* src);
	unsigned short int fn_NetSim_Fill_Link_To_Hello(NODE_OLSR* olsr,OLSR_HELLO_PACKET* hello,double dCurrentTime);
	int fn_NetSim_OLSR_Populate2HopNeighbor();
	int fn_NetSim_OLSR_PopulateMPRSet();
	int fn_NetSim_NDP_FreeHelloMessage(OLSR_HEADER_MESSAGE* message);
	int fn_NetSim_NDP_Init();
	int fn_NetSim_NDP_ReceiveHello();
	int fn_NetSim_OLSR_PopulateNeighborSet();
	int fn_NetSim_NDP_TransmitHello();
	int fn_NetSim_OLSR_Configure_F(void** var,void** data);
	int fn_NetSim_OLSR_CopyOLSRHeader(const NetSim_PACKET* destPacket,const NetSim_PACKET* srcPacket);
	int fn_NetSim_OLSR_FreeOLSRHeader(NetSim_PACKET* packet);
	int fn_NetSim_OLSR_Init();
	OLSR_MPR_SELECTION_SET* olsrFindMPRSelectorSet(OLSR_MPR_SELECTION_SET* set,NETSIM_IPAddress ip);
	OLSR_DUPLICATE_SET* olsrFindDuplicateSet(OLSR_DUPLICATE_SET* duplicate,NETSIM_IPAddress ip,unsigned short int seq_no);
	int fn_NetSim_OLSR_CopyTCMessage(OLSR_HEADER_MESSAGE* dest,OLSR_HEADER_MESSAGE* src);
	int fn_NetSim_OLSR_Finish_F();
	int fn_NetSim_OLSR_FreeTCMessage(OLSR_HEADER_MESSAGE* message);
	int fn_NetSim_OLSR_LinkTupleExpire();
	int fn_NetSim_OLSR_UpdateRoutingTable();
	int fn_NetSim_OLSR_PacketForwarding();
	int fn_NetSim_OLSR_PacketProcessing();
	int fn_NetSim_OLSR_PopulateMPRSelectorSet();
	int fn_NetSim_OLSR_ReceiveTC();
	int fn_NetSim_OLSR_Remove2HopTuple(NODE_OLSR* olsr,OLSR_NEIGHBOR_SET* neighbor);
	int fn_NetSim_OLSR_RemoveNeighbortuple(OLSR_LINK_SET* link);
	int fn_NetSim_OLSR_ScheduleTCTransmission(NETSIM_ID nNodeId,double dTime);
	int fn_NetSim_OLSR_TransmitTCMessage();
	int olsrAddLinktuplesExpiryEvent(OLSR_LINK_SET* link);
	bool olsrExistInDuplicateSet(OLSR_DUPLICATE_SET* set,OLSR_HEADER* header);
	int olsrFlushroutingTable(ptrIP_WRAPPER wrapper,NETSIM_ID nNodeId);
	int olsrMarkMPR(NODE_OLSR* olsr);
	int olsrPrintMPR(OLSR_MPR_SET* mprSet);
	int olsrUpdateIptable(ptrIP_ROUTINGTABLE table,NETSIM_ID nNodeId);
	int olsrUpdateTopologySet(NODE_OLSR* olsr,OLSR_TOPOLOGY_INFORMATION_BASE** topology,OLSR_HEADER_MESSAGE* message);
	bool olsrValidateTopologyInfoOnCondition2(OLSR_TOPOLOGY_INFORMATION_BASE* topology,NETSIM_IPAddress originator,unsigned short int ANSN);

#ifdef  __cplusplus
}
#endif
#endif
