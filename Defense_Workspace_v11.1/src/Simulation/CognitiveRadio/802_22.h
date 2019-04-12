/************************************************************************************
 * Copyright (C) 2013                                                               *
 * TETCOS, Bangalore. India                                                         *
 *                                                                                  *
 * Tetcos owns the intellectual property rights in the Product and its content.     *
 * The copying, redistribution, reselling or publication of any or all of the       *
 * Product or its content without express prior written consent of Tetcos is        *
 * prohibited. Ownership and / or any other right relating to the software and all *
 * intellectual property rights therein shall remain at all times with Tetcos.      *
 *                                                                                  *
 * Author:    Shashi Kant Suman                                                       *
 *                                                                                  *
 * ---------------------------------------------------------------------------------*/
#ifndef _NETSIM_802_22_H_
#define _NETSIM_802_22_H_
//For MSVC compiler. For GCC link via Linker command 
#pragma comment(lib,"CognitiveRadio.lib")
#pragma comment(lib,"Metrics.lib")
#pragma comment(lib,"NetworkStack.lib")
#pragma comment(lib,"PropagationModel.lib")

#include "ErrorModel.h"
#define CHANNEL_LOSS	-1.7
//2 bits binary number
#define B2_00	0
#define B2_01	1
#define B2_10	2
#define B2_11	3

//3 bits binary number
#define B3_000	0
#define B3_001	1
#define B3_010	2
#define B3_011	3
#define B3_100	4
#define B3_101	5
#define B3_110	6
#define B3_111	7

//4 bits binary number
#define B4_0000	0
#define B4_0001	1
#define B4_0010	2
#define B4_0011	3
#define B4_0100	4
#define B4_0101	5
#define B4_0110	6
#define B4_0111	7
#define B4_1000	8
#define B4_1001	9
#define B4_1010	10
#define B4_1011	11
#define B4_1100	12
#define B4_1101	13
#define B4_1110	14
#define B4_1111	15

/** 16 bit Binary number */
#define B16_1111111111111111 65535

/** Mac Subheader bit
   Table 4, IEEE802.22-2011-page 36
 */
#define UPSTREAM_GRANT_MANAGEMENT_BIT	0
#define FRAGMANET_BIT					1
#define EXTENDED_BIT					2
#define ARQ_FEEDBACK_BIT				3
#define BW_REQUEST_BIT					4

/* Seed value for Incumbent operation */
static unsigned long ulIncumbentSeed1 = 12345678;
static unsigned long ulIncumbentSeed2 = 23456789;

#define MAX_FID 8
#define MAX_SID 512
//Typedef for struct
typedef struct stru_802_22_Super_Frame_Control_Header SCH;
typedef struct stru_802_22_Frame_Control_Header FCH;
typedef struct stru_802_22_General_MAC_Header GMH;
typedef struct stru_802_22_FragmentSubHeader FRAGMENT_SUB_HEADER;
typedef struct stru_802_22_BS_MAC BS_MAC;
typedef struct stru_802_22_BS_Phy BS_PHY;
typedef struct stru_802_22_CPE_MAC CPE_MAC;
typedef struct stru_802_22_CPE_PHY CPE_PHY;
typedef struct stru_802_22_Incumbent INCUMBENT;
typedef struct stru_802_22_SymbolParameter SYMBOL_PARAMETER;
typedef struct stru_802_22_DSMAP DSMAP;
typedef struct stru_802_22_DSMAP_IE DSMAP_IE;
typedef struct stru_802_22_UCD_Message_Format UCD;
typedef struct stru_802_22_DCD_Message_Format DCD;
typedef struct stru_802_22_DownStreamBurstProfileFormat DBPF;
typedef struct stru_802_22_USMAP_MessageFormat USMAP;
typedef struct stru_802_22_DSBurst DS_BURST;
typedef struct stru_802_22_USMAPIE USMAP_IE;
typedef struct stru_802_22_Service CR_SERVICE;
typedef struct stru_DSx_IE DSX_IE;
typedef struct stru_DSA_REQ DSA_REQ;
typedef struct stru_DSA_RSP DSA_RSP;
typedef struct stru_DSD_REQ DSD_REQ;
typedef struct stru_DSD_RSP DSD_RSP;
typedef struct stru_BWRequestSubHeader BW_REQUEST;
typedef struct stru_802_22_UplinkAlloctioninfo UPLINKALLOCINFO;
typedef struct stru_802_22_UCD_IE UCD_IE;
typedef struct stru_802_22_SSFOutput SSF_OUTPUT;
typedef struct stru_802_22_SSFInput SSF_INPUT;
typedef struct stru_NetSim_CR_BS_Metrics BS_METRICS;
typedef struct stru_NetSim_CR_Incumbent_Metrics INCUMBENT_METRICS;
typedef struct stru_NetSim_CR_Channel_Metrics CHANNEL_METRICES;
typedef struct stru_NetSim_CR_CPE_Metrics CPE_METRICS;

//Typedef for enum
typedef enum enum_802_22_MAC_Management_Message MANAGEMENT_MESSAGE;
typedef enum enum_802_22_Modulation MODULATION_TECHNIQUE;
typedef enum enum_802_22_CodingRate CODING_RATE;
typedef enum enum_802_22_ChannelState CHANNEL_STATE;
typedef enum enum_802_22_IncumbentStatus INCUMBENT_STATUS;
typedef enum enum_802_22_SubEvent SUBEVENT;

# define SCH_SIZE	45	///< SCH Size is 45 Bytes
# define FCH_SIZE	3	///< FCH Size is 3 Bytes
# define GMH_SIZE	4	///< GMH Size is 4 Bytes
# define CRC		4	///< CRC Size is 4 Bytes
# define FRAGMENT_SIZE 3	///< Fragment size is 3 Bytes
# define MAX_SDU_SIZE 255	///< SDU Size is 255 Bytes
#define SCH_DURATION 160000	///< SCH Duration is 160 milli seconds
#define SCH_PHY_MODE 2
#define FCH_PHY_MODE 4
#define MAX_PHY_MODE 17
#define DUPLEX_TDD 1
#define MULTIPLEACCESS_OFDMA 1

unsigned int g_FragmentId;

/** Enumeration for channel set */
enum enum_802_22_ChannelState
{
	ChannelState_DISALLOWED,
	ChannelState_OPERATING,
	ChannelState_BACKUP,
	ChannelState_CANDIDATE,
	ChannelState_PROTECTED,
	ChannelState_UNCLASSIFIED,
};
/** Enumeration for 802.22 Subevent*/
enum enum_802_22_SubEvent
{
	INCUMBENT_OPERATION_START=MAC_PROTOCOL_IEEE802_22*100+1,
	INCUMBENT_OPERATION_END,
	TRANSMIT_SCH,
	FORM_DS_BURST,
	TRANSMIT_DS_BURST,
	FORM_US_BURST,
	DSA_RSP_TIMEOUT,
	DSA_RVD_TIMEOUT,
	TRANSMIT_US_BURST,
	TRANSMIT_US_BURST_CONTROL,
	QUIET_PERIOD,

