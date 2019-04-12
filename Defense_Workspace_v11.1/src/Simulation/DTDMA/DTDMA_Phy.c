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
#include "DTDMA_enum.h"
#include "DTDMA.h"
#include "ErrorModel.h"

#define LIGHT_SPEED		299792458.0
#define CHANNEL_LOSS	-1.7


double get_propagation_delay(NETSIM_ID i,NETSIM_ID j)
{
	double d = fn_NetSim_Utilities_CalculateDistance(DEVICE_POSITION(i),DEVICE_POSITION(j));
	return (d/LIGHT_SPEED)*SECOND;
}

int fn_NetSim_DTDMA_PhysicalOut()
{
	NETSIM_ID d = pstruEventDetails->nDeviceId;
	NETSIM_ID in = pstruEventDetails->nInterfaceId;
	double dTxTime;
	double* t = &DEVICE_PHYLAYER(pstruEventDetails->nDeviceId, pstruEventDetails->nInterfaceId)->dLastPacketEndTime;
	NetSim_PACKET* packet = pstruEventDetails->pPacket;
	packet->pstruPhyData->dArrivalTime = pstruEventDetails->dEventTime;
	packet->pstruPhyData->dPayload = packet->pstruMacData->dPacketSize;
	packet->pstruPhyData->dOverhead = fn_NetSim_DTDMA_GetPhyOverhead(packet);
	packet->pstruPhyData->dPacketSize = packet->pstruPhyData->dOverhead + packet->pstruPhyData->dPayload;
	packet->pstruPhyData->dStartTime = pstruEventDetails->dEventTime;

	if (*t < packet->pstruPhyData->dStartTime)
		*t = packet->pstruPhyData->dStartTime;
	else
		packet->pstruPhyData->dStartTime = *t;

	dTxTime = fn_NetSim_DTDMA_CalulateTxTime(d, in, packet);


	packet->pstruPhyData->dEndTime = packet->pstruPhyData->dStartTime + dTxTime;
	*t = packet->pstruPhyData->dEndTime;

	if (packet->nReceiverId)
	{
		fn_NetSim_DTDMA_TransmitPacket(d, in, packet);
	}
	else
	{
		//Broadcast packet
		NETSIM_ID loop;
		ptrDTDMA_NET_INFO net = get_dtdma_net(d, in);
		for (loop = 0; loop < net->nDeviceCount; loop++)
		{
			if (net->nodeInfo[loop]->nodeId != packet->nTransmitterId)
			{
				//Add physical in event
				packet->nReceiverId = net->nodeInfo[loop]->nodeId;
				fn_NetSim_DTDMA_TransmitPacket(d, in, fn_NetSim_Packet_CopyPacket(packet));
			}
		}
		fn_NetSim_Packet_FreePacket(packet);
	}
	return 0;
}

int fn_NetSim_DTDMA_TransmitPacket(NETSIM_ID d, NETSIM_ID in, NetSim_PACKET* packet)
{
	NETSIM_ID c = packet->nReceiverId;
	NETSIM_ID ci = fn_NetSim_Stack_GetConnectedInterface(d, in, c);

	DTDMA_NODE_PHY* phy = DTDMA_PHY(c, ci);
	double power = GET_RX_POWER_dbm(d, in, c, ci);

	//Calculate the fading loss
	double fading = propagation_calculate_fadingloss(propagationHandle,
													 d, in, c, ci);
	power -= fading;

	if (power >= phy->dReceiverSensitivity)
	{
		//Add physical in event
		pstruEventDetails->dEventTime = packet->pstruPhyData->dEndTime + get_propagation_delay(packet->nTransmitterId, packet->nReceiverId);
		packet->pstruPhyData->dEndTime = pstruEventDetails->dEventTime;
		pstruEventDetails->dPacketSize = packet->pstruPhyData->dPacketSize;
		pstruEventDetails->nDeviceId = packet->nReceiverId;
		pstruEventDetails->nDeviceType = DEVICE_TYPE(packet->nReceiverId);
		pstruEventDetails->nEventType = PHYSICAL_IN_EVENT;
		pstruEventDetails->nInterfaceId = ci;
		pstruEventDetails->pPacket = packet;
		fnpAddEvent(pstruEventDetails);
	}
	else
	{
		fn_NetSim_Packet_FreePacket(packet);
	}
	return 1;
}

