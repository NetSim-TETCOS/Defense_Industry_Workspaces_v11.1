/************************************************************************************
 * Copyright (C) 2014                                                               *
 * TETCOS, Bangalore. India                                                         *
 *                                                                                  *
 * Tetcos owns the intellectual property rights in the Product and its content.     *
 * The copying, redistribution, reselling or publication of any or all of the       *
 * Product or its content without express prior written consent of Tetcos is        *
 * prohibited. Ownership and / or any other right relating to the software and all  *
 * intellectual property rights therein shall remain at all times with Tetcos.      *
 *                                                                                  *
 * Author:    Shashi Kant Suman                                                     *
 *                                                                                  *
 * ---------------------------------------------------------------------------------*/
#include "main.h"
#include "LTE.h"
#include "RRC.h"

int fn_NetSim_LTE_InitRRCConnection()
{
	RRC_CONNECTION_REQUEST* request;
	LTE_MAC_PACKET* macPacket;
	LTE_PHY_PACKET* phyPacket;
	NetSim_PACKET* packet;
	NETSIM_ID UEId=pstruEventDetails->nDeviceId;
	NETSIM_ID UEInterface=pstruEventDetails->nInterfaceId;
	LTE_UE_MAC* UEMac=get_ue_mac(UEId,UEInterface);
	if(UEMac->rrcState != RRC_IDLE || UEMac->RRC_VAR.counter)
		return 0; //RRC is already connected or requested
	packet=fn_NetSim_LTE_CreateCtrlPacket(pstruEventDetails->dEventTime,
		LTEPacket_RRC_CONNECTION_REQUEST,
		UEMac->nENBId,
		UEId,
		UEMac->nENBId,
		RRC_CONNECTION_REQUEST_SIZE);
	macPacket=calloc(1,sizeof* macPacket);
	macPacket->transportChannel=TransportChannel_RACH;
	macPacket->logicalChannel=LogicalChannel_CCCH;
	packet->pstruMacData->Packet_MACProtocol=macPacket;
	phyPacket=PACKET_PHYPROTOCOLDATA(packet);
	phyPacket->physicalChannel=PhysicalChannel_PRACH;
	packet->pstruPhyData->Packet_PhyData=phyPacket;
	request=calloc(1,sizeof* request);
	request->InitialUEIdentity=UEId;
	request->MessageType=LTEPacket_RRC_CONNECTION_REQUEST;
	request->EstablishmentCause=mo_Data;
	macPacket->MessageType=LTEPacket_RRC_CONNECTION_REQUEST;
	macPacket->MessageVar=request;
	//Add physical out event
	pstruEventDetails->dPacketSize=RRC_CONNECTION_REQUEST_SIZE;
	pstruEventDetails->nApplicationId=0;
	pstruEventDetails->nEventType=PHYSICAL_OUT_EVENT;
	pstruEventDetails->nPacketId=0;
	pstruEventDetails->nSegmentId=0;
	pstruEventDetails->nSubEventType=0;
	pstruEventDetails->pPacket=packet;
	set_carrier_index(packet,0);
	pstruEventDetails->szOtherDetails=NULL;
	fnpAddEvent(pstruEventDetails);
	//Add T300 Timer
	pstruEventDetails->dEventTime += T300[0];
	pstruEventDetails->dPacketSize=0;
	pstruEventDetails->nEventType=TIMER_EVENT;
	pstruEventDetails->nPacketId=0;
	pstruEventDetails->nSubEventType=LTE_T300_Expire;
	pstruEventDetails->pPacket=NULL;
	pstruEventDetails->szOtherDetails=NULL;
	pstruEventDetails->nProtocolId=MAC_PROTOCOL_LTE;
	UEMac->RRC_VAR.nEventId=fnpAddEvent(pstruEventDetails);
	UEMac->RRC_VAR.counter++;
	return 1;
}
int fn_NetSim_LTE_T300_Expire()
{
	RRC_CONNECTION_REQUEST* request;
	LTE_MAC_PACKET* macPacket;
	LTE_PHY_PACKET* phyPacket;
	NetSim_PACKET* packet;
	NETSIM_ID UEId=pstruEventDetails->nDeviceId;
	NETSIM_ID UEInterface=pstruEventDetails->nInterfaceId;
	LTE_UE_MAC* UEMac=get_ue_mac(UEId,UEInterface);
	if(UEMac->rrcState==RRC_CONNECTED)
		return 0;
	if(T300[UEMac->RRC_VAR.counter]==2000*MILLISECOND)
	{
		UEMac->RRC_VAR.counter=0;
		UEMac->RRC_VAR.nEventId=0;
		return 0;//RRC connection fails
	}
	if(UEMac->RRC_VAR.nEventId!=pstruEventDetails->nEventId)
		return 0;//Different event
	packet=fn_NetSim_LTE_CreateCtrlPacket(pstruEventDetails->dEventTime,
		LTEPacket_RRC_CONNECTION_REQUEST,
		UEMac->nENBId,
		UEId,
		UEMac->nENBId,
		RRC_CONNECTION_REQUEST_SIZE);
	macPacket=calloc(1,sizeof* macPacket);
	macPacket->transportChannel=TransportChannel_RACH;
	macPacket->logicalChannel=LogicalChannel_CCCH;
	packet->pstruMacData->Packet_MACProtocol=macPacket;
	phyPacket=calloc(1,sizeof* phyPacket);
	phyPacket->physicalChannel=PhysicalChannel_PRACH;
	packet->pstruPhyData->Packet_PhyData=phyPacket;
	request=calloc(1,sizeof* request);
	request->InitialUEIdentity=UEId;
	request->MessageType=LTEPacket_RRC_CONNECTION_REQUEST;
	request->EstablishmentCause=mo_Data;
	macPacket->MessageType=LTEPacket_RRC_CONNECTION_REQUEST;
	macPacket->MessageVar=request;
	//Add physical out event
	pstruEventDetails->dPacketSize=RRC_CONNECTION_REQUEST_SIZE;
	pstruEventDetails->nApplicationId=0;
	pstruEventDetails->nEventType=PHYSICAL_OUT_EVENT;
	pstruEventDetails->nPacketId=0;
	pstruEventDetails->nSegmentId=0;
	pstruEventDetails->nSubEventType=0;
	pstruEventDetails->pPacket=packet;
	pstruEventDetails->szOtherDetails=NULL;
	fnpAddEvent(pstruEventDetails);
	//Add T300 Timer
	pstruEventDetails->dEventTime += T300[UEMac->RRC_VAR.counter++];
	pstruEventDetails->dPacketSize=0;
	pstruEventDetails->nEventType=TIMER_EVENT;
	pstruEventDetails->nPacketId=0;
	pstruEventDetails->nSubEventType=LTE_T300_Expire;
	pstruEventDetails->pPacket=NULL;
	pstruEventDetails->szOtherDetails=NULL;
	UEMac->RRC_VAR.nEventId=fnpAddEvent(pstruEventDetails);
	return 1;
}
int fn_NetSim_LTE_RRC_ProcessRequest()
{
	RRC_CONNECTION_REQUEST* request=(RRC_CONNECTION_REQUEST*)((LTE_MAC_PACKET*)(pstruEventDetails->pPacket->pstruMacData->Packet_MACProtocol))->MessageVar;
	RRC_CONNECTION_SETUP* setup;
	LTE_MAC_PACKET* macPacket;
	LTE_PHY_PACKET* phyPacket;
	NetSim_PACKET* packet;
	NETSIM_ID eNBId=pstruEventDetails->nDeviceId;
	packet=fn_NetSim_LTE_CreateCtrlPacket(pstruEventDetails->dEventTime,
		LTEPacket_RRC_CONNECTION_SETUP,
		request->InitialUEIdentity,
		eNBId,
		request->InitialUEIdentity,
		RRC_CONNECTION_SETUP_SIZE);
	macPacket=calloc(1,sizeof* macPacket);
	macPacket->transportChannel=TransportChannel_DLSCH;
	macPacket->logicalChannel=LogicalChannel_CCCH;
	packet->pstruMacData->Packet_MACProtocol=macPacket;
	phyPacket=PACKET_PHYPROTOCOLDATA(packet);
	phyPacket->physicalChannel=PhysicalChannel_PDCCH;
	packet->pstruPhyData->Packet_PhyData=phyPacket;
	setup=calloc(1,sizeof* setup);
	setup->InitialUEIdentity=request->InitialUEIdentity;
	setup->MessageType=LTEPacket_RRC_CONNECTION_SETUP;
	setup->TransactionIdentifier=1;
	macPacket->MessageType=LTEPacket_RRC_CONNECTION_SETUP;
	macPacket->MessageVar=setup;
	//Free Connection request packet
	fn_NetSim_Packet_FreePacket(pstruEventDetails->pPacket);
	//Add physical out event
	pstruEventDetails->dPacketSize=RRC_CONNECTION_SETUP_SIZE;
	pstruEventDetails->nApplicationId=0;
	pstruEventDetails->nEventType=PHYSICAL_OUT_EVENT;
	pstruEventDetails->nPacketId=0;
	pstruEventDetails->nSegmentId=0;
	pstruEventDetails->nSubEventType=0;
	pstruEventDetails->pPacket=packet;
	pstruEventDetails->szOtherDetails=NULL;
	fnpAddEvent(pstruEventDetails);
	return 1;
}
int fn_NetSim_LTE_RRC_ProcessSetup()
{
	RRC_CONNECTION_SETUP_COMPLETE* complete;
	RRC_CONNECTION_SETUP* setup=(RRC_CONNECTION_SETUP*)((LTE_MAC_PACKET*)pstruEventDetails->pPacket->pstruMacData->Packet_MACProtocol)->MessageVar;
	LTE_MAC_PACKET* macPacket;
	LTE_PHY_PACKET* phyPacket;
	NetSim_PACKET* packet;
	NETSIM_ID UEId=pstruEventDetails->nDeviceId;
	NETSIM_ID UEInterface=pstruEventDetails->nInterfaceId;
	LTE_UE_MAC* UEMac=get_ue_mac(UEId,UEInterface);

	//Change the mac state
	UEMac->rrcState=RRC_CONNECTED;
	UEMac->RRC_VAR.counter=0;
	UEMac->RRC_VAR.nEventId=0;

	//Transmit setup-complete message
	packet=fn_NetSim_LTE_CreateCtrlPacket(pstruEventDetails->dEventTime,
		LTEPacket_RRC_CONNECTION_SETUP_COMPLETE,
		UEMac->nENBId,
		UEId,
		UEMac->nENBId,
		RRC_CONNECTION_SETUP_COMPLETE_SIZE);
	macPacket=(LTE_MAC_PACKET*)calloc(1,sizeof* macPacket);
	macPacket->transportChannel=TransportChannel_DLSCH;
	macPacket->logicalChannel=LogicalChannel_DCCH;
	packet->pstruMacData->Packet_MACProtocol=macPacket;
	phyPacket=(LTE_PHY_PACKET*)PACKET_PHYPROTOCOLDATA(packet);
	phyPacket->physicalChannel=PhysicalChannel_PDSCH;
	packet->pstruPhyData->Packet_PhyData=phyPacket;
	complete=(RRC_CONNECTION_SETUP_COMPLETE*)calloc(1,sizeof* complete);
	complete->MessageType=LTEPacket_RRC_CONNECTION_SETUP_COMPLETE;
	setup->TransactionIdentifier=1;
	macPacket->MessageType=LTEPacket_RRC_CONNECTION_SETUP_COMPLETE;
	macPacket->MessageVar=complete;
	//Free Connection setup packet
	fn_NetSim_Packet_FreePacket(pstruEventDetails->pPacket);
	//Add physical out event
	pstruEventDetails->dPacketSize=RRC_CONNECTION_SETUP_COMPLETE_SIZE;
	pstruEventDetails->nApplicationId=0;
	pstruEventDetails->nEventType=PHYSICAL_OUT_EVENT;
	pstruEventDetails->nPacketId=0;
	pstruEventDetails->nSegmentId=0;
	pstruEventDetails->nSubEventType=0;
	pstruEventDetails->pPacket=packet;
	pstruEventDetails->szOtherDetails=NULL;
	fnpAddEvent(pstruEventDetails);
	return 1;
}
int fn_NetSim_LTE_RRC_SetupComplete()
{
	LTE_ENB_MAC* enbMac=get_enb_mac(pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId);
	NETSIM_ID ueId=pstruEventDetails->pPacket->nTransmitterId;
	LTE_ASSOCIATEUE_INFO* info=fn_NetSim_LTE_FindInfo(enbMac,ueId);
	if(info)
		info->rrcState=RRC_CONNECTED;
	else
	{
		fnNetSimError("LTE-RRC set up complete message received by unconnected eNB");
		assert(false);
	}
	//Free the packet
	fn_NetSim_Packet_FreePacket(pstruEventDetails->pPacket);
	return 0;
}
int fn_NetSim_Init_PagingRequest(NETSIM_ID enbId,NETSIM_ID enbInterface,LTE_ASSOCIATEUE_INFO* info)
{
	//Transmit paging message
	NetSim_PACKET* packet=fn_NetSim_LTE_CreateCtrlPacket(pstruEventDetails->dEventTime,
		LTEPacket_Paging,
		info->nUEId,
		enbId,
		info->nUEId,
		1);
	//Add physical out event
	pstruEventDetails->dPacketSize=1;
	pstruEventDetails->nApplicationId=0;
	pstruEventDetails->nEventType=PHYSICAL_OUT_EVENT;
	pstruEventDetails->nPacketId=0;
	pstruEventDetails->nSegmentId=0;
	pstruEventDetails->nSubEventType=0;
	pstruEventDetails->pPacket=packet;
	pstruEventDetails->szOtherDetails=NULL;
	fnpAddEvent(pstruEventDetails);
	return 1;
}
int fn_NetSim_LTE_ProcessPage()
{
	return fn_NetSim_LTE_InitRRCConnection();
}