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
#ifndef _NETSIM_IEEE802_11E_H_
#define _NETSIM_IEEE802_11E_H_
#ifdef  __cplusplus
extern "C" {
#endif

	// Data structure for IEEE 802.11e packet lists.
	typedef struct stru_802_11e_Packetlist
	{
		NetSim_PACKET* AC_VO;
		NetSim_PACKET* AC_VI;
		NetSim_PACKET* AC_BE;
		NetSim_PACKET* AC_BK;

		double dMaximumSize;
		double dCurrentSize;
	}QOS_LIST,*PQOS_LIST;


	//Function Prototype
	NetSim_PACKET* fn_NetSim_IEEE802_11e_GetPacketFromQueue(PQOS_LIST pstruQosList,int nPacketRequire,int* nPacketCount);
	void fn_NetSim_IEEE802_11e_AddtoQueue(void* mac,NetSim_PACKET* pstruPacket);
	void fn_NetSim_IEEE802_11e_Updatecw(NetSim_PACKET* packet);

#ifdef  __cplusplus
}
#endif
#endif //_NETSIM_IEEE802_11E_H_