	/* Spectrum Manager subevent
	 * Table 234 IEEE802.22-2011
	 */
	SM_DBS,
	SM_SignalDetected,
	SM_Geolocation,
	SM_UPDATECHANNEL,
};
/** Enumeration for MAC MANAGEMENT MESSAGE
  Source: IEEE802.22-2011, table 19, page 46-47
 */
enum enum_802_22_MAC_Management_Message
{
	MMM_DCD=0,		/**< Downstream channel descriptor, defines the characteristics of downstream physical channel. 
						Transmitted by BS at periodic interval. */
	MMM_DS_MAP,		///< Downstream access definition, defines the access to the downstream information.
	MMM_UCD,		/**< Upstream channel descriptor, defines the characteristics of upstream physical channel. 
						Transmitted by BS at periodic interval.*/
	MMM_US_MAP,		///< Upstream access definition, defines the access to the upstream information.	
	MMM_RNG_REQ,	///< Not implemented
	MMM_RNG_CMD,	///< Not implemented
	MMM_REG_REQ,	///< Not implemented
	MMM_REG_RSP,	///< Not implemented
	MMM_DSA_REQ,	///< Dynamic service addition request, This message is sent by BS or CPE to create new service request.
	MMM_DSA_RSP,	///< This message is generated in response to DSA-REQ message.
	MMM_DSA_ACK,	///< Dynamic service addition acknowledgement
	MMM_DSC_REQ,	///< Dynamic service change request	
	MMM_DSC_REP,	///< Dynamic service change reply.
	MMM_DSC_ACK,	///< Dynamic service change Acknowledgement.
	MMM_DSD_REQ,	///< Dynamic service deletion request
	MMM_DSD_REP,
	MMM_DSX_RVD,
	MMM_MCA_REQ,	///< Not implemented	
	MMM_MCA_RSP,	///< Not implemented
	MMM_CBC_REQ,	///< Not implemented
	MMM_CBC_RSP,	///< Not implemented
	MMM_DREG_CMD,	///< Not implemented
	MMM_DREG_REQ,	///< Not implemented
	MMM_ARQFeedback,///< Not implemented
	
	MMM_ARQ_Discard,///< Not implemented	
	
	MMM_ARQ_Reset,	///< Not implemented

	MMM_CHS_REQ,	///< Channel switch request, Sent by BS in order to switch the entire cell operation to new channel.
	MMM_CHS_REP,
	
	MMM_CHQ_REQ,	///< Channel quiet request
	MMM_CHQ_REP,
	
	MMM_IPC_UPD,	///< Not implemented
	
	MMM_BLM_REQ,	///< Bulk measurement request
	MMM_BLM_RSP,
	
	MMM_BLM_REP,	///< Bulk measurement report
	MMM_BLM_ACK, 
	
	MMM_TFTP_CPLT,	///< Not implemented
	
	MMM_TFTP_RSP,	///< Not implemented
	
	MMM_SCM_REQ,	///< Not implemented
	
	MMM_SCM_RSP,	///< Not implemented
	
	MMM_FRM_UPD,	///< Not implemented
	
	MMM_CBP_RLY,	///< Not implemented

	/**NOT IN STANDARD. Included here for commonality*/
	MMM_FCH,
	MMM_SCH,
	MMM_BW_REQUEST,
	MMM_UCS_NOTIFICATION,
};
#define CR_CONTROL_PACKET(MMM) MAC_PROTOCOL_IEEE802_22*100+MMM

/** Enumeration for coding rate */
enum enum_802_22_CodingRate
{
	Coding_UNCODED,
	Coding_1_2_REP4,
	Coding_1_2_REP3,
	Coding_1_2_REP2,
	Coding_1_2,
	Coding_2_3,
	Coding_3_4,
	Coding_5_6,
};


/** Enumeration for Incumbent operational status */
enum enum_802_22_IncumbentStatus
{
	IncumbentStatus_NONOPERATIONAL,
	IncumbentStatus_OPERATIOAL,
};

/** Super frame control header structure.
  Ref: IEEE802.22-2011 Table 1, page 31-34
 */
struct stru_802_22_Super_Frame_Control_Header
{
	PNETSIM_MACADDRESS sz_BS_Id; ///< 48 bits, Base station mac address	
	unsigned short int n_FrameAllocationMap; /**< 
												16 bits, Indicates which frame of current superframe
												 is allocated to BS transmitting SCH
											 */
	
	unsigned int n_SuperframeNumber:8; /**<
											8 bits, Positive integer that represents the superframe number (modulo 256).
											This field shall be incremented by 1 upon every new superframe.  
										*/
	
	unsigned int n_CP:2; /**<
							~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
							2 bits, Cyclic Prefix Factor
							Specifies the size of the cyclic prefix used by the PHY in the frame
							transmissions in this superframe. Pre-determined values are
								00: 1/4 TFFT
								01: 1/8 TFFT
								10: 1/16 TFFT
								11: 1/32 TFFT
							~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
						 */
	
	unsigned int n_FCH_EncodingFlag:2;  /**<
											~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
											Encoding Flag 2 bit
													00: FCH packet encoded with PHY mode 5
													11: FCH packet encoded with PHY mode 4
											~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~		
										 */
	
	unsigned int n_Self_coexistence_Capability_Indicator:4; /**<
																~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
																		4 bits 
																				0000: no self-coexistence capability supported
																				0001: only Spectrum Etiquette
																				0010: Spectrum Etiquette and Frame Contention
																				0011–1111: Reserved
																~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
															*/
	
	unsigned int n_MAC_version:8; /**< 
										~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
										8 bits IEEE 802.22 MAC version to which the message originator conforms.
																		0x01: IEEE Std 802.22
																		0x02–0xFF: Reserved
										~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
									*/
	
	unsigned int n_Current_Intra_frame_Quiet_Period_Cycle_Length:8; /**<
																			8 bits Specified in number of superframes, it indicates the spacing between the
																			superframes for which the intra-frame quiet period specification is valid.
																			For example, if this field is set to 1, the Quiet Period Cycle repeats every
																			superframe; if it is set to 2, the Quiet Period Cycle repeats every 2
																			superframes, etc.
																			If this field is set to 0, no intra-frame quiet period is scheduled or the
																			current intra-frame quiet period is cancelled.
																		*/
	
	unsigned int n_Current_Intra_frame_Quiet_Period_Cycle_Offset:8; /**< 
																		 8 bits Valid only if Current Intra-frame Quiet period Cycle Length > 0.
																		 Specified in number of superframes, it indicates the offset from this SCH
																		 transmission to the beginning of the first superframe in the Current Intraframe
																		 Quiet period Cycle Length.
																	*/
																	
	char sz_Current_Intra_frame_Quiet_period_Cycle_Frame_Bitmap[17]; /**< 
																		16 bits Valid only if Current Intra-frame Quiet Period Cycle Length > 0.
																		Valid for each superframe identified by the Current Intra-frame Quiet
																		Period Cycle Length, each bit in the bitmap corresponds to one frame
																		within the superframe. If the bit is set to 0, no intra-frame quiet period
																		shall be scheduled in the corresponding frame. If the bit is set to 1, an
																		intra-frame quiet period shall be scheduled within the corresponding
																		frame for the duration specified by the Current Intra-frame Quiet period
																		Duration.
																	*/		
	
