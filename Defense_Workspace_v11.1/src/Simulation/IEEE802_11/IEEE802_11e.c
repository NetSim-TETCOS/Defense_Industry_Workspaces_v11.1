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

/**
This fuction is called from the fn_NetSim_IEEE802_11_Init().
Initialize the parameters relevant to QOS list.
*/
int fn_NetSim_IEEE802_11e_Init(NETSIM_ID nDeviceId,NETSIM_ID nInterfaceId)
{
	PIEEE802_11_MAC_VAR pstruMacVar=IEEE802_11_MAC(nDeviceId,nInterfaceId);
	
	if(!pstruMacVar->nNumberOfAggregatedPackets)
		pstruMacVar->nNumberOfAggregatedPackets=1;
	pstruMacVar->qosList.dMaximumSize = DEVICE_ACCESSBUFFER(nDeviceId,nInterfaceId)->dMaxBufferSize * 1024 * 1024;
	return 0;
}

static bool validate_list(NetSim_PACKET* packet)
{
	while (packet)
	{
		NetSim_PACKET* t = packet;
		if (!fnValidatePacket(packet))
			return false;
		packet = packet->pstruNextPacket;
	}
	return true;
}

static void remove_from_list(NetSim_PACKET** list, NetSim_PACKET* curr)
{
	NetSim_PACKET* t = *list;
	NetSim_PACKET* prev = NULL;
	while (t)
	{
		if (t == curr)
		{
			if (prev)
			{
				prev->pstruNextPacket = t->pstruNextPacket;
				curr->pstruNextPacket = NULL;
			}
			else
			{
				*list = t->pstruNextPacket;
				curr->pstruNextPacket = NULL;
			}
			break;
		}
		prev = t;
		t = t->pstruNextPacket;
	}
}

