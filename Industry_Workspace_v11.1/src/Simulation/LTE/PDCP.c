/************************************************************************************
 * Copyright (C) 2014                                                               *
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
#include "LTE.h"
#include "PDCP.h"
unsigned int nPDCPSN=0;
int fn_NetSim_LTE_PDCP_init(NetSim_PACKET* packet)
{
	fn_NetSim_LTE_PDCP_hdrCompression(packet);
	fn_NetSim_LTE_PDCP_AddSN(packet);
	return 0;
}
int fn_NetSim_LTE_PDCP_AddSN(NetSim_PACKET* packet)
{
	LTE_MAC_PACKET* macPacket;
	if(packet->pstruMacData->Packet_MACProtocol==NULL)
	{
		macPacket=calloc(1,sizeof* macPacket);
		packet->pstruMacData->Packet_MACProtocol=macPacket;
	}
	else
		macPacket=packet->pstruMacData->Packet_MACProtocol;
	if(macPacket->pdcpHdr==NULL)
		macPacket->pdcpHdr=calloc(1,sizeof* macPacket->pdcpHdr);
	macPacket->pdcpHdr->PDCPSN=++nPDCPSN;
	packet->pstruMacData->dOverhead += PDCP_HEADER_SIZE;
	packet->pstruMacData->dPacketSize += PDCP_HEADER_SIZE;
	packet->pstruMacData->nMACProtocol=MAC_PROTOCOL_LTE;
	return 0;
}
int fn_NetSim_LTE_PDCP_hdrCompression(NetSim_PACKET* packet)
{
	double dSize=packet->pstruNetworkData->dPacketSize;
	double dHdrSize=packet->pstruNetworkData->dOverhead;

	//Remove network layer overhead
	dSize-=dHdrSize; //minimum 20 bytes

	packet->pstruMacData->dPayload=dSize;
	packet->pstruMacData->dOverhead = PDCP_TOKEN_SIZE;
	packet->pstruMacData->dPacketSize=dSize+PDCP_TOKEN_SIZE;
	return 0;
}
int fn_NetSim_LTE_PDCP_hdrUncompression(NetSim_PACKET* packet)
{
	double dSize=packet->pstruMacData->dPayload;
	double dHdrSize=packet->pstruNetworkData->dOverhead;

	//Add network layer overhead
	dSize = dSize+dHdrSize;

	packet->pstruMacData->dPayload=dSize;
	packet->pstruMacData->dOverhead -= PDCP_TOKEN_SIZE;
	packet->pstruMacData->dPacketSize = packet->pstruMacData->dPayload+packet->pstruMacData->dOverhead;
	return 0;
}
int fn_NetSim_LTE_PDCP_RemoveHdr(NetSim_PACKET* packet)
{
	fn_NetSim_LTE_FreePacket(packet);
	packet->pstruMacData->Packet_MACProtocol=NULL;
	return 0;
}