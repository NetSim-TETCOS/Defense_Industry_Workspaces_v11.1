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
#include "main.h"
#include "LTE.h"
NETSIM_ID fn_NetSim_LTE_FindNearesteNB(NETSIM_ID nDeviceId);
int fn_NetSim_LTE_CalculateReceivedPower()
{
	NETSIM_ID i;
	for(i=0;i<NETWORK->nDeviceCount;i++)
	{
		if(NETWORK->ppstruDeviceList[i]->nDeviceType==eNB ||
			DEVICE_TYPE(i+1)==RELAY)
		{
			NETSIM_ID ifid=get_eNB_Interface(i+1);
			LTE_ENB_MAC* enbMac=(LTE_ENB_MAC*)DEVICE_MACVAR(i+1,ifid);
			LTE_ASSOCIATEUE_INFO* info=enbMac->associatedUEInfo;
			while(info)
			{
				unsigned int j;
				for(j=0;j<enbMac->ca_count;j++)
				{

					fn_NetSim_LTE_CalculateRxPower(i+1,ifid,info,j);
					fn_NetSim_LTE_CalculateSNR(i+1,ifid,info,j);
					fn_NetSim_LTE_GetCQIIndex(i+1,ifid,info,j);
					fn_NetSim_LTE_GetMCS_TBS_Index(info,j);
					while(info->DLInfo[j].nCQIIndex>1 && info->ULInfo[j].nCQIIndex>1)
					{
						double ber;
						fn_NetSim_LTE_GetMCS_TBS_Index(info,j);
						ber = fn_NetSim_LTE_CalculateBER(0,info->DLInfo[j].MCSIndex,info->DLInfo[j].dSNR);
						if(ber<TARGET_BER)
							break;
						else
						{
							info->DLInfo[j].nCQIIndex--;
							info->ULInfo[j].nCQIIndex--;
						}
					}
				}
				info=(LTE_ASSOCIATEUE_INFO*)LIST_NEXT(info);
			}
		}
	}
	return 1;
}
int fn_NetSim_LTE_CalculateRxPower(NETSIM_ID enbId,NETSIM_ID enbInterface,LTE_ASSOCIATEUE_INFO* info,unsigned int carrier_index)
{
	LTE_ENB_PHY* enbPhy=(LTE_ENB_PHY*)DEVICE_PHYVAR(enbId,enbInterface);
	double dTXPower_DL=enbPhy->dTXPower;
	NETSIM_ID nLinkID=DEVICE_PHYLAYER(enbId,enbInterface)->nLinkId;

	LTE_UE_PHY* uePhy=(LTE_UE_PHY*)DEVICE_PHYVAR(info->nUEId,info->nUEInterface);
	double dTXPower_UL=uePhy->dTXPower;

	double fpi=3.1417;	// TO GET THE PI VALUE
	double dGainTxr=0;	// TO GET THE TRANSMITTER GAIN
	double dGainRver=0;	// TO GET THE RECEIVER GAIN
	int nDefaultDistance=1;	// TO GET THE DEFULT DISTANCE
	double fA1,fWaveLength=0.0; // TO GET THE WAVELENGTH VALUE
	double fA1dB, fA2dB;
	double dDefaultExponent=2;
	double dRxPower_UL,dRxPower_DL;
	double dDistance=fn_NetSim_Utilities_CalculateDistance(DEVICE_POSITION(enbId),DEVICE_POSITION(info->nUEId));

	//TO CALCULATE LEMDA
	fWaveLength=(double)(300.0/(enbPhy->ca_phy[carrier_index].dBaseFrequency *1.0));//pathloss

	// TO CALCULATE (4*3.14*do)
	fA1=fWaveLength/(4*(double) fpi * nDefaultDistance);

	// TO CALCULATE 20log10[ lemda/(4*3.14*do) ]
	fA1dB =  10 * dDefaultExponent * log10(1.0/fA1);

	// TO CALCULATE 10 * n *log10 (d/do)
	fA2dB =  10 * NETWORK->ppstruNetSimLinks[nLinkID-1]->puniMedProp.pstruWirelessLink.propagation->pathlossVar.pathLossExponent * log10(dDistance/nDefaultDistance);

	//TO CALCULATE [Pt]  +  [Gt]  +  [Gr]  +  20log10[ Lemda/(4*3.14*do) ] + ( 10 * n *log10 (do/d) )
	dRxPower_DL = dTXPower_DL + dGainTxr + dGainRver - fA1dB - fA2dB;
	dRxPower_UL = dTXPower_UL + dGainTxr + dGainRver - fA1dB - fA2dB;
	info->DLInfo[carrier_index].dReceivedPower=dRxPower_DL+30;//in dbm
	info->ULInfo[carrier_index].dReceivedPower=dRxPower_UL+30;//in dbm

	return 0;
}
int fn_NetSim_LTE_CalculateSNR(NETSIM_ID enbId,NETSIM_ID enbInterface,LTE_ASSOCIATEUE_INFO* info,unsigned int carrier_index)
{
	LTE_ENB_PHY* enbPhy=(LTE_ENB_PHY*)DEVICE_PHYVAR(enbId,enbInterface);
	double dThermalNoisePower_1Hz = -174.0; //At room temp. In dBm
	double dThermalNoise; //in dBm

	//Calculate the thermal noise
	dThermalNoise = dThermalNoisePower_1Hz + 10 * log10((enbPhy->ca_phy[carrier_index].dChannelBandwidth)*1000000);
	//Calculate the SNR
	info->DLInfo[carrier_index].dSNR = info->DLInfo[carrier_index].dReceivedPower - dThermalNoise;
	info->ULInfo[carrier_index].dSNR = info->ULInfo[carrier_index].dReceivedPower - dThermalNoise;
	return 0;
}

