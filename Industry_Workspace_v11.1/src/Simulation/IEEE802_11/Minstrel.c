/************************************************************************************
* Copyright (C) 2016                                                               *
* TETCOS, Bangalore. India                                                         *
*                                                                                  *
* The code Minstrel Rate Control algorithm is provided per GNU GPLv2 licensing     *
*                                                                                  *
* Author:   Dhruvang									                           *
*                                                                                  *
* ---------------------------------------------------------------------------------*/

#include "Minstrel.h"

static int show=0;		//variable for showing Minstrel Table


#define SEED_r &NETWORK->ppstruDeviceList[0]->ulSeed[0],&NETWORK->ppstruDeviceList[0]->ulSeed[1]
#define rand01() (fn_NetSim_Utilities_GenerateRandomNo(SEED_r)/NETSIM_RAND_MAX)
#define ackTime(phy) (phy->plmeCharacteristics.aSIFSTime + get_preamble_time(phy) + (getAckSize(phy) * 8)/phy->dControlFrameDataRate)
#define tSlot(phy) (phy->plmeCharacteristics.aSlotTime)
#define transmissionTime(phy,byte,rate) (phy->plmeCharacteristics.aSIFSTime + get_preamble_time(phy)+ byte*8/rate)

static Ptr_MinstrelPerRemoteStation getMinstrelInfo(NETSIM_ID dev,NETSIM_ID ifid,NETSIM_ID recv)
{
	switch (IEEE802_11_PHY(dev, ifid)->PhyProtocol)
	{
		case IEEE_802_11a:
		case IEEE_802_11b:
		case IEEE_802_11g:
		case IEEE_802_11p:
		{
			Ptr_MinstrelWifiStation station = IEEE802_11_PHY(dev, ifid)->rateAdaptationData;
			return station->minstrelInfo[recv];
		}
			break;
		case IEEE_802_11ac:
		case IEEE_802_11n:
			break;
		default:
			fnNetSimError("Unknown phy protocol in %s\n", __FUNCTION__);
	}
	return NULL;
}

void get_data_rate(NETSIM_ID devid,NETSIM_ID ifid,double* rate,unsigned int* len)
{
	PIEEE802_11_PHY_VAR phy = IEEE802_11_PHY(devid,ifid);
	switch(phy->PhyProtocol)
	{
	case IEEE_802_11a:
	case IEEE_802_11g:
	case IEEE_802_11p:
		get_ofdm_phy_rate((int)phy->dChannelBandwidth,phy->PhyProtocol,rate,len);
		break;
	case IEEE_802_11b:
		get_dsss_phy_rate((int)phy->dChannelBandwidth,phy->PhyProtocol,rate,len);
		break;
	case IEEE_802_11n:
	case IEEE_802_11ac:
	default:
		fnNetSimError("Unknown phy protocol %d in %s.\n",phy->PhyProtocol,__FUNCTION__);
		return;
	}
}

void Minstrel_Init(NETSIM_ID nDevId,NETSIM_ID nifid)
{
	switch(IEEE802_11_PHY(nDevId,nifid)->PhyProtocol)
	{
	case IEEE_802_11a:
	case IEEE_802_11b:
	case IEEE_802_11g:
	case IEEE_802_11p:
		InitMinstrel(nDevId,nifid);
		break;
	case IEEE_802_11ac:
	case IEEE_802_11n:
		Ht_InitMinstrel(nDevId,nifid);
		break;
	default:
		fnNetSimError("Unknown phy protocol in %s\n",__FUNCTION__);
	}
}

BOOL Minstrel_DoNeedDataSend(NETSIM_ID nDevId,NETSIM_ID nifid,NETSIM_ID recvid)
{
	switch(IEEE802_11_PHY(nDevId,nifid)->PhyProtocol)
	{
	case IEEE_802_11a:
	case IEEE_802_11b:
	case IEEE_802_11g:
	case IEEE_802_11p:
		return DoNeedDataRetransmission(nDevId,nifid,recvid);
		break;
	case IEEE_802_11ac:
	case IEEE_802_11n:
		return Ht_DoNeedDataRetransmission(nDevId,nifid,recvid);
		break;
	default:
		fnNetSimError("Unknown phy protocol in %s\n",__FUNCTION__);
	}
	return false;
}

