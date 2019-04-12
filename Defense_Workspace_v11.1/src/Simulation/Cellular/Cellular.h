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
#ifndef _NETSIM_CELLULAR_H_
#define _NETSIM_CELLULAR_H_
#include "IP_Addressing.h"
#include "GSM.h"
#pragma comment(lib,"Mobility.lib")
typedef struct stru_Cellular_VLR VLR;
typedef struct stru_Cellular_MSC DEVVAR_MSC;
typedef struct stru_Cellular_BaseStation_Phy Cellular_BS_PHY;
typedef struct stru_Cellular_BaseStation_Mac Cellular_BS_MAC;
typedef struct stru_Cellular_MS_Phy Cellular_MS_PHY;
typedef struct stru_Cellular_MS_Mac Cellular_MS_MAC;
typedef struct stru_MS_Metrics Cellular_MS_Metrics;
typedef struct stru_Cellular_ChannelList Cellular_CHANNEL;
typedef struct stru_Cellular_ChannelRequest Cellular_CHANNEL_REQUEST;
typedef struct stru_Cellular_Packet Cellular_PACKET;
typedef struct stru_Cellular_ChannelResponse Cellular_CHANNEL_RESPONSE;

#define DATA_RATE 0.270 //mbps
/// Enumeration for cellular channel types
typedef enum
{
	ChannelType_Pilot=0,
	ChannelType_RACH=0,
	ChannelType_TRAFFICCHANNEL=1,
}CELLULAR_CHANNEL_TYPE;
/// Enumeration for mobile station status
typedef enum
{
	Status_IDLE=0,
	Status_ChannelRequested,
	Status_ChannelRequestedForIncoming,
	Status_ChannelRequestedForHandover,
	Status_CallRequested,
	Status_CallInProgress,
	Status_CallEnd,
}MS_STATUS;
/// Emumeration for cellular packetype
enum enum_cellular_packet_type
{
	PacketType_ChannelRequest,
	PacketType_CallRequest,
	PacketType_ChannelGranted,
	PacketType_ChannelUngranted,
	PacketType_ChannelRequestForIncoming,
	PacketType_CallAccepted,
	PacketType_CallRejected,
	PacketType_ChannelRelease,
	PacketType_ChannelRequestForHandover,
	PacketType_DropCall,
	PacketType_HandoverInfo,
	PacketType_CallEnd,
};
#define CELLULAR_PACKET_TYPE(protocol,type) protocol*100+type
/// Enumeration for cellular subevent
enum enum_cellular_subevent
{
	Subevent_DropCall,
	Subevent_TxNextBurst,
};
#define CELLULAR_SUBEVENT(protocol,type) protocol*100+type
/// Data structure for Visitor Location Register
struct stru_Cellular_VLR
{
	NETSIM_ID nMSId;
	NETSIM_ID nBTSId;
	NETSIM_ID nInterfaceId;
	NETSIM_IPAddress MSIP;
};

/**
	~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	The BTS contains a channel list. It is channel list that are to be used by the				
	BTS & their associated MS. This channel list is static through out the simulation			
	and assigned by the FCA function. 															
	The channel list is defined by following parameters:-										
	1. Channel Id																				
	2. UpLink minimum frequency.																
	3. UpLink maximum frequency																	
	4. Down link minimum frequency.																
	5. Down link maximum frequency																
	6. Time slot		// This is time slot number between 0-7.								
	7. Allocation flag.	// This is a flag stating that channel is free or not.					
	8. Next Channel.																			
																								
	NOTE:-																						
		UpLink Maximum Frequency - UpLink Minimum Frequency = 200 kHz.							
		DownLink Maximum Frequency - DownLink Minimum Frequency = 200 kHz	
	~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~																								
*/
struct stru_Cellular_ChannelList
{
	NETSIM_ID nChannelId;
	NETSIM_ID nBTSId;

	/*** only for GSM ***/
	double dUpLinkMinimumFrequency;
	double dUpLinkMaximumFrequency;
	double dDownLinkMinimumFrequency;
	double dDownLinkMaximumFreqency;
	int nTimeSlot;


