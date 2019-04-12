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
* Author:    Shashi Kant Suman                                                     *
*                                                                                  *
* ---------------------------------------------------------------------------------*/
#include "main.h"
#include "OpenFlow.h"
#include "CLIInterface.h"

void openFlow_saveHandle(NETSIM_ID d,
						 UINT id,
						 void* handle)
{
	ptrOPENFLOW of = GET_OPENFLOW_VAR(d);
	ptrHANDLEINFO h = calloc(1, sizeof* h);
	h->handle = handle;
	h->id = id;
	if (of->handleInfo)
	{
		ptrHANDLEINFO t = of->handleInfo;
		while (t->next)
			t = t->next;
		t->next = h;
	}
	else
	{
		of->handleInfo = h;
	}
}

ptrHANDLEINFO openFlow_getHandle(NETSIM_ID d,
								 UINT id)
{
	ptrOPENFLOW of = GET_OPENFLOW_VAR(d);
	ptrHANDLEINFO t = of->handleInfo;
	ptrHANDLEINFO p = NULL;
	while (t)
	{
		if (t->id == id)
		{
			if (p)
				p->next = t->next;
			else
				of->handleInfo = t->next;
			void* h = t->handle;
			free(t);
			return h;
		}
		p = t;
		t = t->next;
	}
	return NULL;
}


static bool validate_input(CLIHANDLE handle, ptrCOMMANDARRAY cmd)
{
	if (!isSDNController(pstruEventDetails->nDeviceId))
	{
		CLI_SEND_MESSAGE(handle, "%d (%s) device is not a SDN controller\n",
						 DEVICE_CONFIGID(pstruEventDetails->nDeviceId),
						 DEVICE_NAME(pstruEventDetails->nDeviceId));
		return false;
	}
	NETSIM_ID c = fn_NetSim_Stack_GetDeviceId_asName(cmd->commands[0]);
	if (!c ||
		!isOPENFLOWConfigured(c))
	{
		CLI_SEND_MESSAGE(handle, "%d (%s) device is not an OPEN_FLOW compatible device.\n",
						 c,
						 cmd->commands[0]);
		return false;
	}
	return true;
}

typedef struct stru_multArgs
{
	NETSIM_ID d;
	UINT id;
	void* handle;
}MULTARG, *ptrMULTARG;
bool openflow_multiple_response_handler(ptrMULTARG arg,
										char* msg,
										int len,
										bool isMore)
{
	if (!arg->id)
	{
		CLI_PRINT_MESSAGE(arg->handle, msg, len);
		if (!isMore)
			CLI_STOP_WAITING(arg->handle);
	}
	else
	{
		openFlow_send_response(arg->d,
							   arg->id,
							   msg,
							   len,
							   isMore);
	}
	if (!isMore)
		free(arg);
	return isMore;
}

void handle_cli_input()
{
	CLIHANDLE handle = pstruEventDetails->szOtherDetails;
	ptrCOMMANDARRAY cmd = CLI_GET_CMDARRAY_FROM_HANDLE(handle);

	if (validate_input(handle, cmd))
	{
		CLI_SEND_MESSAGE(handle, "Input is validated\n");
		NETSIM_ID d = fn_NetSim_Stack_GetDeviceId_asName(cmd->commands[0]);
		cmd = remove_first_word_from_commandArray(cmd);
		if (d == pstruEventDetails->nDeviceId)
		{
			CLI_SEND_MESSAGE(handle, "Command is for SDN Controller\n");
			int len = 0;
			ptrMULTARG arg = calloc(1, sizeof* arg);
			arg->d = d;
			arg->handle = handle;
			bool isMore;
			char* ret = CLI_EXECUTE_COMMAND(cmd,
											d,
											&len,
											openflow_multiple_response_handler,
											arg,
											&isMore);
			CLI_PRINT_MESSAGE(handle, ret, len);
			if (!isMore)
			{
				free(arg);
				CLI_STOP_WAITING(handle);
			}
		}
		else
		{
			CLI_SEND_MESSAGE(handle, "Sending command to client device %d\n", d);
			if (!fn_NetSim_OPEN_FLOW_SendCommand(d, handle, cmd, cmd->length))
				CLI_STOP_WAITING(handle);
		}
	}
	else
	{
		CLI_STOP_WAITING(handle);
	}
}

char* openFlow_execute_command(void* handle, NETSIM_ID d, UINT id, int* len, bool* isMore)
{
	ptrCOMMANDARRAY cmd = (ptrCOMMANDARRAY)handle;
	ptrMULTARG arg = calloc(1, sizeof* arg);
	arg->d = d;
	arg->id = id;
	return CLI_EXECUTE_COMMAND(handle, d, len, openflow_multiple_response_handler, arg, isMore);
}

void openFlow_print_response(NETSIM_ID d,
							 UINT id,
							 void* msg,
							 int len,
							 bool isMore)
{
	void* handle = openFlow_getHandle(d, id);
	if (handle)
	{
		CLI_PRINT_MESSAGE(handle, msg, len);
		if (!isMore)
			CLI_STOP_WAITING(handle);
		else
			openFlow_saveHandle(d, id, handle);
	}
}
