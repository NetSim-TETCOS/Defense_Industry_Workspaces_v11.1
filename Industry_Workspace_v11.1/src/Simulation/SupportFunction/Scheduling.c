#include "main.h"
enum enum_Buffer_Manipulation_Type
{
	ADD=1,
	GET,
};
#define fnGetList(pstruPacketlist,Priority) (pstruPacketlist+Priority/(Priority_High-Priority_Medium))
int fn_NetSim_CheckBuffer(NetSim_BUFFER*,NetSim_PACKET*);
NetSim_PACKET* fn_NetSim_Priority(NetSim_BUFFER*,NetSim_PACKET*,int,int);
NetSim_PACKET* fn_NetSim_FIFO(NetSim_BUFFER*,NetSim_PACKET*,int,int);
NetSim_PACKET* fn_NetSim_RoundRobin(NetSim_BUFFER*,NetSim_PACKET*,int,int);
NetSim_PACKET* fn_NetSim_WFQ(NetSim_BUFFER*,NetSim_PACKET*,int,int);
unsigned long long int fn_NetSim_GetPosition_MaximumNumber(unsigned long long int*);
int fn_NetSim_GetBufferStatus(NetSim_BUFFER*);
/**
This function is to check whether buffer list has any packet or not
*/
_declspec(dllexport) int fn_NetSim_GetBufferStatus(NetSim_BUFFER* pstruBuffer)
{
	SCHEDULING_TYPE Scheduling_Technique;
	NetSim_PACKET* pstruTempList;
	int nFlag=0;

	Scheduling_Technique=pstruBuffer->nSchedulingType;
	switch(Scheduling_Technique)
	{
	case 0:
	case SCHEDULING_PRIORITY:
	case SCHEDULING_FIFO:
		if(!pstruBuffer->pstruPacketlist)
			return 0;
		else
			return 1;
		break;
	case SCHEDULING_ROUNDROBIN:
	case SCHEDULING_WFQ:
		if(pstruBuffer->pstruPacketlist)
		{
Retry:
		nFlag++;
		if(nFlag==5)
			return 0;
		else
		{	
			pstruBuffer->pstruPacketlist->nPacketPriority -= Priority_High-Priority_Medium;
			if(pstruBuffer->pstruPacketlist->nPacketPriority <= 0)
				pstruBuffer->pstruPacketlist->nPacketPriority=Priority_High;

			pstruTempList=fnGetList(pstruBuffer->pstruPacketlist,pstruBuffer->pstruPacketlist->nPacketPriority);
			if(pstruTempList->pstruNextPacket!=NULL)
				return 1;
			else
				goto Retry;	
		}
		break;
		}
		else
			return 0;
	default:
		return 0;
		break;
	}
}
/** This function is to add packet to the buffer */
_declspec(dllexport)int fn_NetSim_Packet_AddPacketToBuffer(NetSim_BUFFER* pstruBuffer, NetSim_PACKET* pstruData)
{
	int nBufferFlag;
	int nType=1;
	SCHEDULING_TYPE Scheduling_Technique;
	nBufferFlag=fn_NetSim_CheckBuffer(pstruBuffer,pstruData);
	if(nBufferFlag==Buffer_Underflow)
	{
		pstruBuffer->nQueuedPacket++;
		Scheduling_Technique=pstruBuffer->nSchedulingType;
		switch(Scheduling_Technique)
		{
		case SCHEDULING_PRIORITY:
			fn_NetSim_Priority(pstruBuffer,pstruData,nType,1);
			break;
		case SCHEDULING_FIFO:
			fn_NetSim_FIFO(pstruBuffer,pstruData,nType,1);
			break;
		case SCHEDULING_ROUNDROBIN:
			fn_NetSim_RoundRobin(pstruBuffer,pstruData,nType,1);
			break;
		case SCHEDULING_WFQ:
			fn_NetSim_WFQ(pstruBuffer,pstruData,nType,1);
			break;
		default:
			fn_NetSim_FIFO(pstruBuffer,pstruData,nType,1);
			break;
		}
	}
	else if(nBufferFlag==Buffer_Overflow)
	{
		pstruBuffer->nDroppedPacket++;
		pstruData->nPacketStatus=PacketStatus_Buffer_Dropped;
		fn_NetSim_WritePacketTrace(pstruData);
		fn_NetSim_Packet_FreePacket(pstruData);
		pstruData=NULL;
	}
	else
	{
		assert(false);
	}
	return 0;
}
/** This function is to get the packet from the buffer */
_declspec(dllexport) NetSim_PACKET* fn_NetSim_GetPacketFromBuffer(NetSim_BUFFER* pstruBuffer,int nFlag)
{

	int nType=2;
	int nScheduling_Technique;
	NetSim_PACKET* pstruData=NULL;
	if(pstruBuffer->pstruPacketlist)
	{
		if(nFlag)
			pstruBuffer->nDequeuedPacket++;
		nScheduling_Technique=pstruBuffer->nSchedulingType;
		switch(nScheduling_Technique)
		{
		case SCHEDULING_PRIORITY:
			pstruData = fn_NetSim_Priority(pstruBuffer,pstruData,nType,nFlag);
			break;
		case SCHEDULING_FIFO:
			pstruData = fn_NetSim_FIFO(pstruBuffer,pstruData,nType,nFlag);
			break;
		case SCHEDULING_ROUNDROBIN:
			pstruData = fn_NetSim_RoundRobin(pstruBuffer,pstruData,nType,nFlag);
			break;
		case SCHEDULING_WFQ:
			pstruData = fn_NetSim_WFQ(pstruBuffer,pstruData,nType,nFlag);
			break;
		default:
			pstruData = fn_NetSim_FIFO(pstruBuffer,pstruData,nType,nFlag);
			break;
		}
	}
	if(!pstruData)
		return NULL;
	
	if(nFlag &&
		pstruBuffer->dMaxBufferSize)
	{
		double d_NN_DataSize;
		if(pstruData->pstruNetworkData)
			d_NN_DataSize=pstruData->pstruNetworkData->dPacketSize;
		else
			d_NN_DataSize=pstruData->pstruMacData->dPacketSize;
		pstruBuffer->dCurrentBufferSize -= (double)((int)d_NN_DataSize);
		if(pstruBuffer->dCurrentBufferSize < 0)
		{
			assert(false);
			pstruBuffer->dCurrentBufferSize = 0;
			fnNetSimError("Scheduling--- Buffer size negative. how?????\n");
		}
	}
	return pstruData;
}

