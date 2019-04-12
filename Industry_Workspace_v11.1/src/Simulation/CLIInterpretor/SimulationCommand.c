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

bool isCommandAsDeviceName(char* name)
{
	NETSIM_ID i;
	for (i = 0; i < NETWORK->nDeviceCount; i++)
	{
		if (!_stricmp(name, NETWORK->ppstruDeviceList[i]->szDeviceName))
			return true;
	}
	return false;
}

bool validate_command(ptrCLIENTINFO info, ptrCOMMANDARRAY command)
{
	int index = 0;
	if (!info->promptString)
	{
		if (isCommandAsDeviceName(command->commands[0]))
			index = 1;

		if (!_stricmp(command->commands[index], "route"))
			return validate_route_command(info, command, index);

		if (!_stricmp(command->commands[index], "acl"))
			return validate_acl_command(info, command, index);

		if (!_stricmp(command->commands[index], CMD_ACLCONFIG))
			return true;

		if (!_stricmp(command->commands[index], "ping"))
			return validate_ping_command(info, command, index);
	}
	else
	{
		if (strstr(info->promptString, CMD_ACLCONFIG))
			return validate_aclconfig_command(info, command, index);
	}

	send_message(info, "%s command is not a valid command.\n",
				 command->commands[index]);
	return false;
}

void execute_command(ptrCLIENTINFO info, ptrCOMMANDARRAY command, NETSIM_ID d)
{
	int index = 0;
	if (info->promptString)
	{ 
		if (strstr(info->promptString, CMD_ACLCONFIG))
			execute_prompt_aclconfig_command(info, command, index, d);
	}
	else
	{
		if (isCommandAsDeviceName(command->commands[0]))
		{
			d = fn_NetSim_Stack_GetDeviceId_asName(command->commands[0]);
			index = 1;
		}

		if (!_stricmp(command->commands[index], "route"))
			execute_route_command(info, command, index, d);
		else if (!_stricmp(command->commands[index], "ACL"))
			execute_acl_command(info, command, index, d);
		else if (!_stricmp(command->commands[index], "ACLCONFIG"))
			execute_aclconfig_command(info, command, index, d);
		else if (!_stricmp(command->commands[index], "ping"))
			execute_ping_command(info, command, index, d);
	}

}

void pass_to_SDNModule(ptrCLIENTINFO info, ptrCOMMANDARRAY command)
{
	NetSim_EVENTDETAILS pevent;
	memset(&pevent, 0, sizeof pevent);
	pevent.dEventTime = ldEventTime+MILLISECOND;
	pevent.nDeviceId = info->deviceId;
	pevent.nDeviceType = DEVICE_TYPE(info->deviceId);
	pevent.nEventType = TIMER_EVENT;
	pevent.nProtocolId = APP_PROTOCOL_OPENFLOW;
	pevent.szOtherDetails = FORM_CLI_HANDLE(command, info);
	fnpAddEvent(&pevent);
	if (info->clientType == CLIENTTYPE_SOCKET)
	{
		info->CLIENT.sockClient.iswait = true;
		WaitOnAddress(&info->CLIENT.sockClient.iswait,
					  &info->CLIENT.sockClient.dontUseMe,
					  sizeof info->CLIENT.sockClient.iswait,
					  INFINITE);
	}
}
