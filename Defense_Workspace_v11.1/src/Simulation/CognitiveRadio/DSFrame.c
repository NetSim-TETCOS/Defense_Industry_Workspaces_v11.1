#include "main.h"
#include "802_22.h"
/** The DS-MAP message defines the access to the downstream information. 
The length of the DS-MAP shall be an integer number of bytes. This function is used to
generate the FCH frame,DS-MAP packet,US-MAP packet,DCD,UCD etc.*/
int fn_NetSim_CR_FormDSFrame(NETSIM_ID nDeviceId, NETSIM_ID nInterfaceId,double dTime)
{
	NetSim_PACKET* pstruPacket;
	unsigned int nSize;
	BS_PHY* pstruBSPhy;
	BS_MAC* pstruBSMAC;
	int nBurstId;
	int flag;
	unsigned int nSlotRequire;
	SYMBOL_PARAMETER* pstruSymbol;
	pstruBSPhy = DEVICE_PHYVAR(nDeviceId,nInterfaceId);
	pstruBSMAC = DEVICE_MACVAR(nDeviceId,nInterfaceId);
	pstruSymbol = pstruBSPhy->pstruSymbolParameter;
	
	flag =1;
	if(pstruBSPhy->nFrameNumber==0)
		flag=3;

	//Fill the first symbol by FCH frame
	//Generate the FCH frame
	pstruPacket = fn_NetSim_CR_GenerateBroadcastCtrlPacket(nDeviceId,nInterfaceId,MMM_FCH);
	pstruPacket->dEventTime = dTime;
	pstruPacket->pstruMacData->dArrivalTime = dTime;
	//Add FCH to BS MAC
	nBurstId = fn_NetSim_CR_FillDSFrame(FCH_SIZE,pstruBSMAC->pstruDSBurst,pstruSymbol,flag,&nSlotRequire);
	fn_NetSim_CR_AddPacketToDSBurst(pstruBSMAC->pstruDSBurst[nBurstId],pstruPacket);
	//Generate DS-MAP packet
	pstruPacket = fn_NetSim_CR_FormDSMAP(nDeviceId,nInterfaceId);
	pstruPacket->dEventTime = dTime;
	pstruPacket->pstruMacData->dArrivalTime = dTime;
	nSize = (unsigned int)fnGetPacketSize(pstruPacket);
	nBurstId = fn_NetSim_CR_FillDSFrame(nSize,pstruBSMAC->pstruDSBurst,pstruSymbol,flag,&nSlotRequire);
	fn_NetSim_CR_AddPacketToDSBurst(pstruBSMAC->pstruDSBurst[nBurstId],pstruPacket);
	//Generate US-MAP packet
	pstruPacket = fn_NetSim_CR_FormUSMAP(nDeviceId,nInterfaceId);
	pstruPacket->dEventTime = dTime;
	pstruPacket->pstruMacData->dArrivalTime = dTime;
	nSize = (unsigned int)fnGetPacketSize(pstruPacket);
	nBurstId = fn_NetSim_CR_FillDSFrame(nSize,pstruBSMAC->pstruDSBurst,pstruSymbol,flag,&nSlotRequire);
	fn_NetSim_CR_AddPacketToDSBurst(pstruBSMAC->pstruDSBurst[nBurstId],pstruPacket);
	//Generate UCD packet
	pstruPacket = fn_NetSim_CR_FormUCD(nDeviceId,nInterfaceId);
	pstruPacket->dEventTime = dTime;
	pstruPacket->pstruMacData->dArrivalTime = dTime;
	nSize = (unsigned int)fnGetPacketSize(pstruPacket);
	nBurstId = fn_NetSim_CR_FillDSFrame(nSize,pstruBSMAC->pstruDSBurst,pstruSymbol,flag,&nSlotRequire);
	fn_NetSim_CR_AddPacketToDSBurst(pstruBSMAC->pstruDSBurst[nBurstId],pstruPacket);
	//Generate DCD Packet
	pstruPacket = fn_NetSim_CR_FormDCD(nDeviceId,nInterfaceId);
	pstruPacket->dEventTime = dTime;
	pstruPacket->pstruMacData->dArrivalTime = dTime;
	nSize = (unsigned int)fnGetPacketSize(pstruPacket);
	nBurstId = fn_NetSim_CR_FillDSFrame(nSize,pstruBSMAC->pstruDSBurst,pstruSymbol,flag,&nSlotRequire);
	fn_NetSim_CR_AddPacketToDSBurst(pstruBSMAC->pstruDSBurst[nBurstId],pstruPacket);
	//Add other broadcast mac pdu
	pstruBSMAC->chsFrameCount--;
	if(pstruBSMAC->chsFrameCount <=0)
	{
		pstruBSMAC->nCHSREQFlag = ZERO;
		pstruBSMAC->chsFrameCount = 0;
	}
	while(pstruBSMAC->pstruBroadcastPDU)
	{
		pstruPacket = pstruBSMAC->pstruBroadcastPDU;
		pstruBSMAC->pstruBroadcastPDU = pstruBSMAC->pstruBroadcastPDU->pstruNextPacket;
		pstruPacket->pstruNextPacket = NULL;
		nSize = (unsigned int)fnGetPacketSize(pstruPacket);
		nBurstId = fn_NetSim_CR_FillDSFrame(nSize,pstruBSMAC->pstruDSBurst,pstruSymbol,flag,&nSlotRequire);
		if(nBurstId)
			fn_NetSim_CR_AddPacketToDSBurst(pstruBSMAC->pstruDSBurst[nBurstId],pstruPacket);
		else
			printf("No slot");
	}
	return 1;
}
/** This function is used to  transmit the appropriate DSBurst. */
int fn_NetSim_CR_TransmitDSBurst()
{
	BS_MAC* pstruBSMac = DEVICE_MACVAR(pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId);
	BS_PHY* pstruBSPhy = DEVICE_PHYVAR(pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId);
	unsigned int nBurstId = pstruBSMac->nDSBurst;
	NetSim_PACKET* pstruMACPdu;
	SYMBOL_PARAMETER* pstruSymbol = pstruBSPhy->pstruSymbolParameter;
	if(pstruBSPhy->pstruOpratingChannel == NULL || pstruBSMac->dDSFrameTime <= pstruEventDetails->dEventTime)
	{
		return -1; //UCS notification or DSFrame over
	}
	//check for a packet
	if(nBurstId <pstruSymbol->nDownLinkSymbol && pstruBSMac->pstruDSBurst[nBurstId]->pstruMACPDU)
	{
TRANSMITDSBURST:
		pstruMACPdu = fn_NetSim_CR_UNFillSlot(pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId,nBurstId);
		if(pstruMACPdu->nControlDataType/100 == MAC_PROTOCOL_IEEE802_22)
		{
			switch(pstruMACPdu->nControlDataType%100)
			{
			case MMM_DS_MAP:
				pstruBSMac->pstruDSMAP = NULL;
				break;
			case MMM_US_MAP:
				pstruBSMac->pstruUSMAP = NULL;
				break;
			case MMM_UCD:
				pstruBSMac->pstruUCD = NULL;
				break;
			case MMM_DCD:
				pstruBSMac->pstruDCD = NULL;
				break;
			}
		}
		
		//Prepare physical out event
		pstruEventDetails->dPacketSize =  fnGetPacketSize(pstruMACPdu);
		if(pstruMACPdu->pstruAppData)
			pstruEventDetails->nApplicationId = pstruMACPdu->pstruAppData->nApplicationId;
		else
			pstruEventDetails->nApplicationId = 0;
		pstruEventDetails->nEventType = PHYSICAL_OUT_EVENT;
		pstruEventDetails->nPacketId = pstruMACPdu->nPacketId;
		pstruEventDetails->nProtocolId = MAC_PROTOCOL_IEEE802_22;
		pstruEventDetails->nSubEventType = 0;
		pstruEventDetails->pPacket = pstruMACPdu;
		fnpAddEvent(pstruEventDetails);
	}
	else
	{
		pstruBSMac->nDSBurst++;
		nBurstId++;
		if(nBurstId <pstruSymbol->nDownLinkSymbol && pstruBSMac->pstruDSBurst[nBurstId]->pstruMACPDU)
			goto TRANSMITDSBURST;
	}
	return 1;
}

