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
#ifndef _NETSIM_GSM_H_
#define _NETSIM_GSM_H_

#define GSM_PAYLOAD (57*2)/8
#define GSM_OVERHEAD (3+1+26+1+3+8.25)/8
#define GSM_CHANNEL_LENGTH 577
#define GSM_CARRIER_LENGTH 577*8
typedef struct stru_MSC_GSM MSC_GSM;

/** 
Data structure for mobile switching center for GSM network.
*/
struct stru_MSC_GSM
{
	double dUpLinkBandwidthMax;
	double dUpLinkBandwidthMin;
	double dDownLinkBandwidthMax;
	double dDownLinkBandwidthMin;
	double dChannelBandiwdth;
	unsigned int nSlotCountInEachCarrier;
	unsigned int nChannelCount;
};
double fn_NetSim_GSM_GetPacketStartTime(double dCurrentTime,unsigned int nTimeSlot);
#endif