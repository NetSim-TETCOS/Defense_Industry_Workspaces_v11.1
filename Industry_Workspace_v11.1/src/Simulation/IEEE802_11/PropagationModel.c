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
#include "IEEE802_11_Phy.h"

int fn_NetSim_IEEE802_11_PropagationModelCallback(NETSIM_ID nodeId)
{
	return fn_NetSim_IEEE802_11_PropagationModel(nodeId, pstruEventDetails->dEventTime);
}
/**
This function used to calculate the received from any wireless node other wireless
nodes in the network. It check is there any interference between the adjacent channel.
If any interference then it consider that effect to calculate the received power.
*/
int fn_NetSim_IEEE802_11_PropagationModel(NETSIM_ID nodeId,double time)
{
	NETSIM_ID in, c, ci;

	for (in = 0; in < DEVICE(nodeId)->nNumOfInterface; in++)
	{
		if (!isIEEE802_11_Configure(nodeId, in + 1))
			continue;

		for (c = 0; c < NETWORK->nDeviceCount; c++)
		{
			for (ci = 0; ci < DEVICE(c + 1)->nNumOfInterface; ci++)
			{
				if (!isIEEE802_11_Configure(c + 1, ci + 1))
					continue;

				//Distance will change between Tx and Rx due to mobility.
				// So update the power from both side
				PTX_INFO info = get_tx_info(propagationHandle, nodeId, in + 1, c + 1, ci + 1);
				info->dTx_Rx_Distance = DEVICE_DISTANCE(nodeId, c + 1);
				propagation_calculate_received_power(propagationHandle,
													 nodeId, in + 1, c + 1, ci + 1, time);

				info = get_tx_info(propagationHandle, c + 1, ci + 1, nodeId, in + 1);
				info->dTx_Rx_Distance = DEVICE_DISTANCE(c + 1, nodeId);
				propagation_calculate_received_power(propagationHandle,
													 c + 1, ci + 1, nodeId, in + 1, time);
			}
		}
	}
	return 0;
}
