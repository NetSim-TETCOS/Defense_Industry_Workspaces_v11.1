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
#include "main.h"
#include "Cellular.h"
#include "CDMA.h"
/** This function is used to initialize the CDMA parameters in a network */
_declspec(dllexport) int fn_NetSim_CDMA_Init(struct stru_NetSim_Network *NETWORK_Formal,
													NetSim_EVENTDETAILS *pstruEventDetails_Formal,
													char *pszAppPath_Formal,
													char *pszWritePath_Formal,
													int nVersion_Type,
													void **fnPointer)
{
	return fn_NetSim_CDMA_Init_F(NETWORK_Formal,
		pstruEventDetails_Formal,
		pszAppPath_Formal,
		pszWritePath_Formal,
		nVersion_Type,
		fnPointer);
}
/** This function is called by the NetworkStack.dll, while configuring the Network for CDMA protocol */
_declspec(dllexport) int fn_NetSim_CDMA_Configure(void** var)
{
	return fn_NetSim_CDMA_Configure_F(var);
}
/**	
This function is called by NetworkStack.dll, which inturn calls the Cellular run function 
which is present in Cellular.c
 */
_declspec (dllexport) int fn_NetSim_CDMA_Run()
{
	return fn_NetSim_Cellular_Run();
}
/**
This function is called by NetworkStack.dll, while writing the event trace 
to get the sub event as a string.
*/
_declspec (dllexport) char *fn_NetSim_CDMA_Trace(int nSubEvent)
{
	switch(nSubEvent%100)
	{
	case Subevent_DropCall:
		return "DropCall";
	case Subevent_TxNextBurst:
		return "TxNextBurst";
	default:
		return "CDMA_UnknownEvent";
	}
}
/**
	This function is called by NetworkStack.dll, to free the CDMA protocol control packets.
*/
_declspec(dllexport) int fn_NetSim_CDMA_FreePacket(NetSim_PACKET* pstruPacket)
{
	return fn_NetSim_Cellular_FreePacket(pstruPacket);
}
/**
	This function is called by NetworkStack.dll, to copy the CDMA protocol
	related information to a new packet 
*/
_declspec(dllexport) int fn_NetSim_CDMA_CopyPacket(NetSim_PACKET* pstruDestPacket,NetSim_PACKET* pstruSrcPacket)
{
	return fn_NetSim_Cellular_CopyPacket(pstruDestPacket,pstruSrcPacket);
}
/**
	This function writes the CDMA metrics in Metrics.txt	
*/
_declspec(dllexport) int fn_NetSim_CDMA_Metrics(char* szMetrics)
{
	return fn_NetSim_CDMA_Metrics_F(szMetrics);
}
/**
	This function is used to configure the packet trace
*/
_declspec(dllexport) char* fn_NetSim_CDMA_ConfigPacketTrace()
{
	return "";
}
/**
	This function is used to write the packet trace																									
*/
_declspec(dllexport) int fn_NetSim_CDMA_WritePacketTrace(NetSim_PACKET* pstruPacket, char** ppszTrace)
{
	return 1;
}
/**
This function is called by NetworkStack.dll, once simulation ends, to free the 
allocated memory for the network.	
*/
_declspec(dllexport) int fn_NetSim_CDMA_Finish()
{
	return 1;
}