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
IEEE802.11n. 
This function is called from the "fn_NetSim_WLAN_PHY_Configuration()" for HT PHY.
Initialize time parameters as per the Table 20-5—Timing-related constants 802.11n-2009 pdf 
also initialize 802.11n PHY header relevant parameters.
*/
void fn_NetSim_IEEE802_11n_OFDM_MIMO_init(NETSIM_ID nDeviceId,NETSIM_ID nInterfaceId)
{
	UINT nNSTS_MAX = 4;
	UINT nNSS_MAX = 4;
	PIEEE802_11_PHY_VAR phy = IEEE802_11_PHY(nDeviceId,nInterfaceId);

	// Initialize OFDM timing parameters According Table 20-5—Timing-related constants 802.11n-2009 pdf

	if(phy->dChannelBandwidth == 20)
	{
		phy->PHY_TYPE.ofdmPhy_11n.nCH_BANDWIDTH = HT_CBW20;
		phy->PHY_TYPE.ofdmPhy_11n.nNSD = 52; // NSD Number of complex data numbers	
		phy->PHY_TYPE.ofdmPhy_11n.nNSP = 4;	 // NSP Number of pilot values
		phy->PHY_TYPE.ofdmPhy_11n.nNST = 56; // 52 + 4
		phy->PHY_TYPE.ofdmPhy_11n.nNSR = 28; // NSR Highest data subcarrier index
		phy->PHY_TYPE.ofdmPhy_11n.nFFT = 64; // FFT length	
	}
	else
	{
		phy->PHY_TYPE.ofdmPhy_11n.nCH_BANDWIDTH = HT_CBW40;
		phy->PHY_TYPE.ofdmPhy_11n.nNSD = 108; // NSD Number of complex data numbers	
		phy->PHY_TYPE.ofdmPhy_11n.nNSP = 6;	  // NSP Number of pilot values
		phy->PHY_TYPE.ofdmPhy_11n.nNST = 114; // 108 + 6
		phy->PHY_TYPE.ofdmPhy_11n.nNSR = 58;  // NSR Highest data subcarrier index
		phy->PHY_TYPE.ofdmPhy_11n.nFFT = 128; // FFT length
	}

	// Assign all the timing relevant parameters
	phy->PHY_TYPE.ofdmPhy_11n.dDeltaF = phy->dChannelBandwidth/ (phy->PHY_TYPE.ofdmPhy_11n.nFFT * 1.0);//SubcarrierFrequencySpacing 312.5KHz (20 MHz/64) or (40 MHz/128)
	phy->PHY_TYPE.ofdmPhy_11n.dTDFT = 3.2;  // IDFT/DFT period 3.2 MicroSecs
	phy->PHY_TYPE.ofdmPhy_11n.dTGI = phy->PHY_TYPE.ofdmPhy_11n.dTDFT/4; // 3.2/4 = 0.8 MicroSecs  Guard interval duration 0.8 = TDFT/4  
	phy->PHY_TYPE.ofdmPhy_11n.dTGI2 = phy->PHY_TYPE.ofdmPhy_11n.dTGI * 2; // 0.8 * 2 = 1.6 MicroSecs Double guard interval 1.6 
	phy->PHY_TYPE.ofdmPhy_11n.dTGIS = phy->PHY_TYPE.ofdmPhy_11n.dTDFT/8; // Short guard interval duration   dDFT/8 = 0.4 
	phy->PHY_TYPE.ofdmPhy_11n.nTL_STF = 8;     // Non-HT short training sequence duration 8 = 10*TDFT/4 
	phy->PHY_TYPE.ofdmPhy_11n.nTHT_GF_STF = 8; // HT-greenfield short training field duration 8 =10* TDFT/4 
	phy->PHY_TYPE.ofdmPhy_11n.nTL_LTF = 8;     // Non-HT long training field duration 8 =2 * TDFT + TGI2 
	phy->PHY_TYPE.ofdmPhy_11n.dTSYM  = 4.0;   //TSYM: Symbol interval 4 =  TDFT + TGI       
	phy->PHY_TYPE.ofdmPhy_11n.dTSYMS = 3.6;  // TSYMS: Short GI symbol interval = TDFT+TGIS 3.6 
	phy->PHY_TYPE.ofdmPhy_11n.nTL_SIG = 4;   // Non-HT SIGNAL field duration 4  
	phy->PHY_TYPE.ofdmPhy_11n.nTHT_SIG = 8;  // HT SIGNAL field duration 8 = 2TSYM 8 
	phy->PHY_TYPE.ofdmPhy_11n.nTHT_STF = 4;  // HT short training field duration  4 
	phy->PHY_TYPE.ofdmPhy_11n.nTHT_LTF1 = 8; // First HT long training field duration 4 in HT-mixed format, 8 in HTgreenfield format
	phy->PHY_TYPE.ofdmPhy_11n.nTHT_LTFs = 4; // Second, and subsequent, HT long training fields

	if(phy->nGuardInterval == 400)
		phy->PHY_TYPE.ofdmPhy_11n.nGI_TYPE = SHORT_GI;
	else
		phy->PHY_TYPE.ofdmPhy_11n.nGI_TYPE = LONG_GI;

	// Set the STBC coding
	phy->PHY_TYPE.ofdmPhy_11n.nSTBC = 0;
	// Check and Set NSS value, it should be minimum of transmit and received antenna count
	phy->NSS = min((phy->nNTX < phy->nNRX ? phy->nNTX: phy->nNRX),nNSS_MAX);
	
	// Set NSTS value 
	phy->NSTS = min(phy->NSS  + phy->PHY_TYPE.ofdmPhy_11n.nSTBC,nNSTS_MAX);

	// Set  HT_DLTFs  Table 20-12—Number of HT-DLTFs required for data space-time streams
	phy->N_HT_DLTF = phy->NSTS;
	if(phy->NSTS == 3) 
		phy->N_HT_DLTF = 4;

	// Set NESS   NESS + NSTS <= 4
	phy->NESS =  nNSTS_MAX - phy->NSTS;
	
	// Set HT_ELTFs Table 20-13—Number of HT-ELTFs required for extension spatial streams
	phy->N_HT_ELTF = phy->NESS;
	if(phy->NESS == 3)
		phy->N_HT_ELTF = 4;
	
	// Set total HT_LTF = HT_DLTF + HT_ELTF
	phy->N_HT_LTF = phy->N_HT_DLTF + phy->N_HT_ELTF;
	if(phy->N_HT_LTF > 5) // Max is 5
		phy->N_HT_LTF = 5;

	phy->dControlFrameDataRate = fn_NetSim_IEEE802_11_HTPhy_getCtrlFrameDataRate(phy);
}

