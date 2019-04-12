/************************************************************************************
 * Copyright (C) 2016
 * TETCOS, Bangalore. India															*

 * Tetcos owns the intellectual property rights in the Product and its content.     *
 * The copying, redistribution, reselling or publication of any or all of the       *
 * Product or its content without express prior written consent of Tetcos is        *
 * prohibited. Ownership and / or any other right relating to the software and all  *
 * intellectual property rights therein shall remain at all times with Tetcos.      *
 * Author:	Shashi Kant Suman														*
 * ---------------------------------------------------------------------------------*/

#include "main.h"
#include "LTE.h"

static FILE* fpFemtoCell;

typedef struct stru_interfering_hnb
{
	NETSIM_ID nHNBId;
	NETSIM_ID nHNBif;
	UINT ca_id;

	UINT channelIndex;
	double allocated_frequency_max;
	double allocated_frequency_min;
	UINT allocated_rb_count;
	UINT allocated_rb_start_index;
	UINT allocated_rb_end_index;
	_ele* ele;
}INTERFERING_HNB,*PINTERFERING_HNB;
#define INTERFERING_HNB_ALLOC() ((PINTERFERING_HNB)list_alloc(sizeof(INTERFERING_HNB),offsetof(INTERFERING_HNB,ele)))

typedef struct stru_interfering_group
{
	UINT id;
	double fMax;
	double fMin;
	double bandwidth;
	UINT rbCount;
	PINTERFERING_HNB hnbList;
	_ele* ele;
}INTERFERING_GROUP,*PINTERFERING_GROUP;
#define INTERFERING_GROUP_ALLOC() ((PINTERFERING_GROUP)list_alloc(sizeof(INTERFERING_GROUP),offsetof(INTERFERING_GROUP,ele)))

typedef struct stru_hnb_info
{
	NETSIM_ID nHNBId;
	NETSIM_ID nHNBIf;
	NETSIM_ID nHNBGatewayId;
	NETSIM_ID nHNBGatewayIf;
}HNB_INFO,*PHNB_INFO;

typedef struct stru_femtocell_info
{
	NETSIM_ID nFemtocellId;
	NETSIM_ID nHNBGatewayId;
	unsigned int nHNBCount;
	HNB_INFO* hnb_info;
	PINTERFERING_GROUP interfering_group;
}FEMTOCELL_INFO,*PFEMTOCELL_INFO;

static PFEMTOCELL_INFO* femtocell_info;

typedef struct stru_temp_alloc_info
{
	UINT chId;
	UINT shareCount;
	UINT rbCount;
	UINT rbStartIndex;
}TEMP_ALLOC_INFO;

void print_femtocell_group()
{
	NETSIM_ID i,j;
	PFEMTOCELL_INFO info;

	fprintf(fpFemtoCell,"\n\n# Femtocell Group\n");
	fprintf(fpFemtoCell,"HNB Gateway Id\t\tHNB Id-HNB interface\n");

	for(i=1;i<=NETWORK->nDeviceCount;i++)
	{
		if(DEVICE_TYPE(i)==HNB_GATEWAY)
		{
			info = femtocell_info[i];
			fprintf(fpFemtoCell,"%d\t\t",info->nHNBGatewayId);
			
			for(j=1;j<=info->nHNBCount;j++)
				fprintf(fpFemtoCell,"%d-%d,",
				info->hnb_info[j].nHNBId,
				info->hnb_info[j].nHNBIf);
			fprintf(fpFemtoCell,"\n");
		}
	}
	fflush(fpFemtoCell);
}

void femtocell_init()
{
	NETSIM_ID i,j;
	NETSIM_ID id=1;
	NETSIM_ID* ifids;

	char s[BUFSIZ];
	sprintf(s,"%s%s%s",pszIOPath,pathSeperator,"FemtoCell.txt");
	fpFemtoCell = fopen(s,"w");

	femtocell_info = (PFEMTOCELL_INFO*)calloc(NETWORK->nDeviceCount+1,sizeof(PFEMTOCELL_INFO));

	for(i=1;i<=NETWORK->nDeviceCount;i++)
	{
		if(DEVICE_TYPE(i)==HNB_GATEWAY)
		{
			femtocell_info[i] = (PFEMTOCELL_INFO)calloc(1,sizeof(FEMTOCELL_INFO));
			femtocell_info[i]->nFemtocellId = id++;
			femtocell_info[i]->nHNBGatewayId = i;
			ifids = fn_NetSim_Stack_GetInterfaceIdbyInterfaceType(i,
				INTERFACE_LTE,
				&femtocell_info[i]->nHNBCount);
			femtocell_info[i]->hnb_info = (HNB_INFO*)calloc(femtocell_info[i]->nHNBCount+1,
				sizeof(HNB_INFO));
			for(j=1;j<=femtocell_info[i]->nHNBCount;j++)
			{
				NETSIM_ID faltu;
				PHNB_INFO info = &femtocell_info[i]->hnb_info[j];
				info->nHNBGatewayId = i;
				info->nHNBGatewayIf = ifids[j-1];
				fn_NetSim_Stack_GetConnectedDevice(i,ifids[j-1],
					&info->nHNBId,
					&faltu);
				info->nHNBIf = 1;
			}
		}
	}

	print_femtocell_group();
}

