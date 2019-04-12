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

/**
This function is called to initialize the MIMO and OFDM parameters relevant to 
IEEE802.11ac. 
This function is called from the "fn_NetSim_WLAN_PHY_Configuration()" for HT PHY.
Initialize time parameters as per the Table 20-5—Timing-related constants 802.11n-2009 pdf 
also initialize 802.11ac PHY header relevant parameters.
*/
void fn_NetSim_IEEE802_11ac_OFDM_MIMO_init(NETSIM_ID nDeviceId,NETSIM_ID nInterfaceId)
{
	UINT nNSTS_MAX = 8;
	UINT nNSS_MAX = 8;
	PIEEE802_11_PHY_VAR phy = IEEE802_11_PHY(nDeviceId,nInterfaceId);
	
	// Initialize OFDM timing parameters According Table 20-5—Timing-related constants 802.11n-2009 pdf

	if(phy->dChannelBandwidth == 20)
	{
		phy->PHY_TYPE.ofdmPhy_11ac.nCH_BANDWIDTH = HT_CBW20;
		phy->PHY_TYPE.ofdmPhy_11ac.nNSD = 52; // NSD Number of complex data numbers	
		phy->PHY_TYPE.ofdmPhy_11ac.nNSP = 4;	 // NSP Number of pilot values
		phy->PHY_TYPE.ofdmPhy_11ac.nNST = 56; // 52 + 4
		phy->PHY_TYPE.ofdmPhy_11ac.nNSR = 28; // NSR Highest data subcarrier index
		phy->PHY_TYPE.ofdmPhy_11ac.nFFT = 64; // FFT length	
	}
	else if(phy->dChannelBandwidth == 40)
	{
		phy->PHY_TYPE.ofdmPhy_11ac.nCH_BANDWIDTH = HT_CBW40;
		phy->PHY_TYPE.ofdmPhy_11ac.nNSD = 108; // NSD Number of complex data numbers	
		phy->PHY_TYPE.ofdmPhy_11ac.nNSP = 6;	  // NSP Number of pilot values
		phy->PHY_TYPE.ofdmPhy_11ac.nNST = 114; // 108 + 6
		phy->PHY_TYPE.ofdmPhy_11ac.nNSR = 58;  // NSR Highest data subcarrier index
		phy->PHY_TYPE.ofdmPhy_11ac.nFFT = 128; // FFT length
	}
	else if(phy->dChannelBandwidth == 80)
	{
		phy->PHY_TYPE.ofdmPhy_11ac.nCH_BANDWIDTH = VHT_CBW80;
		phy->PHY_TYPE.ofdmPhy_11ac.nNSD = 234; // NSD Number of complex data numbers	
		phy->PHY_TYPE.ofdmPhy_11ac.nNSP = 8;	  // NSP Number of pilot values
		phy->PHY_TYPE.ofdmPhy_11ac.nNST = 242; // 234 + 8
		phy->PHY_TYPE.ofdmPhy_11ac.nNSR = 122;  // NSR Highest data subcarrier index
		phy->PHY_TYPE.ofdmPhy_11ac.nFFT = 256; // FFT length
	}
	else if(phy->dChannelBandwidth == 160)
	{
		phy->PHY_TYPE.ofdmPhy_11ac.nCH_BANDWIDTH = VHT_CBW160;
		phy->PHY_TYPE.ofdmPhy_11ac.nNSD = 468; // NSD Number of complex data numbers	
		phy->PHY_TYPE.ofdmPhy_11ac.nNSP = 16;	  // NSP Number of pilot values
		phy->PHY_TYPE.ofdmPhy_11ac.nNST = 484; // 468 + 16
		phy->PHY_TYPE.ofdmPhy_11ac.nNSR = 250;  // NSR Highest data subcarrier index
		phy->PHY_TYPE.ofdmPhy_11ac.nFFT = 512; // FFT length
	}

	// Assign all the timing relevent parameters
	phy->PHY_TYPE.ofdmPhy_11ac.dDeltaF = phy->dChannelBandwidth / (phy->PHY_TYPE.ofdmPhy_11ac.nFFT * 1.0);//SubcarrierFrequencySpacing 312.5KHz (20 MHz/64) or (40 MHz/128)
	phy->PHY_TYPE.ofdmPhy_11ac.dTDFT = 3.2;  // IDFT/DFT period 3.2 MicroSecs
	phy->PHY_TYPE.ofdmPhy_11ac.dTGI = phy->PHY_TYPE.ofdmPhy_11ac.dTDFT/4; // 3.2/4 = 0.8 MicroSecs  Guard interval duration 0.8 = TDFT/4  
	phy->PHY_TYPE.ofdmPhy_11ac.dTGI2 = phy->PHY_TYPE.ofdmPhy_11ac.dTGI * 2; // 0.8 * 2 = 1.6 MicroSecs Double guard interval 1.6 
	phy->PHY_TYPE.ofdmPhy_11ac.dTGIS = phy->PHY_TYPE.ofdmPhy_11ac.dTDFT/8; // Short guard interval duration   dDFT/8 = 0.4 
	phy->PHY_TYPE.ofdmPhy_11ac.nTL_STF = 8;     // Non-VHT short training sequence duration 8 = 10*TDFT/4 
	phy->PHY_TYPE.ofdmPhy_11ac.nTHT_GF_STF = 8; // HT-greenfield short training field duration 8 =10* TDFT/4 
	phy->PHY_TYPE.ofdmPhy_11ac.nTL_LTF = 8;     // Non-HT long training field duration 8 =2 * TDFT + TGI2 
	phy->PHY_TYPE.ofdmPhy_11ac.dTSYM  = 4.0;   //TSYM: Symbol interval 4 =  TDFT + TGI       
	phy->PHY_TYPE.ofdmPhy_11ac.dTSYMS = 3.6;  // TSYMS: Short GI symbol interval = TDFT+TGIS 3.6 
	phy->PHY_TYPE.ofdmPhy_11ac.nTL_SIG = 4;   // Non-HT SIGNAL field duration 4  
	phy->PHY_TYPE.ofdmPhy_11ac.nTVHT_SIGA = 8;   
	phy->PHY_TYPE.ofdmPhy_11ac.nTVHT_SIGB = 4;  
	phy->PHY_TYPE.ofdmPhy_11ac.nTHT_STF = 4;  // HT short training field duration  4 
	phy->PHY_TYPE.ofdmPhy_11ac.nTHT_LTF1 = 8; // First HT long training field duration 4 in HT-mixed format, 8 in HTgreenfield format
	
	if(phy->nGuardInterval == 400)
		phy->PHY_TYPE.ofdmPhy_11ac.nGI_TYPE = SHORT_GI;
	else
		phy->PHY_TYPE.ofdmPhy_11ac.nGI_TYPE = LONG_GI;

	// Set the STBC coding
	phy->PHY_TYPE.ofdmPhy_11n.nSTBC = 0;

	// Check and Set NSS value, it should be minimum of transmit and received antenna count
	phy->NSS = min((phy->nNTX < phy->nNRX ? phy->nNTX: phy->nNRX),nNSS_MAX);

	// Set NSTS value 
	phy->NSTS = min(phy->NSS  + phy->PHY_TYPE.ofdmPhy_11n.nSTBC,nNSTS_MAX);

	// Set HT_ELTFs Table 20-13—Number of HT-ELTFs required for extension spatial streams
	phy->N_HT_ELTF = phy->NESS;
	if(phy->NESS%2 !=0)
		phy->N_HT_ELTF += 1;

	// Set total HT_LTF = HT_DLTF + HT_ELTF
	phy->N_HT_LTF = phy->N_HT_DLTF + phy->N_HT_ELTF;
	if(phy->N_HT_LTF > 9) // Max is 9
		phy->N_HT_LTF = 9;

	phy->dControlFrameDataRate = fn_NetSim_IEEE802_11_HTPhy_getCtrlFrameDataRate(phy);
}

