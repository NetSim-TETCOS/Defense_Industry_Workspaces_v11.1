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
 * Author:    		   ,~~.															*
			 		  (  6 )-_,	
			 	 (\___ )=='-'
			 	  \ .   ) )
			 	   \ `-' /
			 	~'`~'`~'`~'`~     Shashi kant suman                                                 
 *																					*
 * ---------------------------------------------------------------------------------*/



#include "main.h"
#include "LTE.h"
#include "CA.h"

void fn_NetSim_LTE_A_ConfigCA(void** var)
{
	void* xmlNetSimNode=var[2];
	NETSIM_ID nDeviceId =*((NETSIM_ID*)var[3]);
	NETSIM_ID nInterfaceId = *((NETSIM_ID*)var[4]);
	unsigned int i;
	LTE_ENB_PHY* phy = (LTE_ENB_PHY*)DEVICE_PHYVAR(nDeviceId,nInterfaceId);
	LTE_ENB_MAC* mac = (LTE_ENB_MAC*)DEVICE_MACVAR(nDeviceId,nInterfaceId);
	void* captr;

	//Get the CA_COUNT
	getXmlVar(&phy->ca_count,CA_COUNT,xmlNetSimNode,1,_INT,LTE_A);
	mac->ca_count = phy->ca_count;
	//Get the CA_TYPE
	getXmlVar(&phy->ca_type,CA_TYPE,xmlNetSimNode,1,_STRING,LTE_A);
	//Get the CA_CONFIGURATION
	getXmlVar(&phy->ca_configuration,CA_CONFIGURATION,xmlNetSimNode,1,_STRING,LTE_A);

	for(i=0;i<phy->ca_count;i++)
	{
		captr = fn_NetSim_xmlGetChildElement(xmlNetSimNode,"CA",i);
		if(captr)
		{
			getXmlVar(&phy->ca_phy[i].dDLFrequency_max,DL_FREQUENCY_MAX,captr,1, _DOUBLE,LTE);
			getXmlVar(&phy->ca_phy[i].dDLFrequency_min,DL_FREQUENCY_MIN,captr,1, _DOUBLE,LTE);
			getXmlVar(&phy->ca_phy[i].dULFrequency_max,UL_FREQUENCY_MAX,captr,1, _DOUBLE,LTE);
			getXmlVar(&phy->ca_phy[i].dULFrequency_min,UL_FREQUENCY_MIN,captr,1, _DOUBLE,LTE);
			phy->ca_phy[i].dBaseFrequency = (phy->ca_phy[i].dDLFrequency_max+
				phy->ca_phy[i].dDLFrequency_min)/2.0;
			getXmlVar(&phy->ca_phy[i].dChannelBandwidth,CHANNEL_BANDWIDTH,captr,1, _DOUBLE,LTE);
			getXmlVar(&phy->ca_phy[i].nResourceBlockCount,RESOURCE_BLOCK_COUNT,captr,1,_INT,LTE);
			getXmlVar(&phy->ca_phy[i].dSamplingFrequency,SAMPLING_FREQUENCY,captr,1, _DOUBLE,LTE);
			getXmlVar(&phy->ca_phy[i].nFFTSize,FFT_SIZE,captr,1,_INT,LTE);
			getXmlVar(&phy->ca_phy[i].nOccupiedSubCarrier,OCCUPIED_SUBCARRIER,captr,1,_INT,LTE);
			getXmlVar(&phy->ca_phy[i].nGuardSubCarrier,GUARD_SUBCARRIER,captr,1,_INT,LTE);
		}
	}
}

void fn_NetSim_LTE_A_FormNextFrameForEachCarrier(NETSIM_ID enbId,NETSIM_ID enbInterface,unsigned int nSSId)
{
	unsigned int i;
	LTE_ENB_MAC* enbMac=get_enb_mac(enbId,enbInterface);
	LTE_ASSOCIATEUE_INFO* info=enbMac->associatedUEInfo;
	while (info)
	{
		LTE_UE_MAC* m=get_ue_mac(info->nUEId,info->nUEInterface);
		m->carrier_id=-1;
		info->carrier_id=-1;
		info=(LTE_ASSOCIATEUE_INFO*)LIST_NEXT(info);
	}
	for (i=0;i<enbMac->ca_count;i++)
	{
		enbMac->ca_mac[i].nAllocatedRBG=0;
		fn_NetSim_LTE_FormUPlinkFrame(enbId,enbInterface,i,nSSId);
		fn_NetSim_LTE_FormDownlinkFrame(enbId,enbInterface,i,nSSId);
	}
}

unsigned int get_carrier_index(NetSim_PACKET* packet)
{
	return ((LTE_PHY_PACKET*)PACKET_PHYPROTOCOLDATA(packet))->carrier_index;
}

void set_carrier_index(NetSim_PACKET* packet,unsigned int carrier_id)
{
	((LTE_PHY_PACKET*)PACKET_PHYPROTOCOLDATA(packet))->carrier_index = carrier_id;
}