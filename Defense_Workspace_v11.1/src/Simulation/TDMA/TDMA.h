/************************************************************************************
 * Copyright (C) 2014
 * TETCOS, Bangalore. India															*

 * Tetcos owns the intellectual property rights in the Product and its content.     *
 * The copying, redistribution, reselling or publication of any or all of the       *
 * Product or its content without express prior written consent of Tetcos is        *
 * prohibited. Ownership and / or any other right relating to the software and all  *
 * intellectual property rights therein shall remain at all times with Tetcos.      *
 * Author:	Shashi Kant Suman														*
 * ---------------------------------------------------------------------------------*/
#ifndef _NETSIM_TDMA_H_
#define _NETSIM_TDMA_H_
//For MSVC compiler. For GCC link via Linker command 
#pragma comment(lib,"Metrics.lib")
#pragma comment(lib,"NetworkStack.lib")
#pragma comment(lib,"Mobility.lib")
#pragma comment(lib,"PropagationModel.lib")

#include "List.h"

#ifdef  __cplusplus
extern "C" {
#endif

/***** Default Config Parameter ****/
#define TDMA_EPOCHES_DURATION_DEFAULT			12.8	//minute
#define TDMA_SET_COUNT_DEFAULT					3		//A,B,C
#define TDMA_FRAME_DURATION_DEFAULT				12		//Seconds
#define TDMA_TIME_SLOT_BLOCK_SIZE_DEFAULT		64
#define TDMA_NET_COUNT_DEFAULT					128
#define TDMA_SLOT_DURATION_DEFAULT				7.8125	//millisecond
#define TDMA_GUARD_INTERVAL_DEFAULT				10		//micro second
#define TDMA_INTER_FRAME_TIME_DEFAULT			1		//micro second
#define TDMA_TX_POWER_DEFAULT					20		//mW
#define TDMA_RECEIVER_SENSITIVITY_DBM_DEFAULT	-85		//dBm
#define TDMA_NET_ID_DEFAULT						1
#define TDMA_FREQUENCY_DEFAULT					30		//MHz
#define TDMA_ANTENNA_HEIGHT_DEFAULT				1		//m
#define TDMA_ANTENNA_GAIN_DEFAULT				5		//db
#define TDMA_CHANNEL_BANDWIDTH_DEFAULT			1		//MHz

#define MAX_NET 26

/**** Overhead ******/
#define TDMA_MAC_OVERHEAD		5	//Bytes
#define LINK16_PHY_OVERHEAD		5	//Bytes
#define LINK22_PHY_OVERHEAD		5	//Bytes

#define DATA_RATE 0.025 //mbps


//Typdef struct
typedef struct stru_TDMA_NODE_MAC		TDMA_NODE_MAC;
typedef struct stru_TDMA_NODE_PHY		TDMA_NODE_PHY;
typedef struct stru_Allocation_info		TDMA_ALLOCATION_INFO;

typedef enum
{
	SET_A=1,
	SET_B,
	SET_C,
}SET_ID;

	struct stru_Allocation_info
	{
		NETSIM_ID nNodeId;
		unsigned int nStartingSlotId;
		unsigned int nEndingSlot;
		unsigned int nFrameId;
		double dStartTime;
		double dEndTime;
		_ele* ele;
	};
#define ALLOCATION_INFO_ALLOC() (struct stru_Allocation_info*)list_alloc(sizeof(struct stru_Allocation_info),offsetof(struct stru_Allocation_info,ele))

	struct stru_TDMA_NODE_MAC
	{
		unsigned int nNetCount;
		unsigned int nNetId;
		double dEpochesDuration;
		double dSlotDuration;
		double dFrameDuration;
		unsigned int nSetCount;
		unsigned int nTSBSize;

		double dEpochesCount;
		unsigned int nFrameCount;
		unsigned int nSlotCountInFrame;
		TDMA_ALLOCATION_INFO* allocationInfo;
	};

	struct stru_TDMA_NODE_PHY
	{
		double dFrequency;
		double dAntennaHeight;
		double dAntennaGain;
		double dTXPower;
		double dChannelBandwidth;
		double dReceiverSensitivity;
	};

	unsigned int nTDMAFileFlag;

	PROPAGATION_HANDLE propagationHandle;
	//Propagation macro
#define GET_RX_POWER_dbm(tx,rx) (propagation_get_received_power_dbm(propagationHandle, tx, 1, rx, 1, pstruEventDetails->dEventTime))
#define GET_RX_POWER_mw(tx,rx) (DBM_TO_MW(GET_RX_POWER_dbm(tx,rx)))


	//Function Prototype
	double fnGetTDMAOverHead(NETSIM_ID nNodeId,NETSIM_ID nInterfaceId);
	double fn_NetSim_TDMA_GetPhyOverhead(NetSim_PACKET* packet);
	double fn_NetSim_TDMA_CalulateTxTime(NetSim_PACKET* packet);
	int fn_NetSim_Propagation_CalculatePathLoss(double dTxPower,
		double dDistance,
		double dFrequency,
		double dPathLossExponent,
		double* dReceivedPower);
	int fn_NetSim_Propagation_CalculateShadowLoss(unsigned long* ulSeed1,
		unsigned long* ulSeed2,
		double* dReceivedPower,
		double dStandardDeviation);
	int fn_NetSim_Propagation_CalculateFadingLoss(unsigned long *ulSeed1, unsigned long *ulSeed2,double *dReceivedPower,int fm1);
	int fn_NetSim_CalculateReceivedPower(NETSIM_ID TX,NETSIM_ID RX,double time);
	int fn_NetSim_FormTDMABlock();
	PACKET_STATUS fn_NetSim_TDMA_CalculatePacketError(NetSim_PACKET* packet);
	int fn_NetSim_TDMA_CalulateReceivedPower();
	int fn_NetSim_TDMA_CreateAllocationFile();
	int fn_NetSim_TDMA_NodeInit();
	int fn_NetSim_TDMA_PhysicalOut();
	int fn_NetSim_TDMA_ReadAllocationFile();
	int fn_NetSim_TDMA_RemoveCurrentInfo();
	int fn_NetSim_TDMA_ScheduleDataTransmission(NETSIM_ID nNodeId,double dTime);
	int fn_NetSim_TDMA_TransmitPacket(NetSim_PACKET* packet);
	int fn_NetSim_TDMA_SendToPhy();

#ifdef  __cplusplus
}
#endif
#endif /* _NETSIM_TDMA_H_ */



