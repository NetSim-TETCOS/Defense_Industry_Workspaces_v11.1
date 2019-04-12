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
#ifndef _NETSIM_IEEE802_11_MAC_FRAME_H_
#define _NETSIM_IEEE802_11_MAC_FRAME_H_
#ifdef  __cplusplus
extern "C" {
#endif
	
	// MAC frames
#define MAC_OVERHEAD_802_11n 44
#define MAC_OVERHEAD_802_11ac MAC_OVERHEAD_802_11n
#define MAC_OVERHEAD 40		///< 36 bytes header 4 bytes FCS
#define RTS_SIZE 20			///< 16 bytes header 4 bytes FCS
#define CTS_SIZE 14			///< 10 bytes header 4 bytes FCS
#define ACK_SIZE 14			///< 10 bytes header 4 bytes FCS
#define BlockACK_SIZE_802_11n 32    /**<16 bytes header,2 bytes BA Control, 2 bytes BA starting Sequence Control 
									8bytes BITMAP, 4bytes FCS - Total 32Bytes */
#define BlockACK_SIZE_802_11ac 152  /**< 16 bytes header,2 bytes BA Control, 2 bytes BA starting Sequence Control 
									     128bytes BITMAP, 4bytes FCS - Total 32Bytes */


	typedef struct stru_802_11_MAC_Frame_Header IEEE802_11_MAC_HEADER,*PIEEE802_11_MAC_HEADER;
	typedef struct stru_802_11_RTS_Frame IEEE802_11_RTS,*PIEEE802_11_RTS;
	typedef struct stru_802_11_CTS_Frame IEEE802_11_CTS,*PIEEE802_11_CTS;
	typedef struct stru_802_11_ACK_Frame IEEE802_11_ACK,*PIEEE802_11_ACK;

	/// Enumeration for  WLAN Control packet type
	enum enum_802_11_ControlPacketType		
	{
		WLAN_ACK = MAC_PROTOCOL_IEEE802_11*100+1,
		WLAN_RTS, 
		WLAN_CTS,
		WLAN_BlockACK,
	};

	/** Page-382 IEEE std 802.11-2012 Figure 8-2—Frame Control field */
	typedef struct stru_802_11_FrameControl
	{
		UINT ProtocolVerSion:2;		// 2 bits
		UINT Type:2;				// 2 bits
		UINT SubType:4;				// 4 bits
		UINT ToDS:1;				// 1 bits
		UINT FromDS:1;				// 1 bits
		UINT MoreFragment:1;		// 1 bits
		UINT Retry:1;				// 1 bits
		UINT PowerManagement:1;		// 1 bits
		UINT MoreData:1;			// 1 bits
		UINT ProtectedFrame:1;		// 1 bits
		UINT Order:1;				// 1 bits
	}FRAME_CONTROL,*PFRAME_CONTROL;

	/** Page-389 IEEE std 802.11-2012 Table 8-4—QoS Control field */
	typedef struct stru_802_11_QoS_Control
	{
		UINT TID:4;
		UINT EOSP:1;
		UINT AckPolicy:2;
		UINT AMSDUPresent:1;
		UINT TXOPLimit:8;
	}QOS_CONTROL,*PQOS_CONTROL;

	/** Page-381 IEEE Std 802.11-2012 Figure 8-1—MAC frame format */
	struct stru_802_11_MAC_Frame_Header 
	{
		FRAME_CONTROL FrameControl;		// 2 bytes	
		UINT16 snDurationID;			// 2 bytes
		PNETSIM_MACADDRESS Address1;	// 6 bytes  
		PNETSIM_MACADDRESS Address2;	// 6 bytes  
		PNETSIM_MACADDRESS Address3;	// 6 bytes  

		struct
		{
			UINT FragmentNumber:4;		// 4 bits
			UINT SequenceNumber:12;		// 12 bits
		}SequenceControl;				// 2 bytes

		PNETSIM_MACADDRESS Address4;	// 6 bytes
		QOS_CONTROL QOSControl;			// 2 bytes
		UINT HTControl;					// 4 bytes
		UINT nFCS;						// 4 bytes
	};

	/// Page-404 of IEEE Std 802.11-2012 Figure 8-13—RTS frame
	struct stru_802_11_RTS_Frame
	{
		FRAME_CONTROL FrameControl;	// 2 bytes
		UINT16 Duration;			// 2 bytes
		PNETSIM_MACADDRESS RA;		// 6 bytes
		PNETSIM_MACADDRESS TA;		// 6 bytes
		UINT FCS;					// 4 bytes	
	};

	/// Page-405 of IEEE Std 802.11-2012 Figure 8-14—CTS frame
	struct stru_802_11_CTS_Frame
	{
		FRAME_CONTROL FrameControl; // 2 bytes			
		UINT16 Duration;			// 2 bytes
		PNETSIM_MACADDRESS RA;		// 6 bytes
		UINT FCS;					// 4 bytes
	};

	/// Page-405 of IEEE Std 802.11-2012 Figure 8-15—ACK frame
	struct stru_802_11_ACK_Frame	
	{
		FRAME_CONTROL FrameControl;	// 2 bytes			
		UINT16 Duration;			// 2 bytes
		PNETSIM_MACADDRESS RA;		// 6 bytes
		UINT FCS;					// 4 bytes
	};

	/// Data structure for cumulative acknowledgment.
	typedef struct stru_802_11_BlockACK_Frame
	{
		FRAME_CONTROL FrameControl;			// 2 bytes			
		UINT16 Duration;					// 2 bytes
		PNETSIM_MACADDRESS RA;				// 6 bytes
		PNETSIM_MACADDRESS TA;				// 6 bytes
		struct 
		{
			UINT blockAckPolicy:1;
			UINT multiTID:1;
			UINT compressedBitmap:1;
			UINT reserved:9;
			UINT TID_Info:4;
		}BARControl;
		//UINT16 BARControl;				    // 2 bytes
		struct 
		{
			UINT fragment:4;
			UINT StartingSequenceNumber:12;
		}BAStartingSequenceControl;
		//UINT16 BAStartingSequenceControl;	// 2 bytes
		UINT64 BitMap;					// 8 bytes = 64 bits for n
		UINT nFCS;							// 4 bytes
		// 32bytes total
	}IEEE802_11_BLOCKACK,*PIEEE802_11_BLOCKACK;

#ifdef  __cplusplus
}
#endif
#endif //_NETSIM_IEEE802_11_MAC_FRAME_H_