void Minstrel_ReportDataFailed(NETSIM_ID nDevId,NETSIM_ID nifid,NETSIM_ID recvid)
{
	switch(IEEE802_11_PHY(nDevId,nifid)->PhyProtocol)
	{
	case IEEE_802_11a:
	case IEEE_802_11b:
	case IEEE_802_11g:
	case IEEE_802_11p:
		DoReportDataFailed(nDevId,nifid,recvid);
		break;
	case IEEE_802_11ac:
	case IEEE_802_11n:
		{
			UINT c = 0;
			NetSim_PACKET* p = IEEE802_11_MAC(nDevId,nifid)->currentProcessingPacket;
			while(p)
			{
				c++;
				p=p->pstruNextPacket;
			}
			DoReportAmpduStatus(nDevId,nifid,recvid,0,c);
		}
		break;
	default:
		fnNetSimError("Unknown phy protocol in %s\n",__FUNCTION__);
	}
}

void Minstrel_ReportFinalDataFailed(NETSIM_ID nDevId,NETSIM_ID nifid,NETSIM_ID recvid)
{
	switch(IEEE802_11_PHY(nDevId,nifid)->PhyProtocol)
	{
	case IEEE_802_11a:
	case IEEE_802_11b:
	case IEEE_802_11g:
	case IEEE_802_11p:
		DoReportFinalDataFailed(nDevId,nifid,recvid);
		break;
	case IEEE_802_11ac:
	case IEEE_802_11n:
		{
			UINT c = 0;
			NetSim_PACKET* p = IEEE802_11_MAC(nDevId,nifid)->currentProcessingPacket;
			while(p)
			{
				c++;
				p=p->pstruNextPacket;
			}
			DoReportAmpduStatus(nDevId,nifid,recvid,0,c);
		}
		break;
	default:
		fnNetSimError("Unknown phy protocol in %s\n",__FUNCTION__);
	}
}

void InitMinstrel(NETSIM_ID nDevId,NETSIM_ID nifid){
	NETSIM_ID i;
	Ptr_MinstrelWifiStation wifiStation;
	//initialize the Minstrel global variables
	show=1;
	updateStatsTime = 100*1000;	//100 ms 
	lookAroundRate= 10;
	sampleCol = 10;
	ewmaWeight = 25;

	wifiStation = (Ptr_MinstrelWifiStation)calloc(1,sizeof(MinstrelWifiStation));
	wifiStation->minstrelInfo = (Ptr_MinstrelPerRemoteStation*)calloc(NETWORK->nDeviceCount+1,sizeof(Ptr_MinstrelPerRemoteStation));		

	for(i=1;i<=NETWORK->nDeviceCount;i++)
	{
		if(isIEEE802_11_Configure(i,1))
		{
			wifiStation->minstrelInfo[i] = (Ptr_MinstrelPerRemoteStation)calloc(1,sizeof(MinstrelPerRemoteStation));
			CheckInit(wifiStation->minstrelInfo[i],nDevId,nifid);
		}
	}
	IEEE802_11_PHY(nDevId,nifid)->rateAdaptationData = wifiStation;
}

