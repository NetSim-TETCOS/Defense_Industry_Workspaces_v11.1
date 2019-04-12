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
#ifndef _NETSIM_IEEE802_11_PHYFRAME_H_
#define _NETSIM_IEEE802_11_PHYFRAME_H_
#ifdef  __cplusplus
extern "C" {
#endif

/// Page-1505 IEEE Std 802.11-2012 Figure 16-1—PLCP frame format
typedef struct stru_802_11_DSSS_PLCP_Frame 
{
	// PLCP preamble 144 bits
	long long int nSYNC;			///<128 bits 
	unsigned short int nSFD;		///<16 bits = 1111 0011 1010 0000	
	// PLCP Header 48 bits
	unsigned short int nSIGNAL;		///<8 bits
	unsigned short int nSERVICE;	///<8 bits
	unsigned short int nLENGTH;		///<16 bits
	unsigned short int nCRC;		///<16 bits	
}IEEE802_11_DSSS_PLCP_FRAME,*PIEEE802_11_DSSS_PLCP_FRAME;

/// Page-1588 IEEE Std 802.11-2012 Figure 18-1—PPDU frame format
typedef struct stru_802_11_OFDM_PLCP_Frame			
{
	// PLCP Header 
	int nRATE:4;						///< 4 bits
	bool bReserved;						///< 1 bit
	int bLENGTH:12;						///< 12 bits
	bool bParity;						///< 1 bit
	int nTail_1:6;						///< 6 bits
	int nSERVICE:16;					///< 16 bits													
	int nTail_2:6;						///< 6 bits
	int nPadBits;
}IEEE802_11_OFDM_PLCP_FRAME,*PIEEE802_11_OFDM_PLCP_FRAME;

/// Table 20-4—Elements of the HT PLCP packet  and Figure 20-1—PPDU format
typedef struct stru_802_11_HT_PLCP_Frame 
{
	int L_STF;			///< 8 Microsecs Non-HT Short Training field to support Legacy g and a
	int L_LTF;			///< 8 Microsecs Non-HT Long Training field to support Legacy g and a
	int L_SIG;			///< 4 Microsecs Non-HT SIGNAL field to support Legacy g and a
	int HT_STF;			///< 4 Microsecs HT Short Training field Mixed mode 
	int HT_GF_STF;		///< 8 Microsecs HT-Greenfield Short Training field	
	int HT_LTF1;		///< 8 Microsecs First HT Long Training field (Data)
	//Table 20-10—HT-SIG fields page 275 802.11n-2009.pdf	
	int nMCS:8;					///< 7 bits Modulation and Coding Scheme 7 Index into the MCS table.
	int bCBW_20_40:1;			///< 1 bit Set to 0 for 20 MHz or 40 MHz upper/lower.
								///< Set to 1 for 40 MHz.
	unsigned short int nHT_Length;		///< 16 bits  The number of octets of data in the PSDU in the range of 0 to 65 535.
	int bSmoothing:1;	///< 1 bit  Set to 1 indicates that channel estimate smoothing is recommended.
						///< Set to 0 indicates that only per-carrier independent (unsmoothed) channel estimate is recommended. See 20.3.11.10.1.
	int bNot_Sounding:1;  /**< 1 bit  Set to 0 indicates that PPDU is a sounding PPDU.
						      Set to 1 indicates that the PPDU is not a sounding PPDU. */
	int bReserved:1;		///< 1 bit Set to 1.
	int bAggregation:1;	/**< 1 bit Set to 1 to indicate that the PPDU in the data portion of the packet contains an AMPDU;
						     otherwise, set to 0. */
	int nSTBC:2;		/**< 2 bits Set to a nonzero number, to indicate the difference between the number of spacetime
						    streams ( ) and the number of spatial streams ( ) indicated by the MCS. 
						    Set to 00 to indicate no STBC ( ). */
	int bFEC_coding:1;    ///< 1 bit  Set to 1 for LDPC. Set to 0 for BCC.
	int bShort_GI:1;	  /**< 1 bit  Set to 1 to indicate that the short GI is used after the HT training.
						       Set to 0 otherwise. */
	int Number_Ess:2;		/**< 2 bits  Indicates the number of extension spatial streams ( ).
							    Set to 0 for no extension spatial stream.
							    Set to 1 for 1 extension spatial stream.
						        Set to 2 for 2 extension spatial streams.
						        Set to 3 for 3 extension spatial streams. */
	int CRC:8;			/**< 8 bits  CRC of bits 0–23 in HT-SIG1 and bits 0–9 in HT-SIG2. See 20.3.9.4.4. The first
						     bit to be transmitted is bit C7 as explained in 20.3.9.4.4. */
	int Tail_Bits:7;	///< 6 Used to terminate the trellis of the convolution coder. Set to 0.
	// End of HT SIG

	int HT_LTFs;		///< Additional HT Long Training fields (Data and Extension)	
}IEEE802_11_HT_PLCP_FRAME,*PIEEE802_11_HT_PLCP_FRAME;

typedef struct stru_802_11_PhyHeader
{
	UINT64 transmissionId;
	void* frame;
}IEEE802_11_PHY_HDR,*ptrIEEE802_11_PHY_HDR;

#ifdef  __cplusplus
}
#endif
#endif //_NETSIM_IEEE802_11_PHYFRAME_H_