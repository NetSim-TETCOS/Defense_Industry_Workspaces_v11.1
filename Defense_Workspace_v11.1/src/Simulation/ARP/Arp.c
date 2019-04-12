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
 * Date  :    23-April-2012
 ************************************************************************************/
#define _CRT_SECURE_NO_DEPRECATE
#include "main.h"
#include "ARP.h"
/**
	This function is called by NetworkStack.dll, whenever the event gets triggered		
	inside the NetworkStack.dll for the ARP protocol									
*/
_declspec (dllexport) int fn_NetSim_ARP_Run()
{
	SUB_EVENT nSub_Event_Type;
	int nDestionation;		
	/* Get the Event and subevent Type from Event details */
	nEventType=pstruEventDetails->nEventType;
	nSub_Event_Type = pstruEventDetails->nSubEventType;
	
	switch(nEventType)	//Check  event type
	{			
	case NETWORK_OUT_EVENT:
			switch(nSub_Event_Type)		//Check the subevent type
			{			
			case GENERATE_ARP_REQUEST: // Generate ARP Request
				fn_NetSim_Generate_ARP_Request(pstruEventDetails,NETWORK);
				break;
			default:					 // READ_ARP_TABLE:		
				fn_NetSim_Read_ARP_Table(pstruEventDetails,NETWORK);				
				break;
			}
		break;

	case TIMER_EVENT:
		switch(nSub_Event_Type)
		{
		case ARP_REQUEST_TIMEOUT:// Request Time out event
			fn_NetSim_ARP_Request_Timeout(pstruEventDetails,NETWORK);
			break;
		}
		break;

	case NETWORK_IN_EVENT:
		switch(nSub_Event_Type)
		{		
		case GENERATE_ARP_REPLY: // To generate ARP Reply

			fn_NetSim_Generate_ARP_Reply(pstruEventDetails,NETWORK);
			break;
		case UPDATE_ARP_TABLE_FWD_PKT: // To Forward the buffered packet
			fn_NetSim_Update_ARP_Table_ForwardPacket(pstruEventDetails,NETWORK);
			break;		
		default: 
			switch(pstruEventDetails->pPacket->nControlDataType)//Check the Controlpacket type
			{
			case REQUEST_PACKET: // Add this subevent to generate reply packet
				nDestionation = fn_Netsim_ARP_CheckDestinationDevice(pstruEventDetails,NETWORK);
				if(nDestionation) // Destined device only generate reply
				{
					pstruEventDetails->nSubEventType = GENERATE_ARP_REPLY;
					fnpAddEvent(pstruEventDetails);					
				}
				pstruEventDetails->pPacket = NULL;
				break;
			case REPLY_PACKET: // Add this subevent to forward the buffered packet
				pstruEventDetails->nSubEventType = UPDATE_ARP_TABLE_FWD_PKT;
				fnpAddEvent(pstruEventDetails);
				pstruEventDetails->pPacket = NULL;
				break;
			default: // For other than ARP control packet.
				//Do nothing.
				break;
			}		
			break;
		}
		break;			
	default:
		fnNetSimError("Unknown event type for ARP protocol");
		break;
	}	

	return 0;
}		
/**
 The Following functions are present in the Arp.lib.							
 so NetworkStack.dll can not call Arp.lib. It is calling Arp.dll.				
 From Arp.dll, Arp.lib functions are called.									
*/
_declspec (dllexport) int fn_NetSim_ARP_Init(struct stru_NetSim_Network *NETWORK_Formal,\
 NetSim_EVENTDETAILS *pstruEventDetails_Formal,char *pszAppPath_Formal,\
 char *pszWritePath_Formal,int nVersion_Type,void **fnPointer)
{
	fn_NetSim_ARP_Init_F(NETWORK_Formal,pstruEventDetails_Formal,pszAppPath_Formal,\
		pszWritePath_Formal,nVersion_Type,fnPointer);
	return 0;
}
/**
	This function is called by NetworkStack.dll, once simulation end to free the 
	allocated memory for the network.	
*/
_declspec(dllexport) int fn_NetSim_ARP_Finish()
{
		fn_NetSim_ARP_Finish_F();
		return 0;
}	
/**
	This function is called by NetworkStack.dll, while writing the evnt trace 
	to get the sub event as a string.
*/
_declspec (dllexport) char *fn_NetSim_ARP_Trace(int nSubEvent)
{
	return (fn_NetSim_ARP_Trace_F(nSubEvent));
}
/**
	This function is called by NetworkStack.dll, while configuring the device 
	for ARP protocol.	
*/
_declspec(dllexport) int fn_NetSim_ARP_Configure(void** var)
{
	return fn_NetSim_ARP_Configure_F(var);
}
/**
	This function is called by NetworkStack.dll, to free the ARP protocol.
*/
_declspec(dllexport) int fn_NetSim_ARP_FreePacket(NetSim_PACKET* pstruPacket)
{
	return fn_NetSim_ARP_FreePacket_F(pstruPacket);	
}
/**
	This function is called by NetworkStack.dll, to copy the ARP protocol
	details from source packet to destination.
*/
_declspec(dllexport) int fn_NetSim_ARP_CopyPacket(NetSim_PACKET* pstruDestPacket,NetSim_PACKET* pstruSrcPacket)
{
	return fn_NetSim_ARP_CopyPacket_F(pstruDestPacket,pstruSrcPacket);	
}
/**
This function write the Metrics 	
*/
_declspec(dllexport) int fn_NetSim_ARP_Metrics(PMETRICSWRITER writer)
{
	return 0;
}
/**
This function will return the string to write packet trace heading.
*/
_declspec(dllexport) char* fn_NetSim_ARP_ConfigPacketTrace()
{
	return "";
}
/**
 This function will return the string to write packet trace.																									
*/
_declspec(dllexport) char* fn_NetSim_ARP_WritePacketTrace(NetSim_PACKET* pstruPacket, char** ppszTrace)
{
	return "NULL";
}



