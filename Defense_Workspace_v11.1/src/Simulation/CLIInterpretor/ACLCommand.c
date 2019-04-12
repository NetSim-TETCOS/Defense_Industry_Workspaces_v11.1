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
#include "../Firewall/Firewall.h"

static void execute_acl_status_command(ptrCLIENTINFO info,
									   ptrCOMMANDARRAY command,
									   int index,
									   NETSIM_ID d,
									   bool status)
{
	IP_DEVVAR* ip = GET_IP_DEVVAR(d);
	ip->isFirewallConfigured = status;
	send_message(info, "ACL is %s\n",
				 status ? "enable" : "disable");
}

void execute_acl_command(ptrCLIENTINFO info, ptrCOMMANDARRAY command, int index, NETSIM_ID d)
{
	if (!_stricmp(command->commands[index + 1], "enable"))
		execute_acl_status_command(info, command, index + 1, d, true);

	else if (!_stricmp(command->commands[index + 1], "disable"))
		execute_acl_status_command(info, command, index + 1, d, false);

	else
		send_message(info, "%s is not a valid option for ACL command.\n"
					 "It must be either ENABLE or DISABLE.\n",
					 command->commands[index + 1]);
}

bool validate_acl_command(ptrCLIENTINFO info, ptrCOMMANDARRAY command, int index)
{
	if (command->length - index < 2)
	{
		send_message(info, "Too less argument for ACL command\n");
		return false;
	}
	return true;
}

void execute_aclconfig_command(ptrCLIENTINFO info, ptrCOMMANDARRAY command, int index, NETSIM_ID d)
{
	IP_DEVVAR* ip = GET_IP_DEVVAR(d);
	if (!ip->isFirewallConfigured)
	{
		send_message(info, "ACL is not enable.\n");
		return;
	}

	info->promptString = calloc(strlen(DEVICE_NAME(d)) +
								strlen(CMD_ACLCONFIG) + 10, sizeof(char));

	sprintf(info->promptString, "%s/%s",
			DEVICE_NAME(d),
			CMD_ACLCONFIG);

	send_message(info, "%s %s",
				 CMD_CHANGEPROMPT,
				 info->promptString);
}

static bool isProto(char* s)
{
	if (!_stricmp(s, "TCP"))
		return true;
	else if (!_stricmp(s, "UDP"))
		return true;
	else if (!_stricmp(s, "ANY"))
		return true;
	else
		return false;
}

bool validate_aclconfig_command(ptrCLIENTINFO info, ptrCOMMANDARRAY command, int index)
{
	if (!_stricmp(command->commands[index], "print"))
		return true;

	if (command->length - index < 8)
	{
		send_message(info, "Usage: [PERMIT,DENY] [INBOUND,OUTBOUND,BOTH]"
					 " PROTO SRC DEST SPORT DPORT IFID\n");
		return false;
	}

	if (_stricmp(command->commands[index], "PERMIT") &&
		_stricmp(command->commands[index], "DENY"))
	{
		send_message(info, "First command must be either PERMIT or DENY.\n");
		send_message(info, "Usage: [PERMIT,DENY] [INBOUND,OUTBOUND,BOTH]"
					 " PROTO SRC DEST SPORT DPORT IFID\n");
		return false;
	}

	if (_stricmp(command->commands[index + 1], "INBOUND") &&
		_stricmp(command->commands[index + 1], "OUTBOUND") &&
		_stricmp(command->commands[index + 1], "BOTH"))
	{
		send_message(info, "Second command must be INBOUND, OUTBOUND or BOTH.\n");
		send_message(info, "Usage: [PERMIT,DENY] [INBOUND,OUTBOUND,BOTH]"
					 " PROTO SRC DEST SPORT DPORT IFID\n");
		return false;
	}

	if (!isProto(command->commands[index + 2]))
	{
		send_message(info, "Protocol is not valid. Valid protocol is TCP, UDP, or ANY\n");
		send_message(info, "Usage: [PERMIT,DENY] [INBOUND,OUTBOUND,BOTH]"
					 " PROTO SRC DEST SPORT DPORT IFID\n");
		return false;
	}

	return true;
}



void execute_prompt_aclconfig_command(ptrCLIENTINFO info, ptrCOMMANDARRAY command, int index, NETSIM_ID d)
{
	if (!_stricmp(command->commands[index], "print"))
	{
		char* a = acl_print(d);
		if (a)
			send_message(info, a);
		else
			send_message(info, "ACL list is empty.\n");
		return;
	}

	char* action = command->commands[index++];
	char* direction = command->commands[index++];
	char* proto = command->commands[index++];
	char* srcIP = command->commands[index++];
	char* destIP = command->commands[index++];
	char* sport = command->commands[index++];
	char* dport = command->commands[index++];
	char* in = command->commands[index++];

	char s[BUFSIZ];
	sprintf(s, "%s %s %s %s %s %s %s %s",
			action,
			direction,
			proto,
			srcIP,
			destIP,
			sport,
			dport,
			in);
	acl_add_new_line(d, s);
	send_message(info, "OK!");
}
