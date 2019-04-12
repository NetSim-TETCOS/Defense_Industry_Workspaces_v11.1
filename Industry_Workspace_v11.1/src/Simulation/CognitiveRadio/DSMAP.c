#include "main.h"
#include "802_22.h"
typedef struct stru_ExtentedDSMAPIE
{
	DSMAP_IE* pstruDSMAPIE;
	struct stru_ExtentedDSMAPIE* pstruNext;
}ExtendedDSIE;
/** The DS-MAP message defines the access to the downstream information. 
The length of the DS-MAP shall be an integer number of bytes. */
NetSim_PACKET* fn_NetSim_CR_FormDSMAP(NETSIM_ID nDeviceId, NETSIM_ID nInterfaceId)
{
	BS_PHY* pstruBSPhy;
	BS_MAC* pstruBSMAC;
	unsigned int nDSIECount=0;
	NetSim_PACKET* pstruPacket;
	unsigned int nPacketSize;
	unsigned int nSID;
	int nDSBurst;
	unsigned int SlotRequire;
	ExtendedDSIE *pstruDSMAPIE = NULL;
	ExtendedDSIE *pstruTempDSMAPIE = NULL;
	ExtendedDSIE *pstruFirstDSMAPIE = NULL;
	pstruBSPhy = DEVICE_PHYVAR(nDeviceId,nInterfaceId);
	pstruBSMAC = DEVICE_MACVAR(nDeviceId,nInterfaceId);
	//Allocate memory for DS-MAP
	pstruBSMAC->pstruDSMAP = fnpAllocateMemory(1,sizeof(DSMAP));
	pstruBSMAC->pstruDSMAP->nManagementMessageType = MMM_DS_MAP%10;
	pstruBSMAC->pstruDSMAP->nPaddingBits = 0;

	//Loop through access buffer
	while(pstruBSMAC->pstruDSPacketList)
	{
		pstruPacket = pstruBSMAC->pstruDSPacketList;
		UINT destCount;
		NETSIM_ID* dest = get_dest_from_packet(pstruPacket, &destCount);
		UINT i;
		nSID = 0;
		for (i = 0; i < destCount; i++)
		{
			nSID = pstruBSMAC->anSIDFromDevId[dest[i]];
			if (nSID)
				break;
		}
		if (!nSID)
		{
			if (!isBroadcastPacket(pstruPacket))
			{
				//The packet is not for this BS
				pstruBSMAC->pstruDSPacketList = pstruBSMAC->pstruDSPacketList->pstruNextPacket;
				pstruPacket->pstruNextPacket = NULL;
				fn_NetSim_Packet_FreePacket(pstruPacket);
				continue;
			}
		}
		//Set the receiver id
		pstruPacket->nReceiverId = destCount > 1 ? 0 : dest[0];
		pstruPacket->nTransmitterId = nDeviceId;
	
		nPacketSize = (unsigned int)fnGetPacketSize(pstruPacket);

		if (pstruBSPhy->nFrameNumber == 0)// for considering SCH & preamble
			nDSBurst = fn_NetSim_CR_FillDSFrame(nPacketSize, pstruBSMAC->pstruDSBurst, pstruBSPhy->pstruSymbolParameter, 4, &SlotRequire);
		else
			nDSBurst = fn_NetSim_CR_FillDSFrame(nPacketSize, pstruBSMAC->pstruDSBurst, pstruBSPhy->pstruSymbolParameter, 2, &SlotRequire);

		if (nDSBurst)
		{
			//Allocated
			pstruBSMAC->pstruDSPacketList = pstruBSMAC->pstruDSPacketList->pstruNextPacket;
			pstruPacket->pstruNextPacket = NULL;
			fn_NetSim_CR_AddPacketToDSBurst(pstruBSMAC->pstruDSBurst[nDSBurst],pstruPacket);
			//Prepare the DS-MAP IE
			pstruTempDSMAPIE = calloc(1,sizeof *pstruTempDSMAPIE);
			pstruTempDSMAPIE->pstruDSMAPIE = calloc(1,sizeof *pstruTempDSMAPIE->pstruDSMAPIE);
			pstruTempDSMAPIE->pstruDSMAPIE->SID = nSID;
			pstruTempDSMAPIE->pstruDSMAPIE->length = SlotRequire;
			nDSIECount++;
			if(!pstruFirstDSMAPIE)
			{
				pstruFirstDSMAPIE = pstruTempDSMAPIE;
				pstruDSMAPIE = pstruFirstDSMAPIE;
			}
			else
			{
				pstruDSMAPIE->pstruNext = pstruTempDSMAPIE;
				pstruDSMAPIE = pstruTempDSMAPIE;
			}
		}
		else
			break; // No space available in DS-Frame
	}
	pstruBSMAC->pstruDSMAP->nIECount = nDSIECount;
	if(nDSIECount)
		pstruBSMAC->pstruDSMAP->pstruIE = calloc(nDSIECount,sizeof(DSMAP_IE*));
	nDSIECount = 0;
	while(pstruFirstDSMAPIE)
	{
		pstruBSMAC->pstruDSMAP->pstruIE[nDSIECount++] = pstruFirstDSMAPIE->pstruDSMAPIE;
		pstruTempDSMAPIE = pstruFirstDSMAPIE;
		pstruFirstDSMAPIE = pstruFirstDSMAPIE->pstruNext;
		free(pstruTempDSMAPIE);
	}
	return fn_NetSim_CR_GenerateBroadcastCtrlPacket(nDeviceId,nInterfaceId,MMM_DS_MAP);
}
int fn_NetSim_CR_CPE_ProcessDSMAP()
{
	return 1;
}
