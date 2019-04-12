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
#include "TDMA_enum.h"
#include "TDMA.h"

int fn_NetSim_TDMA_PhysicalOut()
{
	double dTxTime;
	NetSim_PACKET* packet= pstruEventDetails->pPacket;
	packet->pstruPhyData->dArrivalTime=pstruEventDetails->dEventTime;
	packet->pstruPhyData->dPayload=packet->pstruMacData->dPacketSize;
	packet->pstruPhyData->dOverhead=fn_NetSim_TDMA_GetPhyOverhead(packet);
	packet->pstruPhyData->dPacketSize=packet->pstruPhyData->dOverhead+packet->pstruPhyData->dPayload;
	packet->pstruPhyData->dStartTime=pstruEventDetails->dEventTime;
	packet->pstruPhyData->nPacketErrorFlag = fn_NetSim_TDMA_CalculatePacketError(packet);

	dTxTime=fn_NetSim_TDMA_CalulateTxTime(packet);

	packet->pstruPhyData->dEndTime=packet->pstruPhyData->dStartTime+dTxTime;

	if(packet->nReceiverId)
	{
		fn_NetSim_TDMA_TransmitPacket(packet);
	}
	else
	{
		//Broadcast packet
		NETSIM_ID loop;
		for(loop=0;loop<NETWORK->nDeviceCount;loop++)
		{
			if(loop+1!=packet->nTransmitterId)
			{
				//Add physical in event
				packet->nReceiverId=loop+1;
				fn_NetSim_TDMA_TransmitPacket(fn_NetSim_Packet_CopyPacket(packet));
			}
		}
		fn_NetSim_Packet_FreePacket(packet);
	}
	return 0;

}
int fn_NetSim_TDMA_TransmitPacket(NetSim_PACKET* packet)
{
	TDMA_NODE_PHY* phy = (TDMA_NODE_PHY*)DEVICE_PHYVAR(packet->nReceiverId,1);
	double power= GET_RX_POWER_dbm(packet->nTransmitterId,packet->nReceiverId);
	double fading;

	//Calculate the fading loss
	fading = propagation_calculate_fadingloss(propagationHandle,
											  packet->nTransmitterId, 1,
											  packet->nReceiverId, 1);

	power -= fading;

	if(power >= phy->dReceiverSensitivity)
	{
		//Add physical in event
		pstruEventDetails->dEventTime=packet->pstruPhyData->dEndTime;
		pstruEventDetails->dPacketSize=packet->pstruPhyData->dPacketSize;
		pstruEventDetails->nDeviceId=packet->nReceiverId;
		pstruEventDetails->nDeviceType=DEVICE_TYPE(packet->nReceiverId);
		pstruEventDetails->nEventType=PHYSICAL_IN_EVENT;
		pstruEventDetails->nInterfaceId=1;
		pstruEventDetails->pPacket=packet;
		fnpAddEvent(pstruEventDetails);
	}
	else
	{
		fn_NetSim_Packet_FreePacket(packet);
	}
	return 1;
}
PACKET_STATUS fn_NetSim_TDMA_CalculatePacketError(NetSim_PACKET* packet)
{
	return PacketStatus_NoError;
}
double fn_NetSim_TDMA_GetPhyOverhead(NetSim_PACKET* packet)
{
	return (double)LINK16_PHY_OVERHEAD;
}
double fn_NetSim_TDMA_CalulateTxTime(NetSim_PACKET* packet)
{
	double size=fnGetPacketSize(packet);
	double dDataRate=DATA_RATE;
	double dTxTime= size*8.0/dDataRate;
	return dTxTime;
}
int fn_NetSim_TDMA_CalulateReceivedPower()
{
	NETSIM_ID i,j;
	for(i=0;i<NETWORK->nDeviceCount;i++)
	{
		for(j=0;j<NETWORK->nDeviceCount;j++)
			if(i!=j)
				fn_NetSim_CalculateReceivedPower(i+1,j+1,0);
	}
	return 0;
}

int fn_NetSim_CalculateReceivedPower(NETSIM_ID TX,NETSIM_ID RX,double time)
{
	//Distance will change between Tx and Rx due to mobility.
	// So update the power from both side
	PTX_INFO info = get_tx_info(propagationHandle, TX, 1, RX, 1);
	info->dTx_Rx_Distance = DEVICE_DISTANCE(TX, RX);
	propagation_calculate_received_power(propagationHandle,
										 TX, 1, RX, 1, time);
	return 1;
}