/**
	This function is used to add packets in to list if buffer has space, 
	else free the packet.
	After adding the packet increase the current buffer size.	
*/
void fn_NetSim_WLAN_802_11e_AddPacketToList(NetSim_PACKET** pstruList,NetSim_PACKET* packet, double dMaximumBuffSize,double* dCurrentBuffSize)
{
	double siz=fnGetPacketSize(packet);
	if(dMaximumBuffSize != 0 && *dCurrentBuffSize+siz > dMaximumBuffSize)
	{
		fn_NetSim_Packet_FreePacket(packet);
		packet = NULL;
	}
	else
	{
		if(dMaximumBuffSize)
			*dCurrentBuffSize += siz;

//		assert(validate_list(*pstruList));

		if(!*pstruList)
		{
			*pstruList = packet;
		}
		else
		{
			NetSim_PACKET* temp;
			temp = *pstruList;
			while(temp->pstruNextPacket)
				temp = temp->pstruNextPacket;
			temp->pstruNextPacket = packet;
		}
	}
}
/**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
This fuction is called when MAC_OUT_EVENT triggered with sub event type = 0;
Check the pstruPacket->nQOS, based on that move the packets into different list.
Check IEEE802.11e enabled or not.
if not enabled then check the QOS class and move packet in to different list
	if pstruPacket->nQOS = QOS_ertPS or QOS_rtPS
		Voice packets move to AC_VO
		Video packets move to AC_VI
	if pstruPacket->nQOS = QOS_nrtPS and QOS_BE
		Move packets into AC_BE
	if pstruPacket->nQOS = QOS_UGS
		Move packets into AC_VO
if IEEE802.11e not enabled 
	Move all packets into AC_BE
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/
void fn_NetSim_IEEE802_11e_AddtoQueue(PIEEE802_11_MAC_VAR mac,NetSim_PACKET* pstruPacket)
{
	NetSim_PACKET** list;

	//If 802.11e is enabled, then put the packet in respective list. 		
	if(mac->IEEE80211eEnableFlag)
	{
		switch(pstruPacket->nQOS)
		{
		case QOS_ertPS:					
		case QOS_rtPS:
			if(pstruPacket->nPacketType == PacketType_Voice)
				list=&mac->qosList.AC_VO;
			else
				list=&mac->qosList.AC_VI;
			break;
		case QOS_UGS:
			list=&mac->qosList.AC_VO;
			break;
		case QOS_nrtPS:
		case QOS_BE:
			list=&mac->qosList.AC_BE;
			break;
		default:
			list=&mac->qosList.AC_BE;
			break;
		}
	}
	else // IEEE11eflag DISABLED
		list=&mac->qosList.AC_BE;
	fn_NetSim_WLAN_802_11e_AddPacketToList(list,
		pstruPacket,
		mac->qosList.dMaximumSize,
		&mac->qosList.dCurrentSize);
}

/**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
This function is used to Get the packet from the pstruList.
The pstruList has priority from 0, 1, 2, 3.
Packets in AC_VO has highest priority and AC_BE has least priority.
So, here first we check any packets in the list[0], is so we get the packet from 
this list, else we go for next list and so on.
nFlag = 0 Just access the packet, but not move the packet pointer to next packet.
nFlag = 1 Take the packet and move the packet pointer to next packet.
Decrease the current buffer size.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/
NetSim_PACKET* fn_NetSim_IEEE802_11e_GetPacketFromQueue(PQOS_LIST pstruQosList,int nPacketRequire,int* nPacketCount)
{
	NetSim_PACKET* pstruPacket=NULL;
	NetSim_PACKET* last=NULL;
	NetSim_PACKET** list;
	NetSim_PACKET* t;
	NetSim_PACKET* prev=NULL;
	*nPacketCount=0;

	if(pstruQosList->AC_VO)
		list=&pstruQosList->AC_VO;
	else if(pstruQosList->AC_VI)
		list=&pstruQosList->AC_VI;
	else if(pstruQosList->AC_BK)
		list=&pstruQosList->AC_BK;
	else if(pstruQosList->AC_BE)
		list=&pstruQosList->AC_BE;
	else
		return NULL;

	t=*list;
//	assert(validate_list(*list));
	while(t && nPacketRequire)
	{
		if(pstruPacket)
		{
			if(t->nReceiverId==pstruPacket->nReceiverId)
			{
				remove_from_list(list, t);
				last->pstruNextPacket=t;
				last = t;
				t = *list;

				nPacketRequire--;
				*nPacketCount++;
				pstruQosList->dCurrentSize -= fnGetPacketSize(last);
			}
			else
			{
				t=t->pstruNextPacket;
			}
		}
		else
		{
			pstruPacket=t;
			last=t;
			t=t->pstruNextPacket;
			*list=t;
			pstruPacket->pstruNextPacket=NULL;
			nPacketRequire--;
			*nPacketCount++;
			pstruQosList->dCurrentSize -= fnGetPacketSize(last);
		}
	}
	return pstruPacket;
}

void fn_NetSim_IEEE802_11e_Updatecw(NetSim_PACKET* packet)
{
	PIEEE802_11_MAC_VAR mac=IEEE802_11_CURR_MAC;
	PIEEE802_11_PHY_VAR phy=IEEE802_11_CURR_PHY;

	if(mac->nRetryCount)
		return;
	switch(packet->nQOS)
	{
	case QOS_ertPS:					
	case QOS_rtPS:
		if(packet->nPacketType == PacketType_Voice)
		{
			mac->currCwMax=(phy->plmeCharacteristics.aCWmin+1)/2-1;
			mac->currCwMin=(phy->plmeCharacteristics.aCWmin+1)/4-1;
		}
		else
		{
			mac->currCwMax=phy->plmeCharacteristics.aCWmin;
			mac->currCwMin=(phy->plmeCharacteristics.aCWmin+1)/2-1;
		}
		break;
	case QOS_UGS:
		mac->currCwMax=(phy->plmeCharacteristics.aCWmin+1)/2-1;
		mac->currCwMin=(phy->plmeCharacteristics.aCWmin+1)/4-1;
		break;
	case QOS_nrtPS:
	case QOS_BE:
	default:
		mac->currCwMax=phy->plmeCharacteristics.aCWmax;
		mac->currCwMin=phy->plmeCharacteristics.aCWmin;
		break;
	}
	mac->nCWcurrent=mac->currCwMin;
}
