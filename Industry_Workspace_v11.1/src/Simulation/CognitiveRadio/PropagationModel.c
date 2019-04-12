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

#include "802_22.h"

int fn_NetSim_CalculateReceivedPower(NETSIM_ID TX,NETSIM_ID RX)
{
	//Distance will change between Tx and Rx due to mobility.
	// So update the power from both side
	PTX_INFO info = get_tx_info(propagationHandle, TX, 1, RX, 1);
	info->dTx_Rx_Distance = DEVICE_DISTANCE(TX, RX);
	propagation_calculate_received_power(propagationHandle,
										 TX, 1, RX, 1, pstruEventDetails->dEventTime);
	return 0;
}

int fn_NetSim_CR_CalulateReceivedPower()
{
	NETSIM_ID i,j;
	for(i=0;i<NETWORK->nDeviceCount;i++)
	{
		if(DEVICE_MACLAYER(i+1,1)->nMacProtocolId == MAC_PROTOCOL_IEEE802_22)
		{
			for(j=0;j<NETWORK->nDeviceCount;j++)
			{
				if(DEVICE_MACLAYER(j+1,1)->nMacProtocolId == MAC_PROTOCOL_IEEE802_22)
				{
					fn_NetSim_CalculateReceivedPower(i+1,j+1);
				}
			}
		}
	}
	return 0;
}
