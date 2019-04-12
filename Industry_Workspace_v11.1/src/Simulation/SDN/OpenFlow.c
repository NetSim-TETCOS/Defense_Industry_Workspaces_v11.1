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

static void openFlow_createSocketInterface(NETSIM_ID d);

//Event handler
static int fn_NetSim_OPEN_FLOW_HandleTimerEvent();
static int fn_NetSim_OPEN_FLOW_HandleAppInEvent();

/**
OPEN_FLOW Init function initializes the OPEN_FLOW parameters.
*/
_declspec (dllexport) int fn_NetSim_OPEN_FLOW_Init(struct stru_NetSim_Network *NETWORK_Formal,
											 NetSim_EVENTDETAILS *pstruEventDetails_Formal,
											 char *pszAppPath_Formal,
											 char *pszWritePath_Formal,
											 int nVersion_Type,
											 void **fnPointer)
{
	NETSIM_ID i;
	for (i = 0; i < NETWORK->nDeviceCount; i++)
	{
		if (isOPENFLOWConfigured(i + 1))
		{
			ptrOPENFLOW of = GET_OPENFLOW_VAR(i + 1);
			if (!isSDNController(i + 1))
			{
				of->sdnControllerId = fn_NetSim_Stack_GetDeviceId_asName(of->sdnControllerDev);
				if (!of->sdnControllerId)
				{
					fnNetSimError("%s device is not configured as SDN controller.\n"
								  "Disabling OPEN_FLOW protocol in device %d.\n",
								  of->sdnControllerDev,
								  DEVICE_CONFIGID(i + 1));
					of->isSDNConfigured = false;
				}
			}
		}
	}

	for (i = 0; i < NETWORK->nDeviceCount; i++)
	{
		if (isOPENFLOWConfigured(i + 1))
		{
			ptrOPENFLOW of = GET_OPENFLOW_VAR(i + 1);
			of->myIP = openFlow_find_client_IP(i + 1);
			if (isSDNController(i + 1))
			{
			}
			else
			{
				of->INFO.controllerInfo = calloc(1, sizeof* of->INFO.controllerInfo);
				openFlow_createSocketInterface(i + 1);
			}
		}
	}
	return 0;
}

/**
This function is called by NetworkStack.dll, whenever the event gets triggered
inside the NetworkStack.dll for the OPEN_FLOW protocol
*/
_declspec (dllexport) int fn_NetSim_OPEN_FLOW_Run()
{
	switch (pstruEventDetails->nEventType)
	{
		case TIMER_EVENT:
			return fn_NetSim_OPEN_FLOW_HandleTimerEvent();
			break;
		case APPLICATION_IN_EVENT:
			return fn_NetSim_OPEN_FLOW_HandleAppInEvent();
		default:
			fnNetSimError("%d event is not for Open flow protocol\n", pstruEventDetails->nEventType);
			break;
	}
	return 0;
}

/**
This function is called by NetworkStack.dll, once simulation end to free the
allocated memory for the network.
*/
_declspec(dllexport) int fn_NetSim_OPEN_FLOW_Finish()
{
	return 0;
	//return fn_NetSim_OPEN_FLOW_Finish_F();
}

/**
This function is called by NetworkStack.dll, while writing the event trace
to get the sub event as a string.
*/
_declspec (dllexport) char* fn_NetSim_OPEN_FLOW_Trace(NETSIM_ID nSubEvent)
{
	return "";
	//return GetStringOPEN_FLOW_Subevent(nSubEvent);
}

/**
This function is called by NetworkStack.dll, while configuring the device
for Open flow protocol.
*/
_declspec(dllexport) int fn_NetSim_OPEN_FLOW_Configure(void** var)
{
	char* tag;
	void* xmlNetSimNode;
	NETSIM_ID nDeviceId;
	NETSIM_ID nInterfaceId;
	LAYER_TYPE nLayerType;

	tag = (char*)var[0];
	xmlNetSimNode = var[2];
	if (!strcmp(tag, "PROTOCOL_PROPERTY"))
	{
		NETWORK = (struct stru_NetSim_Network*)var[1];
		nDeviceId = *((NETSIM_ID*)var[3]);
		nInterfaceId = *((NETSIM_ID*)var[4]);
		nLayerType = *((LAYER_TYPE*)var[5]);

		char* szVal;
		ptrOPENFLOW of;
		of = GET_OPENFLOW_VAR(nDeviceId);
		if (!of)
		{
			of = calloc(1, sizeof* of);
			SET_OPENFLOW_VAR(nDeviceId, of);
		}
		getXmlVar(&of->isSDNConfigured, OPEN_FLOW_ENABLE, xmlNetSimNode, 1, _BOOL, OPENFLOW);
		getXmlVar(&of->isSDNController, SDN_CONTROLLER, xmlNetSimNode, 1, _BOOL, OPENFLOW);
		if(!of->isSDNController)
			getXmlVar(&of->sdnControllerDev, SDN_CONTROLLER_DEVICE, xmlNetSimNode, 1, _STRING, OPENFLOW);
			
	}
	return 0;
}

