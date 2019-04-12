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
#include "IEEE802_11.h"
#include "IEEE802_11_Phy.h"

int fn_NetSim_802_11_InfrastructureBSS_Init(NETSIM_ID nApID,NETSIM_ID nApInterfaceID)
{
	int nConnectedDeviceCount;
	int nLoop;
	NETSIM_ID nDevId, nIntefId, nProtocolId;
	NETSIM_ID nLinkType;
	PIEEE802_11_MAC_VAR pstruApMac; 
	PIEEE802_11_PHY_VAR pstruApPhy;
	PIEEE802_11_MAC_VAR pstruNodeMac; 
	PIEEE802_11_PHY_VAR pstruNodePhy;

	NetSim_LINKS* pstruLink;
	pstruApMac = IEEE802_11_MAC(nApID,nApInterfaceID);
	pstruApPhy = IEEE802_11_PHY(nApID,nApInterfaceID);
	nLinkType = DEVICE_PHYLAYER(nApID,nApInterfaceID)->pstruNetSimLinks->nLinkType;
	pstruLink = DEVICE_PHYLAYER(nApID,nApInterfaceID)->pstruNetSimLinks;

	if(pstruLink->nLinkType != LinkType_P2MP)
	{
		fnNetSimError("AP %d interface %d is not connected with point-to-multipoint link",
			fn_NetSim_Stack_GetConfigIdOfDeviceById(nApID),
			DEVICE_INTERFACE_CONFIGID(nApID,nApInterfaceID));
		return -1;
	}
	nConnectedDeviceCount = pstruLink->puniDevList.pstrup2MP.nConnectedDeviceCount;

	//AP init
	pstruApMac->BSSType=INFRASTRUCTURE;
	pstruApMac->BSSId= DEVICE_HWADDRESS(nApID,nApInterfaceID);
	pstruApMac->nBSSId = nApID;
	pstruApMac->nAPInterfaceId= nApInterfaceID;
	pstruApMac->devCountinBSS = nConnectedDeviceCount+1;
	pstruApMac->devIdsinBSS=(NETSIM_ID*)calloc(nConnectedDeviceCount+1,sizeof* pstruApMac->devIdsinBSS);
	pstruApMac->devIfinBSS=(NETSIM_ID*)calloc(nConnectedDeviceCount+1,sizeof* pstruApMac->devIfinBSS);
	pstruApMac->devIdsinBSS[0]=nApID;
	pstruApMac->devIfinBSS[0]=nApInterfaceID;
	memcpy(pstruApMac->devIdsinBSS+1,pstruLink->puniDevList.pstrup2MP.anDevIds,
		nConnectedDeviceCount*sizeof(NETSIM_ID));
	memcpy(pstruApMac->devIfinBSS+1,pstruLink->puniDevList.pstrup2MP.anDevInterfaceIds,
		nConnectedDeviceCount*sizeof(NETSIM_ID));

	for(nLoop = 0; nLoop < nConnectedDeviceCount-1; nLoop++)
	{
		nDevId = pstruLink->puniDevList.pstrup2MP.anDevIds[nLoop];
		nIntefId = pstruLink->puniDevList.pstrup2MP.anDevInterfaceIds[nLoop];

		pstruNodeMac = IEEE802_11_MAC(nDevId,nIntefId);
		pstruNodePhy = IEEE802_11_PHY(nDevId,nIntefId);
		nProtocolId = DEVICE_MACLAYER(nDevId,nIntefId)->nMacProtocolId;

		pstruNodeMac->devIdsinBSS=(NETSIM_ID*)calloc(nConnectedDeviceCount+1,sizeof* pstruNodeMac->devIdsinBSS);
		pstruNodeMac->devIfinBSS=(NETSIM_ID*)calloc(nConnectedDeviceCount+1,sizeof* pstruNodeMac->devIfinBSS);
		pstruNodeMac->devIdsinBSS[0]=nApID;
		pstruNodeMac->devIfinBSS[0]=nApInterfaceID;
		memcpy(pstruNodeMac->devIdsinBSS+1,pstruLink->puniDevList.pstrup2MP.anDevIds,
			nConnectedDeviceCount*sizeof(NETSIM_ID));
		memcpy(pstruNodeMac->devIfinBSS+1,pstruLink->puniDevList.pstrup2MP.anDevInterfaceIds,
			nConnectedDeviceCount*sizeof(NETSIM_ID));

		// calculate the distance between AP and the node			
		pstruNodeMac->dDistFromAp = fn_NetSim_Utilities_CalculateDistance(DEVICE_POSITION(nDevId),DEVICE_POSITION(nApID));

		// Assign the link details to Phy variables of device
		pstruNodePhy->dFrequency= pstruApPhy->dFrequency;
		
		// Assign the AP details/ center device details to WLAN nodes
		pstruNodeMac->BSSType=INFRASTRUCTURE;
		pstruNodeMac->BSSId = DEVICE_HWADDRESS(nApID,nApInterfaceID);
		pstruNodeMac->nBSSId = nApID;
		pstruNodeMac->nAPInterfaceId= nApInterfaceID;
	}// End of for loop
	return 0;
}

void fn_NetSim_802_11_InfrastructureBSS_UpdateReceiver(NetSim_PACKET* packet)
{
	PIEEE802_11_MAC_VAR mac=IEEE802_11_CURR_MAC;

	if(MAC_COMPARE(DEVICE_HWADDRESS(pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId),mac->BSSId))
	{	
		packet->nReceiverId = mac->nBSSId;
		packet->nTransmitterId = pstruEventDetails->nDeviceId;
	}
	else
	{
		if (isMulticastPacket(packet) || isBroadcastPacket(packet))
			packet->nReceiverId = 0;
		else
			packet->nReceiverId = get_first_dest_from_packet(packet);
		packet->nTransmitterId = pstruEventDetails->nDeviceId;
	}
}

bool isPacketforsameInfrastructureBSS(PIEEE802_11_MAC_VAR mac,NetSim_PACKET* packet)
{
	UINT i;
	if(!MAC_COMPARE(DEVICE_HWADDRESS(pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId),mac->BSSId))
	{
		for (i = 0; i < mac->devCountinBSS; i++)
		{
			if (isDestFoundinPacket(packet, mac->devIdsinBSS[i]))
				return true;
		}
		return false;
	}
	return true;
}
