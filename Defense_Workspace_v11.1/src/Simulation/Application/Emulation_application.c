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
#include "Application.h"

int fn_NetSim_Emulation_InitApplication(APP_INFO* appInfo);

static UINT update_source_ip(NETSIM_IPAddress* realIP, char* szVal, UINT count)
{
	UINT i = 0;
	char* ip;
	ip = strtok(szVal, ",");
	while (ip)
	{
		if(strcmp(ip, "0.0.0.0"))
			realIP[i] = STR_TO_IP4(ip);
		i++;
		ip = strtok(NULL, ",");
	}

	if (i != count)
	{
		fnNetSimError("Real IP count (%d) and IP configured (%d) mismatched\n", count, i);
	}
	return i;
}

int fn_NetSim_Application_ConfigureEmulationTraffic(APP_INFO* appInfo,void* xmlNetSimNode)
{
	char* szVal;
	APP_EMULATION_INFO* info=(APP_EMULATION_INFO*)calloc(1,sizeof* info);
	void* xmlChild;
	appInfo->appData=info;
	xmlChild=fn_NetSim_xmlGetChildElement(xmlNetSimNode,"EMULATION",0);
	if(xmlChild)
	{
		szVal = fn_NetSim_xmlConfig_GetVal(xmlChild, "DESTINATION_REAL_IP", 1);
		if (szVal && strcmp(szVal, "0.0.0.0"))
			info->realDestIP = STR_TO_IP4(szVal);
		free(szVal);

		if (isMulticastIP(info->realDestIP))
		{
			info->isMulticast = true;
			info->count = appInfo->nSourceCount;

			info->realSourceIP = calloc(info->count, sizeof* info->realSourceIP);
			info->nSourceId = calloc(info->count, sizeof* info->nSourceId);
			info->simSourceIP = calloc(info->count, sizeof* info->simSourceIP);
		}
		else if (isBroadcastIP(info->realDestIP))
		{
			info->isBroadcast = true;
			info->isMulticast = false;
			info->count = appInfo->nSourceCount;
			info->realSourceIP = calloc(info->count, sizeof* info->realSourceIP);
			info->nSourceId = calloc(info->count, sizeof* info->nSourceId);
			info->simSourceIP = calloc(info->count, sizeof* info->simSourceIP);
		}
		else
		{
			info->count = 1;
			info->isMulticast = false;
			info->isBroadcast = false;
			info->realSourceIP = calloc(1, sizeof* info->realSourceIP);
			info->nSourceId = calloc(1, sizeof* info->nSourceId);
			info->simSourceIP = calloc(1, sizeof* info->simSourceIP);
		}

		szVal=fn_NetSim_xmlConfig_GetVal(xmlChild,"SOURCE_REAL_IP",0);
		if(!szVal)
			szVal = fn_NetSim_xmlConfig_GetVal(xmlChild, "DEVICE_REAL_IP", 1);
		info->count = update_source_ip(info->realSourceIP, szVal, info->count);
		free(szVal);
		
		szVal=fn_NetSim_xmlConfig_GetVal(xmlChild,"SOURCE_PORT",0);
		if(szVal)
			info->nSourcePort=atoi(szVal);
		free(szVal);
		szVal=fn_NetSim_xmlConfig_GetVal(xmlChild,"DESTINATION_PORT",0);
		if(szVal)
			info->nDestinationPort=atoi(szVal);
		free(szVal);
	}
	else
		return 0;
	//Assign other value
	if (!(info->isMulticast || info->isBroadcast))
	{
		info->nDestinationId = appInfo->destList[0];
		info->simDestIP = fn_NetSim_Stack_GetIPAddressAsId(info->nDestinationId, 1);
	}
	UINT i;
	for (i = 0; i < info->count; i++)
	{
		info->nSourceId[i] = appInfo->sourceList[i];
		info->simSourceIP[i] = fn_NetSim_Stack_GetIPAddressAsId(info->nSourceId[i], 1);
	}
	fn_NetSim_Emulation_InitApplication(appInfo);
	//Add the emulation environment variable
	_putenv("NETSIM_EMULATOR=1");
	return 0;
}
int fn_NetSim_Emulation_InitApplication(APP_INFO* appInfo)
{
	APP_EMULATION_INFO* info = appInfo->appData;

	fnCreatePort(appInfo);

	//Create the socket buffer
	fnCreateSocketBuffer(appInfo);

	return 0;
}

void fn_NetSim_Emulation_StartApplication(APP_INFO* appInfo)
{
	if (appInfo->nTransmissionType == MULTICAST)
	{
		add_multicast_route(appInfo);
		join_multicast_group(appInfo, appInfo->dStartTime);
	}
}
