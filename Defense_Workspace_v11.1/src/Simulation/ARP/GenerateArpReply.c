/************************************************************************************
 * Copyright (C) 2013     
 *
 * TETCOS, Bangalore. India                                                         *

 * Tetcos owns the intellectual property rights in the Product and its content.     *
 * The copying, redistribution, reselling or publication of any or all of the       *
 * Product or its content without express prior written consent of Tetcos is        *
 * prohibited. Ownership and / or any other right relating to the software and all  *
 * intellectual property rights therein shall remain at all times with Tetcos.      *

 * Author:   Basamma YB 
 * Date  :    
 ************************************************************************************/
#define _CRT_SECURE_NO_DEPRECATE
#include "main.h"
#include "ARP.h"
/**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
This function is called whenever the NETWORK_IN_EVENT triggred with the control 
packet as REQUEST_PACKET. That is ARP request is received. Then call this
function to generate REPLY_PACKET. Do the following to generate Reply
1.Check for the hardware type, protocol type,MAC address lenth, IP adress length
  are correct matching.
2. Check the Source IP and MAC entry present in the table, if not update the table.
3. Check the packet received device is the destination, if not, dicard the packet,
  else Generate the REPLY_PACKET by updating its MAC address.		
4.Add the control packet to access buffer to farward the packet to next layer.	
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/
int fn_NetSim_Generate_ARP_Reply(NetSim_EVENTDETAILS *pstruEventDetails, struct stru_NetSim_Network *NETWORK)
//_declspec (dllexport) int fn_NetSim_Generate_ARP_Reply()
{
	NETSIM_ID nDeviceId;
	NETSIM_ID nInterfaceId;		
	int nMerge_flag = 0;
	int nType;		
	unsigned int unProtocolId;	
	NETSIM_IPAddress szSrcIPadd;		
	NETSIM_IPAddress szDestIPadd;			
	PNETSIM_MACADDRESS szTargetMAC;	
	
	NetSim_PACKET *pstruTemp_Data;		// NetSim Temp packet to store the packet from the event details	
	NetSim_PACKET *pstruControlPacket;	// NetSim packet To store the ARP packet		
	ARP_PACKET *pstruArpRequestPkt;		// ARP PACKET to store ARP Request pcaket  	
	ARP_PACKET *pstruArpReplyPkt;		// ARP PACKET to store ARP Reply pcaket 		
	ARP_TABLE  *pstruTableHead,*pstruCurrentTable;	//Type casting Arp Table
	ARP_VARIABLES *pstruArpVariables;	//type casting Arp variables 

	//Get the packet from the event 
	pstruTemp_Data =pstruEventDetails->pPacket;	
	pstruControlPacket = pstruEventDetails->pPacket;
	// Get the Arp Packet from the NetworkData of temp packet
	pstruArpRequestPkt = pstruTemp_Data->pstruNetworkData->Packet_NetworkProtocol;
	// Get the device and interface Id from the event
	nInterfaceId = pstruEventDetails->nInterfaceId;
	nDeviceId = pstruEventDetails->nDeviceId;

	if(pstruArpRequestPkt->n_ar$hrd != IEEE802)		
		fnNetSimError("Mismatch in ARP Hardware Type");			
	if(pstruArpRequestPkt->usn_ar$hln != HARDWARE_ADDRESS_LENGTH)
		fnNetSimError("Mismatch in ARP Hardware/MAC Adress Length ");	
	if(pstruArpRequestPkt->n_ar$pro != ARP_TO_RESOLVE_IP)
		fnNetSimError("Mismatch in ARP Protocol Type");	
	unProtocolId = fn_NetSim_Stack_GetNWProtocol(nDeviceId);
	if(unProtocolId == NW_PROTOCOL_IPV4)
	{
		if(pstruArpRequestPkt->usn_ar$pln != IPV4_PROTOCOL_ADDREES_LENGTH)
		fnNetSimError("Mismatch in ARP Protocol IPV4 Adress Length ");
	}
	else
	{
		if(pstruArpRequestPkt->usn_ar$pln != IPV6_PROTOCOL_ADDREES_LENGTH)
		fnNetSimError("Mismatch in ARP Protocol IPV6 Adress Length ");
	}
	// Get the Arp variables from device ipVar
	pstruArpVariables = DEVICE_INTERFACE(nDeviceId,nInterfaceId)->ipVar;	
	pstruTableHead = pstruArpVariables->pstruArpTable;	
	pstruCurrentTable = pstruTableHead;	
	// Get the source IP address from the request packet
	szSrcIPadd = pstruArpRequestPkt->sz_ar$spa;//_strdup(pstruArpRequestPkt->sz_ar$spa);
	// Check the Source entry in the ARP TABLE 
	while(pstruCurrentTable != NULL)
	{
		if(IP_COMPARE(pstruCurrentTable->szIPAddress,szSrcIPadd)== 0)
		{	// set the merge flag if entry exists
			nMerge_flag = 1;			
			break;
		}
		else 
		pstruCurrentTable = pstruCurrentTable->pstruNextEntry;
	}			
	// Source entry not there in the ARP table, update the table.
	if(nMerge_flag == 0)
	{		
		nType = DYNAMIC;
		fn_NetSim_Add_IP_MAC_AddressTo_ARP_Table(&pstruTableHead,pstruArpRequestPkt->sz_ar$spa,pstruArpRequestPkt->sz_ar$sha,nType);	
		pstruArpVariables->pstruArpTable = pstruTableHead;
		nMerge_flag =1;
	}
				/* Generate ARP Reply */	
	// Check for REQUEST, and send the REPLY PACKET 
	if(pstruArpRequestPkt->n_ar$op == ares_opSREQUEST)
	{	
		//Allocate memory for the ARP Reply packet
		pstruArpReplyPkt = fnpAllocateMemory(1,sizeof(ARP_PACKET)); 				
		//Swap IP and MAC of source and destinations from the Request packet
		pstruArpReplyPkt->sz_ar$spa = IP_COPY(pstruArpRequestPkt->sz_ar$tpa);
		pstruArpReplyPkt->sz_ar$sha = DEVICE_INTERFACE(nDeviceId,nInterfaceId)->pstruMACLayer->szMacAddress;
		pstruArpReplyPkt->sz_ar$tpa = IP_COPY(pstruArpRequestPkt->sz_ar$spa);
		pstruArpReplyPkt->sz_ar$tha = pstruArpRequestPkt->sz_ar$sha;
		pstruArpReplyPkt->n_ar$op = ares_opSREPLY; // Set Opcode as REPLY 
		//Set the Ethernet header details
		pstruArpReplyPkt->szDestMac = pstruArpRequestPkt->sz_ar$sha;
		pstruArpReplyPkt->szSrcMac = (DEVICE_INTERFACE(nDeviceId,nInterfaceId)->pstruMACLayer->szMacAddress);
		pstruArpReplyPkt->nEther_type = ADDRESS_RESOLUTION;		
		//Add other relevent fields 
		pstruArpReplyPkt->n_ar$hrd = IEEE802;				 // H/W type
		pstruArpReplyPkt->n_ar$pro = ARP_TO_RESOLVE_IP;		 // Protocol type	
		pstruArpReplyPkt->usn_ar$hln = HARDWARE_ADDRESS_LENGTH; // MAC add length in bytes
		// Create netsim packet for NETWORK_LAYER
		pstruControlPacket = fn_NetSim_Packet_CreatePacket(NETWORK_LAYER);
		//Generate the control packet 			
		pstruControlPacket->dEventTime = pstruEventDetails->dEventTime;
		add_dest_to_packet(pstruControlPacket, pstruEventDetails->pPacket->nSourceId);
		pstruControlPacket->nPacketType = PacketType_Control;
		pstruControlPacket->nControlDataType = REPLY_PACKET;
		pstruControlPacket->nTransmitterId=pstruEventDetails->nDeviceId;
		pstruControlPacket->nReceiverId = pstruEventDetails->pPacket->nTransmitterId;//0;
		pstruControlPacket->nSourceId = pstruEventDetails->nDeviceId;
		//Update IP address in the packet
		pstruControlPacket->pstruNetworkData->szSourceIP = IP_COPY(DEVICE_NWADDRESS(nDeviceId,nInterfaceId));
		pstruControlPacket->pstruNetworkData->szDestIP = IP_COPY(pstruArpRequestPkt->sz_ar$spa);		 
		//Update MAC address in the packet
		pstruControlPacket->pstruMacData->szSourceMac = DEVICE_INTERFACE(nDeviceId,nInterfaceId)->pstruMACLayer->szMacAddress;
		pstruControlPacket->pstruMacData->szDestMac = pstruArpRequestPkt->sz_ar$sha;					
		//Update NetworkData packet timings 
		pstruControlPacket->pstruNetworkData->dArrivalTime = pstruEventDetails->dEventTime;
		pstruControlPacket->pstruNetworkData->dEndTime = pstruEventDetails->dEventTime;
		pstruControlPacket->pstruNetworkData->dStartTime = pstruEventDetails->dEventTime;
		pstruControlPacket->nPacketPriority = Priority_High;
		//Add overheads and size to NetworkData of the packet		
		unProtocolId = fn_NetSim_Stack_GetNWProtocol(nDeviceId);
		if(unProtocolId == NW_PROTOCOL_IPV4)
		{			
			pstruArpReplyPkt->usn_ar$pln = IPV4_PROTOCOL_ADDREES_LENGTH; // IP add length in bytes
			pstruControlPacket->pstruNetworkData->dPayload = IPV4_ARP_PACKET_SIZE_WITH_ETH_HEADER;
			pstruControlPacket->pstruNetworkData->dOverhead =IPV4_NETWORK_OVERHEADS;			
		}
		else
		{
			pstruArpReplyPkt->usn_ar$pln = IPV6_PROTOCOL_ADDREES_LENGTH; // IP add length in bytes
			pstruControlPacket->pstruNetworkData->dPayload = IPV6_ARP_PACKET_SIZE_WITH_ETH_HEADER;
			pstruControlPacket->pstruNetworkData->dOverhead = IPV6_NETWORK_OVERHEADS;			
		}
		pstruControlPacket->pstruNetworkData->dPacketSize=pstruControlPacket->pstruNetworkData->dPayload + pstruControlPacket->pstruNetworkData->dOverhead;	 
		// Assign Protocol Id
		pstruControlPacket->pstruNetworkData->nNetworkProtocol= NW_PROTOCOL_ARP;		
		// Assign ARP Reply packet to NetworkProtocolPacket
		pstruControlPacket->pstruNetworkData->Packet_NetworkProtocol = pstruArpReplyPkt;
		pstruControlPacket->pstruNextPacket=NULL;	
		
		// Get buffer status
		if(!fn_NetSim_GetBufferStatus(DEVICE_INTERFACE(nDeviceId,nInterfaceId)->pstruAccessInterface->pstruAccessBuffer))
		{						
			pstruEventDetails->nSubEventType = 0; 			
			pstruEventDetails->dPacketSize = pstruControlPacket->pstruNetworkData->dPacketSize;			
			pstruEventDetails->nProtocolId = fn_NetSim_Stack_GetMacProtocol(nDeviceId,nInterfaceId);				
			pstruEventDetails->nEventType=MAC_OUT_EVENT;
			pstruEventDetails->pPacket =  NULL;					
			fnpAddEvent( pstruEventDetails);			
		}
		//Add packet to mac buffer
		fn_NetSim_Packet_AddPacketToList(DEVICE_INTERFACE(nDeviceId,nInterfaceId)->pstruAccessInterface->pstruAccessBuffer,pstruControlPacket,0);
		
		// Increment the reply sent count		
		pstruArpVariables->pstruArpMetrics->nArpReplySentCount += 1;
		
		// Write to LogFile	
		szDestIPadd = DEVICE_NWADDRESS(nDeviceId,nInterfaceId);
		szTargetMAC = pstruArpReplyPkt->sz_ar$sha;
		printf("ARP---ARP_ReplySent\tEventTime:%0.3lf\tSrcIP:%s\tMAC_Add:%s\tDestIP:%s \n",
			   pstruEventDetails->dEventTime,
			   szDestIPadd->str_ip,
			   szTargetMAC->szmacaddress,
			   szSrcIPadd->str_ip);

	}
	else
	{			
		fnNetSimError("Error in ARP REQEUST. This function should never be called");
	}	
	// After sending the reply drop the request packet
	fn_NetSim_Packet_FreePacket(pstruTemp_Data);
	pstruTemp_Data = NULL;
	pstruEventDetails->pPacket = NULL;	
	return 0;
}



