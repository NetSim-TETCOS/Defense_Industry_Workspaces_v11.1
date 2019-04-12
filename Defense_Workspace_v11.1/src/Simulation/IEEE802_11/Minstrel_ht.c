/************************************************************************************
* Copyright (C) 2016                                                               *
* TETCOS, Bangalore. India                                                         *
*                                                                                  *
* The code Minstrel Rate Control algorithm is provided per GNU GPLv2 licensing     *
*                                                                                  *
* Author:   Dhruvang									                           *
*                                                                                  *
* ---------------------------------------------------------------------------------*/
#include "Minstrel_ht.h"

#define HT_GROUP_RATES 8		//number of rates per group in 802.11n
#define VHT_GROUP_RATES 10		//number of rates per group in 802.11ac
#define HT_BANDWIDTHS	2		//number of bandwidths 20Mhz & 40Mhz
#define VHT_BANDWIDTHS	4		//number of vht bandwidths 20,40,80,160 MHz
#define HT_NSS	4				//number of special streams
#define VHT_NSS	8				//number of special streams in VHT
#define HT_GROUPS 16			//number of groups (bandwidth,sgi,nss)
#define VHT_GROUPS 64			//number of groups (bandwidth,sgi,nss)

#define GetRateId(index,isVht) (isVht ? (index%VHT_GROUP_RATES):(index%HT_GROUP_RATES))
#define GetGroupId(index,isVht) (isVht ? (index/VHT_GROUP_RATES):(index/HT_GROUP_RATES))
#define GetIndex(grpId,rateId,isVht) (isVht ? (grpId*VHT_GROUP_RATES + rateId):(grpId*HT_GROUP_RATES + rateId))
#define IsEqual(b1,g1,s1,b2,g2,s2) ((b1 == b2 && g1 == g2 && s1 ==s2) ? true:false)

#define SEED_r &NETWORK->ppstruDeviceList[0]->ulSeed[0],&NETWORK->ppstruDeviceList[0]->ulSeed[1]
#define rand01() (fn_NetSim_Utilities_GenerateRandomNo(SEED_r)/NETSIM_RAND_MAX)
#define tSlot(phy) (phy->plmeCharacteristics.aSlotTime)
#define ackTime(phy) (phy->plmeCharacteristics.aSIFSTime + get_preamble_time(phy) + (getAckSize(phy) * 8)/phy->dControlFrameDataRate)
#define firstTransmissionTime(phy,byte,rate) (phy->plmeCharacteristics.aSIFSTime + get_preamble_time(phy)+ byte*8/rate)
#define transmissionTime(phy,byte,rate) (byte*8/rate)

static Ptr_MinstrelHtPerRemoteStation getMinstrelInfo(NETSIM_ID dev,NETSIM_ID ifid,NETSIM_ID recv)
{
	Ptr_MinstrelHtWifiStation station = IEEE802_11_PHY(dev,ifid)->rateAdaptationData;
	return station->minstrelHtInfo[recv];
}


void Ht_InitMinstrel(NETSIM_ID nDevId,NETSIM_ID nifid){
	NETSIM_ID i;
	Ptr_MinstrelHtWifiStation htWifiStation;
	//initialize the Minstrel global variables
	ht_updateStatsTime = 100*1000;	//100 ms 
	ht_lookAroundRate= 10;
	ht_sampleCol = 10;
	ht_ewmaWeight = 25;

	htWifiStation = (Ptr_MinstrelHtWifiStation)calloc(1,sizeof(MinstrelHtWifiStation));
	htWifiStation->minstrelHtInfo = (Ptr_MinstrelHtPerRemoteStation*)calloc(NETWORK->nDeviceCount+1,sizeof(Ptr_MinstrelHtPerRemoteStation));		
	htWifiStation->devId = nDevId;
	htWifiStation->interfaceId = nifid;
	for(i=0;i<=NETWORK->nDeviceCount;i++){
		htWifiStation->minstrelHtInfo[i] = (Ptr_MinstrelHtPerRemoteStation)calloc(1,sizeof(MinstrelHtPerRemoteStation));
		htWifiStation->minstrelHtInfo[i]->remoteStationId = i;
		htWifiStation->minstrelHtInfo[i]->stationId = nDevId;
		htWifiStation->minstrelHtInfo[i]->stationInterface = nifid;
		CheckInit(htWifiStation->minstrelHtInfo[i],nDevId,nifid);
	}
	IEEE802_11_PHY(nDevId,nifid)->rateAdaptationData = htWifiStation;
}