int fn_NetSim_LTE_GetCQIIndex(NETSIM_ID enbId,NETSIM_ID enbInterface,LTE_ASSOCIATEUE_INFO* info,unsigned int carrier_index)
{
	unsigned int i;
	LTE_ENB_PHY* enbPhy=(LTE_ENB_PHY*)DEVICE_PHYVAR(enbId,enbInterface);
	LTE_UE_PHY* uePhy=(LTE_UE_PHY*)DEVICE_PHYVAR(info->nUEId,info->nUEInterface);
	double dSNR_UL=info->ULInfo[carrier_index].dSNR;
	double dSNR_DL=info->DLInfo[carrier_index].dSNR;
	fn_NetSim_LTE_GetTransmissionIndex(enbPhy,uePhy,info);
	//uplink
	info ->ULInfo[carrier_index].Rank= 1;
	for(i=0;i<CQI_SNR_TABLE_LEN;i++)  
	{
		if(dSNR_UL<=CQI_SNR_TABLE[info->TransmissionIndex.UL][i])
		{
			info->ULInfo[carrier_index].nCQIIndex=i;
			break;
		}
	}
	if(i==CQI_SNR_TABLE_LEN)
		info->ULInfo[carrier_index].nCQIIndex=CQI_SNR_TABLE_LEN;
	if(i==0)
		info->ULInfo[carrier_index].nCQIIndex=1;
	
	if (info->ULInfo[carrier_index].nCQIIndex>= 10)
	{
		info ->ULInfo[carrier_index].Rank= min(MODE_INDEX_MAPPING[info->TransmissionIndex.UL].Tx_Antenna_UL,MODE_INDEX_MAPPING[info->TransmissionIndex.UL].Rx_Antenna_UL);
	}
	//downlink
	info ->DLInfo[carrier_index].Rank= 1;
	for(i=0;i<CQI_SNR_TABLE_LEN;i++)  
	{
		if(dSNR_DL<=CQI_SNR_TABLE[info->TransmissionIndex.DL][i])
		{
			info->DLInfo[carrier_index].nCQIIndex=i;
			break;
		}
	}
	if(i==CQI_SNR_TABLE_LEN)
		info->DLInfo[carrier_index].nCQIIndex=CQI_SNR_TABLE_LEN;
	if(i==0)
		info->DLInfo[carrier_index].nCQIIndex=1;

	if (info->DLInfo[carrier_index].nCQIIndex>= 10)
	{
		info ->DLInfo[carrier_index].Rank= min(MODE_INDEX_MAPPING[info->TransmissionIndex.DL].Tx_Antenna_DL,MODE_INDEX_MAPPING[info->TransmissionIndex.DL].Rx_Antenna_DL);
	}
	return 0;
}
int fn_NetSim_LTE_GetMCS_TBS_Index(LTE_ASSOCIATEUE_INFO* info,unsigned int carrier_index)
{	
	//uplink
	info->ULInfo[carrier_index].MCSIndex=CQI_MCS_MAPPING[info->ULInfo[carrier_index].nCQIIndex-1].MCS_Index;
	info->ULInfo[carrier_index].modulation=CQI_MCS_MAPPING[info->ULInfo[carrier_index].nCQIIndex-1].modulation;
	info->ULInfo[carrier_index].TBSIndex=MCS_TBS_MAPPING[info->ULInfo[carrier_index].MCSIndex].TBS_Index;
	
	//downlink
	info->DLInfo[carrier_index].MCSIndex=CQI_MCS_MAPPING[info->DLInfo[carrier_index].nCQIIndex-1].MCS_Index;
	info->DLInfo[carrier_index].modulation=CQI_MCS_MAPPING[info->DLInfo[carrier_index].nCQIIndex-1].modulation;
	info->DLInfo[carrier_index].TBSIndex=MCS_TBS_MAPPING[info->DLInfo[carrier_index].MCSIndex].TBS_Index;
	
	return 0;
}

