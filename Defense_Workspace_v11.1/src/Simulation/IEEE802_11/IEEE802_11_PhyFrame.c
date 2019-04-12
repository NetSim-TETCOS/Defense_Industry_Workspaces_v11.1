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
#include "IEEE802_11_PhyFrame.h"

int get_ofdm_rate(int rate)
{
	switch(rate)
	{
	case 6: return 0xD;
	case 9: return 0xF;
	case 12: return 0x5;
	case 18: return 0x7;
	case 24: return 0x9;
	case 36: return 0xB;
	case 48: return 0x1;
	case 54: return 0x3;
	default: return 0xD;
	}
}

/**
This function is called in the fn_NetSim_WLAN_PhysicalOut() function.
Add the PHY header pstruPacket->pstruPhyData->Packet_PhyData to the packet. 
*/
void fn_NetSim_IEEE802_11_Add_Phy_Header(NetSim_PACKET* packet)
{
	/*Adding phy layer overhead does not affect result in netsim.*/
	PIEEE802_11_PHY_VAR phy = IEEE802_11_CURR_PHY;
	ptrIEEE802_11_PHY_HDR hdr = calloc(1, sizeof* hdr);
	hdr->transmissionId = ++phy->transmissionId;
	PACKET_PHYPROTOCOLDATA(packet) = hdr;
	return;

	
	//PIEEE802_11_DSSS_PLCP_FRAME dsssPlcp;
	//PIEEE802_11_OFDM_PLCP_FRAME ofdmPlcp;
	//PIEEE802_11_HT_PLCP_FRAME htPlcp;
	//
	//switch(phy->PhyProtocol)
	//{
	//case IEEE_802_11b:
	//	dsssPlcp = (PIEEE802_11_DSSS_PLCP_FRAME)calloc(1, sizeof *dsssPlcp);
	//	dsssPlcp->nSIGNAL = phy->PHY_TYPE.dsssPhy.dsssrate;
	//	packet->pstruPhyData->Packet_PhyData = dsssPlcp;
	//	break;
	//case IEEE_802_11a:
	//case IEEE_802_11g:
	//case IEEE_802_11p:
	//	ofdmPlcp = (PIEEE802_11_OFDM_PLCP_FRAME)calloc(1, sizeof *ofdmPlcp);
	//	ofdmPlcp->nRATE = get_ofdm_rate((int)phy->PHY_TYPE.ofdmPhy.dDataRate);
	//	packet->pstruPhyData->Packet_PhyData = ofdmPlcp;
	//	break;
	//case IEEE_802_11n:
	//case IEEE_802_11ac:
	//	htPlcp = (PIEEE802_11_HT_PLCP_FRAME)calloc(1,sizeof* htPlcp);		
	//	// Assign the relevant values of sig field
	//	htPlcp->nMCS = phy->MCS;
	//	if(phy->PHY_TYPE.ofdmPhy_11n.nCH_BANDWIDTH == HT_CBW40)
	//		htPlcp->bCBW_20_40 = 1;
	//	
	//	if(phy->PHY_TYPE.ofdmPhy_11n.nAGGREGATION == AGGREGATED)
	//		htPlcp->bAggregation = 1;		
	//	
	//	if(phy->PHY_TYPE.ofdmPhy_11n.nSOUNDING == NOT_SOUNDING)
	//		htPlcp->bNot_Sounding = 1;		
	//	
	//	if(phy->PHY_TYPE.ofdmPhy_11n.nSMOOTHING == SMOOTHING_REC)
	//		htPlcp->bSmoothing = 1;
	//	
	//	if(phy->nGuardInterval == 400)
	//		htPlcp->bShort_GI = 1;
	//	
	//	if(phy->PHY_TYPE.ofdmPhy_11n.nFEC_CODING == LDPC_CODING)
	//		htPlcp->bFEC_coding = 1;
	//	packet->pstruPhyData->Packet_PhyData  = htPlcp;		
	//	break;
	//default:
	//	fnNetSimError("Unknown phy protocol %d in %s.",phy->PhyProtocol,__FUNCTION__);
	//	break;
	//}
}

void free_ieee802_11_phy_header(NetSim_PACKET* packet)
{
	ptrIEEE802_11_PHY_HDR hdr = PACKET_PHYPROTOCOLDATA(packet);
	free(hdr);
	PACKET_PHYPROTOCOLDATA(packet) = NULL;
}

void copy_ieee802_11_phy_header(NetSim_PACKET* d, NetSim_PACKET* s)
{
	ptrIEEE802_11_PHY_HDR sh = PACKET_PHYPROTOCOLDATA(s);
	if (sh)
	{
		ptrIEEE802_11_PHY_HDR dh = calloc(1, sizeof* dh);
		memcpy(dh, sh, sizeof* dh);
		PACKET_PHYPROTOCOLDATA(d) = dh;
	}
}
