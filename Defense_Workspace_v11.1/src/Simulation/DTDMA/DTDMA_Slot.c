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
#include "NetSim_utility.h"
#include "DTDMA_enum.h"
#include "DTDMA.h"

static double form_slot_roundrobin();
static double form_slot_demandbased();
static double form_slot_file();
static void get_slot_required(unsigned int* slot_required, unsigned int s, ptrDTDMA_NET_INFO net);
static void schedule_transmission();

void init_slot_formation()
{
	ptrDTDMA_SESSION session = dtdmaSession;

	while (session)
	{
		ptrDTDMA_ALLOCATION_INFO allocation_info = (ptrDTDMA_ALLOCATION_INFO)calloc(1, sizeof* allocation_info);
		session->allocation_info = allocation_info;

		memset(pstruEventDetails, 0, sizeof* pstruEventDetails);
		pstruEventDetails->nDeviceId = session->netInfo[0]->nodeInfo[0]->nodeId;
		pstruEventDetails->nDeviceType = DEVICE_TYPE(session->netInfo[0]->nodeInfo[0]->nodeId);
		pstruEventDetails->nEventType = TIMER_EVENT;
		pstruEventDetails->nInterfaceId = session->netInfo[0]->nodeInfo[0]->interfaceId;
		pstruEventDetails->nProtocolId = MAC_PROTOCOL_DTDMA;
		pstruEventDetails->nSubEventType = DTDMA_FORM_SLOT;
		fnpAddEvent(pstruEventDetails);

		session = LIST_NEXT(session);
	}
}

void fn_NetSim_DTDMA_FormSlot()
{
	PDTDMA_NODE_MAC mac = DTDMA_MAC(pstruEventDetails->nDeviceId, pstruEventDetails->nInterfaceId);
	double t;
	if (mac->slot_allocation_technique == TECHNIQUE_ROUNDROBIN)
		t = form_slot_roundrobin();
	else if (mac->slot_allocation_technique == TECHNIQUE_DEMANDBASED)
		t = form_slot_demandbased();
	else if(mac->slot_allocation_technique == TECHNIQUE_FILE)
		t = form_slot_file();
	else
	{
		fnNetSimError("Unknown slot allocation technique\n");
		return;
	}
	//Add next event
	pstruEventDetails->dEventTime=t;
	fnpAddEvent(pstruEventDetails);
	//Schedule the transmission
	schedule_transmission();
}

static bool isAnyDeviceConnected(PDTDMA_NODE_MAC mac)
{
	NETSIM_ID i;
	for (i = 0; i < mac->netInfo->nDeviceCount; i++)
	{
		ptrDTDMA_NODE_INFO info = mac->netInfo->nodeInfo[i];
		NETSIM_ID c = info->nodeId;
		if (DEVICE(c)->node_status == CONNECTED)
			return true;
	}
	return false;
}

static double form_slot_roundrobin()
{
	NETSIM_ID d = pstruEventDetails->nDeviceId;
	NETSIM_ID in = pstruEventDetails->nInterfaceId;

	ptrDTDMA_ALLOCATION_INFO allocation_info = get_allocation_info(d, in);

	PDTDMA_NODE_MAC mac = DTDMA_MAC(d, in);
	NETSIM_ID i,slotid=0;
	unsigned int frame_id = allocation_info->nFrameId;
	double time;

	free(allocation_info->slot_info);
	memset(allocation_info,0,sizeof* allocation_info);

	allocation_info->nFrameId = ++frame_id;
	allocation_info->nSlotCount = mac->nSlotCountInFrame;
	allocation_info->slot_info = (PSLOT_INFO*)calloc(allocation_info->nSlotCount,sizeof* allocation_info->slot_info);
	allocation_info->dFrameStartTime=pstruEventDetails->dEventTime;
	allocation_info->dSlotStartTime=pstruEventDetails->dEventTime;
	allocation_info->dNextSlotStartTime=pstruEventDetails->dEventTime+
		mac->dSlotDuration+mac->dGuardInterval;
	allocation_info->nCurrentSlot=1;
	allocation_info->nNextSlot=2;

	time = pstruEventDetails->dEventTime;
	
	if (isAnyDeviceConnected(mac))
	{
		while (slotid < mac->nSlotCountInFrame)
		{
			for (i = 0; i < mac->netInfo->nDeviceCount && slotid < mac->nSlotCountInFrame; i++, slotid++)
			{
				ptrDTDMA_NODE_INFO info = mac->netInfo->nodeInfo[i];
				NETSIM_ID c = info->nodeId;
				NETSIM_ID ci = info->interfaceId;
				if (DEVICE(c)->node_status == CONNECTED)
				{
					allocation_info->slot_info[slotid] = calloc(1, sizeof* allocation_info->slot_info[slotid]);
					allocation_info->slot_info[slotid]->device_id = c;
					allocation_info->slot_info[slotid]->interfaceId = ci;
					allocation_info->slot_info[slotid]->slot_id = slotid + 1;
					allocation_info->slot_info[slotid]->dSlotStartTime = time;
					time += mac->dSlotDuration + mac->dGuardInterval;
				}
				else
				{
					slotid--;
				}
			}
		}
	}
	else
	{
		time = mac->nSlotCountInFrame*(mac->dSlotDuration + mac->dGuardInterval);
	}
	return max(time,pstruEventDetails->dEventTime+mac->dSlotDuration+mac->dGuardInterval);
}

