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
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Function: ARP Request Timeout 
When we generate Arp Request, we will add the Timeout event with the present 
event time + ARP_RETRY_INTERVAL. At that time this function is called.
Do the following to generate Request
1. Check the destination entry present in the table and set nDestinationPresentFlag. 
2. if nDestinationPresentFlag =0 Check RetryCont<RetryLimit.
3. if true generate ARP request, reset pnArpRequestFlag, increament nArpRequestSentCount.
4. else drop the buffered packets and reset the RetryCount and RequestFlag.
3. if nDestinationPresentFlag set,drop temp packet and reset RetryCount and RequestFlag
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/
int fn_NetSim_ARP_Request_Timeout(NetSim_EVENTDETAILS *pstruEventDetails, struct stru_NetSim_Network *NETWORK)
{
	NETSIM_ID nDeviceId;
	NETSIM_ID nInterfaceId;
	NETSIM_ID nDestinationId,i;
	NETSIM_IPAddress szDestIPadd;
	
	int nPktDropCount=0;
	int nDestinationPresentFlag = 0;
	
	NetSim_PACKET *pstruTemp_Data;		// store the packet from the event details	
	ARP_VARIABLES *pstruArpVariables;	//type casting Arp variables
	ARP_BUFFER *pstruPacketBuffer;		//type casting Arp buffer
	ARP_TABLE  *pstruTableHead, *pstruCurrentTable; //Type casting ARP table 
	
	pstruTemp_Data = pstruEventDetails->pPacket;	//Get the packet from the event 	
	nDeviceId = pstruEventDetails->nDeviceId;		// Get the deviceID
	nInterfaceId = pstruEventDetails->nInterfaceId;	// Get the  interfaceId 
	
	
	//Shashi kant
	szDestIPadd = IP_COPY(pstruTemp_Data->pstruNetworkData->szNextHopIp);
	// Get the source and destination IP address from the packet
	//szSrcIPadd = IP_COPY(pstruTemp_Data->pstruNetworkData->szSourceIP);
	//szDestIPadd = IP_COPY(pstruTemp_Data->pstruNetworkData->szDestIP);
	//szNextHopIp =  pstruTemp_Data->pstruNetworkData->szNextHopIp;
	//if(IP_COMPARE(szSrcIPadd,DEVICE_INTERFACE(nDeviceId,nInterfaceId)->szAddress) != 0)
	//{
	//	szSrcIPadd = IP_COPY(DEVICE_INTERFACE(nDeviceId,nInterfaceId)->szAddress);
	//}
	///*if(szNextHopIp)
	//	szDestIPadd = szNextHopIp;*/
	//szSubnetMaskAdd = IP_COPY(DEVICE_INTERFACE(nDeviceId,nInterfaceId)->szSubnetMask);
	//// Call the IPV4 function to check Destination MAC in the same subnet/LAN
	//nCheckSubnet = IP_IS_IN_SAME_NETWORK(szSrcIPadd,szDestIPadd,szSubnetMaskAdd);
	//// Get the Arp variables from device ipVar
	pstruArpVariables = DEVICE_INTERFACE(nDeviceId,nInterfaceId)->ipVar;
	pstruTableHead = pstruArpVariables->pstruArpTable;
	//	switch(nCheckSubnet)
	//	{
	//	case 1: // Destination in the same LAN		
	//		nDestinationId = pstruEventDetails->pPacket->nDestinationId;
	//		//szDestIPadd = _strdup(pstruTemp_Data->pstruNetworkData->szDestIP);
	//		break;
	//	case 2:// Destination in the different LAN
	//		nDestinationId = 0;			// for the Default Gateway
	//		szDestIPadd = IP_COPY(DEVICE_INTERFACE(nDeviceId,nInterfaceId)->szDefaultGateWay);			
	//		break;
	//	default:			
	//		fnNetSimError("Invalid Data From IPV4 in Read ARP Table function");
	//		return 0;
	//		break;
	//	}//switch end





		// Check the destination entry in the table
		pstruCurrentTable = pstruTableHead; 		
		while(pstruCurrentTable != 0) 
		{
			if(IP_COMPARE(pstruCurrentTable->szIPAddress,szDestIPadd)== 0)
			{
				nDestinationPresentFlag = 1; //Entry exist in the table
				break;
			}
			else
				pstruCurrentTable = pstruCurrentTable->pstruNextEntry;
		}		
		nDestinationId =  fn_NetSim_Stack_GetDeviceId_asIP(pstruEventDetails->pPacket->pstruNetworkData->szNextHopIp,&i);
	if(!nDestinationPresentFlag)
	{
		
		// If RetryCont<RetryLimit, generate ARP request, else drop the buffered packets	
		if(pstruArpVariables->pnArpRetryCount[nDestinationId] < pstruArpVariables->nArpRetryLimit)
		{	// Write to Log file	
			printf("\n%s\n","ARP---Reply not received with in the ARP_RETRY_INTERVAL, send request once again");			
			pstruArpVariables->pnArpRequestFlag[nDestinationId] = 0; // Reset the flag
			pstruEventDetails->nSubEventType = GENERATE_ARP_REQUEST;
			fnpAddEvent(pstruEventDetails);				
		}
		else 
		{	// Drop the buffered packets
			char ipstr[_NETSIM_IP_LEN];
			pstruPacketBuffer = pstruArpVariables->pstruPacketBuffer;
			IP_TO_STR(pstruPacketBuffer->pstruPacket->pstruNetworkData->szGatewayIP,ipstr);
			nPktDropCount = 0;
			fn_NetSim_Arp_Drop_Buffered_Packet(nDeviceId,nInterfaceId,szDestIPadd,&nPktDropCount);
			pstruPacketBuffer = pstruArpVariables->pstruPacketBuffer;
			// update the packet count in the buffer				
			pstruArpVariables->pstruArpMetrics->nPacketDropCount+= nPktDropCount;
			pstruArpVariables->pstruArpMetrics->nPacketsInBuffer-= nPktDropCount;
			// Reset the RetryCount and RequestFlag
			pstruArpVariables->pnArpRetryCount[nDestinationId] = 0;
			pstruArpVariables->pnArpRequestFlag[nDestinationId] = 0;
			
			// Write to Log file	
			printf("\n%s\n","ARP---Request reached the ARP_RETRY_LIMIT, drop the packet");
			printf("ARP---Packets Dropped at\tEventTime:%0.3lf\tSrcIP:%s\tDropCount:%d\n",pstruEventDetails->dEventTime,ipstr,nPktDropCount);

		}		
	}
	else
	{	// Reset the RetryCount and RequestFlag
		pstruArpVariables->pnArpRetryCount[nDestinationId] = 0;
		pstruArpVariables->pnArpRequestFlag[nDestinationId] = 0;
		// free the packet
		fn_NetSim_Packet_FreePacket(pstruTemp_Data);
		pstruEventDetails->pPacket = NULL;
	}


	//shashi kant
	/*IP_FREE(szSubnetMaskAdd);
	IP_FREE(szSrcIPadd);
	IP_FREE(szDestIPadd);	*/
	return 0;	
}