/**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
This function is used to calculate the transmission time. If flag=1, then preamble time is
also taken into consideration. else only the time required to transmit the packetsize 
specified will be returned.
For Short GI
TXTIME = TGF_HT_PREAMBLE + THT_SIG + TSYMS * NSYM + SignalExtension
For Regular/ Long GI
TXTIME = TGF_HT_PREAMBLE + THT_SIG + TSYM * NSYM + SignalExtension
TGF_HT_PREAMBLE    is the duration of the preamble in HT_greenfield format, given by
TGF_HT_PREAMBLE = THT_GF_STF +  THT_ LTF1  +  (NLTF – 1)THT_ LTFs
TSYM, TSYMS, THT-SIG, TL-STF, THT-STF, THT-GF-STF, TL-LTF, THT-LTF1 and THT-LTFs defined in Table20-5 Timing-related constants
SignalExtension   is 0 microsecs when TXVECTOR parameter NO_SIG_EXTN is TRUE and is the duration of signal extension as defined by aSignalExtension in
Table 20- 4 when TXVECTOR parameter NO_SIG_EXTN is FALSE NLTF is defined in Equation (20-22)
NSYM is the total number of data symbols in the data portion, which may be calculated according to Equation (20-95)
For BCC coding 
NSYM = mSTBC * ceil ( (8*length + 16 + 6*NES)/(mSTBC*NDBPS) )
For LDPC
NSYM = Navbits / NCBPS 
length 				is the number of octets in the data portion of the PPDU
mSTBC 				is equal to 2 when STBC is used, and otherwise 1
NES and NCBPS 		are defined in Table 20-6
NDBPS				 is defined in Table 20-28
Navbits   			 is defined in Equation (20-39)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/
double fn_NetSim_IEEE802_11ac_TxTimeCalculation(NetSim_PACKET *pstruPacket, NETSIM_ID nDevId, NETSIM_ID nInterfaceId)
{
	double dTxTime = 0.0;
	double dNSYM, dTempNum = 0.0; 
	int mSTBC = 1, nNDBPS, nNCBPS;
	int nNavbits = 1;
	PIEEE802_11_PHY_VAR pstruPhy = IEEE802_11_PHY(nDevId,nInterfaceId);
	double dPacketLength = pstruPacket->pstruPhyData->dPayload;

	if(isIEEE802_11_CtrlPacket(pstruPacket)) // Other than WLAN control packets
	{
		nNDBPS = pstruPhy->PHY_TYPE.ofdmPhy_11ac.nNDBPS/pstruPhy->NSS;
		nNCBPS = pstruPhy->PHY_TYPE.ofdmPhy_11ac.nNCBPS/pstruPhy->NSS;
	}
	else  
	{
		nNDBPS = pstruPhy->PHY_TYPE.ofdmPhy_11ac.nNDBPS;
		nNCBPS = pstruPhy->PHY_TYPE.ofdmPhy_11ac.nNCBPS;
	}


	if(pstruPhy->PHY_TYPE.ofdmPhy_11ac.nFEC_CODING == BCC_CODING)
	{
		dTempNum =  (8 * dPacketLength) + 16 + ( 6 * pstruPhy->NESS) ;		
		if(pstruPhy->PHY_TYPE.ofdmPhy_11ac.nSTBC)
			mSTBC = 2;

		dTempNum = 8*dPacketLength;
		dNSYM = mSTBC* ( dTempNum/(mSTBC * nNDBPS));
		dTxTime = pstruPhy->PHY_TYPE.ofdmPhy_11ac.dTSYMS * dNSYM ;
		return dTxTime;	
	}
	else
	{
		dNSYM = nNavbits * nNCBPS;
		dTxTime = pstruPhy->PHY_TYPE.ofdmPhy_11ac.dTSYMS * dNSYM ;
		return dTxTime;
	}
}

double get_11ac_preamble_time(PIEEE802_11_PHY_VAR phy)
{
	double dTGF_HT_PREAMBLE;
	int nSignalExtension = 0;

	dTGF_HT_PREAMBLE = phy->PHY_TYPE.ofdmPhy_11ac.nTHT_GF_STF \
		+ 2*phy->PHY_TYPE.ofdmPhy_11ac.nTHT_LTF1
		+ phy->PHY_TYPE.ofdmPhy_11ac.nTHT_STF
		+ phy->PHY_TYPE.ofdmPhy_11ac.nTL_SIG;

	return dTGF_HT_PREAMBLE + 
		phy->PHY_TYPE.ofdmPhy_11ac.nTVHT_SIGA +
		phy->PHY_TYPE.ofdmPhy_11ac.nTVHT_SIGB +
		nSignalExtension;
}