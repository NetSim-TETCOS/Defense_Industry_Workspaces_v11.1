#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Ws2tcpip.h>
#include <windows.h>
#include <stdbool.h>
#include <signal.h>
#include "main.h"
#include "CLI.h"
#include "CLIInterface.h"
#pragma comment(lib,"ws2_32.lib")

void init_socket();
DWORD WINAPI command_recv_process(LPVOID lpParam);

static ptrCLIENTINFO firstClient = NULL;
static ptrCLIENTINFO lastClient = NULL;
void* add_new_socket_client(SOCKET s, char* name)
{
	ptrCLIENTINFO n = calloc(1, sizeof* n);
	n->clientType = CLIENTTYPE_SOCKET;
	n->CLIENT.sockClient.clientSocket = s;
	n->deviceName = _strdup(name);
	n->deviceId = fn_NetSim_Stack_GetDeviceId_asName(name);
	n->CLIENT.sockClient.dontUseMe = true;
	n->CLIENT.sockClient.iswait = false;
	
	if (firstClient)
	{
		lastClient->next = n;
		lastClient = n;
	}
	else
	{
		firstClient = n;
		lastClient = n;
	}

	n->CLIENT.sockClient.clientRecvHandle = 
		CreateThread(NULL, 1024, command_recv_process, n, false, &n->CLIENT.sockClient.clientRecvId);
	return n;
}

void* add_new_file_client(char* inputFile)
{
	ptrCLIENTINFO n = calloc(1, sizeof* n);
	n->clientType = CLIENTTYPE_FILE;
	n->CLIENT.fileClient.fpInputFile = fopen(inputFile, "r");
	if (!n->CLIENT.fileClient.fpInputFile)
	{
		perror(inputFile);
		free(n);
		return NULL;
	}
	n->CLIENT.fileClient.outputFile = calloc(strlen(pszIOPath) + 50, sizeof(char));
	sprintf(n->CLIENT.fileClient.outputFile,
			"%s/%s",
			pszIOPath,
			"CLIOutput.txt");
	n->CLIENT.fileClient.fpOutputFile = fopen(n->CLIENT.fileClient.outputFile, "w");
	if (!n->CLIENT.fileClient.fpOutputFile)
	{
		perror(n->CLIENT.fileClient.outputFile);
		free(n->CLIENT.fileClient.outputFile);
		free(n);
		return NULL;
	}
	n->CLIENT.fileClient.inputFile = _strdup(inputFile);

	if (firstClient)
	{
		lastClient->next = n;
		lastClient = n;
	}
	else
	{
		firstClient = n;
		lastClient = n;
	}

	set_fileClientInfo(n);
	read_file_and_execute(n->CLIENT.fileClient.fpInputFile);
	return n;
}

static bool iswait = true;
int fnStopConnection(int sig)
{
	if (sig == SIGINT)
	{
		iswait = false;
	}
	return 0;
}

_declspec(dllexport) bool init_cliInterpretor(ptrCLIINFO cliInfo)
{
	cliInfo->fnHandleTimerEvent = fn_NetSim_CLI_HandleTimerEvent;
	init_socket();
	fprintf(stderr, "Waiting for first client to connect. Press ctrl+c to stop connection.\n");
	if (!cliInfo->inputFileName)
	{
		signal(SIGINT, fnStopConnection);
		while (iswait && !firstClient)
			Sleep(1000);
		signal(SIGINT, SIG_DFL);
	}
	else
	{
		add_new_file_client(cliInfo->inputFileName);
	}
	return iswait;
}

#include "CLI.h"
_declspec(dllexport) ptrCOMMANDARRAY CLI_GET_CMDARRAY_FROM_HANDLE(CLIHANDLE handle)
{
	return handle->cmdArray;
}


CLIHANDLE FORM_CLI_HANDLE(ptrCOMMANDARRAY cmd,
						  ptrCLIENTINFO info)
{
	CLIHANDLE c = calloc(1, sizeof* c);
	c->clientInfo = info;
	c->cmdArray = cmd;
	return c;
}

_declspec(dllexport) void CLI_SEND_MESSAGE(CLIHANDLE handle,
										   char* msg,
										   ...)
{
	char sendMSG[2 * BUFSIZ];
	va_list l;
	va_start(l, msg);
	vsprintf(sendMSG, msg, l);
	send_message(handle->clientInfo, sendMSG);
}

_declspec(dllexport) void CLI_STOP_WAITING(CLIHANDLE handle)
{
	ptrCLIENTINFO info = handle->clientInfo;
	if (info->clientType == CLIENTTYPE_SOCKET)
	{
		info->CLIENT.sockClient.iswait = false;
		WakeByAddressSingle(&info->CLIENT.sockClient.iswait);
	}
}

_declspec(dllexport) char* CLI_EXECUTE_COMMAND(ptrCOMMANDARRAY cmd,
											   NETSIM_ID d,
											   int* len,
											   bool(*multResp)(void*,char* msg,int len,bool isMore),
											   void* arg,
											   bool* isMore)
{
	char* ret;
	ptrCLIENTINFO info = calloc(1, sizeof* info);
	info->clientType = CLIENTTYPE_STRING;
	info->deviceId = d;
	info->multResp = multResp;
	info->multRespArg = arg;
	execute_command(info, cmd, d);
	
	*len = info->CLIENT.stringClient.len;
	ret = info->CLIENT.stringClient.buffer;
	if (!info->isMultResp)
	{
		*isMore = false;
		free(info);
	}
	else
	{
		*isMore = true;
	}
	return ret;
}

_declspec(dllexport) void CLI_PRINT_MESSAGE(CLIHANDLE info, char* msg, int len)
{
	int i = 0;
	char* s = msg;
	char* t;
	while (i<len)
	{
		char* q = strstr(s, CMD_CHANGEPROMPT);
		if (q)
		{
			info->clientInfo->promptString = _strdup(q + 1 + strlen(CMD_CHANGEPROMPT));
		}

		send_message(info->clientInfo, s);
		i += (int)strlen(s) + 1;
		t = strchr(s, 0);
		if (t)
			s = t + 1;
	}
}

int fn_NetSim_CLI_HandleTimerEvent()
{
	ptrCLIHANDLE handle = pstruEventDetails->szOtherDetails;
	execute_command(handle->clientInfo, handle->cmdArray, pstruEventDetails->nDeviceId);
	return 0;
}

void add_to_string(ptrCLIENTINFO info, char* sendMsg, int len)
{
	if (info->CLIENT.stringClient.len)
		info->CLIENT.stringClient.buffer = realloc(info->CLIENT.stringClient.buffer,
												   sizeof(char)*
												   (info->CLIENT.stringClient.len + len));
	else
		info->CLIENT.stringClient.buffer = calloc(len, sizeof(char));
	memcpy(info->CLIENT.stringClient.buffer + info->CLIENT.stringClient.len,
		   sendMsg, len);
	info->CLIENT.stringClient.len += len;
}
