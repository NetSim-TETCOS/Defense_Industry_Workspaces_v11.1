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
#include "LTE.h"

void fn_NetSim_LTE_Init_MIMO(NETSIM_ID enbId,NETSIM_ID enbInterface)
{
	LTE_ENB_PHY* phy=get_enb_phy(enbId,enbInterface);
	if (!phy)
		fnNetSimError("LTE phy is not present for ENB %d, interface %d\n",
					  DEVICE_CONFIGID(enbId),
					  DEVICE_INTERFACE_CONFIGID(enbId, enbInterface));

	if(phy->nTransmissionMode == MU_MIMO && phy->nTXAntennaCount > 4)
		phy->nSS = phy->nTXAntennaCount/4;
	else if(phy->nTransmissionMode == MU_MIMO && phy->nTXAntennaCount > 2)
		phy->nSS = phy->nTXAntennaCount/2;
	else
		phy->nSS = 1;
}
int fn_NetSim_LTE_GetTransmissionIndex(LTE_ENB_PHY* enbPhy,LTE_UE_PHY* uePhy,LTE_ASSOCIATEUE_INFO* info)
{
	//Uplink
	info->TransmissionIndex.UL = 0;

	//Downlink
	if (enbPhy->nTransmissionMode == Single_Antenna && enbPhy->nTXAntennaCount == 1)
	{
		info->TransmissionIndex.DL = 0;
	}
	else if (enbPhy->nTransmissionMode == Transmit_Diversity && enbPhy->nTXAntennaCount == 2)
	{
		info->TransmissionIndex.DL = 1;
	}
	else if (enbPhy->nTransmissionMode == SingleUser_MIMO_Spatial_Multiplexing && enbPhy->nTXAntennaCount == 2)
	{
		info->TransmissionIndex.DL = 2;
	}
	else if(enbPhy->nTransmissionMode == SingleUser_MIMO_Spatial_Multiplexing && enbPhy->nTXAntennaCount == 4)
	{
		info->TransmissionIndex.DL = 3;
	}
	else if(enbPhy->nTransmissionMode == MU_MIMO && enbPhy->nTXAntennaCount > 4)
	{
		info->TransmissionIndex.DL = 3;
	}
	else if(enbPhy->nTransmissionMode == MU_MIMO && enbPhy->nTXAntennaCount > 2)
	{
		info->TransmissionIndex.DL = 2;
	}
	else
	{
		fnNetSimError("Unknown transmission mode, Tx antenna count, Rx antenna count\nValid combinations are...\nFor eNB.....\n"
			"TRANSMISSION MODE INDEX \t TRANSMISSION MODE \t TX_ANTENNA \t RX_ANTENNA\n"
			"0 = 1 1 1\n"
			"1 = 2 2 2\n" 
			"2 = 3 2 2\n"
			"3 = 3 4 2\n"
			"4 = 5 * 2\n"
			"For UE....\n"
			"0 = 1 1 1\n"
			"1 = 2 1 2\n" 
			"2 = 3 1 2\n"
			"3 = 3 1 2\n"
			"4 = 5 1 2\n"
			"1) Transmission Mode 1- Using of a single antenna at eNodeB\n"
			"2) Transmission Mode 2- Transmit Diversity (TxD)\n"
			"3) Transmission Mode 3- SU-MIMO Spatial Multiplexing: Open-Loop\n"
			"4) Transmission Mode 4- MU-MIMO\n"
			"Running without MIMO..\n");
		info->TransmissionIndex.DL = 1;
	}

	return 0;
}

unsigned int get_nSS_index(NetSim_PACKET* packet)
{
	return ((LTE_PHY_PACKET*)PACKET_PHYPROTOCOLDATA(packet))->nSSId;
}

void set_nSS_index(NetSim_PACKET* packet,unsigned int nSSId)
{
	((LTE_PHY_PACKET*)PACKET_PHYPROTOCOLDATA(packet))->nSSId = nSSId;
}

void fn_NetSim_LTE_A_AllocateSS(NETSIM_ID enbId,NETSIM_ID enbInterface)
{
	unsigned int i;
	LTE_ENB_MAC* enbMac=get_enb_mac(enbId,enbInterface);
	LTE_ENB_PHY* enbPhy=get_enb_phy(enbId,enbInterface);
	LTE_ASSOCIATEUE_INFO* info=enbMac->associatedUEInfo;
	unsigned int s=0;
	while (info)
	{
		LTE_UE_MAC* m=get_ue_mac(info->nUEId,info->nUEInterface);
		if(s==enbPhy->nSS)
			s=0;
		s++;
		m->nSSId=s;
		info->nSSId=s;
		info=(LTE_ASSOCIATEUE_INFO*)LIST_NEXT(info);
	}
	for(i=0;i<enbPhy->nSS;i++)
		fn_NetSim_LTE_A_FormNextFrameForEachCarrier(enbId,enbInterface,i+1);
}
