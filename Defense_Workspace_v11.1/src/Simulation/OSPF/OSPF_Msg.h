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

#ifndef _NETSIM_OSPF_MSG_H_
#define _NETSIM_OSPF_MSG_H_
#ifdef  __cplusplus
extern "C" {
#endif

#define OSPF_VERSION 2
#ifndef OSPFID
	typedef NETSIM_IPAddress OSPFID;
#endif // !OSPFID
#define INTERFACE_MTU_DEFAULT	1500 //Bytes
#define IP_HDR_LEN				20	 //Bytes
#define OSPF_INITIAL_SEQUENCE_NUMBER	0x80000001
#define OSPF_MAX_SEQUENCE_NUMBER		0x7FFFFFFF


	enum enum_ospf_msg
	{
		OSPFMSG_HELLO = 1,
		OSPFMSG_DD,
		OSPFMSG_LSREQUEST,
		OSPFMSG_LSUPDATE,
		OSPFMSG_LSACK,
	};
#define OSPFMSG_TO_PACKETTYPE(msg) (APP_PROTOCOL_OSPF*100+msg)
#define OSPFMSG_FROM_PACKETTYPE(type) (type/100==APP_PROTOCOL_OSPF?type%100:0)

	// Type of link.
	typedef enum
	{
		OSPFLINKTYPE_POINT_TO_POINT = 1,
		OSPFLINKTYPE_TRANSIT = 2,
		OSPFLINKTYPE_STUB = 3,
		OSPFLINKTYPE_VIRTUAL = 4
	} OSPFLINKTYPE;
	static char strOSPFLINKTYPE[][50] = { "NULL","Point_to_Point","Transit","Stub","Virtual" };

	/*
	A.3.1 The OSPF packet header
	0                   1                   2                   3
	0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|   Version #   |     Type      |         Packet length         |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                          Router ID                            |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                           Area ID                             |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|           Checksum            |             AuType            |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                       Authentication                          |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                       Authentication                          |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

	*/

	struct stru_ospf_packet_header
	{
		UINT8 Version;
		OSPFMSG Type;
		UINT16 Packet_length;
		OSPFID RouterId;
		OSPFID AreaId;
		UINT16 Checksum;
		UINT16 AuType;
		UINT Authentication[2];

		void* ospfMSG;
	};
#define OSPFPACKETHDR_LEN 24 //Bytes
#define OSPF_HDR_SET_ROUTERID(hdr,id)	((hdr)->RouterId = id)
#define OSPF_HDR_SET_AREAID(hdr,id)		((hdr)->AreaId = id)
#define OSPF_MSG_TYPE(hdr)				((hdr)->Type)
#define OSPF_MSG_IS_TYPE(hdr,type)		((hdr)->Type==type)
#define OSPF_HDR_GET_LEN(hdr)			((hdr)->Packet_length)
#define OSPF_PACKET_GET_HDR(packet)		((ptrOSPFPACKETHDR)((packet)->pstruAppData->Packet_AppProtocol))
#define OSPF_PACKET_SET_HDR(packet,hdr)	((packet)->pstruAppData->Packet_AppProtocol=hdr)
#define OSPF_HDR_GET_MSG(hdr)			((hdr)->ospfMSG)
#define OSPF_PACKET_GET_MSG(packet)		(OSPF_HDR_GET_MSG(OSPF_PACKET_GET_HDR(packet)))
#define OSPF_PACKET_GET_MSG_TYPE(packet)	(OSPF_MSG_TYPE(OSPF_PACKET_GET_HDR(packet)))
	void OSPF_HDR_INCREASE_LEN(NetSim_PACKET* packet,
							   UINT16 len);
	void OSPF_HDR_SET_MSG(ptrOSPFPACKETHDR hdr,
						  OSPFMSG type,
						  void* msg,
						  UINT16 len);
	void OSPF_HDR_FREE(ptrOSPFPACKETHDR hdr);
	ptrOSPFPACKETHDR OSPF_HDR_COPY(ptrOSPFPACKETHDR hdr);

	//OSPF Option
#define OPT_E_BIT_INDEX		2
#define OPT_MC_BIT_INDEX	3
#define OPT_NP_BIT_INDEX	4
#define OPT_EA_BIT_INDEX	5
#define OPT_DC_BIT_INDEX	6
#define OPT_SET_E(opt)		((opt) = setBit((opt),OPT_E_BIT_INDEX))
#define OPT_SET_MC(opt)		((opt) = setBit((opt),OPT_MC_BIT_INDEX))
#define OPT_SET_NP(opt)		((opt) = setBit((opt),OPT_NP_BIT_INDEX))
#define OPT_SET_EA(opt)		((opt) = setBit((opt),OPT_EA_BIT_INDEX))
#define OPT_SET_DC(opt)		((opt) = setBit((opt),OPT_DC_BIT_INDEX))
#define OPT_IS_E(opt)		(isBitSet((opt),OPT_E_BIT_INDEX))
#define OPT_IS_MC(opt)		(isBitSet((opt),OPT_MC_BIT_INDEX))
#define OPT_IS_NP(opt)		(isBitSet((opt),OPT_NP_BIT_INDEX))
#define OPT_IS_EA(opt)		(isBitSet((opt),OPT_EA_BIT_INDEX))
#define OPT_IS_DC(opt)		(isBitSet((opt),OPT_DC_BIT_INDEX))

	/*
	A.3.2 The Hello packet
	0                   1                   2                   3
	0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|   Version #   |       1       |         Packet length         |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                          Router ID                            |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                           Area ID                             |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|           Checksum            |             AuType            |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                       Authentication                          |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                       Authentication                          |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                        Network Mask                           |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|         HelloInterval         |    Options    |    Rtr Pri    |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                     RouterDeadInterval                        |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                      Designated Router                        |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                   Backup Designated Router                    |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                          Neighbor                             |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                              ...                              |
	*/
	struct stru_ospf_hello
	{
		NETSIM_IPAddress NetworkMask;
		UINT16 HelloInterval;
		UINT8 Options;
		UINT8 RtrPri;
		UINT RouterDeadInterval;
		NETSIM_IPAddress DesignatedRouter;
		NETSIM_IPAddress BackupDesignatedRouter;
		NETSIM_IPAddress* Neighbor;

		UINT neighCount;
	};
#define OSPFHELLO_LEN_FIXED 20 //Bytes 
	void OSPF_HELLO_MSG_NEW(ptrOSPFPACKETHDR hdr);
	ptrOSPFHELLO OSPF_HELLO_MSG_COPY(ptrOSPFHELLO hello);
	void OSPF_HELLO_MSG_FREE(ptrOSPFHELLO hello);

	/*
	A.3.3 The Database Description packet
	0                   1                   2                   3
	0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|   Version #   |       2       |         Packet length         |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                          Router ID                            |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                           Area ID                             |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|           Checksum            |             AuType            |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                       Authentication                          |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                       Authentication                          |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|         Interface MTU         |    Options    |0|0|0|0|0|I|M|MS
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                     DD sequence number                        |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                                                               |
	+-                                                             -+
	|                                                               |
	+-                      An LSA Header                          -+
	|                                                               |
	+-                                                             -+
	|                                                               |
	+-                                                             -+
	|                                                               |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                              ...                              |
	*/
	typedef struct stru_ospf_DD
	{
		UINT16 InterfaceMTU;
		UINT8 Option;
		UINT8 IMMS;
		UINT DDSequenceNumber;
		UINT16 numLSA;
		ptrOSPFLSAHDR* LSAHeader;
	}OSPFDD,*ptrOSPFDD;
#define OSPFDD_LEN_FIXED 8 //Bytes
#define DD_INIT_BIT_INDEX	3
#define DD_MORE_BIT_INDEX	2
#define DD_MASTER_BIT_INDEX	1
#define DD_SET_INIT(dd)		((dd)->IMMS = (UINT8)setBit((dd)->IMMS,DD_INIT_BIT_INDEX))
#define DD_SET_MORE(dd)		((dd)->IMMS = (UINT8)setBit((dd)->IMMS,DD_MORE_BIT_INDEX))
#define DD_SET_MASTER(dd)	((dd)->IMMS = (UINT8)setBit((dd)->IMMS,DD_MASTER_BIT_INDEX))
#define DD_IS_INIT(dd)		(isBitSet((dd)->IMMS,DD_INIT_BIT_INDEX))
#define DD_IS_MORE(dd)		(isBitSet((dd)->IMMS,DD_MORE_BIT_INDEX))
#define DD_IS_MASTER(dd)	(isBitSet((dd)->IMMS,DD_MASTER_BIT_INDEX))
#define OSPF_DD_MAX_LSA_COUNT() ((INTERFACE_MTU_DEFAULT - IP_HDR_LEN - OSPFDD_LEN_FIXED - OSPFPACKETHDR_LEN) / OSPFLSAHDR_LEN);
	void OSPF_DD_MSG_NEW(ptrOSPFPACKETHDR hdr);
	ptrOSPFDD OSPF_DD_MSG_COPY(ptrOSPFDD dd);
	void OSPF_DD_MSG_FREE(ptrOSPFDD dd);

	/*
	A.3.4 The Link State Request packet
	0                   1                   2                   3
	0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|   Version #   |       3       |         Packet length         |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                          Router ID                            |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                           Area ID                             |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|           Checksum            |             AuType            |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                       Authentication                          |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                       Authentication                          |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                          LS type                              |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                       Link State ID                           |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                     Advertising Router                        |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                              ...                              |
	*/
	typedef struct stru_ospf_LSREQ_Object
	{
		UINT LSType;
		NETSIM_IPAddress LinkStateId;
		NETSIM_IPAddress AdvertisingRouter;
	}OSPFLSREQOBJ,*ptrOSPFLSREQOBJ;

	typedef struct stru_ospf_lsrequest
	{
		UINT count;
		ptrOSPFLSREQOBJ* lsrObj;
	}OSPFLSREQ, *ptrOSPFLSREQ;
#define OSPFLSREQ_LEN_SINGLE	12 //Bytes
	void OSPF_LSREQ_MSG_NEW(ptrOSPFPACKETHDR hdr);
	ptrOSPFLSREQ OSPF_LSREQ_MSG_COPY(ptrOSPFLSREQ lsr);
	void OSPF_LSREQ_MSG_FREE(ptrOSPFLSREQ lsr);

	/*
	A.3.5 The Link State Update packet
	0                   1                   2                   3
	0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|   Version #   |       4       |         Packet length         |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                          Router ID                            |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                           Area ID                             |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|           Checksum            |             AuType            |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                       Authentication                          |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                       Authentication                          |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                            # LSAs                             |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                                                               |
	+-                                                            +-+
	|                             LSAs                              |
	+-                                                            +-+
	|                              ...                              |

	*/
	typedef struct stur_ospf_lsupdate
	{
		UINT LSAsCount;
		void** LSAs;
	}OSPFLSUPDATE,*ptrOSPFLSUPDATE;
#define OSPFLSUPDATE_LEN_FIXED 4 //Bytes
	void OSPF_LSUPDATE_MSG_NEW(ptrOSPFPACKETHDR hdr);
	ptrOSPFLSUPDATE OSPF_LSUPDATE_MSG_COPY(ptrOSPFLSUPDATE lsu);
	void OSPF_LSUPDATE_MSG_FREE(ptrOSPFLSUPDATE lsu);

	/*
	A.3.6 The Link State Acknowledgment packet
	0                   1                   2                   3
	0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|   Version #   |       5       |         Packet length         |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                          Router ID                            |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                           Area ID                             |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|           Checksum            |             AuType            |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                       Authentication                          |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                       Authentication                          |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                                                               |
	+-                                                             -+
	|                                                               |
	+-                         An LSA Header                       -+
	|                                                               |
	+-                                                             -+
	|                                                               |
	+-                                                             -+
	|                                                               |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                              ...                              |
	*/
	typedef struct stru_ospf_lsack
	{
		UINT count;
		ptrOSPFLSAHDR* LSAHeader;
	}OSPFLSACK,*ptrOSPFLSACK;
#define OSPFLSACK_LEN_FIXED	0 //Bytes
	void OSPF_LSACK_NEW(ptrOSPFPACKETHDR hdr);
	ptrOSPFLSACK OSPF_LSACK_COPY(ptrOSPFLSACK ack);
	void OSPF_LSACK_FREE(ptrOSPFLSACK ack);

	/*
	LS Type   Description
	___________________________________
	1         Router-LSAs
	2         Network-LSAs
	3         Summary-LSAs (IP network)
	4         Summary-LSAs (ASBR)
	5         AS-external-LSAs
	*/
	enum enum_ls_type
	{
		LSTYPE_ROUTERLSA = 1,
		LSTYPE_NETWORKLSA,
		LSTYPE_SUMMARYLSA_ROUTER,
		LSTYPE_SUMMARYLSA_NETWORK,
		LSTYPE_ASEXTERNALLSA,
		LSTYPE_UNDEFINED,
	};
	static char strLSTYPE[][50] = { "UNKNOWN",
		"ROUTER_LSA",
		"NETWORK_LSA",
		"SUMMARY_LSA_ROUTER",
		"SUMMARY_LSA_NETWORK",
		"AS_EXTERNAL_LSA"
		"UNDEFINED"};

	/*
	A.4.1 The LSA header
	0                   1                   2                   3
	0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|            LS age             |    Options    |    LS type    |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                        Link State ID                          |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                     Advertising Router                        |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                     LS sequence number                        |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|         LS checksum           |             length            |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	*/
	struct stru_lsa_header
	{
		UINT16 LSAge;
		UINT8 Options;
		LSTYPE LSType;
		OSPFID LinkStateID;
		OSPFID AdvertisingRouter;
		UINT LSSequenceNumber;
		UINT16 LSChecksum;
		UINT16 length;
		void* lsaInfo;

		//Simulation specific.
		double time;
	};
#define OSPFLSAHDR_LEN 20 //Bytes
	void OSPF_LSA_MSG_FREE(ptrOSPFLSAHDR hdr);
	ptrOSPFLSAHDR OSPF_LSA_MSG_COPY(ptrOSPFLSAHDR hdr);
	ptrOSPFLSAHDR OSPF_LSA_HDR_COPY(ptrOSPFLSAHDR lsa);
	void OSPF_LSA_HDR_FREE(ptrOSPFLSAHDR lsa);

	/*
	A.4.2 Router-LSAs
	0                   1                   2                   3
	0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|            LS age             |     Options   |       1       |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                        Link State ID                          |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                     Advertising Router                        |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                     LS sequence number                        |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|         LS checksum           |             length            |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|    0    |V|E|B|        0      |            # links            |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                          Link ID                              |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                         Link Data                             |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|     Type      |     # TOS     |            metric             |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                              ...                              |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|      TOS      |        0      |          TOS  metric          |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                          Link ID                              |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                         Link Data                             |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                              ...                              |
	*/
	typedef struct stru_ospf_rlsa_link
	{
		OSPFID linkId;
		OSPFID linkData;
		OSPFLINKTYPE type;
		UINT8 tosCount;
		UINT16 metric;
		UINT8* TOS;
		UINT8* TOSMetric;
	}OSPFRLSALINK,*ptrOSPFRLSALINK;
	#define OSPFRLSALINK_LEN_FIXED 12 //Bytes

	struct stru_ospf_router_lsa
	{
		UINT8 VEB;
		UINT8 reserved;
		UINT16 linksCount;
		ptrOSPFRLSALINK* rlsaLink;
	};
#define OSPFRLSA_LEN_FIXED 4 //Bytes
	void OSPFLSAINFO_FREE_RLSA(ptrOSPFRLSA rlsa);
	ptrOSPFRLSA OSPFLSAINFO_COPY_RLSA(ptrOSPFRLSA rlsa);

	//Function prototype
	NetSim_PACKET* OSPF_PACKET_NEW(double time,
								   OSPFMSG type,
								   NETSIM_ID d,
								   NETSIM_ID in);
	void OSPF_SEND_PACKET(NetSim_PACKET* packet);
	bool validate_ospf_packet(NetSim_PACKET* packet,
							  NETSIM_ID d,
							  NETSIM_ID in);

#ifdef  __cplusplus
}
#endif
#endif //_NETSIM_OSPF_MSG_H_