static void CheckInit(Ptr_MinstrelPerRemoteStation station,NETSIM_ID nDevId,NETSIM_ID nifid){
	if(!station->isInitialized){
		UINT i,get_nrate;
		double get_rate[10];
		//function to get the available rates for the device
		get_data_rate(nDevId,nifid,get_rate,&get_nrate);

		station->nextStatsUpdate = ldEventTime + updateStatsTime;		//fill
		station->maxTpRate= 0;
		station->maxTp2Rate= 0;
		station->maxProbRate= 0;
		station->phy = IEEE802_11_PHY(nDevId,nifid);

		station->nRate = get_nrate;	//fill
		station->baseRate = 0;
		for(i=0;i<station->nRate;i++)
			if(get_rate[i] < get_rate[station->baseRate])		//fill
				station->baseRate = i;
		
		station->sampleRate= 0;
		station->trRate= 0;
		
		station->isSampling= false;
		station->samplePacketCount= 0;
		station->totalPacketCount= 0;
		station->shortRetry= 0;
		station->longRetry= 0;
		station->totalRetry= 0;

		station->col= 0;
		station->row= 0;
		InitSampleTable(station);
		InitMinstrelTable(station,get_rate);
		station->isInitialized = true;
	}
}

static void InitSampleTable(Ptr_MinstrelPerRemoteStation station){
	UINT i,j;
	//Memory allocation for Sample Table
	station->sampleTable = calloc(station->nRate, sizeof(UINT*));
	for(i=0;i<station->nRate;i++){
		station->sampleTable[i] = calloc(sampleCol,sizeof(UINT));
		for(j=0;j<sampleCol;j++)
			station->sampleTable[i][j] = 10000;
		//memset(station->sampleTable[i],10000,sampleCol*sizeof(UINT));
	}

	//set indices for traversing in Sample Table
	station->col = 0;
	station->row = 0;
	//Randomly filling table with values [0,nRate-1]
	for(j=0;j<sampleCol;j++){
		for(i=0;i<station->nRate;i++){
			//generate the random number
			int random_var = station->nRate*(int)rand01();
			random_var = (i+random_var)%station->nRate;

			while(station->sampleTable[random_var][j] != 10000){
				random_var = (random_var + 1)%station->nRate;
			}
			station->sampleTable[random_var][j] = i;
		}
	}
}

static void InitMinstrelTable(Ptr_MinstrelPerRemoteStation station,double *rate_arr){
	UINT i,cw;
	double trTime;
	station->minstrelTable = (RateInfo*)calloc(station->nRate,sizeof(RateInfo));
	for(i=0;i<station->nRate;i++){
		station->minstrelTable[i].currNumAttempt= 0;
		station->minstrelTable[i].currNumSuccess= 0;
		station->minstrelTable[i].prevNumAttempt = 0;
		station->minstrelTable[i].prevNumSuccess = 0;
		station->minstrelTable[i].totalNumAttempt= 0;
		station->minstrelTable[i].totalNumSuccess= 0;
		station->minstrelTable[i].prob= 0;
		station->minstrelTable[i].ewmaProb= 0;
		station->minstrelTable[i].throughput= 0;
		station->minstrelTable[i].numSampleSkipped= 0;
		station->minstrelTable[i].rate= rate_arr[i];		//fill
		station->minstrelTable[i].sampleLimit= -1;		//no limit
		station->minstrelTable[i].retryCount= 1;	//atleast 1 retry count for each rate
		station->minstrelTable[i].adjustedRetryCount= 1;
		station->minstrelTable[i].perfectTrTime= transmissionTime(station->phy,1500,rate_arr[i]);		//fill
		
		cw = 15;		//minimum contention window
		trTime= station->minstrelTable[i].perfectTrTime + ackTime(station->phy);		//fill

		while(1){
			//add retransmission time
			trTime += station->minstrelTable[i].perfectTrTime + ackTime(station->phy);	//fill

			//contention window and average backoff
			trTime+= (tSlot(station->phy)*cw)>>1;			//fill
			cw = min((cw<<1)|1,1023);		//max contention window = 1023

			//retry count is limited by 6 millisecond
			if(trTime < 6000){
				station->minstrelTable[i].retryCount++;
				station->minstrelTable[i].adjustedRetryCount++;
			}
			else{
				break;
			}
		}	
	}
	UpdateStats(station);
}

