#pragma once
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

#ifndef _NETSIM_OSPF_TYPEDEF_H_
#define _NETSIM_OSPF_TYPEDEF_H_
#ifdef  __cplusplus
extern "C" {
#endif

	// Struct typedef
	typedef struct stru_ospf_neighbor OSPF_NEIGHBOR, *ptrOSPF_NEIGHBOR;
	typedef struct stru_ospf_pds OSPF_PDS, *ptrOSPF_PDS;
	typedef struct stru_ospf_hello OSPFHELLO, *ptrOSPFHELLO;
	typedef struct stru_ospf_if OSPF_IF, *ptrOSPF_IF;
	typedef struct stru_ospf_packet_header OSPFPACKETHDR, *ptrOSPFPACKETHDR;
	typedef struct stru_lsa_header OSPFLSAHDR, *ptrOSPFLSAHDR;
	typedef struct stru_ospf_router_lsa OSPFRLSA,*ptrOSPFRLSA;
	typedef struct ospf_routingTables OSPFROUTINGTABLE, *ptrOSPFROUTINGTABLE;
	typedef struct stuc_arraylist ARRAYLIST, *ptrARRAYLIST;

	// Enum typedef
	typedef enum enum_ospf_neighbor_state OSPFNEIGHSTATE;
	typedef enum enum_ospf_dest_type OSPFDESTTYPE;
	typedef enum enum_ls_type LSTYPE;
	typedef enum enum_if_state OSPFIFSTATE;
	typedef enum enum_ospf_msg OSPFMSG;

	//Other typedef and macros
	typedef NETSIM_IPAddress OSPFID;
#define OSPFID_COMPARE IP_COMPARE
#define OSPFID_ISGREATER(id1,id2) ((id1)->int_ip[0] > (id2)->int_ip[0])
#define OSPFID_ISSMALLER(id1,id2) ((id1)->int_ip[0] < (id2)->int_ip[0])

	typedef double OSPFTIME;
#define OSPFTIME_TO_UINT16(time) (UINT16)(time)
#define OSPFTIME_FROM_UINT16(time) (OSPFTIME)(time)

	typedef void* ptrOSPFLIST;
#ifdef  __cplusplus
}
#endif
#endif //_NETSIM_OSPF_TYPEDEF_H_