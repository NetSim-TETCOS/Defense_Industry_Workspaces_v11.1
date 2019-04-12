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
#ifndef _NETSIM_TOKEN_H_
#define _NETSIM_TOKEN_H_
#ifdef  __cplusplus
extern "C" {
#endif

	///For MSVC compiler. For GCC link via Linker command 
#pragma comment(lib,"Tokenlib.lib")
#pragma comment(lib,"Metrics.lib")
#pragma comment(lib,"NetworkStack.lib")

#include "Token_enum.h"

#define Token_MAC_OVERHEAD 21 //Bytes
#define Token_PHY_OVERHEAD 0

	enum token_control_packet
	{
		TOKEN_PACKET=MAC_PROTOCOL_TOKEN_RING_BUS*100+1,
	};

	typedef struct stru_Token 
	{
		UINT StartDelimiter:8;
		UINT AccessControl:8;
		UINT EndDelimiter:8;
	}TOKEN,*PTOKEN;
#define TOKEN_SIZE 3
	NetSim_PACKET* token;

	typedef struct stru_Token_macHdr
	{
		UINT SD:8;
		UINT AC:8;
		UINT FC:8;
		PNETSIM_MACADDRESS DA;
		PNETSIM_MACADDRESS SA;
		UINT CRC;
		UINT ED:8;
		UINT FS:8;
	}TOKEN_MACHDR,*PTOKEN_MACHDR;

	typedef struct stru_Token_MacVar
	{
		bool tokenHeld;
		NetSim_PACKET* token;
	}TOKEN_MACVAR,*PTOKEN_MACVAR;
#define TOKEN_MAC(devid,ifid) ((PTOKEN_MACVAR)DEVICE_MACVAR(devid,ifid))
#define TOKEN_CURRMAC (TOKEN_MAC(pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId))

	typedef struct struc_Token_PhyVar
	{
		NETSIM_ID nextDevId;
	}TOKEN_PHYVAR,*PTOKEN_PHYVAR;
#define TOKEN_PHY(devid,ifid) ((PTOKEN_PHYVAR)DEVICE_PHYVAR(devid,ifid))
#define TOKEN_CURRPHY (TOKEN_PHY(pstruEventDetails->nDeviceId,pstruEventDetails->nInterfaceId))

	void fn_NetSim_Token_PhyOut();
	void fn_NetSim_Token_PhyIn();
	void fn_NetSim_Token_MacIn();
	void fn_NetSim_Token_StartTokenTransmission();

#ifdef  __cplusplus
}
#endif
#endif //_NETSIM_TOKEN_H_