void DoReportDataOk(NETSIM_ID dev,NETSIM_ID ifid,NETSIM_ID recv){
	Ptr_MinstrelPerRemoteStation station = getMinstrelInfo(dev,ifid,recv);
	if(!station->isInitialized)
		return;

	station->minstrelTable[station->trRate].currNumAttempt++;
	station->minstrelTable[station->trRate].currNumSuccess++;
	
	UpdatePacketCounter(station);		
	UpdateRetry(station);				
	UpdateStats(station);				

	if(station->nRate >=1)
		station->trRate = FindRate(station);
}

void DoReportDataFailed(NETSIM_ID dev,NETSIM_ID ifid,NETSIM_ID recv){
	Ptr_MinstrelPerRemoteStation station = getMinstrelInfo(dev,ifid,recv);
	if(!station->isInitialized)
		return;
	UpdateRate(station);			
}

static void UpdateRate(Ptr_MinstrelPerRemoteStation station){
	//Retry Chain Table is Implemented here.
	//Only called when data fails

	station->longRetry++;
	station->totalPacketCount++;
	station->minstrelTable[station->trRate].currNumAttempt++;
	//if sample rate is used
	if(station->isSampling && station->trRate == station->sampleRate)	
		station->samplePacketCount++;

	//for normal rate, we aren't sampling rates
	if(!station->isSampling){

		//first try with best throughput
		if(station->longRetry < station->minstrelTable[station->maxTpRate].adjustedRetryCount){
			station->trRate = station->maxTpRate;
		}
		//use 2nd best throughput rate
		else if(station->longRetry <= station->minstrelTable[station->maxTpRate].adjustedRetryCount	+ 
									  station->minstrelTable[station->maxTp2Rate].adjustedRetryCount){
			station->trRate = station->maxTp2Rate;
		}
		//use best probability rate
		else if(station->longRetry <= station->minstrelTable[station->maxTpRate].adjustedRetryCount + 
									station->minstrelTable[station->maxTp2Rate].adjustedRetryCount + 
									station->minstrelTable[station->maxProbRate].adjustedRetryCount){
			station->trRate = station->maxProbRate;
		}
		//use lowest base rate
		else if(station->longRetry > station->minstrelTable[station->maxTpRate].adjustedRetryCount + 
									station->minstrelTable[station->maxTp2Rate].adjustedRetryCount + 
									station->minstrelTable[station->maxProbRate].adjustedRetryCount){
			station->trRate = station->baseRate;
		}
		else{
			fnNetSimError("Failed to Update Rate");
		}
	}
	//for lookaround rate, currently sampling
	else{
		//current sampling is slower than current best rate
		if(station->isDeferredSample){
			//use best throughput rate
			if(station->longRetry < station->minstrelTable[station->maxTpRate].adjustedRetryCount){
				station->trRate = station->maxTpRate;
			}
			//use random rate
			else if(station->longRetry <= station->minstrelTable[station->maxTpRate].adjustedRetryCount + 
									station->minstrelTable[station->sampleRate].adjustedRetryCount){
				station->trRate = station->sampleRate;
			}
			//use best probability rate
			else if(station->longRetry <= station->minstrelTable[station->maxTpRate].adjustedRetryCount + 
									station->minstrelTable[station->sampleRate].adjustedRetryCount + 
									station->minstrelTable[station->maxProbRate].adjustedRetryCount){
				station->trRate = station->maxProbRate;
			}
			//use lowest base rate
			else if(station->longRetry > station->minstrelTable[station->maxTpRate].adjustedRetryCount + 
									station->minstrelTable[station->sampleRate].adjustedRetryCount + 
									station->minstrelTable[station->maxProbRate].adjustedRetryCount){
				station->trRate = station->baseRate;
			}
			else{
			fnNetSimError("Failed to Update Rate");
			}
		}
		//current sampling rate is better than current best rate
		else{
			//use random rate
			if(station->longRetry < station->minstrelTable[station->sampleRate].adjustedRetryCount){
				station->trRate = station->sampleRate;
			}
			//use best throughput rate
			else if(station->longRetry < station->minstrelTable[station->sampleRate].adjustedRetryCount + 
										station->minstrelTable[station->maxTpRate].adjustedRetryCount){
				station->trRate = station->maxTpRate;
			}
			//use best probability rate
			else if(station->longRetry <= station->minstrelTable[station->maxTpRate].adjustedRetryCount + 
									station->minstrelTable[station->sampleRate].adjustedRetryCount + 
									station->minstrelTable[station->maxProbRate].adjustedRetryCount){
				station->trRate = station->maxProbRate;
			}
			//use lowest base rate
			else if(station->longRetry > station->minstrelTable[station->maxTpRate].adjustedRetryCount + 
									station->minstrelTable[station->sampleRate].adjustedRetryCount + 
									station->minstrelTable[station->maxProbRate].adjustedRetryCount){
				station->trRate = station->baseRate;
			}
			else{
			fnNetSimError("Failed to Update Rate");
			}
		}
	}
}