/**
The function will get the array and it will return the Maximum number's position
*/
unsigned long long int fn_NetSim_GetPosition_MaximumNumber(unsigned long long int *nCount)
{
	unsigned long long int nMax;
	int nLoop,nFlag=0;
	nMax=nCount[0];
	for(nLoop=1;nLoop<4;nLoop++)
	{
		if(nCount[nLoop]>=nMax)
		{
			nMax=nCount[nLoop];
			nFlag=nLoop;
		}
	}

	return nFlag+1;
}

/**
If the Current buffer size is greater than the Maximum buffer size --> Buffer Overflow
Otherwise Buffer underflows.
*/
int fn_NetSim_CheckBuffer(NetSim_BUFFER* pstruBuffer, NetSim_PACKET* pstruData)
{
	double d_NN_DataSize=0;
	double ldMaximumBufferSize;
	double ldCurrentBufferSize;
	NetSim_PACKET* p = pstruData;

	while(p)
	{
		if(pstruData->pstruNetworkData)
			d_NN_DataSize+=pstruData->pstruNetworkData->dPacketSize;
		else
			d_NN_DataSize+=pstruData->pstruMacData->dPacketSize;
		p=p->pstruNextPacket;
	}
	if(!pstruBuffer->dMaxBufferSize)
		pstruBuffer->dMaxBufferSize = 0xFFFFFFFFFFFFFFFF;
	ldMaximumBufferSize=pstruBuffer->dMaxBufferSize*1024*1024;
	ldCurrentBufferSize=pstruBuffer->dCurrentBufferSize+d_NN_DataSize;
	if(ldCurrentBufferSize<=ldMaximumBufferSize)
	{
		//buffer underflow
		pstruBuffer->dCurrentBufferSize=(double)((int)ldCurrentBufferSize);
		return Buffer_Underflow;
	}
	else
		//buffer overflow
		return Buffer_Overflow;

}