	unsigned int n_Current_Intra_frame_Quiet_Period_Duration:8;	/**< 
																	8 bits Valid only if Current Intra-frame Quiet Period Cycle Length > 0.
																	If this field is set to a value different from 0 (zero), it indicates the
																	number of symbols starting from the end of the frame during which no
																	transmission shall take place.
																*/
																										
	unsigned int n_Claimed_Intra_frame_Quiet_Period_Cycle_Length:8; /**< 
																		8 bits Specified in number of superframes, it indicates the spacing between the
																		superframes for which the intra-frame quiet period specification claimed
																		by a BS would be valid. For example, if this field is set to 1, the Quiet
																		Period Cycle would repeat every superframe; if it is set to 2, the Quiet
																		Period Cycle would repeat every 2 superframes, etc.
																		If this field is set to 0, no intra-frame quiet period is claimed by the BS.
																	*/		
	
	unsigned int n_Claimed_Intra_frame_Quiet_Period_Cycle_Offset:8;	/**< 
																		8 bits Valid only if Claimed Intra-frame Quiet Period Cycle Length > 0.
																		Specified in number of superframes, it indicates the offset from this SCH
																		transmission to the time where the Claimed Quiet Period Cycle resulting
																		from the inter-BS negotiation (see 2H7.21.2) shall become the Current Intraframe
																		Quiet Period Cycle.
																	*/
	
	unsigned int n_Claimed_Intra_frame_Quiet_period_Cycle_Frame_Bitmap:16;	/**< 
																				16 bits Valid only if Claimed Intra-frame Quiet Period Cycle Length > 0.
																				Valid for each superframes identified by the Claimed Intra-frame Quiet
																				Period Cycle Length, each bit in the bitmap corresponds to one frame
																				within each specified superframe. If the bit is set to 0, no intra-frame
																				quiet period will be scheduled in the corresponding frame. If the bit is set
																				to 1, an intra-frame quiet period will be scheduled within the
																				corresponding frame for the duration specified by Claimed Intra-frame
																				Quiet period Duration.
																			*/
	
	unsigned int n_Claimed_Intra_frame_Quiet_Period_Duration:8;	/**< 
																	8 bits Valid only if Claimed Intra-frame Quiet Period Cycle Length > 0.
																	If this field is set to a value different from 0 (zero): it indicates the
																	number of symbols starting from the end of the frame during which no
																	transmission will take place.
																*/
	
	unsigned int n_Synchronization_Counter_for_Intra_frame_Quiet_Period_Rate:8; /**< 
																					8 bits Valid only if Claimed Intra-frame Quiet Period Cycle Length > 0.
																					This field is used for the purpose of synchronizing the Claimed Intraframe
																					Quiet Period rate among overlapping BSs in order to allow
																					dynamic reduction of the Intra-frame Quiet Period rate. This Quiet Period
																					rate is defined as the number of frames with quiet periods identified by
																					the Cycle Frame Bitmap in the superframes designated by the Cycle
																					Length, divided by this Quiet Period Cycle Length (see 23H7.21.2).
																				*/
	
	unsigned int n_Synchronization_Counter_for_Intra_frame_Quiet_Period_Duration:8;	/**< 
																						8 bits Valid only if Claimed Intra-frame Quiet Period Duration > 0.
																						This field is used for the purpose of synchronizing the Claimed Intraframe
																						Quiet Period Durations among overlapping BSs in order to allow
																						dynamic reduction of the Intra-frame Quiet Period Duration (see 24H7.21.2).
																					*/
	
	unsigned int n_Inter_frame_Quiet_Period_Duration:4;	/**<
															~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
															4 bits Duration of Quiet Period
															It indicates the duration of the next scheduled quiet period in number of
															frames.
															If this field is set to a value different from 0 (zero), it indicates the
															number of frames that shall be used to perform in-band inter-frame
															sensing.
															If this field is set to 0, no inter-frame quiet period is scheduled or the
															current inter-frame quiet period is cancelled.
															~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
														*/
	
	unsigned int n_Inter_frame_Quiet_Period_Offset:12;	/**< 
															It indicates the time span between the transmission of this information
															and the next scheduled quiet period for in-band inter-frame sensing.
															The 8 left most bits (MSB) indicate the superframe number and the 4
															right most bits (LSB) indicate the frame number when the next scheduled
															quiet period for inter-frame sensing shall start.
														*/
	
	unsigned int n_SCW_Cycle_Length:8;	/**< 
											8 bits Specified in number of superframes. If this field is set to 0, then no SCW
											cycle is scheduled. This field has to be 1 or larger to be effective. To
											limit the number of possibilities, the field shall be one of five following
											choices {1, 2, 4, 8, 16}. For example, if this field is set to 1, SCW Cycle
											repeats every superframe, if it is set to 2, SCW Cycle repeats every 2
											superframes, etc.
										*/
	
	unsigned int n_SCW_Cycle_Offset:8;	/**< 
											8 bits Specified in number of superframes, it indicates the offset from this SCH
											transmission to the superframe where the SCW cycle starts, or repeats
											(i.e., the superframe contains SCWs and is specified by the SCW Cycle
											Frame Bitmap). For example, if this field is set to 0, the SCW cycle starts
											from the current superframe. The value of the field shall be less than
											SCW Cycle length unless initial countdown. For initial countdown, this
											field can equal or be larger than SCW Cycle Length. Larger initial
											countdown gives neighboring WRANs longer time to discover and avoid
											any potential SCW reservation collision.
										*/
	
	unsigned int n_SCW_Cycle_Frame_Bitmap:32; /**<
													32 bits Valid for a unit of superframe, each 2-bit in the bitmap corresponds to
													one frame within the superframe. If the 2-bit is set to 00, this means that
													there is no SCW scheduled for this frame. If the 2-bit is set to 11, a
													reservation-based SCW (reserved by the current WRAN) is scheduled in
													the corresponding frame. If the 2-bit is set to 10, a reservation-based
													SCW has been scheduled by a direct-neighbor WRAN cell in the
													corresponding frame and needs to be avoided by other WRAN cells
													receiving this SCH. If the 2-bit is set to 01, a contention-based SCW (that
													could be shared with other WRANs) is scheduled by the current WRAN
													cell in the corresponding frame.
													The number of reservation-based SCWs cannot exceed 2 per WRAN cell
													per SCW Cycle. At least one contention-based SCW shall be scheduled
													in one SCW Cycle (code 01). The BSs shall start scheduling their
													contention-based SCWs from the last frame of the superframe, going
													backward for multiple contention-based SCWs.
													This bitmap applies only to the superframes scheduled by the SCW
													Cycle.
													NOTE—Quiet period scheduling should be done prior to the SCW
													scheduling so that SCWs avoid frames already reserved for QP. If SCW
													conflicts with QP, QP overrides the SCW.
												*/
	