static void UpdatePacketCounter(Ptr_MinstrelPerRemoteStation station){
	//Updates the packet counter of station
	station->totalPacketCount++;

	//If it is sampling and sampling rate is used
	if(station->isSampling && station->trRate == station->sampleRate){
		station->samplePacketCount++;
	}

	if(station->numSampleDeferred >0){
		station->numSampleDeferred--;
	}

	//wrap around
	if(station->totalPacketCount < 0){
		station->numSampleDeferred = 0;
		station->samplePacketCount = 0;
		station->totalPacketCount = 0;
	}

	//whether sampling/deferred sample or not will be decided by "FindRate"
	station->isSampling = false;
	station->isDeferredSample = false;
}

static void UpdateRetry(Ptr_MinstrelPerRemoteStation station){
	station->totalRetry += station->longRetry + station->shortRetry;
	station->longRetry = 0;
	station->shortRetry = 0;
}

static void UpdateStats(Ptr_MinstrelPerRemoteStation station){
	UINT i,index_max_tp, index_max_tp2, index_max_prob;
	double tempProb, max_prob, max_tp;
	//Update time has not come
	if(ldEventTime < station->nextStatsUpdate){
		return;
	}
	if(!station->isInitialized){
		return;
	}
	station->nextStatsUpdate = ldEventTime + updateStatsTime;
	//update for each rate in minstrel table
	for(i=0;i<station->nRate;i++){
		double trTime;
		trTime = station->minstrelTable[i].perfectTrTime;
		
		//just for initialization
		if(trTime == 0){
			trTime = 1000*1000;			//1 second		
		}

		//if attempted something
		if(station->minstrelTable[i].currNumAttempt){
			station->minstrelTable[i].numSampleSkipped = 0;
			//calculate probability of success
			tempProb = (station->minstrelTable[i].currNumSuccess*18000) / station->minstrelTable[i].currNumAttempt;
			tempProb = tempProb/18000.0;
			station->minstrelTable[i].prob = tempProb;

			//Updating this rate for the first time
			if(station->minstrelTable[i].totalNumAttempt == 0){
				station->minstrelTable[i].ewmaProb = tempProb;
			}
			else{
				//ewma probability
				tempProb = (tempProb*ewmaWeight + station->minstrelTable[i].ewmaProb*(100-ewmaWeight))/100;
				station->minstrelTable[i].ewmaProb = tempProb;
			}

			//calculate Throughput(using ewmaProb)
			//linux implementation
			if(tempProb < 0.1)
				station->minstrelTable[i].throughput = 0;      
			else
				station->minstrelTable[i].throughput = (tempProb*8*1500)/(trTime);			//fill

		}
		//if not attempted at this rate
		else{
			station->minstrelTable[i].numSampleSkipped++;
		}

		//bookkeeping
		station->minstrelTable[i].totalNumAttempt += station->minstrelTable[i].currNumAttempt;
		station->minstrelTable[i].totalNumSuccess += station->minstrelTable[i].currNumSuccess;
		station->minstrelTable[i].prevNumAttempt = station->minstrelTable[i].currNumAttempt;
		station->minstrelTable[i].prevNumSuccess = station->minstrelTable[i].currNumSuccess;
		station->minstrelTable[i].currNumAttempt = 0;
		station->minstrelTable[i].currNumSuccess = 0;

		//sample less often below 10% and above 95% success
		if(station->minstrelTable[i].ewmaProb < 0.1 || station->minstrelTable[i].ewmaProb > 0.95){
			/**
		   * See: http://wireless.kernel.org/en/developers/Documentation/mac80211/RateControl/minstrel/
		   *
		   * Analysis of information showed that the system was sampling too hard at some rates.
		   * For those rates that never work (54mb, 500m range) there is no point in retrying 10 sample packets (< 6 ms time).
		   * Consequently, for the very low probability rates, we try at most twice when fails and not sample more than 4 times.
		   */
			if(station->minstrelTable[i].retryCount > 2){
				station->minstrelTable[i].adjustedRetryCount = 2;
			}
			station->minstrelTable[i].sampleLimit = 4;
		}
		else{
			//no sampling limit
			station->minstrelTable[i].sampleLimit = -1;
			station->minstrelTable[i].adjustedRetryCount = station->minstrelTable[i].retryCount;
		}

		//if its 0,allow two retries (generally wont happen)
		if(station->minstrelTable[i].adjustedRetryCount == 0)
			station->minstrelTable[i].adjustedRetryCount = 2;
	}

	max_tp=0;
	index_max_tp= 0;
	index_max_tp2= 0;
	index_max_prob=0;
	max_prob=0;
	//find the max throughput
	for(i=0;i<station->nRate;i++){
		if(max_tp < station->minstrelTable[i].throughput){
			index_max_tp = i;
			max_tp = station->minstrelTable[i].throughput;
		}
	}
	//find the 2nd max throughput and high probability succ
	max_tp=0;
	for(i=0;i<station->nRate;i++){
		if(i != index_max_tp && max_tp < station->minstrelTable[i].throughput){
			index_max_tp2 = i;
			max_tp = station->minstrelTable[i].throughput;
		}
	}
	//find high probability success rate
	for(i=0;i<station->nRate;i++){
		//above 95% choose with highest throughput
		if(station->minstrelTable[i].ewmaProb >= 0.95 && station->minstrelTable[i].throughput >= station->minstrelTable[index_max_prob].throughput){
			index_max_prob = i;
			max_prob = station->minstrelTable[i].ewmaProb;
		}
		else if(station->minstrelTable[i].ewmaProb >= max_prob){
			index_max_prob = i;
			max_prob = station->minstrelTable[i].ewmaProb;
		}
	}
	station->maxTpRate = index_max_tp;
	station->maxTp2Rate = index_max_tp2;
	station->maxProbRate = index_max_prob;
}