/**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
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
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/
double fn_NetSim_IEEE802_11n_TxTimeCalculation(NetSim_PACKET *pstruPacket, NETSIM_ID nDevId, NETSIM_ID nInterfaceId)
{
	double dTxTime = 0.0;
	double dNSYM, dTempNum = 0.0; 
	int mSTBC = 1, nNDBPS, nNCBPS;
	int nNavbits = 1;
	PIEEE802_11_PHY_VAR pstruPhy = IEEE802_11_PHY(nDevId,nInterfaceId);
	double dPacketLength = pstruPacket->pstruPhyData->dPayload;

	if(isIEEE802_11_CtrlPacket(pstruPacket)) // Other than WLAN control packets
	{
		nNDBPS = pstruPhy->PHY_TYPE.ofdmPhy_11n.nNDBPS/pstruPhy->NSS;
		nNCBPS = pstruPhy->PHY_TYPE.ofdmPhy_11n.nNCBPS/pstruPhy->NSS;
	}
	else  
	{
		nNDBPS = pstruPhy->PHY_TYPE.ofdmPhy_11n.nNDBPS;
		nNCBPS = pstruPhy->PHY_TYPE.ofdmPhy_11n.nNCBPS;
	}


	if(pstruPhy->PHY_TYPE.ofdmPhy_11n.nFEC_CODING == BCC_CODING)
	{
		dTempNum =  (8 * dPacketLength) + 16 + ( 6 * pstruPhy->NESS) ;		
		if(pstruPhy->PHY_TYPE.ofdmPhy_11n.nSTBC)
			mSTBC = 2;
		
		dTempNum = 8*dPacketLength;
		dNSYM = mSTBC* ( dTempNum/(mSTBC * nNDBPS));
		dTxTime = pstruPhy->PHY_TYPE.ofdmPhy_11n.dTSYMS * dNSYM ;
		return dTxTime;	
	}
	else
	{
		dNSYM = nNavbits * nNCBPS;
		dTxTime = pstruPhy->PHY_TYPE.ofdmPhy_11n.dTSYMS * dNSYM ;
		return dTxTime;
	}
}


double get_11n_preamble_time(PIEEE802_11_PHY_VAR phy)
{
	double dTGF_HT_PREAMBLE;
	int nSignalExtension = 0;

	dTGF_HT_PREAMBLE = phy->PHY_TYPE.ofdmPhy_11n.nTHT_GF_STF \
		+ phy->PHY_TYPE.ofdmPhy_11n.nTHT_LTF1 \
		+ ((phy->N_HT_LTF - 1) * phy->PHY_TYPE.ofdmPhy_11n.nTHT_LTFs);

	return dTGF_HT_PREAMBLE + 
		phy->PHY_TYPE.ofdmPhy_11n.nTHT_SIG +
		nSignalExtension;
}