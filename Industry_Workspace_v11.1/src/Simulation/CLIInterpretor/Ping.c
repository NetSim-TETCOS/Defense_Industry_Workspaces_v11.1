/************************************************************************************
* Copyright (C) 2018                                                               *
* TETCOS, Bangalore. India                                                         *
*                                                                                  *
* Tetcos owns the intellectual property rights in the Product and its content.     *
* The copying, redistribution, reselling or publication of any or all of the       *
* Product or its content without express prior written consent of Tetcos is        *
* prohibited. Ownership and / or any other right relating to the software and all  *
* intellectual property rights therein shall remain at all times with Tetcos.      *
*                                                                                  *
* Author: Shashi kant suman
*																					*
* ---------------------------------------------------------------------------------*/
#include "main.h"
#include <signal.h>
#include "CLI.h"
#include "../IP/IP.h"

#pragma comment(lib,"libIP.lib")

bool validate_ping_command(ptrCLIENTINFO info, ptrCOMMANDARRAY command, int index)
{
	if (command->length - index < 2)
	{
		send_message(info, "USAGE: ping deviceName or ping IPAddress\n");
		return false;
	}

	if (_stricmp(command->commands[index], "ping"))
		return false;

	if (!isCommandAsDeviceName(command->commands[index + 1]))
	{
		if (!isValidIPAddress(command->commands[index + 1]))
		{
			send_message(info, "%s is not a valid IP address or valid device name.\n",
						 command->commands[index + 1]);
			return false;
		}
	}
	return true;
}

bool resp(ptrCLIENTINFO info,char* msg, bool isMore)
{
	if(info->clientType != CLIENTTYPE_STRING)
		send_message(info, msg);
	if (info->clientType == CLIENTTYPE_SOCKET && !isMore)
	{
		info->CLIENT.sockClient.iswait = false;
		WakeByAddressSingle(&info->CLIENT.sockClient.iswait);
	}
	else if(info->multResp)
	{
		int len = (int)strlen(msg);
		info->multResp(info->multRespArg,
					   _strdup(msg),
					   len,
					   isMore);
	}
	return !isMore;
}

void execute_ping_command(ptrCLIENTINFO info, ptrCOMMANDARRAY command, int index, NETSIM_ID d)
{
	NETSIM_ID dest;
	if (isCommandAsDeviceName(command->commands[index + 1]))
	{
		dest = fn_NetSim_Stack_GetDeviceId_asName(command->commands[index + 1]);
	}
	else
	{
		NETSIM_IPAddress ip = STR_TO_IP(command->commands[index + 1],
										STR_GET_IP_TYPE(command->commands[index + 1]));
		NETSIM_ID in;
		dest = fn_NetSim_Stack_GetDeviceId_asIP(ip, &in);
		if (!dest)
		{
			send_message(info, "%s IP is not associated with any device.\n",
						 command->commands[index + 1]);
			return;
		}
	}
	IP_DEVVAR* ipVar = GET_IP_DEVVAR(d);
	if (!ipVar->isICMP)
	{
		send_message(info, "ICMP is not configured for device %d.\n", d);
		return;
	}

	ipVar = GET_IP_DEVVAR(dest);
	if (!ipVar->isICMP)
	{
		send_message(info, "ICMP is not configured for device %d.\n", dest);
		return;
	}

	void* handle = ICMP_StartPingRequest(d, dest, 4, resp, info);
	if (info->clientType == CLIENTTYPE_SOCKET)
	{
		info->CLIENT.sockClient.iswait = true;
		WaitOnAddress(&info->CLIENT.sockClient.iswait,
					  &info->CLIENT.sockClient.dontUseMe,
					  sizeof info->CLIENT.sockClient.iswait,
					  INFINITE);
	}
	else
	{
		info->isMultResp = true;
	}
}