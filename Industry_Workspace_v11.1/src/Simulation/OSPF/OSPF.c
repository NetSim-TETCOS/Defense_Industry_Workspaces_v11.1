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
#include "main.h"
#include "OSPF.h"
#include "OSPF_enum.h"
#include "OSPF_Msg.h"

#pragma warning(disable:4028)
char* GetStringOSPF_Subevent(NETSIM_ID);
#pragma warning(default:4028)
int fn_NetSim_OSPF_Configure_F(void** var);
int fn_NetSim_OSPF_Init_F(struct stru_NetSim_Network* net,
						  NetSim_EVENTDETAILS* pevent,
						  char* appPath,
						  char* iopath,
						  int version,
						  void** fnPointer);
int fn_NetSim_OSPF_Finish_F();
int fn_NetSim_OSPF_FreePacket_F(NetSim_PACKET* packet);
int fn_NetSim_OSPF_CopyPacket_F(NetSim_PACKET* dst,
								NetSim_PACKET* src);
int fn_NetSim_OSPF_Metrics_F(PMETRICSWRITER metricsWriter);
char* fn_NetSim_OSPF_ConfigPacketTrace_F(const void* xmlNetSimNode);
int fn_NetSim_OSPF_WritePacketTrace_F(NetSim_PACKET* pstruPacket, char** ppszTrace);
static void register_ospf_log();

#pragma comment(lib,"OSPF.lib")

//Function prototype
static void ospf_handle_appin_event();
static void ospf_handle_timer_event();
void ospf_handle_InterfaceUp_Event();

_declspec (dllexport) int fn_NetSim_OSPF_Run()
{
	switch (pstruEventDetails->nEventType)
	{
		case TIMER_EVENT:
			ospf_handle_timer_event();
			break;
		case APPLICATION_IN_EVENT:
			ospf_handle_appin_event();
			break;
		default:
			fnNetSimError("Unknown event type %d for OSPF protocol\n",
						  pstruEventDetails->nEventType);
			break;
	}
	return 0;
}

_declspec(dllexport) int fn_NetSim_OSPF_Configure(void** var)
{
	return fn_NetSim_OSPF_Configure_F(var);
}

_declspec (dllexport) int fn_NetSim_OSPF_Init(struct stru_NetSim_Network *NETWORK_Formal,
												  NetSim_EVENTDETAILS *pstruEventDetails_Formal,
												  char *pszAppPath_Formal,
												  char *pszWritePath_Formal,
												  int nVersion_Type,
												  void **fnPointer)
{
	register_ospf_log();
	return fn_NetSim_OSPF_Init_F(NETWORK_Formal,
									 pstruEventDetails_Formal,
									 pszAppPath_Formal,
									 pszWritePath_Formal,
									 nVersion_Type,
									 fnPointer);
}

_declspec(dllexport) int fn_NetSim_OSPF_Finish()
{
	return fn_NetSim_OSPF_Finish_F();
}

_declspec (dllexport) char *fn_NetSim_OSPF_Trace(int nSubEvent)
{
	return (GetStringOSPF_Subevent(nSubEvent));
}

_declspec(dllexport) int fn_NetSim_OSPF_FreePacket(NetSim_PACKET* pstruPacket)
{
	return fn_NetSim_OSPF_FreePacket_F(pstruPacket);
}

_declspec(dllexport) int fn_NetSim_OSPF_CopyPacket(NetSim_PACKET* pstruDestPacket, NetSim_PACKET* pstruSrcPacket)
{
	return fn_NetSim_OSPF_CopyPacket_F(pstruDestPacket, pstruSrcPacket);
}

_declspec(dllexport) int fn_NetSim_OSPF_Metrics(PMETRICSWRITER metricsWriter)
{
	return fn_NetSim_OSPF_Metrics_F(metricsWriter);
}

_declspec(dllexport) char* fn_NetSim_OSPF_ConfigPacketTrace(const void* xmlNetSimNode)
{
	return fn_NetSim_OSPF_ConfigPacketTrace_F(xmlNetSimNode);
}

_declspec(dllexport) int fn_NetSim_OSPF_WritePacketTrace(NetSim_PACKET* pstruPacket, char** ppszTrace)
{
	return fn_NetSim_OSPF_WritePacketTrace_F(pstruPacket, ppszTrace);
}

static void ospf_handle_timer_event()
{
	if (OSPF_IS_SUBEVENT(pstruEventDetails->nSubEventType))
		OSPF_CALL_SUBEVENT(pstruEventDetails->nSubEventType);
	else
	{
		fnNetSimError("OSPF: Event call back function is not defined for %s event\n",
					  GetStringOSPF_Subevent(pstruEventDetails->nSubEventType));
	}
}

static void ospf_handle_appin_event()
{
	NetSim_PACKET* packet = pstruEventDetails->pPacket;
	NETSIM_ID d = pstruEventDetails->nDeviceId;
	NETSIM_ID in = pstruEventDetails->nInterfaceId;
	if (!validate_ospf_packet(packet, d, in))
	{
		fnNetSimError("OSPF Packet validation is failed received on %d-%d\n", d, in);
		return;
	}

	ptrOSPFPACKETHDR hdr = OSPF_PACKET_GET_HDR(packet);
	switch (hdr->Type)
	{
		case OSPFMSG_HELLO:
			ospf_process_hello();
			break;
		case OSPFMSG_DD:
			ospf_handle_DD();
			break;
		case OSPFMSG_LSUPDATE:
			ospf_handle_LSUPDATE();
			break;
		case OSPFMSG_LSREQUEST:
			ospf_handle_LSRequest();
			break;
		case OSPFMSG_LSACK:
			ospf_handle_LSAck();
			break;
		default:
			fnNetSimError("Unknown ospf msg %d arrives to router %d-%d\n",
						  hdr->Type,
						  d, in);
			break;
	}
}

bool isOSPFHelloLog()
{
	return isOSPFHelloDebug;
}

bool isOSPFSPFLog()
{
	return isOSPFSPFDebug;
}

char logId[BUFSIZ];
char* form_dlogId(char* name,
				 NETSIM_ID d)
{
	sprintf(logId, "%s_%d",
			name, d);
	return logId;
}

static void register_ospf_log()
{
	/*init_ospf_dlog("LSDB_1", "LSDB_1");
	init_ospf_dlog("RXTLIST_1", "RXTLIST_1");
	init_ospf_dlog("RLSALOG_2", "RLSALOG_2");
	init_ospf_dlog("RXTLIST_2", "RXTLIST_2");
	init_ospf_dlog("RCVLSU_1", "RCVLSU_1");
	init_ospf_dlog("RCVLSU_2", "RCVLSU_2");*/
	//init_ospf_dlog("LSULIST_1", "LSULIST_1");
	//init_ospf_dlog("OSPFROUTE_1", "OSPFROUTE_1");
}