static void CheckInit(Ptr_MinstrelHtPerRemoteStation station,NETSIM_ID devid,NETSIM_ID ifid){	//fill
	if(!station->isInitialized){
		station->nextStatsUpdate = ldEventTime + ht_updateStatsTime;
		station->ampduLen = 0;
		station->ampduPacketCount = 0;
		station->avgAmpduLen = 1;
		
		station->isInitialized= true;
		station->isSampling= false;
		station->isVht= (IEEE802_11_PHY(devid,ifid)->PhyType==VHT);
		station->longRetry= 0;
		station->shortRetry= 0;
		station->totalRetry= 0;

		station->maxTpRate = 0;
		station->maxTpRate2= 0;
		station->maxProbRate= 0;
		if(station->isVht){
			station->nGroup= VHT_GROUPS;
			station->nGroupRate= VHT_GROUP_RATES;
		}
		else{
			station->nGroup = HT_GROUPS;
			station->nGroupRate = HT_GROUP_RATES;
		}
		
		station->phy = IEEE802_11_PHY(devid,ifid);
		station->sampleCount = 16;
		station->sampleGroup= 0;
		station->sampleSlow= 0;
		station->sampleTries= 4;
		station->sampleWait= 0;

		station->samplePacketCount= 0;
		station->totalPacketCount= 0;

		station->trRate= 0;
		station->sampleRate = 0;

		InitGroupTable(station);
		InitSampleTable(station);
	}
}

static void InitSampleTable(Ptr_MinstrelHtPerRemoteStation station){
	UINT i,j;
	//Memory allocation for Sample Table
	station->sampleTable = calloc(station->nGroupRate, sizeof(UINT*));
	for(i=0;i<station->nGroupRate;i++){
		station->sampleTable[i] = calloc(ht_sampleCol,sizeof(UINT));
		for(j=0;j<ht_sampleCol;j++)
			station->sampleTable[i][j] = 10000;
		//memset(station->sampleTable[i],10000,sampleCol*sizeof(UINT));
	}

	//Randomly filling table with values [0,nRate-1]
	for(j=0;j<ht_sampleCol;j++){
		for(i=0;i<station->nGroupRate;i++){
			//generate the random number
			int random_var = (int)station->nGroupRate*(int)rand01();
			random_var = (i+random_var)%station->nGroupRate;

			while(station->sampleTable[random_var][j] != 10000){
				random_var = (random_var + 1)%station->nGroupRate;
			}
			station->sampleTable[random_var][j] = i;
		}
	}
}

static void InitGroupTable(Ptr_MinstrelHtPerRemoteStation station){
	//sort order of groups is: BW -> SGI -> #streams
	UINT i,j,ch,sgi,nss,maxCh,maxNss,startid;
	struct stru_802_11_Phy_Parameters_HT* PhyArray;
	station->groupTable = calloc(station->nGroup,sizeof(GroupInfo));
	maxCh=40;
	maxNss=4;
	if(station->isVht){
		maxCh= 160;
		maxNss= 8;
	}

	i=0;
	for(ch=20;ch<=maxCh;ch*=2){		//bandwidth
		for(sgi=400;sgi<=800;sgi*=2){		//sgi

			if(sgi == 400)		//NetSim requirement see IEEE802_11_HTPhy.c
				startid = 10;
			else
				startid = 0;

			for(nss=1;nss<=maxNss;nss++){		//#streams
				PhyArray= get_phy_parameter_HT((double)ch,nss);
				//check if group is supported			//fill

				if(IsEqual(ch,sgi,nss,station->phy->dChannelBandwidth,station->phy->nGuardInterval,station->phy->NSS))
					station->groupTable[i].supported = true;
				else
					station->groupTable[i].supported = false;
				
				if(station->groupTable[i].supported){
					station->groupTable[i].chWidth= ch;
					station->groupTable[i].col= 0;
					station->groupTable[i].maxProbRate=0;
					station->groupTable[i].maxTpRate= 0;
					station->groupTable[i].maxTpRate2= 0;
					station->groupTable[i].row= 0;
					station->groupTable[i].sgi = sgi;
					station->groupTable[i].streams = nss;

					station->groupTable[i].rateTable = calloc(station->nGroupRate,sizeof(HtRateInfo));

					for(j=0;j<station->nGroupRate;j++){
						station->groupTable[i].rateTable[j].currNumAttempt = 0;
						station->groupTable[i].rateTable[j].currNumSuccess = 0;
						station->groupTable[i].rateTable[j].ewmaProb= 0;
						station->groupTable[i].rateTable[j].ewmsdProb = 0;
						station->groupTable[i].rateTable[j].numSampleSkipped= 0;
						//station->groupTable[i].rateTable[j].perfectTrTime = ;	//fill
						station->groupTable[i].rateTable[j].prevNumAttempt= 0;
						station->groupTable[i].rateTable[j].prevNumSuccess= 0;
						station->groupTable[i].rateTable[j].prob= 0;
						station->groupTable[i].rateTable[j].rate= PhyArray[j+startid].dDataRate;
						station->groupTable[i].rateTable[j].retryCount= 0;
						station->groupTable[i].rateTable[j].retryUpdated= false;
						station->groupTable[i].rateTable[j].throughput= 0;
						station->groupTable[i].rateTable[j].totalNumAttempt= 0;
						station->groupTable[i].rateTable[j].totalNumSuccess= 0;

						if(station->groupTable[i].rateTable[j].rate <= 0)
							station->groupTable[i].rateTable[j].supported= false;//fill
						else
							station->groupTable[i].rateTable[j].supported= true;
					}

				}
				i++;
			}
		}
	}
	station->maxTpRate= GetHighestIndex(station);
	station->maxTpRate2 = GetLowestIndex(station);
	station->maxProbRate = GetLowestIndex(station);
	station->trRate = station->maxTpRate;

}

