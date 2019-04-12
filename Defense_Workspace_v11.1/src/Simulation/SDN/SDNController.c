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

bool isSDNController(NETSIM_ID d)
{
	ptrOPENFLOW of = GET_OPENFLOW_VAR(d);
	if (of)
	{
		return of->isSDNController;
	}
	return false;
}

ptrSDNCLIENTINFO openFlow_find_clientInfo(NETSIM_ID ct, NETSIM_ID ci)
{
	ptrOPENFLOW of = GET_OPENFLOW_VAR(ct);
	ptrSDNCLIENTINFO sd = of->INFO.clientInfo;
	while (sd)
	{
		if (sd->clientId == ci)
			return sd;
		sd = sd->next;
	}
	return NULL;
}

void openFlow_add_new_client(NETSIM_ID ct,
							 NETSIM_ID ci,
							 UINT16 port)
{
	ptrOPENFLOW of = GET_OPENFLOW_VAR(ct);
	ptrSDNCLIENTINFO sd = calloc(1, sizeof* sd);
	sd->clientId = ci;
	sd->clientIP = openFlow_find_client_IP(ci);
	sd->clientPort = port;
	sd->sock = fn_NetSim_Socket_CreateNewSocket(ct,
												APP_PROTOCOL_OPENFLOW,
												TX_PROTOCOL_TCP,
												OFMSG_PORT,
												port);

	if (of->INFO.clientInfo)
	{
		ptrSDNCLIENTINFO t = of->INFO.clientInfo;
		while (t->next)
			t = t->next;
		t->next = sd;
	}
	else
	{
		of->INFO.clientInfo = sd;
	}
}
