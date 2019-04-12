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
* Author:    Shashi Kant Suman                                                     *
*                                                                                  *
* ---------------------------------------------------------------------------------*/

#ifndef _NETSIM_ETHERNET_H_
#define _NETSIM_ETHERNET_H_
#ifdef  __cplusplus
extern "C" {
#endif

#pragma comment(lib,"NetworkStack.lib")
#pragma comment(lib,"Metrics.lib")

#define isETHConfigured(d,i) (DEVICE_MACLAYER(d,i)->nMacProtocolId == MAC_PROTOCOL_IEEE802_3)
	//Global variable
	PNETSIM_MACADDRESS multicastSPTMAC;

#define ETH_IFG 0.960 //Micro sec

	typedef enum enum_eth_packet
	{
		ETH_CONFIGBPDU = MAC_PROTOCOL_IEEE802_3 * 100 + 1,
	}ETH_PACKET;

	/** Enumeration for Switching Technique */
	typedef enum enum_SwitchingTechnique
	{
		SWITCHINGTECHNIQUE_NULL,
		SWITCHINGTECHNIQUE_STORE_FORWARD,
		SWITCHINGTECHNIQUE_CUT_THROUGH,
		SWITCHINGTECHNIQUE_FRAGMENT_FREE,
	}SWITCHING_TECHNIQUE;

	/** Enumeration for  SWITCH Port state*/
	typedef enum enum_PortState
	{
		PORTSTATE_DISABLED,
		PORTSTATE_LISTENING,
		PORTSTATE_LEARNING,
		PORTSTATE_FORWARDING,
		PORTSTATE_BLOCKING
	}SWITCH_PORTSTATE;

	typedef enum enum_port_type
	{
		PORTTYPE_DESIGNATED,
		PORTTYPE_NONDESIGNATED,
		PORTTYPE_ROOT,
	}SWITCH_PORTTYPE;


	//Typedefs
	typedef struct stru_ethernet_lan ETH_LAN, *ptrETH_LAN;

#define ETH_HDR_CBPDU	1
#define ETH_HDR_VID		2
#define ETH_HDR_NW		3
	typedef struct stru_eth_packet
	{
		UINT type;
		void* var;
		void* (*fncopy)(void*);
		void(*fnfree)(void*);
	}ETH_HDR,*ptrETH_HDR;
	void set_eth_hdr(NetSim_PACKET* packet, UINT type, void* var, void*(fncopy)(void*), void(*fnfree)(void*));
	void free_eth_hdr(NetSim_PACKET* packet);
	void* get_eth_hdr_var(NetSim_PACKET* packet);

	typedef struct stru_eth_bpdu
	{
		UINT16 protocolIdentifier;
		UINT8 protocolVersionIdentifier;
		UINT8 bpduType;
		UINT8 flags;
		char* rootId;
		UINT rootPathCost;
		char* bridgeId;
		UINT16 portId;
		UINT16 msgAge;
		UINT16 maxAge;
		UINT16 helloTime;
		UINT16 forwardDelay;
	}ETH_BPDU, *ptrETH_BPDU;
#define ETH_BPDU_LEN	60 //Bytes (35 SPT+ 22 Ethernet + 3 LLC)

	typedef struct ieee802_1q_tag
	{
		UINT16 TPId;
		UINT PCP : 3;
		UINT DEI : 1;
		UINT VID : 12;
	}IEEE801_1Q_TAG, *ptrIEEE801_1Q_TAG;
#define IEEE801_1Q_TAG_LEN	4 //Bytes
#define VLAN_TPID	0x8100
	bool isVLANTagPresent(NetSim_PACKET* packet);

	typedef struct stru_SwitchTable
	{
		NETSIM_ID vlanId;
		PNETSIM_MACADDRESS mac;
		NETSIM_ID outPort;
		_ele* ele;
	}SWITCHTABLE, *ptrSWITCHTABLE;
#define SWITCHTABLE_ALLOC() (ptrSWITCHTABLE)list_alloc(sizeof(SWITCHTABLE),offsetof(SWITCHTABLE,ele))
#define SWITCHTABLE_ADD(l,m) LIST_ADD_LAST((void**)(l),m)
#define SWITCHTABLE_NEXT(t) t=(ptrSWITCHTABLE)LIST_NEXT(t)
#define SWITCHTABLE_GET(d,in) (GET_ETH_LAN(d,in,0)->switchTable)
#define SWITCHTABLE_GET_LAN(lan) (lan->switchTable)
	ptrSWITCHTABLE SWITCHTABLE_FIND(ptrETH_LAN lan, PNETSIM_MACADDRESS dest);
	void SWITCHTABLE_NEW(ptrETH_LAN lan, PNETSIM_MACADDRESS dest, NETSIM_ID outport);
	void switchtable_metrics_print(PMETRICSWRITER metricsWriter);

	typedef enum
	{
		VLANPORT_NONE,
		VLANPORT_ACCESS,
		VLANPORT_TRUNK,
	}VLANPORT;
	VLANPORT vlan_port_type_from_str(char* val);

	typedef struct stru_eth_interface_var
	{
		NETSIM_ID interfaceId;
		PNETSIM_MACADDRESS macAddress;
		SWITCH_PORTSTATE portState;
		SWITCH_PORTTYPE portType;
		VLANPORT vlanPortType;
	}ETH_IF, *ptrETH_IF;
	ptrETH_IF GET_ETH_IF(ptrETH_LAN lan, NETSIM_ID i);

	typedef struct stru_ethernet_spt
	{
		char* STPProtocol;
		int stpCost;
		UINT priority;
		char* switchId;

		char* rootId;
		UINT rootPathCost;
		NETSIM_ID rootPort;

	}ETH_SPT, *ptrETH_SPT;

	struct stru_ethernet_lan
	{
		NETSIM_ID lanId;
		UINT interfaceCount;
		NETSIM_ID* interfaceId;
		NETSIM_IPAddress lanIP; // For layer 3 or above device
		UINT16 vlanId;

		ptrETH_IF* ethIfVar;

		ptrETH_SPT spt;

		//Only for switch
		SWITCHING_TECHNIQUE switchingTechnique;
		double latency;
		bool STPStatus;
		ptrSWITCHTABLE switchTable;
		
		char* ethernetStandard;
		double speed;
		
		bool ispromiscuousMode;
	};
	ptrETH_LAN GET_ETH_LAN(NETSIM_ID d, NETSIM_ID i, UINT16 vlanId);

	typedef struct stru_ethernet_var
	{
		NETSIM_ID devId;
		UINT lanCount;
		ptrETH_LAN* lanVar;

		bool isSPT;

		NetSim_BUFFER* buffer;

		bool isVLAN;
		UINT trunkPortCount;
		NETSIM_ID* trunkPortIds;

		bool isIGMPSnooping;
	}ETH_VAR,*ptrETH_VAR;
	ptrETH_VAR GET_ETH_VAR(NETSIM_ID d);
	void SET_ETH_VAR(NETSIM_ID d, ptrETH_VAR var);
	void FREE_ETH_VAR(NETSIM_ID d);

	typedef struct stru_eth_phy
	{
		NETSIM_ID interfaceId;
		double speed;
		double BER;
		double propagationDelay;
		NETSIM_ID connectedDevice;
		NETSIM_ID connectedInterface;
		double IFG;

		NetSim_BUFFER* packet;
		double lastPacketEndTime;

		bool isErrorChkReqd;
	}ETH_PHY, *ptrETH_PHY;
#define ETH_PHY_GET(d,i) ((ptrETH_PHY)DEVICE_PHYLAYER(d,i)->phyVar)

	//Spanning Tree
	void init_spanning_tree_protocol();
	int multicast_config_bpdu();
	void process_configbpdu();

	//VLAN
	void vlan_add_trunk_port(ptrETH_VAR eth, NETSIM_ID in);
	void vlan_forward_to_all_trunk(NETSIM_ID d,
								   NetSim_PACKET* packet,
								   ptrETH_LAN lan,
								   double time);
	void vlan_macout_forward_packet(NETSIM_ID d,
									NETSIM_ID in,
									UINT16 vlanId,
									NETSIM_ID incoming,
									NetSim_PACKET* packet,
									double time);
	void vlan_macin_forward_packet();

	// MAC
	NETSIM_ID find_forward_interface(ptrETH_LAN lan, NetSim_PACKET* packet);
	void check_move_frame_up(NETSIM_ID d,
							 NETSIM_ID in,
							 ptrETH_LAN lan,
							 NetSim_PACKET* packet,
							 double time);
	void forward_frame(NETSIM_ID d,
					   NETSIM_ID in,
					   ptrETH_LAN lan,
					   NetSim_PACKET* packet,
					   double time);
	void send_to_phy(NETSIM_ID d, NETSIM_ID in, NetSim_PACKET* packet, double time);

	//Ethernet utility function
	NETSIM_ID get_interface_id(NETSIM_ID d, NETSIM_ID c);
	void print_ethernet_log(char* format, ...);
	void print_spanning_tree();
	void store_eth_param_macin(NetSim_PACKET* packet, NETSIM_ID devId, NETSIM_ID in, ptrETH_LAN lan);
	void store_eth_incoming(NetSim_PACKET* packet, NETSIM_ID devId, NETSIM_ID incomingPort, UINT16 vlanId);
	UINT16 eth_get_incoming_vlanid(NetSim_PACKET* packet, NETSIM_ID devId);
	NETSIM_ID eth_get_incoming_port(NetSim_PACKET* packet, NETSIM_ID devId);
	void clear_eth_incoming(NetSim_PACKET* packet);

#ifdef  __cplusplus
}
#endif
#endif //_NETSIM_ETHERNET_H_
