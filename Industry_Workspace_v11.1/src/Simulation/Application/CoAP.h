/************************************************************************************
* Copyright (C) 2017                                                               *
* TETCOS, Bangalore. India                                                         *
*                                                                                  *
* Tetcos owns the intellectual property rights in the Product and its content.     *
* The copying, redistribution, reselling or publication of any or all of the       *
* Product or its content without express prior written consent of Tetcos is        *
* prohibited. Ownership and / or any other right relating to the software and all  *
* intellectual property rights therein shall remain at all times with Tetcos.      *
*                                                                                  *
* Author:    Siddharth Makadia                                                     *
*                                                                                  *
* ---------------------------------------------------------------------------------*/

#include "main.h"
#include "Stack.h"

//CoAP Derived Protocol Parameters default values (in seconds)
#define MAX_TRANSMIT_SPAN		45	
#define MAX_TRANSMIT_WAIT		93
#define MAX_LATENCY				100
#define PROCESSING_DELAY		2
#define MAX_RTT					202
#define EXCHANGE_LIFETIME		427
#define NON_LIFETIME			145


#define MESSAGE_SIZE_UPPER_BOUND 1152 //BYTES
#define PAYLOAD_SIZE_UPPER_BOUND 1152 //BYTES
typedef struct stru_NetSim_COAP_Header COAP_Header;
typedef struct stru_NetSim_COAP_Options COAP_Options;

typedef struct stru_NetSim_COAP_data COAP_Data;


/*
A summary of the contents of the CoAP header as per RFC 7252 follows:

		0                   1                   2                   3
		0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		|Ver| T |  TKL  |      Code     |          Message ID           |
		+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		|   Token (if any, TKL bytes) ...
		+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		|   Options (if any) ...
		+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		|1 1 1 1 1 1 1 1|    Payload (if any) ...
		+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

*/
	
typedef enum
{
	CONFIRMABLE,
	NON_CONFIRMABLE,
	ACKNOWLEDGEMENT,
	RESET,
}COAP_TYPE;

typedef enum
{
	REQUEST = 0,
	RESPONSE  = 2,
	CLIENT_ERROR = 4,
	SERVER_ERROR = 5,
}COAP_CODE;
struct stru_NetSim_COAP_data {
	
	unsigned int Request_messageID;
	unsigned int Request_tokenValue;
	unsigned int Response_messageID;
	unsigned int Response_tokenValue;

	unsigned int Response_Received;
	unsigned int RequestACK_Received;
	unsigned int Request_Received;
	unsigned int ResponseACK_Received;

	int _RETRANSMIT;
	int _gz;

};
struct stru_NetSim_COAP_Options {
	unsigned int optionDelta : 4;
	unsigned int optionLength : 4;
	void* optionvalue;
};
struct stru_NetSim_COAP_Header
{
	unsigned int Version : 2; // always 1
	unsigned int Type : 2; // Confirmable (0), Non-confirmable (1), Acknowledgement (2), or Reset(3)
	unsigned int TokenLength : 4; //length of tokenvalue field in bytes
	unsigned int Code : 8; // request (0) , success response (2) , client error response (4) , server error response(5)
	unsigned int MessageID : 16; //must be match for request and response ACK 
	unsigned int tokenvalue : 32;   //must be a match for request and response message
	struct stru_NetSim_COAP_Options* Options;
	
};


COAP_Header* fn_NetSim_Application_COAP_GenrateHeader(APP_COAP_INFO* info);

void fn_NetSim_Application_COAP_AppIn(APP_INFO* pstruappinfo, NetSim_PACKET* pstruPacket);

int fn_NetSim_Application_COAP_Start(APP_INFO* appInfo, NetSim_EVENTDETAILS* pstruEventDetails);
int fn_NetSim_Application_StartCOAPAPP(APP_INFO* appInfo, double time);
int fn_NetSim_Application_COAP_ProcessRequest(APP_INFO* pstruappinfo, NetSim_PACKET* pstruPacket);
int fn_NetSim_Application_COAP_Sent_ACK(APP_INFO* pstruappinfo, NetSim_PACKET* pstruPacket);
int fn_NetSim_Application_COAP_Genrate_RequestPacket(APP_INFO* appInfo, NETSIM_ID nSource, NETSIM_ID nDestination, double time, char* PacketType);
int fn_NetSim_Application_COAP_Genrate_Packet(APP_INFO* pstruappinfo, NETSIM_ID nSourceId, NETSIM_ID nDestinationId, double size, char* PacketType);
COAP_Header* fn_NetSim_Application_COAP_CopyHeader(COAP_Header* src);