static UINT FindRate(Ptr_MinstrelPerRemoteStation station){
	UINT idx;
	int delta;
	if(station->totalPacketCount == 0)
		return 0;

	delta = (int)(((station->totalPacketCount*lookAroundRate)/100) - (station->samplePacketCount + station->numSampleDeferred/2 ));
	//if delta < 0: no sampling required
	//if delta > 0: insufficient sampling
	if(delta >=0)
	{
		UINT n_rates = station->nRate;
		if(delta > (int)(2* n_rates)){
			/* From Linux implementation:
		   * With multi-rate retry, not every planned sample
		   * attempt actually gets used, due to the way the retry
		   * chain is set up - [max_tp,sample,prob,lowest] for
		   * sample_rate < max_tp.
		   *
		   * If there's too much sampling backlog and the link
		   * starts getting worse, minstrel would start bursting
		   * out lots of sampling frames, which would result
		   * in a large throughput loss. */
			station->samplePacketCount += delta - 2*n_rates;
		}

		idx = GetNextSample(station);	//fill
		if(idx >= n_rates)
			fnNetSimError("Unknown rate...!");

		station->sampleRate = idx;
		
		/* From Linux implementation:
	   * Decide if direct ( 1st mrr stage) or indirect (2nd mrr stage)
	   * rate sampling method should be used.
	   * Respect such rates that are not sampled for 20 iterations.
	   */

		if((station->minstrelTable[idx].perfectTrTime > station->minstrelTable[station->maxTpRate].perfectTrTime)
			&& station->minstrelTable[idx].numSampleSkipped < 20){
			//if rate is slower defer to 2nd stage
			station->isDeferredSample = true;
			station->numSampleDeferred++;

			//set flag for currently sampling
			station->isSampling = true;
		}
		else{
			//if sample limit is zero then dont sample this rate
			if(station->minstrelTable[idx].sampleLimit == 0){
				idx = station->maxTpRate;
				station->isSampling = false;
			}
			else{
				//set flag for currently sampling
				station->isSampling = true;
				if(station->minstrelTable[idx].sampleLimit > 0)
					station->minstrelTable[idx].sampleLimit--;
			}
		}

		//sample rate is slower
		if(station->isDeferredSample){
			idx = station->maxTpRate;
		}
	}
	//delta < 0: no sampling required, continue using best rate
	else
	{
		idx = station->maxTpRate;
	}

	return idx;
}

