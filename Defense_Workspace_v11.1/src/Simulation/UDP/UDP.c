/*---------------------------------------------------------------------------------
* Copyright (C) 2013															  *
*																				  *
* TETCOS, Bangalore. India                                                        *
*
* Tetcos owns the intellectual property rights in the Product and its content.    *
* The copying, redistribution, reselling or publication of any or all of the      *
* Product or its content without express prior written consent of Tetcos is       *
* prohibited. Ownership and / or any other right relating to the software and all *
* intellectual property rights therein shall remain at all times with Tetcos.     *

* Author:   P.Sathishkumar 
* Date  :    05-Mar-2013
* --------------------------------------------------------------------------------*/

#include "main.h"
#include "UDP.h"

/**
		This function is called by NetworkStack.dll, whenever the event gets triggered	
		inside the NetworkStack.dll for the Transport layer UDP protocol
*/
_declspec (dllexport) int fn_NetSim_UDP_Run()
{
	nEventType=pstruEventDetails->nEventType;	/* Get the EventType from Event details */

	/*Check  event type*/
	switch(nEventType)
	{
	case TRANSPORT_OUT_EVENT:
		/* Send the user datagram */
		fn_NetSim_UDP_Send_User_Datagram(pstruEventDetails,NETWORK);
		break;
	case TRANSPORT_IN_EVENT:
		/* Receive the user datagram */
		fn_NetSim_UDP_Receive_User_Datagram(pstruEventDetails,NETWORK);
		break;	
	default:
		fnNetSimError("UDP-- Unknown event type for UDP protocol\n");
		break;
	}
	return 0;
}

/********************************************************************************/
/* The Following functions are present in the libUDP.lib.						*/
/* so NetworkStack.dll can not call libUDP.lib, NetworkStack.dll is calling 	*/
/* libUDP.dll. From libUDP.dll, libUDP.lib functions are called.	            */
/********************************************************************************/
/**
	This function is called by NetworkStack.dll, while configuring the device 
	TRANSPORT layer for UDP protocol.	
*/
_declspec(dllexport) int fn_NetSim_UDP_Configure(void** var)
{
	return fn_NetSim_UDP_Configure_F(var);
}

/**
	This functon initializes the UDP parameters. 
*/
_declspec (dllexport) int fn_NetSim_UDP_Init(struct stru_NetSim_Network *NETWORK_Formal,\
											 NetSim_EVENTDETAILS *pstruEventDetails_Formal,char *pszAppPath_Formal,\
											 char *pszWritePath_Formal,int nVersion_Type,void **fnPointer)
{
	fn_NetSim_UDP_Init_F(NETWORK_Formal,pstruEventDetails_Formal,pszAppPath_Formal,\
		pszWritePath_Formal,nVersion_Type,fnPointer);
	return 0;
}
/**
	This function is called by NetworkStack.dll, once simulation end to free the 
	allocated memory for the network.	
*/
_declspec(dllexport) int fn_NetSim_UDP_Finish()
{
	fn_NetSim_UDP_Finish_F();
	return 0;
}	

/************************************************************************************/
/* This function will return the string to wrie packet trace heading				*/																			
/************************************************************************************/
/**
	This function will return the string to wrie packet trace heading
*/
_declspec(dllexport) char* fn_NetSim_UDP_ConfigPacketTrace()
{
	return "";
}

/************************************************************************************/
/* This function will return the string to wrie packet trace						*/																			
/************************************************************************************/
_declspec(dllexport) int fn_NetSim_UDP_WritePacketTrace(NetSim_PACKET* pstruPacket,char** ppszTrace)
{
	return 1;
}
/**
	This function is called by NetworkStack.dll, while writing the evnt trace 
	to get the sub event as a string.
*/
_declspec(dllexport) char *fn_NetSim_UDP_Trace(int nSubEvent)
{
	return (fn_NetSim_UDP_Trace_F(nSubEvent));
}

_declspec(dllexport) int fn_NetSim_UDP_FreePacket(NetSim_PACKET* pstruPacket)
{
	return fn_NetSim_UDP_FreePacket_F(pstruPacket);	
}
/**
	This function is called by NetworkStack.dll, to free the packet of UDP protocol
*/
_declspec(dllexport) int fn_NetSim_UDP_CopyPacket(NetSim_PACKET* pstruDestPacket,NetSim_PACKET* pstruSrcPacket)
{
	return fn_NetSim_UDP_CopyPacket_F(pstruDestPacket,pstruSrcPacket);	
}
/** This function write the Metrics in Metrics.txt */
_declspec(dllexport) int fn_NetSim_UDP_Metrics(PMETRICSWRITER metricsWriter)
{
	return fn_NetSim_UDP_Metrics_F(metricsWriter);
}
