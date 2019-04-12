/************************************************************************************
* Copyright (C) 2016                                                               *
* TETCOS, Bangalore. India                                                         *
*                                                                                  *
* The code Minstrel Rate Control algorithm is provided per GNU GPLv2 licensing     *
*                                                                                  *
* Author:   Dhruvang									                           *
*                                                                                  *
* ---------------------------------------------------------------------------------*/
//#include "Minstrel.h"
//#include "IEEE802_11_HTPhy.h"
#include "IEEE802_11_Phy.h"

#ifdef __cplusplus
extern "C" {
#endif

static struct stru_802_11_Phy_Parameters_HT
{
	int nIndex;
	int MCS;
	double dRxSensitivity;
	PHY_MODULATION nModulation;
	IEEE802_11_CODING nCodingRate;
	int nNBPSC;
	int nNSD;
	int NSP;
	int nNCBPS;
	int nNDBPS;
	int NES;
	double dGI;
	double dDataRate;
	
	
}a;

struct stru_McsGroup{
	UINT streams;		//number of spatial streams
	UINT sgi;			//short guard Interval
	UINT chWidth;		//bandwidth
	bool isVht;
	bool isSupported;

	double* rate;			//rates supported by this group
	double* trTime;			//transmission time for the rates
}McsGroup;

typedef struct stru_HtRateInfo{
	/**
   * Perfect transmission time calculation, or frame calculation.
   * Given a bit rate and a packet length n bytes.
   */
  double perfectTrTime;
  double rate;

  BOOL supported;               //!< If the rate is supported.

  UINT retryCount;          //!< Retry limit.
  UINT adjustedRetryCount;  //!< Adjust the retry limit for this rate.
  double prob;                  //!< Current probability within last time interval. (# frame success )/(# total frames)

  BOOL retryUpdated;            //!< If number of retries was updated already.

  /**
   * Exponential weighted moving average of probability.
   * EWMA calculation:
   * ewma_prob =[prob *(100 - ewma_level) + (ewma_prob_old * ewma_level)]/100
   */
  double ewmaProb;
  double throughput;            //!< Throughput of this rate (in pkts per second).
  double ewmsdProb;             //!< Exponential weighted moving standard deviation of probability.

  UINT numSampleSkipped;	//number of skipped stats updates
  UINT currNumAttempt;		//attempts made so far
  UINT currNumSuccess;		//success pkts so far
  UINT prevNumAttempt;		//transmission made with previous rate
  UINT prevNumSuccess;		//successful transmission made with previous rate
  UINT64 totalNumAttempt;		//total number of attempts made
  UINT64 totalNumSuccess;		//total number of success pkts

  
}HtRateInfo,*Ptr_HtRateInfo;

typedef struct stru_GroupInfo{
	UINT col;				//sample table row
	UINT row;				//sample table col
	BOOL supported;			//if the group is supported
	UINT streams;			//number of special streams
	UINT sgi;				//short guard interval
	UINT chWidth;			//bandwidth

	UINT maxTpRate;			//index of rate with max throughput in this group
	UINT maxTpRate2;		//index of rate with 2nd max throughput in this group
	UINT maxProbRate;		//index of rate with max transmission probability in this group

	Ptr_HtRateInfo rateTable;	//Info of rates supported by this group
}GroupInfo,*Ptr_GroupInfo;

typedef struct stru_HtPerRemoteStation{
	NETSIM_ID remoteStationId;
	NETSIM_ID stationId;
	NETSIM_ID stationInterface;
	//variables for HT-info
	UINT sampleGroup;			//!< The group that the sample rate belongs to.
	UINT sampleWait;			//!< How many transmission attempts to wait until a new sample.
	UINT sampleTries;			//!< Number of sample tries after waiting sampleWait.
	UINT sampleCount;			//!< Max number of samples per update interval.
	UINT sampleSlow;			//!< Number of times a slow rate was sampled.

	double avgAmpduLen;			//!< Average number of MPDUs in an A-MPDU.
	double ampduLen;			//!< Number of MPDUs tried since last update
	UINT ampduPacketCount;		//!< Number of A-MPDUs transmitted since last update
	Ptr_GroupInfo groupTable;  //!< Table of groups with stats.
	UINT nGroup;				//Number of groups in the station
	UINT nGroupRate;			//number of rates supported
	BOOL isVht;					//If this station is VHT(802.11ac)

	//General variables
	BOOL isInitialized;				//Check if station is Initialized
	BOOL isSampling;				//Check if we are sampling
	double nextStatsUpdate;			//Scheduled time for next
	UINT trRate;					//Index of the transmission rate
	UINT sampleRate;				//Index of the sample rate
	UINT maxTpRate;					//Index of the max rate
	UINT maxTpRate2;				//Index of the 2nd best throughput
	UINT maxProbRate;				//Index of the max probability rate
	UINT longRetry;									
	UINT shortRetry;
	UINT totalRetry;				//total number of retries
	UINT** sampleTable;				//sample table for the station

	UINT64 totalPacketCount;		//total packet count of the station
	UINT64 samplePacketCount;		//sample packet count of the station
	PIEEE802_11_PHY_VAR phy;		//NetSim implementation requirement
}MinstrelHtPerRemoteStation,*Ptr_MinstrelHtPerRemoteStation;


typedef struct stru_MinstrelHtWifiStation{
	NETSIM_ID devId;
	NETSIM_ID interfaceId;
	Ptr_MinstrelHtPerRemoteStation *minstrelHtInfo;
}MinstrelHtWifiStation,*Ptr_MinstrelHtWifiStation;

double ht_updateStatsTime;					//Time Interval for updating Minstrel Table
double ht_lookAroundRate;				//% of pkts to send with other rates. ~10
double ht_ewmaWeight;					//weight to be given to new value (0,100) ~25%
UINT ht_sampleCol;						//number of columns in Sample Table


void Ht_InitMinstrel(NETSIM_ID nDevId,NETSIM_ID nifid);
static void CheckInit(Ptr_MinstrelHtPerRemoteStation station,NETSIM_ID devid,NETSIM_ID ifid);
static void InitSampleTable(Ptr_MinstrelHtPerRemoteStation station);
static void InitGroupTable(Ptr_MinstrelHtPerRemoteStation station);
static void UpdatePacketCounter(Ptr_MinstrelHtPerRemoteStation station,UINT success,UINT failed);
static void UpdateRetry(Ptr_MinstrelHtPerRemoteStation station);
static void UpdateStats(Ptr_MinstrelHtPerRemoteStation station);
static void UpdateRate(Ptr_MinstrelHtPerRemoteStation station);
static UINT FindRate(Ptr_MinstrelHtPerRemoteStation station);
static void SetStationThRate(Ptr_MinstrelHtPerRemoteStation station);
static void SetStationProbRate(Ptr_MinstrelHtPerRemoteStation station);
static UINT GetNextSample(Ptr_MinstrelHtPerRemoteStation station);
static void SetNextSample(Ptr_MinstrelHtPerRemoteStation station);
static double CalculateEwmsd(double oldEwmsd, double currentProb, double ewmaProb, double ewmaWeight);
static double CalculateThroughput(Ptr_MinstrelHtPerRemoteStation station,UINT grpId, UINT rateId);
static void CalculateRetransmits(Ptr_MinstrelHtPerRemoteStation station,UINT index);
static UINT CountRetries(Ptr_MinstrelHtPerRemoteStation station);
static UINT GetLowestIndex(Ptr_MinstrelHtPerRemoteStation station);
static UINT GetHighestIndex(Ptr_MinstrelHtPerRemoteStation station);


#ifdef __cplusplus
}
#endif