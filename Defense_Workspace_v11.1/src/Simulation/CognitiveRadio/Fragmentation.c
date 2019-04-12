#include "main.h"
#include "802_22.h"
/** In the MAC, the transmitting side has full discretion whether or not to pack 
a group of MAC SDUs into a single MAC PDU. BSs and CPEs shall both have the capability 
of unpacking. If packing is turned on for a connection, the MAC may pack multiple MAC SDUs
into a single MAC PDU. Also, packing makes use of the connection attribute indicating 
whether the connection carries fixed-length or variable-length packets. */
NetSim_PACKET* fn_NetSim_CR_BS_PackPacket(BS_MAC* pstruBSMac,NetSim_PACKET* pstruPacket)
{
	NetSim_PACKET* pstruprevFrag=NULL;
	FRAGMENT_SUB_HEADER* pstruFragHed;
	GMH* pstruGMH = (GMH*)pstruPacket->pstruMacData->Packet_MACProtocol;
	NetSim_PACKET* pstruFragmentList = pstruBSMac->pstruFragmentPacketList;
	if(pstruGMH->sz_Type[FRAGMANET_BIT] == '0')
	{
		return pstruPacket;
	}
	pstruFragHed = pstruGMH->subheader[FRAGMANET_BIT];
	if(pstruFragmentList == NULL)
	{
		pstruBSMac->pstruFragmentPacketList = pstruPacket;
		return NULL;
	}
	while(pstruFragmentList)
	{
		FRAGMENT_SUB_HEADER* pstruFrag = ((GMH*)pstruFragmentList->pstruMacData->Packet_MACProtocol)->subheader[FRAGMANET_BIT];
		if(pstruFrag->nFragmentId == pstruFragHed->nFragmentId)
		{
			pstruFragmentList->pstruMacData->dPacketSize += pstruPacket->pstruMacData->dPacketSize;
			pstruFragmentList->pstruMacData->dPayload += pstruPacket->pstruMacData->dPayload;
			pstruFragmentList->pstruMacData->dOverhead += pstruPacket->pstruMacData->dOverhead;
			pstruFragmentList->pstruMacData->dEndTime = pstruPacket->pstruMacData->dEndTime;
			
			pstruFragmentList->pstruPhyData->dPacketSize += pstruPacket->pstruPhyData->dPacketSize;
			pstruFragmentList->pstruPhyData->dPayload += pstruPacket->pstruPhyData->dPayload;
			pstruFragmentList->pstruPhyData->dOverhead += pstruPacket->pstruPhyData->dOverhead;
			pstruFragmentList->pstruPhyData->dEndTime = pstruPacket->pstruPhyData->dEndTime;
			
			if(pstruFragmentList->pstruAppData && pstruPacket->pstruAppData)
				pstruFragmentList->pstruAppData->dPayload += pstruPacket->pstruAppData->dPayload;
			fn_NetSim_Packet_FreePacket(pstruPacket);
			pstruPacket = NULL;
			if(pstruFragHed->FC == B2_01)
			{
				//Last fragment
				if(pstruprevFrag)
				{
					pstruprevFrag->pstruNextPacket = pstruFragmentList->pstruNextPacket;
					pstruFragmentList->pstruNextPacket = NULL;
				}
				else
				{
					pstruBSMac->pstruFragmentPacketList = pstruFragmentList->pstruNextPacket;
					pstruFragmentList->pstruNextPacket = NULL;
				}
				break;
			}
			return NULL;
		}
		pstruprevFrag = pstruFragmentList;
		pstruFragmentList = pstruFragmentList->pstruNextPacket;
	}
	if(pstruFragmentList)
	{
		return pstruFragmentList;
	}
	if(pstruPacket)
	{
		if(pstruprevFrag)
		{
			pstruprevFrag->pstruNextPacket = pstruPacket;
		}
	}
	return NULL;
}
/** In the MAC, the transmitting side has full discretion whether or not to pack 
a group of MAC SDUs into a single MAC PDU. BSs and CPEs shall both have the capability 
of unpacking. If packing is turned on for a connection, the MAC may pack multiple MAC SDUs
into a single MAC PDU. Also, packing makes use of the connection attribute indicating 
whether the connection carries fixed-length or variable-length packets. */
int fn_NetSim_CR_CPE_PackPacket()
{
	NetSim_PACKET* pstruprevFrag=NULL;
	FRAGMENT_SUB_HEADER* pstruFragHed;
	CPE_MAC* pstruCPEMAC = (CPE_MAC*)DEVICE_MACVAR(pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId);
	NetSim_PACKET* pstruPacket = pstruEventDetails->pPacket;
	GMH* pstruGMH = (GMH*)pstruPacket->pstruMacData->Packet_MACProtocol;
	NetSim_PACKET* pstruFragmentList = pstruCPEMAC->pstruFragmentPacketList;
	if(pstruGMH->sz_Type[FRAGMANET_BIT] == '0')
	{
		//Move packet to network in
		pstruEventDetails->nEventType = NETWORK_IN_EVENT;
		pstruEventDetails->dPacketSize -= pstruPacket->pstruMacData->dOverhead;
		pstruEventDetails->nProtocolId = (NETSIM_ID)fn_NetSim_Stack_GetNWProtocol(pstruEventDetails->nDeviceId);
		pstruEventDetails->nSubEventType = 0;
		fnpAddEvent(pstruEventDetails);
		return 0;
	}
	pstruFragHed = pstruGMH->subheader[FRAGMANET_BIT];
	if(pstruFragmentList == NULL)
	{
		pstruCPEMAC->pstruFragmentPacketList = pstruPacket;
		return 0;
	}
	while(pstruFragmentList)
	{
		FRAGMENT_SUB_HEADER* pstruFrag = ((GMH*)pstruFragmentList->pstruMacData->Packet_MACProtocol)->subheader[FRAGMANET_BIT];
		if(pstruFrag->nFragmentId == pstruFragHed->nFragmentId)
		{
			pstruFragmentList->pstruMacData->dPacketSize += pstruPacket->pstruMacData->dPacketSize;
			pstruFragmentList->pstruMacData->dPayload += pstruPacket->pstruMacData->dPayload;
			pstruFragmentList->pstruMacData->dOverhead += pstruPacket->pstruMacData->dOverhead;
			pstruFragmentList->pstruMacData->dEndTime = pstruPacket->pstruMacData->dEndTime;

			pstruFragmentList->pstruPhyData->dPacketSize += pstruPacket->pstruPhyData->dPacketSize;
			pstruFragmentList->pstruPhyData->dPayload += pstruPacket->pstruPhyData->dPayload;
			pstruFragmentList->pstruPhyData->dOverhead += pstruPacket->pstruPhyData->dOverhead;
			pstruFragmentList->pstruPhyData->dEndTime = pstruPacket->pstruPhyData->dEndTime;


			if(pstruFragmentList->pstruAppData && pstruPacket->pstruAppData)
				pstruFragmentList->pstruAppData->dPayload += pstruPacket->pstruAppData->dPayload;
			if(pstruFragHed->FC == B2_01)
			{
				//Last fragment
				if(pstruprevFrag)
				{
					pstruprevFrag->pstruNextPacket = pstruFragmentList->pstruNextPacket;
					pstruFragmentList->pstruNextPacket = NULL;
				}
				else
				{
					pstruCPEMAC->pstruFragmentPacketList = pstruFragmentList->pstruNextPacket;
					pstruFragmentList->pstruNextPacket = NULL;
				}
				break;
			}
			fn_NetSim_Packet_FreePacket(pstruPacket);
			pstruPacket = NULL;
			return 0;
		}
		pstruprevFrag = pstruFragmentList;
		pstruFragmentList = pstruFragmentList->pstruNextPacket;
	}
	if(pstruFragmentList)
	{
		//Move packet to network in
		pstruEventDetails->nEventType = NETWORK_IN_EVENT;
		pstruEventDetails->dPacketSize -= pstruFragmentList->pstruMacData->dOverhead;
		pstruEventDetails->nProtocolId = (NETSIM_ID)fn_NetSim_Stack_GetNWProtocol(pstruEventDetails->nDeviceId);
		pstruEventDetails->nSubEventType = 0;
		pstruEventDetails->pPacket = pstruFragmentList;
		fnpAddEvent(pstruEventDetails);
		return 0;
	}
	if(pstruPacket)
	{
		if(pstruprevFrag)
		{
			pstruprevFrag->pstruNextPacket = pstruPacket;
		}
	}
	return 0;
}

