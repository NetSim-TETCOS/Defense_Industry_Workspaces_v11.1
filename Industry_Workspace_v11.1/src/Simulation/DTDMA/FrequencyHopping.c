/************************************************************************************
 * Copyright (C) 2015                                                               *
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


unsigned long int* hopping_seed1;
unsigned long int* hopping_seed2;

int fn_NetSim_DTDMA_InitFrequencyHopping()
{
	NETSIM_ID i;
	hopping_seed1=(unsigned long int*)calloc(NETWORK->nDeviceCount,sizeof* hopping_seed1);
	hopping_seed2=(unsigned long int*)calloc(NETWORK->nDeviceCount,sizeof* hopping_seed2);
	for(i=0;i<NETWORK->nDeviceCount;i++)
	{
		hopping_seed1[i]=12345678;
		hopping_seed2[i]=23456789;
	}
	return 0;
}
int fn_NetSim_DTDMA_FinishFrequencyHopping()
{
	free(hopping_seed1);
	free(hopping_seed2);
	return 0;
}
int fn_NetSim_DTDMA_FrequencyHopping()
{
	NETSIM_ID i=pstruEventDetails->nDeviceId-1;
	DTDMA_NODE_PHY* phy = DTDMA_PHY(pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId);
	int range = (int)((phy->dUpperFrequency-phy->dLowerFrequency)/phy->dBandwidth);
	int r = (int)((fn_NetSim_Utilities_GenerateRandomNo(&hopping_seed1[i],&hopping_seed2[i])/NETSIM_RAND_MAX)*range);
	phy->dFrequency = phy->dLowerFrequency+r*phy->dBandwidth;

	fprintf(stdout,"Frequency Hopping --- %d\t%d\t%lf\n",i+1,r,phy->dFrequency);
	return 0;
}
