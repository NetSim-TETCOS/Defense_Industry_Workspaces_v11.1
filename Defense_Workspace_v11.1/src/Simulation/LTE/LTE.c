/************************************************************************************
* Copyright (C) 2014                                                               *
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
#include "LTE.h"

char* fn_NetSim_LTE_WritePacketTrace_F();

_declspec(dllexport) int fn_NetSim_LTE_Init()
{
	return fn_NetSim_LTE_Init_F();
}
_declspec(dllexport) int fn_NetSim_LTE_Run()
{
	switch(pstruEventDetails->nEventType)
	{
	case MAC_OUT_EVENT:
		{
			switch(pstruEventDetails->nDeviceType)
			{
			case UE:
LTE_UE_MAC_OUT:
				{
					switch(pstruEventDetails->nSubEventType)
					{
					case 0:
						//From network layer
						fn_NetSim_LTE_AddInQueue();
						break; //case 0
					default:
						fnNetSimError("unknown subevent for LTE-UE-MAC_OUT");
						break; //default
					}
				}
				break; //UE
			case eNB:
LTE_ENB_MAC_OUT:
				{
					NetSim_BUFFER* buffer=DEVICE_MAC_NW_INTERFACE(pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId)->pstruAccessBuffer;
					NetSim_PACKET* packet=fn_NetSim_Packet_GetPacketFromBuffer(buffer,0);
					switch(packet->nControlDataType)
					{
					case LTEPacket_HandoverRequest:
						fn_NetSim_LTE_ProcessHORequest();
						break;
					case LTEPacket_HandoverRequestAck:
						fn_NetSim_LTE_ProcessHORequestAck();
						break;
					default:
						fn_NetSim_LTE_eNB_AddInQueue();
						break;
					}
				}
				break; //eNB
			case RELAY:
				{
					if(pstruEventDetails->nInterfaceId==get_UE_Interface(pstruEventDetails->nDeviceId))
						goto LTE_UE_MAC_OUT;
					else
						goto LTE_ENB_MAC_OUT;
				}
				break;
			default:
				fnNetSimError("Unknown device type for LTE-MAC_OUT");
				break; //default
			}
		}
		break; //MAC_OUT
	case MAC_IN_EVENT:
		{
			switch(pstruEventDetails->nDeviceType)
			{
			case eNB:
LTE_ENB_MAC_IN:
				{
					NetSim_PACKET* packet=pstruEventDetails->pPacket;
					switch(packet->nControlDataType)
					{
					case LTEPacket_RRC_CONNECTION_REQUEST:
						fn_NetSim_LTE_RRC_ProcessRequest();
						break;
					case LTEPacket_RRC_CONNECTION_SETUP_COMPLETE:
						fn_NetSim_LTE_RRC_SetupComplete();
						break;
					case LTEPacket_RLC_SDU:
						fn_NetSim_LTE_RLC_eNB_In();
						break;
					case LTEPacket_MeasurementReport:
						fn_NetSim_LTE_DecideHandover();
						break;
					case LTEPacket_ACK:
						fn_NetSim_LTE_ENB_ProcessAck();
						break;
					default:
						fnNetSimError("Unknown packet for LTE-ENB-Macin");
						break;
					}
				}
				break; //enB
			case UE:
LTE_UE_MAC_IN:
				{
					NetSim_PACKET* packet=pstruEventDetails->pPacket;
					switch(packet->nControlDataType)
					{
					case LTEPacket_RRC_CONNECTION_SETUP:
						fn_NetSim_LTE_RRC_ProcessSetup();
						break;
					case LTEPacket_Paging:
						fn_NetSim_LTE_ProcessPage();
						break;
					case LTEPacket_RLC_SDU:
						fn_NetSim_LTE_RLC_UE_In();
						break;
					case LTEPacket_ACK:
						fn_NetSim_LTE_UE_ProcessAck();
						break;
					default:
						fnNetSimError("Unknown packet for LTE-UE-Macin");
						break;
					}
				}
				break; //UE
			case RELAY:
				{
					if(pstruEventDetails->nInterfaceId==get_UE_Interface(pstruEventDetails->nDeviceId))
						goto LTE_UE_MAC_IN;
					else
						goto LTE_ENB_MAC_IN;
				}
				break;
			default:
				fnNetSimError("Unknown device type for LTE-MAC_IN");
				break; //default
			}
		}
		break; //MAC_IN
	case PHYSICAL_OUT_EVENT:
		{
			NetSim_PACKET* packet=pstruEventDetails->pPacket;
			packet->pstruPhyData->dArrivalTime=pstruEventDetails->dEventTime;
			packet->pstruPhyData->dEndTime = pstruEventDetails->dEventTime;
			packet->pstruPhyData->dOverhead=0;
			packet->pstruPhyData->dPayload=packet->pstruMacData->dPacketSize;
			packet->pstruPhyData->dPacketSize=packet->pstruMacData->dPacketSize;
			packet->pstruPhyData->dStartTime=pstruEventDetails->dEventTime;
			packet->pstruPhyData->nPhyMedium=PHY_MEDIUM_WIRELESS;

			if (!fn_NetSim_HWI_Send(packet))
			{
				pstruEventDetails->nDeviceId = packet->nReceiverId;
				pstruEventDetails->nInterfaceId = get_rcv_interface_id(packet);
				pstruEventDetails->nDeviceType = DEVICE_TYPE(packet->nReceiverId);
				pstruEventDetails->nEventType = PHYSICAL_IN_EVENT;
				fnpAddEvent(pstruEventDetails);
			}
		}
		break; //PHYSICAL_OUT
	case PHYSICAL_IN_EVENT:
		{
			unsigned int carrier_index = get_carrier_index(pstruEventDetails->pPacket);
			if(pstruEventDetails->nDeviceType==eNB)
			{
				LTE_ASSOCIATEUE_INFO* info=fn_NetSim_LTE_FindInfo(get_enb_mac(pstruEventDetails->nDeviceId,1),pstruEventDetails->pPacket->nTransmitterId);
				if(!info)
				{
					//Handover is in progress
					pstruEventDetails->pPacket->nPacketStatus = PacketStatus_Dropped;
					fn_NetSim_WritePacketTrace(pstruEventDetails->pPacket);
					fn_NetSim_Metrics_Add(pstruEventDetails->pPacket);
					return -1;
				}
			}
			if(pstruEventDetails->nSubEventType == SUBEVENT_HWIRECV ||
			   fn_NetSim_LTE_DecideError(carrier_index))
			{
				pstruEventDetails->nEventType=MAC_IN_EVENT;
				fnpAddEvent(pstruEventDetails);
			}
			fn_NetSim_WritePacketTrace(pstruEventDetails->pPacket);
			fn_NetSim_Metrics_Add(pstruEventDetails->pPacket);
		}
		break; //PHYSICAL_IN
	case TIMER_EVENT:
		{
			switch(pstruEventDetails->nSubEventType)
			{
			case LTE_T300_Expire:
				fn_NetSim_LTE_T300_Expire();
				break;
			case LTE_TXNEXTFRAME:
				fn_NetSim_LTE_FormNextFrame();
				break;
			case UPDATE_D2D_PEERS:
				fn_NetSim_D2D_UpdatePeers();
				break;
			default:
				fnNetSimError("Unknown subevent for LTE-Timer_event");
				break;
			}
		}
		break;//TIMER_EVENT
	case NETWORK_OUT_EVENT:
		switch(pstruEventDetails->nDeviceType)
		{
		case MME:
			fn_NetSim_LTE_MME_RoutePacket();
			break;
		default:
			fnNetSimError("Unknown device type for LTE-Network out event");
			break;
		}
		break;
	case NETWORK_IN_EVENT:
		switch(pstruEventDetails->nDeviceType)
		{
		case MME:
			{
				switch(pstruEventDetails->pPacket->nControlDataType)
				{
				case LTEPacket_HandoverRequestAck:
				case LTEPacket_HandoverRequest:
					fn_NetSim_LTE_MME_RouteHOPacket();
					break;
				default:
					//No processing pass to IP layer
					break;
				}
			}
			break;
		default:
			//No processing pass to IP layer
			break;
		}
		break;
	default:
		fnNetSimError("Unknown event type for LTE");
		break; //default
	}
	return 0;
}
_declspec(dllexport) char* fn_NetSim_LTE_Trace(NETSIM_ID id)
{
	return fn_NetSim_LTE_Trace_F(id);
}
_declspec(dllexport) int fn_NetSim_LTE_FreePacket(NetSim_PACKET* packet)
{
	return fn_NetSim_LTE_FreePacket_F(packet);
}
_declspec(dllexport) int fn_NetSim_LTE_CopyPacket(const NetSim_PACKET* destPacket,const NetSim_PACKET* srcPacket)
{
	return fn_NetSim_LTE_CopyPacket_F(destPacket,srcPacket);
}
_declspec(dllexport) int fn_NetSim_LTE_Metrics(PMETRICSWRITER file)
{
	return fn_NetSim_LTE_Metrics_F(file);
}
_declspec(dllexport) int fn_NetSim_LTE_Configure(void** var)
{
	return fn_NetSim_LTE_Configure_F(var);
}
_declspec(dllexport) char* fn_NetSim_LTE_ConfigPacketTrace()
{
	return fn_NetSim_LTE_ConfigPacketTrace_F();
}
_declspec(dllexport) int fn_NetSim_LTE_Finish()
{
	return fn_NetSim_LTE_Finish_F();
}
_declspec(dllexport) char* fn_NetSim_LTE_WritePacketTrace()
{
	return fn_NetSim_LTE_WritePacketTrace_F();
}