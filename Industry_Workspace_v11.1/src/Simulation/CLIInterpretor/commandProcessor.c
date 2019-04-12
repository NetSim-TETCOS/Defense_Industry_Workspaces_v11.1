#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Ws2tcpip.h>
#include <windows.h>
#include <stdbool.h>
#include "CLI.h"

void send_message(ptrCLIENTINFO info, char* msg, ...)
{
	char sendMSG[2 * BUFSIZ];
	va_list l;
	va_start(l, msg);
	vsprintf(sendMSG, msg, l);
	if (info->clientType == CLIENTTYPE_SOCKET)
		send_to_socket(info, sendMSG, (int)strlen(sendMSG) + 1);
	else if (info->clientType == CLIENTTYPE_FILE)
		write_to_file(info, sendMSG, (int)strlen(sendMSG) + 1);
	else if (info->clientType == CLIENTTYPE_STRING)
		add_to_string(info, sendMSG, (int)strlen(sendMSG) + 1);
}

static void commandProcessor(ptrCLIENTINFO info, char* command)
{
	ptrCOMMANDARRAY ls = get_commandArray(command);
	char* first = ls->commands[0];
	if (!_stricmp(first, CMD_CONTINUE))
		cli_continue_simulation(info);
	else if (!_stricmp(first, CMD_PAUSE))
		cli_pause_simulation(info);
	else if (!_stricmp(first, CMD_PAUSEAT))
		cli_pause_simulation_at(info, atof(ls->commands[1]));
	else if (!_stricmp(first, CMD_STOP))
		cli_stop_simulation(info);
	else if (!_stricmp(first, CMD_EXIT))
		cli_clear_prompt(info);
	else
	{
		if (validate_command(info, ls))
		{
			if (isCommandAsDeviceName(ls->commands[0]))
			{
				pass_to_SDNModule(info, ls);
			}
			else
			{
				execute_command(info, ls, info->deviceId);
			}
		}
	}

	free_commandArray(ls);
}

void process_command(ptrCLIENTINFO clientInfo, char* command, int len)
{
	commandProcessor(clientInfo, command);
}