void DoReportAmpduStatus(NETSIM_ID devid,NETSIM_ID ifid,NETSIM_ID recvid,UINT success,UINT failed){
	UINT groupId, rateId;
	Ptr_MinstrelHtPerRemoteStation station = getMinstrelInfo(devid,ifid,recvid);
	if(!station->isInitialized)
		return;

	//update ampdu variables
	station->ampduPacketCount++;
	station->ampduLen += success + failed;

	//update station packet counters
	UpdatePacketCounter(station,success,failed);		//fill
	groupId = GetGroupId(station->trRate,station->isVht);		//fill
	rateId = GetRateId(station->trRate,station->isVht);		//fill

	//update rate counters
	station->groupTable[groupId].rateTable[rateId].currNumAttempt += success + failed;
	station->groupTable[groupId].rateTable[rateId].currNumSuccess += success;

	if(success== 0 && station->longRetry < CountRetries(station)){
		//Not a single success..Entire A-MPDU failed
		UpdateRate(station);			//fill
	}
	else{
		station->isSampling = false;
		UpdateRetry(station);			//fill
		
		if(ldEventTime >= station->nextStatsUpdate)
			UpdateStats(station);			//fill

		FindRate(station);				//fill
	}
}

static void UpdatePacketCounter(Ptr_MinstrelHtPerRemoteStation station,UINT success,UINT failed){
	station->totalPacketCount += success + failed;
	//if sampling update samplecount
	if(station->isSampling){
		station->samplePacketCount += success + failed;
	}

	//wraparound
	if(station->totalPacketCount < 0){
		station->samplePacketCount = 0;
		station->totalPacketCount = 0;
	}

	if(station->sampleWait == 0 && station->sampleTries == 0 && station->sampleCount > 0){
		station->sampleWait = (UINT)(16 + 2*station->avgAmpduLen);
		station->sampleTries = 1;
		station->sampleCount--;
	}
}

static void UpdateRetry(Ptr_MinstrelHtPerRemoteStation station){
	station->totalRetry += station->longRetry + station->shortRetry;
	station->shortRetry = 0;
	station->longRetry = 0;
}

