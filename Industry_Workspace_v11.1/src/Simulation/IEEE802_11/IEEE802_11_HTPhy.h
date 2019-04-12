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
#ifndef _NETSIM_IEEE802_11N_PHY_H_
#define _NETSIM_IEEE802_11N_PHY_H_
#ifdef  __cplusplus
extern "C" {
#endif
	
	/** 
	Enumerations for 802.11n 
	Table 20-1—TXVECTOR and RXVECTOR parameters page 249- 802.11n-2009.pdf 
	*/
	typedef enum enum_802_11n_FORMAT
	{
		NON_HT, ///< For legacy non 802.11n
		HT_MF,	///< indicates HT-mixed format. legacy + 802.11n
		HT_GF,	///< indicates HT-greenfield format. Only 802.11n supported devices.
		///< We are considered this method only.
	}FORMAT;

	typedef enum enum_802_11_HT_CH_BANDWIDTH
	{
		HT_CBW20,	   ///< 20 MHz and 40 MHz upper and 40 MHz lower modes
		HT_CBW40,	   ///< 40 MHz
		VHT_CBW80,	   ///< 80 MHz
		VHT_CBW160,	   ///< 160 MHz
		NON_VHT_CBW160,  ///< for non-HT duplicate format
		NON_VHT_CBW80,  ///< for all other non-HT formats
		NON_HT_CBW40,  ///< for non-HT duplicate format
		NON_HT_CBW20,  ///< for all other non-HT formats
	}CH_BANDWIDTH;

	//Frequency-domain smoothing is recommended as part of channel estimation.
	typedef enum enum_802_11_HT_SMOOTHING 
	{ 
		SMOOTHING_NOT_REC,	///< smoothing is not recommended.
		SMOOTHING_REC,		///< smoothing is recommended.
	}SMOOTHING;

	/// Indicates whether this packet is a sounding packet.
	typedef enum enum_802_11_HT_SOUNDING 
	{	
		SOUNDING,		///< sounding packet.
		NOT_SOUNDING	///<  not a sounding packet
	}HT_SOUNDING;

	/// Indicates whether the PSDU contains an A-MPDU.
	typedef enum enum_802_11_HT_AGGREGATION 
	{
		NOT_AGGREGATED, ///< Packet does not have A-MPDU aggregation
		AGGREGATED,		///< Packet has A-MPDU aggregation	
	}AGGREGATION;

	/// Indicates which FEC encoding is used.
	typedef enum enum_802_11_HT_FEC_CODING 
	{
		BCC_CODING,		///< Binary convolutional code -BCC
		LDPC_CODING,	///< low-density parity check code - LDPC
	}FEC_CODING;

	/// Guard interval is used in the transmission of the packet.
	typedef enum enum_802_11_HT_GI_TYPE 
	{	
		LONG_GI,	///< Long GI is not used in the packet 800ns
		SHORT_GI,	///< Short GI is used in the packet 400ns
	}GI_TYPE;

	typedef enum enum_802_11_HT_CHAN_MAT_TYPE
	{
		COMPRESSED_SV,		///< CHAN_MAT is a set of compressed beamforming vector matrices.
		NON_COMPRESSED_SV,	///< CHAN_MAT is a set of noncompressed beamforming vector matrices.
		CSI_MATRICES,		///< CHAN_MAT is a set of channel state matrices.
	}CHAN_MAT_TYPE;

	/// Table 20-5—Timing-related constants and Table 20-6—Frequently used parameters
	typedef struct stru_802_11_OFDM_MIMO_Parameters
	{
		double dRxSensitivity;
		PHY_MODULATION modulation;
		double dDataRate;
		IEEE802_11_CODING coding;
		int nNBPSC;
		int nNCBPS;
		int nNDBPS;

		int nChannelBandwidth;	// 20MHz or 40MHz
		int nFFT;			///< FFT length
		int	nNSD;			///< NSD Number of complex data numbers
		int nNSP;			///< NSP Number of pilot values
		int nNST;			///< NST Total number of subcarriers
		int nNSR;			///< NSR Highest data subcarrier index
		double dDeltaF;		///<dSubcarrierFrequencySpacing; // 312.5KHz (20 MHz/64) or (40 MHz/128)
		double dTDFT;		///< IDFT/DFT period 3.2  
		double dTGI;		///< Guard interval duration 0.8 = TDFT/4   
		double dTGI2;		///< Double guard interval 1.6 
		double dTGIS;		///< Short guard interval duration   dDFT/8 0.4 
		int nTL_STF;		///< Non-HT short training sequence duration 8 = 10*TDFT/4 
		int nTHT_GF_STF;	///< HT-greenfield short training field duration 8 =10* TDFT/4 
		int nTL_LTF;		///< Non-HT long training field duration 8 =2 * TDFT + TGI2  
		double dTSYM;		///< TSYM: Symbol interval 4 =  TDFT + TGI  
		double dTSYMS;		///< dShortGIsymbolInterval;      // TSYMS: Short GI symbol interval N/A 3.6  = TDFT+TGIS 3.6 
		int nTL_SIG;		///< Non-HT SIGNAL field duration 4   
		int nTHT_SIG;		///< HT SIGNAL field duration 8 = 2TSYM 8 
		int nTVHT_SIGA;		///< HT SIGNAL field duration = 2TSYM =  8
		int nTVHT_SIGB;		///< HT SIGNAL field duration = TSYM = 4
		int nTHT_STF;		///< HT short training field duration  4 
		int nTHT_LTF1;		///< First HT long training field duration 4 in HT-mixed format, 8 in HTgreenfield format
		int nTHT_LTFs;		///< Second, and subsequent, HT long training fields duration 4 	

		// Table 20-1—TXVECTOR and RXVECTOR parameters page 249- 802.11n-2009.pdf
		FORMAT nFormat;
		int nNonHtModulation;
		int nL_LENGTH;
		int nL_DATARATE;
		int nLSIGVALID;
		int nSERVICE;
		int nTXPWR_LEVEL;
		int nRSSI;
		int nPREAMBLE_TYPE;
		int nMCS;
		int nREC_MCS;
		CH_BANDWIDTH nCH_BANDWIDTH;
		int nCH_OFFSET;
		int nLENGTH;
		SMOOTHING nSMOOTHING;
		HT_SOUNDING nSOUNDING;
		AGGREGATION nAGGREGATION;
		int nSTBC;
		FEC_CODING nFEC_CODING;
		GI_TYPE nGI_TYPE;
		int NUM_EXTEN_SS;
		int ANTENNA_SET;
		int nN_TX;		
		int nEXPANSION_MAT;
		int nEXPANSION_MAT_TYPE;
		int nCHAN_MAT;
		CHAN_MAT_TYPE nCHAN_MAT_TYPE;
		int	nRCPI;
		int nSNR;
		int nNO_SIG_EXTN;	
	}IEEE802_11_OFDM_MIMO;


#ifdef  __cplusplus
}
#endif
#endif //_NETSIM_IEEE802_11N_PHY_H_