/** Fragmentation is the process by which a MAC SDU is divided into one or more MAC PDUs. 
This process is undertaken to allow efficient use of available bandwidth relative to the 
QoS requirements of a connection’s service flow. Upon the creation of a connection by the 
MAC SAP, fragmentation capability is defined. Fragmentation may be initiated by a BS for 
downstream connections and by a CPE for upstream connections. */
int fn_NetSim_CR_FragmentPacket(NetSim_PACKET* pstruPacket,double dSDUSize)
{
	NetSim_PACKET* pstruTempPacket;
	FRAGMENT_SUB_HEADER* pstruFragment;
	GMH* pstruGMH;
	int nCount;
	unsigned int n=0;
	double dSize = pstruPacket->pstruNetworkData->dPacketSize;
	double dAppSize;
	if(dSize <= dSDUSize)
	{
		/* update the MAC header*/
		pstruPacket->pstruMacData->dArrivalTime = pstruEventDetails->dEventTime;
		pstruPacket->pstruMacData->dPayload = pstruPacket->pstruNetworkData->dPacketSize;
		pstruPacket->pstruMacData->dOverhead = GMH_SIZE;
		pstruPacket->pstruMacData->dPacketSize = pstruPacket->pstruMacData->dOverhead + pstruPacket->pstruMacData->dPayload;
		
		pstruPacket->pstruPhyData->dPayload = pstruPacket->pstruMacData->dPacketSize;
		pstruPacket->pstruPhyData->dOverhead = 0;
		pstruPacket->pstruPhyData->dPacketSize = pstruPacket->pstruPhyData->dOverhead + pstruPacket->pstruPhyData->dPayload;
		
		pstruGMH = fnpAllocateMemory(1,sizeof *pstruGMH);
		pstruGMH->n_Length = (int)pstruPacket->pstruMacData->dPacketSize;
		pstruGMH->n_UCS = 0;
		pstruGMH->sz_Type[FRAGMANET_BIT] = '0';
		pstruPacket->pstruMacData->Packet_MACProtocol =pstruGMH;
		pstruPacket->pstruMacData->nMACProtocol = MAC_PROTOCOL_IEEE802_22;
		return 0;
	}
	++g_FragmentId;
	nCount = (int)ceil(dSize/dSDUSize);
	if(pstruPacket->pstruAppData)
		dAppSize = pstruPacket->pstruAppData->dPayload/nCount;
	else
		dAppSize = 0;
	/* update the MAC header*/
	pstruPacket->pstruMacData->dArrivalTime = pstruEventDetails->dEventTime;
	pstruPacket->pstruMacData->dPayload = dSDUSize;
	pstruPacket->pstruMacData->dOverhead = GMH_SIZE + FRAGMENT_SIZE;
	pstruPacket->pstruMacData->dPacketSize = pstruPacket->pstruMacData->dOverhead + pstruPacket->pstruMacData->dPayload;
	
	pstruPacket->pstruPhyData->dPayload = pstruPacket->pstruMacData->dPacketSize;
	pstruPacket->pstruPhyData->dOverhead = 0;
	pstruPacket->pstruPhyData->dPacketSize = pstruPacket->pstruPhyData->dOverhead + pstruPacket->pstruPhyData->dPayload;
	
	if(pstruPacket->pstruAppData)
		pstruPacket->pstruAppData->dPayload = dAppSize;
	pstruGMH = fnpAllocateMemory(1,sizeof *pstruGMH);
	pstruGMH->n_Length = (int)pstruPacket->pstruMacData->dPacketSize;
	pstruGMH->n_UCS = 0;
	pstruPacket->pstruMacData->Packet_MACProtocol =pstruGMH;
	pstruPacket->pstruMacData->nMACProtocol = MAC_PROTOCOL_IEEE802_22;
	pstruFragment = fnpAllocateMemory(1,sizeof *pstruFragment);
	pstruFragment->purpos = 0;
	pstruFragment->FC = B2_10;
	pstruFragment->length = (unsigned int)dSDUSize;
	pstruFragment->FSN = ++n;
	pstruFragment->nFragmentId = g_FragmentId;
	pstruGMH->subheader[FRAGMANET_BIT] = pstruFragment;
	pstruGMH->sz_Type[FRAGMANET_BIT] = '1';
	dSize -= dSDUSize;
	nCount--;
	while(nCount)
	{
		pstruTempPacket = fn_NetSim_Packet_CopyPacket(pstruPacket);
		pstruTempPacket->pstruNextPacket = NULL;
		pstruPacket->pstruNextPacket = pstruTempPacket;
		pstruPacket = pstruPacket->pstruNextPacket;
		pstruPacket->pstruMacData->dArrivalTime = pstruEventDetails->dEventTime;
		pstruPacket->pstruMacData->dOverhead = GMH_SIZE + FRAGMENT_SIZE;
		pstruPacket->pstruMacData->dPacketSize = pstruPacket->pstruMacData->dOverhead + pstruPacket->pstruMacData->dPayload;
		
		pstruPacket->pstruPhyData->dPayload = pstruPacket->pstruMacData->dPacketSize;
		pstruPacket->pstruPhyData->dOverhead = 0;
		pstruPacket->pstruPhyData->dPacketSize = pstruPacket->pstruPhyData->dOverhead + pstruPacket->pstruPhyData->dPayload;
		
		pstruGMH = PACKET_MACPROTOCOLDATA(pstruPacket);
		pstruGMH->n_Length = (int)pstruPacket->pstruMacData->dPacketSize;
		pstruGMH->n_UCS = 0;
		pstruPacket->pstruMacData->Packet_MACProtocol =pstruGMH;
		pstruPacket->pstruMacData->nMACProtocol = MAC_PROTOCOL_IEEE802_22;
		pstruFragment = pstruGMH->subheader[FRAGMANET_BIT];
		pstruFragment->purpos = 0;
		pstruFragment->length = (unsigned int)dSDUSize;
		pstruFragment->FSN = ++n;
		pstruFragment->nFragmentId = g_FragmentId;
		if(dSize>dSDUSize)
		{
			pstruPacket->pstruMacData->dPayload = dSDUSize;
			pstruFragment->FC = B2_11;
		}
		else
		{
			pstruPacket->pstruMacData->dPayload = dSize;
			pstruFragment->FC = B2_01;
		}
		pstruGMH->sz_Type[FRAGMANET_BIT] = '1';
		dSize -= dSDUSize;
		nCount--;
	}
	return 0;
}