static void UpdateStats(Ptr_MinstrelHtPerRemoteStation station){
	//Call this function after the scheduled update
	double tempProb;
	UINT i,j;
	station->nextStatsUpdate = ldEventTime + ht_updateStatsTime;
	station->sampleCount = 0;
	station->sampleSlow = 0;

	if(station->ampduPacketCount > 0){
		double newLen = station->ampduLen / station->ampduPacketCount ;
		//exponentially weighted average MPDUs per A-MPDU
		station->avgAmpduLen = (newLen* ht_ewmaWeight + station->avgAmpduLen*(100- ht_ewmaWeight))/100;
		station->ampduLen = 0;
		station->ampduPacketCount = 0;
	}

	//update stats for each group and each rate
	for(i=0;i<station->nGroup;i++){
		if (!station->groupTable[i].supported)
			continue;
		for(j=0;j<station->nGroupRate;j++){
			//check if rate is supported
			if(station->groupTable[i].rateTable[j].supported){
				//try to sample all available rates during each interval
				station->sampleCount++;
				station->groupTable[i].rateTable[j].retryUpdated = false;

				//if we have attempted something
				if(station->groupTable[i].rateTable[j].currNumAttempt > 0){
					station->groupTable[i].rateTable[j].numSampleSkipped = 0;
					tempProb = (18000* station->groupTable[i].rateTable[j].currNumSuccess)/station->groupTable[i].rateTable[j].currNumAttempt;
					tempProb = tempProb/18000;

					station->groupTable[i].rateTable[j].prob= tempProb;

					//if updating the rate for the first time
					if(station->groupTable[i].rateTable[j].totalNumAttempt == 0){
						station->groupTable[i].rateTable[j].ewmaProb = tempProb;
					}
					//take weighted average
					else{
						station->groupTable[i].rateTable[j].ewmsdProb = CalculateEwmsd(station->groupTable[i].rateTable[j].ewmsdProb,
																		tempProb,station->groupTable[i].rateTable[j].ewmaProb,ht_ewmaWeight);	//fill

						//EWMA probability
						tempProb = (tempProb * ht_ewmaWeight + (100 - ht_ewmaWeight)*station->groupTable[i].rateTable[j].ewmaProb)/100;
						station->groupTable[i].rateTable[j].ewmaProb = tempProb;
					}
					
					station->groupTable[i].rateTable[j].throughput = CalculateThroughput(station,i,j); //fill

				}
				//nothing is attempted at this rate
				else{
					station->groupTable[i].rateTable[j].numSampleSkipped++;
				}
			
				station->groupTable[i].rateTable[j].totalNumAttempt += station->groupTable[i].rateTable[j].currNumAttempt;
				station->groupTable[i].rateTable[j].totalNumSuccess += station->groupTable[i].rateTable[j].currNumSuccess;
				station->groupTable[i].rateTable[j].prevNumAttempt = station->groupTable[i].rateTable[j].currNumAttempt;
				station->groupTable[i].rateTable[j].prevNumSuccess = station->groupTable[i].rateTable[j].currNumSuccess;
				station->groupTable[i].rateTable[j].currNumAttempt = 0;
				station->groupTable[i].rateTable[j].currNumAttempt = 0;
			}
		}
	}
	//update the group and station max rates
	SetStationThRate(station);			//fill
	SetStationProbRate(station);		//fill

	//Calculate Retransmits..changed based on AMpdu values
	CalculateRetransmits(station,station->maxTpRate);		//fill
	CalculateRetransmits(station,station->maxTpRate2);
	CalculateRetransmits(station,station->maxProbRate);
}

