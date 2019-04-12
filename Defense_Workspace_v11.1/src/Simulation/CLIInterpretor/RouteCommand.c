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

static bool validate_route_add_command(ptrCLIENTINFO info, ptrCOMMANDARRAY command, int index)
{
	if (command->length - index >= 9)
		return true;
	send_message(info, "Too few argument for route add command\n");
	return false;
}

static bool validate_route_delete_command(ptrCLIENTINFO info, ptrCOMMANDARRAY command, int index)
{
	if (command->length - index >= 1)
		return true;
	send_message(info, "Too few argument for route delete command\n");
	return false;
}

bool validate_route_command(ptrCLIENTINFO info, ptrCOMMANDARRAY command, int index)
{
	if (command->length - index < 2)
	{
		send_message(info, "Too less argument for route command\n");
		return false;
	}

	if (!_stricmp(command->commands[index + 1], "add"))
		return validate_route_add_command(info, command, index + 1);

	else if (!_stricmp(command->commands[index + 1], "print"))
		return true;

	else if (!_stricmp(command->commands[index + 1], "delete"))
		return validate_route_delete_command(info, command, index + 1);

	send_message(info, "%s is not valid argument for route command\n",
				 command->commands[index + 1]);
	return false;
}

static void execute_route_add_command(ptrCLIENTINFO info,
									  ptrCOMMANDARRAY command,
									  int index,
									  NETSIM_ID d)
{
	NETSIM_IPAddress dest = NULL;
	NETSIM_IPAddress mask = NULL;
	NETSIM_IPAddress gateway = NULL;
	UINT prefix = 0;
	NETSIM_ID in = 0;
	UINT metric = 0;
	int ipType = STR_GET_IP_TYPE(command->commands[index + 1]);

	if (!ipType)
		fnNetSimError("%s ip address is not valid.\n", command->commands[index + 1]);

	dest = STR_TO_IP(command->commands[index + 1], ipType);
	
	if (ipType == 4)
		mask = STR_TO_IP4(command->commands[index + 3]);
	else
		prefix = atoi(command->commands[index + 3]);

	gateway = STR_TO_IP(command->commands[index + 4], ipType);

	metric = atoi(command->commands[index + 6]);

	in = atoi(command->commands[index + 8]);

	iptable_add(IP_WRAPPER_GET(d), dest, mask, prefix, gateway, 1, &DEVICE_NWADDRESS(d, in), &in, metric, "STATIC");
	send_message(info, "OK!\n");
}

static void execute_route_print_command(ptrCLIENTINFO info,
										ptrCOMMANDARRAY command,
										int index,
										NETSIM_ID d)
{
	ptrIP_WRAPPER w = IP_WRAPPER_GET(d);
	ptrIP_ROUTINGTABLE table = (ptrIP_ROUTINGTABLE)w->table;
	send_message(info, "===========================================\n\n");
	send_message(info, " IP Route Table\n");
	send_message(info, "===========================================\n\n");

	send_message(info, "%30s\t%15s\t%30s\t%30s\t%10s\t%10s\n",
				 "Network Destination",
				 "Netmask//Prefix",
				 "Gateway",
				 "Interface",
				 "Metric",
				 "Type");

	while (table)
	{
		send_message(info, "%30s\t",
					 table->networkDestination->str_ip);

		if (table->networkDestination->type == 4)
			send_message(info, "%15s\t", table->netMask->str_ip);
		else
			send_message(info, "%15d\t", table->prefix_len);

		if (table->gateway)
			send_message(info, "%30s\t", table->gateway->str_ip);
		else
			send_message(info, "%30s\t", "on-link");

		send_message(info, "%30s\t%10d\t%10s\n",
					 table->Interface[0]->str_ip,
					 table->Metric,
					 table->szType);


		table = LIST_NEXT(table);
	}
	send_message(info, "================================================\n");
	send_message(info, "\n\n");
}

static void execute_route_delete_command(ptrCLIENTINFO info,
										 ptrCOMMANDARRAY command,
										 int index,
										 NETSIM_ID d)
{
	NETSIM_IPAddress dest = STR_TO_IP(command->commands[index + 1],
									  STR_GET_IP_TYPE(command->commands[index + 1]));
	iptable_delete(IP_WRAPPER_GET(d), dest, NULL);
	send_message(info, "OK!\n");
}

void execute_route_command(ptrCLIENTINFO info, ptrCOMMANDARRAY command, int index, NETSIM_ID d)
{
	if (!_stricmp(command->commands[index + 1], "add"))
		execute_route_add_command(info, command, index + 1, d);

	else if (!_stricmp(command->commands[index + 1], "print"))
		execute_route_print_command(info, command, index + 1, d);

	else if (!_stricmp(command->commands[index + 1], "delete"))
		execute_route_delete_command(info, command, index + 1, d);
}
