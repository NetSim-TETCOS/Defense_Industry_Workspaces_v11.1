/************************************************************************************
* Copyright (C) 2018                                                               *
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
#include "OpenFlow.h"

static NetSim_PACKET* openFlow_CreatePacket(OFMSGTYPE type,
											UINT id,
											NETSIM_ID src,
											NETSIM_ID dest,
											NETSIM_IPAddress srcIP,
											NETSIM_IPAddress destIP,
											UINT16 srcPort,
											UINT16 destPort,
											void* payload,
											int len,
											bool isMore)
{
	NetSim_PACKET* packet = fn_NetSim_Packet_CreatePacket(APPLICATION_LAYER);
	packet->dEventTime = pstruEventDetails->dEventTime;
	packet->nControlDataType = type;
	packet->nPacketPriority = Priority_Normal;
	packet->nPacketStatus = PacketStatus_NoError;
	packet->nPacketType = PacketType_Control;
	packet->nQOS = QOS_BE;
	packet->nServiceType = ServiceType_NULL;
	packet->nSourceId = src;
	packet->nTransmitterId = src;

	packet->pstruAppData->dArrivalTime = pstruEventDetails->dEventTime;
	packet->pstruAppData->dEndTime = pstruEventDetails->dEventTime;
	packet->pstruAppData->dStartTime = pstruEventDetails->dEventTime;
	packet->pstruAppData->dPayload = len;
	packet->pstruAppData->dOverhead = OFMSG_OVERHEAD;
	packet->pstruAppData->dPacketSize = packet->pstruAppData->dPayload +
		packet->pstruAppData->dOverhead;
	packet->pstruAppData->nApplicationProtocol = APP_PROTOCOL_OPENFLOW;
	packet->pstruAppData->nPacketFragment = Segment_Unfragment;

	add_dest_to_packet(packet, dest);

	packet->pstruTransportData->nSourcePort = srcPort;
	packet->pstruTransportData->nDestinationPort = destPort;

	packet->pstruNetworkData->szDestIP = destIP;
	packet->pstruNetworkData->szSourceIP = srcIP;
	packet->pstruNetworkData->nTTL = MAX_TTL;


	ptrOPENFLOWMSG ofmsg = calloc(1, sizeof* ofmsg);
	ofmsg->payload = payload;
	ofmsg->id = id;
	ofmsg->len = len;
	ofmsg->msgTYpe = OFMSGTYPE_COMMAND;
	ofmsg->isMore = isMore;

	packet->pstruAppData->Packet_AppProtocol = ofmsg;

	return packet;
}

static void openFlow_send_packet(NETSIM_ID d,
								 NetSim_PACKET* packet,
								 ptrSOCKETINTERFACE sock)
{
	if (!DEVICE(d)->pstruTransportLayer->isTCP)
	{
		fnNetSimError("TCP is disabled for device %d.\n"
					  "Open flow protocol will run only over TCP.",
					  DEVICE_CONFIGID(d));
		fn_NetSim_Packet_FreePacket(packet);
		return;
	}
	
	NETSIM_ID dest = get_first_dest_from_packet(packet);
	if (!DEVICE(dest)->pstruTransportLayer->isTCP)
	{
		fnNetSimError("TCP is disabled for device %d.\n"
					  "Open flow protocol will run only over TCP.",
					  DEVICE_CONFIGID(dest));
		fn_NetSim_Packet_FreePacket(packet);
		return;
	}

	int nSegmentCount=fn_NetSim_Stack_FragmentPacket(packet,(int)fn_NetSim_Stack_GetMSS(packet));
	fn_NetSim_Socket_PassPacketToInterface(d, packet, sock);
	NetSim_EVENTDETAILS pevent;
	memset(&pevent, 0, sizeof pevent);
	pevent.dEventTime = pstruEventDetails->dEventTime;
	pevent.dPacketSize = packet->pstruAppData->dPacketSize;
	pevent.nDeviceId = d;
	pevent.nDeviceType = DEVICE_TYPE(d);
	pevent.nEventType = TRANSPORT_OUT_EVENT;
	pevent.nProtocolId = TX_PROTOCOL_TCP;
	pevent.pPacket = packet;
	pevent.szOtherDetails = sock;
	fnpAddEvent(&pevent);
}

bool fn_NetSim_OPEN_FLOW_SendCommand(NETSIM_ID dest,
									 void* handle,
									 void* command,
									 UINT len)
{
	ptrOPENFLOW of = GET_OPENFLOW_VAR(pstruEventDetails->nDeviceId);
	ptrSDNCLIENTINFO sd = openFlow_find_clientInfo(pstruEventDetails->nDeviceId, dest);
	if (!sd)
	{
		fnNetSimError("%d is not a SDN controller for device %d.\n",
					  DEVICE_CONFIGID(pstruEventDetails->nDeviceId),
					  DEVICE_CONFIGID(dest));
		return false;
	}

	NetSim_PACKET* packet = openFlow_CreatePacket(OFMSGTYPE_COMMAND,
												  ++of->msgId,
												  pstruEventDetails->nDeviceId,
												  sd->clientId,
												  of->myIP,
												  sd->clientIP,
												  OFMSG_PORT,
												  sd->clientPort,
												  command,
												  len,
												  false);

	openFlow_saveHandle(pstruEventDetails->nDeviceId, of->msgId, handle);
	openFlow_send_packet(pstruEventDetails->nDeviceId,
						 packet,
						 sd->sock);
	
	return true;
}

void openFlow_send_response(NETSIM_ID dev,
							UINT id,
							char* payload,
							int len,
							bool isMore)
{
	ptrOPENFLOW of = GET_OPENFLOW_VAR(dev);
	NetSim_PACKET* response = openFlow_CreatePacket(OFMSGTYPE_RESPONSE,
													id,
													dev,
													of->INFO.controllerInfo->controllerId,
													of->myIP,
													of->INFO.controllerInfo->controllerIP,
													of->INFO.controllerInfo->port,
													OFMSG_PORT,
													payload,
													len,
													isMore);
	openFlow_send_packet(dev,
						 response,
						 of->INFO.controllerInfo->sock);
}