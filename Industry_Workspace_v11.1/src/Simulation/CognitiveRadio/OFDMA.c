#include "main.h"
#include "802_22.h"

/* Constant Value */

/* Phy Mode- corresponding modulation, coding rate, data rate and spectral effieciency
 * Source - IEEE802.22-2011, table 202, page 310
 */
struct stru_802_22_Phy_Mode struPhyMode[MAX_PHY_MODE] = /* 16 + 1(for 0 mode) */
{
	{0,0,0,0,0},
	{1,Modulation_BPSK,Coding_UNCODED,6,6},		//Used only for CDMA burst
	{2,Modulation_QPSK,Coding_1_2_REP4,6,6},	//Used only for SCH transmission
	{3,Modulation_QPSK,Coding_1_2_REP3,6,6},	//Used only for CBP transmission
	{4,Modulation_QPSK,Coding_1_2_REP2,6,6},	//Used only for FCH transmission
	{5,Modulation_QPSK,Coding_1_2,4.54,0.76},
	{6,Modulation_QPSK,Coding_2_3,6.05,1.01},
	{7,Modulation_QPSK,Coding_3_4,6.81,1.13},
	{8,Modulation_QPSK,Coding_5_6,7.56,1.26},
	{9,Modulation_16_QAM,Coding_1_2,9.08,1.51},
	{10,Modulation_16_QAM,Coding_2_3,12.10,2.02},
	{11,Modulation_16_QAM,Coding_3_4,13.61,2.27},
	{12,Modulation_16_QAM,Coding_5_6,15.13,2.52},
	{13,Modulation_64_QAM,Coding_1_2,13.61,2.27},
	{14,Modulation_64_QAM,Coding_2_3,18.15,3.03},
	{15,Modulation_64_QAM,Coding_3_4,20.42,3.40},
	{16,Modulation_64_QAM,Coding_5_6,22.69,3.78}
};
/** The PHY specification is based on an orthogonal frequency division multiple access (OFDMA) 
scheme where information to (downstream) or from (upstream) multiple CPEs are modulated 
on orthogonal subcarriers using Inverse Fourier Transforms.  */
int fn_NetSim_Init_OFDMA(BS_PHY* pstruBSPhy)
{
	double dCoding;
	int nBits;
	SYMBOL_PARAMETER* pstruSymbol;
	pstruBSPhy->pstruSymbolParameter = fnpAllocateMemory(1,sizeof *pstruBSPhy->pstruSymbolParameter);
	pstruSymbol = pstruBSPhy->pstruSymbolParameter;
	pstruSymbol->nChannelBandwidth = pstruBSPhy->nChannelBandwidth;
	switch(pstruBSPhy->nChannelBandwidth)
	{
	case 6:
		pstruSymbol->dBasicSamplingFrequency = 6.856;
		pstruBSPhy->dRTG=561;
		pstruBSPhy->dTTG=1439;
		break;
	case 7:
		pstruSymbol->dBasicSamplingFrequency = 8;
		pstruBSPhy->dRTG=1520;
		pstruBSPhy->dTTG=1680;
		break;
	case 8:
		pstruSymbol->dBasicSamplingFrequency = 9.136;
		pstruBSPhy->dRTG=2402;
		pstruBSPhy->dTTG=1918;
		break;
	default:
		pstruSymbol->dBasicSamplingFrequency = 6.856;
		pstruBSPhy->dRTG=561;
		pstruBSPhy->dTTG=1439;
		break;
	}
	/* Ref Table 201 page 310 IEEE802.22-2011 */
	pstruSymbol->nSubcarrierCount = 2048;
	pstruSymbol->nDataSubCarrierCount = 1440;
	pstruSymbol->nGuardSubcarrierCount = 368;
	pstruSymbol->nPilotSubCarrierCount = 240;
	pstruSymbol->nUsedSubCarrierCount = 1680;
	pstruSymbol->nSubChannelCount = 60; //Consist of 28 subcarrier ( 24 Data + 4 pilot)
	pstruSymbol->nOFDMSlots = 60; //1 symbol * 1 subchannel
	
	pstruSymbol->dIntercarrierSpacing = (pstruSymbol->dBasicSamplingFrequency/pstruSymbol->nSubcarrierCount)*1000000;
	pstruSymbol->dSubCarrierSpacing = 1000000/pstruSymbol->dIntercarrierSpacing;
	pstruSymbol->dTimeUnit = (pstruSymbol->dSubCarrierSpacing /pstruSymbol->nSubcarrierCount)*1000;
	
	pstruBSPhy->dRTG=pstruBSPhy->dRTG*pstruSymbol->dTimeUnit/1000;
	pstruBSPhy->dTTG=pstruBSPhy->dTTG*pstruSymbol->dTimeUnit/1000;

	switch(pstruBSPhy->nCP)
	{
	case B2_00:
		pstruSymbol->nCP = 4;
		break;
	case B2_01:
		pstruSymbol->nCP = 8;
		break;
	case B2_10:
		pstruSymbol->nCP = 16;
		break;
	case B2_11:
		pstruSymbol->nCP = 32;
		break;
	}
	pstruSymbol->dCPDuration = pstruSymbol->dSubCarrierSpacing/pstruSymbol->nCP;
	pstruSymbol->dSymbolDuration = pstruSymbol->dSubCarrierSpacing + pstruSymbol->dCPDuration;

	//Calculate number of bits per symbol
	switch(pstruBSPhy->nCodingRate)
	{
	case Coding_1_2:
	case Coding_1_2_REP2:
	case Coding_1_2_REP3:
	case Coding_1_2_REP4:
		dCoding = 1/2.0;
		break;
	case Coding_2_3:
		dCoding = 2/3.0;
		break;
	case Coding_3_4:
		dCoding = 3/4.0;
		break;
	case Coding_5_6:
		dCoding = 5/6.0;
		break;
	default:
		dCoding = 1/2.0;
		break;
	}
	switch(pstruBSPhy->nModulation)
	{
	case Modulation_BPSK:
		nBits = 1;
		break;
	case Modulation_QPSK:
		nBits = 2;
		break;
	case Modulation_16_QAM:
		nBits = 4;
		break;
	case Modulation_64_QAM:
		nBits = 6;
		break;
	default:
		nBits = 2;
		break;
	}
	pstruSymbol->nBitsCountInOneSymbol = (int)(pstruSymbol->nDataSubCarrierCount*nBits*dCoding);
	pstruSymbol->nBitsCountInOneSlot = pstruSymbol->nBitsCountInOneSymbol/pstruSymbol->nOFDMSlots;
	pstruSymbol->dDataRate = pstruSymbol->nBitsCountInOneSymbol/pstruSymbol->dSymbolDuration;
	{
		double dDLTime = (10000-pstruBSPhy->dTTG-pstruBSPhy->dRTG)*pstruBSPhy->dDlUlRatio;//Approximation: We have considered TTG as a part of US subframe & Us subframe ~ US subframe + TTG
		double dULTime = (10000-pstruBSPhy->dTTG-pstruBSPhy->dRTG) - dDLTime;
		pstruSymbol->nDownLinkSymbol = (int)(dDLTime/pstruSymbol->dSymbolDuration);
		pstruSymbol->nUPlinkSymbol = (int)(dULTime/pstruSymbol->dSymbolDuration);// This includes a symbol, BW, ranging etc
	}
	//pstruSymbol->nUPlinkFrameStartSymbol = pstruSymbol->nDownLinkSymbol + (int)ceil(pstruBSPhy->dTTG/pstruSymbol->dSymbolDuration);
	pstruSymbol->nUPlinkFrameStartSymbol = pstruSymbol->nDownLinkSymbol ;
	return 1;
}

