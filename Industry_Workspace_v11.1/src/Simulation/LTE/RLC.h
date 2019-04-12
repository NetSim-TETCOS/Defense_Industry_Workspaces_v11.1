/************************************************************************************
 * Copyright (C) 2014
 * TETCOS, Bangalore. India															*

 * Tetcos owns the intellectual property rights in the Product and its content.     *
 * The copying, redistribution, reselling or publication of any or all of the       *
 * Product or its content without express prior written consent of Tetcos is        *
 * prohibited. Ownership and / or any other right relating to the software and all  *
 * intellectual property rights therein shall remain at all times with Tetcos.      *
 * Author:	Shashi Kant Suman														*
 * ---------------------------------------------------------------------------------*/
#ifndef _NETSIM_RLC_H_
#define _NETSIM_RLC_H_
#include "List.h"
#ifdef  __cplusplus
extern "C" {
#endif

#define RLC_HEADER_SIZE 5 //Bytes
	typedef struct stru_RLC_Header
	{
		unsigned int DC:1;	//Data/control bit
		unsigned int RF:1;	//re-segmentation flag
		unsigned int P:1;	//polling bit
		unsigned int FI:2;	//Framing information bit
		unsigned int E[3];	//Extension bit
		unsigned int SN:10;	//sequence number
		unsigned int LI1:11;//length indicator field
		unsigned int LI2:11;
		unsigned int LSF:2;	//Last segment flag
	}RLC_HEADER;

	typedef struct stru_RLC_PDU
	{
		RLC_HEADER* header;
		NetSim_PACKET* pdcpPDU[2];
		bool freeflag;//NetSim specific
	}RLC_PDU;
#ifdef  __cplusplus
}
#endif
#endif /* _NETSIM_RLC_H_ */