	int nAllocationFlag;				/**<
										   ~~~~~~~~~~~~~~~~~~~~~~~
										   0 for unallocated
										   1 for allocated
										   ~~~~~~~~~~~~~~~~~~~~~~~
										*/
	NETSIM_ID nMSId;					/**< 
											~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
											Id of MS for which this channel is allocated.
											0 in case free channel.
											~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
										*/
	NETSIM_ID nDestId;

	int nApplicationId;					/**<
											~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
											Application id for which this channel is allocated.
											0 in case of free channel.
											~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
										*/

	CELLULAR_CHANNEL_TYPE nChannelType;		/**<
												~~~~~~~~~~~~~~~~~~~~~~
												0 for RACH,pilot
											    1 for traffic channel.
												~~~~~~~~~~~~~~~~~~~~~~
											 */


	struct stru_Cellular_ChannelList *pstru_NextChannel;
};
/** Data structure for base station MAC layer */
struct stru_Cellular_BaseStation_Mac
{
	struct stru_BS_GSM_Var
	{
		double dDuplexDistance;
		char* szDuplexTechnique;
		char* szHandoverType;
	}GSMVar;
	struct stru_BS_CDMA_Var
	{
		char* standard;
		double dTotalBandwidth;
		double dChipRate;
		double dVoiceActivityFactor;
		double dTargetSNR;
	}CDMAVar;
	double dChannelDataRate;
	double dBTSRange;
	char* szMultipleAccessTechnology;
	char* szSpeechCoding;

	/*Channel List*/
	unsigned int nChannelCount;
	unsigned int nAllocatedChannel;
	unsigned int nFreeChannel;
	unsigned int nRACHChannel;
	unsigned int nTrafficChannel;
	struct stru_Cellular_ChannelList* pstruChannelList;
};
/** Data structure for Mobile switching centre */
struct stru_Cellular_MSC
{
	int nProtocol;
	unsigned int nBTSCount;
	unsigned int nMSCount;
	NETSIM_ID* BTSList;
	VLR** VLRList;
	MSC_GSM* gsmVar;
};
/** Data structure for mobile station metrics */
struct stru_MS_Metrics
{
	unsigned int nCallGenerated;
	unsigned int nCallBlocked;
	double dCallBlockingProbability;
	unsigned int nChannelRequestSent;
	unsigned int nCallRequestSent;
	unsigned int nCallRequestReceived;
	unsigned int nCallAccepted;
	unsigned int nCallRejected;
	unsigned int nHandoverRequest;
	double dCallDroppingProbability;
	unsigned int nCallDropeed;
};
/** Data structure for mobile station MAC layer */
struct stru_Cellular_MS_Mac
{
	char* szMobilieNumber;
	char* szIMEINumber;

	//For UGS Traffic
	MS_STATUS nMSStatusFlag;
	NETSIM_ID nApplicationId;
	unsigned int nSourceFlag;

	//BTS info
	NETSIM_ID nBTSId;
	NETSIM_ID nBTSInterface;
	Cellular_CHANNEL* pstruAllocatedChannel;

	//Packet list
	NetSim_PACKET**** pstruPacketList; //For each application,source,destination
	NetSim_PACKET**** receivedPacketList;

	//Handover info
	NETSIM_ID nNewBTS;
	NETSIM_ID nLinkId;
	double dHandoverTime;

	//MS Metrics
	Cellular_MS_Metrics MSMetrics;
};
/** Data structure for mobile station PHYSICAL layer */
struct stru_Cellular_MS_Phy
{
	double dTxPower;
	char* szModulation;
};
/** Data structure for base station PHYSICAL layer */
struct stru_Cellular_BaseStation_Phy
{
	double dTxPower;
	char* szModulation;
};
/** Data structure for Channel request */
struct stru_Cellular_ChannelRequest
{
	unsigned int nRequestType;
	NETSIM_ID nMSId;
	NETSIM_ID nApplicationId;
	NETSIM_ID nDestId;
};
/** Data structure for channel response */
struct stru_Cellular_ChannelResponse
{
	unsigned int nAllocationFlag;
	struct stru_Cellular_ChannelList* channel;
};
/** 
Data structure for cellular packet 
*/
struct stru_Cellular_Packet
{
	unsigned int nTimeSlot;

