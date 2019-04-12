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
#ifndef _NETSIM_IEEE802_11_H_
#define _NETSIM_IEEE802_11_H_
#ifdef  __cplusplus
extern "C" {
#endif
#ifndef _NO_DEFAULT_LINKER_
//For MSVC compiler. For GCC link via Linker command 
#pragma comment(lib,"IEEE802.11.lib")
#pragma comment(lib,"Metrics.lib")
#pragma comment(lib,"NetworkStack.lib")
#pragma comment(lib,"Mobility.lib")
#pragma comment(lib,"PropagationModel.lib")
#endif

#include "IEEE802_11_enum.h"
#include "IEEE802_11e.h"

// Control frame data rate RTS and CTS
#define CONTRL_FRAME_RATE_11B 1			///< Control frame data rate for IEEE 802.11b in Mbps
#define CONTRL_FRAME_RATE_11A_AND_G 6	///< Control frame data rate for IEEE 802.11a/g in Mbps
#define CONTRL_FRAME_RATE_11P 3			///< Control frame data rate for IEEE 802.11p in Mbps


	/// Page 2341 Enumerated types used in Mac and MLME service primitives
	typedef enum
	{
		INFRASTRUCTURE,
		INDEPENDENT,
		MESH,
		ANY_BSS,
	}IEEE802_11_BSS_TYPE;

	/// Enumeration for WLAN PHY protocols
	typedef enum
	{
		IEEE_802_11a = 1,
		IEEE_802_11b,
		IEEE_802_11g,
		IEEE_802_11p,
		IEEE_802_11n,
		IEEE_802_11e,
		IEEE_802_11ac,
	}IEEE802_11_PROTOCOL;

	/// page-1534 16.4.8.5 CCA, and 17.4.8.5 CCA  IEEE802.11-2012 
	typedef enum
	{
		ED_ONLY=1,			// CCA Mode 1: Energy above threshold.
		CS_ONLY,			// CCA Mode 2: CS only. 
		ED_and_CS,			// CCA Mode 3: CS with energy above threshold. 
		CS_WITH_TIMER,		// CCA Mode 4: CS with timer. 
		HR_CS_and_ED,		// CCA Mode 5: A combination of CS and energy above threshold. 
	}IEEE802_11_CCAMODE;

	/// Page-382 of IEEE Std 802.11-2012 Table 8-1—Valid type and subtype combinations
	enum enum_802_11_FrameControl_Type		
	{
		MANAGEMENT = 0x0,
		CONTROL = 0x1,
		DATA = 0x2,
		RESERVED = 0x3,
	};

	/// Page-382 of IEEE Std 802.11-2012 Table 8-1—Valid type and subtype combinations 
	enum enum_802_11_Management_Frame_SubType
	{
		AssociationRequest = 0x0,
		AssociationResponse	= 0x1,
		ReassociationRequest = 0x2,
		ReassociationResponse = 0x3,
		ProbeRequest = 0x4,
		ProbeResponse = 0x5,
		TimingAdvertisement = 0x6,
		Reserved_1 = 0x7,
		Beacon = 0x8,
		ATIM = 0x9,
		Disassociation = 0xA,
		Authentication = 0xB,
		Deauthentication = 0xC,
		Action = 0xD,
		ActionNoAck	= 0xE,
		Reserved_2 = 0xF,
	};

	/// Page-383 of IEEE Std 802.11-2012 Table 8-1—Valid type and subtype combinations
	enum enum_802_11_Control_Frame_SubType
	{
		Reserved = 0x0,	
		ControlWrapper = 0x7,
		BlockAckRequest,
		BlockAck,
		PS_Poll,
		RTS,
		CTS,
		ACK,
		CF_End,
		CF_End_Plus_CF_Ack,
	};

	/// Page-383 of IEEE Std 802.11-2012 Table 8-1—Valid type and subtype combinations
	enum enum_802_11_Data_Frame_SubType
	{
		Data=0x0,
		Data_Plus_CFAck,
		Data_Plus_CFPoll,
		Data_Plus_CFAck_Plus_CFPoll,
		noData,
		CF_Ack,
		CF_Poll,
		CFAck_Plus_CFPoll,
		QoS_Data,
		QoSData_Plus_CFAck,
		QoSData_Plus_CFPoll,
		QoSData_Plus_CFAck_Plus_CFPoll,
		QoS_Null,
		Reserved_4,
		QoS_CFPoll,
		QoS_CFAck_Plus_CFPoll,
	};

	/// Enumeration for 802.11 MAC states
	typedef enum
	{
		IEEE802_11_MACSTATE_MAC_IDLE = 0,
		IEEE802_11_MACSTATE_WF_NAV,			
		IEEE802_11_MACSTATE_Wait_DIFS,
		IEEE802_11_MACSTATE_BACKING_OFF,
		IEEE802_11_MACSTATE_TXing_MPDU,
		IEEE802_11_MACSTATE_Txing_BroadCast,
		IEEE802_11_MACSTATE_TXing_ACK,
		IEEE802_11_MACSTATE_TXing_RTS,	
		IEEE802_11_MACSTATE_TXing_CTS,
		IEEE802_11_MACSTATE_Wait_DATA,
		IEEE802_11_MACSTATE_Wait_CTS,
		IEEE802_11_MACSTATE_Wait_ACK,
		IEEE802_11_MACSTATE_Wait_BlockACK,
		IEEE802_11_MACSTATE_OFF,
		IEEE802_11_MACSTATE_LAST,
	}IEEE802_11_MAC_STATE;
	static char strIEEE802_11_MAC_STATE[IEEE802_11_MACSTATE_LAST][48] =
	{
		"MAC_IDLE",
		"WAIT_FOR_NAV",
		"WAIT_FOR_DIFS",
		"BACKING_OFF",
		"TRANSMITTING_MPDU",
		"TRANSMITTING_BROADCAST",
		"TRANSMITTING_ACK",
		"TRANSMITTING_RTS",
		"TRANSMITTING_CTS",
		"WAIT_FOR_DATA",
		"WAIT_FOR_CTS",
		"WAIT_FOR_ACK",
		"WAIT_FOR_BLOCKACK",
		"OFF"
		"LAST"
	};

	typedef enum
	{
		DISABLE,
		MINSTREL,
		GENERIC,
	}IEEE802_11_RATE_ADAPTATION;

	/** Structure for metrics */
	typedef struct stru_802_11_Metrics
	{
		UINT nBackoffFailedCount;
		UINT nBackoffSuccessCount;
		UINT nTransmittedFrameCount;
		UINT nReceivedFrameCount;
		UINT nRTSSentCount;
		UINT nCTSSentCount;
		UINT nRTSReceivedCount;
		UINT nCTSReceivedCount;
	}IEEE802_11_METRICS,*PIEEE802_11_METRICS;

	typedef struct stru_802_11_Mac_Var
	{
		NETSIM_ID deviceId;
		NETSIM_ID interfaceId;

		//Config parameter
		IEEE802_11_BSS_TYPE BSSType;
		IEEE802_11_RATE_ADAPTATION rate_adaptationAlgo;
		UINT nRetryLimit;
		UINT nRTSThreshold;
		bool IEEE80211eEnableFlag;
		
		// Mac aggregation
		bool macAggregationStatus;
		UINT nNumberOfAggregatedPackets;
		NetSim_PACKET* blockAckPacket;

		//IEEE802.11e
		QOS_LIST qosList;
		UINT currCwMin;
		UINT currCwMax;

		//CSMA/CA
		IEEE802_11_MAC_STATE currMacState;
		IEEE802_11_MAC_STATE prevMacState;
		double dNAV;
		double dBackOffStartTime;
		int nBackOffCounter;
		double dBackoffLeftTime;

		UINT nRetryCount;
		UINT nCWcurrent;

		//Buffer
		NetSim_PACKET* currentProcessingPacket;
		NetSim_PACKET* waitingforCTS;

		//BSS
		double dDistFromAp;
		PNETSIM_MACADDRESS BSSId;
		NETSIM_ID nBSSId;
		NETSIM_ID nAPInterfaceId;
		UINT devCountinBSS;
		NETSIM_ID* devIdsinBSS;
		NETSIM_ID* devIfinBSS;

		double dPacketProcessingEndTime;

		// Event Id.
		struct
		{
			long long int backoff;
			long long int difsEnd;
		}EVENTID;
		//Metrics
		IEEE802_11_METRICS metrics;

	}IEEE802_MAC_VAR,*PIEEE802_11_MAC_VAR;

#define IEEE802_11_CURR_MAC IEEE802_11_MAC(pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId)

	PROPAGATION_HANDLE propagationHandle;

	//Useful macro
#define isSTAIdle(macVar,phyVar) (macVar->currMacState == IEEE802_11_MACSTATE_MAC_IDLE && phyVar->radio.radioState == RX_ON_IDLE)
#define isCurrSTAIdle isSTAIdle(IEEE802_11_CURR_MAC,IEEE802_11_CURR_PHY)
#define isSTAIdlebyId(devid,ifid) isSTAIdle(IEEE802_11_MAC(devid,ifid),IEEE802_11_PHY(devid,ifid))
#define isCurrSTAMediumIdle() (isMediumIdle(pstruEventDetails->nDeviceId, pstruEventDetails->nInterfaceId))
	//Function prototype
	//Mesh BSS
	void fn_NetSim_802_11_MeshBSS_UpdateReceiver(NetSim_PACKET* packet);
	bool isPacketforsameMeshBSS(PIEEE802_11_MAC_VAR mac,NetSim_PACKET* packet);

	//Infrastructure BSS
	void fn_NetSim_802_11_InfrastructureBSS_UpdateReceiver(NetSim_PACKET* packet);
	bool isPacketforsameInfrastructureBSS(PIEEE802_11_MAC_VAR mac,NetSim_PACKET* packet);

	//Mac
	bool isMacTransmittingState(PIEEE802_11_MAC_VAR mac);
	bool isMacReceivingState(PIEEE802_11_MAC_VAR mac);
	bool isMacIdle(PIEEE802_11_MAC_VAR mac);
	void IEEE802_11_Change_Mac_State(PIEEE802_11_MAC_VAR mac,IEEE802_11_MAC_STATE state);
	void fn_NetSim_IEE802_11_MacReInit(NETSIM_ID nDeviceId,NETSIM_ID nInterfaceId);
	void fn_NetSim_IEEE802_11_Timer();
	void fn_NetSim_IEEE802_11_MacOut();
	int fn_NetSim_IEEE802_11_MacIn();
	double calculate_nav(NETSIM_ID d, NETSIM_ID i, NetSim_PACKET* packet);

	//CSMA/CA
	int fn_NetSim_IEEE802_11_CSMACA_Init();
	bool fn_NetSim_IEEE802_11_CSMACA_CS();
	int fn_NetSim_IEEE802_11_CSMACA_CheckNAV();
	void fn_NetSim_IEEE802_11_CSMACA_DIFSEnd();
	void ieee802_11_csmaca_difs_failed(PIEEE802_11_MAC_VAR mac);
	void fn_NetSim_IEEE802_11_CSMACA_ProcessAck();
	void fn_NetSim_IEEE802_11_CSMACA_IncreaseCW(PIEEE802_11_MAC_VAR mac);
	bool fn_NetSim_IEEE802_11_CSMACA_CheckRetryLimit(PIEEE802_11_MAC_VAR mac);
	void fn_NetSim_IEEE802_11_CSMACA_ProcessBlockAck();
	void fn_NetSim_IEEE802_11_CSMA_AckTimeOut();
	int fn_NetSim_IEEE802_11_CSMACA_SendBlockACK();
	int fn_NetSim_IEEE802_11_CSMACA_SendACK();
	bool fn_NetSim_IEEE802_11_CSMACA_Backoff();
	void ieee802_11_csmaca_pause_backoff(PIEEE802_11_MAC_VAR mac);
	void fn_NetSim_IEEE802_11_CSMACA_AddAckTimeOut(NetSim_PACKET* packet,NETSIM_ID devId,NETSIM_ID devIf);


	//Mac Frame
	bool isIEEE802_11_CtrlPacket(NetSim_PACKET* packet);
	double getAckSize(void* phy);
	double getCTSSize();
	double getRTSSize();
	double getMacOverhead(void* phy, double size);
	double calculate_CTS_duration(NETSIM_ID d,
								  NETSIM_ID i,
								  double rtsduration);
	bool is_more_fragment_coming(NetSim_PACKET* packet);
	bool is_first_packet(NetSim_PACKET* packet);
	NetSim_PACKET* fn_NetSim_IEEE802_11_CreateRTSPacket(NetSim_PACKET* data,double duration);
	NetSim_PACKET* fn_NetSim_IEEE802_11_CreateCTSPacket(NetSim_PACKET* data);
	NetSim_PACKET* fn_NetSim_IEEE802_11_CreateAckPacket(NETSIM_ID nDeviceId,NETSIM_ID nInterfaceId,NetSim_PACKET* data,double time);
	NetSim_PACKET* fn_NetSim_IEEE802_11_CreateBlockAckPacket(NETSIM_ID nDeviceId,NETSIM_ID nInterfaceId,NetSim_PACKET* data,double time);
	void fn_NetSim_IEEE802_11_Add_MAC_Header(NETSIM_ID nDeviceId,NETSIM_ID nInterfaceId, NetSim_PACKET *pstruPacket,unsigned int i);
	void set_blockack_bitmap(NetSim_PACKET* ackPacket,NetSim_PACKET* packet);
	void fn_NetSim_Process_CtrlPacket();
	void free_ieee802_11_mac_header(NetSim_PACKET* packet);
	void copy_ieee802_11_mac_header(NetSim_PACKET* src,NetSim_PACKET* dest);

	//RTS CTS
	void fn_NetSim_IEEE802_11_RTS_CTS_Init();
	void fn_NetSim_IEEE802_11_RTS_CTS_CTSTimeOut();
	void fn_NetSim_IEEE802_11_RTS_CTS_ProcessRTS();
	void fn_NetSim_IEEE802_11_RTS_CTS_ProcessCTS();
	void fn_NetSim_IEEE802_11_RTS_CTS_SendCTS();
	void fn_NetSim_IEEE802_11_RTS_CTS_AddCTSTimeOut(NetSim_PACKET* packet,NETSIM_ID devId,NETSIM_ID devIf);

	//Rate adaptation
	void Generic_Rate_adaptation_init(NETSIM_ID nDevId,NETSIM_ID nifid);
	void free_rate_adaptation_data(void* phy);
	void packet_recv_notify(NETSIM_ID devid,NETSIM_ID ifid,NETSIM_ID rcvid);
	void packet_drop_notify(NETSIM_ID devid,NETSIM_ID ifid,NETSIM_ID rcvid);
	unsigned int get_rate_index(NETSIM_ID devid,NETSIM_ID ifid,NETSIM_ID rcvid);

	//Lib function prototype
	_declspec(dllexport) int fn_NetSim_IEEE802_11_FreePacket(NetSim_PACKET* pstruPacket);
	int fn_NetSim_IEEE802_11_Finish_F();

	//MINSTREL 
	void InitMinstrel(NETSIM_ID nDevId,NETSIM_ID nifid);
	void Minstrel_Init(NETSIM_ID nDevId,NETSIM_ID nifid);
	void Ht_InitMinstrel(NETSIM_ID nDevId,NETSIM_ID nifid);
	void DoReportDataFailed(NETSIM_ID dev,NETSIM_ID ifid,NETSIM_ID recv);
	void DoReportDataOk(NETSIM_ID dev,NETSIM_ID ifid,NETSIM_ID recv);
	void DoReportFinalDataFailed(NETSIM_ID dev,NETSIM_ID ifid,NETSIM_ID recv);
	BOOL DoNeedDataRetransmission(NETSIM_ID dev,NETSIM_ID ifid,NETSIM_ID recv);
	UINT get_minstrel_rate_index(NETSIM_ID dev,NETSIM_ID ifid,NETSIM_ID recv);
	void FreeMinstrel(NETSIM_ID nDevId, NETSIM_ID nifid);
	void DoReportAmpduStatus(NETSIM_ID devid,NETSIM_ID ifid,NETSIM_ID recvid,UINT success,UINT failed);
	BOOL Ht_DoNeedDataRetransmission(NETSIM_ID devid,NETSIM_ID ifid,NETSIM_ID recvid);
	BOOL Minstrel_DoNeedDataSend(NETSIM_ID nDevId,NETSIM_ID nifid,NETSIM_ID recvid);
	void Minstrel_ReportDataFailed(NETSIM_ID nDevId,NETSIM_ID nifid,NETSIM_ID recvid);
	void Minstrel_ReportFinalDataFailed(NETSIM_ID nDevId,NETSIM_ID nifid,NETSIM_ID recvid);
	struct stru_802_11_Phy_Parameters_HT getTxRate(NETSIM_ID devid,NETSIM_ID ifid,NETSIM_ID recvid);
	void HT_Minstrel_Free(NETSIM_ID nDevId, NETSIM_ID nifid);

	//IEEE1609 Interface
	PIEEE802_11_MAC_VAR IEEE802_11_MAC(NETSIM_ID ndeviceId, NETSIM_ID nInterfaceId);
	void SET_IEEE802_11_MAC(NETSIM_ID ndeviceId, NETSIM_ID nInterfaceId, PIEEE802_11_MAC_VAR mac);
	bool isIEEE802_11_Configure(NETSIM_ID ndeviceId, NETSIM_ID nInterfaceId);
	bool validate_processing_time(double time, NETSIM_ID nDeviceId, NETSIM_ID nInterfaceId);
	NetSim_PACKET* get_from_queue(NETSIM_ID nDeviceId, NETSIM_ID nInterfaceId, int nPacketRequire, int* nPacketCount);
	void add_to_queue(NETSIM_ID nDeviceId, NETSIM_ID nInterfaceId, NetSim_PACKET* packet);
	void readd_to_queue(NETSIM_ID nDeviceId, NETSIM_ID nInterfaceId, NetSim_PACKET* packet);

	//Medium
	void medium_change_callbackHandler(NETSIM_ID d,
									   NETSIM_ID in,
									   bool status);
	bool medium_isRadioIdleHandler(NETSIM_ID d,
								   NETSIM_ID in);
	bool medium_isTransmitterBusyHandler(NETSIM_ID d,
										 NETSIM_ID in);
	double medium_getRxPowerHandler(NETSIM_ID tx,
									NETSIM_ID txif,
									NETSIM_ID rx,
									NETSIM_ID rxif,
									double time);

	//Log
	bool isIEEE802_11_log();
	void print_ieee802_11_log(char* format, ...);
#ifdef  __cplusplus
}
#endif
#endif //_NETSIM_IEEE802_11_H_