void femotocell_finish()
{
	NETSIM_ID i;
	for(i=1;i<=NETWORK->nDeviceCount;i++)
	{
		if(femtocell_info[i])
		{
			PINTERFERING_GROUP g = femtocell_info[i]->interfering_group;
			while(g)
			{
				PINTERFERING_HNB h = g->hnbList;
				while(h)
					LIST_FREE(&h,h);
				LIST_FREE(&g,g);
			}

			free(femtocell_info[i]->hnb_info);
			free(femtocell_info[i]);
		}
	}
	free(femtocell_info);
	fclose(fpFemtoCell);
}

static bool check_for_interfering_group(CA_PHY phyi,CA_PHY phyj)
{
	if(phyi.dDLFrequency_max == phyj.dDLFrequency_max &&
		phyi.dDLFrequency_min == phyj.dDLFrequency_min)
		return true;
	return false;
}

static void form_interfering_group(NETSIM_ID gid)
{
	UINT id =1;
	bool *isVisited[MAX_CA_COUNT];
	UINT i,j,k,l;
	for(i=0;i<MAX_CA_COUNT;i++)
		isVisited[i] = (bool*)calloc(femtocell_info[gid]->nHNBCount+1,sizeof * isVisited[i]);
	for(i=1;i<=femtocell_info[gid]->nHNBCount;i++)
	{
		NETSIM_ID enbId = femtocell_info[gid]->hnb_info[i].nHNBId;
		NETSIM_ID enbIf = femtocell_info[gid]->hnb_info[i].nHNBIf;
		LTE_ENB_PHY* phy = get_enb_phy(enbId,enbIf);
		for(j=0;j<phy->ca_count;j++)
		{
			PINTERFERING_GROUP t;
			PINTERFERING_HNB h;

			if(isVisited[j][i]) //Check if already a part of interfering group
				continue;

			t = INTERFERING_GROUP_ALLOC();
			LIST_ADD_LAST((void**)&femtocell_info[gid]->interfering_group,t);
			
			t->fMax = phy->ca_phy[j].dDLFrequency_max;
			t->fMin = phy->ca_phy[j].dDLFrequency_min;
			t->bandwidth = phy->ca_phy[j].dChannelBandwidth;
			t->rbCount = phy->ca_phy[j].nResourceBlockCount;

			t->id = id++;
			
			h=INTERFERING_HNB_ALLOC();
			LIST_ADD_LAST((void**)&t->hnbList,h);
			h->ca_id=j;
			h->nHNBId=enbId;
			h->nHNBif=enbIf;

			//Mark visited
			isVisited[j][i]=true;

			for(k=1;k<=femtocell_info[gid]->nHNBCount;k++)
			{
				NETSIM_ID eid = femtocell_info[gid]->hnb_info[k].nHNBId;
				NETSIM_ID eif = femtocell_info[gid]->hnb_info[k].nHNBIf;
				LTE_ENB_PHY* kphy = get_enb_phy(eid,eif);
				for(l=0;l<phy->ca_count;l++)
				{
					if(isVisited[l][k])
						continue;

					if(check_for_interfering_group(phy->ca_phy[j],kphy->ca_phy[l]))
					{
						h=INTERFERING_HNB_ALLOC();
						LIST_ADD_LAST((void**)&t->hnbList,h);
						h->ca_id=j;
						h->nHNBId=eid;
						h->nHNBif=eif;

						//Mark visited
						isVisited[l][k]=true;
					}
				}
			}
		}
	}
	
	for(i=0;i<MAX_CA_COUNT;i++)
		free(isVisited[i]);

}