/**
If the type is ADD, then all the packets will be added in the buffer based on packet priority
If the type is GET, then the packets to be retrieved based on the priority
*/
NetSim_PACKET* fn_NetSim_Priority(NetSim_BUFFER* pstruBuffer, NetSim_PACKET* pstruData,int nType,int nFlag)
{

	switch(nType)
	{
	case ADD:
		if(pstruBuffer->pstruPacketlist)
		{
			NetSim_PACKET *pstruTmpBufferPacketList;
			NetSim_PACKET *pstruPrevPacketList;

			pstruTmpBufferPacketList = pstruBuffer->pstruPacketlist;
			pstruPrevPacketList = pstruBuffer->pstruPacketlist;
			if(pstruTmpBufferPacketList->nPacketPriority < pstruData->nPacketPriority)
			{
				pstruData->pstruNextPacket=pstruTmpBufferPacketList;
				pstruBuffer->pstruPacketlist = pstruData;
				return 0;
			}
			while(pstruTmpBufferPacketList)
			{
				if(pstruTmpBufferPacketList->nPacketPriority < pstruData->nPacketPriority)
				{
					pstruData->pstruNextPacket=pstruTmpBufferPacketList; 
					pstruPrevPacketList->pstruNextPacket = pstruData;
					break;
				}
				else if(!pstruTmpBufferPacketList->pstruNextPacket)
				{
					pstruTmpBufferPacketList->pstruNextPacket = pstruData;
					break;
				}
				else
				{
					pstruPrevPacketList = pstruTmpBufferPacketList;
					pstruTmpBufferPacketList = pstruTmpBufferPacketList->pstruNextPacket;
				}
			}

		}
		else
			pstruBuffer->pstruPacketlist=pstruData;	
		break;
	case GET:
		{
			NetSim_PACKET* pstruPacket;
			pstruPacket=pstruBuffer->pstruPacketlist;
			if(nFlag)
			{
				pstruBuffer->pstruPacketlist = pstruPacket->pstruNextPacket;
				pstruPacket->pstruNextPacket = NULL;
			}
			return pstruPacket;
			break;
		}
	default:
		printf("\nInvalid Selection in Buffer\n");
		break;
	}
	return NULL;

}
/**
If the type is ADD, then all the packets will be added in the buffer based on the arrival
If the type is GET, then the packets to be retrieved depends on the arrangement in the queue
*/
NetSim_PACKET* fn_NetSim_FIFO(NetSim_BUFFER* pstruBuffer, NetSim_PACKET* pstruData,int nType,int nFlag)
{
	NetSim_PACKET* p=pstruData;
	while(p && p->pstruNextPacket)
		p=p->pstruNextPacket;
	switch(nType)
	{
	case ADD:
		if(pstruBuffer->pstruPacketlist)
		{
			pstruBuffer->last->pstruNextPacket = pstruData;
			pstruBuffer->last = p;
		}
		else
		{
			pstruBuffer->pstruPacketlist=pstruData;
			pstruBuffer->last = p;
		}
		break;
	case GET:
		{
			NetSim_PACKET* pstruPacket;
			pstruPacket=pstruBuffer->pstruPacketlist;
			if(nFlag)
			{
				pstruBuffer->pstruPacketlist = pstruPacket->pstruNextPacket;
				if(pstruBuffer->pstruPacketlist == NULL)
					pstruBuffer->last = NULL;
				pstruPacket->pstruNextPacket = NULL;
			}
			return pstruPacket;
			break;
		}
	default:
		printf("\nInvalid selection in Buffer\n");
		break;
	}
	return 0;
}
/**
If the type is ADD, then all the packets will be added in the buffer based on packet priority in the corresponding list
If the type is GET, then the packets to be retrieved in the order of one packet from each list(high priority list,Medium,Normal,Low)
*/
NetSim_PACKET* fn_NetSim_RoundRobin(NetSim_BUFFER* pstruBuffer, NetSim_PACKET* pstruData,int nType,int nFlag)
{	
	switch(nType)
	{
	case ADD:
		{
			NetSim_PACKET *pstruTmpBufferPacketList;
			NetSim_PACKET *pstruTempTraversePacketList;

			if(pstruBuffer->pstruPacketlist==NULL)
			{
				pstruBuffer->pstruPacketlist=fnpAllocateMemory(5,sizeof *pstruData);
			}
			pstruTmpBufferPacketList=pstruBuffer->pstruPacketlist;
			if(pstruData->nPacketPriority)
			{
			pstruTempTraversePacketList=fnGetList(pstruTmpBufferPacketList,pstruData->nPacketPriority);

			while(pstruTempTraversePacketList->pstruNextPacket!=NULL)
				pstruTempTraversePacketList=pstruTempTraversePacketList->pstruNextPacket;
			pstruTempTraversePacketList->pstruNextPacket=pstruData;	
			}
			else
			{
			pstruTempTraversePacketList=fnGetList(pstruTmpBufferPacketList,Priority_Low);
			while(pstruTempTraversePacketList->pstruNextPacket!=NULL)
				pstruTempTraversePacketList=pstruTempTraversePacketList->pstruNextPacket;
			pstruTempTraversePacketList->pstruNextPacket=pstruData;	
			}
			break;
		}
	case GET:
		{
			NetSim_PACKET* pstruTemp;
			NetSim_PACKET* pstruTempList;
			int nFlag1=0;
			PACKET_PRIORITY nPacketPriority;
			if(pstruBuffer->pstruPacketlist)
			{
			nPacketPriority = pstruBuffer->pstruPacketlist->nPacketPriority;
Retry:
			nFlag1++;
			if(nFlag1==5)
				return NULL;
			else
			{	
				pstruBuffer->pstruPacketlist->nPacketPriority -= Priority_High-Priority_Medium;
				if(pstruBuffer->pstruPacketlist->nPacketPriority <= 0)
					pstruBuffer->pstruPacketlist->nPacketPriority=Priority_High;

				pstruTempList=fnGetList(pstruBuffer->pstruPacketlist,pstruBuffer->pstruPacketlist->nPacketPriority);
				if(pstruTempList->pstruNextPacket!=NULL)
				{
					pstruTemp=pstruTempList->pstruNextPacket;
					if(nFlag)
					{
						pstruTempList->pstruNextPacket=pstruTemp->pstruNextPacket;
						pstruTemp->pstruNextPacket=NULL;
					}
					else
						pstruBuffer->pstruPacketlist->nPacketPriority = nPacketPriority;
					return pstruTemp;
				}
				else
					goto Retry;	
			}
			}
			else
				return NULL;
		}
	default:
		printf("\nInvalid Selection in Buffer\n");
		break;
	}
	return 0;
}


