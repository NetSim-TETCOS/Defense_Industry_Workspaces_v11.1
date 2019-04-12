/************************************************************************************
 * Copyright (C) 2013     
 *
 * TETCOS, Bangalore. India                                                         *

 * Tetcos owns the intellectual property rights in the Product and its content.     *
 * The copying, redistribution, reselling or publication of any or all of the       *
 * Product or its content without express prior written consent of Tetcos is        *
 * prohibited. Ownership and / or any other right relating to the software and all  *
 * intellectual property rights therein shall remain at all times with Tetcos.      *

 * Author:    P.Sathishkumar                                                        *
 * ---------------------------------------------------------------------------------*/

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                                       *
 *	THIS FILES CONTAINS UDP DATASTUCTURE WHICH HAS VARIABLES THAT ARE PROVIDED FOR USERS.*    
 *																						 *
 *	BY MAKING USE OF THESE VARIABLES THE USER CAN CREATE THEIR OWN PROJECT IN UDP 		 *
 *	                                                                                     *    
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#define _CRT_SECURE_NO_DEPRECATE
#ifndef _UDP_H_
#define _UDP_H_
/// UDP Over heads in bytes
#define TRANSPORT_UDP_OVERHEADS 8

/** Typedef declaration of structures */
typedef struct stru_UserDatagram_Header DATAGRAM_HEADER_UDP;
typedef struct stru_UDP_Metrics UDP_METRICS;

/** UDP Metrics - sepecific to NetSim */
struct stru_UDP_Metrics
{
	NETSIM_ID nDevice_ID;
	NETSIM_IPAddress szSrcIP;
	NETSIM_IPAddress szDestIP;
	unsigned short int usnSourcePort;
	unsigned short int usnDestinationPort;
	int nDataGramsSent;
	int nDataGramsReceived;
	struct stru_UDP_Metrics *pstruNextUPD_Metrics;
};

/** User Datagram Header Format as per RFC 768 28 August 1980 */
struct stru_UserDatagram_Header
{
	unsigned short int usnSourcePort;
	unsigned short int usnDestinationPort;
	unsigned short int usnLength;
	unsigned short int usnChecksum;
};


int fn_NetSim_UDP_Create_ApplicationMetrics(NETSIM_ID, unsigned short, unsigned short, NETSIM_IPAddress, NETSIM_IPAddress, UDP_METRICS **);
int fn_NetSim_UDP_Check_ApplicationMetrics(NETSIM_ID, NETSIM_ID, NETSIM_ID, NETSIM_IPAddress, NETSIM_IPAddress, UDP_METRICS **);

/****************** NetWorkStack DLL functions declarations *****************************************/
/** Function for configuring UDP parameters*/
_declspec(dllexport) int fn_NetSim_UDP_Configure(void** var);
int fn_NetSim_UDP_Configure_F(void** var);
/** Function for Intializing UDP protocol */
_declspec (dllexport) int fn_NetSim_UDP_Init(struct stru_NetSim_Network *NETWORK_Formal,NetSim_EVENTDETAILS *pstruEventDetails_Formal,char *pszAppPath_Formal,char *pszWritePath_Formal,int nVersion_Type,void **fnPointer);
int fn_NetSim_UDP_Init_F(struct stru_NetSim_Network *,NetSim_EVENTDETAILS *,char *,char *,int ,void **fnPointer);
/** Function to run UDP protocol */
_declspec (dllexport) int fn_NetSim_UDP_Run();
/// Function to free the UDP protocol variable and Unload the primitives
_declspec(dllexport) int fn_NetSim_UDP_Finish();
int fn_NetSim_UDP_Finish_F();
/// Return the subevent name with respect to the subevent number for writting event trace
_declspec (dllexport) char *fn_NetSim_UDP_Trace(int nSubEvent);
char *fn_NetSim_UDP_Trace_F(int nSubEvent);
/// Function to free the allocated memory for the UDP packet
_declspec(dllexport) int fn_NetSim_UDP_FreePacket(NetSim_PACKET* );
int fn_NetSim_UDP_FreePacket_F(NetSim_PACKET* );
/// Function to copy the UDP packet from source to destination
_declspec(dllexport) int fn_NetSim_UDP_CopyPacket(NetSim_PACKET* ,NetSim_PACKET* );
int fn_NetSim_UDP_CopyPacket_F(NetSim_PACKET* ,NetSim_PACKET* );
/// Function to write UDP Metrics into Metrics.txt
_declspec(dllexport) int fn_NetSim_UDP_Metrics(PMETRICSWRITER metricsWriter);
int fn_NetSim_UDP_Metrics_F(PMETRICSWRITER metricsWriter);
int fn_NetSim_UDP_Send_User_Datagram(NetSim_EVENTDETAILS *pstruEventDetails,struct stru_NetSim_Network *NETWORK);
_declspec(dllexport) int fn_NetSim_UDP_Receive_User_Datagram(NetSim_EVENTDETAILS *pstruEventDetails,struct stru_NetSim_Network *NETWORK);

#endif