static void UpdateRate(Ptr_MinstrelHtPerRemoteStation station){
	/**
   * Retry Chain table is implemented here.
   * Note this Retry Chain is different from Legacy Minstrel.
   *
   * Try |     LOOKAROUND RATE     | NORMAL RATE
   * -------------------------------------------------------
   *  1  |  Random rate            | Best throughput
   *  2  |  Next best throughput   | Next best throughput
   *  3  |  Best probability       | Best probability
   *
   * Note: For clarity, multiple blocks of if's and else's are used
   * Following implementation in Linux, in MinstrelHT Lowest baserate is not used.
   * Explanation can be found here: http://marc.info/?l=linux-wireless&m=144602778611966&w=2
   */
	UINT maxTpRateId, maxTpGrpId, maxTp2RateId, maxTp2GrpId, maxProbRateId, maxProbGrpId;
	if(!station->isInitialized)
		return;

	station->longRetry++;

	maxTpRateId = GetRateId(station->maxTpRate ,station->isVht);
	maxTpGrpId = GetGroupId(station->maxTpRate, station->isVht);
	maxTp2RateId = GetRateId(station->maxTpRate2, station->isVht);
	maxTp2GrpId = GetGroupId(station->maxTpRate2, station->isVht);
	maxProbRateId = GetRateId(station->maxProbRate, station->isVht);
	maxProbGrpId = GetRateId(station->maxProbRate, station->isVht);


	//for normal rates
	if (!station->isSampling) {
		//use best rate
		if (station->longRetry < station->groupTable[maxTpGrpId].rateTable[maxTpRateId].retryCount)
			station->trRate = station->maxTpRate;

		// Use second best throughput rate.
		else if (station->longRetry < (station->groupTable[maxTpGrpId].rateTable[maxTpRateId].retryCount +
									   station->groupTable[maxTp2GrpId].rateTable[maxTp2RateId].retryCount)) {
			station->trRate = station->maxTpRate2;
		}
		//Use best probability rate
		else if (station->longRetry <= (station->groupTable[maxTpGrpId].rateTable[maxTpRateId].retryCount +
										station->groupTable[maxTp2GrpId].rateTable[maxTp2RateId].retryCount +
										station->groupTable[maxProbGrpId].rateTable[maxProbRateId].retryCount)) {
			station->trRate = station->maxProbRate;
		}
		else {
			fnNetSimError("Invalid Retry Condition..!");
		}

	}
	//for lookaround rates
	else{
		//sample rate is used only once
		//use 2nd best rate **FIXME..!**
		if(station->longRetry < 1 + station->groupTable[maxTp2GrpId].rateTable[maxTp2RateId].retryCount){
			station->trRate = station->maxTpRate2;
		}
		//use the best proability rate
		else if(station->longRetry >= 1 + station->groupTable[maxTp2GrpId].rateTable[maxTp2RateId].retryCount){
			station->trRate = station->maxProbRate;
		}
		else{
			fnNetSimError("Invalid Retry Condition..!");
		}
	}
}

static UINT FindRate(Ptr_MinstrelHtPerRemoteStation station){
	if(station->totalPacketCount == 0){
		return station->maxTpRate;
	}
	//if we have waited enough then sample
	if(station->sampleWait == 0 && station->sampleTries != 0){
		//sampling
		UINT sampleIdx = GetNextSample(station);		//fill
		//evaluate if the sample rate should be used
		UINT sampleGroupId = GetGroupId (sampleIdx,station->isVht);
		UINT sampleRateId = GetRateId (sampleIdx,station->isVht);

		//use sample rate only if it is supported
		if(station->groupTable[sampleGroupId].supported && station->groupTable[sampleGroupId].rateTable[sampleRateId].supported){
			/*dont use sampling for the currently used rates
			* also dont use sampling if probability is already higher than 95%
			* to avoid wasting airtime */
			if(sampleIdx != station->maxTpRate && sampleIdx != station->maxTpRate2
				&& sampleIdx != station->maxProbRate && station->groupTable[sampleGroupId].rateTable[sampleRateId].ewmaProb <= 95){
				
					UINT maxTp2GrpId = GetGroupId(station->maxTpRate2,station->isVht);
					UINT maxTp2RateId = GetRateId(station->maxTpRate2,station->isVht);
					UINT maxProbRateId = GetRateId(station->maxProbRate,station->isVht);
					UINT maxProbGrpId = GetGroupId(station->maxProbRate, station->isVht);

					UINT sampleRate = (UINT)station->groupTable[sampleGroupId].rateTable[sampleRateId].rate;
					UINT maxTp2Rate = (UINT)station->groupTable[maxTp2GrpId].rateTable[maxTp2RateId].rate;
					UINT maxProbRate = (UINT)station->groupTable[maxProbGrpId].rateTable[maxProbRateId].rate;

					UINT sampleStream = station->groupTable[sampleGroupId].streams;
					UINT maxTpStream = station->groupTable[maxTp2GrpId].streams;
						
						/* Make sure that lower rates get sampled only occasionally,
						* if the link is working perfectly */
					if(sampleRate > maxTp2Rate || (sampleStream <= maxTpStream -1 && sampleRate > maxProbRate)){
						//set flag for currently sampling
						station->isSampling = true;
						//set the sample rate
						station->sampleRate = sampleIdx;
						station->sampleTries--;
						return sampleIdx;
					}
					else{
						if(station->groupTable[sampleGroupId].rateTable[sampleRateId].numSampleSkipped >= 20 && station->sampleSlow++ <=2){
							//set flag for sampling
							station->isSampling = true;
							//set the sample rate
							station->sampleRate = sampleIdx;
							station->sampleTries--;
							return sampleIdx;
						}
					}
			}
		}
	}
	if(station->sampleWait > 0)
		station->sampleWait--;

	//Continue using best rate
	return station->maxTpRate;
}

