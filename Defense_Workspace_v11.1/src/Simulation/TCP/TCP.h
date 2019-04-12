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

#ifndef _NETSIM_TCP_H_
#define _NETSIM_TCP_H_
#ifdef  __cplusplus
extern "C" {
#endif

	/* RFC List
	* RFC 793  : TRANSMISSION CONTROL PROTOCOL
	* RFC 1122 : Requirements for Internet Hosts -- Communication Layers
	* RFC 5681 : TCP Congestion Control
	* RFC 3390 : Increasing TCP's Initial Window
	* RFC 6298 : Computing TCP's Retransmission Timer
	* RFC 2018 : TCP Selective Acknowledgment Options
	* RFC 6582 : The NewReno Modification to TCP's Fast Recovery Algorithm
	* RFC 6675 : A Conservative Loss Recovery Algorithm Based on Selective Acknowledgment (SACK) for TCP
	* RFC 7323 : TCP Extensions for High Performance
	* https://research.csc.ncsu.edu/netsrv/sites/default/files/cubic_a_new_tcp_2008.pdf
	* https://research.csc.ncsu.edu/netsrv/sites/default/files/bitcp.pdf
	* https://research.csc.ncsu.edu/netsrv/sites/default/files/hystart_techreport_2008.pdf
	*/

//#define _TEST_TCP_ //Enable to test the TCP
#ifdef _TEST_TCP_
	static double dropProbability = 0.5;
	static bool burstDrop = true;
	static int burstDropSize = 10;
	static bool isAckDrop = true;
#endif

#pragma comment (lib,"NetworkStack.lib")

	//USEFUL MACRO
#define isTCPConfigured(d) (DEVICE_TRXLayer(d) && DEVICE_TRXLayer(d)->isTCP)
#define isTCPControl(p) (p->nControlDataType/100 == TX_PROTOCOL_TCP)

	//Constant
#define TCP_DupThresh	3

	//Typedef
	typedef struct stru_TCP_Socket  NETSIM_SOCKET, *PNETSIM_SOCKET;

	typedef enum enum_tcpstate
	{
		TCPCONNECTION_CLOSED,
		TCPCONNECTION_LISTEN,
		TCPCONNECTION_SYN_SENT,
		TCPCONNECTION_SYN_RECEIVED,
		TCPCONNECTION_ESTABLISHED,
		TCPCONNECTION_FIN_WAIT_1,
		TCPCONNECTION_FIN_WAIT_2,
		TCPCONNECTION_CLOSE_WAIT,
		TCPCONNECTION_CLOSING,
		TCPCONNECTION_LAST_ACK,
		TCPCONNECTION_TIME_WAIT,
	}TCP_CONNECTION_STATE;

	typedef enum enum_tcp_variant
	{
		TCPVariant_OLDTAHOE, //Slow Start and Congestion Avoidance
		TCPVariant_TAHOE,	 //Fast Retransmit/Fast Recovery
		TCPVariant_RENO,
		TCPVariant_NEWRENO,
		TCPVariant_BIC,
		TCPVariant_CUBIC,
	}TCPVARIANT;

	typedef enum enum_tcp_ack_type
	{
		TCPACKTYPE_UNDELAYED,
		TCPACKTYPE_DELAYED,
	}TCPACKTYPE;

#include "TCB.h"

	typedef struct stru_tcp_metrics
	{
		char* source;
		char* dest;
		char* localAddr;
		char* remoteAddr;
		UINT synSent;
		UINT synAckSent;
		UINT segmentSent;
		UINT segmentReceived;
		UINT segmentRetransmitted;
		UINT ackSent;
		UINT ackReceived;
		UINT dupAckReceived;
		UINT timesRTOExpired;
		UINT dupSegmentReceived;
		UINT outOfOrderSegmentReceived;

		void* congestionPlot;
		bool isCongestionPlotRequire;
	}TCP_METRICS,*PTCP_METRICS;

	typedef struct stru_socket_addr
	{
		NETSIM_IPAddress ip;
		UINT16 port;
	}SOCKETADDRESS, *PSOCKETADDRESS;

	struct stru_TCP_Socket
	{
		UINT SocketId;
		PSOCKETADDRESS localAddr;
		PSOCKETADDRESS remoteAddr;

		//For simulation
		NETSIM_ID localDeviceId;
		NETSIM_ID remoteDeviceId;
		ptrSOCKETINTERFACE sId;
		bool waitFromApp;
		NETSIM_ID appId;

		PTCB tcb; //Transmission control block

		/* Callback function. This is required only for simulator
		 * As all TCP connection is under same system process so
		 * no lock function.
		 */
		void(*listen_callback)(struct stru_TCP_Socket* s, NetSim_PACKET*);

		PTCP_METRICS tcpMetrics;
	};

	typedef struct stru_TCP_device_var
	{
		TCPVARIANT tcpVariant;
		void* socket_list;
		UINT16 initSSThresh;
		UINT16 MSS;
		UINT MaxSynRetries;
		TCPACKTYPE ackType;
		double timeWaitTimer;
		double dDelayedAckTime;
		bool isSackPermitted;
		bool isWindowScaling;
		UINT8 shiftCount;
		bool isTimestampOpt;

		double beta;
		UINT32 low_window;
		UINT32 max_increment;
		UINT32 smooth_part;
		UINT32 bic_scale;
		UINT32 hystart_low_window;
		UINT32 hystart_ack_delta;
	}TCP_DEV_VAR,*PTCP_DEV_VAR;
	static PTCP_DEV_VAR GET_TCP_DEV_VAR(NETSIM_ID d)
	{
		if (isTCPConfigured(d))
			return DEVICE_TRXLayer(d)->TCPVar;
		else return NULL;
	}
#define GET_TCP_VARIANT(d) (GET_TCP_DEV_VAR(d)->tcpVariant)

	//Function Prototype
	//Socket
	void add_to_socket_list(NETSIM_ID devId, PNETSIM_SOCKET s);
	PNETSIM_SOCKET find_socket(NETSIM_ID devId,
						NETSIM_IPAddress srcIP,
						NETSIM_IPAddress destIP,
						UINT16 srcPort,
						UINT16 destPort);
	PNETSIM_SOCKET find_socket_at_source(NetSim_PACKET* packet);
	PNETSIM_SOCKET find_socket_at_dest(NetSim_PACKET* packet);
	PNETSIM_SOCKET tcp_create_socket();
	void tcp_bind(PNETSIM_SOCKET s, PSOCKETADDRESS addr);
	void tcp_listen(PNETSIM_SOCKET s,
					void(*listen_callback)(PNETSIM_SOCKET, NetSim_PACKET*));
	void tcp_connect(PNETSIM_SOCKET s,
					 PSOCKETADDRESS localAddr,
					 PSOCKETADDRESS remoteAddr);
	PNETSIM_SOCKET tcp_accept(PNETSIM_SOCKET s,
							  NetSim_PACKET* p);
	void close_all_socket(NETSIM_ID devId);
	void tcp_close(PNETSIM_SOCKET s);
	void tcp_close_socket(PNETSIM_SOCKET s, NETSIM_ID devId);

	//Connection
	void tcp_active_open(PNETSIM_SOCKET s);
	void tcp_passive_open(PNETSIM_SOCKET s, PNETSIM_SOCKET ls);

	//RTO
	double get_RTT(PTCB tcb, UINT ackNo);
	double calculate_RTO(double R,
						 double* srtt,
						 double* rtt_var);
	void add_timeout_event(PNETSIM_SOCKET s,
						   NetSim_PACKET* packet);
	void handle_rto_timer();

	//Time wait Timer
	void start_timewait_timer(PNETSIM_SOCKET s);

	//TCP Queue
	bool isSegmentInQueue(PTCP_QUEUE queue, NetSim_PACKET* packet);
	void add_packet_to_queue(PTCP_QUEUE queue, NetSim_PACKET* packet, double time);
	NetSim_PACKET* get_segment_from_queue(PTCP_QUEUE queue, UINT32 seqNo);
	NetSim_PACKET* get_copy_segment_from_queue(PTCP_QUEUE queue, UINT32 seqNo, bool* isSacked);
	void delete_all_segment_from_queue(PTCP_QUEUE queue);
	void delete_segment_from_queue(PTCP_QUEUE queue, UINT32 ackNo);
	void check_segment_in_queue(PNETSIM_SOCKET s);
	NetSim_PACKET* check_for_other_segment_to_send_from_queue(PNETSIM_SOCKET s);

	//TCP Control
	//RST
	void send_rst(PNETSIM_SOCKET s, NetSim_PACKET* p, UINT c);
	//Syn
	bool isSynPacket(NetSim_PACKET* packet);
	bool isSynbitSet(NetSim_PACKET* packet);
	NetSim_PACKET* create_syn(PNETSIM_SOCKET s,
							  double time);
	void resend_syn(PNETSIM_SOCKET s);
	void rcv_SYN(PNETSIM_SOCKET s, NetSim_PACKET* syn);

	//Syn-Ack
	NetSim_PACKET* create_synAck(PNETSIM_SOCKET s,
								 double time);

	//Ack
	NetSim_PACKET* create_ack(PNETSIM_SOCKET s,
							  double time,
							  UINT32 seqno,
							  UINT32 ackno);
	void send_ack(PNETSIM_SOCKET s, double time, UINT32 seqNo, UINT32 ackNo);

	//FIN
	NetSim_PACKET* create_fin(PNETSIM_SOCKET s,
							  double time);
	void send_fin(PNETSIM_SOCKET s);

	//FIN-Ack
	void send_fin_ack(PNETSIM_SOCKET s, double time, UINT32 seqNo, UINT32 ackNo);

	//Rst
	NetSim_PACKET* create_rst(NetSim_PACKET* p,
							  double time,
							  UINT c);

	//Segment
	UINT32 get_seg_len(NetSim_PACKET* p);
	void add_tcp_hdr(NetSim_PACKET* p, PNETSIM_SOCKET s);

	//Segment Processing
	void packet_arrive_at_closed_state(PNETSIM_SOCKET s, NetSim_PACKET* p);
	void packet_arrives_at_listen_state(PNETSIM_SOCKET s, NetSim_PACKET* p);
	void packet_arrives_at_synsent_state(PNETSIM_SOCKET s, NetSim_PACKET* p);
	void packet_arrives_at_incoming_tcp(PNETSIM_SOCKET s, NetSim_PACKET* p);

	//TCP Outgoing
	void resend_segment_without_timeout(PNETSIM_SOCKET s, UINT seq);
	void send_segment(PNETSIM_SOCKET s);
	void resend_segment(PNETSIM_SOCKET s, NetSim_PACKET* expired);

	//TCP Incoming
	void update_seq_num_on_receiving(PNETSIM_SOCKET s,
									 NetSim_PACKET* p);

	//TCB
	void tcp_change_state(PNETSIM_SOCKET s, TCP_CONNECTION_STATE state);
	void create_TCB(PNETSIM_SOCKET s);
	void free_tcb(PTCB tcb);
	void delete_tcb(PNETSIM_SOCKET s);

	//Congestion Algo
	void congestion_setcallback(PNETSIM_SOCKET s);

	//Delayed Ack
	void set_ack_type(PNETSIM_SOCKET s, PTCP_DEV_VAR tcp);
	bool send_ack_or_not(PNETSIM_SOCKET s);

	//SACK
	UINT32 get_highAck(PNETSIM_SOCKET s);
	void set_highRxt(PNETSIM_SOCKET s, UINT32 seq);
	void set_rescueRxt(PNETSIM_SOCKET s, UINT32 seq);
	void tcp_sack_fastRetransmit(PNETSIM_SOCKET s);
	bool tcp_sack_lossRecoveryPhase(PNETSIM_SOCKET s);
	bool tcp_sack_isLost(PNETSIM_SOCKET s, UINT seqNum);

	//Window scaling 
	UINT8 get_shift_count(PNETSIM_SOCKET s);
	void set_window_scaling_option(PNETSIM_SOCKET s, PTCP_DEV_VAR tcp);
	UINT32 window_scale_get_cwnd(PNETSIM_SOCKET s);
	UINT16 window_scale_get_wnd(PNETSIM_SOCKET s);

	//UTILITY Function
	void print_tcp_log(char* format, ...);
	void write_congestion_plot(PNETSIM_SOCKET s, double time, UINT val);
	void tcp_create_metrics(PNETSIM_SOCKET s, bool isCongestionPlotRequire);
	TCPVARIANT get_tcp_variant_from_str(char* szVal);
	TCPACKTYPE get_tcp_ack_type_from_str(char* szVal);
	char* state_to_str(TCP_CONNECTION_STATE state);

	//App Layer interface
	void tcp_init(NETSIM_ID d);
	NetSim_PACKET* GET_PACKET_FROM_APP(bool isRemove);
	int packet_arrive_from_application_layer();
	void send_to_application(PNETSIM_SOCKET s, NetSim_PACKET* p);

	//Network layer interface
	void packet_arrive_from_network_layer();
	void send_to_network(NetSim_PACKET* packet, PNETSIM_SOCKET s);


#ifdef  __cplusplus
}
#endif
#endif //_NETSIM_TCP_H_