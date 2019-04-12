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
#include "main.h"
#include "Ethernet.h"
#include "../Firewall/Firewall.h"

static ptrETH_LAN get_vlan(NETSIM_ID d, UINT16 vlanId)
{
	ptrETH_VAR eth = GET_ETH_VAR(d);
	if (!eth->isVLAN)
		return NULL;

	UINT i;
	for (i = 0; i < eth->lanCount; i++)
	{
		ptrETH_LAN lan = eth->lanVar[i];
		if (lan->vlanId == vlanId)
			return lan;
	}
	return NULL;
}

bool isVLANTagPresent(NetSim_PACKET* packet)
{
	ptrIEEE801_1Q_TAG tag = get_eth_hdr_var(packet);
	if (tag)
		return tag->TPId == VLAN_TPID;
	else
		return false;
}

VLANPORT vlan_port_type_from_str(char* val)
{
	if (!_stricmp(val, "ACCESS_PORT"))
		return VLANPORT_ACCESS;
	else if (!_stricmp(val, "TRUNK_PORT"))
		return VLANPORT_TRUNK;
	else
		return VLANPORT_NONE;
}

void vlan_add_trunk_port(ptrETH_VAR eth, NETSIM_ID in)
{
	if (eth->trunkPortCount)
		eth->trunkPortIds = realloc(eth->trunkPortIds,
		(eth->trunkPortCount + 1) * sizeof* eth->trunkPortIds);
	else
		eth->trunkPortIds = calloc(1, sizeof* eth->trunkPortIds);
	eth->trunkPortIds[eth->trunkPortCount] = in;
	eth->trunkPortCount++;
}

static bool is_port_present_in_lan(ptrETH_LAN lan, NETSIM_ID in)
{
	UINT i;
	for (i = 0; i < lan->interfaceCount; i++)
	{
		if (lan->interfaceId[i] == in)
			return true;
	}
	return false;
}

void free_vtag(void* tag)
{
	free((ptrIEEE801_1Q_TAG)tag);
}

void* copy_vtag(void* tag)
{
	ptrIEEE801_1Q_TAG t = calloc(1, sizeof* t);
	memcpy(t, tag, sizeof* t);
	return t;
}

static void add_vlan_tag(NetSim_PACKET* packet, UINT16 vlanId)
{
	ptrIEEE801_1Q_TAG tag = calloc(1, sizeof* tag);
	tag->TPId = VLAN_TPID;
	tag->VID = vlanId;
	set_eth_hdr(packet, ETH_HDR_VID, tag, copy_vtag, free_vtag);
	packet->pstruMacData->dOverhead += IEEE801_1Q_TAG_LEN;
	packet->pstruMacData->dPacketSize = packet->pstruMacData->dOverhead +
		packet->pstruMacData->dPayload;
}

static void remove_vlan_tag(NetSim_PACKET* packet)
{
	free_eth_hdr(packet);
	packet->pstruMacData->dOverhead -= IEEE801_1Q_TAG_LEN;
	packet->pstruMacData->dPacketSize = packet->pstruMacData->dOverhead +
		packet->pstruMacData->dPayload;
}

void vlan_forward_to_all_trunk(NETSIM_ID d,
							   NetSim_PACKET* packet,
							   ptrETH_LAN lan,
							   double time)
{
	ptrETH_VAR eth = GET_ETH_VAR(d);
	if (!eth->isVLAN)
		return; // VLAN is not configured

	NETSIM_ID incoming = eth_get_incoming_port(packet, d);
	UINT16 vlanId = eth_get_incoming_vlanid(packet, d);

	UINT i;
	for (i = 0; i < eth->trunkPortCount; i++)
	{
		NETSIM_ID inter = eth->trunkPortIds[i];

		if (is_port_present_in_lan(lan, inter))
			continue;

		if(inter == incoming)
			continue; //Incoming port

		ptrETH_LAN l = GET_ETH_LAN(d, inter, 0);
		ptrETH_IF iif = GET_ETH_IF(l, inter);

		if (iif->portState == PORTSTATE_BLOCKING)
			continue;

		if (fn_NetSim_MAC_Firewall(d, inter, packet, ACLTYPE_OUTBOUND) == ACLACTION_PERMIT)
		{
			NetSim_PACKET* p = fn_NetSim_Packet_CopyPacket(packet);
			clear_eth_incoming(p);
			add_vlan_tag(p, vlanId);
			send_to_phy(d, inter, p, time);
		}
	}
}

