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
#include "CDMA.h"
#include "Cellular.h"

/**
	~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	This function is used to form the CDMA channel based on the following
    assumption.
  		1. One call class : All MS requires same target SNR
  		2. Co-located MS: MS are uniformly distributed on area covered by BTS.
  		3. Hard handover
    max number of call handle is given by formula below 
	
	dProcessingGain = (BSMac->CDMAVar.dChipRate)/dCDMA_DATARATE*1.0
	dTau = BSMac->CDMAVar.dChipRate/dProcessingGain*1.0
	dMaxNoOfCallHandled = 1.0/(1+dNeu)
	dMaxNoOfCallHandled *= (1+dTau)/dTau
	dMaxNoOfCallHandled = dMaxNoOfCallHandled/BSMac->CDMAVar.dVoiceActivityFactor

   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
_declspec(dllexport) int fn_NetSim_FormCDMAChannel(NETSIM_ID nBTSId,Cellular_BS_MAC* BSMac,int nCDMA_ETA,int nCDMA_SIGMA,double dCDMA_DATARATE)
{
	static int nChannelId=1;
	Cellular_CHANNEL *tempChannel1,*tempChannel2; //Temp channel used for assignment
	int nLoop1; //Loop counter
	int nMaxNoOfCallHandled; //Max number of call handled by the CDMA system
	double dMaxNoOfCallHandled;
	double dNeu; //Path loss exponent
	double dTau; //desired lower bound of SNR
	double dTemp; // Temp for calculation channel number
	double dPower; // Power used to calculate other cell interference
	double dProcessingGain; //Processing gain of CDMA system
	double dCDMA_TARGETSNR=BSMac->CDMAVar.dTargetSNR;


	if(nCDMA_SIGMA==0)
		nCDMA_SIGMA=12;
	/* Calculate the other cell interface factor based on
	 * the path loss exponent. The value are taken from the
	 * book "CDMA: Principles of spread spectrum" by Andy Viterbe.
	 */
	switch(nCDMA_ETA)
	{
	default:
	case 3:
		dTemp = 0.77;
		break;
	case 4:
		dTemp = 0.44;
		break;
	case 5:
		dTemp = 0.30;
		break;
	}

	dPower = nCDMA_SIGMA* nCDMA_SIGMA/2.0;
	dPower = dPower*log(10)*log(10)/100.0;
	dNeu = pow(2.71828,dPower);
	dNeu *= dTemp;
	dCDMA_TARGETSNR = pow(10,dCDMA_TARGETSNR/10.0);

	//Calculate the processing gain = Chip rate/Data rate
	dProcessingGain = (BSMac->CDMAVar.dChipRate)/dCDMA_DATARATE*1.0;
	dTau = BSMac->CDMAVar.dChipRate/dProcessingGain*1.0;
	dMaxNoOfCallHandled = 1.0/(1+dNeu);
	dMaxNoOfCallHandled *= (1+dTau)/dTau;
	dMaxNoOfCallHandled = dMaxNoOfCallHandled/BSMac->CDMAVar.dVoiceActivityFactor;
	nMaxNoOfCallHandled = (int)ceil(dMaxNoOfCallHandled);
	for(nLoop1=0;nLoop1<nMaxNoOfCallHandled;nLoop1++)
	{
		//Assigning memory for temporary channel
		tempChannel1 = calloc(1,sizeof* tempChannel1);
		tempChannel1->nAllocationFlag=0; // Initially all channel is free
		tempChannel1->nChannelId = nChannelId++;
		tempChannel1->nBTSId=nBTSId;
		if(nLoop1 == 0) // First Channel (Pilot channel)
		{
			// Setting first channel of BTS (Pilot channel) = Temporary channel
			BSMac->pstruChannelList = tempChannel1;
			tempChannel1->nChannelType = ChannelType_Pilot; // First channel is pilot channel
			tempChannel2=tempChannel1;
		}
		else // All channel except first (Traffic channel).
		{
			// Appending temporary channel to BTS channel list for Traffic channel
			tempChannel1 ->nChannelType = ChannelType_TRAFFICCHANNEL;
			tempChannel2->pstru_NextChannel = tempChannel1;
			tempChannel2 = tempChannel1;
		}
	}
	BSMac->nAllocatedChannel = 0; // Initially no channel is busy
	BSMac->nRACHChannel=1;
	BSMac->nFreeChannel=nMaxNoOfCallHandled;	// Initially all channel is free
	BSMac->nTrafficChannel=nMaxNoOfCallHandled-1;
	BSMac->nChannelCount = nMaxNoOfCallHandled; // Total number of channel
	return 1;
}
