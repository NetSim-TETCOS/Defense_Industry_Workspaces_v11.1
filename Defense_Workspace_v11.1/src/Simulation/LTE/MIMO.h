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

#ifndef _NETSIM_LTE_MIMO_H_
#define _NETSIM_LTE_MIMO_H_

#ifdef  __cplusplus
extern "C" {
#endif

	enum enumTransmissionMode
	{
		Single_Antenna = 1,
		Transmit_Diversity,
		SingleUser_MIMO_Spatial_Multiplexing,
		Closed_loop_Spatial_Multiplexing,
		MU_MIMO,
	};


	typedef struct stru_MODE_INDEX
	{
		unsigned int TRANSMISSION_MODE;
		unsigned int Tx_Antenna_DL;
		unsigned int Rx_Antenna_DL;
		unsigned int Tx_Antenna_UL;
		unsigned int Rx_Antenna_UL;
	}MODE_INDEX;

	static const MODE_INDEX MODE_INDEX_MAPPING[] = {
		{Single_Antenna, 1, 1, 1, 1},
		{Transmit_Diversity, 2, 2, 1, 1},
		{SingleUser_MIMO_Spatial_Multiplexing, 2, 2, 1, 1},
		{SingleUser_MIMO_Spatial_Multiplexing, 4, 2, 1, 1},
		{MU_MIMO, 0, 2, 1, 1},
	};

#ifdef  __cplusplus
}
#endif
#endif //_NETSIM_LTE_MIMO_H_