/**
	This function is called for NETWORK_IN_EVENT with packet type = REQUEST_PACKET.
	Check the request packet received device is the destination. If so it add the 
	GENERATE ARP REPLY sub event, else update the table and drop the request packet.										
*/
int fn_Netsim_ARP_CheckDestinationDevice(NetSim_EVENTDETAILS *pstruEventDetails, struct stru_NetSim_Network *NETWORK)
{
	NETSIM_ID nDeviceId;
	NETSIM_ID nInterfaceId;		
	int nMerge_flag = 0;
	int nType;
	NETSIM_IPAddress szSrcIPadd;	
	NetSim_PACKET *pstruTemp_Data;	// To store the packet from the event details	
	ARP_PACKET *pstruArpRequestPkt;	// ARP PACKET to store ARP pcaket data 		
	ARP_TABLE  *pstruTableHead,*pstruCurrentTable;	//Type casting Arp Table and variables 
	ARP_VARIABLES *pstruArpVariables; //Type casting Arp and variables 

	//Get the packet from the event 
	pstruTemp_Data =pstruEventDetails->pPacket;	
	// Get the device and interface Id from the event
	nInterfaceId = pstruEventDetails->nInterfaceId;
	nDeviceId = pstruEventDetails->nDeviceId;
	// Get the Arp Packet from the NetworkData of temp packet
	pstruArpRequestPkt = pstruTemp_Data->pstruNetworkData->Packet_NetworkProtocol;
	// check for destination, if so Add the GENERATE ARP REPLY subevent, else drop the packet	
	if(IP_COMPARE(pstruArpRequestPkt->sz_ar$tpa,DEVICE_NWADDRESS(nDeviceId,nInterfaceId)) == 0)
	{		
		return 1;
	}
	else // Update ARP table and drop the packet
	{
		// Get the Arp variables from device ipVar
		pstruArpVariables = DEVICE_INTERFACE(nDeviceId,nInterfaceId)->ipVar;	
		pstruTableHead = pstruArpVariables->pstruArpTable;	
		pstruCurrentTable = pstruTableHead;
		// Get the source IP address from the packet
		szSrcIPadd = IP_COPY(pstruArpRequestPkt->sz_ar$spa);
		// Check the Source entry in the ARP TABLE 
		while(pstruCurrentTable != NULL)
		{
			if(IP_COMPARE(pstruCurrentTable->szIPAddress,szSrcIPadd)== 0)
			{	// set the merge flag if entry exists
				nMerge_flag = 1;			
				break;
			}
			else // 
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
		IP_FREE(szSrcIPadd);
		//Drop the packet		
		fn_NetSim_Packet_FreePacket(pstruTemp_Data);
		pstruTemp_Data = NULL;
		pstruEventDetails->pPacket = NULL;			
		return 0;
	}	
}


