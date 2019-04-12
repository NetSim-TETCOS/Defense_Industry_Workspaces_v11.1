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
#include "Ethernet.h"
#include "Ethernet_enum.h"

char* GetStringEthernet_Subevent(NETSIM_ID);

#pragma comment(lib,"Ethernet.lib")

//Function prototype
int fn_NetSim_Ethernet_Configure_F(void **var);
int fn_NetSim_Ethernet_Init_F(struct stru_NetSim_Network* net,
						 NetSim_EVENTDETAILS* pevent,
						 char* appPath,
						 char* iopath,
						 int version,
						 void** fnPointer);
int fn_NetSim_Ethernet_CopyPacket_F(NetSim_PACKET* dst,
							   NetSim_PACKET* src);
int fn_NetSim_Ethernet_FreePacket_F(NetSim_PACKET* packet);
int fn_NetSim_Ethernet_Finish_F();
int fn_NetSim_Ethernet_Metrics_F(PMETRICSWRITER metricsWriter);
char* fn_NetSim_Ethernet_ConfigPacketTrace_F(const void* xmlNetSimNode);
int fn_NetSim_Ethernet_WritePacketTrace_F(NetSim_PACKET* pstruPacket, char** ppszTrace);

int fn_NetSim_Ethernet_HandleMacOut();
int fn_NetSim_Ethernet_HandleMacIn();
int fn_NetSim_Ethernet_HandlePhyOut();
int fn_NetSim_Ethernet_HandlePhyIn();
static int fn_NetSim_Ethernet_HandleTimer();

/**
This function is called by NetworkStack.dll, whenever the event gets triggered
inside the NetworkStack.dll for the Mac/Phy layer Ethernet protocol
It includes MAC_OUT, MAC_IN, PHY_OUT, PHY_IN and TIMER_EVENT.
*/
_declspec (dllexport) int fn_NetSim_Ethernet_Run()
{
	switch (pstruEventDetails->nEventType)
	{
	case MAC_OUT_EVENT:
		return fn_NetSim_Ethernet_HandleMacOut();
		break;
	case MAC_IN_EVENT:
		return fn_NetSim_Ethernet_HandleMacIn();
		break;
	case PHYSICAL_OUT_EVENT:
		return fn_NetSim_Ethernet_HandlePhyOut();
		break;
	case PHYSICAL_IN_EVENT:
		return fn_NetSim_Ethernet_HandlePhyIn();
		break;
	case TIMER_EVENT:
		return fn_NetSim_Ethernet_HandleTimer();
		break;
	default:
		fnNetSimError("Unknown event %d for Ethernet protocolin %s\n",
					  pstruEventDetails->nEventType,
					  __FUNCTION__);
		return -1;
	}
}

/**
This function is called by NetworkStack.dll, while configuring the device
Mac/Phy layer for Ethernet protocol.
*/
_declspec(dllexport) int fn_NetSim_Ethernet_Configure(void** var)
{
	return fn_NetSim_Ethernet_Configure_F(var);
}

/**
This function initializes the Ethernet parameters.
*/
_declspec (dllexport) int fn_NetSim_Ethernet_Init(struct stru_NetSim_Network *NETWORK_Formal,
											 NetSim_EVENTDETAILS *pstruEventDetails_Formal,
											 char *pszAppPath_Formal,
											 char *pszWritePath_Formal,
											 int nVersion_Type,
											 void **fnPointer)
{
	return fn_NetSim_Ethernet_Init_F(NETWORK_Formal,
								pstruEventDetails_Formal,
								pszAppPath_Formal,
								pszWritePath_Formal,
								nVersion_Type,
								fnPointer);
}

/**
This function is called by NetworkStack.dll, once simulation end to free the
allocated memory for the network.
*/
_declspec(dllexport) int fn_NetSim_Ethernet_Finish()
{
	return fn_NetSim_Ethernet_Finish_F();
}

/**
This function is called by NetworkStack.dll, while writing the event trace
to get the sub event as a string.
*/
_declspec (dllexport) char *fn_NetSim_Ethernet_Trace(int nSubEvent)
{
	return (GetStringEthernet_Subevent(nSubEvent));
}

/**
This function is called by NetworkStack.dll, to free the Ethernet protocol
*/
_declspec(dllexport) int fn_NetSim_Ethernet_FreePacket(NetSim_PACKET* pstruPacket)
{
	return fn_NetSim_Ethernet_FreePacket_F(pstruPacket);
}

/**
This function is called by NetworkStack.dll, to copy the Ethernet protocol from source packet to destination.
*/
_declspec(dllexport) int fn_NetSim_Ethernet_CopyPacket(NetSim_PACKET* pstruDestPacket, NetSim_PACKET* pstruSrcPacket)
{
	return fn_NetSim_Ethernet_CopyPacket_F(pstruDestPacket, pstruSrcPacket);
}

/**
This function write the Metrics in Metrics.txt
*/
_declspec(dllexport) int fn_NetSim_Ethernet_Metrics(PMETRICSWRITER metricsWriter)
{
	return fn_NetSim_Ethernet_Metrics_F(metricsWriter);
}

/**
This function will return the string to write packet trace heading.
*/
_declspec(dllexport) char* fn_NetSim_Ethernet_ConfigPacketTrace(const void* xmlNetSimNode)
{
	return fn_NetSim_Ethernet_ConfigPacketTrace_F(xmlNetSimNode);
}

/**
This function will return the string to write packet trace.
*/
_declspec(dllexport) int fn_NetSim_Ethernet_WritePacketTrace(NetSim_PACKET* pstruPacket, char** ppszTrace)
{
	return fn_NetSim_Ethernet_WritePacketTrace_F(pstruPacket, ppszTrace);
}

static int fn_NetSim_Ethernet_HandleTimer()
{
	switch (pstruEventDetails->nSubEventType)
	{
		case ETH_IF_UP:
			notify_interface_up(pstruEventDetails->nDeviceId, pstruEventDetails->nInterfaceId);
			break;
	default:
		fnNetSimError("Unknown subevent %d in %s\n",
					  pstruEventDetails->nSubEventType,
					  __FUNCTION__);
		break;
	}
	return 0;
}
