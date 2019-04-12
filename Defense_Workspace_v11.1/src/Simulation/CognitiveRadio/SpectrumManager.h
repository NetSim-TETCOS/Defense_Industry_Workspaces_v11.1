/************************************************************************************
 * Copyright (C) 2013                                                               *
 * TETCOS, Bangalore. India                                                         *
 *                                                                                  *
 * Tetcos owns the intellectual property rights in the Product and its content.     *
 * The copying, redistribution, reselling or publication of any or all of the       *
 * Product or its content without express prior written consent of Tetcos is        *
 * prohibited. Ownership and / or any other right relating to the software and all *
 * intellectual property rights therein shall remain at all times with Tetcos.      *
 *                                                                                  *
 * Author:    Shashi Kant Suman                                                       *
 *                                                                                  *
 * ---------------------------------------------------------------------------------*/
struct stru_802_22_SpectrumManager
{
	unsigned int nInitiate_Channel_Move:1;
	unsigned int nSelf_Coexistence_mode:1;
};
/** Table 236 IEEE 802.22-2011 page 407
	Spectrum Sensing Function input signals
*/
struct stru_802_22_SSFInput
{
	struct stru_802_22_Channel* pstruChannelList;
	char szCountryCode[4]; //24 bits + 8 bits for '\0'
	unsigned int nChannelNumber:8;
	unsigned int nChannelBandwidth;
	unsigned int nSignalTypeArray;
	struct stru_SensingWindowSpecificationArray
	{
		unsigned int NumSensingPeriods;
		unsigned int SensingPeriodDuration;
		unsigned int SensingPeriodInterval;
	}SensingWindowSpecificationArray;
	unsigned int nSensingMode;
	unsigned int nMaxProbabilityOfFalseAlram;
	unsigned int nIncumbentCount;
	INCUMBENT** pstruIncumbent;
};
/// Spectrum Sensing Function output signals
struct stru_802_22_SSFOutput
{
	unsigned int nSensingMode;
	unsigned int nSignalTypeVector;
	unsigned int nSignalPresentVector;
	unsigned int nConfidenceVector;
	unsigned int nMeanRSSIMeasurement;
	unsigned int nStdDevRSSIMeasurement;
};
/// Structure for channel switch request, Sent by BS in order to switch the entire cell operation to new channel.
struct stru_802_22_CHS_REQ
{
	unsigned int nMMMType:8;
	unsigned int nTransactionId:16;
	unsigned int nConfirmationFlag:1;
	unsigned int nSwitchMode:1;
	unsigned int nSwitchCount:8;
};
//_declspec(dllexport) struct stru_802_22_SSFOutput* fn_NetSim_CR_CPE_SSF(struct stru_802_22_SSFInput* input,NETSIM_ID nDevId,NETSIM_ID nInterfaceId);