/**
This function is called by NetworkStack.dll, to free the OPEN_FLOW protocol data.
*/
_declspec(dllexport) int fn_NetSim_OPEN_FLOW_FreePacket(NetSim_PACKET* pstruPacket)
{
	return 0;
	//return fn_NetSim_OPEN_FLOW_FreePacket_F(pstruPacket);
}

/**
This function is called by NetworkStack.dll, to copy the OPEN_FLOW protocol
details from source packet to destination.
*/
_declspec(dllexport) int fn_NetSim_OPEN_FLOW_CopyPacket(NetSim_PACKET* pstruDestPacket, NetSim_PACKET* pstruSrcPacket)
{
	return 0;
	//return fn_NetSim_OPEN_FLOW_CopyPacket_F(pstruDestPacket, pstruSrcPacket);
}

/**
This function write the Metrics
*/
_declspec(dllexport) int fn_NetSim_OPEN_FLOW_Metrics(PMETRICSWRITER metricsWriter)
{
	return 0;
}

/**
This function will return the string to write packet trace heading.
*/
_declspec(dllexport) char* fn_NetSim_OPEN_FLOW_ConfigPacketTrace()
{
	return "";
}

/**
This function will return the string to write packet trace.
*/
_declspec(dllexport) char* fn_NetSim_OPEN_FLOW_WritePacketTrace(NetSim_PACKET* pstruPacket, char** ppszTrace)
{
	return "";
}

bool isOPENFLOWConfigured(NETSIM_ID d)
{
	ptrOPENFLOW of = GET_OPENFLOW_VAR(d);
	if (of)
	{
		return of->isSDNConfigured;
	}
	return false;
}

static int fn_NetSim_OPEN_FLOW_HandleTimerEvent()
{
	switch (pstruEventDetails->nSubEventType)
	{
		case 0:
			handle_cli_input();
			break;
		default:
			fnNetSimError("%d subevent is not for Open flow protocol\n", pstruEventDetails->nSubEventType);
			break;
	}
	return 0;
}

static void openFlow_createSocketInterface(NETSIM_ID d)
{
	ptrOPENFLOW of = GET_OPENFLOW_VAR(d);

	UINT16 port = (UINT16)(NETSIM_RAND_01() * 65535);
	of->INFO.controllerInfo->port = port;
	of->INFO.controllerInfo->controllerId = of->sdnControllerId;
	of->INFO.controllerInfo->controllerIP = openFlow_find_client_IP(of->sdnControllerId);
	of->INFO.controllerInfo->sock = fn_NetSim_Socket_CreateNewSocket(d,
																	 APP_PROTOCOL_OPENFLOW,
																	 TX_PROTOCOL_TCP,
																	 port,
																	 OFMSG_PORT);

	openFlow_add_new_client(of->sdnControllerId,
							d,
							port);
}

NETSIM_IPAddress openFlow_find_client_IP(NETSIM_ID d)
{
	NETSIM_ID i;
	for (i = 0; i < DEVICE(d)->nNumOfInterface; i++)
	{
		if (DEVICE_INTERFACE(d, i + 1)->nInterfaceType == INTERFACE_WAN_ROUTER)
			return DEVICE_NWADDRESS(d, i + 1);
	}
	return DEVICE_NWADDRESS(d, 1);
}

static int fn_NetSim_OPEN_FLOW_HandleAppInEvent()
{
	NetSim_PACKET* packet = pstruEventDetails->pPacket;
	if (!(packet->pstruAppData->nPacketFragment == Segment_Unfragment ||
		  packet->pstruAppData->nPacketFragment == Segment_FirstSegment))
	{
		fn_NetSim_Packet_FreePacket(packet);
		return -1;
	}

	switch (packet->nControlDataType)
	{
		case OFMSGTYPE_COMMAND:
		{
			ptrOPENFLOWMSG msg = packet->pstruAppData->Packet_AppProtocol;
			int len = 0;
			bool isMore = false;
			char* ret = openFlow_execute_command(msg->payload,
												 pstruEventDetails->nDeviceId,
												 msg->id,
												 &len,
												 &isMore);
			if(len)
				openFlow_send_response(pstruEventDetails->nDeviceId,
									   msg->id,
									   ret,
									   len,
									   isMore);
			fn_NetSim_Packet_FreePacket(packet);
		}
		break;
		case OFMSGTYPE_RESPONSE:
		{
			ptrOPENFLOWMSG msg = packet->pstruAppData->Packet_AppProtocol;
			openFlow_print_response(pstruEventDetails->nDeviceId,
									msg->id,
									msg->payload,
									msg->len,
									msg->isMore);
			fn_NetSim_Packet_FreePacket(packet);
		}
		break;
		default:
			fnNetSimError("Unknown packet type %d arrives in APP_IN event of OPEN_FLOW protocol.",
						  packet->nControlDataType);
			break;
	}
	return 0;
}