static void SetStationThRate(Ptr_MinstrelHtPerRemoteStation station){
	//set the max and 2nd max throughput rates of the groups and the station
	UINT i,j;
	BOOL isVht = station->isVht;
	UINT grpmaxThId,grpmaxTh2Id;
	double grpmaxTh,grpmaxThProb,grpmaxTh2,grpmaxTh2Prob,th,prob;

	UINT maxThId = GetLowestIndex(station);
	double maxTh = station->groupTable[GetGroupId(maxThId,isVht)].rateTable[GetRateId(maxThId,isVht)].throughput;
	double maxThProb = station->groupTable[GetGroupId(maxThId,isVht)].rateTable[GetRateId(maxThId,isVht)].ewmaProb;


	UINT maxTh2Id = GetLowestIndex(station);
	double maxTh2 = station->groupTable[GetGroupId(maxTh2Id,isVht)].rateTable[GetRateId(maxTh2Id,isVht)].throughput;	
	double maxTh2Prob = station->groupTable[GetGroupId(maxTh2Id,isVht)].rateTable[GetRateId(maxTh2Id,isVht)].ewmaProb;
	for(i=0;i<station->nGroup;i++){			//for each group
		if(!station->groupTable[i].supported)
			continue;
		
		grpmaxTh = 0;
		grpmaxThId = 0;		//will be rate id
		grpmaxThProb = 0;
		
		grpmaxTh2 = 0;
		grpmaxTh2Id = 0;
		grpmaxTh2Prob = 0;
		for(j=0;j<station->nGroupRate;j++){
			if(!station->groupTable[i].rateTable[j].supported)
				continue;
			
			th = station->groupTable[i].rateTable[j].throughput;
			prob = station->groupTable[i].rateTable[j].ewmaProb;
			//rate is better than current grp best
			if(th > grpmaxTh || (th == grpmaxTh && prob > grpmaxThProb)){
				grpmaxTh2 = grpmaxTh;
				grpmaxTh2Prob = grpmaxThProb;
				grpmaxTh2Id = grpmaxThId;
				
				grpmaxTh = th;
				grpmaxThProb = prob;
				grpmaxThId = j;
			}
			//rate is better than current grp 2nd best
			else if(th > grpmaxTh2 || (th == grpmaxTh2 && prob > grpmaxTh2Prob)){
				grpmaxTh2 = th;
				grpmaxTh2Prob = prob;
				grpmaxTh2Id = j;
			}
			//do nothing
			else{
			}
		}

		//compare grp values with station values
		if(grpmaxTh > maxTh || (grpmaxTh == maxTh && grpmaxThProb > maxThProb)){
			maxTh2 = maxTh;
			maxTh2Id = maxThId;
			maxTh2Prob = maxThProb;

			maxTh = grpmaxTh;
			maxThId = GetIndex(i,grpmaxThId,station->isVht);
			maxThProb = grpmaxThProb;
		}
		else if(grpmaxTh > maxTh2 || (grpmaxTh == maxTh2 && grpmaxThProb > maxTh2Prob)){
			maxTh2 = grpmaxTh;
			maxTh2Prob = grpmaxThProb;
			maxTh2Id = grpmaxThId;
		}

		if(grpmaxTh2 > maxTh2 || (grpmaxTh2 == maxTh2 && grpmaxTh2Prob > maxTh2Prob)){
			maxTh2 = grpmaxTh2;
			maxTh2Prob = grpmaxTh2Prob;
			maxTh2Id = grpmaxTh2Id;
		}
		else{
		}
	}

	station->maxTpRate = maxThId;
	station->maxTpRate2 = maxTh2Id;
}

