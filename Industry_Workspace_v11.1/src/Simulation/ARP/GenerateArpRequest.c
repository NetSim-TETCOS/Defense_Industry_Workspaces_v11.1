/************************************************************************************
 * Copyright (C) 2013																*
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
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Function: Generate ARP Request
Whenever the ARP Table do not have the destination device MAC address, this function            
is called to generate BROARDCAST ARP request. Once the Request packet generated, if 
reply won't get, before generating the next request, it has to wait till the 
ARP_RETRY_INTERVAL (default 10secs) The number of retries is equal to ARP_RETRY_LIMIT 
(default 3). So ARP Request packet generated, buffer the packet till you get reply or 
RequestSentCount reaches  ARP_RETRY_LIMIT. If you got the reply, farward the buffered 
packet, else drop the buffered packets.
Do the following to generate Request
1. Check the RequestFlag. First time it is 0, buffer the packet, Generate the request
	and set the flag to 1.
2.Add the relent filelds hardware type, protocol type,MAC address lenth, IP adress length
  SrcAddIP, DestAddIP,SrcMacAdd etc relevent fields for ARP request
3.Add the request packet to access buffer to farward the packet to next layer.	
4. Generate the Request TimeOut event by adding the ARP_RETRY_INTERVAL to the current event time.
5.When the RequestFlag is 1, buffer the packets
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/
int fn_NetSim_Generate_ARP_Request(NetSim_EVENTDETAILS *pstruEventDetails, struct stru_NetSim_Network *NETWORK)
{
	NETSIM_ID nDeviceId;	
	NETSIM_ID nInterfaceId;	
	NETSIM_ID nDestinationId = 0;
	int nCheckSubnet;	
	NETSIM_IPAddress szSrcIPadd,szNextHopIp;	
	NETSIM_IPAddress szSubnetMaskAdd;		
	NETSIM_IPAddress szDestIPadd;		
	unsigned int unProtocolId;
	
	NetSim_PACKET *pstruTemp_Data;		// NetSim Temp packet to store the packet from the event details				
	NetSim_PACKET *pstruControlPacket;	// NetSim packet To store the ARP packet	
	ARP_PACKET *pstruArpRequestPkt;		// ARP PACKET to store ARP packet data 	
	ARP_VARIABLES *pstruArpVariables;	// type casting Arp variables 

	//Get the packet from the event 
	pstruTemp_Data =pstruEventDetails->pPacket;		
	// Get the device and interface Id from the event
	nInterfaceId = pstruEventDetails->nInterfaceId;
	nDeviceId = pstruEventDetails->nDeviceId;
	// Get the source and destination IP address from the packet
	szSrcIPadd = pstruTemp_Data->pstruNetworkData->szSourceIP;
	szDestIPadd = pstruTemp_Data->pstruNetworkData->szDestIP;
	szNextHopIp = pstruTemp_Data->pstruNetworkData->szNextHopIp;
	if(IP_COMPARE(szSrcIPadd,DEVICE_INTERFACE(nDeviceId,nInterfaceId)->szAddress) != 0)
	{
		szSrcIPadd = DEVICE_INTERFACE(nDeviceId,nInterfaceId)->szAddress;
	}
	if(szNextHopIp)
		szDestIPadd = szNextHopIp;

	// Get the SubnetMaskAdd from the device
	szSubnetMaskAdd = DEVICE_INTERFACE(nDeviceId,nInterfaceId)->szSubnetMask;
	// Call the IPV4 function to check Destination MAC in the same subnet/LAN
	nCheckSubnet = IP_IS_IN_SAME_NETWORK_IPV4(szSrcIPadd,szDestIPadd,szSubnetMaskAdd);	
	// Get the Arp variables from device ipVar
	pstruArpVariables = DEVICE_INTERFACE(nDeviceId,nInterfaceId)->ipVar;	
	/* Generate ARP Request */	
	//Allocate memory for the ARP packet
	pstruArpRequestPkt = fnpAllocateMemory(1,sizeof(ARP_PACKET)); 	
	switch(nCheckSubnet)
	{
	case 1: // Destination is in the same LAN	
		nDestinationId = get_first_dest_from_packet(pstruEventDetails->pPacket);
		pstruArpRequestPkt->sz_ar$tpa = IP_COPY(szDestIPadd);			
		break;
	default:
		{
			char sip[_NETSIM_IP_LEN],dip[_NETSIM_IP_LEN];
			IP_TO_STR(szSrcIPadd,sip);
			IP_TO_STR(szDestIPadd,dip);
			fnNetSimError("ARP--- Packet nexthop and gateway in different LAN\nSrc IP = %s\tDest IP =%s\n",sip,dip);
		}
		break;
	}
	//Add the relevant fields for ARP REQUEST
	pstruArpRequestPkt->n_ar$hrd = IEEE802;					// H/W type
	pstruArpRequestPkt->n_ar$pro = ARP_TO_RESOLVE_IP;		// Protocol type
	pstruArpRequestPkt->usn_ar$hln = HARDWARE_ADDRESS_LENGTH; // MAC add length in bytes
	pstruArpRequestPkt->n_ar$op = ares_opSREQUEST;			// 1-REQUEST, 2-REPLY
	//Get the source MAC address from the device MAC layer details	
	pstruArpRequestPkt->sz_ar$sha= DEVICE_INTERFACE(nDeviceId,nInterfaceId)->pstruMACLayer->szMacAddress;
	pstruArpRequestPkt->sz_ar$spa = IP_COPY(szSrcIPadd);	// Source IP Add	
	pstruArpRequestPkt->sz_ar$tha = NULL;
	//Add the Ethernet header details
	pstruArpRequestPkt->szDestMac = BROADCAST_MAC;
	pstruArpRequestPkt->szSrcMac = DEVICE_INTERFACE(nDeviceId,nInterfaceId)->pstruMACLayer->szMacAddress;
	pstruArpRequestPkt->nEther_type = ADDRESS_RESOLUTION;			
	// Create netsim packet for  NETWORK_LAYER
	pstruControlPacket = fn_NetSim_Packet_CreatePacket(NETWORK_LAYER);	
	//Generate the control packet 
	pstruControlPacket->dEventTime = pstruEventDetails->dEventTime;
	add_dest_to_packet(pstruControlPacket, 0);
	pstruControlPacket->nPacketType = PacketType_Control; 
	pstruControlPacket->nControlDataType = REQUEST_PACKET;
	pstruControlPacket->nTransmitterId = pstruEventDetails->nDeviceId;
	pstruControlPacket->nReceiverId = 0;
	pstruControlPacket->nSourceId = pstruEventDetails->nDeviceId;
	//Update IP address in the packet. 
	pstruControlPacket->pstruNetworkData->szSourceIP = IP_COPY(DEVICE_NWADDRESS(nDeviceId,nInterfaceId));
	// For the BROADCAST control packet, DestIP should be broadcast IP 
	pstruControlPacket->pstruNetworkData->szDestIP = szBroadcastIPaddress;
	//Update MAC address in the packet
	pstruControlPacket->pstruMacData->szSourceMac = (DEVICE_INTERFACE(nDeviceId,nInterfaceId)->pstruMACLayer->szMacAddress);
	// for the BROADCAST control packet, DestMac should be broadcast MAC 
	pstruControlPacket->pstruMacData->szDestMac = BROADCAST_MAC;
	//Update NetworkData packet timings 
	pstruControlPacket->pstruNetworkData->dArrivalTime = pstruEventDetails->dEventTime;
	pstruControlPacket->pstruNetworkData->dEndTime = pstruEventDetails->dEventTime;
	pstruControlPacket->pstruNetworkData->dStartTime = pstruEventDetails->dEventTime;
	pstruControlPacket->nPacketPriority = Priority_High;
	//Add overheads and size to NetworkData of the packet
	unProtocolId = fn_NetSim_Stack_GetNWProtocol(nDeviceId);
	if(unProtocolId == NW_PROTOCOL_IPV4)
	{
		pstruArpRequestPkt->usn_ar$pln = IPV4_PROTOCOL_ADDREES_LENGTH; 
		pstruControlPacket->pstruNetworkData->dPayload = IPV4_ARP_PACKET_SIZE_WITH_ETH_HEADER;
		pstruControlPacket->pstruNetworkData->dOverhead = IPV4_NETWORK_OVERHEADS;	
	}
	else
	{
		pstruArpRequestPkt->usn_ar$pln = IPV6_PROTOCOL_ADDREES_LENGTH; 
		pstruControlPacket->pstruNetworkData->dPayload = IPV6_ARP_PACKET_SIZE_WITH_ETH_HEADER;
		pstruControlPacket->pstruNetworkData->dOverhead = IPV6_NETWORK_OVERHEADS;
	}
	pstruControlPacket->pstruNetworkData->dPacketSize = pstruControlPacket->pstruNetworkData->dPayload + pstruControlPacket->pstruNetworkData->dOverhead;	 
	// Assign Protocol Id
	pstruControlPacket->pstruNetworkData->nNetworkProtocol = NW_PROTOCOL_ARP;	
	// Assign ARP Request packet to NetworkProtocolPacket
	pstruControlPacket->pstruNetworkData->Packet_NetworkProtocol = pstruArpRequestPkt;
	pstruControlPacket->pstruNextPacket=NULL;
	
	// Get buffer status
	if(!fn_NetSim_GetBufferStatus(DEVICE_INTERFACE(nDeviceId,nInterfaceId)->pstruAccessInterface->pstruAccessBuffer))
	{	
		pstruEventDetails->nSubEventType = 0; 
		pstruEventDetails->dPacketSize = pstruControlPacket->pstruNetworkData->dPacketSize;			
		pstruEventDetails->nProtocolId = fn_NetSim_Stack_GetMacProtocol(nDeviceId,nInterfaceId);	
		pstruEventDetails->nApplicationId = 0;	
		pstruEventDetails->nPacketId = 0;
		pstruEventDetails->nSegmentId = 0;
		pstruEventDetails->nEventType =MAC_OUT_EVENT;
		pstruEventDetails->pPacket = NULL;
		fnpAddEvent( pstruEventDetails);	
	}
	fn_NetSim_Packet_AddPacketToList(DEVICE_INTERFACE(nDeviceId,nInterfaceId)->pstruAccessInterface->pstruAccessBuffer,pstruControlPacket,0);

	pstruArpVariables->pnArpRequestFlag[nDestinationId] = 1; //Set the request Flag 
	pstruArpVariables->pstruArpMetrics->nArpRequestSentCount+= 1;
	
	// Write to Log file	
	printf("ARP---ARP_RequestSent\tEventTime:%0.3lf\tSrcIP:%s\tDestIP:%s \n",pstruEventDetails->dEventTime,szSrcIPadd->str_ip,pstruArpRequestPkt->sz_ar$tpa->str_ip);
	
	// copy the control packet for generating time out event
	pstruControlPacket = fn_NetSim_Packet_CopyPacket(pstruTemp_Data);
	remove_dest_from_packet(pstruControlPacket, 0);
	add_dest_to_packet(pstruControlPacket, nDestinationId);
	// Generate Timeout Event by adding ARP_RETRY_INTERVAL to EventTime
	pstruEventDetails->dEventTime = pstruEventDetails->dEventTime + (pstruArpVariables->nArpRetryInterval*1000000);		
	pstruEventDetails->nEventType = TIMER_EVENT;  //NETWORK_OUT_EVENT;		
	pstruEventDetails->nSubEventType = ARP_REQUEST_TIMEOUT;		
	pstruEventDetails->pPacket = pstruControlPacket;
	pstruEventDetails->nProtocolId = NW_PROTOCOL_ARP;			
	fnpAddEvent(pstruEventDetails);		
	pstruArpVariables->pnArpRetryCount[nDestinationId]+= 1; //Increment Retry count	
	pstruEventDetails->pPacket = NULL;		
	return 0;
}


