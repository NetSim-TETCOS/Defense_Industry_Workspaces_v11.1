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
#include "NetSim_utility.h"
#include <signal.h>
#include "CLI.h"

static ptrCLIENTINFO fileClientInfo = NULL;

void set_fileClientInfo(ptrCLIENTINFO info)
{
	fileClientInfo = info;
}

ptrCLIENTINFO get_fileClientInfo()
{
	return fileClientInfo;
}

void read_file_and_execute(FILE* fp)
{
	double time = 0;
	NETSIM_ID dev = 0;

	char input[BUFSIZ];
	NetSim_EVENTDETAILS pevent;
	memset(&pevent, 0, sizeof pevent);
	pevent.nEventType = TIMER_EVENT;
	pevent.nProtocolId = PROTOCOL_CLI;
	pevent.nSubEventType = SUBEVENT_EXECUTECOMMAND;
	while (fgets(input, BUFSIZ, fp) != NULL)
	{
		char* s = input;
		s = lskip(s);
		if (!*s || *s == '#')
			continue; //Empty or comment line

		_strupr(s);
		
		size_t len = strlen(s);
		if (len > 0 && s[len - 1] == '\n') s[--len] = '\0';

		if (*s == 'T')
		{
			char* f = strtok(s, "=");
			if (!_stricmp(f, "time"))
			{
				char* l = strtok(NULL, "=\n");
				time = atof(l)*SECOND;
			}
			else
			{
				goto CMDINPUT;
			}
		}
		else if (*s == 'D')
		{
			char* f = strtok(s, "=");
			if (!_stricmp(f, "device"))
			{
				char* l = strtok(NULL, "=\n");
				dev = fn_NetSim_Stack_GetDeviceId_asName(l);
			}
			else
			{
				goto CMDINPUT;
			}
		}
		else
		{
		CMDINPUT:
			pevent.dEventTime = time;
			pevent.nDeviceId = dev;
			pevent.nDeviceType = DEVICE_TYPE(dev);
			ptrCOMMANDARRAY cmd = get_commandArray(s);
			pevent.szOtherDetails = FORM_CLI_HANDLE(cmd, get_fileClientInfo());
			fnpAddEvent(&pevent);
		}
	}
}

void write_to_file(ptrCLIENTINFO info, char* msg, int len)
{
	fwrite(msg, sizeof(char), len, info->CLIENT.fileClient.fpOutputFile);
	fflush(info->CLIENT.fileClient.fpOutputFile);
}