static void SetStationProbRate(Ptr_MinstrelHtPerRemoteStation station){
	UINT i,j,grpmaxProbId;
	BOOL isVht = station->isVht;
	double th,prob,grpmaxProb,grpmaxProbTh;

	UINT maxProbId= GetLowestIndex(station);
	double maxProb= station->groupTable[GetGroupId(maxProbId,isVht)].rateTable[GetRateId(maxProbId,isVht)].ewmaProb;
	double maxProbTh= station->groupTable[GetGroupId(maxProbId,isVht)].rateTable[GetRateId(maxProbId,isVht)].throughput;
	for(i=0;i<station->nGroup;i++){
		if(!station->groupTable[i].supported)
			continue;
		
		grpmaxProb=0;
		grpmaxProbId=0;	//will be rate id
		grpmaxProbTh=0;	
		for(j=0;j<station->nGroupRate;j++){
			if(!station->groupTable[i].rateTable[j].supported)
				continue;

			th = station->groupTable[i].rateTable[j].throughput;
			prob = station->groupTable[i].rateTable[j].ewmaProb;
			if(prob > 0.75){
				if(th > grpmaxProbTh){
					grpmaxProbTh = th;
					grpmaxProb = prob;
					grpmaxProbId = j;
				}
			}
			else{
				if(prob > grpmaxProb){
					grpmaxProbTh =th;
					grpmaxProb = prob;
					grpmaxProbId = j;
				}
			}
		}

		//update station values based on group values if necessary
		if(grpmaxProb > 0.75){
			if(grpmaxProbTh > maxProbTh){
				maxProbTh = grpmaxProbTh;
				maxProb = grpmaxProb;
				maxProbId = GetIndex(i,grpmaxProbId,station->isVht);
			}
		}
		else{
			if(grpmaxProb > maxProb){
				maxProbTh = grpmaxProbTh;
				maxProb = grpmaxProb;
				maxProbId = GetIndex(i,grpmaxProbId,station->isVht);				
			}
		}
	}
	
}

static UINT GetNextSample(Ptr_MinstrelHtPerRemoteStation station){
	UINT sampleGroup = station->sampleGroup;
	UINT row = station->groupTable[sampleGroup].row;
	UINT col = station->groupTable[sampleGroup].col;
	UINT sampleIndex = station->sampleTable[row][col];		//rateId
	UINT rateIndex = GetIndex(sampleGroup, sampleIndex,station->isVht);	//fill
	SetNextSample(station);		//set the next sample rate

	return rateIndex;
}

static void SetNextSample(Ptr_MinstrelHtPerRemoteStation station){
	//search for the next supported group
	do{
		station->sampleGroup = (++station->sampleGroup)%station->nGroup;
	}while(!station->groupTable[station->sampleGroup].supported);

	station->groupTable[station->sampleGroup].row++;
	if(station->groupTable[station->sampleGroup].row >= station->nGroupRate){
		station->groupTable[station->sampleGroup].row = 0;
		if(++station->groupTable[station->sampleGroup].col >= ht_sampleCol)
			station->groupTable[station->sampleGroup].col = 0;
	}
}

static double CalculateEwmsd(double oldEwmsd, double currentProb, double ewmaProb, double ewmaWeight){
  double diff, incr, tmp;

  /* calculate exponential weighted moving variance */
  diff = currentProb - ewmaProb;
  incr = (ewmaWeight) * diff / 100;
  tmp = oldEwmsd * oldEwmsd;
  tmp = (100- ewmaWeight)* (tmp + diff * incr) / 100;

  /* return standard deviation */
  return sqrt (tmp);
}

static double CalculateThroughput(Ptr_MinstrelHtPerRemoteStation station,UINT grpId, UINT rateId){
	if(station->groupTable[grpId].rateTable[rateId].ewmaProb < 0.1)
		return 0;
	else{
		double ewmaProb = station->groupTable[grpId].rateTable[rateId].ewmaProb;
		double rate = station->groupTable[grpId].rateTable[rateId].rate;
		double nPackets = station->avgAmpduLen;
		double TrTime = firstTransmissionTime(station->phy,1500,rate) + transmissionTime(station->phy,1500,rate)*(nPackets - 1);
		return (max(ewmaProb,0.9)*8*1500*nPackets)/TrTime;			//check and fill
	}
}