	unsigned int n_Current_DS_US_Split:6;	/**< 
												6 bits Effective start time (in OFDM symbols from the start of the frame
												including all preambles) of the first symbol of the upstream allocation
												when a BS-to-BS interference situation has been identified by direct
												reception of this parameter by a BS from a SCH or a CBP burst
												transmitted by another BS. The Allocation Start Time as provided in the
												US-MAP (see 25HTable 34) shall be equal to this value if BS-to-BS
												interference has been identified. This value shall be set to zero if no BSto-
												BS interference has been identified (i.e., BS has not received this
												parameter from another BS). In this case, the Allocation Start Time in the
												US-MAP (see 26HTable 34) can be defined independently on a frame-byframe
												basis by the respective BSs based on their traffic requirement.
											*/
	
	unsigned int n_Claimed_US_DS_Split:6;	/**< 
												6 bits Specified by each BS in the case of BS-to-BS interference (i.e., when
												SCH and/or CBP burst can be received by a BS directly from another
												BS) indicating the required DS/US split based in the traffic requirement
												of the transmitting BS and the negotiation process between the BSs (see
												27H7.20.3). This value shall be set to zero if no BS-to-BS interference has
												been identified.
											*/
	
	unsigned int n_DS_US_Change_Offset:12;	/**< 
												12 bits It indicates the time span between the transmission of this information
												and the next scheduled change of the DS/US split where the “Claimed
												DS/US split” value will become the “Current DS/US split” value. The 8
												left most bits (MSB) indicate the superframe number and the 4 right most
												bits (LSB) indicate the frame number when the next DS/US split change
												shall take place. The value of this parameter is determined by the
												negotiation process between concerned BSs (see 28H7.20.3). This value shall
												be set to zero if no BS-to-BS interference has been identified.
											*/
	
	unsigned int n_Incumbent_detection_reporting_inhibit_timer:32;	/**< 
																		32 bits In the case where the BS is informed by the database service that it can
																		continue operating on the current channel even though its CPEs are
																		repetitively reporting an incumbent detection situation (i.e., on N or
																		N±1), the BS can use this parameter to inhibit such reporting by the CPEs
																		for a specified period of time. This will avoid the CPEs flooding the
																		upstream subframe with unnecessary incumbent detection reports.
																		Bit 0–4: Signal type (see 29HTable 237)
																		Bit 5–31: Inhibit Period (number of frames)
																	*/
	
	unsigned int n_HCS:8;	///< 8 bits Header Check Sequence
	
	unsigned int n_Padding_bits;	/**< 
									56 bits Padding bits to fill the rest of the 360 bits of the SCH symbol. All bits
									shall be set to 0.
									*/
};


/** Frame control header structure.
	Ref: IEEE802.22-2011 Table 2, page 34
 */
struct stru_802_22_Frame_Control_Header
{	
	unsigned int n_Length_of_the_frame:6; /**< 
												6 bits Indicates the length of the frame in number
												of OFDM symbols from the start of the frame including all preambles. */
	
	unsigned int n_Length_of_the_MAP_message:10;	/**< 
														10 bits This field specifies the length of the MAP
														information element following the FCH in
														OFDM slots. A length of 0 (zero) indicates
														the absence of any burst in the frame.
													*/
	
	unsigned int n_HCS:8;	///< 8 bits Header Check Sequence
};

/** Generic MAC header structure.
   Ref: IEEE802.22-2011 Table 3, page 36
 */
struct stru_802_22_General_MAC_Header
{
	
	unsigned int n_Length:11;	/**< 
									11 bits The length in bytes of the MAC PDU including the MAC
									header and the CRC. Any length smaller than 4 bytes shall be
									reserved for future use and the data shall be discarded by
									receivers only able to comply with this standard release.
								*/
	
	unsigned int n_UCS:1;	/**<
								~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
								  1 bit Urgent Coexistence Situation
								  Used by the CPE to alert the BS about an UCS with
								  incumbents in the channel currently being used by the BS or
								  either of its adjacent channels:
												0: no incumbent (default)
												1: incumbent detected
								~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
							*/
	
	unsigned int n_QPA:1;	/**< 
							~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
							   1 bit Quiet Period Adjustment sent by the CPE when it is missing
							   QP opportunities to complete its in-band sensing within the
							   required in-band detection time (see 245H10.3.3):
											0: no adjustment needed (default)
											1: increase of the number of quiet periods is needed
							~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
							*/
	
	unsigned int n_EC:1;	/**<
							~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
							1 bit Encryption control:
										0: payload is not encrypted
										1: payload is encrypted
							~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
							*/
	
	unsigned int n_EKS:2;	/**< 
								2 bits Encryption key sequence
								The index of the traffic encryption key (TEK) and
								initialization vector used to encrypt the payload. This field is
								only meaningful if the EC field is set to 1. When transitioning
								to newer TEK/GTEK (see 246H8.3.1.4 and 247H8.3.1.5), EKS is
								incremented +1 (modulo 4).
							*/
	
	char sz_Type[6];	/**< 5 bits Indicates the subheaders and special payload types present in the message payload.*/	
	
	unsigned int n_FID:3; ///< 3 bits Flow ID
	
	unsigned int n_HCS:8;	/**<
							~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
								8 bits Header check sequence
								The transmitter shall calculate the HCS value for the content
								of the header excluding the HCS field, and insert the result
								into the HCS field which is the last byte of the header). It
								shall be the remainder of the division (Modulo 2) by the
								generator polynomial g(D = D8 + D2 + D + 1) of the
								polynomial D8 multiplied by the content of the header
								excluding the HCS field. (Example: [Length=0x447, UCS=0,QPA=0, EC=1, EKS=01, Type=11001, FID=010; 
								the resulting GMH=0x88E5CB and the HCS calculated over it =	0x27]).
							~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
							*/	
	void* subheader[5];		///< Subheader
};
/** Structure for fragment sub header */
struct stru_802_22_FragmentSubHeader
{
	unsigned int purpos:1;
	unsigned int FC:2;
	unsigned int FSN:10;
	unsigned int length:11;
	unsigned int nFragmentId; //NetSim Variable to keep track of fragment
};
/** Structure for up link allocation information. */
struct stru_802_22_UplinkAlloctioninfo
{
	unsigned int nSID;
	unsigned int nSlotRequested;
	unsigned int nSlotAllocated;
	SERVICE_TYPE nQoS;
	struct stru_802_22_UplinkAlloctioninfo* next;
};
/** CR_BS Metrics structure */
struct stru_NetSim_CR_BS_Metrics
{
	NETSIM_ID nBSID;
	unsigned int nSCHSent;
	unsigned int nFCHSent;
	unsigned int nDSA_REQReceived;
	unsigned int nDSA_REPSent;
	unsigned int nDSD_REQReceived;
	unsigned int nDSC_REQReceived;
	unsigned int nDSC_REPSent;
	unsigned int nDSD_REPSent;
	unsigned int nCHS_REQSent;
};
/// Incumbent Metrics
struct stru_NetSim_CR_Incumbent_Metrics
{
	unsigned int nOperationTime;
	unsigned int nIdleTime;
	unsigned int nInterferenceTime;
	unsigned int nPrevTime;
	double dInterferenceStartTime;
};
/// Channel Metrics
struct stru_NetSim_CR_Channel_Metrics
{
	unsigned int nChannelNumber;
	char* szFrequency;
	double dEfficiency;
	double dUsage;
};
/// CPE Metrics
struct stru_NetSim_CR_CPE_Metrics
{
	NETSIM_ID nCPEID;
	unsigned int nSCHReceived;
	unsigned int nFCHReceived;
	unsigned int nDSA_REQSent;
	unsigned int nDSC_REQSent;
	unsigned int nDSD_REQSent;
	unsigned int nDSA_REPReceived;
	unsigned int nDSC_REPReceived;
	unsigned int nDSD_REPReceived;
	unsigned int nCHS_REQReceived;
	unsigned int nUCSSent;
};