static double form_slot_demandbased()
{
	NETSIM_ID d = pstruEventDetails->nDeviceId;
	NETSIM_ID in = pstruEventDetails->nInterfaceId;
	ptrDTDMA_ALLOCATION_INFO allocation_info = get_allocation_info(d, in);
	PDTDMA_NODE_MAC mac = DTDMA_MAC(d, in);
	ptrDTDMA_NET_INFO net = mac->netInfo;
	NETSIM_ID i, slotid = 0, j;
	unsigned int frame_id = allocation_info->nFrameId;
	double time;
	static unsigned int* slot_required;

	free(allocation_info->slot_info);
	memset(allocation_info, 0, sizeof* allocation_info);

	if (!slot_required)
		slot_required = (unsigned int*)calloc(net->nDeviceCount, sizeof* slot_required);

	get_slot_required(slot_required, (mac->bitsPerSlot - mac->overheadPerSlot) / 8, net);

	mac->nSlotCountInFrame = 0;
	printf("Slot required---\n");
	fprintf(stdout, "DeviceId\tSlot Required\n");
	for (i = 0; i < NETWORK->nDeviceCount; i++)
	{
		fprintf(stdout, "%d\t%d\n", i + 1, slot_required[i]);
		mac->nSlotCountInFrame += slot_required[i];
	}

	allocation_info->nFrameId = ++frame_id;
	allocation_info->nSlotCount = mac->nSlotCountInFrame;
	allocation_info->slot_info = (PSLOT_INFO*)calloc(allocation_info->nSlotCount, sizeof* allocation_info->slot_info);
	allocation_info->dFrameStartTime = pstruEventDetails->dEventTime;
	allocation_info->dSlotStartTime = pstruEventDetails->dEventTime;
	allocation_info->dNextSlotStartTime = pstruEventDetails->dEventTime +
		mac->dSlotDuration + mac->dGuardInterval;
	allocation_info->nCurrentSlot = 1;
	allocation_info->nNextSlot = 2;

	time = pstruEventDetails->dEventTime;

	if (isAnyDeviceConnected(mac))
	{
		for (i = 0; i < net->nDeviceCount; i++)
		{
			ptrDTDMA_NODE_INFO info = net->nodeInfo[i];
			NETSIM_ID c = info->nodeId;
			NETSIM_ID ci = info->interfaceId;

			for (j = 0; j < slot_required[i]; j++)
			{
				allocation_info->slot_info[slotid] = calloc(1, sizeof* allocation_info->slot_info[slotid]);
				allocation_info->slot_info[slotid]->device_id = c;
				allocation_info->slot_info[slotid]->interfaceId = ci;
				allocation_info->slot_info[slotid]->slot_id = slotid + 1;
				allocation_info->slot_info[slotid]->dSlotStartTime = time;
				time += mac->dSlotDuration + mac->dGuardInterval;
				slotid++;
			}
		}
	}
	else
	{
		time = mac->nSlotCountInFrame*(mac->dSlotDuration + mac->dGuardInterval);
	}
	return max(time, pstruEventDetails->dEventTime + mac->dSlotDuration + mac->dGuardInterval);
}

