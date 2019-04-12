/************************************************************************************
* Copyright (C) 2016                                                               *
* TETCOS, Bangalore. India                                                         *
*                                                                                  *
* The code Minstrel Rate Control algorithm is provided per GNU GPLv2 licensing     *
*                                                                                  *
* Author:   Dhruvang									                           *
*                                                                                  *
* ---------------------------------------------------------------------------------*/
#include "main.h"
#include "IEEE802_11_Phy.h"

#ifdef __cplusplus
extern "C" {
#endif

//Structure to contain all information related to a data-rate
typedef struct stru_Rate_Info{
	double rate;					//data-rate
	
   // Perfect transmission time calculation, or frame calculation
   // Given a bit rate and a packet length n bytes
	double perfectTrTime;

	UINT retryCount;			//retry limit (constant for each rate)
	UINT adjustedRetryCount;		//retry limit to be used(changes with time)
	UINT currNumAttempt;		//attempts made so far
	UINT currNumSuccess;		//success pkts so far
	UINT prevNumAttempt;		
	UINT prevNumSuccess;

	double prob;				//(# successful pkts / # attempted pkts)
	//ewma_prob =[prob *(ewma_weight) + ewma_prob_old *(100- ewma_weight)]/100
	double ewmaProb;			//Expected weighted moving average
	double throughput;			//throughput of this rate

	UINT64 totalNumAttempt;		//total number of attempts made
	UINT64 totalNumSuccess;		//total number of success pkts

	UINT numSampleSkipped;		//Number of Samples skipped
	int sampleLimit;			//Sampling Limit
}RateInfo;

//Structure to contain information for each remote station
typedef struct stru_MinstrelWifiRemoteStation{
	double nextStatsUpdate;			//time to update next time
	BOOL isInitialized;				//to check if station is intialized
	UINT maxTpRate;					//index of maximum throughput rate
	UINT maxTp2Rate;				//index of 2nd maximum throughput rate
	UINT maxProbRate;				//index of rate with maximum success probability
	UINT baseRate;					//index of lowest rate
	UINT nRate;						//number of rates supported
	UINT sampleRate;				//Index of current sample Rate
	UINT trRate;					//Index of current transmit Rate

	BOOL isSampling;				//currently sampling
	BOOL isDeferredSample;			//flag to indicate sample rate is on 2nd stage
	UINT64 numSampleDeferred;			//count the deferred number of samples
	UINT64 totalPacketCount;			//number of total packets (all retries will count as 1)
	UINT64 samplePacketCount;			//number of sampled packets
	UINT shortRetry;				//number of short retries such as control packets
	UINT longRetry;					//number of long retries such as data packets
	UINT64 totalRetry;				//total retry= long + short
	
	RateInfo* minstrelTable;			//minstrel Table
	UINT** sampleTable;				//sample Table of nRate x col int values
	UINT row,col;					//current position in sample table
	PIEEE802_11_PHY_VAR phy;		//NetSim implementation requirement
}MinstrelPerRemoteStation,*Ptr_MinstrelPerRemoteStation;

double updateStatsTime;					//Time Interval for updating Minstrel Table ~100ms
double lookAroundRate;				//% of pkts to send with other rates. ~10
double ewmaWeight;					//weight to be given to new value (0,100) ~25%
UINT sampleCol;						//number of columns in Sample Table ~10

//One structure per device
typedef struct stru_MinstrelWifiStation{
	Ptr_MinstrelPerRemoteStation *minstrelInfo;	//(Array)containing info for each remote connected device
}MinstrelWifiStation,*Ptr_MinstrelWifiStation;

void InitMinstrel(NETSIM_ID nDevId,NETSIM_ID nifid);
static void CheckInit(Ptr_MinstrelPerRemoteStation station,NETSIM_ID nDevId,NETSIM_ID nifid);
static void InitSampleTable(Ptr_MinstrelPerRemoteStation station);
static void InitMinstrelTable(Ptr_MinstrelPerRemoteStation station,double *rate_arr);
void DoReportDataOk(NETSIM_ID dev,NETSIM_ID ifid,NETSIM_ID recv);
void DoReportDataFailed(NETSIM_ID dev,NETSIM_ID ifid,NETSIM_ID recv);
static void UpdateRate(Ptr_MinstrelPerRemoteStation station);
static void UpdatePacketCounter(Ptr_MinstrelPerRemoteStation station);
static void UpdateRetry(Ptr_MinstrelPerRemoteStation station);
static void UpdateStats(Ptr_MinstrelPerRemoteStation station);
static UINT FindRate(Ptr_MinstrelPerRemoteStation station);
static UINT GetNextSample(Ptr_MinstrelPerRemoteStation station);
void DoReportFinalDataFailed(NETSIM_ID dev,NETSIM_ID ifid,NETSIM_ID recv);
BOOL DoNeedDataRetransmission(NETSIM_ID dev,NETSIM_ID ifid,NETSIM_ID recv);
static UINT CountRetry(Ptr_MinstrelPerRemoteStation station);
void FreeMinstrel(NETSIM_ID nDevId,NETSIM_ID nifid);
void FreeTables(Ptr_MinstrelPerRemoteStation station);


#ifdef __cplusplus
}
#endif