/** Base station structure for 802.22 MAC */
struct stru_802_22_BS_MAC
{
	int nDuplexMode;
	bool nSelfCoexitenceFlag;	
	int nDSXRequestRetries;		///< 3 fixed	
	int nDSXResponseReties;		///< 3 fixed	
	double T7;		///< Wait for DSA/DSC/DSD Response timeout. Max 1 sec	
	double T8;		///< Wait for DSA/DSC Acknowledge timeout. Max 300 ms	
	double T31;		///! Wait for BLM-REP timeout. >= 1MAC frame	
	double dChannelAvailabilityCheckTime;	/**< 
												The time during which a channel SHALL be
												checked for the presence of licensed incumbent
												signals having a level above the Incumbent
												Detection Threshold prior to the commencement
												of WRAN operation in that channel, and in the
												case of TV, a related channel at an EIRP level
												that can affect the measured channel
												Default- 30 sec
											*/
	
	int nNonOccupancyPeriod;		/**< 
										The required period during which WRAN
										device transmissions SHALL NOT occur in a
										given channel because of the detected presence
										of an incumbent signal in that channel above the
										Incumbent Detection Threshold or, in the case
										of TV, above a given EIRP level.
										Min - 10 min
									*/
	
	int nChannelDetectionTime;	/**< 
									The maximum time taken by a WRAN device to
									detect a licensed incumbent signal above the
									Incumbent Detection Threshold within a given
									channel during normal WRAN operation.
									Default - 2 sec
								*/	
	
	int nFalseAlramProbability;		/**< 
										In sensing modes 0 and 1 this value specifies the
										maximum probability of false alarm for each
										sensing mode decision in the signal present	array
										Range - 0.0 to 0.255
									*/
	
	int nChannelMoveTime;	/**< 
								The time taken by a WRAN system to cease all
								interfering transmissions on the current channel
								upon detection of a licensed incumbent signal
								above the relevant Incumbent Detection
								Threshold or, in the case TV, to alternatively
								reduce its EIRP to that which is allowable
								within a given channel upon detection of a TV
								signal in the same or a related channel
									Default - 2 sec
							*/		
	
	int nNumSensingPeriod;		/**< 
									Number of sensing periods field in a Sensing
									Window Specification Array entry
									Range 0-127
									Default - 1
								*/	
	
	double dSensingPeriodDuration;	/**< 
										Duration of sensing period field (in units of
										OFDM symbols) in a Sensing Window
										Specification Array entry
										Range 0-1023
										Default - 16
									*/
	
	double dSensingPeriodInterval;	/**< 
										Periodicity of Sensing period field in a Sensing
										Window Specification Array entry
										Range 0-2047
										Default 200
									*/
	
	int nSensingMode;		/**< 
								The sensing mode a CPE supports. Negotiated
								during CPE initialization.
								Default - No sensing
							*/		
	
	int nCandidateChannelRefreshTime;	/**< 
											Maximum time interval allowed before sensing
											is performed on the candidate channel to ensure
											that no incumbents are detected.
											Range 1-10 sec
											Default 6 sec
										*/
	
	int nBackupChannelRefreshTime;		/**< 
											Maximum time interval allowed before sensing
											is performed on the backup channel to ensure
											that no incumbents are detected.
											Range 1-10 sec
											Default 6 sec
										*/
	
	int nCandidateChannelTransitionTime;	/**< 
												Minimum time duration without detection of
												any incumbent for a candidate channel to
												transition to the backup channel.
												Range 1-100 sec
												Default 30 sec
											*/
	
	int nWaitBeforeChannelMove;		/**< 
										Waiting time before which the BS moves to the
										first backup channel. This is used to make sure
										that all the CPEs are ready to move to the
										backup channel before BS switches operation to
										this backup channel.
										Range 1-256*16 frames
									*/
	
	char szISOCountryCode[4];	/**< 
									3 character, ASCII string denoteing the
									regulatory domain of operation (e.g., “USA” is
									for United States of America)
								*/	
	NETSIM_ID nIncumbentCount;
	INCUMBENT** pstruIncumbent;	
	unsigned int nDSBurst; ///< Current DS burst	
	DS_BURST** pstruDSBurst; ///< DS-Burst each is one symbol long
	DSMAP* pstruDSMAP;
	USMAP* pstruUSMAP;	
	int *anSIDFromDevId; ///< Have the list of CPE associated with BS. If SID is set CPE is associated else not
	UCD* pstruUCD;
	DCD* pstruDCD;
	double dSuperframeStartTime;
	double dDSFrameTime;
	double dFrameStartTime;
	struct stru_802_22_UplinkAlloctioninfo* uplinkAllocInfo;
	unsigned int nUIUC;
	unsigned int nDIUC;
	NetSim_PACKET* pstruFragmentPacketList;
	NetSim_PACKET* pstruBroadcastPDU;
	FLAG nCHSREQFlag;
	int chsFrameCount;
	BS_METRICS struBSMetrics;
	NetSim_PACKET* pstruDSPacketList;
};
struct stru_802_22_DSBurst
{
	unsigned int nSlotCount;
	unsigned int nUnfilledSlot;
	NetSim_PACKET* pstruMACPDU;
};

