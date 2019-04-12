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

#ifndef _NETSIM_OSPF_H_
#define _NETSIM_OSPF_H_
#ifdef  __cplusplus
extern "C" {
#endif

#pragma comment(lib,"NetworkStack.lib")
#pragma comment(lib,"Metrics.lib")

#include "OSPF_Typedef.h"

	//DEBUG Flag
	static bool isOSPFHelloDebug = true;
	static bool isOSPFSPFDebug = true;
	typedef enum
	{
		OSPF_LOG,
		OSPF_HELLO_LOG,
		OSPF_SPF_LOG,
	}OSPFLOGFLAG;

//Useful macro
#define GET_LOWER_IP(ip1,ip2) ((ip1) ? ((ip2) ? ((ip1)->int_ip < (ip2)->int_ip ? (ip1) : (ip2)) : (ip1)) : (ip2));
#define DEVICE_CURR_IP	(DEVICE_NWADDRESS(pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId))
#define DEVICE_CURR_MASK	(DEVICE_SUBNETMASK(pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId))

#define OSPF_CURR_TIME()	(pstruEventDetails->dEventTime)

#define strAllSPFRouters	"224.0.0.5"
#define strAllDRouters		"224.0.0.6"
#define strNULLIP			"0.0.0.0"
#define strInvalidAreaId	"255.255.255.254" //0xFFFFFFFE
#define strSingleAreaId		"255.255.255.255" //0xFFFFFFFF
#define strBackboneAreaId	"0.0.0.0" //0x00
	NETSIM_IPAddress AllSPFRouters;
	NETSIM_IPAddress AllDRouters;
	OSPFID NullID;
	OSPFID invalidAreaId;
	OSPFID backboneAreaId;
	OSPFID singleAreaId;
#define ANY_DEST	(GET_BROADCAST_IP(4))

#define OSPF_BROADCAST_JITTER	(0*MILLISECOND)
#define OSPF_DO_NOT_AGE			(0x8000)
#define OSPF_LSA_MAX_AGE_DIFF   (15 * MINUTE)

	typedef enum enum_ospf_router_type
	{
		OSPFRTYPE_INTERNAL,
		OSPFRTYPE_ABR,
		OSPFRTYPE_BACKBONE,
		OSPFRTYPE_ASBOUNDARY,
	}OSPFRTYPE;

	typedef enum enum_address_range_status
	{
		ADDRRNSTATUS_ADVERTISE,
		ADDRRNSTATUS_DONOTADVERTISE,
	}ADDR_RN_STATUS;

	typedef struct stru_address_range
	{
		NETSIM_IPAddress address;
		NETSIM_IPAddress mask;
		ADDR_RN_STATUS status;
	}ADDR_RN, *ptrADDR_RN;

	typedef struct stru_area_ds
	{
		OSPFID areaId;
		ptrOSPFLIST addressRangeList;
		UINT assocInterfaceCount;
		NETSIM_IPAddress* assocRouterInterface;
		NETSIM_ID* assocRouterInterfaceId;

		ptrOSPFLIST routerLSAList;
		ptrOSPFLIST nwLSAList;
		ptrOSPFLIST routerSummaryLSAList;
		ptrOSPFLIST nwSummaryLSAList;
		ptrOSPFLIST maxAgeList;
		bool extRoutingCapability;

		bool isRouterLSTimer;
		double lastLSAOriginateTime;

		ptrOSPFLIST shortestPathList;
	}OSPFAREA_DS,*ptrOSPFAREA_DS;
	ptrOSPFAREA_DS OSPF_AREA_GET(ptrOSPF_PDS ospf,
								 NETSIM_ID in,
								 OSPFID areaId,
								 NETSIM_IPAddress interfaceIP); //Pass only one
#define OSPF_AREA_GET_IN(ospf,in) (OSPF_AREA_GET((ospf),(in),NULL,NULL))
#define OSPF_AREA_GET_ID(ospf,id) (OSPF_AREA_GET((ospf),0,(id),NULL))
#define OSPF_AREA_GET_IP(ospf,ip) (OSPF_AREA_GET((ospf),0,NULL,(ip)))
	void OSPF_AREA_SET(ptrOSPF_PDS ospf,
					   ptrOSPFAREA_DS area);

	struct stru_ospf_pds
	{
		NETSIM_ID myId;
		OSPFID routerId;
		OSPFRTYPE routerType;
		UINT areaCount;
		UINT ifCount;
		ptrOSPF_IF* ospfIf;
		ptrOSPFAREA_DS* areaDS;
		double minLSInterval;
		bool supportDC;
		double dFloodTimer;
		bool isAdvertSelfIntf;

		UINT16 LSAMaxAge;
		bool isMaxAgeRemovalTimerSet;
		double maxAgeRemovalTime;

		double LSRefreshTime;

		double SPFTimer;
		double spfCalcDelay;
		ptrOSPFROUTINGTABLE routingTable;
		UINT ipTableCount;
		void** ipTable;

		bool isPartitionedIntoArea;

		// Enabling this might reduce some duplicate LSA transmission in a network.
		// When enable this, a node wait for time [0, OSPFv2_FLOOD_TIMER] before
		// sending Update in response to a Request. Thus at synchronization time
		// a node send less number of Update packet in response to several duplicate
		// Request from different node in the network. This intern also reduce
		// number of Ack packet transmitted in the network.
		bool isSendDelayedUpdate;

		double incrementTime;
	};
#define OSPF_PDS_SET(d,ospf) (fn_NetSim_Stack_SetAppProtocolData(d,APP_PROTOCOL_OSPF,ospf))
#define OSPF_PDS_GET(d) ((ptrOSPF_PDS)(DEVICE_APPVAR(d,APP_PROTOCOL_OSPF)))
#define OSPF_PDS_CURRENT() (OSPF_PDS_GET(pstruEventDetails->nDeviceId))

	typedef struct stru_event_details_lsdb
	{
		LSTYPE lsType;
		ptrOSPFAREA_DS area;
		bool isFlush;

		ptrOSPF_NEIGHBOR neigh; // For RLSA
		UINT nwLSASeqNum; // For NWLSA
						  //For summary LSA
		NETSIM_IPAddress destAddr;
		NETSIM_IPAddress destMask;
		OSPFDESTTYPE destType;
	}EVENTLSDB, *ptrEVENTLSDB;

	//Function prototype
	//Hello
	void ospf_process_hello();
	void ospf_handle_helloTimer_event();
	void start_interval_hello_timer();

	//DD
	void start_sending_dd_msg();
	void ospf_handle_DD();

	//OSPF Interface
	ptrOSPF_IF OSPF_IF_GET(ptrOSPF_PDS ospf, NETSIM_ID ifId);
#define OSPF_IF_CURRENT() (OSPF_IF_GET(OSPF_PDS_CURRENT(), pstruEventDetails->nInterfaceId))
	void ospf_handle_interfaceUp_event();
	void ospf_handle_interfaceDown_event();
	void ospf_interface_handleMultipleInterfaceEvent();

	//OSPF Neighbor
	ptrOSPF_NEIGHBOR OSPF_NEIGHBOR_FIND(ptrOSPF_IF ospf, OSPFID id);
	ptrOSPF_NEIGHBOR OSPF_NEIGHBOR_FIND_BY_IP(ptrOSPF_IF thisInterface, NETSIM_IPAddress ip);
	bool ospf_is_router_fullAdjacentWithDR(ptrOSPF_IF ospf);
	bool ospf_is_dr_router_fulladjacentwithAnother(ptrOSPF_IF ospf);
	void ospf_neighbor_insertToSendList(ptrOSPFLIST list,
										ptrOSPFLSAHDR lsa,
										double time);
	ptrOSPFLSAHDR ospf_neighbor_searchSendList(ptrOSPFLIST list,
											   ptrOSPFLSAHDR lsa);
	bool ospf_neighbor_isAnyNeighborInExchangeOrLoadingState(ptrOSPF_PDS ospf);
	NETSIM_ID ospf_neighbor_getInterfaceIdforThisNeighbor(ptrOSPF_PDS ospf,
														  NETSIM_IPAddress neighIPaddr);
	ptrOSPF_NEIGHBOR ospf_neighbor_new(NETSIM_IPAddress ip,
									   OSPFID rid);
	void ospf_neighbor_add(ptrOSPF_IF ospf, ptrOSPF_NEIGHBOR neigh);

	// OSPF Neighbor event handler
	void ospf_neighbor_handle_helloReceived_event();
	void ospf_neighbor_handle_1way_event();
	void ospf_neighbor_handle_2wayReceived_event();
	void ospf_neighbor_handle_negotiationDone_event();
	void ospf_neighbor_handle_exchangeDone_event();
	void ospf_neighbor_handle_start_event();
	void ospf_neighbor_handle_inactivityTimer_event();
	void ospf_neighbor_handle_LoadingDoneEvent();
	void ospf_neighbor_handle_KillNbrEvent();

	//OSPF Area
	void ospf_area_init(NETSIM_ID d, NETSIM_ID in);
	void ospf_area_handleABRTask(ptrOSPF_PDS ospf);

	// LSA Header
	void ospf_lsahdr_add_lsa(ptrOSPFLSAHDR lhdr,
							 void* lsa,
							 UINT16 len);
	bool ospf_lsa_update_lsahdr(ptrOSPF_PDS ospf,
								ptrOSPFAREA_DS area,
								ptrOSPFLSAHDR lsa,
								ptrOSPFLSAHDR old,
								LSTYPE lstype);

	//LSA
	void ospf_lsa_print(char* logid,
						ptrOSPFLSAHDR LSHeader,
						char* msg);
	void ospf_lsa_printList(char* logid,
							ptrOSPFLIST list,
							char* name);
	void ospf_lsa_schedule_routerLSA(ptrOSPF_PDS ospf,
									 ptrOSPFAREA_DS area,
									 bool isFlush);
	void ospf_lsa_scheduleNWLSA(ptrOSPF_PDS ospf,
								ptrOSPF_IF thisInterface,
								ptrOSPFAREA_DS area,
								bool isFlush);
	void ospf_lsa_scheduleSummaryLSA(ptrOSPF_PDS ospf,
									 ptrOSPF_IF thisInterface,
									 ptrOSPFAREA_DS area,
									 NETSIM_IPAddress destAddr,
									 NETSIM_IPAddress destMask,
									 OSPFDESTTYPE destType,
									 bool isFlush);
	void ospf_lsa_schedule(ptrOSPF_PDS ospf,
						   ptrOSPF_IF thisInterface,
						   ptrOSPFAREA_DS area,
						   ptrOSPFLSAHDR lsa);
	void ospf_lsa_ScheduleLSDB();
	void ospf_lsa_handle_floodTimer_event();
	UINT16 ospf_lsa_maskDoNotAge(ptrOSPF_PDS ospf,
								 UINT16 routerLSAAge);
	void ospf_lsa_assignNewLSAge(ptrOSPF_PDS ospf,
								 UINT16* routerLSAAge,
								 UINT16 newLSAAge);
	int ospf_lsa_compare(ptrOSPF_PDS ospf,
						 ptrOSPFLSAHDR oldLS,
						 ptrOSPFLSAHDR newLS);
	bool ospf_lsa_compareToListMem(ptrOSPF_PDS ospf,
								   ptrOSPFLSAHDR oldLS,
								   ptrOSPFLSAHDR newLS);
	int ospf_lsa_isMoreRecent(ptrOSPF_PDS ospf, ptrOSPFLSAHDR newLSA, ptrOSPFLSAHDR oldLSA);
	bool ospf_lsa_isSelfOriginated(ptrOSPF_PDS ospf,
								   ptrOSPFLSAHDR lsa);
	void ospf_lsa_queueToFlood(ptrOSPF_PDS pds,
							   ptrOSPF_IF ospf,
							   ptrOSPFLSAHDR lsa);
	bool ospf_lsa_flood(ptrOSPF_PDS pds,
						OSPFID area,
						ptrOSPFLSAHDR lsa,
						NETSIM_IPAddress srcAddr,
						NETSIM_ID in);
	void ospf_lsa_flush(ptrOSPF_PDS ospf,
						ptrOSPFAREA_DS area,
						ptrOSPFLSAHDR lsa);
	bool ospf_lsa_is_content_changed(ptrOSPF_PDS ospf,
									 ptrOSPFLSAHDR newLSA,
									 ptrOSPFLSAHDR oldLSA);
	void ospf_lsa_assignNewLSAIntoLSOrigin(ptrOSPF_PDS pds,
										   ptrOSPFLSAHDR LSA,
										   ptrOSPFLSAHDR newLSA);
	bool ospf_lsa_hasMaxAge(ptrOSPF_PDS ospf,
							ptrOSPFLSAHDR lsa);
	bool ospf_lsa_checkForDoNotAge(ptrOSPF_PDS ospf,
								   UINT16 routerLSAAge);
	void ospf_lsa_addToMaxAgeLSAList(ptrOSPF_PDS ospf,
									 OSPFID areaId,
									 ptrOSPFLSAHDR lsa);
	bool ospf_lsa_hasLink(ptrOSPF_PDS ospf,
						  ptrOSPFLSAHDR wlsa,
						  ptrOSPFLSAHDR vlsa);
	ptrOSPFLSAHDR ospf_lsa_find_old_lsa(ptrOSPFLIST list,
										OSPFID rid,
										OSPFID lid);

	//RLSA
	void ospf_lsa_printRLSA(char* logid,
							ptrOSPFRLSA rlsa);
	bool Ospf_rlsa_getASBRouter(UINT8 VEB);
	bool Ospf_rlsa_getABRouter(UINT8 VEB);
	void ospf_rlsa_originateRouterLSA(ptrOSPFAREA_DS area,
									  bool isFlush);
	bool ospf_rlsa_isBodyChanged(ptrOSPFLSAHDR newLSA,
								 ptrOSPFLSAHDR oldLSA);
	bool ospf_rlsa_hasLink(ptrOSPF_PDS ospf,
						   ptrOSPFLSAHDR wlsa,
						   ptrOSPFLSAHDR vlsa);

	//LSDB
	ptrOSPFLSAHDR ospf_lsdb_lookup_lsaList(ptrOSPFLIST list,
										   OSPFID adverRouter,
										   OSPFID linkStateId);
	ptrOSPFLSAHDR ospf_lsdb_lookup_lsaListByID(ptrOSPFLIST list,
											   OSPFID linkStateId);
	ptrOSPFLSAHDR ospf_lsdb_lookup(ptrOSPF_PDS ospf,
								   ptrOSPFAREA_DS area,
								   LSTYPE lsType,
								   OSPFID adveRouter,
								   OSPFID linkStateID);
	bool ospf_lsdb_install(ptrOSPF_PDS ospf,
						   OSPFID areaId,
						   ptrOSPFLSAHDR lsa,
						   ptrOSPFLIST list);
	bool ospf_lsdb_update(ptrOSPF_PDS ospf,
						  ptrOSPF_IF thisInterface,
						  ptrOSPFLSAHDR lsa,
						  ptrOSPFAREA_DS thisArea,
						  NETSIM_IPAddress srcAddr);
	void ospf_lsdb_scheduleMaxAgeRemovalTimer(ptrOSPF_PDS ospf);
	void ospf_lsdb_handleMaxAgeRemovalTimer();
	void ospf_lsdb_removeLSA(ptrOSPF_PDS ospf,
							 ptrOSPFAREA_DS area,
							 ptrOSPFLSAHDR lsa);
	void ospf_LSDB_handle_IncrementAge_event();

	//LSU
	void ospf_handle_LSUPDATE();
	void ospf_lsupdate_send();
	void ospf_lsu_sendLSUpdateToNeighbor(ptrOSPF_PDS ospf,
										 ptrOSPF_IF thisInterface);
	void ospf_lsupdate_handleRxmtTimer();

	//LSAACK
	void ospf_lsaAck_sendDelayedAck(ptrOSPF_PDS ospf,
									ptrOSPF_IF thisInterface,
									ptrOSPFLSAHDR lsa);
	void ospf_lsaAck_sendDirectAck(ptrOSPF_PDS ospf,
								   NETSIM_ID interfaceId,
								   ptrARRAYLIST ackList,
								   NETSIM_IPAddress nbrAddr);
	void ospf_lsack_handleDelayedAckTimer();
	void ospf_handle_LSAck();

	//LSREQ
	ptrOSPFLIST ospf_lsreq_initList();
	ptrOSPFLSAHDR ospf_lsReq_searchFromList(ptrOSPF_NEIGHBOR neigh,
											ptrOSPFLSAHDR lsa);
	void ospf_lsreq_insertToList(ptrOSPF_NEIGHBOR neigh,
								 ptrOSPFLSAHDR lsHdr,
								 double time);
	void ospf_lsreq_removeFromReqList(ptrOSPFLIST list,
									  ptrOSPFLSAHDR lsa);
	bool ospf_lsreq_isRequestedLSAReceived(ptrOSPF_NEIGHBOR neigh);
	void ospf_lsreq_send(ptrOSPF_PDS ospf,
						 NETSIM_ID interfaceId,
						 NETSIM_IPAddress nbrAddr,
						 bool retx);
	void ospf_lsreq_retransmit(ptrOSPF_PDS ospf,
							   ptrOSPF_IF thisInterface,
							   ptrOSPF_NEIGHBOR neigh,
							   UINT seqNum);
	void ospf_handle_LSRequest();

	//SPF
	void ospf_spf_scheduleCalculation(ptrOSPF_PDS ospf);
	void ospf_spf_handleCalculateSPFEvent();

	//DR
	OSPFIFSTATE ospf_DR_election(ptrOSPF_PDS ospf,
								 ptrOSPF_IF thisInterface);

	//OSPF common function
	bool ospf_isMyAddr(NETSIM_ID d, NETSIM_IPAddress addr);
	NETSIM_ID ospf_getInterfaceIdForNextHop(NETSIM_ID d,
											NETSIM_IPAddress addr);

	//OSPF utility function
	bool isOSPFConfigured(NETSIM_ID d);
	char* form_dlogId(char* name,
					  NETSIM_ID d);
	bool isOSPFSPFLog();
	bool isOSPFHelloLog();
	void print_ospf_log(OSPFLOGFLAG logFlag, char* format, ...);
	void print_ospf_Dlog(char* id, char* format, ...);
	void init_ospf_dlog(char* id,
						char* fileName);
	bool ospf_dlog_isEnable(char* id);
#define ospf_event_add(time,d,in,subevent,packet,eventdata) ospf_event_add_dbg(time,d,in,subevent,packet,eventdata,__LINE__,__FILE__)
	UINT64 ospf_event_add_dbg(double time,
							  NETSIM_ID d,
							  NETSIM_ID in,
							  int subevent,
							  NetSim_PACKET* packet,
							  void* eventData,
							  int line,
							  char* file);
	void ospf_event_set(int subevent);
	void ospf_event_set_and_call(int subevent, void* otherDetails);

#ifdef  __cplusplus
}
#endif
#endif //_NETSIM_OSPF_H_
