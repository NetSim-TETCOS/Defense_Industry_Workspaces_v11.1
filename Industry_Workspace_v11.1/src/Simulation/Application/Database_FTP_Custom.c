/************************************************************************************
 * Copyright (C) 2013                                                               *
 * TETCOS, Bangalore. India                                                         *
 *                                                                                  *
 * Tetcos owns the intellectual property rights in the Product and its content.     *
 * The copying, redistribution, reselling or publication of any or all of the       *
 * Product or its content without express prior written consent of Tetcos is        *
 * prohibited. Ownership and / or any other right relating to the software and all *
 * intellectual property rights therein shall remain at all times with Tetcos.      *
 *                                                                                  *
 * Author:    Shashi Kant Suman                                                       *
 *                                                                                  *
 * ---------------------------------------------------------------------------------*/
#include "Application.h"

/**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
This function is used to create create applications

Users can model their own mathematical distributions for packet sizes and itnerarrival times

 For Custom
 2nd parameter: Distribution Type| Distribution input1|Distribution input2|>
 Details
 If user selects any of the distributions listed below, then pass the corresponding number for the distribution type
 Exponential -1
 Uniform -2
 Triangular -3
 Weibull -4
 Constant -5
 Distribution input1-Value of Mean if exponential distribution/Upper bound for others
 Distribution input2-Value of lower bound

 Dont pass any thing if any of the distribution has no distribution input 2.
 3rd parameter:  Distribution Type |Distribution input1|Distribution input2|>
 Details
 If user selects any of the distributions listed below, then pass the corresponding number for the distribution type: Refer field -19
 Exponential -1
 Uniform -2
 Triangular-3
 Weibull-4
 Constant-5

 Distribution input1-Value of Mean if exponential distribution/Upper bound for others
 Distribution input2-Value of lower bound
 Dont pass any thing if any of the distribution has no distribution input 2.

 2)ldArrival-Arrival time of the last frame.
 3)unsigned long uSeed,unsigned long uSeed1-Seed values for packet size
 4)unsigned long uSeed2,unsigned long uSeed3-Seed values for inter arrival time
 Return Value:pointer to an array of long double
 1st element:packet size
 2nd element:arrival time
 3rd,4th element:seed values for packet size
 5th,6th element:seed values for inter arrival time
 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */

_declspec(dllexport) int fn_NetSim_TrafficGenerator_Custom(APP_DATA_INFO* info,
														   double* fSize,
														   double* ldArrival,
														   unsigned long* uSeed,
														   unsigned long* uSeed1,
														   unsigned long* uSeed2,
														   unsigned long* uSeed3)
{
	double time=0.0;
	do
	{
		fnDistribution(info->packetSizeDistribution, fSize, uSeed,uSeed1,&(info->dPacketSize));
	}while(*fSize <= 1.0);
	//Call the fn in Distribution DLL to generate inter arrival time
	do
	{
		fnDistribution(info->IATDistribution,&time,uSeed,uSeed1,&(info->dIAT));
	}while (time <= 0.0);
	*ldArrival = *ldArrival + time;
	return 1;
}

/** This function is used to start the Database, FTP and Custom applications */
int fn_NetSim_Application_StartDataAPP(APP_INFO* appInfo, double time)
{
	APP_DATA_INFO* info = (APP_DATA_INFO*)appInfo->appData;

	if (appInfo->dEndTime <= time)
		return 0;

	NETSIM_ID nSource = appInfo->sourceList[0];

	NETSIM_ID* nDestination = appInfo->destList;
	UINT destCount = appInfo->nDestCount;

	double arrivalTime = 0;
	double packetSize = 0;

	fnCreatePort(appInfo);

	//Create the socket buffer
	fnCreateSocketBuffer(appInfo);

	if (appInfo->nTransmissionType == MULTICAST)
	{
		add_multicast_route(appInfo);
		join_multicast_group(appInfo, time);
	}

	//Generate the app start event
	fn_NetSim_TrafficGenerator_Custom((APP_DATA_INFO*)appInfo->appData, &packetSize, &arrivalTime,
									  &(NETWORK->ppstruDeviceList[nSource - 1]->ulSeed[0]),
									  &(NETWORK->ppstruDeviceList[nSource - 1]->ulSeed[1]),
									  &(NETWORK->ppstruDeviceList[nSource - 1]->ulSeed[0]),
									  &(NETWORK->ppstruDeviceList[nSource - 1]->ulSeed[1]));
	
	if(!appInfo->nPacketId)
		arrivalTime = 0; /* Don't understand why i need to do this. Customer is God.*/

	pstruEventDetails->dEventTime = time + arrivalTime;
	pstruEventDetails->dPacketSize = packetSize;
	pstruEventDetails->nApplicationId = appInfo->id;
	pstruEventDetails->nDeviceId = nSource;
	pstruEventDetails->nDeviceType = DEVICE_TYPE(nSource);
	pstruEventDetails->nEventType = TIMER_EVENT;
	pstruEventDetails->nInterfaceId = 0;
	pstruEventDetails->nProtocolId = PROTOCOL_APPLICATION;
	pstruEventDetails->nSegmentId = 0;
	pstruEventDetails->nSubEventType = event_APP_START;
	pstruEventDetails->pPacket =
		fn_NetSim_Application_GeneratePacket(appInfo,
											 pstruEventDetails->dEventTime,
											 nSource,
											 destCount,
											 nDestination,
											 ++appInfo->nPacketId,
											 appInfo->nAppType,
											 appInfo->qos,
											 appInfo->sourcePort,
											 appInfo->destPort);
	pstruEventDetails->szOtherDetails = appInfo;
	pstruEventDetails->nPacketId = appInfo->nPacketId;
	fnpAddEvent(pstruEventDetails);
	return 0;
}

