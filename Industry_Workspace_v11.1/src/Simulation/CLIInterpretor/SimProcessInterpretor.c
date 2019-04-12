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

void cli_stop_simulation(ptrCLIENTINFO info)
{
	SIMSTATE sim = netsim_get_simstate();
	if (sim == SIMSTATE_NOTSTARTED)
		send_message(info, "Simulation is not yet started\n");
	else if (sim == SIMSTATE_STOPPED)
		send_message(info, "Simulation is already stopped\n");
	else
		raise(SIGINT);
}

void cli_pause_simulation_at(ptrCLIENTINFO info, double time)
{
	SIMSTATE sim = netsim_get_simstate();
	if (sim == SIMSTATE_STOPPED)
		send_message(info, "Simulation is already stopped\n");
	else
	{
		NetSim_EVENTDETAILS pevent;
		memset(&pevent, 0, sizeof pevent);
		pevent.dEventTime = time*SECOND;
		pevent.nEventType = TIMER_EVENT;
		pevent.nSubEventType = SUBEVENT_PAUSESIMULATION;
		fnpAddEvent(&pevent);
	}
}

void cli_pause_simulation(ptrCLIENTINFO info)
{
	SIMSTATE sim = netsim_get_simstate();
	if (sim == SIMSTATE_NOTSTARTED)
	{
		send_message(info, "Simulation is not yet started. Pausing at 0 sec.\n");
		cli_pause_simulation_at(info, 0);
	}
	else if (sim != SIMSTATE_RUNNING)
		send_message(info, "Simulation is not in running state\n");
	else
		netsim_set_simstate(SIMSTATE_PAUSED);
}

void cli_continue_simulation(ptrCLIENTINFO info)
{
	SIMSTATE sim = netsim_get_simstate();
	if (sim == SIMSTATE_NOTSTARTED)
		send_message(info, "Simulation is not yet started\n");
	else if (sim == SIMSTATE_STOPPED)
		send_message(info, "Simulation is already stopped\n");
	else if (sim == SIMSTATE_RUNNING)
		send_message(info, "Simulation is already running\n");
	else
		netsim_set_simstate(SIMSTATE_RUNNING);
}

void cli_clear_prompt(ptrCLIENTINFO info)
{
	free(info->promptString);
	info->promptString = NULL;
	send_message(info, "%s %s", CMD_CHANGEPROMPT, DEFAULT_PROMPT);
}
