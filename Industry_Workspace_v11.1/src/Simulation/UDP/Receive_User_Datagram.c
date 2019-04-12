#include "main.h"
#include "UDP.h"

/*---------------------------------------------------------------------------------
* Copyright (C) 2013															  *
*																				  *
* TETCOS, Bangalore. India                                                        *
*
* Tetcos owns the intellectual property rights in the Product and its content.    *
* The copying, redistribution, reselling or publication of any or all of the      *
* Product or its content without express prior written consent of Tetcos is       *
* prohibited. Ownership and / or any other right relating to the software and all *
* intellectual property rights therein shall remain at all times with Tetcos.     *

* Author:   P.Sathishkumar 
* Date  :   05-Mar-2013
* --------------------------------------------------------------------------------*/

/** This function is used to receive the user datagram */

_declspec(dllexport) int fn_NetSim_UDP_Receive_User_Datagram(NetSim_EVENTDETAILS *pstruEventDetails,struct stru_NetSim_Network *NETWORK)
{
	NetSim_PACKET *pstru_User_Datagram;
	UDP_METRICS *pstruUDP_Metrics=NULL;
	NETSIM_ID nSrcID;
	unsigned short usnSourcePort, usnDest_Port;
	NETSIM_IPAddress szSourceIP, szDestIP;

	pstru_User_Datagram=pstruEventDetails->pPacket;
	nSrcID = pstruEventDetails->nDeviceId;
	usnSourcePort = pstru_User_Datagram->pstruTransportData->nDestinationPort; 
	usnDest_Port = pstru_User_Datagram->pstruTransportData->nSourcePort;
	szSourceIP = pstru_User_Datagram->pstruNetworkData->szDestIP;
	szDestIP = pstru_User_Datagram->pstruNetworkData->szSourceIP;

	if(fn_NetSim_UDP_Check_ApplicationMetrics(nSrcID, usnSourcePort, usnDest_Port, szSourceIP, szDestIP, &pstruUDP_Metrics))
		fn_NetSim_UDP_Create_ApplicationMetrics(nSrcID, usnSourcePort, usnDest_Port, szSourceIP, szDestIP, &pstruUDP_Metrics);

	pstru_User_Datagram->pstruTransportData->dOverhead -= TRANSPORT_UDP_OVERHEADS;
	pstru_User_Datagram->pstruTransportData->dPacketSize = pstru_User_Datagram->pstruTransportData->dPayload + pstru_User_Datagram->pstruTransportData->dOverhead;

	//Update the metrics
	pstruUDP_Metrics->nDataGramsReceived++;

	pstruEventDetails->nEventType=APPLICATION_IN_EVENT;
	pstruEventDetails->nPacketId = pstru_User_Datagram->nPacketId;
	pstruEventDetails->nSubEventType=0;
	pstruEventDetails->nProtocolId = 0;
	pstruEventDetails->dPacketSize=pstru_User_Datagram->pstruTransportData->dPacketSize;
	pstruEventDetails->pPacket = pstru_User_Datagram;
	fnpAddEvent(pstruEventDetails);
	return 1;
}
