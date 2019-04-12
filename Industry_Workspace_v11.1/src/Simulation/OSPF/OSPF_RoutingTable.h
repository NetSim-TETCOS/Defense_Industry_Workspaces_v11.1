#pragma once
/************************************************************************************
* Copyright (C) 2018                                                               *
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

#ifndef _NETSIM_OSPF_ROUTINGTABLE_H_
#define _NETSIM_OSPF_ROUTINGTABLE_H_
#ifdef  __cplusplus
extern "C" {
#endif

#define OSPF_LS_INFINITY 0xFFFFFF

#ifndef ptrIP_ROUTINGTABLE
#define ptrIP_ROUTINGTABLE void*
#endif

enum enum_ospf_dest_type
{
	OSPFDESTTYPE_ABR,
	OSPFDESTTYPE_ASBR,
	OSPFDESTTYPE_ABR_ASBR,
	OSPFDESTTYPE_NETWORK,
	OSPFDESTTYPE_ROUTER
};

typedef enum
{
	OSPFPATHTYPE_INTRA_AREA,
	OSPFPATHTYPE_INTER_AREA,
	OSPFPATHTYPE_TYPE1_EXTERNAL,
	OSPFPATHTYPE_TYPE2_EXTERNAL
} OSPFPATHTYPE;

typedef enum
{
	OSPFROUTEFLAG_INVALID,
	OSPFROUTEFLAG_CHANGED,
	OSPFROUTEFLAG_NO_CHANGE,
	OSPFROUTEFLAG_NEW
} OSPFROUTEFLAG;

// A row struct for routing table
typedef struct ospf_routingTableRow
{
	OSPFDESTTYPE destType;
	NETSIM_IPAddress destAddr;
	NETSIM_IPAddress addrMask;
	char option;
	OSPFID areaId;
	OSPFPATHTYPE pathType;
	UINT metric;
	UINT type2Metric;
	void* LSOrigin;
	NETSIM_IPAddress nextHop;
	NETSIM_ID nextHopId;
	NETSIM_IPAddress outInterface;
	NETSIM_ID outInterfaceId;
	NETSIM_IPAddress advertisingRouter;
	OSPFROUTEFLAG flag;
}OSPFROUTINGTABLEROW,*ptrOSPFROUTINGTABLEROW;

struct ospf_routingTables
{
	UINT numRow;
	ptrOSPFROUTINGTABLEROW* rows;
};

typedef enum
{
	OSPFVERTEXTYPE_ROUTER,
	OSPFVERTEXTYPE_NETWORK
} OSPFVERTEXTYPE;
static char strOSPFVERTEXTYPE[][20] = { "VERTEX_ROUTER","VERTEX_NETWORK" };

typedef struct
{
	NETSIM_ID outIntf;
	NETSIM_IPAddress nextHop;
} OSPFNEXTHOPLISTITEM, *ptrOSPFNEXTHOPLISTITEM;

typedef struct
{
	NETSIM_IPAddress vertexId;
	OSPFVERTEXTYPE vertexType;
	ptrOSPFLSAHDR lsa;
	ptrOSPFLIST nextHopList;
	UINT distance;
} OSPFVERTEX,*ptrOSPFVERTEX;

//OSPF Routing table
void ospf_rtTable_addRoute(ptrOSPF_PDS ospf,
						   ptrOSPFROUTINGTABLEROW newRoute);
void ospf_rtTable_freeRoute(ptrOSPF_PDS ospf,
							ptrOSPFROUTINGTABLEROW row);
void ospf_rtTable_freeAllInvalidRoute(ptrOSPF_PDS ospf);
ptrOSPFROUTINGTABLEROW ospf_rtTable_getValidHostRoute(ptrOSPF_PDS ospf,
													  NETSIM_IPAddress destAddr,
													  OSPFDESTTYPE destType);
ptrOSPFROUTINGTABLEROW ospf_rtTable_getValidRoute(ptrOSPF_PDS ospf,
												  NETSIM_IPAddress destAddr,
												  OSPFDESTTYPE destType);
void ospf_rtTable_updateIPTable(ptrOSPF_PDS ospf);

#ifdef  __cplusplus
}
#endif
#endif //_NETSIM_OSPF_ROUTINGTABLE_H_
