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
 * Author:    		   ,~~.															*
			 		  (  6 )-_,	
			 	 (\___ )=='-'
			 	  \ .   ) )
			 	   \ `-' /
			 	~'`~'`~'`~'`~     Shashi kant suman                                                 
 *																					*
 * ---------------------------------------------------------------------------------*/
#ifndef _NETSIM_LTE_CA_H_
#define _NETSIM_LTE_CA_H_

#ifdef  __cplusplus
extern "C" {
#endif

#define LTE_DL_FREQUENCY_MAX_DEFAULT	889	//MHz
#define LTE_DL_FREQUENCY_MIN_DEFAULT	869 //MHz
#define LTE_UL_FREQUENCY_MAX_DEFAULT	844 //MHz
#define LTE_UL_FREQUENCY_MIN_DEFAULT	824 //MHz


#define MAX_CA_COUNT	5
#define MAX_CA_STRING_LEN 100

	typedef struct stru_CA_Phy
	{
		double dULFrequency_min;
		double dULFrequency_max;
		double dDLFrequency_min;
		double dDLFrequency_max;
		double dBaseFrequency;
		double dChannelBandwidth;
		double dSamplingFrequency;
		unsigned int nFFTSize;
		unsigned int nOccupiedSubCarrier;
		unsigned int nGuardSubCarrier;
		unsigned int nResourceBlockCount;
	}CA_PHY,*PCA_PHY;

	typedef struct stru_CA_MAC
	{
		unsigned int nRBCount;
		unsigned int nRBGCount;
		unsigned int nRBCountInGroup;
		unsigned int nAllocatedRBG;
	}CA_MAC,*PCA_MAC;


#ifdef  __cplusplus
}
#endif
#endif /* _NETSIM_LTE_CA_H_ */