static void print_interfering_group(NETSIM_ID gid)
{
	static bool isheading=false;
	PINTERFERING_GROUP g;
	PINTERFERING_HNB h;
	
	if(!isheading)
	{
		fprintf(fpFemtoCell,"\n\n# Interfering Group\n");
		fprintf(fpFemtoCell,"HNB Gateway Id\t\tHNB Ids-CA ids\n");
		isheading = true;
	}

	fprintf(fpFemtoCell,"%d\t\t",femtocell_info[gid]->nHNBGatewayId);

	g=femtocell_info[gid]->interfering_group;
	while(g)
	{
		h=g->hnbList;
		while(h)
		{
			fprintf(fpFemtoCell,"%d-%d,",h->nHNBId,h->ca_id);
			h=LIST_NEXT(h);
		}
		fprintf(fpFemtoCell,"\n");
		g=LIST_NEXT(g);
		fprintf(fpFemtoCell,"   \t\t");
	}
	fprintf(fpFemtoCell,"\n");
	fflush(fpFemtoCell);
}

static int get_hnb_count(PINTERFERING_HNB h)
{
	int c=0;
	while(h)
	{
		c++;
		h=LIST_NEXT(h);
	}
	return c;
}

static void allocate_rbg(PINTERFERING_HNB h,TEMP_ALLOC_INFO* aInfo)
{
	LTE_ENB_MAC* mac = get_enb_mac(h->nHNBId,h->nHNBif);

	mac->ca_mac[h->ca_id].nRBGCount /= aInfo[h->channelIndex].shareCount;

	if(mac->ca_mac[h->ca_id].nRBGCount < 1)
		mac->ca_mac[h->ca_id].nRBGCount = 1; //Minimum one RBG allocated

	h->allocated_rb_count = mac->ca_mac[h->ca_id].nRBGCount *
		mac->ca_mac[h->ca_id].nRBCountInGroup;

	h->allocated_rb_start_index = aInfo[h->channelIndex].rbStartIndex;

	aInfo[h->channelIndex].rbStartIndex += h->allocated_rb_count;
	if(aInfo[h->channelIndex].rbStartIndex == 
		mac->ca_mac[h->ca_id].nRBCount)
		aInfo[h->channelIndex].rbStartIndex = 0; //Repeat allocation

	h->allocated_rb_end_index = h->allocated_rb_start_index +
		h->allocated_rb_count;
}

static void Femotocell_AllocateChannelAndRBG(NETSIM_ID gid)
{
	PINTERFERING_GROUP g;
	PINTERFERING_HNB h;
	g=femtocell_info[gid]->interfering_group;
	while(g)
	{
		int c;
		int ch;
		TEMP_ALLOC_INFO* ainfo;
		int i=0;

		h=g->hnbList;
		c = get_hnb_count(h);
		
		ch = (int)((g->fMax-g->fMin)/g->bandwidth);

		ainfo = (TEMP_ALLOC_INFO*)calloc(ch,sizeof* ainfo);

		//Allocate channel
		while(h)
		{
			h->allocated_frequency_min = g->fMin+i*g->bandwidth;
			h->allocated_frequency_max = g->fMin+(i+1)*g->bandwidth;
			h->allocated_rb_count = g->rbCount;
			h->allocated_rb_start_index = 0;
			h->allocated_rb_end_index = h->allocated_rb_count;
			h->channelIndex = i;

			ainfo[i].chId = i;
			ainfo[i].shareCount++;

			i++;
			if(i==ch) i=0;

			h=LIST_NEXT(h);
		}

		// Allocate RBG
		h=g->hnbList;
		while(h)
		{
			allocate_rbg(h,ainfo);
			h=LIST_NEXT(h);
		}

		free(ainfo);
		g=LIST_NEXT(g);
	}
}

static void print_channelAllocation()
{
	NETSIM_ID i;
	PINTERFERING_GROUP g;
	PINTERFERING_HNB h;

	fprintf(fpFemtoCell,"\n\n# Allocation Information\n");
	fprintf(fpFemtoCell,"HNB Id-CA Id\t\tFmin\tFmax\tRBstart\tRBend\n");
	for(i=1;i<=NETWORK->nDeviceCount;i++)
	{
		if(!femtocell_info[i])
			continue;
		g=femtocell_info[i]->interfering_group;
		while(g)
		{
			h = g->hnbList;
			while(h)
			{
				fprintf(fpFemtoCell,"%d-%d\t\t%0.2lf\t%0.2lf\t\t%d\t%d\n",
					h->nHNBId,
					h->ca_id,
					h->allocated_frequency_min,
					h->allocated_frequency_max,
					h->allocated_rb_start_index,
					h->allocated_rb_end_index);
				h=LIST_NEXT(h);
			}
			g = LIST_NEXT(g);
		}
	}
	fflush(fpFemtoCell);
}

void femtocell_updateRBGCount()
{
	NETSIM_ID i;

	for(i=1;i<=NETWORK->nDeviceCount;i++)
	{
		if(!femtocell_info[i])
			continue;
		form_interfering_group(i);
		print_interfering_group(i);

		Femotocell_AllocateChannelAndRBG(i);
	}

	print_channelAllocation();
}
