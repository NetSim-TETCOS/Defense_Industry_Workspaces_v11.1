#pragma once
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

#ifndef _NETSIM_SDN_H_
#define _NETSIM_SDN_H_
#ifdef  __cplusplus
extern "C" {
#endif

#pragma comment(lib,"NetworkStack.lib")

	//Default config parameter
#define OPENFLOW_SDN_CONTROLLER_DEFAULT			false
#define OPENFLOW_OPEN_FLOW_ENABLE_DEFAULT		false
#define OPENFLOW_SDN_CONTROLLER_DEVICE_DEFAULT	_strdup("unknown")

	typedef struct stru_clientInfo
	{
		NETSIM_ID clientId;
		UINT16 clientPort;
		NETSIM_IPAddress clientIP;
		ptrSOCKETINTERFACE sock;
		struct stru_clientInfo* next;
	}SDNCLIENTINFO,*ptrSDNCLIENTINFO;

	typedef struct stru_controllerInfo
	{
		NETSIM_ID controllerId;
		NETSIM_IPAddress controllerIP;
		UINT16 port;
		ptrSOCKETINTERFACE sock;
	}SDNCONTROLLERINFO, *ptrSDNCONTROLLERINFO;

	typedef struct stru_HandleInfo
	{
		UINT id;
		void* handle;
		struct stru_HandleInfo* next;
	}HANDLEINFO, *ptrHANDLEINFO;

	typedef struct stru_openflow
	{
		bool isSDNConfigured;
		bool isSDNController;

		NETSIM_IPAddress myIP;

		char* sdnControllerDev;
		NETSIM_ID sdnControllerId;

		union sdn_info
		{
			ptrSDNCONTROLLERINFO controllerInfo;
			ptrSDNCLIENTINFO clientInfo;
		}INFO;

		UINT msgId;
		ptrHANDLEINFO handleInfo;
	}OPENFLOW, *ptrOPENFLOW;
#define GET_OPENFLOW_VAR(d) ((ptrOPENFLOW)(DEVICE_APPVAR(d,APP_PROTOCOL_OPENFLOW)))
#define SET_OPENFLOW_VAR(d,var) (fn_NetSim_Stack_SetAppProtocolData(d,APP_PROTOCOL_OPENFLOW,var))

#define OFMSG_OVERHEAD 50
#define OFMSG_PORT	99
	typedef enum enumOpenFlowMsgType
	{
		OFMSGTYPE_COMMAND = APP_PROTOCOL_OPENFLOW * 100 + 1,
		OFMSGTYPE_RESPONSE,
	}OFMSGTYPE;

	typedef struct stru_openFlowMsg
	{
		UINT id;
		OFMSGTYPE msgTYpe;
		void* payload;
		int len;
		bool isMore;
	}OPENFLOWMSG, *ptrOPENFLOWMSG;

	//Open flow utility function
	bool isOPENFLOWConfigured(NETSIM_ID d);
	NETSIM_IPAddress openFlow_find_client_IP(NETSIM_ID d);

	//CLI Interface
	void handle_cli_input();
	char* openFlow_execute_command(void* handle, NETSIM_ID d, UINT id, int* len, bool* isMore);
	void openFlow_saveHandle(NETSIM_ID d,
							 UINT id,
							 void* handle);
	ptrHANDLEINFO openFlow_getHandle(NETSIM_ID d,
									 UINT id);

	//SDN Controller
	bool isSDNController(NETSIM_ID d);
	void openFlow_add_new_client(NETSIM_ID ct,
								 NETSIM_ID ci,
								 UINT16 port);
	ptrSDNCLIENTINFO openFlow_find_clientInfo(NETSIM_ID ct, NETSIM_ID ci);

	//Open flow msg
	bool fn_NetSim_OPEN_FLOW_SendCommand(NETSIM_ID dest,
										 void* handle,
										 void* command,
										 UINT len);
	void openFlow_send_response(NETSIM_ID dev,
								UINT id,
								char* payload,
								int len,
								bool isMore);
	void openFlow_print_response(NETSIM_ID d,
								 UINT id,
								 void* msg,
								 int len,
								 bool isMore);

#ifdef  __cplusplus
}
#endif
#endif