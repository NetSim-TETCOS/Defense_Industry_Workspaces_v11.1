/************************************************************************************
* Copyright (C) 2012     
*
* TETCOS, Bangalore. India                                                         *

* Tetcos owns the intellectual property rights in the Product and its content.     *
* The copying, redistribution, reselling or publication of any or all of the       *
* Product or its content without express prior written consent of Tetcos is        *
* prohibited. Ownership and / or any other right relating to the software and all  *
* intellectual property rights therein shall remain at all times with Tetcos.      *

* Author:  Thangarasu.K                                                       *
* ---------------------------------------------------------------------------------*/

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*                                                                                                   *
*	THIS FILES CONTAIN RIP DATASTUCTURE WHICH HAS VARIABLES THAT ARE PROVIDED FOR USERS.    *    
*																								     *
*	BY MAKING USE OF THESE VARIABLES THE USER CAN CREATE THEIR OWN PROJECT IN RIP 	         *
*	                                                                                                 *    
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  */
#ifndef _RIP_H_
#define _RIP_H_
#ifdef  __cplusplus
extern "C" {
#endif

#define RIP_PACKET_SIZE_WITH_HEADER 512 
#define RIP_PORT 520 // Reference : RFC 2453, November 1998, Page 20
	// RIP has a routing process that sends and receives datagrams on UDP port number 520
#define EXPIRED_ROUTE 16 //Infinity route
#define _RIP_Trace
	FILE *fpRIPTrace;

	//RIP
	typedef struct stru_NetSim_Router DEVICE_ROUTER;
	typedef struct stru_Router_Buffer BUFFER_ROUTER;
	typedef struct stru_NetSim_RIP_Packet RIP_PACKET; //RIP Packet structure
	typedef struct stru_Router_RIP_Routing_database RIP_ROUTING_DATABASE; // Routing database structure
	typedef struct  stru_RIP_TempRouting_database RIP_TEMPROUTING_DATABASE;
	typedef struct stru_RIPEntry RIP_ENTRY; 
	typedef enum enum_RIP_command RIP_COMMAND; 
	typedef enum enum_RIP_Subevent_Type SUB_EVENT;



	/** Enumeration for RIP Sub event Types*/
	enum enum_RIP_Subevent_Type
	{
		SEND_RIP_UPDATE_PKT=APP_PROTOCOL_RIP*100+1,
		RIP_TIMEOUT,
		RIP_GARBAGE_COLLECTION,
		ROUTING_TABLE_UPDATION,
	};


	/// This enumeration is to denote the command field in RIP packet
	enum enum_RIP_command
	{
		REQUEST=1,
		RESPONSE,
	};


	/** Enumeration for App Layer Packet Type*/
	enum enum_RIP_ControlPacketType		
	{
		RIP_Packet = APP_PROTOCOL_RIP*100+1,
	};
	///Routing database structure Reference : RFC 2453, November 1998, Page 8
	struct  stru_Router_RIP_Routing_database

	{
		NETSIM_IPAddress szAddress; ///< IP address of the destination host or destination network		
		NETSIM_IPAddress szSubnetmask; ///< Subnet mask for the destination network		
		NETSIM_IPAddress szRouter; ///< The first router along the route to the destination		
		NETSIM_ID nInterface; ///< The physical network which must be used to reach the first router		
		unsigned int nMetric; ///< Distance to the destination		
		double dTimer; ///< The amount of time since the entry was last updated
		struct stru_Router_RIP_Routing_database *pstru_Router_NextEntry;
	};

/**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	RIP packet structure. RFC 2453 November 1998 Pages : 20,21

	The RIP packet format is:

	   0                   1                   2                   3
	   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
	  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	  |  command (1)  |  version (1)  |       must be zero (2)        |
	  +---------------+---------------+-------------------------------+
	  |                                                               |
	  ~                         RIP Entry (20)                        ~
	  |                                                               |
	  +---------------+---------------+---------------+---------------+
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ 
 */
	struct stru_NetSim_RIP_Packet
	{
		RIP_COMMAND command;	/**< The command field is used to specify the purpose of this message. <br/>
									If it is 1-->request message, 2-->response message */		
		unsigned int nVersion:8; ///< The version field is used to specify the RIP version (version 1 or 2)
		struct stru_RIPEntry *pstruRIPEntry;
	};

	 /**
	 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		The format for the 20-octet route entry (RTE) for
		RIP-2 is:

		0                   1                   2                   3 3
		0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
	   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	   | Address Family Identifier (2) |        Route Tag (2)          |
	   +-------------------------------+-------------------------------+
	   |                         IP Address (4)                        |
	   +---------------------------------------------------------------+
	   |                         Subnet Mask (4)                       |
	   +---------------------------------------------------------------+
	   |                         Next Hop (4)                          |
	   +---------------------------------------------------------------+
	   |                         Metric (4)                            |
	   +---------------------------------------------------------------+
	~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	  */
	struct stru_RIPEntry
	{
		unsigned int nAddress_family_identifier:16; ///< This is used to identify the Address family of the IP address
		short int RouteTag;		
		NETSIM_IPAddress szIPv4_address; ///< Destination IPv4 address		
		NETSIM_IPAddress szSubnet_Mask;  ///< Destination Subnet Mask		
		NETSIM_IPAddress szNext_Hop; /**< The immediate next hop IP address to which packets to the destination. 
										  specified by this route entry should be forwarded */		
		unsigned int nMetric; ///< Cost to reach the destination
		struct stru_RIPEntry *pstru_RIP_NextEntry;
	};
	/** Structure to store RIP information */
	struct stru_RIP 
	{
		int n_Update_timer;   ///< These variable values are get from configuration.xml,in RFC the update timer is mentioned as 30-second timer 
		int n_timeout_timer;
		int n_garbage_collection_timer;
		unsigned int n_RIP_Version:8;		
		unsigned int nRIP_Update;	///< Used to check the updates in the router		
		int nDataDropped;		///< Used to store the dropped packets                
		unsigned short int nStatus;

		bool nStaticRoutingFlag;		
		char* pszFilePath;	///< Stores File path		
		char* pszFileName;	///< Stores File Name	
	};

	/// This function is for forming the initial table for each router	
	int fn_NetSim_RIP_InitialTable_Creation(NETSIM_ID nDeviceId);

	/// This function is used to update the database of router
	int fn_NetSim_RIP_UpdatingEntriesinRoutingDatabase(struct stru_NetSim_Network *,int,NETSIM_IPAddress,NETSIM_IPAddress,NETSIM_IPAddress,NETSIM_ID,double,unsigned int);

	/// This function is used receive the updates from neighbor routers
	_declspec(dllexport) int fn_NetSim_RIP_ReceivingOf_RIP_Message(struct stru_NetSim_Network* ,NetSim_EVENTDETAILS*  ); 

	/// This function is used to trigger the update
	_declspec(dllexport)int fn_NetSim_RIP_TriggeredUpdate(struct stru_NetSim_Network *,NetSim_EVENTDETAILS *);

	int fn_NetSim_RIP_Run_F();

	int fn_NetSim_RIPTrace(struct stru_NetSim_Network*,NETSIM_ID,int);



	/****************** NetWorkStack DLL functions declarations *****************************************/
	/** Function for configuring RIP parameters*/
	_declspec(dllexport) int fn_NetSim_RIP_Configure_F(void** var);

	/** Function for Intializing RIP protocol */
	_declspec (dllexport) int fn_NetSim_RIP_Init(struct stru_NetSim_Network *NETWORK_Formal,NetSim_EVENTDETAILS *pstruEventDetails_Formal,char *pszAppPath_Formal,char *pszWritePath_Formal,int nVersion_Type,void **fnPointer);
	int fn_NetSim_RIP_Init_F(struct stru_NetSim_Network * ,NetSim_EVENTDETAILS * ,char * ,char * ,int  ,void **fnPointer);
	/** Function to run RIP protocol */
	_declspec (dllexport) int fn_NetSim_RIP_Run();
	/// Function to free the RIP protocol variable and Unload the primitives
	_declspec(dllexport) int fn_NetSim_RIP_Finish();
	int fn_NetSim_RIP_Finish_F();
	/// Return the subevent name with respect to the subevent number for writting event trace
	_declspec (dllexport) char *fn_NetSim_RIP_Trace(int nSubEvent);
	char *fn_NetSim_RIP_Trace_F(int nSubEvent);
	/// Function to free the allocated memory for the RIP packet
	_declspec(dllexport) int fn_NetSim_RIP_FreePacket(NetSim_PACKET* );
	int fn_NetSim_RIP_FreePacket_F(NetSim_PACKET* );
	/// Function to copy the RIP packet from source to destination
	_declspec(dllexport) int fn_NetSim_RIP_CopyPacket(NetSim_PACKET*  ,NetSim_PACKET* );
	int fn_NetSim_RIP_CopyPacket_F(NetSim_PACKET* ,NetSim_PACKET* );

	_declspec(dllexport) int fn_NetSim_RIP_Metrics(PMETRICSWRITER metricsWriter);
	int fn_NetSim_RIP_Metrics_F(PMETRICSWRITER metricsWriter);

	int fn_NetSim_RIP_DistanceVectorAlgorithm(struct stru_NetSim_Network *NETWORK,NetSim_EVENTDETAILS *pstruEventDetails);
	int fn_NetSim_RIP_Update_Timer(struct stru_NetSim_Network *pstruNETWORK,NetSim_EVENTDETAILS *pstruEventDetails);
	int fn_NetSim_RIP_Timeout_Timer(struct stru_NetSim_Network *pstruNETWORK,NetSim_EVENTDETAILS *pstruEventDetails);
	int fn_NetSim_RIP_Garbage_Collection_Timer(struct stru_NetSim_Network *pstruNETWORK,NetSim_EVENTDETAILS *pstruEventDetails);
	bool isRIPConfigured(NETSIM_ID d);
	DEVICE_ROUTER* get_RIP_var(NETSIM_ID d);
	void set_RIP_var(NETSIM_ID d, DEVICE_ROUTER* rip);
#ifdef  __cplusplus
}
#endif
#endif /* _RIP_H_*/
