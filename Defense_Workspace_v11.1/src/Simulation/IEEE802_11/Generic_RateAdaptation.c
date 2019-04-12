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

#define RATE_UP_INDEX 19
#define RATE_DOWN_INDEX 3
typedef struct  
{
	unsigned int maxPhyindex;
	unsigned int minPhyindex;
	unsigned int* currentPhyindex;
	int* currentDroppedCount;
	int* currentReceivedCount;
}GENERIC_RATE_ADAPTATION,*PGENERIC_RATE_ADAPTATION;

static PGENERIC_RATE_ADAPTATION get_rate_adaptation_data(NETSIM_ID devid,NETSIM_ID ifid)
{
	return (PGENERIC_RATE_ADAPTATION)(IEEE802_11_PHY(devid,ifid)->rateAdaptationData);
}

static void set_rate_adaptation_data(NETSIM_ID devid,NETSIM_ID ifid,PGENERIC_RATE_ADAPTATION rate)
{
	IEEE802_11_PHY(devid,ifid)->rateAdaptationData = rate;
}

void free_rate_adaptation_data(PIEEE802_11_PHY_VAR phy)
{
	PGENERIC_RATE_ADAPTATION rate = (PGENERIC_RATE_ADAPTATION)phy->rateAdaptationData;
	if(rate)
	{
		free(rate->currentDroppedCount);
		free(rate->currentPhyindex);
		free(rate->currentReceivedCount);
		free(rate);
		phy->rateAdaptationData=NULL;
	}
}

unsigned int get_max_phy_index(NETSIM_ID devid,NETSIM_ID ifid)
{
	PIEEE802_11_PHY_VAR phy = IEEE802_11_PHY(devid,ifid);
	switch(phy->PhyProtocol)
	{
	case IEEE_802_11a:
	case IEEE_802_11g:
	case IEEE_802_11p:
		return get_ofdm_phy_max_index((int)phy->dChannelBandwidth,phy->PhyProtocol);
	case IEEE_802_11b:
		return get_dsss_phy_max_index((int)phy->dChannelBandwidth,phy->PhyProtocol);
	case IEEE_802_11n:
	case IEEE_802_11ac:
		return get_ht_phy_max_index((int)phy->dChannelBandwidth,phy->PhyProtocol,phy->nGuardInterval);
	default:
		fnNetSimError("Unknown phy protocol %d in %s.\n",phy->PhyProtocol,__FUNCTION__);
		return 0;
	}
}

static unsigned int get_min_phy_index(NETSIM_ID devid,NETSIM_ID ifid)
{
	PIEEE802_11_PHY_VAR phy = IEEE802_11_PHY(devid,ifid);
	switch(phy->PhyProtocol)
	{
	case IEEE_802_11a:
	case IEEE_802_11g:
	case IEEE_802_11p:
		return get_ofdm_phy_min_index((int)phy->dChannelBandwidth,phy->PhyProtocol);
	case IEEE_802_11b:
		return get_dsss_phy_min_index((int)phy->dChannelBandwidth,phy->PhyProtocol);
	case IEEE_802_11n:
	case IEEE_802_11ac:
		return get_ht_phy_min_index((int)phy->dChannelBandwidth,phy->PhyProtocol,phy->nGuardInterval);
	default:
		fnNetSimError("Unknown phy protocol %d in %s.\n",phy->PhyProtocol,__FUNCTION__);
		return 0;
	}
}

void Generic_Rate_adaptation_init(NETSIM_ID nDevId,NETSIM_ID nifid)
{
	NETSIM_ID i;
	PGENERIC_RATE_ADAPTATION rate=(PGENERIC_RATE_ADAPTATION)calloc(1,sizeof* rate);
	set_rate_adaptation_data(nDevId,nifid,rate);

	rate->currentDroppedCount=(int*)calloc(NETWORK->nDeviceCount+1,sizeof* rate->currentDroppedCount);
	rate->currentPhyindex=(unsigned int*)calloc(NETWORK->nDeviceCount+1,sizeof* rate->currentPhyindex);
	rate->currentReceivedCount=(int*)calloc(NETWORK->nDeviceCount+1,sizeof* rate->currentReceivedCount);

	rate->maxPhyindex = get_max_phy_index(nDevId,nifid);
	rate->minPhyindex = get_min_phy_index(nDevId,nifid);
	for(i=0;i<=NETWORK->nDeviceCount;i++)
		rate->currentPhyindex[i]=rate->maxPhyindex;
}

void packet_drop_notify(NETSIM_ID devid,NETSIM_ID ifid,NETSIM_ID rcvid)
{
	PGENERIC_RATE_ADAPTATION rate = get_rate_adaptation_data(devid,ifid);

	rate->currentDroppedCount[rcvid]++;
	//printf("\nDrop count -- %d\n",rate->currentDroppedCount[rcvid]);
	if(rate->currentDroppedCount[rcvid] > RATE_DOWN_INDEX)
	{
		if(rate->currentPhyindex[rcvid] > rate->minPhyindex)
			rate->currentPhyindex[rcvid]--;
		rate->currentDroppedCount[rcvid]=0;
		//printf("\nIndex for %d to %d is %d\n",devid,rcvid,rate->currentPhyindex[rcvid]);
	}
	rate->currentReceivedCount[rcvid] = 0;
}

void packet_recv_notify(NETSIM_ID devid,NETSIM_ID ifid,NETSIM_ID rcvid)
{
	PGENERIC_RATE_ADAPTATION rate = get_rate_adaptation_data(devid,ifid);
	rate->currentReceivedCount[rcvid]++;
	//printf("\nReceive count -- %d\n",rate->currentReceivedCount[rcvid]);
	if(rate->currentReceivedCount[rcvid] > RATE_UP_INDEX)
	{
		if(rate->currentPhyindex[rcvid] < rate->maxPhyindex)
			rate->currentPhyindex[rcvid]++;
		rate->currentReceivedCount[rcvid]=0;
		//printf("\nIndex for %d to %d is %d\n",devid,rcvid,rate->currentPhyindex[rcvid]);
	}
	rate->currentDroppedCount[rcvid] = 0;
}

unsigned int get_rate_index(NETSIM_ID devid,NETSIM_ID ifid,NETSIM_ID rcvid)
{
	return get_rate_adaptation_data(devid,ifid)->currentPhyindex[rcvid];
}