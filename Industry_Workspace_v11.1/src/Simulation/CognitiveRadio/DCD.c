#include "main.h"
#include "802_22.h"
/** DCD (Downstream Channel Descriptor) message shall be transmitted by the BS at a 
periodic interval to define the characteristics of a downstream physical channel. */
NetSim_PACKET* fn_NetSim_CR_FormDCD(NETSIM_ID nDeviceId,NETSIM_ID nInterfaceId)
{
	BS_MAC* pstruBSMAC = (BS_MAC*)DEVICE_MACVAR(nDeviceId,nInterfaceId);
	BS_PHY* pstruBSPHY = (BS_PHY*)DEVICE_PHYVAR(nDeviceId,nInterfaceId);
	pstruBSMAC->pstruDCD = (DCD*)calloc(1,sizeof(DCD));
	pstruBSMAC->pstruDCD->nConfigurationChangeCount = 0;
	pstruBSMAC->pstruDCD->nDownStreamBurstProfileCount = 0;
	pstruBSMAC->pstruDCD->nManagementMessageType = MMM_DCD;
	pstruBSMAC->pstruDCD->pstruDCDChannelIE.EIRP = 0;
	pstruBSMAC->pstruDCD->pstruDCDChannelIE.nActionFrameNumber = pstruBSPHY->nFrameNumber;
	pstruBSMAC->pstruDCD->pstruDCDChannelIE.nActionMode = 0;
	pstruBSMAC->pstruDCD->pstruDCDChannelIE.nActionSuperframeNumber = pstruBSPHY->nSuperframeNumber;
	pstruBSMAC->pstruDCD->pstruDCDChannelIE.nBackupChannelCount = 0;
	pstruBSMAC->pstruDCD->pstruDCDChannelIE.nChannelAction = B3_000;
	pstruBSMAC->pstruDCD->pstruDCDChannelIE.nDownStreamBurstProfile = 0;
	pstruBSMAC->pstruDCD->pstruDCDChannelIE.nMACVersion = 0x01;
	pstruBSMAC->pstruDCD->pstruDCDChannelIE.RSS = 0;
	pstruBSMAC->pstruDCD->pstruDCDChannelIE.struBackupChannelList.nChannelCount = 0;
	return fn_NetSim_CR_GenerateBroadcastCtrlPacket(nDeviceId,nInterfaceId,MMM_DCD);
}