static UINT GetNextSample(Ptr_MinstrelPerRemoteStation station){
	UINT sample_rate = station->sampleTable[station->row][station->col];

	//bookkeeping for next sample index
	station->row++;
	if(station->row >= station->nRate){
		station->row = 0;
		station->col++;
		if(station->col >= sampleCol)
			station->col = 0;
	}
	return sample_rate;
}

//void DoReportFinalDataFailed(Ptr_MinstrelPerRemoteStation station){
void DoReportFinalDataFailed(NETSIM_ID dev,NETSIM_ID ifid,NETSIM_ID recv){
	Ptr_MinstrelPerRemoteStation station = getMinstrelInfo(dev,ifid,recv);
	if(!station->isInitialized)
		return;

	UpdatePacketCounter(station);
	UpdateRetry(station);
	UpdateStats(station);

	if(station->nRate >= 1)
		station->trRate=FindRate(station);
}

//BOOL DoNeedDataRetransmission(Ptr_MinstrelPerRemoteStation station){
BOOL DoNeedDataRetransmission(NETSIM_ID dev,NETSIM_ID ifid,NETSIM_ID recv){
	Ptr_MinstrelPerRemoteStation station = getMinstrelInfo(dev,ifid,recv);
	//check if we can do retransmission
	if(!station->isInitialized)
		return false;
	if(station->longRetry > CountRetry(station))
		return false;
	else
		return true;
}

static UINT CountRetry(Ptr_MinstrelPerRemoteStation station){
	//if not sampling
	if(!station->isSampling){
		return station->minstrelTable[station->maxTpRate].adjustedRetryCount +
			station->minstrelTable[station->maxTp2Rate].adjustedRetryCount +
			station->minstrelTable[station->maxProbRate].adjustedRetryCount + 
			station->minstrelTable[station->baseRate].adjustedRetryCount;
	}
	else{
		return station->minstrelTable[station->maxTpRate].adjustedRetryCount +
			station->minstrelTable[station->sampleRate].adjustedRetryCount +
			station->minstrelTable[station->maxProbRate].adjustedRetryCount +
			station->minstrelTable[station->baseRate].adjustedRetryCount;
	}
}

static void DoReportRtsFailed(Ptr_MinstrelPerRemoteStation station){
	station->shortRetry++;
}

static void DoReportFinalRtsFailed(Ptr_MinstrelPerRemoteStation station){
	UpdateRetry(station);
}
void Minstrel_Free(NETSIM_ID nDevId, NETSIM_ID nifid)
{
	NETSIM_ID i;
	Ptr_MinstrelWifiStation wifistation= IEEE802_11_PHY(nDevId,nifid)->rateAdaptationData;
	for(i=0;i<=NETWORK->nDeviceCount;i++){
		if(wifistation->minstrelInfo[i])
		{
			FreeTables(wifistation->minstrelInfo[i]);
			free(wifistation->minstrelInfo[i]);
		}
	}
	free(wifistation->minstrelInfo);
	free(wifistation);
	IEEE802_11_PHY(nDevId,nifid)->rateAdaptationData = NULL;
}