/**
If the type is ADD, then all the packets will be added in the buffer based on packet priority in the corresponding list
If the type is GET, then the packets to be retrieved in the order of maximum weight priority list(High,Medium,Normal,Low)
*/
NetSim_PACKET* fn_NetSim_WFQ(NetSim_BUFFER* pstruBuffer, NetSim_PACKET* pstruData,int nType,int nFlag)
{
	switch(nType)
	{
	case ADD:
		{
			NetSim_PACKET *pstruTmpBufferPacketList;
			NetSim_PACKET *pstruTempTraversePacketList;

			if(pstruBuffer->pstruPacketlist==NULL)
			{
				pstruBuffer->pstruPacketlist=fnpAllocateMemory(5,sizeof *pstruData);
			}
			pstruTmpBufferPacketList=pstruBuffer->pstruPacketlist;
			if(pstruData->nPacketPriority)
			{
			pstruTempTraversePacketList=fnGetList(pstruTmpBufferPacketList,pstruData->nPacketPriority);

			pstruTempTraversePacketList->nPacketId++;

			while(pstruTempTraversePacketList->pstruNextPacket!=NULL)
				pstruTempTraversePacketList=pstruTempTraversePacketList->pstruNextPacket;
			pstruTempTraversePacketList->pstruNextPacket=pstruData;	
			}
			else
			{
			pstruTempTraversePacketList=fnGetList(pstruTmpBufferPacketList,Priority_Low);

			pstruTempTraversePacketList->nPacketId++;

			while(pstruTempTraversePacketList->pstruNextPacket!=NULL)
				pstruTempTraversePacketList=pstruTempTraversePacketList->pstruNextPacket;
			pstruTempTraversePacketList->pstruNextPacket=pstruData;	
			}

			break;
		}
	case GET:
		{
			unsigned long long int nCount[4],nPosition;
			NetSim_PACKET* pstruTempList;
			NetSim_PACKET* pstruTemp;
			NetSim_PACKET* pstruHighPriorityList;
			NetSim_PACKET* pstruMediumPriorityList;
			NetSim_PACKET* pstruNormalPriorityList;
			NetSim_PACKET* pstruLowPriorityList;

			pstruLowPriorityList=fnGetList(pstruBuffer->pstruPacketlist,Priority_Low);
			nCount[0]=pstruLowPriorityList->nPacketId*1;

			pstruNormalPriorityList=fnGetList(pstruBuffer->pstruPacketlist,Priority_Normal);
			nCount[1]=pstruNormalPriorityList->nPacketId*2;

			pstruMediumPriorityList=fnGetList(pstruBuffer->pstruPacketlist,Priority_Medium);
			nCount[2]=pstruMediumPriorityList->nPacketId*3;

			pstruHighPriorityList=fnGetList(pstruBuffer->pstruPacketlist,Priority_High);
			nCount[3]=pstruHighPriorityList->nPacketId*4;


			nPosition=fn_NetSim_GetPosition_MaximumNumber(nCount);



			pstruTempList = pstruBuffer->pstruPacketlist+nPosition;

			if(pstruTempList && pstruTempList->pstruNextPacket)
			{
				pstruTempList->nPacketId--;
				pstruTemp = pstruTempList->pstruNextPacket;
				if(nFlag)
				{
					pstruTempList->pstruNextPacket = pstruTemp->pstruNextPacket;
					pstruTemp->pstruNextPacket = NULL;
				}
				else
					pstruTempList->nPacketId++;
				return pstruTemp;
			}
			else
				return NULL;
		}
	default:
		printf("\nInvalid Selection in Buffer\n");
		break;
	}
	return 0;
}

