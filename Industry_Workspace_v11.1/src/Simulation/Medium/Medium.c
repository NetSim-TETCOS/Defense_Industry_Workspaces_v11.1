/************************************************************************************
* Copyright (C) 2018                                                               *
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
#define _NETSIM_MDEIUM_CODE_
#include "main.h"
#include "Medium.h"
#pragma comment(lib,"NetworkStack.lib")
#pragma comment(lib,"Metrics.lib")

typedef struct stru_Packet_info
{
	NetSim_PACKET* packet;
	NETSIM_ID txId;
	NETSIM_ID txIf;
	NETSIM_ID rxId;
	NETSIM_ID rxIf;
	double startTime;
	double endTime;
	struct stru_Packet_info* next;
}PACKETINFO, *ptrPACKETINFO;

typedef struct stru_Device
{
	NETSIM_ID deviceId;
	NETSIM_ID interfaceId;
	double frequency;
	double bandwidth;
	double dRxSensitivity;
	double dEDThreshold;

	double dCummulativeReceivedPower_mw;
	double dCummulativeReceivedPower_dbm;

	void(*medium_change_callback)(NETSIM_ID, NETSIM_ID, bool);
	bool(*isRadioIdle)(NETSIM_ID, NETSIM_ID);
	double(*getRxPower_mw)(NETSIM_ID, NETSIM_ID, NETSIM_ID, NETSIM_ID, double);
	bool(*isTranmitterBusy)(NETSIM_ID, NETSIM_ID);
}DEVICE_IN_MEDIUM, *ptrDEVICE_IN_MEDIUM;


typedef struct stru_medium
{
	ptrDEVICE_IN_MEDIUM** devices;
	ptrPACKETINFO transmissionPacket;
}MEDIUM,*ptrMEDIUM;

static MEDIUM medium;

static ptrDEVICE_IN_MEDIUM medium_find_device(NETSIM_ID devId,
											  NETSIM_ID devIf)
{
	if (!devId || !devIf)
		fnNetSimError("Device id = %d, device interface = %d is not a valid input for %s,",
					  devId,
					  devIf,
					  __FUNCTION__);
	if (medium.devices &&
		medium.devices[devId - 1])
		return medium.devices[devId - 1][devIf - 1];
	return NULL;
}

_declspec(dllexport) void medium_add_device(NETSIM_ID d,
											NETSIM_ID ifid,
											double dFrequency,
											double dBandwidth,
											double dRxSensitivity,
											double dEdThreshold,
											void(*medium_change_callback)(NETSIM_ID, NETSIM_ID, bool),
											bool(*isRadioIdle)(NETSIM_ID, NETSIM_ID),
											bool(*isTransmitterBusy)(NETSIM_ID,NETSIM_ID),
											double(*getRXPower_mw)(NETSIM_ID, NETSIM_ID, NETSIM_ID, NETSIM_ID, double))
{
	ptrDEVICE_IN_MEDIUM dm = calloc(1, sizeof* dm);
	dm->deviceId = d;
	dm->interfaceId = ifid;
	dm->dEDThreshold = dEdThreshold;
	dm->dRxSensitivity = dRxSensitivity;
	dm->frequency = dFrequency;
	dm->bandwidth = dBandwidth;
	dm->dCummulativeReceivedPower_dbm = NEGATIVE_INFINITY;
	dm->medium_change_callback = medium_change_callback;
	dm->isRadioIdle = isRadioIdle;
	dm->isTranmitterBusy = isTransmitterBusy;
	dm->getRxPower_mw = getRXPower_mw;

	if (!medium.devices)
		medium.devices = calloc(NETWORK->nDeviceCount, sizeof* medium.devices);
	if (!medium.devices[d - 1])
		medium.devices[d - 1] = calloc(DEVICE(d)->nNumOfInterface, sizeof* medium.devices[d - 1]);
	medium.devices[d - 1][ifid - 1] = dm;
}

_declspec(dllexport) void medium_update_device(NETSIM_ID d,
											   NETSIM_ID ifid,
											   double dFrequency,
											   double dBandwidth,
											   double dRxSensitivity,
											   double dEdThreshold)
{
	ptrDEVICE_IN_MEDIUM dm = medium_find_device(d, ifid);
	if (!dm)
		fnNetSimError("%s is called without adding device to medium. use medium_add_device to add the device.\n",
					  __FUNCTION__);
	dm->bandwidth = dBandwidth;
	dm->frequency = dFrequency;
	dm->dRxSensitivity = dRxSensitivity;
	dm->dEDThreshold = dEdThreshold;
}

static void medium_remove_device(NETSIM_ID d,
								 NETSIM_ID ifId)
{
	ptrDEVICE_IN_MEDIUM dm = medium_find_device(d, ifId);
	if (dm)
	{
		free(dm);
		medium.devices[d - 1][ifId - 1] = NULL;
	}
}


static void medium_call_callback(ptrDEVICE_IN_MEDIUM d, bool isMediumBusy)
{
	if(d->medium_change_callback)
		d->medium_change_callback(d->deviceId, d->interfaceId, isMediumBusy);
}

static ptrPACKETINFO packetInfo_add(NetSim_PACKET* packet,
						   NETSIM_ID txId,
						   NETSIM_ID txIf,
						   NETSIM_ID rxId,
						   NETSIM_ID rxIf)
{
	ptrPACKETINFO p = calloc(1, sizeof* p);
	p->packet = packet;
	p->rxId = rxId;
	p->rxIf = rxIf;
	p->startTime = packet->pstruPhyData->dArrivalTime;
	p->endTime = packet->pstruPhyData->dEndTime;
	p->txId = txId;
	p->txIf = txIf;

	if (medium.transmissionPacket)
	{
		ptrPACKETINFO t = medium.transmissionPacket;
		while (t->next)
			t = t->next;
		t->next = p;
	}
	else
	{
		medium.transmissionPacket = p;
	}
	return p;
}

static ptrPACKETINFO packetInfo_remove(NetSim_PACKET* packet)
{
	ptrPACKETINFO p = medium.transmissionPacket;
	ptrPACKETINFO pr = NULL;
	while (p)
	{
		if (p->packet == packet)
		{
			if (pr)
			{
				pr->next = p->next;
				break;
			}
			else
			{
				medium.transmissionPacket = p->next;
				break;
			}
		}
		pr = p;
		p = p->next;
	}
	p->next = NULL;
	return p;
}

static void packetInfo_free(ptrPACKETINFO info)
{
	free(info);
}

static void medium_mark_collision(ptrPACKETINFO info)
{
	ptrPACKETINFO i = medium.transmissionPacket;
	while (i)
	{
		if (i->txId == info->txId &&
			i->txIf == info->txIf &&
			i != info)
		{
			i = i->next;
			continue;
		}
		ptrDEVICE_IN_MEDIUM dm = medium_find_device(i->rxId, i->rxIf);
		
		double p = dm->dCummulativeReceivedPower_mw - dm->getRxPower_mw(i->txId, i->txIf,
																		i->rxId, i->rxIf,
																		i->startTime);
		double pdbm = MW_TO_DBM(p);
		if (pdbm > dm->dRxSensitivity)
			i->packet->nPacketStatus = PacketStatus_Collided;

		if(dm->isTranmitterBusy(dm->deviceId,dm->interfaceId))
			i->packet->nPacketStatus = PacketStatus_Collided;

		i = i->next;
	}
}

static bool isAnypacketIsThereForThisTransmitter(ptrPACKETINFO info)
{
	ptrPACKETINFO i = medium.transmissionPacket;
	while (i)
	{
		if (i != info)
		{
			if (i->txId == info->txId &&
				i->txIf == info->txIf)
				return true;
		}
		i = i->next;
	}
	return false;
}

static bool CheckFrequencyInterfrence(double dFrequency1, double dFrequency2, double bandwidth)
{
	if (dFrequency1 > dFrequency2)
	{
		if ((dFrequency1 - dFrequency2) >= bandwidth)
			return false; // no interference
		else
			return true; // interference
	}
	else
	{
		if ((dFrequency2 - dFrequency1) >= bandwidth)
			return false; // no interference
		else
			return true; // interference
	}
}

static void update_power_due_to_transmission(ptrPACKETINFO info)
{	
	ptrDEVICE_IN_MEDIUM txdm = medium_find_device(info->txId, info->txIf);
	if (!txdm)
	{
		fnNetSimError("Device %d, interface %d is not added to medium before transmission.",
					  info->txId, info->txIf);
		return;
	}

	NETSIM_ID nLoop, nLoopInterface;
	for (nLoop = 1; nLoop <= NETWORK->nDeviceCount; nLoop++)
	{
		for (nLoopInterface = 1; nLoopInterface <= DEVICE(nLoop)->nNumOfInterface; nLoopInterface++)
		{
			if (nLoop == info->txId &&
				nLoopInterface == info->txIf)
				continue;

			ptrDEVICE_IN_MEDIUM dm = medium_find_device(nLoop, nLoopInterface);
			if (!dm)
				continue;

			if (!CheckFrequencyInterfrence(txdm->frequency, dm->frequency, txdm->bandwidth))
				continue; //Different band


			dm->dCummulativeReceivedPower_mw += dm->getRxPower_mw(txdm->deviceId, txdm->interfaceId,
																  dm->deviceId, dm->interfaceId,
																  info->startTime);

			dm->dCummulativeReceivedPower_dbm = MW_TO_DBM(dm->dCummulativeReceivedPower_mw);

			if (info->rxId != dm->deviceId)
			{
				if (dm->dCummulativeReceivedPower_dbm > dm->dEDThreshold &&
					dm->isRadioIdle(dm->deviceId, dm->interfaceId) &&
					dm->medium_change_callback)
					dm->medium_change_callback(dm->deviceId, dm->interfaceId, true);
			}
			else if (info->rxIf != dm->interfaceId)
			{
				if (dm->dCummulativeReceivedPower_dbm > dm->dEDThreshold &&
					dm->isRadioIdle(dm->deviceId, dm->interfaceId) &&
					dm->medium_change_callback)
					dm->medium_change_callback(dm->deviceId, dm->interfaceId, true);
			}
		}
	}
}

static void update_power_due_to_transmission_stop(ptrPACKETINFO info)
{
	ptrDEVICE_IN_MEDIUM txdm = medium_find_device(info->txId, info->txIf);
	if (!txdm)
	{
		fnNetSimError("Device %d, interface %d is not added to medium before transmission.",
					  info->txId, info->txIf);
		return;
	}

	NETSIM_ID nLoop, nLoopInterface;
	for (nLoop = 1; nLoop <= NETWORK->nDeviceCount; nLoop++)
	{
		for (nLoopInterface = 1; nLoopInterface <= DEVICE(nLoop)->nNumOfInterface; nLoopInterface++)
		{
			if (nLoop == info->txId &&
				nLoopInterface == info->txIf)
			{
				if(txdm->isRadioIdle(txdm->deviceId, txdm->interfaceId) &&
				   txdm->medium_change_callback)
					txdm->medium_change_callback(txdm->deviceId, txdm->interfaceId, false);
				continue;
			}

			ptrDEVICE_IN_MEDIUM dm = medium_find_device(nLoop, nLoopInterface);
			if (!dm)
				continue;

			if (!CheckFrequencyInterfrence(txdm->frequency, dm->frequency, txdm->bandwidth))
				continue; //Different band


			dm->dCummulativeReceivedPower_mw -= dm->getRxPower_mw(txdm->deviceId, txdm->interfaceId,
																  dm->deviceId, dm->interfaceId,
																  info->startTime);

			dm->dCummulativeReceivedPower_dbm = MW_TO_DBM(dm->dCummulativeReceivedPower_mw);

			if (info->rxId != dm->deviceId)
			{
				if (dm->dCummulativeReceivedPower_dbm < dm->dEDThreshold &&
					dm->isRadioIdle(dm->deviceId, dm->interfaceId) &&
					dm->medium_change_callback)
					dm->medium_change_callback(dm->deviceId, dm->interfaceId, false);
			}
			else if (info->rxIf != dm->interfaceId)
			{
				if (dm->dCummulativeReceivedPower_dbm < dm->dEDThreshold &&
					dm->isRadioIdle(dm->deviceId, dm->interfaceId) &&
					dm->medium_change_callback)
					dm->medium_change_callback(dm->deviceId, dm->interfaceId, false);
			}
		}
	}
}

_declspec(dllexport) void medium_notify_packet_send(NetSim_PACKET* packet,
													NETSIM_ID txId,
													NETSIM_ID txIf,
													NETSIM_ID rxId,
													NETSIM_ID rxIf)
{
	ptrPACKETINFO info = packetInfo_add(packet, txId, txIf, rxId, rxIf);
	if (isAnypacketIsThereForThisTransmitter(info))
		return;
	update_power_due_to_transmission(info);
	medium_mark_collision(info);
}

_declspec(dllexport) void medium_notify_packet_received(NetSim_PACKET* packet)
{
	ptrPACKETINFO info = packetInfo_remove(packet);
	if (!isAnypacketIsThereForThisTransmitter(info))
		update_power_due_to_transmission_stop(info);
	packetInfo_free(info);
}

_declspec(dllexport) bool medium_isIdle(NETSIM_ID d,
										NETSIM_ID in)
{
	ptrDEVICE_IN_MEDIUM dm = medium_find_device(d, in);
	return (dm->dCummulativeReceivedPower_dbm < dm->dEDThreshold);
}