void FreeMinstrel(NETSIM_ID nDevId, NETSIM_ID nifid)
{
	PIEEE802_11_PHY_VAR phy = IEEE802_11_PHY(nDevId, nifid);
	switch(phy->PhyProtocol)
	{
	case IEEE_802_11a:
	case IEEE_802_11b:
	case IEEE_802_11g:
	case IEEE_802_11p:
		Minstrel_Free(nDevId,nifid);
		break;
	case IEEE_802_11ac:
	case IEEE_802_11n:
		HT_Minstrel_Free(nDevId,nifid);
		break;
	default:
		fnNetSimError("Unknown phy protocol %d for device %d interface %d in %s\n", phy->PhyProtocol,
					  nDevId, nifid, __FUNCTION__);
	}
}

void FreeTables(Ptr_MinstrelPerRemoteStation station){
	UINT i;
	for(i=0;i<station->nRate;i++){
		free(station->sampleTable[i]);
	}
	free(station->sampleTable);
	free(station->minstrelTable);
}

UINT get_minstrel_rate_index(NETSIM_ID dev,NETSIM_ID ifid,NETSIM_ID recv){
	Ptr_MinstrelPerRemoteStation station = getMinstrelInfo(dev,ifid,recv);
	if(!station || !station->isInitialized)
		return 0;
	return station->trRate;
}

void print_minstrel_table(PMETRICSWRITER metricsWriter)
{
	NETSIM_ID i, j;
	UINT k;
	Ptr_MinstrelPerRemoteStation station;
	if(!show)
		return;
	
	PMETRICSNODE menu = init_metrics_node(MetricsNode_Menu, "Minstrel Data", NULL);
	PMETRICSNODE table = init_metrics_node(MetricsNode_Table, "Minstrel table", NULL);
	add_node_to_menu(menu, table);

	add_table_heading_special(table,"SourceId#1,DestinationId#1,Rate#1,RetryCount#0,Throughput#1,EwmaProb#0,CurrentSuccess#0,CurrentAttempt#0,TotalSuccess#0,TotalAttempt#0,StationSample#0,StationTotal#0,StationRetry#0,");
	for(i=1;i<=NETWORK->nDeviceCount;i++)
	{
		if(isIEEE802_11_Configure(i,1))
		{
			if(IEEE802_11_MAC(i,1)->rate_adaptationAlgo!=MINSTREL)
				continue;
			for(j=1;j<=NETWORK->nDeviceCount;j++)
			{
				station= getMinstrelInfo(i,1,j);		//ifid is believed to be 1..Change if necessary
				if(!station)
					continue;
				if(station && station->totalPacketCount > 0){		//print only if there is transmission
					
					for(k=0;k<station->nRate;k++){
						add_table_row_formatted(false,table, "%d,%d,%lf,",i,j,station->minstrelTable[k].rate);
						add_table_row_formatted(true, table, "%u,%lf,%lf,",station->minstrelTable[k].retryCount,station->minstrelTable[k].throughput,station->minstrelTable[k].ewmaProb);
						add_table_row_formatted(true, table, "%u,%u,%llu,",station->minstrelTable[k].currNumSuccess,station->minstrelTable[k].currNumAttempt,station->minstrelTable[k].totalNumSuccess);
						add_table_row_formatted(true, table, "%llu,",station->minstrelTable[k].totalNumAttempt);
						if(k==0)
							add_table_row_formatted(true, table, "%llu,%llu,%llu,",station->samplePacketCount, station->totalPacketCount,station->totalRetry);
						else
							add_table_row_formatted(true, table, ",,,");

					}
				}
			}
		}
	}
	write_metrics_node(metricsWriter, WriterPosition_Current, NULL, menu);

}
