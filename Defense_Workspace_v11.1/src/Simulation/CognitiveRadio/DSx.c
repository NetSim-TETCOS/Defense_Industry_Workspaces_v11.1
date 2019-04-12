
#include "main.h"
#include "802_22.h"
#include "../Application/Application.h"
/** A service flow is a MAC transport service that provides unidirectional transport of 
packets either to upstream packets transmitted by the CPE or to downstream packets 
transmitted by the BS.
A service flow is characterized by a set of QoS parameters such as latency, jitter, and
throughput assurances. In order to standardize operation between the CPE and BS, these
attributes include details of how the CPE requests upstream bandwidth allocations and the
expected behavior of the BS upstream scheduler. */
int fn_NetSim_CR_CreateServiceFlow(NETSIM_ID nDeviceId,NETSIM_ID nInterfaceId,int nId,NetSim_PACKET* packet,double dTime)
{
	CPE_MAC* pstruCPEMAC = DEVICE_MACVAR(nDeviceId,nInterfaceId);
	if(pstruCPEMAC->pstruServiceParameter->Status[nId] == ServiceState_Null ||
		pstruCPEMAC->pstruServiceParameter->Status[nId] == ServiceState_Deleted)
	{
		//Create a DSA_REQ packet
		NetSim_PACKET* pstruPacket;
		DSA_REQ* pstruDSAREQ;
		int appid;
		int generationrate = 0;
		if(packet->pstruAppData)
			appid = packet->pstruAppData->nApplicationId;
		else
			appid = 0;
		if(appid)
		{
			generationrate = (int)ceil((((APP_INFO**)NETWORK->appInfo)[appid-1]->dGenerationRate*1000000));
		}
		pstruPacket = fn_NetSim_Packet_CreatePacket(MAC_LAYER);
		pstruPacket->nControlDataType = CR_CONTROL_PACKET(MMM_DSA_REQ);
		add_dest_to_packet(pstruPacket, pstruCPEMAC->nBSID);
		pstruPacket->nPacketPriority = Priority_High;
		pstruPacket->nPacketType = PacketType_Control;
		pstruPacket->nReceiverId = pstruCPEMAC->nBSID;
		pstruPacket->nSourceId = pstruEventDetails->nDeviceId;
		pstruPacket->nTransmitterId = pstruEventDetails->nDeviceId;
		pstruDSAREQ = fnpAllocateMemory(1,sizeof *pstruDSAREQ);
		pstruDSAREQ->nTransactionId = ++g_nTransactionId;
		pstruDSAREQ->type = MMM_DSA_REQ;
		pstruDSAREQ->pstruIE.nSFDirection = UPSTREAM;
		switch(packet->nQOS)
		{
		case QOS_BE:
			strcpy(pstruDSAREQ->pstruIE.className,"BE");
			break;
		case QOS_ertPS:
			strcpy(pstruDSAREQ->pstruIE.className,"ERTPS");
			break;
		case QOS_nrtPS:
			strcpy(pstruDSAREQ->pstruIE.className,"NRTPS");
			break;
		case QOS_rtPS:
			strcpy(pstruDSAREQ->pstruIE.className,"RTPS");
			break;
		case QOS_UGS:
			strcpy(pstruDSAREQ->pstruIE.className,"UGS");
			break;
		default:
			strcpy(pstruDSAREQ->pstruIE.className,"NULL");
			break;
		}
		pstruDSAREQ->pstruIE.maxSustainedTrafficrate = generationrate;
		pstruPacket->pstruMacData->Packet_MACProtocol = pstruDSAREQ;
		pstruPacket->pstruMacData->dArrivalTime = dTime;
		pstruPacket->pstruMacData->dOverhead = (double)(DSX_IE_FIXED_SIZE + 3 + strlen(pstruDSAREQ->pstruIE.className));
		pstruPacket->pstruMacData->dPacketSize = pstruPacket->pstruMacData->dOverhead;
		pstruPacket->pstruMacData->nMACProtocol = MAC_PROTOCOL_IEEE802_22;
		pstruPacket->pstruMacData->szSourceMac = DEVICE_HWADDRESS(nDeviceId,nInterfaceId);
		//Place the packet to head of the list
		pstruPacket->pstruNextPacket = pstruCPEMAC->pstruQueuedPacketList[0];
		pstruCPEMAC->pstruQueuedPacketList[0] = pstruPacket;
		//Add the T7 timeout event
		pstruEventDetails->dEventTime = dTime+pstruCPEMAC->T7;
		pstruEventDetails->dPacketSize = 0;
		pstruEventDetails->nApplicationId = 0;
		pstruEventDetails->nDeviceId = nDeviceId;
		pstruEventDetails->nDeviceType = NETWORK->ppstruDeviceList[nDeviceId-1]->nDeviceType;
		pstruEventDetails->nEventType = TIMER_EVENT;
		pstruEventDetails->nInterfaceId = nInterfaceId;
		pstruEventDetails->nPacketId = 0;
		pstruEventDetails->nProtocolId = MAC_PROTOCOL_IEEE802_22;
		pstruEventDetails->pPacket = NULL;
		pstruEventDetails->nSubEventType = DSA_RSP_TIMEOUT;
		fnpAddEvent(pstruEventDetails);

		//Add T14 time out event
		pstruEventDetails->dEventTime = dTime+pstruCPEMAC->T14;
		pstruEventDetails->nSubEventType = DSA_RVD_TIMEOUT;
		fnpAddEvent(pstruEventDetails);

		//Change the state
		pstruCPEMAC->pstruServiceParameter->Status[nId] = ServiceState_Adding_remote;
		//store the packet into buffer
		pstruCPEMAC->pstruServiceParameter->pTemp[nId] = fn_NetSim_Packet_CopyPacket(pstruPacket);
		//Add DSA-REQ to CR Metrics
		pstruCPEMAC->struCPEMetrics.nDSA_REQSent++;
	}
	else
		return 0; //Already requested/created/in progress
	return 0;
}
/** To manage the various traffic flows between CPEs and the BS, the MAC protocol shall have the
capability to dynamically manage the addition, deletion, and change of service flows.
The format of a Dynamic Service Addition Request (DSA-REQ) message is shown in Table.

This message is sent either by a CPE or BS and is to create a new service flow,and shall not 
contain parameters for more than one service flow.The FID field carried in the MAC header 
of the PDU where this message is transmitted shall be the primary management FID of the CPE. */
int fn_NetSim_CR_BS_ProcessDSAReq()
{
	NetSim_PACKET* pstruRSP;
	NETSIM_ID nDeviceId = pstruEventDetails->nDeviceId;
	NETSIM_ID nInterfaceId = pstruEventDetails->nInterfaceId;
	BS_MAC* pstruBSMAC = DEVICE_MACVAR(nDeviceId,nInterfaceId);
	NetSim_PACKET* pstruPacket = pstruEventDetails->pPacket;
	DSA_REQ* pstruDSA = PACKET_MACPROTOCOLDATA(pstruPacket);
	SYMBOL_PARAMETER* pstruSymbol = ((BS_PHY*)DEVICE_PHYVAR(nDeviceId,nInterfaceId))->pstruSymbolParameter;
	unsigned int nSlot = (int)ceil(pstruDSA->pstruIE.maxSustainedTrafficrate/(100.0*pstruSymbol->nBitsCountInOneSlot));
	NETSIM_ID nSID = (NETSIM_ID)(pstruBSMAC->anSIDFromDevId[pstruPacket->nTransmitterId]);

	//Add DSA-REQ 
	pstruBSMAC->struBSMetrics.nDSA_REQReceived++;
	if(!fn_NetSim_CR_AllocBandwidth(nSID,fnGetQOS(pstruDSA->pstruIE.className),&(pstruBSMAC->uplinkAllocInfo),nSlot,pstruSymbol->nUPlinkSymbol*pstruSymbol->nOFDMSlots))
		return 0;
	//Create DSC-RSP packet
	pstruRSP = fn_NetSim_Packet_CreatePacket(MAC_LAYER);
	pstruRSP->nControlDataType = CR_CONTROL_PACKET(MMM_DSA_RSP);
	add_dest_to_packet(pstruRSP, pstruPacket->nTransmitterId);
	pstruRSP->nPacketPriority = Priority_High;
	pstruRSP->nPacketType = PacketType_Control;
	pstruRSP->nReceiverId = pstruPacket->nTransmitterId;
	pstruRSP->nSourceId = nDeviceId;
	pstruRSP->nTransmitterId = nDeviceId;
	pstruRSP->pstruMacData->dArrivalTime = pstruEventDetails->dEventTime;
	pstruRSP->pstruMacData->nMACProtocol = MAC_PROTOCOL_IEEE802_22;
	{
		DSA_RSP* pstruDSARSP = fnpAllocateMemory(1,sizeof *pstruDSARSP);
		pstruDSARSP->nTransactionID = pstruDSA->nTransactionId;
		pstruDSARSP->type = MMM_DSA_RSP;
		pstruDSARSP->pstruIE = pstruDSA->pstruIE;
		pstruDSARSP->pstruIE.nSFID = ++g_nTransactionId;
		pstruRSP->pstruMacData->Packet_MACProtocol = pstruDSARSP;
	}
	pstruRSP->pstruMacData->dOverhead = pstruPacket->pstruMacData->dOverhead +1;
	pstruRSP->pstruMacData->dPacketSize = pstruRSP->pstruMacData->dOverhead;
	//Add packet to buffer list
	{
		NetSim_BUFFER* pstruBuffer = DEVICE_MAC_NW_INTERFACE(nDeviceId,nInterfaceId)->pstruAccessBuffer;
		if(!pstruBuffer->pstruPacketlist)
		{
			pstruEventDetails->nSubEventType = 0;
			pstruEventDetails->dPacketSize = pstruRSP->pstruMacData->dPacketSize;
			pstruEventDetails->nApplicationId = 0;
			pstruEventDetails->nEventType = MAC_OUT_EVENT;
			pstruEventDetails->nPacketId = 0;
			pstruEventDetails->pPacket = NULL;
			fnpAddEvent(pstruEventDetails);
		}
		fn_NetSim_Packet_AddPacketToList(pstruBuffer,pstruRSP,0);
	}
	//Add DSA-REP to CR metrics
	pstruBSMAC->struBSMetrics.nDSA_REPSent++;
	return 0;
}
/** A DSA-RSP message shall be generated in response to a received DSA-REQ message.
If the transaction is successful, the DSA-RSP message may contain the following: 

Service Flow parameters (the completespecification of the service flow shall be included 
in the DSA-RSP if it includes a newly assigned FID or an expanded service class name) 
and CS parameter encodings (specification of the service flow’s CS-specific parameters).*/
int fn_NetSim_CR_CPE_ProcessDSA_RSP()
{
	NETSIM_ID nDeviceId = pstruEventDetails->nDeviceId;
	NETSIM_ID nInterfaceId = pstruEventDetails->nInterfaceId;
	CPE_MAC* pstruCPEMAC = DEVICE_MACVAR(nDeviceId,nInterfaceId);
	NetSim_PACKET* pstruPacket = pstruEventDetails->pPacket;
	DSA_RSP* pstruDSA = pstruPacket->pstruMacData->Packet_MACProtocol;
	if(!strcmp(pstruDSA->pstruIE.className,"BE"))
	{
		pstruCPEMAC->pstruServiceParameter->SFID[2] = pstruDSA->pstruIE.nSFID;
	}
	else if(!strcmp(pstruDSA->pstruIE.className,"ERTPS"))
	{
		pstruCPEMAC->pstruServiceParameter->SFID[4] = pstruDSA->pstruIE.nSFID;
	}
	else if(!strcmp(pstruDSA->pstruIE.className,"NRTPS"))
	{
		pstruCPEMAC->pstruServiceParameter->SFID[3] = pstruDSA->pstruIE.nSFID;
	}
	else if(!strcmp(pstruDSA->pstruIE.className,"UGS"))
	{
		pstruCPEMAC->pstruServiceParameter->SFID[5] = pstruDSA->pstruIE.nSFID;
	}
	else if(!strcmp(pstruDSA->pstruIE.className,"NULL"))
	{
		pstruCPEMAC->pstruServiceParameter->SFID[0] = pstruDSA->pstruIE.nSFID;
	}
	else if(!strcmp(pstruDSA->pstruIE.className,"RTPS"))
	{
		pstruCPEMAC->pstruServiceParameter->SFID[4] = pstruDSA->pstruIE.nSFID;
	}
	//Add DSA-REP to CR metrics
	pstruCPEMAC->struCPEMetrics.nDSA_REPReceived++;
	return 0;
}
/** Any service flow can be deleted with the DSD messages. When a service flow is deleted, 
all resources associated with it are released. If a service flow for a provisioned service 
is deleted, the ability to reestablish the service flow for that service is network 
management dependent. However, the deletion of a provisioned service flow shall not
cause a CPE to reinitialize. */
int fn_NetSim_TerminateServiceFlow(NETSIM_ID nDeviceId, NETSIM_ID nInterfaceId, NetSim_PACKET* pstruPacket)
{
	NetSim_PACKET* pstruDSD;
	DSD_REQ* pstruREQ;
	CPE_MAC* pstruCPEMAC = DEVICE_MACVAR(nDeviceId,nInterfaceId);
	int nFID = fn_NetSim_CR_GetFID(pstruPacket->nQOS);
	if(((GMH*)pstruPacket->pstruMacData->Packet_MACProtocol)->sz_Type[FRAGMANET_BIT] == '1')
	{
		FRAGMENT_SUB_HEADER* pstruFragment = ((GMH*)pstruPacket->pstruMacData->Packet_MACProtocol)->subheader[FRAGMANET_BIT];
		if(pstruFragment->FC != B2_01)
			return 1;
	}
		//Create the DSD packet
		pstruDSD = fn_NetSim_Packet_CreatePacket(MAC_LAYER);
		pstruDSD->nControlDataType = CR_CONTROL_PACKET(MMM_DSD_REQ);
		add_dest_to_packet(pstruDSD, pstruCPEMAC->nBSID);
		pstruDSD->nPacketPriority = Priority_High;
		pstruDSD->nPacketType = PacketType_Control;
		pstruDSD->nReceiverId = pstruCPEMAC->nBSID;
		pstruDSD->nSourceId = nDeviceId;
		pstruDSD->nTransmitterId = nDeviceId;
		pstruDSD->pstruMacData->dArrivalTime = pstruEventDetails->dEventTime;
		pstruDSD->pstruMacData->dEndTime = pstruEventDetails->dEventTime;
		pstruDSD->pstruMacData->dStartTime = pstruEventDetails->dEventTime;
		pstruDSD->pstruMacData->nMACProtocol = MAC_PROTOCOL_IEEE802_22;
		pstruREQ = fnpAllocateMemory(1,sizeof * pstruREQ);
		pstruREQ->type = MMM_DSD_REQ;
		pstruREQ->nSFID = pstruCPEMAC->pstruServiceParameter->SFID[nFID];
		pstruREQ->nTransactionID = ++g_nTransactionId;
		pstruREQ->pstruIE.nSFDirection = UPSTREAM;
		pstruREQ->pstruIE.nSFID = pstruREQ->nSFID;
		switch(pstruPacket->nQOS)
		{
		case QOS_BE:
			strcpy(pstruREQ->pstruIE.className,"BE");
			break;
		case QOS_ertPS:
			strcpy(pstruREQ->pstruIE.className,"ERTPS");
			break;
		case QOS_nrtPS:
			strcpy(pstruREQ->pstruIE.className,"NRTPS");
			break;
		case QOS_rtPS:
			strcpy(pstruREQ->pstruIE.className,"RTPS");
			break;
		case QOS_UGS:
			strcpy(pstruREQ->pstruIE.className,"UGS");
			break;
		default:
			strcpy(pstruREQ->pstruIE.className,"NULL");
			break;
		}
		pstruDSD->pstruMacData->Packet_MACProtocol = pstruREQ;
		pstruDSD->pstruMacData->dOverhead = (double)(7+strlen(pstruREQ->pstruIE.className)+DSX_IE_FIXED_SIZE);
		pstruDSD->pstruMacData->dPacketSize = pstruDSD->pstruMacData->dOverhead;
		//Add packet to queued packet list
		pstruDSD->pstruNextPacket = pstruCPEMAC->pstruQueuedPacketList[0];
		pstruCPEMAC->pstruQueuedPacketList[0] = pstruDSD;
		pstruCPEMAC->pstruServiceParameter->Status[nFID]= ServiceState_Deleting;
		//Add DSD-REQ to CR metrics
		pstruCPEMAC->struCPEMetrics.nDSD_REQSent++;
	return 1;
}
/** A Dynamic Service Deletion Request (DSD-REQ) is sent by a CPE or BS to delete an 
existing service flow. */
int fn_NetSim_CR_BS_ProcessDSDReq()
{
	DSD_RSP* pstruDSDRSP;
	NetSim_PACKET* pPacket;
	NETSIM_ID nDeviceId = pstruEventDetails->nDeviceId;
	NETSIM_ID nInterfaceId = pstruEventDetails->nInterfaceId;
	BS_MAC* pstruBSMAC = DEVICE_MACVAR(nDeviceId,nInterfaceId);
	NetSim_PACKET* pstruPacket = pstruEventDetails->pPacket;
	DSD_REQ* pstruDSD = pstruPacket->pstruMacData->Packet_MACProtocol;
	unsigned int nSID = pstruBSMAC->anSIDFromDevId[pstruPacket->nTransmitterId];
	UPLINKALLOCINFO* pstruInfo = pstruBSMAC->uplinkAllocInfo;
	UPLINKALLOCINFO* pstruPrevInfo=NULL;
	//Add DSD-REQ to CR Metrics
	pstruBSMAC->struBSMetrics.nDSD_REQReceived++;
	while(pstruInfo)
	{
		if(nSID == pstruInfo->nSID && pstruInfo->nQoS == fnGetQOS(pstruDSD->pstruIE.className))
		{
			if(pstruPrevInfo)
			{
				pstruPrevInfo->next = pstruInfo->next;
				free(pstruInfo);
			}
			else
			{
				pstruBSMAC->uplinkAllocInfo = pstruInfo->next;
				free(pstruInfo);
			}
			break;
		}
		pstruPrevInfo = pstruInfo;
		pstruInfo = pstruInfo->next;
	}
	//Create DSD rep packet
	pstruDSDRSP = fnpAllocateMemory(1,sizeof* pstruDSDRSP);
	pstruDSDRSP->nTransactionID = pstruDSD->nTransactionID;
	pstruDSDRSP->nSFID = pstruDSD->nSFID;
	pstruDSDRSP->pstruIE = pstruDSD->pstruIE;
	pstruDSDRSP->type = MMM_DSD_REP;
	pPacket = fn_NetSim_Packet_CreatePacket(MAC_LAYER);
	pPacket->nControlDataType = CR_CONTROL_PACKET(MMM_DSD_REP);
	add_dest_to_packet(pPacket, pstruPacket->nSourceId);
	pPacket->nPacketPriority = Priority_High;
	pPacket->nPacketType = PacketType_Control;
	pPacket->nReceiverId = pstruPacket->nTransmitterId;
	pPacket->nSourceId = get_first_dest_from_packet(pstruPacket);
	pPacket->nTransmitterId = pstruPacket->nReceiverId;
	pPacket->pstruMacData->dArrivalTime = pstruEventDetails->dEventTime;
	pPacket->pstruMacData->dOverhead = (double)(DSX_IE_FIXED_SIZE + 8 + strlen(pstruDSDRSP->pstruIE.className));
	pPacket->pstruMacData->dPacketSize = pPacket->pstruMacData->dOverhead;
	pPacket->pstruMacData->nMACProtocol = MAC_PROTOCOL_IEEE802_22;
	pPacket->pstruMacData->Packet_MACProtocol = pstruDSDRSP;
	//Add packet to buffer list
	{
		NetSim_BUFFER* pstruBuffer = DEVICE_MAC_NW_INTERFACE(nDeviceId,nInterfaceId)->pstruAccessBuffer;
		fn_NetSim_Packet_AddPacketToList(pstruBuffer,pPacket,0);
	}
	//Add DSD-REP packet to CR metrics
	pstruBSMAC->struBSMetrics.nDSD_REPSent++;
	return 1;
}
/** A DSD-REP shall be generated in response to a received DSD-REQ. */
int fn_NetSim_CR_CPE_ProcessDSD_REP()
{
	NETSIM_ID nDeviceId = pstruEventDetails->nDeviceId;
	NETSIM_ID nInterfaceId = pstruEventDetails->nInterfaceId;
	CPE_MAC* pstruCPEMAC = (CPE_MAC*)DEVICE_MACVAR(nDeviceId,nInterfaceId);
	NetSim_PACKET* pstruPacket = pstruEventDetails->pPacket;
	DSD_RSP* pstruDSD = (DSD_RSP*)pstruPacket->pstruMacData->Packet_MACProtocol;
	unsigned int nLoop;
	for(nLoop=0;nLoop<MAX_FID;nLoop++)
	{
		if(pstruCPEMAC->pstruServiceParameter->SFID[nLoop] == pstruDSD->nSFID)
		{
			pstruCPEMAC->pstruServiceParameter->SFID[nLoop] = 0;
			pstruCPEMAC->pstruServiceParameter->Status[nLoop] = ServiceState_Deleted;
			//Add DSD-REP to CR metrics
			pstruCPEMAC->struCPEMetrics.nDSD_REPReceived++;
			break;
		}
	}
	return 0;
}
