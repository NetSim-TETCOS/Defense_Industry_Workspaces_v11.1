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
#include "Application.h"
#include "../IP/IP.h"

void add_multicast_route(APP_INFO* info)
{
	NETSIM_ID i;
	NETSIM_ID dev;
	NETSIM_IPAddress ip = info->multicastDestIP;
	NETSIM_ID interfaceId = 1;

	for (i = 0; i < info->nSourceCount; i++)
	{
		dev = info->sourceList[i];
		if (!isHost(dev))
			fnNetSimError("Dev %d is not host. Configured to run %d application.",
						  dev,
						  info->id);
		
		iptable_add(IP_WRAPPER_GET(dev),
					ip,
					STR_TO_IP4("255.255.255.255"),
					0,
					ip,
					1,
					&DEVICE_NWADDRESS(dev, 1),
					&interfaceId,
					331,
					"Multicast");
	}

	

	for (i = 0; i < info->nDestCount; i++)
	{
		dev = info->destList[i];
		if (!isHost(dev))
			fnNetSimError("Dev %d is not host. Configured to run %d application.",
						  dev,
						  info->id);
		iptable_add(IP_WRAPPER_GET(dev),
					ip,
					STR_TO_IP4("255.255.255.255"),
					0,
					ip,
					1,
					&DEVICE_NWADDRESS(dev, 1),
					&interfaceId,
					331,
					"Multicast");
	}
}

static void call_ip_to_join_group(NETSIM_ID d, NETSIM_IPAddress ip,double time)
{
	NetSim_EVENTDETAILS pevent;
	memset(&pevent, 0, sizeof pevent);
	pevent.dEventTime = time;
	pevent.nDeviceId = d;
	pevent.nDeviceType = DEVICE_TYPE(d);
	pevent.nEventType = TIMER_EVENT;
	pevent.nProtocolId = NW_PROTOCOL_IPV4;
	pevent.nSubEventType = SUBEVENT_JOIN_MULTICAST_GROUP;
	pevent.szOtherDetails = ip;
	fnpAddEvent(&pevent);
}

void join_multicast_group(APP_INFO* info, double time)
{
	NETSIM_IPAddress ip = info->multicastDestIP;
	UINT i;
	for (i = 0; i < info->nSourceCount; i++)
		call_ip_to_join_group(info->sourceList[i], ip, time);

	for (i = 0; i < info->nDestCount; i++)
		call_ip_to_join_group(info->destList[i], ip, time);
}