/** Base structure for 802.22 Phy */
struct stru_802_22_BS_Phy
{	
	unsigned int nSuperframeNumber:8; ///< Current super frame number	
	unsigned int nFrameNumber;	///< Current super frame number	
	unsigned int nMAXFrequency;	///< Maximum frequency <=862 MHz	
	unsigned int nMINFrequency; ///! Minimum frequency >=54 MHz	
	unsigned int nChannelBandwidth; ///< 6,7,8 MHz
	double dSamplingFactor;
	PHY_MODULATION nModulation;
	CODING_RATE nCodingRate;
	int nMultipleAccess;
	int nFFT;
	unsigned int nCP:2;	
	struct stru_802_22_Channel* pstruChannelSet; ///< Channel set where BS can operate	
	struct stru_802_22_Channel* pstruOpratingChannel; ///< Currently operating channel
	int nPhyMode;
	double dTXPower;
	double dAntennaHeight;
	double dAntennaGain;
	int nSelfCoexitenceCapability;	
	double dDCDInterval;	///< Time between transmission of DCD messages. Max 10 sec	
	double dUCDInterval;	
	int nBWRequestBackoffStart;	///< Initial size of BW Request opportunity used by CPEs to contend to send BW requests to BS. Range 0-15	
	int nBWRequestBackoffEnd;	///< Final size of BW Request opportunity used by CPEs to contend to send BW requests to BS. Range 1-15
	double dUpstreamChannelChangeResponseWaitTime;	
	double dTTG;		///< Transmit Transition Gap. 105-333 micro sec. default 210 microsec
	double dRTG;       ///<Receive Transition Gap
	double dDlUlRatio;
	SYMBOL_PARAMETER* pstruSymbolParameter;
	unsigned int nIntraFrameQuietPeriodCycleLength;
	char szIntraframeQuietPeiordBitmap[17];
	unsigned int nIntraFrameQuietPeriodDuration;
	unsigned int nIFQPOffset;
};
struct stru_802_22_CPE_BWRequestInfo
{
	USMAP_IE* pstruUSMAPIE;
	double dBytesRequested;
	double dBytesAllocated;
	double dStartTime;
};
/// CPE structure for IEEE 802.22 MAC
struct stru_802_22_CPE_MAC
{
	NETSIM_ID nBSID;
	NETSIM_ID nBSInterface;
	/// Unique station idetifier
	unsigned int nSID:9; 
	/// 3 fixed
	int nDSXRequestRetries;		
	/// 3 fixed
	int nDSXResponseReties;		
	/// Wait for DSA/DSC/DSD Response timeout. Max 1 sec
	double T7;		
	/// Wait for DSA/DSC Acknowledge timeout. Max 300 ms
	double T8;		
	/// Wait for DSx-RSP/DSX-RVD Timeout. Max 200 ms
	double T14;		
	/// Wait for bandwidth request grant. Min 10 ms
	double T16;		
	/// Wait for BLM-ACK timeout. Range 10-300 ms
	double T29;
	/// Number of retries allowed for sending BLMREP. Default 3
	int nBLM_REPRetries; 
	struct stru_802_22_CPE_BWRequestInfo *BWRequestInfo;
	NetSim_PACKET* pstruQueuedPacketList[MAX_FID];
	CR_SERVICE* pstruServiceParameter;
	NetSim_PACKET* pstruUSBurst;
	NetSim_PACKET* pstruFragmentPacketList;
	char szCountryCode[4];
	unsigned int nMaxProbabilityofFalseAlarm;
	CPE_METRICS struCPEMetrics;
};

/// CPE structure for CPE PHY.
struct stru_802_22_CPE_PHY
{
	/// Time the CPE searches for preambles on a given channel
	double T20;		
	/// Initial size of BW Request opportunity used by CPEs to contend to send BW requests to BS. Range 0-15
	int nBWRequestBackoffStart;	
	/// Final size of BW Request opportunity used by CPEs to contend to send BW requests to BS. Range 1-15
	int nBWRequestBackoffEnd;
	/// Same parameter as BS have
	SYMBOL_PARAMETER* pstruSymbol; 
	unsigned int nIntraFrameQuietPeriodLength;
	char szIntraFrameQuietPeriodBitmap[17];
	unsigned int nIntraFrameQuietPeriodDuration;
	unsigned int nFrameNumber;
	double dTXPower;
	double dAntennaHeight;
	double dAntennaGain;
	struct stru_802_22_Channel* pstruOperatingChannel; 
	struct stru_802_22_Channel* pstruChannelSet;
};
/** Channel structure */
struct stru_802_22_Channel
{
	unsigned int nChannelNumber;
	double dUpperFrequency;
	double dLowerFrequency;
	double dChannelBandwidth;
	CHANNEL_STATE nChannelState;
	CHANNEL_METRICES struChannelMetrics;
	struct stru_802_22_Channel* pstruNextChannel;
};

/** Structure for IEEE 802.22 phy mode */
struct stru_802_22_Phy_Mode
{
	unsigned int n_PhyMode;
	MODULATION_TECHNIQUE n_Modulation;
	CODING_RATE n_CodingRate;
	double dDataRate; //In mbps
	double dSpectralEfficency;
};
extern struct stru_802_22_Phy_Mode struPhyMode[MAX_PHY_MODE];

/** Structure for symbol parameter
	Ref:IEEE802.22-2011, table 199,200,201, page 309-310
 */
struct stru_802_22_SymbolParameter
{
	// Sub carrier
	/// Channel Bandwidth in MHz
	int nChannelBandwidth;			
	double dSamplingFactor;	
	/// Sampling Frequency Fs (in MHz)
	double dBasicSamplingFrequency;	
	/// Inter carrier spacing ^f (Hz) = Fs/2048 Hz
	double dIntercarrierSpacing;	
	/// Sub carrier spacing Tfft (micro sec) = 1/^f
	double dSubCarrierSpacing;		
	/// Time Unit TU = Tfft/2048
	double dTimeUnit;				
	/// Symbol duration
	int nCP;
	/// Tcp In micro sec
	double dCPDuration;		
	/// Tsym = Tfft + Tcp in microsec
	double dSymbolDuration;			

	/// Transmission Parameter
	/// Sub carrier count Nfft = 2048
	int nSubcarrierCount;
	/// Guard Sub-carrier count Ng = 368
	int nGuardSubcarrierCount;		
	/// Used Sub-carrier count Nt = 1680
	int nUsedSubCarrierCount;		
	/// Data Sub-carrier count Nd = 1440
	int nDataSubCarrierCount;	
	/// Pilot Sub-carrier count Np = 240
	int nPilotSubCarrierCount;		

	int nSubChannelCount;
	/// OFDM Slot = 1 symbol * 1 subChannel
	int nOFDMSlots;				
	int nBitsCountInOneSlot;
	/// Data subCarrier count*Bits based on modulation * coding rate
	int nBitsCountInOneSymbol;	
	/// Number of uncoded bits/OFDM symbol duration
	double dDataRate;			
	unsigned int nUPlinkSymbol;
	unsigned int nDownLinkSymbol;
	unsigned int nUPlinkFrameStartSymbol;
};

/** Structure for WRAN frame parameter
	Ref: IEEE802.22-2011, table 203, page 313,
 */
struct stru_802_22_FrameParameter
{
	/// Channel Bandwidth in MHz
	int nChannelBandwidth;	
	double dCP;
	int nSymbolsPerFrame;
	/// Transmitter-receiver turnaround gap. in TU
	int nTTG;				
	/// Receiver_transmitter turnaround gap. in TU
	int nRTG;				
};
/// Structure for incumbent
struct stru_802_22_Incumbent
{
	int nId;
	int nStartFrequeny;
	int nEndFrequency;
	int nOperationalTime;
	int nOperationalIntervalTime;
	DISTRIBUTION nOperationalDistribution;
	INCUMBENT_STATUS nIncumbentStatus;
	double dPrevOperationalTime;
	double dPrevOperationalInterval;
	double dKeepOutDistance;
	NetSim_COORDINATES* position;
	INCUMBENT_METRICS struIncumbentMetrics;
};



/** Table 26 IEEE 802.22-2011 page 50
	DS-MAP Information Element
 */
struct stru_802_22_DSMAP_IE
{
	unsigned int DIUC:6;
	unsigned int SID:9;
	unsigned int length:12;
	unsigned int boosting:3;
};
/** Table 25 IEEE 802.22-2011 page 49
	DS-MAP Message Format
 */
struct stru_802_22_DSMAP
{
	unsigned int nManagementMessageType:8;
	unsigned int nDCDCount:8;
	unsigned int nIECount;
	struct stru_802_22_DSMAP_IE** pstruIE;
	unsigned int nPaddingBits:7;
};

