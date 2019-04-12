/************************************************************************************
* Copyright (C) 2016                                                               *
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
#include "NetSim_utility.h"
#include "DTDMA_enum.h"
#include "DTDMA.h"

ptrDTDMA_SESSION find_dtdma_session(NETSIM_ID d, NETSIM_ID in)
{
	return DTDMA_MAC(d, in)->session;
}

ptrDTDMA_SESSION find_session(NETSIM_ID l)
{
	ptrDTDMA_SESSION s = dtdmaSession;
	while (s)
	{
		if (s->linkId == l)
			return s;
		s = LIST_NEXT(s);
	}
	return NULL;
}

ptrDTDMA_NET_INFO find_net(ptrDTDMA_SESSION session, NETSIM_ID netId)
{
	UINT i;
	for (i = 0; i < session->netCount; i++)
	{
		if (session->netInfo[i]->net_id == netId)
			return session->netInfo[i];
	}
	return NULL;
}

static ptrDTDMA_NET_INFO init_net(ptrDTDMA_SESSION session, NETSIM_ID netId)
{
	ptrDTDMA_NET_INFO net = calloc(1, sizeof* net);
	net->net_id = netId;
	
	if (session->netCount)
		session->netInfo = realloc(session->netInfo, (session->netCount + 1)*(sizeof* session->netInfo));
	else
		session->netInfo = calloc(1, sizeof* session->netInfo);

	session->netInfo[session->netCount] = net;
	session->netCount++;
	return net;
}

static void add_Node_to_net(ptrDTDMA_SESSION session, NETSIM_ID d, NETSIM_ID in)
{
	PDTDMA_NODE_MAC mac = DTDMA_MAC(d, in);
	NETSIM_ID netid = mac->nNetId;
	ptrDTDMA_NET_INFO net = find_net(session, netid);

	if (!net)
	{
		net = init_net(session, netid);
	}

	if (net->nodeInfo)
		net->nodeInfo = realloc(net->nodeInfo, (net->nDeviceCount + 1) * sizeof* net->nodeInfo);
	else
		net->nodeInfo = calloc(1, sizeof* net->nodeInfo);

	ptrDTDMA_NODE_INFO info = calloc(1, sizeof* info);
	net->nodeInfo[net->nDeviceCount] = info;
	net->nDeviceCount++;
	info->interfaceId = in;
	info->nodeId = d;
	info->nodeAction = CONNECTED;
	mac->netInfo = net;
	mac->nodeInfo = info;
}

static void add_node_to_session(NETSIM_ID d, NETSIM_ID in)
{
	NETSIM_ID l = DEVICE_PHYLAYER(d, in)->nLinkId;
	ptrDTDMA_SESSION session = find_session(l);
	if (!session)
	{
		session = DTDMA_SESSION_ALLOC();
		session->linkId = l;
		LIST_ADD_LAST(&dtdmaSession, session);
	}
	PDTDMA_NODE_MAC mac = DTDMA_MAC(d, in);
	mac->session = session;
	add_Node_to_net(session, d, in);
}

void init_dtdma_session()
{
	NETSIM_ID i, j;
	for (i = 0; i < NETWORK->nDeviceCount; i++)
	{
		for (j = 0; j < DEVICE(i + 1)->nNumOfInterface; j++)

		{
			if (!isDTDMAConfigured(i + 1, j + 1))
				continue;
			add_node_to_session(i + 1, j + 1);
		}
	}
}

static void free_dtdma_node_info(ptrDTDMA_NODE_INFO info)
{
	free(info);
}

static void free_dtdma_net(ptrDTDMA_NET_INFO net)
{
	UINT i;
	for (i = 0; i < net->nDeviceCount; i++)
		free_dtdma_node_info(net->nodeInfo[i]);
	free(net->nodeInfo);
	free(net);
}

void free_dtdma_session()
{
	while (dtdmaSession)
	{
		UINT i;
		for(i=0;i<dtdmaSession->netCount;i++)
			free_dtdma_net(dtdmaSession->netInfo[i]);
		free(dtdmaSession->netInfo);
		LIST_FREE(&dtdmaSession, dtdmaSession);
	}
}

ptrDTDMA_ALLOCATION_INFO get_allocation_info(NETSIM_ID d, NETSIM_ID in)
{
	ptrDTDMA_SESSION session = find_dtdma_session(d, in);
	return session ? session->allocation_info : NULL;
}

ptrDTDMA_NET_INFO get_dtdma_net(NETSIM_ID d, NETSIM_ID in)
{
	PDTDMA_NODE_MAC mac = DTDMA_MAC(d, in);
	return mac->netInfo;
}