static void get_slot_required(unsigned int* slot_required,
							  unsigned int s,
							  ptrDTDMA_NET_INFO net)
{
	UINT i;
	for (i = 0; i < net->nDeviceCount; i++)
	{
		ptrDTDMA_NODE_INFO info = net->nodeInfo[i];
		NETSIM_ID d = info->nodeId;
		NETSIM_ID in = info->interfaceId;

		PDTDMA_NODE_MAC mac = DTDMA_MAC(d, in);
		NetSim_BUFFER* b = DEVICE_ACCESSBUFFER(d, in);
		unsigned int siz = (unsigned int)b->dCurrentBufferSize;
		if (NETWORK->ppstruDeviceList[d]->node_status == CONNECTED)
			slot_required[i] = max(mac->min_slot, min(mac->max_slot, siz / s));
		else
			slot_required[i] = 0;
	}
}

typedef struct stru_file_slot
{
	ptrDTDMA_SESSION session;
	NETSIM_ID netId;
	UINT count;
	NETSIM_ID* array;
	struct stru_file_slot* next;
}FILE_SLOT,*ptrFILE_SLOT;
ptrFILE_SLOT fileSlot = NULL;

static ptrFILE_SLOT find_slot(ptrDTDMA_SESSION session, NETSIM_ID netId)
{
	ptrFILE_SLOT f = fileSlot;
	while (f)
	{
		if (f->session == session && f->netId == netId)
			return f;
		f = f->next;
	}

	ptrFILE_SLOT n = calloc(1, sizeof* n);
	f = fileSlot;
	if (f)
	{
		while (f->next)
			f = f->next;
		f->next = n;
	}
	else
		fileSlot = n;

	n->session = session;
	n->netId = netId;

	return n;
}

NETSIM_ID* alloc_slot_array(NETSIM_ID* oldArray, UINT oldCount, UINT newCount)
{
	NETSIM_ID* ret = calloc(newCount, sizeof* ret);
	if (oldCount)
	{
		memcpy(ret, oldArray, oldCount * sizeof*oldArray);
		free(oldArray);
	}
	return ret;
}

static void init_slot_array()
{
	NETSIM_ID link = 0;
	NETSIM_ID netId = 0;
	ptrDTDMA_SESSION session;
	ptrFILE_SLOT slot = NULL;
	char path[BUFSIZ];
	sprintf(path, "%s%s%s",
			pszIOPath,
			pathSeperator,
			"SlotAllocation.txt");
	FILE* fp = fopen(path, "r");
	if (!fp)
	{
		fnSystemError("Unable to open SlotAllocation.txt");
		perror(path);
		return;
	}

	while (fgets(path, BUFSIZ, fp))
	{
		lskip(path);
		if (*path == '#')
			continue; //Comment
		if (*path == 0)
			continue; // empty line

		if (*path == 'l' || *path == 'L')
		{
			//new link
			strtok(path, "=");
			link = atoi(strtok(NULL, ","));
			link = fn_NetSim_Stack_GetLinkIdByConfigId(link);
			session = find_session(link);
			continue;
		}

		if (*path == 'N' || *path == 'n')
		{
			//new Net
			strtok(path, "=");
			netId = atoi(strtok(NULL, ","));
			slot = find_slot(session, netId);
			continue;
		}

		strtok(path, ",");
		char* s = strtok(NULL, ",");
		UINT slotid = atoi(s);
		UINT m = max(slotid, slot->count + 1);
		slot->array = alloc_slot_array(slot->array, slot->count, m);

		NETSIM_ID d = fn_NetSim_Stack_GetDeviceId_asName(path);
		slot->array[slotid-1] = d;
		slot->count++;
	}
	fclose(fp);
}

static void free_slot_info(PSLOT_INFO* info, UINT count)
{
	UINT i;
	for (i = 0; i < count; i++)
	{
		PSLOT_INFO s = info[i];
		free(s);
	}
	free(info);
}