/** Table 31 IEEE 802.22-2011 page 53
	UCD Channel Information Element
 */
struct stru_802_22_UCD_IE
{
	unsigned int nUpstreamBurstProfile;
	unsigned int nContentionBasedReservationTimeOut;
	unsigned int nBWRequestOpportunitySize;
	unsigned int nUCSNotificationRequestOpportunitySize;
	unsigned int nInitialRangingCode;
	unsigned int nPeriodicRangingCode;
	unsigned int nBandwidthRequestCode;
	unsigned int nUCSNotificationRequestCode;
	unsigned int nStartOfCDMACodeGroups;
};
/** Table 30 IEEE 802.22-2011 page 52
	UCD Message Format
 */
struct stru_802_22_UCD_Message_Format
{
	unsigned int nManagementMessageType:8;
	unsigned int nConfigurationChangeCount:8;
	unsigned int nBWRequestBackoffStart:4;
	unsigned int nBWRequestBackoffEnd:4;
	unsigned int nUCSNotificationBackoffStart:4;
	unsigned int nUCSNotificationBackoffEnd:4;
	UCD_IE struUCDIE;
	unsigned int nUpstreamBurstProfileCount:6;
	struct stru_802_22_Upstream_Burst_profile_Format** pstruUPStreamBurstProfile;
};

/** Table 33 IEEE 802.22-2011 Page 54
	Upstream burst profile information element
 */
struct stru_802_22_Upstream_Burst_profile_IE
{
	unsigned int nRangingDataRatio;
	unsigned int nNormalizedCNROverride;
};

/** Table 32 IEEE 802.22-2011 Page 53
	Upstream Burst profile format
 */
struct stru_802_22_Upstream_Burst_profile_Format
{
	unsigned int nType:8;
	unsigned int nLength:8;
	unsigned int nUUIC:6;
	unsigned int nReserved:2;
	struct stru_802_22_Upstream_Burst_profile_IE pstruUBPIE;
};

/** Table 21 IEEE 802.22-2011 Page 48
	DCD Channel Information Element
 */
struct stru_802_22_DCD_Channel_Information_Element
{
	unsigned int nDownStreamBurstProfile;
	unsigned int EIRP:8;
	unsigned int TTG:8;
	unsigned int RSS:8;
	unsigned int nChannelAction:3;
	unsigned int nActionMode:1;
	unsigned int nActionSuperframeNumber:8;
	unsigned int nActionFrameNumber:4;
	unsigned int nBackupChannelCount:4;
	struct stru_802_22_BackupChannelList
	{
		unsigned int nElementId:8;
		unsigned int nLength:8;
		unsigned int nChannelCount:8;
		unsigned int* nChannelNumber;
	}struBackupChannelList;
	unsigned int nMACVersion:8;
};
/** Table 20 IEEE 802.22-2011 page 47
	DCD Message Format
 */
struct stru_802_22_DCD_Message_Format
{
	unsigned int nManagementMessageType:8;
	unsigned int nConfigurationChangeCount:8;
	struct stru_802_22_DCD_Channel_Information_Element pstruDCDChannelIE;
	unsigned int nDownStreamBurstProfileCount;
	DBPF** pstruDBPF;
};
struct stru_802_22_DownStreamBurstProfileFormat
{
	unsigned int nType:8;
	unsigned int nLength:8;
	unsigned int nDIUC:6;
	unsigned int nReserved:2;
	struct stru_802_22_DBPIE
	{
		unsigned int nDIUCManadatoryExitThershold:8;
		unsigned int nDIUCMinimumEntruThershold:8;
	}struDBPIE;
};

/** Table 34 IEEE802.22-2011 Page 53
	US-MAP Message format
 */
struct stru_802_22_USMAP_MessageFormat
{
	MANAGEMENT_MESSAGE nManagementMessageType;
	unsigned int nUCDCount:8;
	unsigned int nAllocationStartTime:6;
	unsigned int nIECount:12;
	struct stru_802_22_USMAPIE** pstruUSMAPIE;
};
/** Table 35 IEEE802.22-2011 Page 55
	US-MAP Information Element
 */
struct stru_802_22_USMAPIE
{
	unsigned int nSID:9;
	unsigned int nUIUC:6;

	struct struUIUC_0_1
	{
		unsigned int nCBPFrameNumber:4;
		struct struUIUC_0
		{
			unsigned int nTimingAdvance:16;
			unsigned int nEIRPDensityLevel:8;
		}UIUC_0;
		struct struUIUC_1
		{
			unsigned int nChannelNumber:8;
			unsigned int nSynchronizationMode:1;
		}UIUC_1;
	}UIUC_0_1;
	struct struUIUC_2_3
	{
		unsigned int nSubChannelCount:4;
	}UIUC_2_3;
	struct struUIUC_4_6
	{
		unsigned int nSubChannelCount:4;
		unsigned int nSubSymbolsCount:4;
	}UIUC_4_6;
	struct struUIUC_7
	{
		void* CDMA_Allocation_IE;
	}UIUC_7;
	struct struUIUC_8
	{
		unsigned int nSubChannelCount:4;
	}UIUC_8;
	struct struUIUC_9
	{
		void* USMAP_EIRP_Control_IE;
	}UIUC_9;
	struct struUIUC_62
	{
		void* US_extended_IE;
	}UIUC_62;
	struct struUIUC
	{
		unsigned int nBurstType:1;
		unsigned int nDuration:12;
		unsigned int nMDP:1;
		unsigned int nMRT:1;
		unsigned int nCMRP:1;
	}UIUC;
};

/** Table 5, IEEE802.22-2011 page 37
	Bandwidth request sub header
 */
struct stru_BWRequestSubHeader
{
	unsigned int nType:1;
	unsigned int nBR:20;
};

PROPAGATION_HANDLE propagationHandle;
//Propagation macro
#define GET_RX_POWER_dbm(tx, rx) (propagation_get_received_power_dbm(propagationHandle, tx, 1, rx, 1, pstruEventDetails->dEventTime))
#define GET_RX_POWER_mw(tx,rx) (DBM_TO_MW(GET_RX_POWER_dbm(tx,rx)))


//Function definition
_declspec(dllexport) int fn_NetSim_CR_CopyPacket_F(const NetSim_PACKET* pstruDestPacket,const NetSim_PACKET* pstruSrcPacket);
int fn_NetSim_CR_TransmitP2PPacket(NetSim_PACKET* pstruPacket,NETSIM_ID nDevId,NETSIM_ID nInterface);
_declspec(dllexport) int fn_NetSim_CR_Configure_F(void** var);
_declspec(dllexport) int fn_NetSim_CR_Init_F(struct stru_NetSim_Network *NETWORK_Formal,\
 NetSim_EVENTDETAILS *pstruEventDetails_Formal,char *pszAppPath_Formal,\
 char *pszWritePath_Formal,int nVersion_Type,void **fnPointer);
