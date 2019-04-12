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
#include "Application.h"

int fn_NetSim_Application_BSM(PAPP_BSM_INFO info,
							  double* fSize,
							  double* ldArrival,
							  unsigned long* uSeed,
							  unsigned long* uSeed1,
							  unsigned long* uSeed2,
							  unsigned long* uSeed3)
{
	double time = 0.0;
	do
	{
		fnDistribution(info->packetSizeDistribution, fSize, uSeed, uSeed1, &(info->dPacketSize));
	} while (*fSize <= 1.0);
	//Call the Distribution DLL to generate inter arrival time
	do
	{
		fnDistribution(info->IATDistribution, &time, uSeed, uSeed1, &(info->dIAT));
	} while (time <= 0.0);
	*ldArrival = *ldArrival + time;

	//generate random time in range of[-5,5]
	time = NETSIM_RAND_RN(info->dRandomWaitTime*-1, info->dRandomWaitTime);
	*ldArrival += time;
	return 1;
}

/** This function is used to start the Database, FTP and Custom applications */
int fn_NetSim_Application_StartBSM(APP_INFO* appInfo, double time)
{
	PAPP_BSM_INFO info = (PAPP_BSM_INFO)appInfo->appData;

	if (appInfo->dEndTime <= time)
		return 0;

	fnCreatePort(appInfo);

	NETSIM_ID nSource = appInfo->sourceList[0];
	NETSIM_ID* nDest = appInfo->destList;
	UINT destCount = appInfo->nDestCount;

	double arrivalTime = 0;
	double packetSize = 0;

	//Create the socket buffer
	fnCreateSocketBuffer(appInfo);

	//Generate the app start event
	fn_NetSim_Application_BSM((PAPP_BSM_INFO)appInfo->appData,
		&packetSize,
		&arrivalTime,
		&(NETWORK->ppstruDeviceList[nSource - 1]->ulSeed[0]),
		&(NETWORK->ppstruDeviceList[nSource - 1]->ulSeed[1]),
		&(NETWORK->ppstruDeviceList[nSource - 1]->ulSeed[0]),
		&(NETWORK->ppstruDeviceList[nSource - 1]->ulSeed[1]));

	pstruEventDetails->dEventTime = time + arrivalTime;
	pstruEventDetails->dPacketSize = packetSize;
	pstruEventDetails->nApplicationId = appInfo->id;
	pstruEventDetails->nDeviceId = nSource;
	pstruEventDetails->nDeviceType = DEVICE_TYPE(nSource);
	pstruEventDetails->nEventType = TIMER_EVENT;
	pstruEventDetails->nInterfaceId = 0;
	pstruEventDetails->nPacketId = 0;
	pstruEventDetails->nProtocolId = PROTOCOL_APPLICATION;
	pstruEventDetails->nSegmentId = 0;
	pstruEventDetails->nSubEventType = event_APP_START;

	pstruEventDetails->pPacket =
		fn_NetSim_Application_GeneratePacket(appInfo,
											 pstruEventDetails->dEventTime,
											 nSource,
											 destCount,
											 nDest,
											 ++appInfo->nPacketId,
											 appInfo->nAppType,
											 appInfo->qos,
											 appInfo->sourcePort,
											 appInfo->destPort);
	pstruEventDetails->szOtherDetails = appInfo;
	fnpAddEvent(pstruEventDetails);

	return 0;
}

/* Below function are out of scope for NetSim.
 * An User can modify these function to implement Vanet packet type.
 */

bool add_sae_j2735_payload(NetSim_PACKET* packet, APP_INFO* info)
{
	// Add the payload based on SAE J2735 or any other standard
	// return true after adding.
	return false;
}

void process_saej2735_packet(NetSim_PACKET* packet)
{
	//Add the code to process the SAE J2735 packet.
	//This function is called in Application_IN event.
}