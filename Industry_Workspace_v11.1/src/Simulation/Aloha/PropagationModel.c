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
#include "Aloha.h"

static PROPAGATION_HANDLE propagationHandle;

void ALOHA_gettxinfo(NETSIM_ID nTxId,
					NETSIM_ID nTxInterface,
					NETSIM_ID nRxId,
					NETSIM_ID nRxInterface,
					PTX_INFO Txinfo)
{
	PALOHA_PHY_VAR txphy = ALOHA_PHY(nTxId, nTxInterface);
	PALOHA_PHY_VAR rxphy = ALOHA_PHY(nRxId, nRxInterface);

	Txinfo->dCentralFrequency = txphy->frequency;
	Txinfo->dRxAntennaHeight = rxphy->dAntennaHeight;
	Txinfo->dRxGain = rxphy->dAntennaGain;
	Txinfo->dTxAntennaHeight = txphy->dAntennaHeight;
	Txinfo->dTxGain = txphy->dAntennaGain;
	Txinfo->dTxPower_mw = (double)txphy->tx_power;
	Txinfo->dTxPower_dbm = MW_TO_DBM(Txinfo->dTxPower_mw);
	Txinfo->dTx_Rx_Distance = DEVICE_DISTANCE(nTxId, nRxId);
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
	PALOHA_PHY_VAR tp = ALOHA_PHY(t, ti);
	PALOHA_PHY_VAR rp = ALOHA_PHY(r, ri);
	return CheckFrequencyInterfrence(tp->frequency, rp->frequency, tp->bandwidth);
}

static void fn_NetSim_Aloha_InitPropagationModel()
{
	propagationHandle = propagation_init(MAC_PROTOCOL_ALOHA,NULL,ALOHA_gettxinfo,check_interference);
}

double get_received_power(NETSIM_ID TX,NETSIM_ID RX, double time)
{
	return propagation_get_received_power_dbm(propagationHandle, TX, 1, RX, 1, time);
}

double get_rx_power_callbackhandler(NETSIM_ID tx, NETSIM_ID txi,
									NETSIM_ID rx, NETSIM_ID rxi,
									double time)
{
	return DBM_TO_MW(propagation_get_received_power_dbm(propagationHandle, tx, txi, rx, rxi, time));
}

static void calculate_received_power(NETSIM_ID TX,NETSIM_ID RX)
{
	//Distance will change between Tx and Rx due to mobility.
	// So update the power from both side
	PTX_INFO info = get_tx_info(propagationHandle, TX, 1, RX, 1);
	info->dTx_Rx_Distance = DEVICE_DISTANCE(TX, RX);
	propagation_calculate_received_power(propagationHandle,
										 TX, 1, RX, 1, pstruEventDetails->dEventTime);
}

int fn_NetSim_Aloha_CalulateReceivedPower()
{
	NETSIM_ID i,j;
	fn_NetSim_Aloha_InitPropagationModel();
	for(i=0;i<NETWORK->nDeviceCount;i++)
	{
		for(j=0;j<NETWORK->nDeviceCount;j++)
			calculate_received_power(i+1,j+1);
	}
	return 0;
}