_declspec(dllexport) int fn_NetSim_CR_FreePacket_F(const NetSim_PACKET* pstruPacket);
int fn_NetSim_CR_Metrics_F(PMETRICSWRITER metricsWriter);
_declspec(dllexport) int fn_NetSim_CR_Finish_F();
int fn_NetSim_CR_PacketArrive();
int fn_NetSim_CR_GetFID(QUALITY_OF_SERVICE nQOS);
QUALITY_OF_SERVICE fnGetQOS(char* Qos);
int fn_NetSim_CR_AllocBandwidth(NETSIM_ID nSID,QUALITY_OF_SERVICE nQos,UPLINKALLOCINFO** ppstruInfo,unsigned int nSlotRequired,unsigned int nTotalSlot);
int fn_NetSim_Check_Interference(unsigned int nChannelNumber,
struct stru_802_22_Channel* pstruChannelList,
	unsigned int nLowerFrequency,
	unsigned int nHigherFrequency);
int fn_NetSim_CR_UpdateInfo(BS_MAC* pstruBSMAC,BS_PHY* pstruBSPHY);
int fn_NetSim_AddPacketToList(NetSim_PACKET** list,NetSim_PACKET* packet);
long long int fn_NetSim_CR_TransmitPacket(NetSim_PACKET* pstruPacket, int nDevId, int nInterfaceId, int nConDevId,int nConInterface);
double fn_NetSim_CR_CalculateTransmissionTime(double dPacketSize,SYMBOL_PARAMETER* pstruSymbolParameter);
_declspec(dllexport) char* fn_NetSim_CR_Trace_F(int nSubEvent);
int fn_NetSim_CR_InitIncumbent(BS_MAC* pstruBSMAC, NETSIM_ID nDeviceId,NETSIM_ID nInterfaceId);
int fn_NetSim_CR_IniScanChannel(BS_MAC* pstruBSMAC,BS_PHY* pstruBSPhy);
int fn_NetSim_CR_UpdateOperatingChannel(BS_PHY* pstruBSPhy);
int fn_NetSim_CR_StartSCH(int nBTSId,int nInterfaceId);
int fnIsinRange(double l1, double u1, double l2, double u2);
int fn_NetSim_CR_ConfigIncumbent(void* xmlNetSimNode,BS_MAC* pstruBSMAC);
int fn_NetSim_CR_BroadCastPacket(NetSim_PACKET* pstruPacket,NETSIM_ID nDevId,NETSIM_ID nInterface);
int fn_NetSim_CR_MulticastPacket(NetSim_PACKET* pstruPacket, NETSIM_ID nDevId, NETSIM_ID nInterface);
NetSim_PACKET* fn_NetSim_CR_GenerateBroadcastCtrlPacket(int nDeviceId,int nInterfaceId,MANAGEMENT_MESSAGE nMessageType);
int fn_NetSim_CR_FillDSFrame(unsigned int size/*Bytes*/,DS_BURST** pstruDSBurst,SYMBOL_PARAMETER* pstruSymbolParameter,int nFlag,unsigned int* nSlotRequire);
int fn_NetSim_CR_AddPacketToDSBurst(DS_BURST* pstruDSBurst,NetSim_PACKET* pstruPacket);
NetSim_PACKET* fn_NetSim_CR_UNFillSlot(NETSIM_ID nDeviceId, NETSIM_ID nInterfaceId, unsigned int nBurstId);
int fn_NetSim_CR_UpdateIncumbentMetrics(BS_MAC* pstruBSMAC,BS_PHY* pstruBSPHY,double dTime);

//802_22 dll function
int fn_NetSim_CR_BS_MACIN();
int fn_NetSim_CR_CPE_PhysicalOut();
_declspec(dllexport) char* fn_NetSim_CR_Trace(int nSubEvent);
int fn_NetSim_CR_BS_AllocateBandwidth();
int fn_NetSim_CR_ScanChannel(BS_MAC* pstruBSMAC,BS_PHY* pstruBSPhy);
int fn_NetSim_CR_FormUSBurst();
int fn_NetSim_CR_FormChannelSet(BS_PHY* pstruBSPhy);
int fn_NetSim_Init_OFDMA(BS_PHY* pstruBSPhy);
int fn_NetSim_CR_SSA_Initialization(BS_MAC* pstruBSMAC,BS_PHY* pstruBSPHY);
int fn_NetSim_CR_AssociateCPE(NETSIM_ID nDeviceId,NETSIM_ID nInterfaceId);
int fn_NetSim_CR_FragmentPacket(NetSim_PACKET* pstruPacket,double dSDUSize);
int fn_NetSim_CR_CreateServiceFlow(NETSIM_ID nDeviceId,NETSIM_ID nInterfaceId,int nId,NetSim_PACKET* packet,double dTime);
int fn_NetSim_CR_IncumbentStart();
int fn_NetSim_CR_IncumbentEnd();
int fn_NetSim_CR_TransmitSCH();
int fn_NetSim_CR_TransmitFCH();
int fn_NetSim_CR_TransmitDSBurst();
int fn_NetSim_CR_TransmitUSBurst();
int fn_NetSim_CR_QuietPeriod();
int fn_NetSim_CR_UpdateChannel();
int fn_NetSim_CR_CPE_ProcessSCH(NetSim_PACKET* pstruPacket);
int fn_NetSim_CR_CPE_ProcessFCH(NetSim_PACKET* pstruPacket);
int fn_NetSim_CR_CPE_ProcessDSMAP();
int fn_NetSim_CR_CPE_ProcessUSMAP();
int fn_NetSim_CR_CPE_ProcessDSA_RSP();
int fn_NetSim_CR_CPE_ProcessDSD_REP();
int fn_NetSim_CR_CPE_SwitchChannel();
int fn_NetSim_CR_CPE_PackPacket();
int fn_NetSim_CR_BS_ForwardDataPacket();
int fn_NetSim_CR_BS_ProcessDSAReq();
int fn_NetSim_CR_BS_ProcessDSDReq();
int fn_NetSim_CR_BS_UCS();
NetSim_PACKET* fn_NetSim_CR_FormDSMAP(NETSIM_ID nDeviceId, NETSIM_ID nInterfaceId);
NetSim_PACKET* fn_NetSim_CR_FormUSMAP(NETSIM_ID nDeviceId, NETSIM_ID nInterfaceId);
NetSim_PACKET* fn_NetSim_CR_FormUCD(NETSIM_ID nDeviceId,NETSIM_ID nInterfaceId);
NetSim_PACKET* fn_NetSim_CR_FormDCD(NETSIM_ID nDeviceId,NETSIM_ID nInterfaceId);
int fn_NetSim_CR_FormDSFrame(NETSIM_ID nDeviceId, NETSIM_ID nInterfaceId,double dTime);
NetSim_PACKET* fn_NetSim_CR_BS_PackPacket(BS_MAC* pstruBSMac,NetSim_PACKET* pstruPacket);
int fn_Netsim_CR_SM_ScheduleQuietPeriod(BS_PHY* pstruBSPhy,SCH* pstruSCH);
struct stru_802_22_SSFOutput* fn_NetSim_CR_CPE_SSF(struct stru_802_22_SSFInput* input,NETSIM_ID nDevId,NETSIM_ID nInterfaceId);
int fn_NetSim_TerminateServiceFlow(NETSIM_ID nDeviceId, NETSIM_ID nInterfaceId, NetSim_PACKET* pstruPacket);
int fn_NetSim_CR_CalulateReceivedPower();
#include "DSx.h"
#endif

