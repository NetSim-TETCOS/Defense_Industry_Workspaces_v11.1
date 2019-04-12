#include "main.h"
#include "802_22.h"
/** The frame control header is transmitted as part of the downstream PDU in the DS subframe. 
The length of the FCH shall be 3 bytes.The FCH shall be sent in the first subchannel of the 
symbol immediately following the frame preamble symbol except when it I s the first frame 
of a superframe belonging to a specific BS where this symbol will follow the SCH. 

This second symbol of the frame carrying the FCH shall use a cyclic prefix TCP=1/4 TFFT.
The FCH shall be encoded using the binary convolutional channel coding .The FCH shall be
transmitted using the PHY mode 5. */
int fn_NetSim_CR_TransmitFCH()

{
	double dTime = pstruEventDetails->dEventTime;
	BS_MAC* pstruBSMAC;
	BS_PHY* pstruBSPhy;
	pstruBSMAC = (BS_MAC*)NETWORK->ppstruDeviceList[pstruEventDetails->nDeviceId-1]->ppstruInterfaceList[pstruEventDetails->nInterfaceId-1]->pstruMACLayer->MacVar;
	pstruBSPhy = (BS_PHY*)NETWORK->ppstruDeviceList[pstruEventDetails->nDeviceId-1]->ppstruInterfaceList[pstruEventDetails->nInterfaceId-1]->pstruPhysicalLayer->phyVar;

	if(pstruBSPhy->pstruOpratingChannel == NULL)
	{
		return 0;//No activity
	}
	pstruBSMAC->nDSBurst = 1;
	if(pstruBSPhy->nFrameNumber==0)
		pstruBSMAC->nDSBurst = 3;

	//Call function to form DS-MAP
	fn_NetSim_CR_FormDSFrame(pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId,dTime);
	
	if(pstruBSPhy->nFrameNumber)
	{
		pstruBSMAC->dFrameStartTime = pstruEventDetails->dEventTime;
	}
	//Increment the frame number
	pstruBSPhy->nFrameNumber++;
	//Update the DSFrametime
	pstruBSMAC->dDSFrameTime = pstruBSMAC->dFrameStartTime + pstruBSPhy->pstruSymbolParameter->nDownLinkSymbol*pstruBSPhy->pstruSymbolParameter->dSymbolDuration;
	
	//Add timer event to tranmit the packet
	pstruEventDetails->dPacketSize = 0;
	pstruEventDetails->nEventType = TIMER_EVENT;
	pstruEventDetails->nPacketId = 0;
	pstruEventDetails->nSubEventType = TRANSMIT_DS_BURST;
	pstruEventDetails->pPacket = NULL;
	fnpAddEvent(pstruEventDetails);
	//Add timer event to transmit form next frame
	if(pstruBSPhy->nFrameNumber != 16)
	{
		pstruEventDetails->dEventTime = pstruBSMAC->dSuperframeStartTime + pstruBSPhy->nFrameNumber*10000+pstruBSPhy->pstruSymbolParameter->dSymbolDuration;// symbol duration added for frame preamble
		pstruEventDetails->dPacketSize = 0;
		pstruEventDetails->nSubEventType = FORM_DS_BURST;
		fnpAddEvent(pstruEventDetails);
	}
	//Add FCH to CR metrics
	pstruBSMAC->struBSMetrics.nFCHSent++;
	return 1;
}
/** The FCH shall be encoded using the binary convolutional channel coding .The FCH shall be
transmitted using the PHY mode 5. The 15-bit randomizer is initialized using the 15 LSBs 
of the BS ID. The BS ID is transmitted as part of the SCH and is thus available to the CPEs 
for decoding. 
The 24 FCH bits are encoded and mapped onto 24 data subcarriers. In order to
increase the robustness of the FCH, as signaled in the SCH, the encoded and mapped FCH data
may be transmitted using the PHY mode 4. The FCH then occupies the first two OFDM slots. */
int fn_NetSim_CR_CPE_ProcessFCH(NetSim_PACKET* pstruPacket)
{
	CPE_MAC* pstruCPEMAC = DEVICE_MACVAR(pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId);
	CPE_PHY* pstruCPEPhy = DEVICE_PHYVAR(pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId);
	FCH* pstruFCH = PACKET_MACPROTOCOLDATA(pstruPacket);
	SYMBOL_PARAMETER* pstruSymbol = pstruCPEPhy->pstruSymbol;
	BS_MAC* pstruBSMAC = DEVICE_MACVAR(pstruCPEMAC->nBSID,pstruCPEMAC->nBSInterface);
	BS_PHY* pstruBSPHY = DEVICE_PHYVAR(pstruCPEMAC->nBSID,pstruCPEMAC->nBSInterface);
	pstruCPEPhy->nFrameNumber++;
	if(pstruCPEPhy->nIntraFrameQuietPeriodLength)
	{
		if(pstruCPEPhy->szIntraFrameQuietPeriodBitmap[pstruCPEPhy->nFrameNumber-1] == '1')
		{
			//Quiet period is scheduled
			//Add an event to sense the channel
			pstruEventDetails->dEventTime =pstruBSMAC->dFrameStartTime + (pstruFCH->n_Length_of_the_frame - pstruCPEPhy->nIntraFrameQuietPeriodDuration)*pstruSymbol->dSymbolDuration+pstruBSPHY->dTTG;
			pstruEventDetails->nEventType = TIMER_EVENT;
			pstruEventDetails->nSubEventType = QUIET_PERIOD;
			pstruEventDetails->pPacket = NULL;
			fnpAddEvent(pstruEventDetails);
		}
	}
	//Add FCH to CR metrics
	pstruCPEMAC->struCPEMetrics.nFCHReceived++;
	return 1;
}