	NETSIM_ID nApplicationId;
	//For fragmentation
	unsigned int isLast;
	unsigned int nId;
	NetSim_PACKET* originalPacket;

	void* controlData;
};
NetSim_PACKET* fn_NetSim_Cellular_createPacket(double time,
											   unsigned int nPacketType,
											   NETSIM_ID nSourceId,
											   NETSIM_ID nDestinationId,
											   double dSize,
											   MAC_LAYER_PROTOCOL protocol);
_declspec(dllexport) int fn_NetSim_FormCDMAChannel(NETSIM_ID nBTSId,Cellular_BS_MAC* BSMac,int nCDMA_ETA,int nCDMA_SIGMA,double dCDMA_DATARATE);
int fn_NetSim_Cellular_MoveMS(NETSIM_ID nDeviceId,NETSIM_ID nBTSId);
int fn_NetSim_Cellular_Run();
int fn_NetSim_Cellular_FreePacket(NetSim_PACKET* packet);
int fn_NetSim_Cellular_FormBurst(NetSim_PACKET* packet,Cellular_MS_MAC* MSMac);
int fn_NetSim_Cellular_MS_SendChannelRelease(Cellular_CHANNEL* channel,NETSIM_ID nMSId,NETSIM_ID nMSInterface,double time);
int isCellularChannelAllocated(NETSIM_ID nMSId,NETSIM_ID nInterfaceId,NETSIM_ID nApplicationId);
int fn_NetSim_Cellular_AddPacketToBuffer(NetSim_PACKET* packet,NETSIM_ID nDeviceId,NETSIM_ID nInterfaceId);
int fn_NetSim_Cellular_AllocateChannel(NetSim_EVENTDETAILS* pstruEventDetails,NetSim_PACKET* packet);
int fn_NetSim_Cellular_ChannelResponse(NetSim_PACKET* packet);
int fn_NetSim_Cellular_MS_ProcessCallRequest();
int fn_NetSim_Cellular_MS_ProcessCallResponse();
int fn_NetSim_Cellular_MS_SendChannelRelease(Cellular_CHANNEL* channel,NETSIM_ID nMSId,NETSIM_ID nMSInterface,double time);
int fn_NetSim_Cellular_DropCall();
int fn_NetSim_Cellular_MS_ReassembleBurst();
int fn_NetSim_Cellular_allocateChannel(NetSim_PACKET* packet);
int fn_NetSim_Cellular_BS_ReleaseChannel();
int fn_NetSim_GSM_BS_PhyOut();
int fn_NetSim_Cellular_BS_AssignTimeSlot(NetSim_PACKET* packet,NETSIM_ID nBTSId);
int fn_NetSim_Cellular_ChannelResponseForHandover();
int fn_NetSim_Cellular_ForwardToMSC();
int fn_NetSim_Cellular_HandoverCall(NETSIM_ID nMSId,NETSIM_ID nMSInterface,double time);
int fn_NetSim_Cellular_InitBTSList(NETSIM_ID nBTSId);
int fn_NetSim_Cellular_InitVLRList(NETSIM_ID nMSId,NETSIM_ID nMSInterface);
int fn_NetSim_Cellular_Metrics(PMETRICSWRITER metricsWriter);
int fn_NetSim_Cellular_MS_PhyOut();
int fn_NetSim_Cellular_Msc_ProcessPacket();
int fn_NetSim_Cellular_SendCallend(NETSIM_ID nMSID,NETSIM_ID nMSInterface,NETSIM_ID nDestinationId,double time);
int fn_NetSim_Cellular_TransmitOnwireline();
int fn_NetSim_FormGSMChannel(DEVVAR_MSC* mscVar);
int fn_NetSim_Cellular_CopyPacket(NetSim_PACKET* pstruDestPacket,NetSim_PACKET* pstruSrcPacket);
#endif