/************************************************************************************
 * Copyright (C) 2013                                                               *
 * TETCOS, Bangalore. India                                                         *
 *                                                                                  *
 * Tetcos owns the intellectual property rights in the Product and its content.     *
 * The copying, redistribution, reselling or publication of any or all of the       *
 * Product or its content without express prior written consent of Tetcos is        *
 * prohibited. Ownership and / or any other right relating to the software and all *
 * intellectual property rights therein shall remain at all times with Tetcos.      *
 *                                                                                  *
 * Author:    Shashi Kant Suman                                                       *
 *                                                                                  *
 * ---------------------------------------------------------------------------------*/
#ifndef _NETSIM_CR_DSX_H_
#define _NETSIM_CR_DSX_H_
#define DOWNSTREAM 0x00
#define UPSTREAM 0x01
#define DSX_IE_FIXED_SIZE 117
unsigned int g_nTransactionId;

typedef enum service_state SERVICE_STATE;
enum service_state
{
	ServiceState_Null=0,
	ServiceState_Adding_remote,
	serviceState_Add_Failed,
	ServiceState_Adding_Local,
	ServiceState_Nominal,
	ServiceState_Changing_Local,
	ServiceState_Changing_Remote,
	ServiceState_Deleting,
	ServiceState_Deleted,
};
struct stru_802_22_Service
{
	unsigned int SFID[8];
	SERVICE_STATE Status[8];
	NetSim_PACKET* pTemp[8];
};
/** Table 72 IEEE 802.22-2011 Page 70
	Service flow encodings information elements
*/
struct stru_DSx_IE
{
	unsigned int nSFDirection:8;
	unsigned int nSFID;
	char className[10];
	unsigned int QOSParameterSetType:8;
	unsigned int maxSustainedTrafficrate:24;
	unsigned int maxTrafficBurst:24;
	unsigned int minReservedTrafficRate:24;
	unsigned int minTolerableTrafficRate:24;
	unsigned int SFSchedulingType:8;
	unsigned int Req_TrxPolicy:8;
	unsigned int toleratedJitter:24;
	unsigned int maxLatency:24;
	unsigned int sduIndicator:8;
	unsigned int sduSize:1;
	unsigned int targetSAID:16;
	unsigned int maxTolerablePacketLossRate:8;
	void* arqIE;
	unsigned int csSpec:8;
	unsigned int DSCAction:8;
	void* packetClassficationRule;
	void* reserved;
};
/** Table 64 IEEE 802.22-2011 Page 67
	DSA-REQ message format
*/
struct stru_DSA_REQ
{
	MANAGEMENT_MESSAGE type;
	unsigned int nTransactionId:16;
	DSX_IE pstruIE;
};
/** Table 65 IEEE 802.22-2011 page 67
	DSA-REP message format
*/
struct stru_DSA_RSP
{
	MANAGEMENT_MESSAGE type;
	unsigned int nTransactionID:16;
	unsigned int nConfirmationCode:8;
	DSX_IE pstruIE;
};
/** Table 70 IEEE 802.22-2011 page 69
	DSD-REQ message format
*/
struct stru_DSD_REQ
{
	MANAGEMENT_MESSAGE type;
	unsigned int nTransactionID:16;
	unsigned int nSFID:32;
	DSX_IE pstruIE;
};
/** Table 71 IEEE 802.22-2011 page 69
	DSD-RSP message format
*/
struct stru_DSD_RSP
{
	MANAGEMENT_MESSAGE type;
	unsigned int nTransactionID:16;
	unsigned int nSFID:32;
	DSX_IE pstruIE;
	unsigned int nConfirmationCode:8;
};
#endif
