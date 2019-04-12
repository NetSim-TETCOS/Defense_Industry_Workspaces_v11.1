/************************************************************************************
* Copyright (C) 2015
* TETCOS, Bangalore. India															*

* Tetcos owns the intellectual property rights in the Product and its content.     *
* The copying, redistribution, reselling or publication of any or all of the       *
* Product or its content without express prior written consent of Tetcos is        *
* prohibited. Ownership and / or any other right relating to the software and all  *
* intellectual property rights therein shall remain at all times with Tetcos.      *
* Author:	Shashi Kant Suman														*
* ---------------------------------------------------------------------------------*/
#ifndef _NETSIM_DTDMA_H_
#define _NETSIM_DTDMA_H_
//For MSVC compiler. For GCC link via Linker command 
#pragma comment(lib,"Metrics.lib")
#pragma comment(lib,"NetworkStack.lib")
#pragma comment(lib,"Mobility.lib")
#pragma comment(lib,"PropagationModel.lib")

#include "List.h"
#include "ErrorModel.h"

#ifdef  __cplusplus
extern "C" {
#endif

	/***** Default Config Parameter ****/
#define DTDMA_BANDWIDTH_DEFAULT					20		//MHz
#define DTDMA_LOWER_FREQUENCY_DEFAULT			30		//MHz
#define DTDMA_UPPER_FREQUENCY_DEFAULT			300		//MHz
#define DTDMA_FREQUENCY_HOPPING_DEFAULT			_strdup("ON")
#define DTDMA_FRAME_DURATION_DEFAULT			1		//Seconds
#define DTDMA_NET_COUNT_DEFAULT					128
#define DTDMA_SLOT_DURATION_DEFAULT				2.0		//millisecond
#define DTDMA_GUARD_INTERVAL_DEFAULT			100		//micro second
#define DTDMA_TX_POWER_DEFAULT					20		//W
#define DTDMA_RECEIVER_SENSITIVITY_DBM_DEFAULT	-101	//dBm
#define DTDMA_NET_ID_DEFAULT					1
#define DTDMA_BITS_PER_SLOT_DEFAULT				3000	//bits
#define DTDMA_OVERHEADS_PER_SLOT_DEFAULT		600		//bits
#define DTDMA_MODULATION_TECHNIQUE_DEFAULT		_strdup("QPSK")
#define DTDMA_NETWORK_TYPE_DEFAULT				_strdup("AD-HOC")
#define DTDMA_SLOT_ALLOCATION_TECHNIQUE_DEFAULT	_strdup("ROUNDROBIN")
#define DTDMA_MAX_SLOT_PER_DEVICE_DEFAULT		5
#define DTDMA_MIN_SLOT_PER_DEVICE_DEFAULT		1
#define DTDMA_ANTENNA_HEIGHT_DEFAULT			1
#define DTDMA_ANTENNA_GAIN_DEFAULT				1
#define DTDMA_D0_DEFAULT						1
#define DTDMA_PL_D0_DEFAULT						-30

#define MAX_NET 128
#define DTDMA_INTER_FRAME_GAP					1		//micro second


	//Typedef struct
	typedef struct stru_DTDMA_NODE_MAC		DTDMA_NODE_MAC,*PDTDMA_NODE_MAC;
	typedef struct stru_DTDMA_NODE_PHY		DTDMA_NODE_PHY,*PDTDMA_NODE_PHY;

	typedef enum
	{
		ADHOC,
		INFRASTRUCTURE,
	}NETWORK_TYPE;

	typedef enum
	{
		TECHNIQUE_ROUNDROBIN,
		TECHNIQUE_DEMANDBASED,
		TECHNIQUE_FILE,
	}SLOT_ALLOCATION_TECHNIQUE;

	typedef enum enum_DTDMA_Packet
	{
		DTDMA_FRAME=MAC_PROTOCOL_DTDMA*100,
	}DTDMA_PACKET;

	typedef enum
	{
		FIRST_FRAGMENT,
		CONTINUE_FRAGMENT,
		LAST_FRAGMENT,
	}FRAGMENT;

	typedef struct 
	{
		unsigned long long int segment_id;
		unsigned int fragment_id;
		FRAGMENT frag;
	}DTDMA_MAC_PACKET,*PDTDMA_MAC_PACKET;
	
	typedef struct stru_slot_info
	{
		NETSIM_ID device_id;
		NETSIM_ID interfaceId;
		unsigned int slot_id;
		double dSlotStartTime;
	}SLOT_INFO,*PSLOT_INFO;
	typedef struct stru_Allocation_info
	{
		unsigned int nSlotCount;
		PSLOT_INFO* slot_info;

		unsigned int nFrameId;
		unsigned int nCurrentSlot;
		unsigned int nNextSlot;
		double dFrameStartTime;
		double dSlotStartTime;
		double dNextSlotStartTime;
	}DTDMA_ALLOCATION_INFO, *ptrDTDMA_ALLOCATION_INFO;
	ptrDTDMA_ALLOCATION_INFO get_allocation_info(NETSIM_ID d, NETSIM_ID in);

	typedef struct stru_dtdma_node_info
	{
		NETSIM_ID nodeId;
		NETSIM_ID interfaceId;
		NODE_ACTION nodeAction;
	}DTDMA_NODE_INFO, *ptrDTDMA_NODE_INFO;

	typedef struct stru_NetInfo
	{
		NETSIM_ID net_id;
		NETSIM_ID nDeviceCount;
		ptrDTDMA_NODE_INFO* nodeInfo;
	}DTDMA_NET_INFO, *ptrDTDMA_NET_INFO;

	typedef struct stru_dtdma_session
	{
		NETSIM_ID linkId;
		UINT netCount;
		ptrDTDMA_NET_INFO* netInfo;
		ptrDTDMA_ALLOCATION_INFO allocation_info;
		_ptr_ele ele;
	}DTDMA_SESSION, *ptrDTDMA_SESSION;
#define DTDMA_SESSION_ALLOC() (ptrDTDMA_SESSION)list_alloc(sizeof(DTDMA_SESSION),offsetof(DTDMA_SESSION,ele))
	ptrDTDMA_SESSION dtdmaSession;
	void init_dtdma_session();
	void free_dtdma_session();
	ptrDTDMA_SESSION find_session(NETSIM_ID linkId);
	ptrDTDMA_NET_INFO find_net(ptrDTDMA_SESSION session, NETSIM_ID netId);
	ptrDTDMA_NET_INFO get_dtdma_net(NETSIM_ID d, NETSIM_ID in);

	struct stru_DTDMA_NODE_MAC
	{
		NETWORK_TYPE network_type;
		SLOT_ALLOCATION_TECHNIQUE slot_allocation_technique;
		
		//Demand based slot allocation
		unsigned int max_slot;
		unsigned int min_slot;

		unsigned int nNetCount;
		unsigned int nNetId;
		double dSlotDuration;
		double dFrameDuration;
		double dGuardInterval;
		unsigned int nFrameCount;
		unsigned int nSlotCountInFrame;

		unsigned int bitsPerSlot;
		unsigned int overheadPerSlot;

		NetSim_PACKET* fragment;
		unsigned long long int segment_id;
		unsigned int fragment_id;
		NetSim_PACKET** reassembly;

		//DTDMA session
		ptrDTDMA_SESSION session;
		ptrDTDMA_NET_INFO netInfo;
		ptrDTDMA_NODE_INFO nodeInfo;
	};
#define DTDMA_MAC(devid,ifid) ((DTDMA_NODE_MAC*)DEVICE_MACVAR(devid,ifid))
#define DTDMA_CURR_MAC DTDMA_MAC(pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId)

	struct stru_DTDMA_NODE_PHY
	{
		double dBandwidth;
		double dFrequency;
		double dLowerFrequency;
		double dUpperFrequency;
		bool frequency_hopping;
		double dTXPower;
		double dReceiverSensitivity;
		PHY_MODULATION modulation;
		double dDataRate;
		double dAntennaHeight;
		double dAntennaGain;
		double d0;
		double pld0;
	};
#define DTDMA_PHY(devid,ifid) (DTDMA_NODE_PHY*)DEVICE_PHYVAR(devid,ifid)

	unsigned int nDTDMAFileFlag;

	PROPAGATION_HANDLE propagationHandle;
	//Propagation macro
	static double GET_RX_POWER_dbm(NETSIM_ID tx, NETSIM_ID txi, NETSIM_ID rx, NETSIM_ID rxi)
	{
		if (NETWORK->ppstruDeviceList[tx - 1]->node_status == NOT_CONNECTED)
			return -9999;
		else if (NETWORK->ppstruDeviceList[rx-1]->node_status == NOT_CONNECTED)
			return -9999;
		else
			return propagation_get_received_power_dbm(propagationHandle, tx, txi, rx, rxi, pstruEventDetails->dEventTime);
	}
#define GET_RX_POWER_mw(tx,txi,rx,rxi) (DBM_TO_MW(GET_RX_POWER_dbm(tx,txi,rx,rxi)))

#define isDTDMAConfigured(d,i) (DEVICE_MACLAYER(d,i)->nMacProtocolId == MAC_PROTOCOL_DTDMA)

	//Function Prototype
	void update_receiver(NetSim_PACKET* packet);
	double fnGetDTDMAOverHead(NETSIM_ID nNodeId,NETSIM_ID nInterfaceId);
	double fn_NetSim_DTDMA_GetPhyOverhead(NetSim_PACKET* packet);
	double fn_NetSim_DTDMA_CalulateTxTime(NETSIM_ID d, NETSIM_ID in, NetSim_PACKET* packet);
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
	int fn_NetSim_CalculateReceivedPower(NETSIM_ID TX,NETSIM_ID RX);
	int fn_NetSim_FormDTDMABlock();
	PACKET_STATUS fn_NetSim_DTDMA_CalculatePacketError(NETSIM_ID d, NETSIM_ID in, NetSim_PACKET* packet);
	int fn_NetSim_DTDMA_CalulateReceivedPower();
	int fn_NetSim_DTDMA_CreateAllocationFile();
	int fn_NetSim_DTDMA_NodeInit();
	int fn_NetSim_DTDMA_PhysicalOut();
	int fn_NetSim_DTDMA_ReadAllocationFile();
	int fn_NetSim_DTDMA_RemoveCurrentInfo();
	int fn_NetSim_DTDMA_ScheduleDataTransmission();
	int fn_NetSim_DTDMA_TransmitPacket(NETSIM_ID d, NETSIM_ID in, NetSim_PACKET* packet);
	int fn_NetSim_DTDMA_SendToPhy();
	int dtdma_CalculateReceivedPower(NETSIM_ID tx, NETSIM_ID txi, NETSIM_ID rx, NETSIM_ID rxi);

	int fn_NetSim_DTDMA_NetInit(NETSIM_ID dev, NETSIM_ID in);
	int fnDTDMANodeJoinCallBack(NETSIM_ID nDeviceId,double time,NODE_ACTION action);
	PHY_MODULATION getModulationFromString(char* val);

	//Frequency Hopping
	int fn_NetSim_DTDMA_InitFrequencyHopping();
	int fn_NetSim_DTDMA_FrequencyHopping();
	int fn_NetSim_DTDMA_FinishFrequencyHopping();

	//Fragmentation
	NetSim_PACKET* get_packet_from_buffer(NETSIM_ID dev_id,NETSIM_ID if_id,int flag);
	NetSim_PACKET* get_packet_from_buffer_with_size(NETSIM_ID dev_id,NETSIM_ID if_id,double size);
	void reassemble_packet();

	//Slot
	void init_slot_formation();
	void fn_NetSim_DTDMA_FormSlot();

#ifdef  __cplusplus
}
#endif
#endif /* _NETSIM_DTDMA_H_ */



