/************************************************************************************
* Copyright (C) 2017                                                               *
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
#include "Ethernet.h"

ptrSWITCHTABLE SWITCHTABLE_FIND(ptrETH_LAN lan, PNETSIM_MACADDRESS dest)
{
	ptrSWITCHTABLE table = lan->switchTable;
	while (table)
	{
		if (!MAC_COMPARE(dest, table->mac))
			return table;
		SWITCHTABLE_NEXT(table);
	}
	return NULL;
}

void SWITCHTABLE_NEW(ptrETH_LAN lan, PNETSIM_MACADDRESS dest, NETSIM_ID outport)
{
	ptrSWITCHTABLE table = SWITCHTABLE_FIND(lan, dest);
	if (!table)
	{
		table = SWITCHTABLE_ALLOC();
		table->mac = dest;
		table->outPort = outport;
		SWITCHTABLE_ADD(&SWITCHTABLE_GET_LAN(lan), table);
	}
}

PMETRICSNODE menu = NULL;

static PMETRICSNODE write_header(char* name)
{
	if(!menu)
		menu = init_metrics_node(MetricsNode_Menu, "Switch Mac address table", NULL);
	PMETRICSNODE submenu = init_metrics_node(MetricsNode_Menu, name, NULL);
	add_node_to_menu(menu, submenu);
	return submenu;
}

void switchtable_metrics_print(PMETRICSWRITER metricsWriter)
{
	NETSIM_ID i;
	for (i = 0; i < NETWORK->nDeviceCount; i++)
	{
		PMETRICSNODE submenu = NULL;
		bool isStarted = false;
		ptrETH_VAR eth = GET_ETH_VAR(i + 1);
		if (!eth)
			continue;

		UINT j;
		for (j = 0; j < eth->lanCount; j++)
		{
			ptrETH_LAN lan = eth->lanVar[j];
			if(!lan->switchTable)
				continue;

			if (!isStarted)
			{
				submenu = write_header(DEVICE_NAME(i + 1));
				isStarted = true;
			}

			char heading[BUFSIZ];
			sprintf(heading, "%s_%d", DEVICE_NAME(i + 1), lan->lanId);
			PMETRICSNODE table = init_metrics_node(MetricsNode_Table, heading, NULL);
			add_node_to_menu(submenu, table);

			add_table_heading_special(table, "Mac Address#1,Type#1,OutPort#1,");
			
			ptrSWITCHTABLE s = lan->switchTable;
			while (s)
			{
				add_table_row_formatted(false, table, "%s,%s,%d,",
										s->mac->szmacaddress,
										"Dynamic",
										s->outPort);
				SWITCHTABLE_NEXT(s);
			}
		}
	}
	if(menu)
		write_metrics_node(metricsWriter, WriterPosition_Current, NULL, menu);
}