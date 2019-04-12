/************************************************************************************
 * Copyright (C) 2016                                                               *
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
#include "Token.h"

//Function prototype
int fn_NetSim_Token_Configure_F(void** var);
int fn_NetSim_Token_Init_F(struct stru_NetSim_Network *NETWORK_Formal,
	NetSim_EVENTDETAILS *pstruEventDetails_Formal,
	char *pszAppPath_Formal,
	char *pszWritePath_Formal,
	int nVersion_Type,
	void **fnPointer);

_declspec(dllexport) int fn_NetSim_Token_Configure(void** var)
{
	return fn_NetSim_Token_Configure_F(var);
}

_declspec (dllexport) int fn_NetSim_Token_Init(struct stru_NetSim_Network *NETWORK_Formal,
	NetSim_EVENTDETAILS *pstruEventDetails_Formal,
	char *pszAppPath_Formal,
	char *pszWritePath_Formal,
	int nVersion_Type,
	void **fnPointer)
{
	return fn_NetSim_Token_Init_F(NETWORK_Formal,pstruEventDetails_Formal,pszAppPath_Formal,
		pszWritePath_Formal,nVersion_Type,fnPointer);
}

_declspec (dllexport) int fn_NetSim_Token_Run()
{
	switch(pstruEventDetails->nEventType)
	{
	case MAC_OUT_EVENT:
		//do nothing
		break;
	case MAC_IN_EVENT:
		fn_NetSim_Token_MacIn();
		break;
	case PHYSICAL_OUT_EVENT:
		fn_NetSim_Token_PhyOut();
		break;
	case PHYSICAL_IN_EVENT:
		fn_NetSim_Token_PhyIn();
		break;
	case TIMER_EVENT:
		{
			switch(pstruEventDetails->nSubEventType)
			{
			case START_TOKEN_TRANSMISSION:
				fn_NetSim_Token_StartTokenTransmission();
				break;
			default:
				fnNetSimError("Unknown subevent type %d in %s.",pstruEventDetails->nSubEventType,
					__FUNCTION__);
				break;
			}
		}
		break;
	default:
		fnNetSimError("Unknown event type %d in %s.",pstruEventDetails->nEventType,
			__FUNCTION__);
		break;
	}
	return 0;
}

_declspec(dllexport) int fn_NetSim_Token_Finish()
{
	return 0;
}

_declspec (dllexport) char* fn_NetSim_Token_Trace(int nSubEvent)
{
	return GetStringToken_Subevent(nSubEvent);
}
_declspec(dllexport) int fn_NetSim_Token_FreePacket(NetSim_PACKET* pstruPacket)
{
	return 0;
}

/**
	This function is called by NetworkStack.dll, to copy the aloha protocol
	details from source packet to destination.
*/
_declspec(dllexport) int fn_NetSim_Token_CopyPacket(NetSim_PACKET* pstruDestPacket,NetSim_PACKET* pstruSrcPacket)
{
	return 0;
}

/**
This function write the Metrics 	
*/
_declspec(dllexport) int fn_NetSim_Token_Metrics(PMETRICSWRITER metricsWriter)
{
	return 0;
}

/**
This function will return the string to write packet trace heading.
*/
_declspec(dllexport) char* fn_NetSim_Token_ConfigPacketTrace()
{
	return "";
}

/**
 This function will return the string to write packet trace.																									
*/
_declspec(dllexport) char* fn_NetSim_Token_WritePacketTrace(NetSim_PACKET* pstruPacket, char** ppszTrace)
{
	return "";
}