PACKET_STATUS fn_NetSim_DTDMA_CalculatePacketError(NETSIM_ID d, NETSIM_ID in, NetSim_PACKET* packet)
{
	NETSIM_ID c = packet->nTransmitterId;
	NETSIM_ID ci = fn_NetSim_Stack_GetConnectedInterface(d, in, c);
	DTDMA_NODE_PHY* phy = DTDMA_PHY(c, ci);
	double pb = calculate_BER(phy->modulation,
							  GET_RX_POWER_dbm(c, ci, d, in),
							  phy->dBandwidth);

	return fn_NetSim_Packet_DecideError(pb, packet->pstruPhyData->dPacketSize);
}

double fn_NetSim_DTDMA_GetPhyOverhead(NetSim_PACKET* packet)
{
	return (double)0;
}

double fn_NetSim_DTDMA_CalulateTxTime(NETSIM_ID d, NETSIM_ID in, NetSim_PACKET* packet)
{
	double size=fnGetPacketSize(packet);
	DTDMA_NODE_PHY* phy = DTDMA_PHY(d, in);
	double dDataRate=phy->dDataRate;
	double dTxTime= size*8.0/dDataRate;
	return dTxTime;
}

void DTDMA_gettxinfo(NETSIM_ID nTxId,
					NETSIM_ID nTxInterface,
					NETSIM_ID nRxId,
					NETSIM_ID nRxInterface,
					PTX_INFO Txinfo)
{
	DTDMA_NODE_PHY* txphy = (DTDMA_NODE_PHY*)DEVICE_PHYVAR(nTxId, nTxInterface);
	DTDMA_NODE_PHY* rxphy = (DTDMA_NODE_PHY*)DEVICE_PHYVAR(nRxId, nRxInterface);

	Txinfo->dCentralFrequency = txphy->dFrequency?txphy->dFrequency:txphy->dLowerFrequency;
	Txinfo->dRxAntennaHeight = rxphy->dAntennaHeight;
	Txinfo->dRxGain = rxphy->dAntennaGain;
	Txinfo->dTxAntennaHeight = txphy->dAntennaHeight;
	Txinfo->dTxGain = txphy->dAntennaGain;
	Txinfo->dTxPower_mw = (double)txphy->dTXPower;
	Txinfo->dTxPower_dbm = MW_TO_DBM(Txinfo->dTxPower_mw);
	Txinfo->dTx_Rx_Distance = DEVICE_DISTANCE(nTxId, nRxId);
	Txinfo->d0 = txphy->d0;
}

bool CheckFrequencyInterfrence(double dFrequency1, double dFrequency2, double bandwidth)
{
	if (dFrequency1 > dFrequency2)
	{
		if ((dFrequency1 - dFrequency2) >= bandwidth)
			return false; // no interference
		else
			return true; // interference
	}
	else
	{
		if ((dFrequency2 - dFrequency1) >= bandwidth)
			return false; // no interference
		else
			return true; // interference
	}
}

bool check_interference(NETSIM_ID t, NETSIM_ID ti, NETSIM_ID r, NETSIM_ID ri)
{
	DTDMA_NODE_PHY* tp = (DTDMA_NODE_PHY*)DEVICE_PHYVAR(t, ti);
	DTDMA_NODE_PHY* rp = (DTDMA_NODE_PHY*)DEVICE_PHYVAR(r, ri);
	return CheckFrequencyInterfrence(tp->dFrequency, rp->dFrequency, tp->dBandwidth);
}

int dtdma_CalculateReceivedPower(NETSIM_ID tx, NETSIM_ID txi, NETSIM_ID rx, NETSIM_ID rxi)
{
	//Distance will change between Tx and Rx due to mobility.
	// So update the power from both side
	PTX_INFO info = get_tx_info(propagationHandle, tx, txi, rx, rxi);
	info->dTx_Rx_Distance = DEVICE_DISTANCE(tx, rx);
	propagation_calculate_received_power(propagationHandle,
										 tx, txi, rx, rxi, pstruEventDetails->dEventTime);
	return 1;
}

int fn_NetSim_DTDMA_CalulateReceivedPower()
{
	NETSIM_ID t, ti, r, ri;

	propagationHandle = propagation_init(MAC_PROTOCOL_DTDMA, NULL,
										 DTDMA_gettxinfo, check_interference);

	for (t = 0; t < NETWORK->nDeviceCount; t++)
	{
		for (ti = 0; ti < DEVICE(t + 1)->nNumOfInterface; ti++)
		{
			if (!isDTDMAConfigured(t + 1, ti + 1))
				continue;
			for (r = 0; r < NETWORK->nDeviceCount; r++)
			{
				for (ri = 0; ri < DEVICE(r + 1)->nNumOfInterface; ri++)
				{
					if (!isDTDMAConfigured(r + 1, ri + 1))
						continue;
					dtdma_CalculateReceivedPower(t + 1, ti + 1, r + 1, ri + 1);
				}
			}
		}
	}
	return 0;
}
