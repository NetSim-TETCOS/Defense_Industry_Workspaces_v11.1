#include "main.h"
#include "802_22.h"
/** This message shall be transmitted by the BS at a periodic interval to define 
the characteristics of an upstream physical channel. */
NetSim_PACKET* fn_NetSim_CR_FormUCD(NETSIM_ID nDeviceId,NETSIM_ID nInterfaceId)
{
	NetSim_PACKET* pstruPacket;
	BS_MAC* pstruBSMAC = (BS_MAC*)DEVICE_MACVAR(nDeviceId,nInterfaceId);
	BS_PHY* pstruBSPHY = (BS_PHY*)DEVICE_PHYVAR(nDeviceId,nInterfaceId);
	pstruBSMAC->pstruUCD = (UCD*)calloc(1,sizeof(UCD));
	pstruBSMAC->pstruUCD->nBWRequestBackoffEnd = pstruBSPHY->nBWRequestBackoffEnd;
	pstruBSMAC->pstruUCD->nBWRequestBackoffStart = pstruBSPHY->nBWRequestBackoffStart;
	pstruBSMAC->pstruUCD->nConfigurationChangeCount = 0;
	pstruBSMAC->pstruUCD->nManagementMessageType = MMM_UCD%10;
	pstruBSMAC->pstruUCD->nUCSNotificationBackoffEnd = 0; //NO UCS
	pstruBSMAC->pstruUCD->nUCSNotificationBackoffStart = 0; //NO UCS
	pstruBSMAC->pstruUCD->nUpstreamBurstProfileCount = 0;

	//Create a UCD Packet
	pstruPacket = fn_NetSim_CR_GenerateBroadcastCtrlPacket(nDeviceId,nInterfaceId,MMM_UCD);
	return pstruPacket;
}
