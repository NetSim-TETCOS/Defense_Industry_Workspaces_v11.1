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
#include "TCP.h"
#include "TCP_Header.h"

/*
* This file is used for testing the TCP working in NetSim.
* Work based on drop probability and burst drop.
*/

bool pass_test()
{
#ifdef _TEST_TCP_
	double d = NETSIM_RAND_01();
	static int c = 0;
	PTCP_SEGMENT_HDR hdr = TCP_GET_SEGMENT_HDR(pstruEventDetails->pPacket);
	if (!isAckDrop && hdr->Ack == 1)
		return true;

	if (burstDrop)
	{
		if (c && c < burstDropSize)
		{
			c++;
			return false;
		}
		else
		{
			c = 0;
		}
	}
	if (d < dropProbability)
	{
		if (burstDrop)
			c = 1;
		return false;
	}
	else
		return true;
#endif
	return true;
}