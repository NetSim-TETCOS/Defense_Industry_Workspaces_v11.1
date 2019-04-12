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
#include "CSMACD.h"

//Function prototype
int fn_NetSim_CSMACD_Configure_F(void** var);
int fn_NetSim_CSMACD_Init_F(struct stru_NetSim_Network *NETWORK_Formal,
	NetSim_EVENTDETAILS *pstruEventDetails_Formal,
	char *pszAppPath_Formal,
	char *pszWritePath_Formal,
	int nVersion_Type,
	void **fnPointer);

_declspec(dllexport) int fn_NetSim_CSMACD_Configure(void** var)
{
	return fn_NetSim_CSMACD_Configure_F(var);
}

_declspec (dllexport) int fn_NetSim_CSMACD_Init(struct stru_NetSim_Network *NETWORK_Formal,
	NetSim_EVENTDETAILS *pstruEventDetails_Formal,
	char *pszAppPath_Formal,
	char *pszWritePath_Formal,
	int nVersion_Type,
	void **fnPointer)
{
	char *fcollisioncount,*fcontentionwindow;
	fcollisioncount = calloc(1,strlen(pszIOPath)+50);
	fn_NetSim_Utilities_ConcatString(3,fcollisioncount,pszIOPath,"/collisioncount.txt");
	file_collision = fopen(fcollisioncount,"w");

	if(file_collision == NULL)
		fnSystemError(fcollisioncount);
	else
	{
		fprintf(file_collision,"Source_ID\tCollision_Count\tTime (Micro Sec)\n");
	}


	fcontentionwindow = calloc(1,strlen(pszIOPath)+50);
	fn_NetSim_Utilities_ConcatString(3,fcontentionwindow,pszIOPath,"/ContentionWindow.txt");
	file_contention = fopen(fcontentionwindow,"w");

	if(file_contention == NULL)
		fnSystemError(fcontentionwindow);
	else
	{
		fprintf(file_contention,"Source_ID\tContention_Window\tTime (Micro Sec)\n");
	}

	return fn_NetSim_CSMACD_Init_F(NETWORK_Formal,pstruEventDetails_Formal,pszAppPath_Formal,
		pszWritePath_Formal,nVersion_Type,fnPointer);
}

_declspec (dllexport) int fn_NetSim_CSMACD_Run()
{
	switch(pstruEventDetails->nEventType)
	{
	case MAC_OUT_EVENT:
		fn_NetSim_CSMACD_MacOut();
		break;
	case MAC_IN_EVENT:
		fn_NetSim_CSMACD_MacIn();
		break;
	case PHYSICAL_OUT_EVENT:
		fn_NetSim_CSMACD_PhyOut();
		break;
	case PHYSICAL_IN_EVENT:
		fn_NetSim_CSMACD_PhyIn();
		break;
	case TIMER_EVENT:
		{
			switch(pstruEventDetails->nSubEventType)
			{
			case WAIT_FOR_RANDOM_TIME:
			case PERSISTANCE_WAIT:
				fn_NetSim_CSMACS_PersistanceWait();
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

_declspec(dllexport) int fn_NetSim_CSMACD_Finish()
{
	fclose(file_collision);
	fclose(file_contention);
	return 0;
}

_declspec (dllexport) char* fn_NetSim_CSMACD_Trace(int nSubEvent)
{
	return GetStringCSMACD_Subevent(nSubEvent);
}
_declspec(dllexport) int fn_NetSim_CSMACD_FreePacket(NetSim_PACKET* pstruPacket)
{
	return 0;
}

/**
This function is called by NetworkStack.dll, to copy the aloha protocol
details from source packet to destination.
*/
_declspec(dllexport) int fn_NetSim_CSMACD_CopyPacket(NetSim_PACKET* pstruDestPacket,NetSim_PACKET* pstruSrcPacket)
{
	return 0;
}

/**
This function write the Metrics 	
*/
_declspec(dllexport) int fn_NetSim_CSMACD_Metrics(PMETRICSWRITER metricsWriter)
{
	return 0;
}

/**
This function will return the string to write packet trace heading.
*/
_declspec(dllexport) char* fn_NetSim_CSMACD_ConfigPacketTrace()
{
	return "";
}

/**
This function will return the string to write packet trace.																									
*/
_declspec(dllexport) char* fn_NetSim_CSMACD_WritePacketTrace(NetSim_PACKET* pstruPacket, char** ppszTrace)
{
	return "";
}
