#include "main.h"
#include "802_22.h"
/** This function is used to start the Incumbent operation and is also used to
    generate the time for which incumbent operates. */
int fn_NetSim_CR_IncumbentStart()
{
	NETSIM_ID nDeviceId;
	NETSIM_ID nInterfaceId;
	int nIncumbentId;
	INCUMBENT* pstruIncumbent;
	BS_PHY* pstruBSPHY;
	
	nDeviceId = pstruEventDetails->nDeviceId;
	nInterfaceId = pstruEventDetails->nInterfaceId;
	pstruBSPHY = DEVICE_PHYVAR(nDeviceId,nInterfaceId);
	nIncumbentId = atoi((char*)pstruEventDetails->szOtherDetails);
	fnpFreeMemory((char*)pstruEventDetails->szOtherDetails);
	pstruEventDetails->szOtherDetails = NULL;

	pstruIncumbent = ((BS_MAC*)(NETWORK->ppstruDeviceList[nDeviceId-1]->ppstruInterfaceList[nInterfaceId-1]->pstruMACLayer->MacVar))->pstruIncumbent[nIncumbentId];

	//Generate the time for which incumbent operation will run
	fn_NetSim_Utils_Distribution(pstruIncumbent->nOperationalDistribution,
		&(pstruIncumbent->dPrevOperationalTime),
		 &DEVICE(1)->ulSeed[0],
		&DEVICE(1)->ulSeed[1],
		pstruIncumbent->nOperationalTime,
		0.0);

	//Add Incumbent operation to CR metrics
	if(pstruBSPHY->pstruOpratingChannel && fnIsinRange(pstruIncumbent->nStartFrequeny,pstruIncumbent->nEndFrequency,
		pstruBSPHY->pstruOpratingChannel->dLowerFrequency,pstruBSPHY->pstruOpratingChannel->dUpperFrequency))
	{
		pstruIncumbent->struIncumbentMetrics.dInterferenceStartTime = pstruEventDetails->dEventTime;
	}
	else
		pstruIncumbent->struIncumbentMetrics.dInterferenceStartTime = -1;
	pstruIncumbent->struIncumbentMetrics.nIdleTime += (int)(pstruEventDetails->dEventTime-pstruIncumbent->struIncumbentMetrics.nPrevTime);
	pstruIncumbent->struIncumbentMetrics.nPrevTime = (int)pstruEventDetails->dEventTime;
	// Add an event for incumbent operation end
	pstruEventDetails->dEventTime += pstruIncumbent->dPrevOperationalTime;
	pstruEventDetails->nSubEventType = INCUMBENT_OPERATION_END;
	pstruEventDetails->szOtherDetails = fnpAllocateMemory(10,sizeof(char));
	sprintf(pstruEventDetails->szOtherDetails,"%d",nIncumbentId);
	fnpAddEvent(pstruEventDetails);
	pstruIncumbent->nIncumbentStatus = IncumbentStatus_OPERATIOAL;
	
	return 1;
}
/**	This function is used to end the Incumbent operation and is also used to
    generate the time for which incumbent is idle. 	*/
int fn_NetSim_CR_IncumbentEnd()
{
	NETSIM_ID nDeviceId;
	NETSIM_ID nInterfaceId;
	int nIncumbentId;
	INCUMBENT* pstruIncumbent;
	BS_PHY* pstruBSPHY;
	
	nDeviceId = pstruEventDetails->nDeviceId;
	nInterfaceId = pstruEventDetails->nInterfaceId;
	pstruBSPHY = DEVICE_PHYVAR(nDeviceId,nInterfaceId);
	nIncumbentId = atoi((char*)pstruEventDetails->szOtherDetails);
	fnpFreeMemory((char*)pstruEventDetails->szOtherDetails);
	pstruEventDetails->szOtherDetails = NULL;

	pstruIncumbent = ((BS_MAC*)(NETWORK->ppstruDeviceList[nDeviceId-1]->ppstruInterfaceList[nInterfaceId-1]->pstruMACLayer->MacVar))->pstruIncumbent[nIncumbentId];

	//Generate the time for which incumbent is idle
	fn_NetSim_Utils_Distribution(pstruIncumbent->nOperationalDistribution,
		&(pstruIncumbent->dPrevOperationalInterval),
		 &DEVICE(1)->ulSeed[0],
		&DEVICE(1)->ulSeed[1],
		pstruIncumbent->nOperationalIntervalTime,
		0.0);
	//Add Incumbent operation to CR metrics
	if(pstruBSPHY->pstruOpratingChannel && fnIsinRange(pstruIncumbent->nStartFrequeny,pstruIncumbent->nEndFrequency,
		pstruBSPHY->pstruOpratingChannel->dLowerFrequency,pstruBSPHY->pstruOpratingChannel->dUpperFrequency) &&
		pstruIncumbent->struIncumbentMetrics.dInterferenceStartTime >= 0.0)
	{
		pstruIncumbent->struIncumbentMetrics.nInterferenceTime += (int)(pstruEventDetails->dEventTime-pstruIncumbent->struIncumbentMetrics.dInterferenceStartTime);
		pstruIncumbent->struIncumbentMetrics.dInterferenceStartTime = -1;
	}
	else
		pstruIncumbent->struIncumbentMetrics.dInterferenceStartTime = -1;
	pstruIncumbent->struIncumbentMetrics.nOperationTime += (int)(pstruEventDetails->dEventTime-pstruIncumbent->struIncumbentMetrics.nPrevTime);
	pstruIncumbent->struIncumbentMetrics.nPrevTime = (int)pstruEventDetails->dEventTime;
	// Add an event for incumbent operation start
	pstruEventDetails->dEventTime += pstruIncumbent->dPrevOperationalInterval;
	pstruEventDetails->nSubEventType = INCUMBENT_OPERATION_START;
	pstruEventDetails->szOtherDetails = fnpAllocateMemory(10,sizeof(char));
	sprintf(pstruEventDetails->szOtherDetails,"%d",nIncumbentId);
	fnpAddEvent(pstruEventDetails);
	pstruIncumbent->nIncumbentStatus = IncumbentStatus_NONOPERATIONAL;
	return 1;
}