void vlan_forward_to_trunk(NETSIM_ID d,
						   NETSIM_ID in,
						   UINT16 vlanId,
						   NetSim_PACKET* packet,
						   double time)
{
	ptrETH_VAR eth = GET_ETH_VAR(d);
	if (!eth->isVLAN)
		return; // VLAN is not configured

	UINT i;
	for (i = 0; i < eth->trunkPortCount; i++)
	{
		NETSIM_ID inter = eth->trunkPortIds[i];

		if (inter == in)
			continue;

		ptrETH_LAN l = GET_ETH_LAN(d, inter, 0);
		ptrETH_IF iif = GET_ETH_IF(l, inter);

		if (iif->portState == PORTSTATE_BLOCKING)
			continue;

		if (fn_NetSim_MAC_Firewall(d, inter, packet, ACLTYPE_OUTBOUND) == ACLACTION_PERMIT)
		{
			NetSim_PACKET* p = fn_NetSim_Packet_CopyPacket(packet);
			clear_eth_incoming(p);
			add_vlan_tag(p, vlanId);
			send_to_phy(d, inter, p, time);
		}
	}
}

void vlan_macin_forward_packet()
{
	NETSIM_ID d = pstruEventDetails->nDeviceId;
	NETSIM_ID in = pstruEventDetails->nInterfaceId;
	double time = pstruEventDetails->dEventTime;
	NetSim_PACKET* packet = pstruEventDetails->pPacket;
	ptrETH_LAN lan;

	UINT16 vlanId = eth_get_incoming_vlanid(packet, d);
	lan = GET_ETH_LAN(d, in, 0);
	if (lan->lanIP)
	{
		check_move_frame_up(d, in, lan, packet, time);
		return;
	}

	lan = get_vlan(d, vlanId);
	if (!lan)
	{
		// No vlan is found
		vlan_forward_to_trunk(d, in, vlanId, packet, time);
		fn_NetSim_Packet_FreePacket(packet);
		pstruEventDetails->pPacket = NULL;
		return;
	}

	NETSIM_ID f = find_forward_interface(lan, packet);

	if (f)
	{
		//Forward interface found
		if (fn_NetSim_MAC_Firewall(d, f, packet, ACLTYPE_OUTBOUND) == ACLACTION_PERMIT &&
			f != in)
		{
			clear_eth_incoming(packet);
			send_to_phy(d, f, packet, time);
			return;
		}
		else
		{
			fn_NetSim_Packet_FreePacket(packet);
			pstruEventDetails->pPacket = NULL;
			return;
		}
	}
	else
	{
		//Forward to all other trunk
		vlan_forward_to_trunk(d, in, vlanId, packet, time);

		//Forward to all interface
		UINT j;
		for (j = 0; j < lan->interfaceCount; j++)
		{
			if (lan->ethIfVar[j]->interfaceId == in)
				continue;

			if (lan->ethIfVar[j]->portState == PORTSTATE_BLOCKING)
				continue;

			if (fn_NetSim_MAC_Firewall(d, lan->ethIfVar[j]->interfaceId, packet, ACLTYPE_OUTBOUND) == ACLACTION_PERMIT)
			{
				NetSim_PACKET* p = fn_NetSim_Packet_CopyPacket(packet);
				clear_eth_incoming(p);
				send_to_phy(d, lan->ethIfVar[j]->interfaceId, p, time);
			}
		}

		fn_NetSim_Packet_FreePacket(packet);
		pstruEventDetails->pPacket = NULL;
	}
}

void vlan_macout_forward_packet(NETSIM_ID d,
								NETSIM_ID in,
								UINT16 vlanId,
								NETSIM_ID incoming,
								NetSim_PACKET* packet,
								double time)
{
	NETSIM_ID out = 0;
	ptrETH_LAN lan = GET_ETH_LAN(d, in, 0);
	
	out = find_forward_interface(lan, packet);

	if (out)
	{
		if (fn_NetSim_MAC_Firewall(d, out, packet, ACLTYPE_OUTBOUND) == ACLACTION_PERMIT &&
			out != incoming)
		{
			clear_eth_incoming(packet);
			send_to_phy(d, out, packet, time);
		}
		else
			fn_NetSim_Packet_FreePacket(packet);
	}
	else
	{
		vlan_forward_to_trunk(d, incoming, vlanId, packet, time);

		UINT j;
		for (j = 0; j < lan->interfaceCount; j++)
		{
			if (lan->ethIfVar[j]->portState == PORTSTATE_BLOCKING)
				continue;

			if (fn_NetSim_MAC_Firewall(d, lan->ethIfVar[j]->interfaceId, packet, ACLTYPE_OUTBOUND) == ACLACTION_PERMIT &&
				lan->ethIfVar[j]->interfaceId != incoming)
			{
				NetSim_PACKET* p = fn_NetSim_Packet_CopyPacket(packet);
				clear_eth_incoming(p);
				send_to_phy(d, lan->ethIfVar[j]->interfaceId, p, time);
			}
		}
		fn_NetSim_Packet_FreePacket(packet);
	}
}
