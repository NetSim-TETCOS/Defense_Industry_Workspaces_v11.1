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
#include "GSM.h"
#include "Cellular.h"
/**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~																								
 This function is called from init block for allocating the channel to each BTS if the			
 user choice is FCA (Fixed Carreir allocation).												
 This function first calculate the bandwidth allocated for the GSM								
	Bandwidth = Maximum frequency - Minimum frequency;											
																								
 After that, calculate the total no of the channel.											
	Total no of channel = Bandwidth * 8 * (1000/200); // KHz to MHz.							
																								
 Calculate no of channel per BTS.																
	no of channel per BTS = (Total no of channel/no of BTS).									
																								
 Initiallize the channel list.																	
 Allocate channel to the BTS.																	
 Set, no of busy channel = 0;																	
 set, no of free channel = No of channel per BTS.												
 set, Total no of channel = No of channel per BTS.												
																								
 NOTE: In netSim we have only two type of channel:												
			1. Random Access Channel (RACH) First channel of each BTS.							
			2. Traffic Channel.																	
																								
------------------------------ Input parameters -----------------------------------------------
																								
 This function takes three parameters as input,												
	1. Minimum value of down link frequency														
	2. Maximum value of up link frequency.														
	3. No of BTS.																				
	All these parameters are user input.														
																								
------------------------------ Output Parameters ----------------------------------------------
																								
 This function return 1 on successfull completion												
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~																								
*/
int fn_NetSim_FormGSMChannel(DEVVAR_MSC* mscVar)
{
	double dBandWidth;
	unsigned int nTotalNoOfChannel, nNoOfChannelPerBTS;
	unsigned int nLoop,nLoop1;							// Loop Counter
	Cellular_CHANNEL *tempChannel1, *tempChannel2=NULL;		// Temporary channel list
	int i=0;									// For keeping track of time slot.
	double dDLFrequencyMin,dDLFrequencyMax,dULFrequencyMin,dULFrequencyMax;

	//Calculating bandwidth = Maximum - Minimum
	dBandWidth = mscVar->gsmVar->dUpLinkBandwidthMax - mscVar->gsmVar->dUpLinkBandwidthMin;

	//Calculating total number of channel
	nTotalNoOfChannel = (int)(dBandWidth/mscVar->gsmVar->dChannelBandiwdth);	// bandwidth for each channel is 200 KHz.
	nTotalNoOfChannel = nTotalNoOfChannel*mscVar->gsmVar->nSlotCountInEachCarrier;			// Each frequency have 8 time slot.

	//Save the total number of channel
	mscVar->gsmVar->nChannelCount = nTotalNoOfChannel;
	//Calculating number of channel per BTS
	nNoOfChannelPerBTS = nTotalNoOfChannel/mscVar->nBTSCount;
	if(nNoOfChannelPerBTS <=1)
	{
		fnNetSimError("Too less channel for BTS. Increase frequency...");
		return 0;
	}
	dDLFrequencyMin=mscVar->gsmVar->dDownLinkBandwidthMin;
	dDLFrequencyMax=dDLFrequencyMin+mscVar->gsmVar->dChannelBandiwdth;
	dULFrequencyMin=mscVar->gsmVar->dUpLinkBandwidthMin;
	dULFrequencyMax=mscVar->gsmVar->dChannelBandiwdth+dULFrequencyMin;
	// Loop through each BTS
	for(nLoop = 0; nLoop < mscVar->nBTSCount; nLoop++,i++)
	{
		Cellular_BS_MAC* BSMAC=((Cellular_BS_MAC*)DEVICE_MACVAR(mscVar->BTSList[nLoop],1));
		//Assign memory for channel
		BSMAC->nChannelCount=nNoOfChannelPerBTS;
		BSMAC->nFreeChannel=nNoOfChannelPerBTS;
		BSMAC->nRACHChannel=1;
		BSMAC->nTrafficChannel=nNoOfChannelPerBTS-1;

		for(nLoop1=0;nLoop1<nNoOfChannelPerBTS;nLoop1++)
		{
			tempChannel1=calloc(1,sizeof* tempChannel1);
			tempChannel1->dDownLinkMaximumFreqency=dDLFrequencyMax;
			tempChannel1->dDownLinkMinimumFrequency=dDLFrequencyMin;
			tempChannel1->dUpLinkMaximumFrequency=dULFrequencyMax;
			tempChannel1->dUpLinkMinimumFrequency=dULFrequencyMin;
			tempChannel1->nChannelId=nLoop1+1;
			if(nLoop1)
				tempChannel1->nChannelType=ChannelType_TRAFFICCHANNEL;
			else
				tempChannel1->nChannelType=ChannelType_RACH;
			tempChannel1->nTimeSlot=i;
			if(BSMAC->pstruChannelList)
			{
				tempChannel2->pstru_NextChannel=tempChannel1;
				tempChannel2=tempChannel1;
			}
			else
			{
				tempChannel2=tempChannel1;
				BSMAC->pstruChannelList=tempChannel1;
			}
			i++;
			if(i==mscVar->gsmVar->nSlotCountInEachCarrier)
			{
				dDLFrequencyMax += mscVar->gsmVar->dChannelBandiwdth;
				dDLFrequencyMin += mscVar->gsmVar->dChannelBandiwdth;
				dULFrequencyMax += mscVar->gsmVar->dChannelBandiwdth;
				dULFrequencyMin += mscVar->gsmVar->dChannelBandiwdth;
				i=0;
			}
		}
	}
	return 1;
}
/** This function is used to check whether cellular channel is allocated or not */
int isCellularChannelAllocated(NETSIM_ID nMSId,NETSIM_ID nInterfaceId,NETSIM_ID nApplicationId)
{
	Cellular_MS_MAC* MSMac=DEVICE_MACVAR(nMSId,nInterfaceId);
	if(MSMac->pstruAllocatedChannel)
	{
		Cellular_CHANNEL* channel=MSMac->pstruAllocatedChannel;
		while(channel)
		{
			if(channel->nMSId == nMSId && channel->nApplicationId==nApplicationId && channel->nAllocationFlag)
				return 1;
			channel=channel->pstru_NextChannel;
		}
	}
	return 0;
}