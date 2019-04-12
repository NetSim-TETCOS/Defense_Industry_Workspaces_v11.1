/************************************************************************************
* Copyright (C) 2013                                                               *
* TETCOS, Bangalore. India                                                         *
*                                                                                  *
* Tetcos owns the intellectual property rights in the Product and its content.     *
* The copying, redistribution, reselling or publication of any or all of the       *
* Product or its content without express prior written consent of Tetcos is        *
* prohibited. Ownership and / or any other right relating to the software and all *
* intellectual property rights therein shall remain at all times with Tetcos.      *
*                                                                                  *
* Author:    Thangarasu                                                       *
*                                                                                  *
* ---------------------------------------------------------------------------------*/
#include "main.h"
#include "Routing.h"
/** This function is used to call the inter and intra area routing protocols */
_declspec(dllexport) int fn_NetSim_Routing_Run()
{
	switch(pstruEventDetails->nProtocolId)
	{
	case APP_PROTOCOL_RIP:
		fn_NetSim_RIP_Run_F();
		break;
	default:
		break;
	}
	return 1;
}
/** This function is used to update the entries in routing table */
_declspec(dllexport)int fn_NetSim_UpdateEntryinRoutingTable(struct stru_NetSim_Network *NETWORK,NetSim_EVENTDETAILS *pstruEventDetails,NETSIM_ID nDeviceId,NETSIM_ID nInterfaceId,unsigned int bgpRemoteAS,NETSIM_IPAddress szDestinationIP,NETSIM_IPAddress szNexthop,NETSIM_IPAddress szSubnetmask,unsigned int prefix_len,int nCost,int nFlag)
{
	DEVICE_ROUTER *pstruRouter = get_RIP_var(nDeviceId);
	if(pstruRouter->AppIntRoutingProtocol == APP_PROTOCOL_RIP)
	{
		fn_NetSim_RIP_UpdatingEntriesinRoutingDatabase(NETWORK,nDeviceId,szDestinationIP,szSubnetmask,szNexthop,nInterfaceId,pstruEventDetails->dEventTime,nCost);	
		if(nFlag)
		{
			fn_NetSim_RIP_TriggeredUpdate(NETWORK,pstruEventDetails);
		}
	}
	
	return 0;
}
/** This function is used to update the protocol status in the interface */
_declspec(dllexport)int UpdateInterfaceList()
{
	NETSIM_ID i,j;
	DEVICE_ROUTER *pstruRouter;
	NETSIM_ID nDeviceCount,nInterfaceCount,nConnectedDevId,nConnectedInterId;
	nDeviceCount = NETWORK->nDeviceCount;
	for(i=0;i<nDeviceCount;i++)
	{
		//if(NETWORK->ppstruDeviceList[i]->nDeviceType == ROUTER)
		if(fnCheckRoutingProtocol(i+1))
		{
			nInterfaceCount = NETWORK->ppstruDeviceList[i]->nNumOfInterface;
			if(NETWORK->ppstruDeviceList[i]->pstruApplicationLayer)
			{
				pstruRouter = get_RIP_var(i + 1);
				if(!pstruRouter->RoutingProtocol)
				{
					pstruRouter->RoutingProtocol = calloc(nInterfaceCount,sizeof* pstruRouter->RoutingProtocol);
				}
				for(j=0;j<nInterfaceCount;j++)
				{
					if(NETWORK->ppstruDeviceList[i]->ppstruInterfaceList[j]->nInterfaceType == INTERFACE_WAN_ROUTER)
					{
						fn_NetSim_Stack_GetConnectedDevice(i+1,j+1,&nConnectedDevId,&nConnectedInterId);
						pstruRouter->RoutingProtocol[j] = APP_PROTOCOL_RIP;
					}
				}
			}
		}
	}
	return 0;
}
int fnCheckRoutingPacket()
{
	if(pstruEventDetails->pPacket && pstruEventDetails->pPacket->nControlDataType/100 == APP_PROTOCOL_RIP)
		return 1;
	return 0;
}

APPLICATION_LAYER_PROTOCOL fnCheckRoutingProtocol(NETSIM_ID deviceId)
{
	if(fn_NetSim_Stack_isProtocolConfigured(deviceId,APPLICATION_LAYER,APP_PROTOCOL_RIP))
		return APP_PROTOCOL_RIP;
	else
		return APP_PROTOCOL_NULL;
}
