#pragma once
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
#ifndef _NETSIM_WSMP_H_
#define _NETSIM_WSMP_H_
#ifdef  __cplusplus
extern "C" {
#endif

#define WSMP_WSM_MAX_LENGTH_DEFAULT	1400 //Bytes Range 1-2302

#define WSMP_HDR_LEN	11 //Bytes


	//As per linux implementation
	typedef struct wsmp_header
	{
		UINT8	wsm_version;
		UINT8	psid;
		UINT8	ch_no[3];
		UINT8	data_rate[3];
		UINT8	tx_power_level[3];
		UINT16	ele_id;
		UINT8	wsm_len;
		UINT8 *data;
	}WSMP_HDR,*PWSMP_HDR;
	static __inline PWSMP_HDR GET_WSMP_HDR(NetSim_PACKET* packet)
	{
		if (packet->pstruTransportData &&
			packet->pstruTransportData->nTransportProtocol == TX_PROTOCOL_WSMP)
			return packet->pstruTransportData->Packet_TransportProtocol;
		else
			return NULL;
	}
	static __inline void SET_WSMP_HDR(NetSim_PACKET* packet, PWSMP_HDR hdr)
	{
		if (packet->pstruTransportData)
		{
			packet->pstruTransportData->nTransportProtocol = TX_PROTOCOL_WSMP;
			packet->pstruTransportData->Packet_TransportProtocol = hdr;
		}
	}

	typedef struct wsmp_data
	{
		UINT wsm_max_length;
	}WSMP_DATA, *PWSMP_DATA;
#define GET_WSMP_DATA(devId) ((PWSMP_DATA)DEVICE_TRXLayer(devId)->TransportProtocolVar)
#define GET_CURR_WSMP_DATA GET_WSMP_DATA(pstruEventDetails->nDeviceId)
#define SET_WSMP_DATA(devId,data) (DEVICE_TRXLayer(devId)->TransportProtocolVar=(void*)data)

#ifdef  __cplusplus
}
#endif
#endif //_NETSIM_WSMP_H_