static double form_slot_file()
{
	if (!fileSlot)
		init_slot_array();

	NETSIM_ID d = pstruEventDetails->nDeviceId;
	NETSIM_ID in = pstruEventDetails->nInterfaceId;
	PDTDMA_NODE_MAC mac = DTDMA_MAC(d, in);
	ptrFILE_SLOT slot = find_slot(mac->session, mac->nNetId);
	if (!slot->count)
	{
		fnNetSimError("File based slot allocation is not configured for Device %d, Net Id %d.\n",
					  DEVICE_CONFIGID(d),
					  mac->nNetId);
		assert(false);
	}
	ptrDTDMA_ALLOCATION_INFO allocation_info = get_allocation_info(d, in);

	NETSIM_ID slotid = 0;
	UINT i;
	unsigned int frame_id = allocation_info->nFrameId;
	double time;

	free_slot_info(allocation_info->slot_info, allocation_info->nSlotCount);
	memset(allocation_info, 0, sizeof* allocation_info);

	allocation_info->nFrameId = ++frame_id;
	allocation_info->nSlotCount = mac->nSlotCountInFrame;
	allocation_info->slot_info = (PSLOT_INFO*)calloc(allocation_info->nSlotCount+1, sizeof* allocation_info->slot_info);
	allocation_info->dFrameStartTime = pstruEventDetails->dEventTime;
	allocation_info->dSlotStartTime = pstruEventDetails->dEventTime;
	allocation_info->dNextSlotStartTime = pstruEventDetails->dEventTime +
		mac->dSlotDuration + mac->dGuardInterval;
	allocation_info->nCurrentSlot = 1;
	allocation_info->nNextSlot = 2;

	time = pstruEventDetails->dEventTime;
	if (isAnyDeviceConnected(mac))
	{
		while (slotid < mac->nSlotCountInFrame)
		{
			for (i = 0; i < slot->count; i++)
			{
				if (slotid == mac->nSlotCountInFrame)
					break;
				NETSIM_ID c = slot->array[i];
				if (!c)
				{
					//Empty slot.
					allocation_info->slot_info[slotid] = calloc(1, sizeof* allocation_info->slot_info[slotid]);
					allocation_info->slot_info[slotid]->device_id = 0;
					allocation_info->slot_info[slotid]->interfaceId = 0;
					allocation_info->slot_info[slotid]->slot_id = slotid + 1;
					allocation_info->slot_info[slotid]->dSlotStartTime = time;
					time += mac->dSlotDuration + mac->dGuardInterval;
					slotid++;
				}
				else
				{
					NETSIM_ID ci = fn_NetSim_Stack_GetConnectedInterface(d, in, c);
					
					if (DEVICE(c)->node_status == CONNECTED)
					{
						allocation_info->slot_info[slotid] = calloc(1, sizeof* allocation_info->slot_info[slotid]);
						allocation_info->slot_info[slotid]->device_id = c;
						allocation_info->slot_info[slotid]->interfaceId = ci;
						allocation_info->slot_info[slotid]->slot_id = slotid + 1;
						allocation_info->slot_info[slotid]->dSlotStartTime = time;
						time += mac->dSlotDuration + mac->dGuardInterval;
						slotid++;
					}
				}
			}
		}
	}
	else
	{
		time = mac->nSlotCountInFrame*(mac->dSlotDuration + mac->dGuardInterval);
	}
	return max(time, pstruEventDetails->dEventTime + mac->dSlotDuration + mac->dGuardInterval);
}

static void schedule_transmission()
{
	NETSIM_ID d = pstruEventDetails->nDeviceId;
	NETSIM_ID in = pstruEventDetails->nInterfaceId;
	ptrDTDMA_ALLOCATION_INFO allocation_info = get_allocation_info(d, in);
	unsigned int i;
	NetSim_EVENTDETAILS pevent;
	memset(&pevent,0,sizeof pevent);
	pevent.nEventType = TIMER_EVENT;
	pevent.nProtocolId = MAC_PROTOCOL_DTDMA;
	pevent.nSubEventType = DTDMA_SCHEDULE_Transmission;

	for(i=0;i<allocation_info->nSlotCount;i++)
	{
		if (allocation_info->slot_info[i] &&
			allocation_info->slot_info[i]->device_id)
		{
			pevent.dEventTime = allocation_info->slot_info[i]->dSlotStartTime;
			pevent.nDeviceId = allocation_info->slot_info[i]->device_id;
			pevent.nInterfaceId = allocation_info->slot_info[i]->interfaceId;
			pevent.nDeviceType = DEVICE_TYPE(pevent.nDeviceId);
			fnpAddEvent(&pevent);
		}
	}
}
