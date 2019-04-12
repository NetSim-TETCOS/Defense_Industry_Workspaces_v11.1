/************************************************************************************
 * Copyright (C) 2013
 * TETCOS, Bangalore. India															*

 * Tetcos owns the intellectual property rights in the Product and its content.     *
 * The copying, redistribution, reselling or publication of any or all of the       *
 * Product or its content without express prior written consent of Tetcos is        *
 * prohibited. Ownership and / or any other right relating to the software and all  *
 * intellectual property rights therein shall remain at all times with Tetcos.      *
 * Author:    Basamma YB															*
 * ---------------------------------------------------------------------------------*/
#ifndef _ARP_H_
#define _ARP_H_
#ifdef  __cplusplus
extern "C" {
#endif

#include "NetSim_utility.h"

#define DEFAULT_ARP_RETRY_INTERVAL 10	// secs
#define DEFAULT_ARP_RETRY_LIMIT 3	//Retry count	


#define HARDWARE_ADDRESS_LENGTH 6	///< Lenghth of 48 bit MAC address in bytes
#define ARP_ETHERNET_HEADER_LENGTH 14	///< Bytes :6(Dest Mac add)+6(src Mac add)+2(frame type).
#define IPV4_PROTOCOL_ADDREES_LENGTH 4	///< Length of 32 bit IP address in bytes.
#define IPV4_ARP_PACKET_SIZE 28			///< Bytes:2(H/Wtype)+2(ProtocolType)+1(H/Wlength)+1(ProtocolLength)+ 2(opcode)+6(srcMac)+4(srcIP)+6(TargetMac)+4(TargetIP).
#define IPV4_ARP_PACKET_SIZE_WITH_ETH_HEADER 42	///< ARP frame size in Bytes: 28 + 14 
#define IPV4_NETWORK_OVERHEADS 20 ///< In bytes for IPV4.	
#define IPV6_PROTOCOL_ADDREES_LENGTH 16	///< Length of 128 bit IP address in bytes.
#define IPV6_ARP_PACKET_SIZE 52	///< bytes:2(H/Wtype)+2(ProtocolType)+1(H/Wlength)+1(ProtocolLength)+ 2(opcode)+6(srcMac)+16(srcIP)+6(TargetMac)+16(TargetIP).			
#define IPV6_ARP_PACKET_SIZE_WITH_ETH_HEADER 66	///< ARP frame size in Bytes: 52 + 14.
#define IPV6_NETWORK_OVERHEADS 40 ///< In bytes for IPV6 (320 bits).	


//Typedef declaration of structure 
typedef struct stru_ARP_Table ARP_TABLE;
typedef struct stru_Arp_Packet ARP_PACKET;
typedef struct stru_ARP_Interface_Variables ARP_VARIABLES;
typedef struct stru_ArpDataPacket_Buffer ARP_BUFFER;
typedef struct stru_ARP_Metrics ARP_METRICS;
typedef struct stru_ARP_Static_Table_Configuration STATIC_TABLE_CONFIG;
//Typedef declaration of enumaration 
typedef enum enum_ARP_Subevent_Type SUB_EVENT;
typedef enum enum_ARP_opcode OPCODE;
typedef enum enum_ARP_EthernetFrameType ETHERNET_TYPE;
typedef enum enum_ARP_HardwareType HARDWARETYPE;
typedef enum enum_ARP_PrptocolType PROTOCOLTYPE;
typedef enum enum_ArpControlPacket_Type ARP_CONTROL_PACKET;
typedef enum enum_Transmission_Type ARP_FRAME_TX_FLAG;
typedef enum enum_ARP_Table_Entries_Type ENTRY_TYPE;
typedef enum enum_Static_Arp_Status STATIC_ARP_STATUS;

/** Enumeration for Sub event Type */
 enum enum_ARP_Subevent_Type 
{
	READ_ARP_TABLE =  NW_PROTOCOL_ARP*100+1,
	GENERATE_ARP_REQUEST,	
	GENERATE_ARP_REPLY,
	UPDATE_ARP_TABLE_FWD_PKT,
	ARP_REQUEST_TIMEOUT,	
 }; 
 /** Enumeration for Operation code to specify Packet Type*/
enum enum_ARP_opcode 
{
	ares_opSREQUEST=1,
	ares_opSREPLY,
};
/** Enumeration for Ethernet frame type */
 enum enum_ARP_EthernetFrameType 
 {
	 ADDRESS_RESOLUTION = 0x8060 	
 };
 /** Enumeration for hardware type */
 enum enum_ARP_HardwareType
 {
	 ETHERNET = 0x0001,
	 IEEE802 = 0x0006
 };
 /** Enumeration for protocol type */
 enum enum_ARP_PrptocolType
 {
	 ARP_TO_RESOLVE_IP = 0x8000 
 };
 /** Enumeration for ARP control packet type */
 enum enum_ArpControlPacket_Type	
{
	REQUEST_PACKET = NW_PROTOCOL_ARP*100+1,
	REPLY_PACKET,
};
 /** Enumeration for ARP Table entries type */ 
enum enum_ARP_Table_Entries_Type 
{
	STATIC,
	DYNAMIC,
};
/** Enumeration for static ARP status */
enum enum_Static_Arp_Status	
{
	DISABLE,
	ENABLE
}; 
/// This Arp packet structure is according to RFC 826.
struct stru_Arp_Packet	
{	// Ethernet Header	
	PNETSIM_MACADDRESS szDestMac;	///< Destination MAC address.	
	PNETSIM_MACADDRESS szSrcMac;		///< Source MAC address.
	ETHERNET_TYPE nEther_type;		///<Ethernet Type
	// ARP Packet DATA	
	HARDWARETYPE n_ar$hrd;		///< Hardware Type 2 bytes.	
	PROTOCOLTYPE n_ar$pro;		///< Protocol Type 2 bytes.		
	unsigned short int usn_ar$hln;	///< H/W address length 1 byte ,specifies the sizes of the H/W address in bytes.	
	unsigned short int usn_ar$pln;	///< Protocol address length 1 byte,specifies the sizes of the protocol address in bytes.	
	OPCODE n_ar$op;	///< Operation REQUEST/REPLY 2 bytes.	
	PNETSIM_MACADDRESS sz_ar$sha;	///< Hardware address of the sender.	
	NETSIM_IPAddress sz_ar$spa;		///< Protocol address of the sender.	
	PNETSIM_MACADDRESS sz_ar$tha;	///< Hardware address of target (if know) otherwise empty 6 bytes.	
	NETSIM_IPAddress sz_ar$tpa;		///< Protocol address of target.
};

/** Structure for ARP Table */
struct stru_ARP_Table 
{	
	NETSIM_IPAddress szIPAddress;	///< IP address of the deivce	
	PNETSIM_MACADDRESS szMACAddress;	///< MAC address or Hardware address of the device	
	ENTRY_TYPE nType;	///< 0-Static,1-Dynamic.	
	struct stru_ARP_Table *pstruNextEntry;	///< Next entry pointer
};
/** Structure to buffer the packet */
struct stru_ArpDataPacket_Buffer	
{	
	NETSIM_IPAddress szDestAdd;	///< Store the destination IP address.	
	NETSIM_ID nBufferInterfaceID; ///< Store the InterfaceId while buffering the packet.	
	NetSim_PACKET *pstruPacket;	///< Store the packet	
	struct stru_ArpDataPacket_Buffer *pstruNextBuffer;
};
/** Structure to to store the ARP metrics */
struct stru_ARP_Metrics 
{	
	int nArpRequestSentCount;	///< Number of requests sent from the source	 
	int nArpReplyReceivedCount;	///< Number of replies received from destination	
	int nArpReplySentCount;	///< Number of replies sent from the destination	
	int nPacketsInBuffer;   ///< Number of packets in the buffer		
	int nPacketDropCount;   /// Number of packets droped in the buffer		
};
/** Structure to to store the ARP variables */
struct stru_ARP_Interface_Variables 
{	
	int nStaticTableFlag;	///< Check ARP_TABLE intialized by static table or not.	
	ARP_TABLE *pstruArpTable;	
	int nArpRetryLimit;		///< Store the ARP_RETRY_LIMIT from the config file.	
	int nArpRetryInterval;	///< Store the ARP_RETRY_INTERVAL from the config file	 
	int *pnArpRequestFlag;	///< Set when generate Request.	
	int *pnArpReplyFlag;	///< Set when receive the Reply.	
	int *pnArpRetryCount;	///< To keep track of number of retries.	
	ARP_BUFFER *pstruPacketBuffer;	
	ARP_METRICS *pstruArpMetrics; ///< NetSim specific ARP metrics structure.

};
/// Structure for Static ARP Table configuration
struct stru_ARP_Static_Table_Configuration
{
	STATIC_ARP_STATUS nStaticArpFlag;
	char* pszFilePath;		///< Stores File path
	char* pszFileName;		///< Stores File Name
};

NETSIM_IPAddress szBroadcastIPaddress;

/* NetSim specific global variable for configuration and metrics */
STATIC_TABLE_CONFIG *g_pstruStaticTableConfig;
// ************ Other Functions in ARP **********************************************************
/// Function to Read the static table and assign to the ARP_TABLE 
int fn_NetSim_StaticArpTable_Read(char* pszARPstasticTablePath);
/// Function to add the new entry to the ARP_TABLE(IP add, MAC add and Type)
int fn_NetSim_Add_IP_MAC_AddressTo_ARP_Table(ARP_TABLE** ,NETSIM_IPAddress , PNETSIM_MACADDRESS , int  );
/// Function to Copy the ARP_TABLE from source to destination. Returm destination ARP Table head pointer reference.
ARP_TABLE* fn_Netsim_CopyArpTable(ARP_TABLE* );
/// Function to Delete/ deallocate the memory assigned to the ARP_TABLE 
void fn_NetSim_DeleteArpTable(ARP_TABLE** );
// Function to add the data packet and interface Id to the ARP_BUFFER
//int fn_NetSim_Add_PacketTo_Buffer(ARP_BUFFER** ,NetSim_PACKET* ,NETSIM_IPAddress , NETSIM_ID );
// Function to delete/ deallocate data packet from the ARP_BUFFER
//void fn_NetSim_Arp_Drop_Buffered_Packet(ARP_BUFFER** ,  NETSIM_IPAddress , int * );
// Function to check the destination device IP address to generate the ARP REPLY
int fn_Netsim_ARP_CheckDestinationDevice(NetSim_EVENTDETAILS* ,struct stru_NetSim_Network*);
int fn_NetSim_Arp_Drop_Buffered_Packet(NETSIM_ID nDeviceId, NETSIM_ID nInterfaceId,NETSIM_IPAddress szDestIPadd, int *nPacketDropCount);
int fn_NetSim_Add_PacketTo_Buffer(NETSIM_ID nDeviceId, NetSim_PACKET* pstruNewPacket,NETSIM_IPAddress szDestIPadd, NETSIM_ID nInterfaceId);



/****************** NetWorkStack DLL functions declarations *****************************************/
/// Function for configuring ARP parameters
_declspec(dllexport) int fn_NetSim_ARP_Configure(void** var);
int fn_NetSim_ARP_Configure_F(void** var);
/// Function for Intializing ARP protocol 
_declspec (dllexport) int fn_NetSim_ARP_Init(struct stru_NetSim_Network *NETWORK_Formal,NetSim_EVENTDETAILS *pstruEventDetails_Formal,char *pszAppPath_Formal,char *pszWritePath_Formal,int nVersion_Type,void **fnPointer);
int fn_NetSim_ARP_Init_F(struct stru_NetSim_Network *,NetSim_EVENTDETAILS *,char *,char *,int ,void **fnPointer);
/// Function to run ARP protocol 
_declspec (dllexport) int fn_NetSim_ARP_Run();
/// Function to free the ARP protocol variable and Unload the primitives
_declspec(dllexport) int fn_NetSim_ARP_Finish();
int fn_NetSim_ARP_Finish_F();
/// Return the subevent name with respect to the subevent number for writting event trace
_declspec (dllexport) char *fn_NetSim_ARP_Trace(int nSubEvent);
char *fn_NetSim_ARP_Trace_F(int nSubEvent);
/// Function to free the allocated memory for the ARP packet
_declspec(dllexport) int fn_NetSim_ARP_FreePacket(NetSim_PACKET*);
int fn_NetSim_ARP_FreePacket_F(NetSim_PACKET* );
/// Function to copy the ARP packet from source to destination
_declspec(dllexport) int fn_NetSim_ARP_CopyPacket(NetSim_PACKET* ,NetSim_PACKET* );
int fn_NetSim_ARP_CopyPacket_F(NetSim_PACKET* ,NetSim_PACKET* );
/// Function to write ARP Metrics into Metrics.txt
_declspec(dllexport) int fn_NetSim_ARP_Metrics(char* );
int fn_NetSim_ARP_Metrics_F(char* );
_declspec(dllexport) char* fn_NetSim_ARP_ConfigPacketTrace();

_declspec(dllexport) char* fn_NetSim_ARP_WritePacketTrace(NetSim_PACKET* pstruPacket, char** ppszTrace);


/// Function used to check the destination is in the same subnet or not from IPV4.lib
int fn_NetSim_ipv4_network_check(char* ,char* ,char* );

int fn_NetSim_Generate_ARP_Request(NetSim_EVENTDETAILS *pstruEventDetails, struct stru_NetSim_Network *NETWORK);
int fn_NetSim_Read_ARP_Table(NetSim_EVENTDETAILS *pstruEventDetails, struct stru_NetSim_Network *NETWORK);
int fn_NetSim_ARP_Request_Timeout(NetSim_EVENTDETAILS *pstruEventDetails, struct stru_NetSim_Network *NETWORK);
int fn_NetSim_Generate_ARP_Reply(NetSim_EVENTDETAILS *pstruEventDetails, struct stru_NetSim_Network *NETWORK);
int fn_NetSim_Update_ARP_Table_ForwardPacket(NetSim_EVENTDETAILS *pstruEventDetails, struct stru_NetSim_Network *NETWORK);
#ifdef  __cplusplus
}
#endif
#endif /* _ARP_H_*/
