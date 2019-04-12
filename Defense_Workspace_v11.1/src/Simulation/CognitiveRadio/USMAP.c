#include "main.h"
#include "802_22.h"
typedef struct stru_ExtentedUSMAPIE
{
	USMAP_IE* pstruUSMAPIE;
	struct stru_ExtentedUSMAPIE* pstruNext;
}ExtendedUSIE;
/** The US-MAP message defines the access to the upstream channel using US-MAP IEs */
NetSim_PACKET* fn_NetSim_CR_FormUSMAP(NETSIM_ID nDeviceId, NETSIM_ID nInterfaceId)
{
	int nIECount = 0;
	int nLoop,nLoop1;
	USMAP_IE* pstruUSIE[MAX_SID];
	BS_MAC* pstruBSMAC = (BS_MAC*)DEVICE_MACVAR(nDeviceId,nInterfaceId);
	BS_PHY* pstruBSPHY = (BS_PHY*)DEVICE_PHYVAR(nDeviceId,nInterfaceId);
	UPLINKALLOCINFO* pstruInfo = pstruBSMAC->uplinkAllocInfo;
	memset(pstruUSIE,0,sizeof(USMAP_IE*)*MAX_SID);
	pstruBSMAC->pstruUSMAP = fnpAllocateMemory(1,sizeof(USMAP));
	pstruBSMAC->pstruUSMAP->nManagementMessageType = MMM_US_MAP;
	pstruBSMAC->pstruUSMAP->nAllocationStartTime = pstruBSPHY->pstruSymbolParameter->nUPlinkFrameStartSymbol;
	fn_NetSim_CR_UpdateInfo(pstruBSMAC,pstruBSPHY);
	//Prepare US-MAP IE for BW-request
	pstruUSIE[0] = fnpAllocateMemory(1,sizeof *pstruUSIE[0]);
	pstruUSIE[0]->nSID = 0;//Broadcast
	pstruUSIE[0]->nUIUC = 3;
	pstruUSIE[0]->UIUC_2_3.nSubChannelCount = 6;
	nIECount++;
	//Loop through allocation info
	while(pstruInfo)
	{
		if(pstruUSIE[pstruInfo->nSID] == NULL)
		{
			nIECount++;
			pstruUSIE[pstruInfo->nSID] = fnpAllocateMemory(1,sizeof *pstruUSIE[pstruInfo->nSID]);
		}
		pstruUSIE[pstruInfo->nSID]->nSID = pstruInfo->nSID;
		pstruUSIE[pstruInfo->nSID]->nUIUC = pstruBSMAC->nUIUC;
		pstruUSIE[pstruInfo->nSID]->UIUC.nDuration += pstruInfo->nSlotAllocated;
		pstruInfo = pstruInfo->next;
	}
	pstruBSMAC->pstruUSMAP->nIECount = nIECount;
	pstruBSMAC->pstruUSMAP->pstruUSMAPIE = fnpAllocateMemory(nIECount,sizeof(USMAP_IE*));
	nLoop1 = 0;
	for(nLoop=0;nLoop<MAX_SID && nIECount;nLoop++)
	{
		if(pstruUSIE[nLoop])
		{
			nIECount--;
			pstruBSMAC->pstruUSMAP->pstruUSMAPIE[nLoop1++] = pstruUSIE[nLoop];
		}
	}
	return fn_NetSim_CR_GenerateBroadcastCtrlPacket(nDeviceId,nInterfaceId,MMM_US_MAP);
}
/** The US-MAP message defines the access to the upstream channel using US-MAP IEs */
int fn_NetSim_CR_CPE_ProcessUSMAP()
{
	int nSlot=0;
	unsigned int nLoop;
	USMAP* pstruUSMAP;
	double dTime;
	NETSIM_ID nDeviceId = pstruEventDetails->nDeviceId;
	NETSIM_ID nInterfaceId = pstruEventDetails->nInterfaceId;
	CPE_MAC* pstruCPEMAC = DEVICE_MACVAR(pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId);
	CPE_PHY* pstruCPEPhy = DEVICE_PHYVAR(nDeviceId,nInterfaceId);
	SYMBOL_PARAMETER* pstruSymbol = pstruCPEPhy->pstruSymbol;
	BS_MAC* pstruBSMac = DEVICE_MACVAR(pstruCPEMAC->nBSID,pstruCPEMAC->nBSInterface);
	BS_PHY* pstruBSPhy = DEVICE_PHYVAR(pstruCPEMAC->nBSID,pstruCPEMAC->nBSInterface);
	pstruUSMAP = pstruEventDetails->pPacket->pstruMacData->Packet_MACProtocol;
	dTime = pstruUSMAP->nAllocationStartTime*pstruSymbol->dSymbolDuration+pstruBSPhy->dTTG;
	dTime += pstruBSMac->dFrameStartTime;
	nSlot = pstruSymbol->nOFDMSlots-pstruUSMAP->pstruUSMAPIE[0]->UIUC_2_3.nSubChannelCount;
	//Update the allocated byted
	for(nLoop=0;nLoop<pstruUSMAP->nIECount;nLoop++)
	{
		if(pstruUSMAP->pstruUSMAPIE[nLoop]->nSID == pstruCPEMAC->nSID)
		{
			if(pstruUSMAP->pstruUSMAPIE[nLoop]->nUIUC >= 13 && pstruUSMAP->pstruUSMAPIE[nLoop]->nUIUC <= 61)
			{
				pstruCPEMAC->BWRequestInfo->dBytesAllocated = pstruUSMAP->pstruUSMAPIE[nLoop]->UIUC.nDuration*pstruSymbol->nBitsCountInOneSlot/8.0;
				pstruCPEMAC->BWRequestInfo->dStartTime = dTime + nSlot*pstruSymbol->dSymbolDuration/pstruSymbol->nOFDMSlots;
			}
		}
		else
		{
			switch(pstruUSMAP->pstruUSMAPIE[nLoop]->nUIUC)
			{
			case 0:
			case 1:
				break;
			case 2:
			case 3:
				nSlot += pstruUSMAP->pstruUSMAPIE[nLoop]->UIUC_2_3.nSubChannelCount;
				break;
			case 4:
			case 5:
			case 6:
				nSlot += pstruUSMAP->pstruUSMAPIE[nLoop]->UIUC_4_6.nSubChannelCount;
				nSlot += pstruUSMAP->pstruUSMAPIE[nLoop]->UIUC_4_6.nSubSymbolsCount*pstruSymbol->nOFDMSlots;
				break;
			case 8:
				nSlot += pstruUSMAP->pstruUSMAPIE[nLoop]->UIUC_8.nSubChannelCount;
				break;
			case 7:
			case 9:
			case 62:
				break;
			default:
				nSlot += pstruUSMAP->pstruUSMAPIE[nLoop]->UIUC.nDuration;
				break;
			}
		}
	}
	//Create an event to form US-Burst
	pstruEventDetails->dEventTime = dTime;
	pstruEventDetails->dPacketSize = 0;
	pstruEventDetails->nApplicationId = 0;
	pstruEventDetails->nEventType = TIMER_EVENT;
	pstruEventDetails->nPacketId = 0;
	pstruEventDetails->nSubEventType = FORM_US_BURST;
	pstruEventDetails->pPacket = NULL;
	fnpAddEvent(pstruEventDetails);
	return 0;
}

