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
#include "IEEE1609.h"
#include "WSMP.h"
#pragma comment(lib,"ws2_32.lib")

void set_hdr(PWSMP_HDR p,UINT8 len)
{
	p->wsm_version = 0x02;
	p->psid = 0x0a;
	p->ch_no[0] = 0x0F;
	p->ch_no[1] = 0x01;
	p->ch_no[2] = 0xAC;
	p->data_rate[0] = 0x10;
	p->data_rate[1] = 0x01;
	p->data_rate[2] = 0x0C;
	p->tx_power_level[0] = 0x04;
	p->tx_power_level[1] = 0x01;
	p->tx_power_level[2] = 0x0F;
	p->ele_id = htons(0x8000);
	p->wsm_len = len;
}

void fn_NetSim_WSNP_SendWSMPacket()
{
	NetSim_PACKET *wsmPacket;
	NETSIM_ID nAppId;
	ptrSOCKETINTERFACE nSocketId;
	NetSim_BUFFER* buffer;
	PWSMP_HDR hdr;

	nSocketId = pstruEventDetails->szOtherDetails;


	nAppId = pstruEventDetails->nApplicationId;

	wsmPacket = fn_NetSim_Socket_GetPacketFromInterface(nSocketId, 0);
	if (!wsmPacket)
		return;//No packet in buffer

	while (fn_NetSim_Socket_GetBufferStatus(nSocketId))
	{
		wsmPacket = fn_NetSim_Socket_GetPacketFromInterface(nSocketId, 1);

		hdr = (PWSMP_HDR)calloc(1, sizeof *hdr);
		set_hdr(hdr, (UINT8)wsmPacket->pstruTransportData->dPayload);
		SET_WSMP_HDR(wsmPacket, hdr);

		/* Add payload and overheads */
		wsmPacket->pstruTransportData->dPayload = wsmPacket->pstruAppData->dPacketSize;
		wsmPacket->pstruTransportData->dOverhead = WSMP_HDR_LEN;

		/* Assign the packet size */
		wsmPacket->pstruTransportData->dPacketSize = 
			wsmPacket->pstruTransportData->dPayload + wsmPacket->pstruTransportData->dOverhead;

		/* Update TransportLayer time */
		wsmPacket->pstruTransportData->dArrivalTime = pstruEventDetails->dEventTime;
		wsmPacket->pstruTransportData->dStartTime = pstruEventDetails->dEventTime;
		wsmPacket->pstruTransportData->dEndTime = pstruEventDetails->dEventTime;
		wsmPacket->pstruTransportData->nTransportProtocol = TX_PROTOCOL_WSMP;

		//Set network layer timing
		wsmPacket->pstruNetworkData->dArrivalTime = pstruEventDetails->dEventTime;
		wsmPacket->pstruNetworkData->dStartTime = pstruEventDetails->dEventTime;
		wsmPacket->pstruNetworkData->dEndTime = pstruEventDetails->dEventTime;

		wsmPacket->pstruNetworkData->dPacketSize = wsmPacket->pstruTransportData->dPacketSize;
		wsmPacket->pstruNetworkData->dPayload = wsmPacket->pstruTransportData->dPacketSize;
		wsmPacket->pstruNetworkData->dOverhead = 0;

		wsmPacket->pstruNetworkData->szGatewayIP = wsmPacket->pstruNetworkData->szSourceIP;
		wsmPacket->pstruNetworkData->szNextHopIp = wsmPacket->pstruNetworkData->szDestIP;

		wsmPacket->pstruMacData->szSourceMac = (fn_NetSim_Stack_GetMacAddressFromIP(wsmPacket->pstruNetworkData->szGatewayIP));
		wsmPacket->pstruMacData->szDestMac = (fn_NetSim_Stack_GetMacAddressFromIP(wsmPacket->pstruNetworkData->szNextHopIp));

		if (!wsmPacket->pstruMacData->szSourceMac)
			wsmPacket->pstruMacData->szSourceMac = BROADCAST_MAC;
		if (!wsmPacket->pstruMacData->szDestMac)
			wsmPacket->pstruMacData->szDestMac = BROADCAST_MAC;



		NetSim_BUFFER* buf = DEVICE_ACCESSBUFFER(pstruEventDetails->nDeviceId, 1);
		if (!fn_NetSim_GetBufferStatus(buf))
		{
			/* Add the event details for NETWORK_OUT_EVENT*/
			pstruEventDetails->nInterfaceId = 1;
			pstruEventDetails->nApplicationId = wsmPacket->pstruAppData->nApplicationId;
			pstruEventDetails->nProtocolId = fn_NetSim_Stack_GetMacProtocol(pstruEventDetails->nDeviceId, 1);
			pstruEventDetails->nPacketId = wsmPacket->nPacketId;
			pstruEventDetails->nSegmentId = wsmPacket->pstruAppData->nSegmentId;
			pstruEventDetails->dPacketSize = wsmPacket->pstruTransportData->dPacketSize;
			pstruEventDetails->nEventType = MAC_OUT_EVENT;
			pstruEventDetails->nSubEventType = 0;
			pstruEventDetails->pPacket = NULL;
			fnpAddEvent(pstruEventDetails);
		}
		fn_NetSim_Packet_AddPacketToList(buf, wsmPacket,0);
		
	}
}

int fn_NetSim_WSNP_ReceiveWSMPacket()
{
	NetSim_PACKET *wsmPacket;

	wsmPacket = pstruEventDetails->pPacket;

	wsmPacket->pstruTransportData->dOverhead -= WSMP_HDR_LEN;
	wsmPacket->pstruTransportData->dPacketSize = wsmPacket->pstruTransportData->dPayload +
		wsmPacket->pstruTransportData->dOverhead;

	pstruEventDetails->nEventType = APPLICATION_IN_EVENT;
	pstruEventDetails->nPacketId = wsmPacket->nPacketId;
	pstruEventDetails->nSubEventType = 0;
	pstruEventDetails->nProtocolId = 0;
	pstruEventDetails->dPacketSize = wsmPacket->pstruTransportData->dPacketSize;
	pstruEventDetails->pPacket = wsmPacket;
	fnpAddEvent(pstruEventDetails);
	return 1;
}