static void CalculateRetransmits(Ptr_MinstrelHtPerRemoteStation station,UINT index){
	UINT grpId = GetGroupId(index,station->isVht);
	UINT rateId = GetRateId(index,station->isVht);
	UINT cw = 15;
	UINT cwMax = 1023;
	double cwTime,txTime,dataTxTime,rate;


	if(station->groupTable[grpId].rateTable[rateId].retryUpdated)
		return;

	if(station->groupTable[grpId].rateTable[rateId].ewmaProb < 0.01)
		station->groupTable[grpId].rateTable[rateId].retryCount =1;
	else{
		station->groupTable[grpId].rateTable[rateId].retryCount =2; 
		station->groupTable[grpId].rateTable[rateId].retryUpdated = true;
		
		rate = station->groupTable[grpId].rateTable[rateId].rate;
		//data transmission time
		dataTxTime = firstTransmissionTime(station->phy,1500,rate) + transmissionTime(station->phy,1500,rate) * (station->avgAmpduLen - 1);

		//contention time for first two transmission
		cwTime = (tSlot(station->phy)*cw)>>1;
		cw = min((cw<<1)| 1,cwMax);
		cwTime += (tSlot(station->phy)*cw)>>1;
		cw = min((cw<<1)| 1,cwMax);

		txTime = cwTime + 2*(dataTxTime + ackTime(station->phy));

		do{
			cwTime = (tSlot(station->phy)*cw)>>1;
			cw = min((cw<<1)| 1,cwMax);
			txTime += dataTxTime + ackTime(station->phy) + cwTime;
		}while(txTime < 6000 && (++station->groupTable[grpId].rateTable[rateId].retryCount < 7));
	
	}
}

BOOL Ht_DoNeedDataRetransmission(NETSIM_ID devid,NETSIM_ID ifid,NETSIM_ID recvid){
	Ptr_MinstrelHtPerRemoteStation station = getMinstrelInfo(devid,ifid,recvid);
	return (station->longRetry < CountRetries(station));
}

static UINT CountRetries(Ptr_MinstrelHtPerRemoteStation station){	//fill
	//is station is sampling
	BOOL isVht = station->isVht;
	UINT th = station->maxTpRate;
	UINT th2 = station->maxTpRate2;
	UINT prob = station->maxProbRate;
	if(!station->isSampling){
		return station->groupTable[GetGroupId(th,isVht)].rateTable[GetRateId(th,isVht)].retryCount +
			   station->groupTable[GetGroupId(th2,isVht)].rateTable[GetRateId(th2,isVht)].retryCount +
			   station->groupTable[GetGroupId(prob,isVht)].rateTable[GetRateId(prob,isVht)].retryCount;
	}
	else{
		return 1 + station->groupTable[GetGroupId(th2,isVht)].rateTable[GetRateId(th2,isVht)].retryCount +
				station->groupTable[GetGroupId(prob,isVht)].rateTable[GetRateId(prob,isVht)].retryCount;
	}
}

static UINT GetLowestIndex(Ptr_MinstrelHtPerRemoteStation station){
	UINT grpId = 0;
	UINT rateId = 0;

	while(!(int)grpId < (int)station->nGroup &&
		!station->groupTable[grpId].supported)
		grpId++;

	if(grpId == station->nGroup)
		fnNetSimError("No groups supported..!");

	while(rateId < station->nGroupRate && !station->groupTable[grpId].rateTable[rateId].supported)
		rateId++;

	if(rateId == station->nGroupRate)
		fnNetSimError("No rates supported...!");

	return GetIndex(grpId,rateId,station->isVht);
}

static UINT GetHighestIndex(Ptr_MinstrelHtPerRemoteStation station){
	int grpId = station->nGroup -1;
	int rateId = station->nGroupRate -1;
	while(grpId >= 0 && !station->groupTable[grpId].supported)
		grpId--;

	if(grpId == -1)
		fnNetSimError("No groups supported..!");

	while(rateId >= 0 && !station->groupTable[grpId].rateTable[rateId].supported)
		rateId--;

	if(rateId == -1)
		fnNetSimError("No rates supported..!");

	return GetIndex(grpId,rateId,station->isVht);
}

struct stru_802_11_Phy_Parameters_HT getTxRate(NETSIM_ID devid,NETSIM_ID ifid,NETSIM_ID recvid)
{
	Ptr_MinstrelHtPerRemoteStation station = getMinstrelInfo(devid,ifid,recvid);
	UINT grpId = GetGroupId(station->trRate,station->isVht);
	UINT rateId = GetRateId(station->trRate,station->isVht);
	UINT W = station->groupTable[grpId].chWidth;
	UINT S = station->groupTable[grpId].streams;
	UINT r;
	if(station->groupTable[grpId].sgi == 800){
		r = rateId;
	}
	else{
		r = 10 + rateId;
	}
	return get_phy_parameter_HT(W,S)[r];

}

void HT_Minstrel_Free(NETSIM_ID nDevId, NETSIM_ID nifid)
{
#pragma message(__LOC__"Implement the HT_Minstrel_Free")
	IEEE802_11_PHY(nDevId,nifid)->rateAdaptationData = NULL;
}