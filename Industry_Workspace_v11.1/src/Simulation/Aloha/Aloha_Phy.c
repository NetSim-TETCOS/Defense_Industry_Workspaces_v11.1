/************************************************************************************
 * Copyright (C) 2016                                                               *
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
#include "Aloha.h"
#include "Aloha_enum.h"

typedef struct stru_packetList
{
	NetSim_PACKET* head;
	NetSim_PACKET* tail;
}PACKETLIST, *ptrPACKETLIST;

PACKETLIST txPacketList;

static void add_packet_to_txList(NetSim_PACKET* packet)
{
	if (txPacketList.head)
	{
		txPacketList.tail->pstruNextPacket = packet;
		txPacketList.tail = packet;
	}
	else
	{
		txPacketList.head = packet;
		txPacketList.tail = packet;
	}
}

static void remove_packet_from_txList(NetSim_PACKET* packet)
{
	NetSim_PACKET* t = txPacketList.head;
	NetSim_PACKET* p = NULL;
	while (t)
	{
		if (t == packet)
		{
			if (p)
			{
				p->pstruNextPacket = t->pstruNextPacket;
				if (!t->pstruNextPacket)
					txPacketList.tail = p;
			}
			else
			{
				txPacketList.head = t->pstruNextPacket;
				if (!t->pstruNextPacket)
					txPacketList.tail = NULL;
			}
			break;
		}
		t = t->pstruNextPacket;
	}
}

static void mark_packet_for_collision()
{
	NetSim_PACKET* p = txPacketList.head;
	while (p)
	{
		PALOHA_PHY_VAR phy = ALOHA_PHY(p->nReceiverId, 1);
		
		double power = get_received_power(p->nTransmitterId, p->nReceiverId, p->pstruPhyData->dArrivalTime);
		double interference = substract_power_in_dbm(phy->dInterferencePower, power);

		if ( interference > phy->rx_sensitivity)
			p->nPacketStatus = PacketStatus_Collided;

		p = p->pstruNextPacket;
	}
}

static void update_interference_power(NETSIM_ID tx,
									  NETSIM_ID rx,
									  double time,
									  bool isAdd)
{
	NETSIM_ID i;
	PALOHA_PHY_VAR phy;
	for (i = 0; i < NETWORK->nDeviceCount; i++)
	{
		if (i + 1 == tx)
			continue;

		phy = ALOHA_PHY(i + 1, 1);
		double p = phy->dInterferencePower;
		if (isAdd)
			phy->dInterferencePower = add_power_in_dbm(p,
													   get_received_power(tx, i + 1, time));
		else
			phy->dInterferencePower = substract_power_in_dbm(p,
															 get_received_power(tx, i + 1, time));
	}
}

static double calulate_transmission_time(NetSim_PACKET* packet)
{
	double size=fnGetPacketSize(packet);
	NETSIM_ID source=packet->nTransmitterId;
	PALOHA_PHY_VAR phy = ALOHA_PHY(source,1);
	double dDataRate=phy->data_rate;
	double dTxTime= size*8.0/dDataRate;
	return dTxTime;
}

static void transmit_packet(NetSim_PACKET* packet, bool* flag)
{
	PALOHA_PHY_VAR phy = ALOHA_PHY(packet->nReceiverId,1);
	PALOHA_PHY_VAR phyt = ALOHA_PHY(packet->nTransmitterId,1);
	double power = get_received_power(packet->nTransmitterId, packet->nReceiverId,
									  packet->pstruPhyData->dArrivalTime);
	
	phyt->transmitter_status = TRX_ON_BUSY;

	if(power < phy->rx_sensitivity)
		packet->nPacketStatus=PacketStatus_Error;

	//Add physical in event
	pstruEventDetails->dEventTime=packet->pstruPhyData->dEndTime;
	packet->pstruPhyData->dEndTime = pstruEventDetails->dEventTime;
	pstruEventDetails->dPacketSize=packet->pstruPhyData->dPacketSize;
	pstruEventDetails->nDeviceId=packet->nReceiverId;
	pstruEventDetails->nDeviceType=DEVICE_TYPE(packet->nReceiverId);
	pstruEventDetails->nEventType=PHYSICAL_IN_EVENT;
	pstruEventDetails->nInterfaceId=1;
	pstruEventDetails->pPacket=packet;
	fnpAddEvent(pstruEventDetails);
	
	add_packet_to_txList(packet);
	if (!*flag)
		update_interference_power(packet->nTransmitterId,
								  packet->nReceiverId,
								  packet->pstruPhyData->dArrivalTime,
								  true);
	*flag = true;
	mark_packet_for_collision();
}

int fn_NetSim_Aloha_PhyOut()
{
	double dTxTime;
	NetSim_PACKET* packet= pstruEventDetails->pPacket;
	packet->pstruPhyData->dArrivalTime=pstruEventDetails->dEventTime;
	packet->pstruPhyData->dPayload=packet->pstruMacData->dPacketSize;
	packet->pstruPhyData->dOverhead=ALOHA_PHY_OVERHEAD;
	packet->pstruPhyData->dPacketSize=packet->pstruPhyData->dOverhead+packet->pstruPhyData->dPayload;
	packet->pstruPhyData->dStartTime=pstruEventDetails->dEventTime;

	dTxTime=calulate_transmission_time(packet);


	packet->pstruPhyData->dEndTime=packet->pstruPhyData->dStartTime+dTxTime;

	bool flag = false;
	if(packet->nReceiverId)
	{
		transmit_packet(packet, &flag);
	}
	else
	{
		//Broadcast packet
		NETSIM_ID loop;
		for(loop=0;loop<NETWORK->nDeviceCount;loop++)
		{
			if(loop+1!=packet->nTransmitterId)
			{
				NetSim_PACKET* p = fn_NetSim_Packet_CopyPacket(packet);
				//Add physical in event
				p->nReceiverId=loop+1;
				transmit_packet(p, &flag);
			}
		}
		fn_NetSim_Packet_FreePacket(packet);
	}
	return 0;
}

static void notify_src(NETSIM_ID tx,
					   bool isError)
{
	PALOHA_MAC_VAR mac = ALOHA_MAC(tx, 1);
	double acktime = 0;//mac->slotlength;
	NetSim_EVENTDETAILS pevent;
	memset(&pevent, 0, sizeof pevent);
	pevent.dEventTime = pstruEventDetails->dEventTime + acktime;
	pevent.nDeviceId = tx;
	pevent.nDeviceType = DEVICE_TYPE(tx);
	pevent.nEventType = MAC_OUT_EVENT;
	pevent.nInterfaceId = 1;
	pevent.nProtocolId = MAC_PROTOCOL_ALOHA;
	pevent.nSubEventType = isError ? ALOHA_PACKET_ERROR : ALOHA_PACKET_SUCCESS;
	fnpAddEvent(&pevent);
}

void fn_NetSim_Aloha_PhyIn()
{
	PALOHA_PHY_VAR phy = ALOHA_CURR_PHY;
	double ber;
	NetSim_PACKET* packet = pstruEventDetails->pPacket;
	NETSIM_ID tx = packet->nTransmitterId;
	NETSIM_ID rx = packet->nReceiverId;

	if (isBroadcastPacket(packet))
	{
		if ((tx == 1 && rx == 2) ||
			rx == 1)
			update_interference_power(tx, rx,
									  packet->pstruPhyData->dArrivalTime, false);
	}
	else
	{
		update_interference_power(tx, rx,
								  packet->pstruPhyData->dArrivalTime, false);
	}

	remove_packet_from_txList(packet);

	if(pstruEventDetails->pPacket->nPacketStatus == PacketStatus_Collided)
		goto RET_PHYIN;

	ber = calculate_BER(phy->modulation,
						get_received_power(tx, rx, packet->pstruPhyData->dArrivalTime),
						phy->bandwidth);

	packet->nPacketStatus =	fn_NetSim_Packet_DecideError(ber, packet->pstruPhyData->dPacketSize);

	if(packet->nPacketStatus == PacketStatus_Error)
		goto RET_PHYIN;

	pstruEventDetails->nEventType = MAC_IN_EVENT;
	fnpAddEvent(pstruEventDetails);

RET_PHYIN:
	fn_NetSim_WritePacketTrace(packet);
	fn_NetSim_Metrics_Add(packet);

	if (pstruEventDetails->pPacket->nPacketStatus != PacketStatus_NoError)
	{
		notify_src(tx, true);
		fn_NetSim_Packet_FreePacket(packet);
	}
	else
	{
		notify_src(tx, false);
	}
}