int fn_NetSim_LTE_Mobility(NETSIM_ID nDeviceId)
{
	unsigned int j;
	if(DEVICE_MACLAYER(nDeviceId,1)->nMacProtocolId == MAC_PROTOCOL_LTE)
	{
		LTE_UE_MAC* ueMac=(LTE_UE_MAC*)DEVICE_MACVAR(nDeviceId,1);
		NETSIM_ID enbId=ueMac->nENBId;
		NETSIM_ID enbInterface=ueMac->nENBInterface;
		LTE_ENB_MAC* enbMac=(LTE_ENB_MAC*)DEVICE_MACVAR(enbId,enbInterface);
		LTE_ASSOCIATEUE_INFO* info=fn_NetSim_LTE_FindInfo(enbMac,nDeviceId);
	
		fn_NetSim_LTE_InitHandover(nDeviceId,enbId);

		enbId=ueMac->nENBId;
		enbInterface=ueMac->nENBInterface;
		enbMac=(LTE_ENB_MAC*)DEVICE_MACVAR(enbId,enbInterface);
		info=fn_NetSim_LTE_FindInfo(enbMac,nDeviceId);

		for (j=0;j<enbMac->ca_count;j++)
		{
			fn_NetSim_LTE_CalculateRxPower(enbId,enbInterface,info,j);
			fn_NetSim_LTE_CalculateSNR(enbId,enbInterface,info,j);
			fn_NetSim_LTE_GetCQIIndex(enbId,enbInterface,info,j);
			fn_NetSim_LTE_GetMCS_TBS_Index(info,j);
			while(info->DLInfo[j].nCQIIndex>1 && info->ULInfo[j].nCQIIndex>1)
			{
				double ber;
				fn_NetSim_LTE_GetMCS_TBS_Index(info,j);
				ber = fn_NetSim_LTE_CalculateBER(0,info->DLInfo[j].MCSIndex,info->DLInfo[j].dSNR);
				if(ber<TARGET_BER)
					break;
				else
				{
					info->DLInfo[j].nCQIIndex--;
					info->ULInfo[j].nCQIIndex--;
				}
			}
		}
	}
	else
	{
		//Do nothing other mac protocol
	}
	return 1;
}
NETSIM_ID fn_NetSim_LTE_FindNearesteNB(NETSIM_ID nDeviceId)
{
	NETSIM_ID i;
	double distance=0xFFFFFFFF;
	NETSIM_ID enb=0;
	for(i=0;i<NETWORK->nDeviceCount;i++)
	{
		if(DEVICE_TYPE(i+1)==eNB)
		{
			double temp=fn_NetSim_Utilities_CalculateDistance(DEVICE_POSITION(nDeviceId),DEVICE_POSITION(i+1));
			if(temp<distance)
			{
				distance=temp;
				enb=i+1;
			}
		}
	}
	return enb;
}

double get_received_power_between_ue(NETSIM_ID ue1,NETSIM_ID ue2,LTE_ENB_PHY* phy)
{
	LTE_UE_PHY* phy1 = get_ue_phy(ue1,1);
	double power = phy1->dTXPower;
	NetSim_LINKS* link = DEVICE_PHYLAYER(ue1,1)->pstruNetSimLinks;

	double fpi=3.1417;	// TO GET THE PI VALUE
	double dGainTxr=0;	// TO GET THE TRANSMITTER GAIN
	double dGainRver=0;	// TO GET THE RECEIVER GAIN
	int nDefaultDistance=1;	// TO GET THE DEFULT DISTANCE
	double fA1,fWaveLength=0.0; // TO GET THE WAVELENGTH VALUE
	double fA1dB, fA2dB;
	double dDefaultExponent=2;
	double dRxPower;
	double dDistance=fn_NetSim_Utilities_CalculateDistance(DEVICE_POSITION(ue1),DEVICE_POSITION(ue2));

	//TO CALCULATE LEMDA
	fWaveLength=(double)(300.0/(phy->ca_phy[0].dDLFrequency_max*1.0));//path loss

	// TO CALCULATE (4*3.14*do)
	fA1=fWaveLength/(4*(double) fpi * nDefaultDistance);

	// TO CALCULATE 20log10[ lemda/(4*3.14*do) ]
	fA1dB =  10 * dDefaultExponent * log10(1.0/fA1);

	// TO CALCULATE 10 * n *log10 (d/do)
	fA2dB =  10 * link->puniMedProp.pstruWirelessLink.propagation->pathlossVar.pathLossExponent * log10(dDistance/nDefaultDistance);

	//TO CALCULATE [Pt]  +  [Gt]  +  [Gr]  +  20log10[ Lemda/(4*3.14*do) ] + ( 10 * n *log10 (do/d) )
	dRxPower = power + dGainTxr + dGainRver - fA1dB - fA2dB;
	return dRxPower+30;
}

double get_snr_between_ue(double power,LTE_ENB_PHY* phy)
{
	double dThermalNoisePower_1Hz = -174.0; //At room temp. In dBm
	double dThermalNoise; //in dBm

	//Calculate the thermal noise
	dThermalNoise = dThermalNoisePower_1Hz + 10 * log10((phy->ca_phy[0].dChannelBandwidth)*1000000);
	//Calculate the SNR
	return power - dThermalNoise;
}