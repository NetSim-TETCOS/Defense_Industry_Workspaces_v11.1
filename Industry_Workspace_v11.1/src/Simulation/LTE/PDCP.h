/************************************************************************************
 * Copyright (C) 2014																*
 * TETCOS, Bangalore. India															*
 * Tetcos owns the intellectual property rights in the Product and its content.     *
 * The copying, redistribution, reselling or publication of any or all of the       *
 * Product or its content without express prior written consent of Tetcos is        *
 * prohibited. Ownership and / or any other right relating to the software and all  *
 * intellectual property rights therein shall remain at all times with Tetcos.      *
 * Author:	Shashi Kant Suman														*
 * ---------------------------------------------------------------------------------*/
#ifndef _NETSIM_PDCP_H_
#define _NETSIM_PDCP_H_
#include "List.h"
#ifdef  __cplusplus
extern "C" {
#endif

#define PDCP_SN_SIZE 7

#if PDCP_SN_SIZE == 7
#define PDCP_HEADER_SIZE 1	//Bytes
#else
#define PDCP_HEADER_SIZE 2 //Bytes
#endif

#define PDCP_TOKEN_SIZE 4 //Bytes

	struct stru_LTE_PDCP_Header
	{
#if PDCP_SN_SIZE == 12
		unsigned int R[3];//3 bits
#endif
		unsigned int D_C;
		unsigned int PDCPSN;
	};
#ifdef  __cplusplus
}
#endif
#endif /* _NETSIM_PDCP_H_ */