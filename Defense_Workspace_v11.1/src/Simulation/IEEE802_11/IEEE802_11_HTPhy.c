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
#include "main.h"
#include "IEEE802_11.h"
#include "IEEE802_11_Phy.h"
#include "ErrorModel.h"

/// Data structure for physical layer parameters
struct stru_802_11_Phy_Parameters_HT
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


};

static struct stru_802_11_Phy_Parameters_HT struPhyParameters_20MHz_1NSS[20] =
{
	//IEEE802.11-HT Phy 20 MHz
	{1,0,-82,Modulation_BPSK,Coding_1_2,1,52,4,52,26,1,800,6.5},
	{2,1,-79,Modulation_QPSK,Coding_1_2,1,52,4,104,52,1,800,13.0},
	{3,2,-77,Modulation_QPSK,Coding_3_4,2,52,4,104,78,1,800,19.5},
	{4,3,-74,Modulation_16_QAM,Coding_1_2,4,52,4,208,104,1,800,26.0},
	{5,4,-70,Modulation_16_QAM,Coding_3_4,4,52,4,208,156,1,800,39.0},
	{6,5,-66,Modulation_64_QAM,Coding_2_3,6,52,4,312,208,1,800,52.0},
	{7,6,-65,Modulation_64_QAM,Coding_3_4,6,52,4,312,234,1,800,58.5},
	{8,7,-64,Modulation_64_QAM,Coding_5_6,6,52,4,312,260,1,800,65.0},
	{9,8,-59,Modulation_256_QAM,Coding_3_4,8,52,4,416,312,1,800,78.0},
	{10,9,-57,Modulation_256_QAM,Coding_3_4,8,0,0,0,0,0,0,-1},//Not valid
	{11,0,-82,Modulation_BPSK,Coding_1_2,1,52,4,52,26,1,400,7.2},
	{12,1,-79,Modulation_QPSK,Coding_1_2,1,52,4,104,52,1,400,14.4},
	{13,2,-77,Modulation_QPSK,Coding_3_4,2,52,4,104,78,1,400,21.7},
	{14,3,-74,Modulation_16_QAM,Coding_1_2,4,52,4,208,104,1,400,28.9},
	{15,4,-70,Modulation_16_QAM,Coding_3_4,4,52,4,208,156,1,400,43.3},
	{16,5,-66,Modulation_64_QAM,Coding_2_3,6,52,4,312,208,1,400,57.8},
	{17,6,-65,Modulation_64_QAM,Coding_3_4,6,52,4,312,234,1,400,65.0},
	{18,7,-64,Modulation_64_QAM,Coding_5_6,6,52,4,312,260,1,400,72.2},
	{19,8,-59,Modulation_256_QAM,Coding_3_4,8,52,4,416,312,1,400,86.7},
	{20,9,-57,Modulation_256_QAM,Coding_3_4,8,0,0,0,0,0,0,-1},
};

static struct stru_802_11_Phy_Parameters_HT struPhyParameters_20MHz_2NSS[20] =
{
	//IEEE802.11-HT Phy 20 MHz
	{1,0,-82,Modulation_BPSK,Coding_1_2,1,52,4,104,52,1,800,13.0},
	{2,1,-79,Modulation_QPSK,Coding_1_2,1,52,4,208,104,1,800,26.0},
	{3,2,-77,Modulation_QPSK,Coding_3_4,2,52,4,208,156,1,800,39.0},
	{4,3,-74,Modulation_16_QAM,Coding_1_2,4,52,4,416,208,1,800,52.0},
	{5,4,-70,Modulation_16_QAM,Coding_3_4,4,52,4,416,312,1,800,78.0},
	{6,5,-66,Modulation_64_QAM,Coding_2_3,6,52,4,624,416,1,800,104.0},
	{7,6,-65,Modulation_64_QAM,Coding_3_4,6,52,4,624,468,1,800,117.0},
	{8,7,-64,Modulation_64_QAM,Coding_5_6,6,52,4,624,520,1,800,130.0},
	{9,8,-59,Modulation_256_QAM,Coding_3_4,8,52,4,832,624,1,800,156.0},
	{10,9,-57,Modulation_256_QAM,Coding_3_4,8,0,0,0,0,0,0,-1},//Not valid
	{11,0,-82,Modulation_BPSK,Coding_1_2,1,52,4,104,52,1,400,14.4},
	{12,1,-79,Modulation_QPSK,Coding_1_2,1,52,4,208,104,1,400,28.9},
	{13,2,-77,Modulation_QPSK,Coding_3_4,2,52,4,208,156,1,400,43.3},
	{14,3,-74,Modulation_16_QAM,Coding_1_2,4,52,4,416,208,1,400,57.8},
	{15,4,-70,Modulation_16_QAM,Coding_3_4,4,52,4,416,312,1,400,86.7},
	{16,5,-66,Modulation_64_QAM,Coding_2_3,6,52,4,624,416,1,400,115.6},
	{17,6,-65,Modulation_64_QAM,Coding_3_4,6,52,4,624,468,1,400,130.0},
	{18,7,-64,Modulation_64_QAM,Coding_5_6,6,52,4,624,520,1,400,144.4},
	{19,8,-59,Modulation_256_QAM,Coding_3_4,8,52,4,832,624,1,400,173.3},
	{20,8,-57,Modulation_256_QAM,Coding_3_4,8,0,0,0,0,0,0,-1},
};

static struct stru_802_11_Phy_Parameters_HT struPhyParameters_20MHz_3NSS[20] =
{
	//IEEE802.11-HT Phy 20 MHz
	{1,0,-82,Modulation_BPSK,Coding_1_2,1,52,4,156,78,1,800,19.5},
	{2,1,-79,Modulation_QPSK,Coding_1_2,1,52,4,312,156,1,800,39.0},
	{3,2,-77,Modulation_QPSK,Coding_3_4,2,52,4,312,234,1,800,58.5},
	{4,3,-74,Modulation_16_QAM,Coding_1_2,4,52,4,624,312,1,800,78.0},
	{5,4,-70,Modulation_16_QAM,Coding_3_4,4,52,4,624,468,1,800,117.0},
	{6,5,-66,Modulation_64_QAM,Coding_2_3,6,52,4,936,624,1,800,156.0},
	{7,6,-65,Modulation_64_QAM,Coding_3_4,6,52,4,936,702,1,800,175.5},
	{8,7,-64,Modulation_64_QAM,Coding_5_6,6,52,4,936,780,1,800,195.0},
	{9,8,-59,Modulation_256_QAM,Coding_3_4,8,52,4,1248,936,1,800,234.0},
	{10,9,-57,Modulation_256_QAM,Coding_3_4,8,52,4,1248,1040,1,800,260.0},
	{11,0,-82,Modulation_BPSK,Coding_1_2,1,52,4,156,78,1,400,21.7},
	{12,1,-79,Modulation_QPSK,Coding_1_2,1,52,4,312,156,1,400,43.3},
	{13,2,-77,Modulation_QPSK,Coding_3_4,2,52,4,312,234,1,400,65.0},
	{14,3,-74,Modulation_16_QAM,Coding_1_2,4,52,4,624,312,1,400,86.7},
	{15,4,-70,Modulation_16_QAM,Coding_3_4,4,52,4,624,468,1,400,130.0},
	{16,5,-66,Modulation_64_QAM,Coding_2_3,6,52,4,936,624,1,400,173.3},
	{17,6,-65,Modulation_64_QAM,Coding_3_4,6,52,4,936,702,1,400,195.0},
	{18,7,-64,Modulation_64_QAM,Coding_5_6,6,52,4,936,780,1,400,216.7},
	{19,8,-59,Modulation_256_QAM,Coding_3_4,8,52,4,1248,936,1,400,260.0},
	{20,9,-57,Modulation_256_QAM,Coding_3_4,8,52,4,1248,1040,1,400,288.9},
};

static struct stru_802_11_Phy_Parameters_HT struPhyParameters_20MHz_4NSS[20] =
{
	//IEEE802.11-HT Phy 20 MHz
	{1,0,-82,Modulation_BPSK,Coding_1_2,1,52,4,208,104,1,800,26.0},
	{2,1,-79,Modulation_QPSK,Coding_1_2,1,52,4,416,208,1,800,52.0},
	{3,2,-77,Modulation_QPSK,Coding_3_4,2,52,4,416,312,1,800,78.0},
	{4,3,-74,Modulation_16_QAM,Coding_1_2,4,52,4,832,416,1,800,104.0},
	{5,4,-70,Modulation_16_QAM,Coding_3_4,4,52,4,832,624,1,800,156.0},
	{6,5,-66,Modulation_64_QAM,Coding_2_3,6,52,4,1248,832,1,800,208.0},
	{7,6,-65,Modulation_64_QAM,Coding_3_4,6,52,4,1248,936,1,800,234.0},
	{8,7,-64,Modulation_64_QAM,Coding_5_6,6,52,4,1248,1040,1,800,260.0},
	{9,8,-59,Modulation_256_QAM,Coding_3_4,8,52,4,1664,1248,1,800,312.0},
	{10,9,-57,Modulation_256_QAM,Coding_3_4,8,0,0,0,0,0,0,-1},//Not valid
	{11,0,-82,Modulation_BPSK,Coding_1_2,1,52,4,208,104,1,400,28.9},
	{12,1,-79,Modulation_QPSK,Coding_1_2,1,52,4,416,208,1,400,57.8},
	{13,2,-77,Modulation_QPSK,Coding_3_4,2,52,4,416,312,1,400,86.7},
	{14,3,-74,Modulation_16_QAM,Coding_1_2,4,52,4,832,416,1,400,115.6},
	{15,4,-70,Modulation_16_QAM,Coding_3_4,4,52,4,832,624,1,400,173.3},
	{16,5,-66,Modulation_64_QAM,Coding_2_3,6,52,4,1248,832,1,400,231.1},
	{17,6,-65,Modulation_64_QAM,Coding_3_4,6,52,4,1248,936,1,400,260.0},
	{18,7,-64,Modulation_64_QAM,Coding_5_6,6,52,4,1248,1040,1,400,288.9},
	{19,8,-59,Modulation_256_QAM,Coding_3_4,8,52,4,1664,1248,1,400,346.7},
	{20,9,-57,Modulation_256_QAM,Coding_3_4,8,0,0,0,0,0,0,-1},//Not valid
};

static struct stru_802_11_Phy_Parameters_HT struPhyParameters_20MHz_5NSS[20] =
{
	//IEEE802.11-HT Phy 20 MHz
	{1,0,-82,Modulation_BPSK,Coding_1_2,1,52,4,260,130,1,800,32.5},
	{2,1,-79,Modulation_QPSK,Coding_1_2,1,52,4,520,260,1,800,65.0},
	{3,2,-77,Modulation_QPSK,Coding_3_4,2,52,4,520,390,1,800,97.5},
	{4,3,-74,Modulation_16_QAM,Coding_1_2,4,52,4,1040,520,1,800,130.0},
	{5,4,-70,Modulation_16_QAM,Coding_3_4,4,52,4,1040,780,1,800,195.0},
	{6,5,-66,Modulation_64_QAM,Coding_2_3,6,52,4,1560,1040,1,800,260.0},
	{7,6,-65,Modulation_64_QAM,Coding_3_4,6,52,4,1560,1170,1,800,292.5},
	{8,7,-64,Modulation_64_QAM,Coding_5_6,6,52,4,1560,1300,1,800,325.0},
	{9,8,-59,Modulation_256_QAM,Coding_3_4,8,52,4,2080,1560,1,800,390.0},
	{10,9,-57,Modulation_256_QAM,Coding_3_4,8,0,0,0,0,0,0,-1},//Not valid
	{11,0,-82,Modulation_BPSK,Coding_1_2,1,52,4,260,130,1,400,36.1},
	{12,1,-79,Modulation_QPSK,Coding_1_2,1,52,4,520,260,1,400,72.2},
	{13,2,-77,Modulation_QPSK,Coding_3_4,2,52,4,520,390,1,400,108.3},
	{14,3,-74,Modulation_16_QAM,Coding_1_2,4,52,4,1040,520,1,400,144.4},
	{15,4,-70,Modulation_16_QAM,Coding_3_4,4,52,4,1040,780,1,400,216.7},
	{16,5,-66,Modulation_64_QAM,Coding_2_3,6,52,4,1560,1040,1,400,288.9},
	{17,6,-65,Modulation_64_QAM,Coding_3_4,6,52,4,1560,1170,1,400,325.0},
	{18,7,-64,Modulation_64_QAM,Coding_5_6,6,52,4,1560,1300,1,400,361.1},
	{19,8,-59,Modulation_256_QAM,Coding_3_4,8,52,4,2080,1560,1,400,433.3},
	{20,9,-57,Modulation_256_QAM,Coding_3_4,8,0,0,0,0,0,0,-1},//Not valid
};

static struct stru_802_11_Phy_Parameters_HT struPhyParameters_20MHz_6NSS[20] =
{
	//IEEE802.11-HT Phy 20 MHz
	{1,0,-82,Modulation_BPSK,Coding_1_2,1,52,4,312,156,1,800,39.0},
	{2,1,-79,Modulation_QPSK,Coding_1_2,1,52,4,624,312,1,800,78.0},
	{3,2,-77,Modulation_QPSK,Coding_3_4,2,52,4,624,468,1,800,117.0},
	{4,3,-74,Modulation_16_QAM,Coding_1_2,4,52,4,1248,624,1,800,156.0},
	{5,4,-70,Modulation_16_QAM,Coding_3_4,4,52,4,1248,936,1,800,234.0},
	{6,5,-66,Modulation_64_QAM,Coding_2_3,6,52,4,1872,1248,1,800,312.0},
	{7,6,-65,Modulation_64_QAM,Coding_3_4,6,52,4,1872,1404,1,800,351.0},
	{8,7,-64,Modulation_64_QAM,Coding_5_6,6,52,4,1872,1560,1,800,390.0},
	{9,8,-59,Modulation_256_QAM,Coding_3_4,8,52,4,2496,1872,1,800,468.0},
	{10,9,-57,Modulation_256_QAM,Coding_3_4,8,52,4,2496,2080,1,800,520.0},
	{11,0,-82,Modulation_BPSK,Coding_1_2,1,52,4,312,156,1,400,43.3},
	{12,1,-79,Modulation_QPSK,Coding_1_2,1,52,4,624,312,1,400,86.7},
	{13,2,-77,Modulation_QPSK,Coding_3_4,2,52,4,624,468,1,400,130.0},
	{14,3,-74,Modulation_16_QAM,Coding_1_2,4,52,4,1248,624,1,400,173.3},
	{15,4,-70,Modulation_16_QAM,Coding_3_4,4,52,4,1248,936,1,400,260.0},
	{16,5,-66,Modulation_64_QAM,Coding_2_3,6,52,4,1872,1248,1,400,346.7},
	{17,6,-65,Modulation_64_QAM,Coding_3_4,6,52,4,1872,1404,1,400,390.0},
	{18,7,-64,Modulation_64_QAM,Coding_5_6,6,52,4,1872,1560,1,400,433.3},
	{19,8,-59,Modulation_256_QAM,Coding_3_4,8,52,4,2496,1872,1,400,520.0},
	{20,9,-57,Modulation_256_QAM,Coding_3_4,8,52,4,2496,2080,1,400,577.8},
};

static struct stru_802_11_Phy_Parameters_HT struPhyParameters_20MHz_7NSS[20] =
{
	//IEEE802.11-HT Phy 20 MHz
	{1,0,-82,Modulation_BPSK,Coding_1_2,1,52,4,364,182,1,800,45.5},
	{2,1,-79,Modulation_QPSK,Coding_1_2,1,52,4,728,364,1,800,91.0},
	{3,2,-77,Modulation_QPSK,Coding_3_4,2,52,4,728,546,1,800,136.5},
	{4,3,-74,Modulation_16_QAM,Coding_1_2,4,52,4,1456,728,1,800,182.0},
	{5,4,-70,Modulation_16_QAM,Coding_3_4,4,52,4,1456,1092,1,800,273.0},
	{6,5,-66,Modulation_64_QAM,Coding_2_3,6,52,4,2184,1456,1,800,364.0},
	{7,6,-65,Modulation_64_QAM,Coding_3_4,6,52,4,2184,1638,1,800,409.5},
	{8,7,-64,Modulation_64_QAM,Coding_5_6,6,52,4,2184,1820,1,800,455.0},
	{9,8,-59,Modulation_256_QAM,Coding_3_4,8,52,4,2912,2184,2,800,546.0},
	{10,9,-57,Modulation_256_QAM,Coding_3_4,8,0,0,0,0,0,0,-1},//Not valid
	{11,0,-82,Modulation_BPSK,Coding_1_2,1,52,4,364,182,1,400,50.6},
	{12,1,-79,Modulation_QPSK,Coding_1_2,1,52,4,728,364,1,400,101.1},
	{13,2,-77,Modulation_QPSK,Coding_3_4,2,52,4,728,546,1,400,151.7},
	{14,3,-74,Modulation_16_QAM,Coding_1_2,4,52,4,1456,728,1,400,202.2},
	{15,4,-70,Modulation_16_QAM,Coding_3_4,4,52,4,1456,1092,1,400,303.3},
	{16,5,-66,Modulation_64_QAM,Coding_2_3,6,52,4,2184,1456,1,400,404.4},
	{17,6,-65,Modulation_64_QAM,Coding_3_4,6,52,4,2184,1638,1,400,455.0},
	{18,7,-64,Modulation_64_QAM,Coding_5_6,6,52,4,2184,1820,1,400,505.6},
	{19,8,-59,Modulation_256_QAM,Coding_3_4,8,52,4,2912,2184,2,400,606.7},
	{20,9,-57,Modulation_256_QAM,Coding_3_4,8,0,0,0,0,0,0,-1},//Not valid
};

static struct stru_802_11_Phy_Parameters_HT struPhyParameters_20MHz_8NSS[20] =
{
	//IEEE802.11-HT Phy 20 MHz
	{1,0,-82,Modulation_BPSK,Coding_1_2,1,52,4,416,208,1,800,52.0},
	{2,1,-79,Modulation_QPSK,Coding_1_2,1,52,4,832,416,1,800,104.0},
	{3,2,-77,Modulation_QPSK,Coding_3_4,2,52,4,832,624,1,800,156.0},
	{4,3,-74,Modulation_16_QAM,Coding_1_2,4,52,4,1664,832,1,800,208.0},
	{5,4,-70,Modulation_16_QAM,Coding_3_4,4,52,4,1664,1248,1,800,312.0},
	{6,5,-66,Modulation_64_QAM,Coding_2_3,6,52,4,2496,1664,1,800,416.0},
	{7,6,-65,Modulation_64_QAM,Coding_3_4,6,52,4,2496,1872,1,800,468.0},
	{8,7,-64,Modulation_64_QAM,Coding_5_6,6,52,4,2496,2080,1,800,520.0},
	{9,8,-59,Modulation_256_QAM,Coding_3_4,8,52,4,3328,2496,2,800,624.0},
	{10,9,-57,Modulation_256_QAM,Coding_3_4,8,0,0,0,0,0,0,-1},//Not valid
	{11,0,-82,Modulation_BPSK,Coding_1_2,1,52,4,416,208,1,400,57.8},
	{12,1,-79,Modulation_QPSK,Coding_1_2,1,52,4,832,416,1,400,115.6},
	{13,2,-77,Modulation_QPSK,Coding_3_4,2,52,4,832,624,1,400,173.3},
	{14,3,-74,Modulation_16_QAM,Coding_1_2,4,52,4,1664,832,1,400,231.1},
	{15,4,-70,Modulation_16_QAM,Coding_3_4,4,52,4,1664,1248,1,400,346.7},
	{16,5,-66,Modulation_64_QAM,Coding_2_3,6,52,4,2496,1664,1,400,462.2},
	{17,6,-65,Modulation_64_QAM,Coding_3_4,6,52,4,2496,1872,1,400,520.0},
	{18,7,-64,Modulation_64_QAM,Coding_5_6,6,52,4,2496,2080,1,400,577.8},
	{19,8,-59,Modulation_256_QAM,Coding_3_4,8,52,4,3328,2496,2,400,693.3},
	{20,9,-57,Modulation_256_QAM,Coding_3_4,8,0,0,0,0,0,0,-1},
};


static struct stru_802_11_Phy_Parameters_HT struPhyParameters_40MHz_1NSS[20] =
{
	//IEEE802.11-HT Phy 40 MHz
	{1,0,-79,Modulation_BPSK,Coding_1_2,1,108,6,108,54,1,800,13.5},
	{2,1,-76,Modulation_QPSK,Coding_1_2,2,108,6,216,108,1,800,27.0},
	{3,2,-74,Modulation_QPSK,Coding_3_4,2,108,6,216,162,1,800,40.5},
	{4,3,-71,Modulation_16_QAM,Coding_1_2,4,108,6,432,216,1,800,54.0},
	{5,4,-67,Modulation_16_QAM,Coding_3_4,4,108,6,432,324,1,800,81.0},
	{6,5,-63,Modulation_64_QAM,Coding_2_3,6,108,6,648,432,1,800,108.0},
	{7,6,-62,Modulation_64_QAM,Coding_3_4,6,108,6,648,486,1,800,121.5},
	{8,7,-61,Modulation_64_QAM,Coding_5_6,6,108,6,648,540,1,800,135.0},
	{9,8,-56,Modulation_256_QAM,Coding_3_4,8,108,6,864,648,1,800,162.0},
	{10,9,-54,Modulation_256_QAM,Coding_5_6,8,108,6,864,720,1,800,180.0},
	{11,0,-79,Modulation_BPSK,Coding_1_2,1,108,6,108,54,1,400,15.0},
	{12,1,-76,Modulation_QPSK,Coding_1_2,2,108,6,216,108,1,400,30.0},
	{13,2,-74,Modulation_QPSK,Coding_3_4,2,108,6,216,162,1,400,45.0},
	{14,3,-71,Modulation_16_QAM,Coding_1_2,4,108,6,432,216,1,400,60.0},
	{15,4,-67,Modulation_16_QAM,Coding_3_4,4,108,6,432,324,1,400,90.0},
	{16,5,-63,Modulation_64_QAM,Coding_2_3,6,108,6,648,432,1,400,120.0},
	{17,6,-62,Modulation_64_QAM,Coding_3_4,6,108,6,648,486,1,400,135.0},
	{18,7,-61,Modulation_64_QAM,Coding_5_6,6,108,6,648,540,1,400,150.0},
	{19,8,-56,Modulation_256_QAM,Coding_3_4,8,108,6,864,648,1,400,180.0},
	{20,9,-54,Modulation_256_QAM,Coding_5_6,8,108,6,864,720,1,400,200.0},

};

static struct stru_802_11_Phy_Parameters_HT struPhyParameters_40MHz_2NSS[20] =
{
	//IEEE802.11-HT Phy 40 MHz
	{1,0,-79,Modulation_BPSK,Coding_1_2,1,108,6,216,108,1,800,27.0},
	{2,1,-76,Modulation_QPSK,Coding_1_2,2,108,6,432,216,1,800,54.0},
	{3,2,-74,Modulation_QPSK,Coding_3_4,2,108,6,432,324,1,800,81.0},
	{4,3,-71,Modulation_16_QAM,Coding_1_2,4,108,6,864,432,1,800,108.0},
	{5,4,-67,Modulation_16_QAM,Coding_3_4,4,108,6,864,648,1,800,162.0},
	{6,5,-63,Modulation_64_QAM,Coding_2_3,6,108,6,1296,864,1,800,216.0},
	{7,6,-62,Modulation_64_QAM,Coding_3_4,6,108,6,1296,972,1,800,243.0},
	{8,7,-61,Modulation_64_QAM,Coding_5_6,6,108,6,1296,1080,1,800,270.0},
	{9,8,-56,Modulation_256_QAM,Coding_3_4,8,108,6,1728,1296,1,800,324.0},
	{10,9,-54,Modulation_256_QAM,Coding_5_6,8,108,6,1728,1440,1,800,360.0},
	{11,0,-79,Modulation_BPSK,Coding_1_2,1,108,6,216,108,1,400,30.0},
	{12,1,-76,Modulation_QPSK,Coding_1_2,2,108,6,432,216,1,400,60.0},
	{13,2,-74,Modulation_QPSK,Coding_3_4,2,108,6,432,324,1,400,90.0},
	{14,3,-71,Modulation_16_QAM,Coding_1_2,4,108,6,864,432,1,400,120.0},
	{15,4,-67,Modulation_16_QAM,Coding_3_4,4,108,6,864,648,1,400,180.0},
	{16,5,-63,Modulation_64_QAM,Coding_2_3,6,108,6,1296,864,1,400,240.0},
	{17,6,-62,Modulation_64_QAM,Coding_3_4,6,108,6,1296,972,1,400,270.0},
	{18,7,-61,Modulation_64_QAM,Coding_5_6,6,108,6,1296,1080,1,400,300.0},
	{19,8,-56,Modulation_256_QAM,Coding_3_4,8,108,6,1728,1296,1,400,360.0},
	{20,9,-54,Modulation_256_QAM,Coding_5_6,8,108,6,1728,1440,1,400,400.0},

};

static struct stru_802_11_Phy_Parameters_HT struPhyParameters_40MHz_3NSS[20] =
{
	//IEEE802.11-HT Phy 40 MHz
	{1,0,-79,Modulation_BPSK,Coding_1_2,1,108,6,324,162,1,800,40.5},
	{2,1,-76,Modulation_QPSK,Coding_1_2,2,108,6,648,324,1,800,81.0},
	{3,2,-74,Modulation_QPSK,Coding_3_4,2,108,6,648,486,1,800,121.5},
	{4,3,-71,Modulation_16_QAM,Coding_1_2,4,108,6,1296,648,1,800,162.0},
	{5,4,-67,Modulation_16_QAM,Coding_3_4,4,108,6,1296,972,1,800,243.0},
	{6,5,-63,Modulation_64_QAM,Coding_2_3,6,108,6,1944,1296,1,800,324.0},
	{7,6,-62,Modulation_64_QAM,Coding_3_4,6,108,6,1944,1458,1,800,364.5},
	{8,7,-61,Modulation_64_QAM,Coding_5_6,6,108,6,1944,1620,1,800,405.0},
	{9,8,-56,Modulation_256_QAM,Coding_3_4,8,108,6,2592,1944,1,800,486.0},
	{10,9,-54,Modulation_256_QAM,Coding_5_6,8,108,6,2592,2160,1,800,540.0},
	{11,0,-79,Modulation_BPSK,Coding_1_2,1,108,6,324,162,1,400,45.0},
	{12,1,-76,Modulation_QPSK,Coding_1_2,2,108,6,648,324,1,400,90.0},
	{13,2,-74,Modulation_QPSK,Coding_3_4,2,108,6,648,486,1,400,135.0},
	{14,3,-71,Modulation_16_QAM,Coding_1_2,4,108,6,1296,648,1,400,180.0},
	{15,4,-67,Modulation_16_QAM,Coding_3_4,4,108,6,1296,972,1,400,270.0},
	{16,5,-63,Modulation_64_QAM,Coding_2_3,6,108,6,1944,1296,1,400,360.0},
	{17,6,-62,Modulation_64_QAM,Coding_3_4,6,108,6,1944,1458,1,400,405.5},
	{18,7,-61,Modulation_64_QAM,Coding_5_6,6,108,6,1944,1620,1,400,450.0},
	{19,8,-56,Modulation_256_QAM,Coding_3_4,8,108,6,2592,1944,1,400,540.0},
	{20,9,-54,Modulation_256_QAM,Coding_5_6,8,108,6,2592,2160,1,400,600.0},

};


static struct stru_802_11_Phy_Parameters_HT struPhyParameters_40MHz_4NSS[20] =
{
	//IEEE802.11-HT Phy 40 MHz
	{1,0,-79,Modulation_BPSK,Coding_1_2,1,108,6,432,216,1,800,54.0},
	{2,1,-76,Modulation_QPSK,Coding_1_2,2,108,6,864,432,1,800,108.0},
	{3,2,-74,Modulation_QPSK,Coding_3_4,2,108,6,864,648,1,800,162.0},
	{4,3,-71,Modulation_16_QAM,Coding_1_2,4,108,6,1728,864,1,800,216.0},
	{5,4,-67,Modulation_16_QAM,Coding_3_4,4,108,6,1728,1296,1,800,324.0},
	{6,5,-63,Modulation_64_QAM,Coding_2_3,6,108,6,2592,1728,1,800,432.0},
	{7,6,-62,Modulation_64_QAM,Coding_3_4,6,108,6,2592,1944,1,800,486.5},
	{8,7,-61,Modulation_64_QAM,Coding_5_6,6,108,6,2592,2160,1,800,540.0},
	{9,8,-56,Modulation_256_QAM,Coding_3_4,8,108,6,3456,2592,2,800,648.0},
	{10,9,-54,Modulation_256_QAM,Coding_5_6,8,108,6,3456,2880,2,800,720.0},
	{11,0,-79,Modulation_BPSK,Coding_1_2,1,108,6,432,216,1,400,60.0},
	{12,1,-76,Modulation_QPSK,Coding_1_2,2,108,6,864,432,1,400,120.0},
	{13,2,-74,Modulation_QPSK,Coding_3_4,2,108,6,864,648,1,400,180.0},
	{14,3,-71,Modulation_16_QAM,Coding_1_2,4,108,6,1728,864,1,400,240.0},
	{15,4,-67,Modulation_16_QAM,Coding_3_4,4,108,6,1728,1296,1,400,360.0},
	{16,5,-63,Modulation_64_QAM,Coding_2_3,6,108,6,2592,1728,1,400,480.0},
	{17,6,-62,Modulation_64_QAM,Coding_3_4,6,108,6,2592,1944,1,400,540.0},
	{18,7,-61,Modulation_64_QAM,Coding_5_6,6,108,6,2592,2160,1,400,600.0},
	{19,8,-56,Modulation_256_QAM,Coding_3_4,8,108,6,3456,2592,2,400,720.0},
	{20,9,-54,Modulation_256_QAM,Coding_5_6,8,108,6,3456,2880,2,400,800.0}

};


static struct stru_802_11_Phy_Parameters_HT struPhyParameters_40MHz_5NSS[20] =
{
	//IEEE802.11-HT Phy 40 MHz
	{1,0,-79,Modulation_BPSK,Coding_1_2,1,108,6,540,270,1,800,67.5},
	{2,1,-76,Modulation_QPSK,Coding_1_2,2,108,6,1080,540,1,800,135.0},
	{3,2,-74,Modulation_QPSK,Coding_3_4,2,108,6,1080,810,1,800,202.5},
	{4,3,-71,Modulation_16_QAM,Coding_1_2,4,108,6,2160,1080,1,800,270.0},
	{5,4,-67,Modulation_16_QAM,Coding_3_4,4,108,6,2160,1620,1,800,405.0},
	{6,5,-63,Modulation_64_QAM,Coding_2_3,6,108,6,3240,2160,1,800,540.0},
	{7,6,-62,Modulation_64_QAM,Coding_3_4,6,108,6,3240,2430,2,800,607.5},
	{8,7,-61,Modulation_64_QAM,Coding_5_6,6,108,6,3240,2700,2,800,675.0},
	{9,8,-56,Modulation_256_QAM,Coding_3_4,8,108,6,4320,3240,2,800,810.0},
	{10,9,-54,Modulation_256_QAM,Coding_5_6,8,108,6,4320,3600,2,800,900.0},
	{11,0,-79,Modulation_BPSK,Coding_1_2,1,108,6,540,270,1,400,75.0},
	{12,1,-76,Modulation_QPSK,Coding_1_2,2,108,6,1080,540,1,400,150.0},
	{13,2,-74,Modulation_QPSK,Coding_3_4,2,108,6,1080,810,1,400,225.5},
	{14,3,-71,Modulation_16_QAM,Coding_1_2,4,108,6,2160,1080,1,400,300.0},
	{15,4,-67,Modulation_16_QAM,Coding_3_4,4,108,6,2160,1620,1,400,450.0},
	{16,5,-63,Modulation_64_QAM,Coding_2_3,6,108,6,3240,2160,1,400,600.0},
	{17,6,-62,Modulation_64_QAM,Coding_3_4,6,108,6,3240,2430,2,400,675.0},
	{18,7,-61,Modulation_64_QAM,Coding_5_6,6,108,6,3240,2700,2,400,750.0},
	{19,8,-56,Modulation_256_QAM,Coding_3_4,8,108,6,4320,3240,2,400,900.0},
	{20,9,-54,Modulation_256_QAM,Coding_5_6,8,108,6,4320,3600,2,400,1000.0},

};

static struct stru_802_11_Phy_Parameters_HT struPhyParameters_40MHz_6NSS[20] =
{
	//IEEE802.11-HT Phy 40 MHz
	{1,0,-79,Modulation_BPSK,Coding_1_2,1,108,6,648,324,1,800,81.0},
	{2,1,-76,Modulation_QPSK,Coding_1_2,2,108,6,1296,648,1,800,162.0},
	{3,2,-74,Modulation_QPSK,Coding_3_4,2,108,6,1296,972,1,800,243.0},
	{4,3,-71,Modulation_16_QAM,Coding_1_2,4,108,6,2592,1296,1,800,324.0},
	{5,4,-67,Modulation_16_QAM,Coding_3_4,4,108,6,2592,1944,1,800,486.0},
	{6,5,-63,Modulation_64_QAM,Coding_2_3,6,108,6,3888,2592,1,800,648.0},
	{7,6,-62,Modulation_64_QAM,Coding_3_4,6,108,6,3888,2916,2,800,729.0},
	{8,7,-61,Modulation_64_QAM,Coding_5_6,6,108,6,3888,3240,2,800,810.0},
	{9,8,-56,Modulation_256_QAM,Coding_3_4,8,108,6,5184,3888,2,800,972.0},
	{10,9,-54,Modulation_256_QAM,Coding_5_6,8,108,6,5184,4320,2,800,1080.0},
	{11,0,-79,Modulation_BPSK,Coding_1_2,1,108,6,648,324,1,400,90.0},
	{12,1,-76,Modulation_QPSK,Coding_1_2,2,108,6,1296,648,1,400,180.0},
	{13,2,-74,Modulation_QPSK,Coding_3_4,2,108,6,1296,972,1,400,270.0},
	{14,3,-71,Modulation_16_QAM,Coding_1_2,4,108,6,2592,1296,1,400,360.0},
	{15,4,-67,Modulation_16_QAM,Coding_3_4,4,108,6,2592,1944,1,400,540.0},
	{16,5,-63,Modulation_64_QAM,Coding_2_3,6,108,6,3888,2592,1,400,720.0},
	{17,6,-62,Modulation_64_QAM,Coding_3_4,6,108,6,3888,2916,2,400,810.0},
	{18,7,-61,Modulation_64_QAM,Coding_5_6,6,108,6,3888,3240,2,400,900.0},
	{19,8,-56,Modulation_256_QAM,Coding_3_4,8,108,6,5184,3888,2,400,1080.0},
	{20,9,-54,Modulation_256_QAM,Coding_5_6,8,108,6,5184,4320,2,400,1200.0}

};

static struct stru_802_11_Phy_Parameters_HT struPhyParameters_40MHz_7NSS[20] =
{
	//IEEE802.11-HT Phy 40 MHz
	{1,0,-79,Modulation_BPSK,Coding_1_2,1,108,6,756,378,1,800,94.5},
	{2,1,-76,Modulation_QPSK,Coding_1_2,2,108,6,1512,756,1,800,189.0},
	{3,2,-74,Modulation_QPSK,Coding_3_4,2,108,6,1512,1134,1,800,283.5},
	{4,3,-71,Modulation_16_QAM,Coding_1_2,4,108,6,3024,1512,1,800,378.0},
	{5,4,-67,Modulation_16_QAM,Coding_3_4,4,108,6,3024,2268,1,800,567.0},
	{6,5,-63,Modulation_64_QAM,Coding_2_3,6,108,6,4536,3024,1,800,756.0},
	{7,6,-62,Modulation_64_QAM,Coding_3_4,6,108,6,4536,3402,2,800,850.5},
	{8,7,-61,Modulation_64_QAM,Coding_5_6,6,108,6,4536,3780,2,800,945.0},
	{9,8,-56,Modulation_256_QAM,Coding_3_4,8,108,6,6048,4536,3,800,1134.0},
	{10,9,-54,Modulation_256_QAM,Coding_5_6,8,108,6,6048,5040,3,800,1260.0},
	{11,0,-79,Modulation_BPSK,Coding_1_2,1,108,6,756,378,1,400,105.0},
	{12,1,-76,Modulation_QPSK,Coding_1_2,2,108,6,1512,756,1,400,210.0},
	{13,2,-74,Modulation_QPSK,Coding_3_4,2,108,6,1512,1134,1,400,315.0},
	{14,3,-71,Modulation_16_QAM,Coding_1_2,4,108,6,3024,1512,1,400,420.0},
	{15,4,-67,Modulation_16_QAM,Coding_3_4,4,108,6,3024,2268,1,400,630.0},
	{16,5,-63,Modulation_64_QAM,Coding_2_3,6,108,6,4536,3024,1,400,840.0},
	{17,6,-62,Modulation_64_QAM,Coding_3_4,6,108,6,4536,3402,2,400,945.0},
	{18,7,-61,Modulation_64_QAM,Coding_5_6,6,108,6,4536,3780,2,400,1050.0},
	{19,8,-56,Modulation_256_QAM,Coding_3_4,8,108,6,6048,4536,3,400,1260.0},
	{20,9,-54,Modulation_256_QAM,Coding_5_6,8,108,6,6048,5040,3,400,1400.0},

};

static struct stru_802_11_Phy_Parameters_HT struPhyParameters_40MHz_8NSS[20] =
{
	//IEEE802.11-HT Phy 40 MHz
	{1,0,-79,Modulation_BPSK,Coding_1_2,1,108,6,864,432,1,800,108.0},
	{2,1,-76,Modulation_QPSK,Coding_1_2,2,108,6,1728,864,1,800,216.0},
	{3,2,-74,Modulation_QPSK,Coding_3_4,2,108,6,1728,1296,1,800,324.0},
	{4,3,-71,Modulation_16_QAM,Coding_1_2,4,108,6,3456,1728,1,800,432.0},
	{5,4,-67,Modulation_16_QAM,Coding_3_4,4,108,6,3456,2592,1,800,648.0},
	{6,5,-63,Modulation_64_QAM,Coding_2_3,6,108,6,5184,3456,1,800,864.0},
	{7,6,-62,Modulation_64_QAM,Coding_3_4,6,108,6,5184,3888,2,800,972.0},
	{8,7,-61,Modulation_64_QAM,Coding_5_6,6,108,6,5184,4320,2,800,1080.0},
	{9,8,-56,Modulation_256_QAM,Coding_3_4,8,108,6,6912,5184,3,800,1296.0},
	{10,9,-54,Modulation_256_QAM,Coding_5_6,8,108,6,6912,5760,3,800,1440.0},
	{11,0,-79,Modulation_BPSK,Coding_1_2,1,108,6,864,432,1,400,120.0},
	{12,1,-76,Modulation_QPSK,Coding_1_2,2,108,6,1728,864,1,400,240.0},
	{13,2,-74,Modulation_QPSK,Coding_3_4,2,108,6,1728,1296,1,400,360.0},
	{14,3,-71,Modulation_16_QAM,Coding_1_2,4,108,6,3456,1728,1,400,480.0},
	{15,4,-67,Modulation_16_QAM,Coding_3_4,4,108,6,3456,2592,1,400,720.0},
	{16,5,-63,Modulation_64_QAM,Coding_2_3,6,108,6,5184,3456,1,400,960.0},
	{17,6,-62,Modulation_64_QAM,Coding_3_4,6,108,6,5184,3888,2,400,1080.0},
	{18,7,-61,Modulation_64_QAM,Coding_5_6,6,108,6,5184,4320,2,400,1200.0},
	{19,8,-56,Modulation_256_QAM,Coding_3_4,8,108,6,6912,5184,3,400,1440.0},
	{20,9,-54,Modulation_256_QAM,Coding_5_6,8,108,6,6912,5760,3,400,1600.0},

};

static struct stru_802_11_Phy_Parameters_HT struPhyParameters_80MHz_1NSS[20] =
{
	//IEEE802.11-VHT Phy 80 MHz
	{1,0,-76,Modulation_BPSK,Coding_1_2,1,234,8,234,117,1,800,29.3},
	{2,1,-73,Modulation_QPSK,Coding_1_2,2,234,8,468,234,1,800,58.5},
	{3,2,-71,Modulation_QPSK,Coding_3_4,2,264,8,468,351,1,800,87.8},
	{4,3,-68,Modulation_16_QAM,Coding_1_2,4,234,8,936,468,1,800,117.0},
	{5,4,-64,Modulation_16_QAM,Coding_3_4,4,234,8,936,702,1,800,175.5},
	{6,5,-60,Modulation_64_QAM,Coding_2_3,6,234,8,1404,936,1,800,234.0},
	{7,6,-59,Modulation_64_QAM,Coding_3_4,6,234,8,1404,1053,1,800,263.3},
	{8,7,-58,Modulation_64_QAM,Coding_5_6,6,234,8,1404,1170,1,800,292.5},
	{9,8,-53,Modulation_256_QAM,Coding_3_4,8,234,8,1872,1404,1,800,351.0},
	{10,9,-51,Modulation_256_QAM,Coding_5_6,8,234,8,1872,1560,1,800,390.0},
	{11,0,-76,Modulation_BPSK,Coding_1_2,1,234,8,234,117,1,400,32.5},
	{12,1,-73,Modulation_QPSK,Coding_1_2,2,234,8,468,234,1,400,65.0},
	{13,2,-71,Modulation_QPSK,Coding_3_4,2,264,8,468,351,1,400,97.5},
	{14,3,-68,Modulation_16_QAM,Coding_1_2,4,234,8,936,468,1,400,130.0},
	{15,4,-64,Modulation_16_QAM,Coding_3_4,4,234,8,936,702,1,400,195.0},
	{16,5,-60,Modulation_64_QAM,Coding_2_3,6,234,8,1404,936,1,400,260.0},
	{17,6,-59,Modulation_64_QAM,Coding_3_4,6,234,8,1404,1053,1,400,292.5},
	{18,7,-58,Modulation_64_QAM,Coding_5_6,6,234,8,1404,1170,1,400,325.0},
	{19,8,-53,Modulation_256_QAM,Coding_3_4,8,234,8,1872,1404,1,400,390.0},
	{20,9,-51,Modulation_256_QAM,Coding_5_6,8,234,8,1872,1560,1,400,433.3},

};

static struct stru_802_11_Phy_Parameters_HT struPhyParameters_80MHz_2NSS[20] =
{
	//IEEE802.11-VHT Phy 80 MHz
	{1,0,-76,Modulation_BPSK,Coding_1_2,1,234,8,468,234,1,800,58.5},
	{2,1,-73,Modulation_QPSK,Coding_1_2,2,234,8,936,468,1,800,117.0},
	{3,2,-71,Modulation_QPSK,Coding_3_4,2,264,8,936,702,1,800,175.5},
	{4,3,-68,Modulation_16_QAM,Coding_1_2,4,234,8,1872,936,1,800,234.0},
	{5,4,-64,Modulation_16_QAM,Coding_3_4,4,234,8,1872,1404,1,800,351.0},
	{6,5,-60,Modulation_64_QAM,Coding_2_3,6,234,8,2808,1872,1,800,468.0},
	{7,6,-59,Modulation_64_QAM,Coding_3_4,6,234,8,2808,2106,1,800,526.5},
	{8,7,-58,Modulation_64_QAM,Coding_5_6,6,234,8,2808,2340,2,800,585.0},
	{9,8,-53,Modulation_256_QAM,Coding_3_4,8,234,8,3744,2808,2,800,702.0},
	{10,9,-51,Modulation_256_QAM,Coding_5_6,8,234,8,3744,3120,2,800,780.0},
	{11,0,-76,Modulation_BPSK,Coding_1_2,1,234,8,468,234,1,400,65.0},
	{12,1,-73,Modulation_QPSK,Coding_1_2,2,234,8,936,468,1,400,130.0},
	{13,2,-71,Modulation_QPSK,Coding_3_4,2,264,8,936,702,1,400,195.0},
	{14,3,-68,Modulation_16_QAM,Coding_1_2,4,234,8,1872,936,1,400,260.0},
	{15,4,-64,Modulation_16_QAM,Coding_3_4,4,234,8,1872,1404,1,400,390.0},
	{16,5,-60,Modulation_64_QAM,Coding_2_3,6,234,8,2808,1872,1,400,520.0},
	{17,6,-59,Modulation_64_QAM,Coding_3_4,6,234,8,2808,2106,1,400,585.0},
	{18,7,-58,Modulation_64_QAM,Coding_5_6,6,234,8,2808,2340,2,400,650.0},
	{19,8,-53,Modulation_256_QAM,Coding_3_4,8,234,8,3744,2808,2,400,780.0},
	{20,9,-51,Modulation_256_QAM,Coding_5_6,8,234,8,3744,3120,2,400,866.7},

};

static struct stru_802_11_Phy_Parameters_HT struPhyParameters_80MHz_3NSS[20] =
{
	//IEEE802.11-VHT Phy 80 MHz
	{1,0,-76,Modulation_BPSK,Coding_1_2,1,234,8,702,351,1,800,87.8},
	{2,1,-73,Modulation_QPSK,Coding_1_2,2,234,8,1404,702,1,800,175.5},
	{3,2,-71,Modulation_QPSK,Coding_3_4,2,264,8,1404,1053,1,800,263.3},
	{4,3,-68,Modulation_16_QAM,Coding_1_2,4,234,8,2808,1404,1,800,351.0},
	{5,4,-64,Modulation_16_QAM,Coding_3_4,4,234,8,2808,2106,1,800,526.5},
	{6,5,-60,Modulation_64_QAM,Coding_2_3,6,264,8,4212,2808,2,800,702.0},
	{7,6,-59,Modulation_64_QAM,Coding_3_4,6,0,0,0,0,0,0,-1},
	{8,7,-58,Modulation_64_QAM,Coding_5_6,6,234,8,4212,3510,2,800,877.5},
	{9,8,-53,Modulation_256_QAM,Coding_3_4,8,234,8,5616,4212,2,800,1053.0},
	{10,9,-51,Modulation_256_QAM,Coding_5_6,8,234,8,5616,4680,3,800,1170.0},
	{11,0,-76,Modulation_BPSK,Coding_1_2,1,234,8,702,351,1,400,97.5},
	{12,1,-73,Modulation_QPSK,Coding_1_2,2,234,8,1404,702,1,400,195.0},
	{13,2,-71,Modulation_QPSK,Coding_3_4,2,264,8,1404,1053,1,400,292.5},
	{14,3,-68,Modulation_16_QAM,Coding_1_2,4,234,8,2808,1404,1,400,390.0},
	{15,4,-64,Modulation_16_QAM,Coding_3_4,4,234,8,2808,2106,1,400,585.0},
	{16,5,-60,Modulation_64_QAM,Coding_2_3,6,264,8,4212,2808,2,400,780.0},
	{17,6,-59,Modulation_64_QAM,Coding_3_4,6,0,0,0,0,0,0,-1},
	{18,7,-58,Modulation_64_QAM,Coding_5_6,6,234,8,4212,3510,2,400,975.0},
	{19,8,-53,Modulation_256_QAM,Coding_3_4,8,234,8,5616,4212,2,400,1170.0},
	{20,9,-51,Modulation_256_QAM,Coding_5_6,8,234,8,5616,4680,3,400,1300.0},

};

static struct stru_802_11_Phy_Parameters_HT struPhyParameters_80MHz_4NSS[20] =
{
	//IEEE802.11-VHT Phy 80 MHz
	{1,0,-76,Modulation_BPSK,Coding_1_2,1,234,8,936,468,1,800,117.0},
	{2,1,-73,Modulation_QPSK,Coding_1_2,2,234,8,1872,936,1,800,234.0},
	{3,2,-71,Modulation_QPSK,Coding_3_4,2,264,8,1872,1404,1,800,351.0},
	{4,3,-68,Modulation_16_QAM,Coding_1_2,4,234,8,3744,1872,1,800,468.0},
	{5,4,-64,Modulation_16_QAM,Coding_3_4,4,234,8,3744,2808,2,800,702.0},
	{6,5,-60,Modulation_64_QAM,Coding_2_3,6,264,8,5616,3744,2,800,936.0},
	{7,6,-59,Modulation_64_QAM,Coding_3_4,6,234,8,5616,4212,2,800,1053.0},
	{8,7,-58,Modulation_64_QAM,Coding_5_6,6,234,8,5616,4680,3,800,1170.0},
	{9,8,-53,Modulation_256_QAM,Coding_3_4,8,234,8,7488,5616,3,800,1404.0},
	{10,9,-51,Modulation_256_QAM,Coding_5_6,8,234,8,7488,6240,3,800,1560.0},
	{11,0,-76,Modulation_BPSK,Coding_1_2,1,234,8,936,468,1,400,130.0},
	{12,1,-73,Modulation_QPSK,Coding_1_2,2,234,8,1872,936,1,400,260.0},
	{13,2,-71,Modulation_QPSK,Coding_3_4,2,264,8,1872,1404,1,400,390.0},
	{14,3,-68,Modulation_16_QAM,Coding_1_2,4,234,8,3744,1872,1,400,520.0},
	{15,4,-64,Modulation_16_QAM,Coding_3_4,4,234,8,3744,2808,2,400,780.0},
	{16,5,-60,Modulation_64_QAM,Coding_2_3,6,264,8,5616,3744,2,400,1040.0},
	{17,6,-59,Modulation_64_QAM,Coding_3_4,6,234,8,5616,4212,2,400,1170.0},
	{18,7,-58,Modulation_64_QAM,Coding_5_6,6,234,8,5616,4680,3,400,1300.0},
	{19,8,-53,Modulation_256_QAM,Coding_3_4,8,234,8,7488,5616,3,400,1560.0},
	{20,9,-51,Modulation_256_QAM,Coding_5_6,8,234,8,7488,6240,3,400,1733.3},

};

static struct stru_802_11_Phy_Parameters_HT struPhyParameters_80MHz_5NSS[20] =
{
	//IEEE802.11-VHT Phy 80 MHz
	{1,0,-76,Modulation_BPSK,Coding_1_2,1,234,8,1170,585,1,800,146.3},
	{2,1,-73,Modulation_QPSK,Coding_1_2,2,234,8,2340,1170,1,800,292.5},
	{3,2,-71,Modulation_QPSK,Coding_3_4,2,264,8,2340,1755,1,800,438.8},
	{4,3,-68,Modulation_16_QAM,Coding_1_2,4,234,8,4680,2340,2,800,585.0},
	{5,4,-64,Modulation_16_QAM,Coding_3_4,4,234,8,4680,3510,2,800,877.5},
	{6,5,-60,Modulation_64_QAM,Coding_2_3,6,264,8,7020,4680,3,800,1170.0},
	{7,6,-59,Modulation_64_QAM,Coding_3_4,6,234,8,7020,5265,3,800,1316.3},
	{8,7,-58,Modulation_64_QAM,Coding_5_6,6,234,8,7020,5850,3,800,1462.5},
	{9,8,-53,Modulation_256_QAM,Coding_3_4,8,234,8,9360,7020,4,800,1755.0},
	{10,9,-51,Modulation_256_QAM,Coding_5_6,8,234,8,9360,7800,4,800,1950.0},
	{11,0,-76,Modulation_BPSK,Coding_1_2,1,234,8,1170,585,1,400,162.5},
	{12,1,-73,Modulation_QPSK,Coding_1_2,2,234,8,2340,1170,1,400,325.0},
	{13,2,-71,Modulation_QPSK,Coding_3_4,2,264,8,2340,1755,1,400,487.5},
	{14,3,-68,Modulation_16_QAM,Coding_1_2,4,234,8,4680,2340,2,400,650.0},
	{15,4,-64,Modulation_16_QAM,Coding_3_4,4,234,8,4680,3510,2,400,975.0},
	{16,5,-60,Modulation_64_QAM,Coding_2_3,6,264,8,7020,4680,3,400,1300.0},
	{17,6,-59,Modulation_64_QAM,Coding_3_4,6,234,8,7020,5265,3,400,1462.5},
	{18,7,-58,Modulation_64_QAM,Coding_5_6,6,234,8,7020,5850,3,400,1625.0},
	{19,8,-53,Modulation_256_QAM,Coding_3_4,8,234,8,9360,7020,4,400,1950.0},
	{20,9,-51,Modulation_256_QAM,Coding_5_6,8,234,8,9360,7800,4,400,2166.7},

};

static struct stru_802_11_Phy_Parameters_HT struPhyParameters_80MHz_6NSS[20] =
{
	//IEEE802.11-VHT Phy 80 MHz
	{1,0,-76,Modulation_BPSK,Coding_1_2,1,234,8,1404,702,1,800,175.5},
	{2,1,-73,Modulation_QPSK,Coding_1_2,2,234,8,2808,1404,1,800,351.0},
	{3,2,-71,Modulation_QPSK,Coding_3_4,2,264,8,2808,2106,1,800,526.5},
	{4,3,-68,Modulation_16_QAM,Coding_1_2,4,234,8,5616,2808,2,800,702.0},
	{5,4,-64,Modulation_16_QAM,Coding_3_4,4,234,8,5616,4212,2,800,1053.0},
	{6,5,-60,Modulation_64_QAM,Coding_2_3,6,264,8,8424,5616,3,800,1404.0},
	{7,6,-59,Modulation_64_QAM,Coding_3_4,6,234,8,8424,6318,3,800,1579.5},
	{8,7,-58,Modulation_64_QAM,Coding_5_6,6,234,8,8424,7020,4,800,1755.0},
	{9,8,-53,Modulation_256_QAM,Coding_3_4,8,234,8,11232,8424,4,800,2106.0},
	{10,9,-51,Modulation_256_QAM,Coding_5_6,8,0,0,0,0,0,0,-1},//Not valid
	{11,0,-76,Modulation_BPSK,Coding_1_2,1,234,8,1404,702,1,400,195.0},
	{12,1,-73,Modulation_QPSK,Coding_1_2,2,234,8,2808,1404,1,400,390.0},
	{13,2,-71,Modulation_QPSK,Coding_3_4,2,264,8,2808,2106,1,400,585.0},
	{14,3,-68,Modulation_16_QAM,Coding_1_2,4,234,8,5616,2808,2,400,780.0},
	{15,4,-64,Modulation_16_QAM,Coding_3_4,4,234,8,5616,4212,2,400,1170.0},
	{16,5,-60,Modulation_64_QAM,Coding_2_3,6,264,8,8424,5616,3,400,1560.0},
	{17,6,-59,Modulation_64_QAM,Coding_3_4,6,234,8,8424,6318,3,400,1755.0},
	{18,7,-58,Modulation_64_QAM,Coding_5_6,6,234,8,8424,7020,4,400,1950.0},
	{19,8,-53,Modulation_256_QAM,Coding_3_4,8,234,8,11232,8424,4,400,2340.0},
	{20,9,-51,Modulation_256_QAM,Coding_5_6,8,0,0,0,0,0,0,-1},//Not valid

};

static struct stru_802_11_Phy_Parameters_HT struPhyParameters_80MHz_7NSS[20] =
{
	//IEEE802.11-VHT Phy 80 MHz
	{1,0,-76,Modulation_BPSK,Coding_1_2,1,234,8,1638,819,1,800,204.8},
	{2,1,-73,Modulation_QPSK,Coding_1_2,2,234,8,3276,1638,1,800,409.5},
	{3,2,-71,Modulation_QPSK,Coding_3_4,2,264,8,3276,2457,3,800,614.3},
	{4,3,-68,Modulation_16_QAM,Coding_1_2,4,234,8,6552,3276,2,800,819.0},
	{5,4,-64,Modulation_16_QAM,Coding_3_4,4,234,8,6552,4914,3,800,1228.5},
	{6,5,-60,Modulation_64_QAM,Coding_2_3,6,264,8,9828,6552,4,800,1638.0},
	{7,6,-59,Modulation_64_QAM,Coding_3_4,6,0,0,0,0,0,0,-1},//Not valid
	{8,7,-58,Modulation_64_QAM,Coding_5_6,6,234,8,9828,8190,6,800,2047.5},
	{9,8,-53,Modulation_256_QAM,Coding_3_4,8,234,8,13104,9828,6,800,2457.0},
	{10,9,-51,Modulation_256_QAM,Coding_5_6,8,234,8,13104,10920,6,800,2730},
	{11,0,-76,Modulation_BPSK,Coding_1_2,1,234,8,1638,819,1,400,227.5},
	{12,1,-73,Modulation_QPSK,Coding_1_2,2,234,8,3276,1638,1,400,455.0},
	{13,2,-71,Modulation_QPSK,Coding_3_4,2,264,8,3276,2457,3,400,682.5},
	{14,3,-68,Modulation_16_QAM,Coding_1_2,4,234,8,6552,3276,2,400,910.0},
	{15,4,-64,Modulation_16_QAM,Coding_3_4,4,234,8,6552,4914,3,400,1365.0},
	{16,5,-60,Modulation_64_QAM,Coding_2_3,6,264,8,9828,6552,4,400,1820.0},
	{17,6,-59,Modulation_64_QAM,Coding_3_4,6,0,0,0,0,0,0,-1},//Not valid
	{18,7,-58,Modulation_64_QAM,Coding_5_6,6,234,8,9828,8190,6,400,2275.0},
	{19,8,-53,Modulation_256_QAM,Coding_3_4,8,234,8,13104,9828,6,400,2730.0},
	{20,9,-51,Modulation_256_QAM,Coding_5_6,8,234,8,13104,10920,6,400,3033.3},

};

static struct stru_802_11_Phy_Parameters_HT struPhyParameters_80MHz_8NSS[20] =
{
	//IEEE802.11-VHT Phy 80 MHz
	{1,0,-76,Modulation_BPSK,Coding_1_2,1,234,8,1872,936,1,800,234.0},
	{2,1,-73,Modulation_QPSK,Coding_1_2,2,234,8,3744,1872,1,800,468.0},
	{3,2,-71,Modulation_QPSK,Coding_3_4,2,264,8,3744,2808,2,800,702.0},
	{4,3,-68,Modulation_16_QAM,Coding_1_2,4,234,8,7488,3744,2,800,936.0},
	{5,4,-64,Modulation_16_QAM,Coding_3_4,4,234,8,7488,5616,3,800,1404.0},
	{6,5,-60,Modulation_64_QAM,Coding_2_3,6,264,8,11232,7488,4,800,1872.0},
	{7,6,-59,Modulation_64_QAM,Coding_3_4,6,234,8,11232,8424,4,800,2106.0},
	{8,7,-58,Modulation_64_QAM,Coding_5_6,6,234,8,11232,9360,6,800,2340.0},
	{9,8,-53,Modulation_256_QAM,Coding_3_4,8,234,8,14976,11232,6,800,2808.0},
	{10,9,-51,Modulation_256_QAM,Coding_5_6,8,234,8,14976,12480,6,800,3120.0},
	{11,0,-76,Modulation_BPSK,Coding_1_2,1,234,8,1872,936,1,400,260.0},
	{12,1,-73,Modulation_QPSK,Coding_1_2,2,234,8,3744,1872,1,400,520.0},
	{13,2,-71,Modulation_QPSK,Coding_3_4,2,264,8,3744,2808,2,400,780.0},
	{14,3,-68,Modulation_16_QAM,Coding_1_2,4,234,8,7488,3744,2,400,1040.0},
	{15,4,-64,Modulation_16_QAM,Coding_3_4,4,234,8,7488,5616,3,400,1560.0},
	{16,5,-60,Modulation_64_QAM,Coding_2_3,6,264,8,11232,7488,4,400,2080.0},
	{17,6,-59,Modulation_64_QAM,Coding_3_4,6,234,8,11232,8424,4,400,2340.0},
	{18,7,-58,Modulation_64_QAM,Coding_5_6,6,234,8,11232,9360,6,400,2600.0},
	{19,8,-53,Modulation_256_QAM,Coding_3_4,8,234,8,14976,11232,6,400,3120.0},
	{20,9,-51,Modulation_256_QAM,Coding_5_6,8,234,8,14976,12480,6,400,3466.7},

};

static struct stru_802_11_Phy_Parameters_HT struPhyParameters_160MHz_1NSS[20] =
{
	//IEEE802.11-VHT Phy 160 MHz
	{1,0,-73,Modulation_BPSK,Coding_1_2,1,468,16,468,234,1,800,58.5},
	{2,1,-70,Modulation_QPSK,Coding_1_2,2,468,16,936,468,1,800,117.0},
	{3,2,-68,Modulation_QPSK,Coding_3_4,2,468,16,936,702,1,800,175.5},
	{4,3,-65,Modulation_16_QAM,Coding_1_2,4,468,16,1872,936,1,800,234.0},
	{5,4,-61,Modulation_16_QAM,Coding_3_4,4,468,16,1872,1404,1,800,351.0},
	{6,5,-57,Modulation_64_QAM,Coding_2_3,6,468,16,2808,1872,1,800,468.0},
	{7,6,-56,Modulation_64_QAM,Coding_3_4,6,468,16,2808,2106,1,800,526.5},
	{8,7,-55,Modulation_64_QAM,Coding_5_6,6,268,16,2808,2340,2,800,585.0},
	{9,8,-50,Modulation_256_QAM,Coding_3_4,8,468,16,3744,2808,2,800,702.0},
	{10,9,-48,Modulation_256_QAM,Coding_5_6,8,468,16,3744,3120,2,800,780.0},
	{11,0,-73,Modulation_BPSK,Coding_1_2,1,468,16,468,234,1,400,65.0},
	{12,1,-70,Modulation_QPSK,Coding_1_2,2,468,16,936,468,1,400,130.0},
	{13,2,-68,Modulation_QPSK,Coding_3_4,2,468,16,936,702,1,400,195.0},
	{14,3,-65,Modulation_16_QAM,Coding_1_2,4,468,16,1872,936,1,400,260.0},
	{15,4,-61,Modulation_16_QAM,Coding_3_4,4,468,16,1872,1404,1,400,390.0},
	{16,5,-57,Modulation_64_QAM,Coding_2_3,6,468,16,2808,1872,1,400,520.0},
	{17,6,-56,Modulation_64_QAM,Coding_3_4,6,468,16,2808,2106,1,400,585.0},
	{18,7,-55,Modulation_64_QAM,Coding_5_6,6,268,16,2808,2340,2,400,650.0},
	{19,8,-50,Modulation_256_QAM,Coding_3_4,8,468,16,3744,2808,2,400,780.0},
	{20,9,-48,Modulation_256_QAM,Coding_5_6,8,468,16,3744,3120,2,400,866.7},

};

static struct stru_802_11_Phy_Parameters_HT struPhyParameters_160MHz_2NSS[20] =
{
	//IEEE802.11-VHT Phy 160 MHz
	{1,0,-73,Modulation_BPSK,Coding_1_2,1,468,16,936,468,1,800,117.0},
	{2,1,-70,Modulation_QPSK,Coding_1_2,2,468,16,1872,936,1,800,234.0},
	{3,2,-68,Modulation_QPSK,Coding_3_4,2,468,16,1872,1404,1,800,351.0},
	{4,3,-65,Modulation_16_QAM,Coding_1_2,4,468,16,3744,1872,1,800,468.0},
	{5,4,-61,Modulation_16_QAM,Coding_3_4,4,468,16,3744,2808,2,800,702.0},
	{6,5,-57,Modulation_64_QAM,Coding_2_3,6,468,16,5616,3744,2,800,936.0},
	{7,6,-56,Modulation_64_QAM,Coding_3_4,6,468,16,5616,4212,2,800,1053.0},
	{8,7,-55,Modulation_64_QAM,Coding_5_6,6,268,16,5616,4680,3,800,1170.0},
	{9,8,-50,Modulation_256_QAM,Coding_3_4,8,468,16,7488,5616,3,800,1404.0},
	{10,9,-48,Modulation_256_QAM,Coding_5_6,8,468,16,7488,6240,3,800,1560.0},
	{11,0,-73,Modulation_BPSK,Coding_1_2,1,468,16,936,468,1,400,130.0},
	{12,1,-70,Modulation_QPSK,Coding_1_2,2,468,16,1872,936,1,400,260.0},
	{13,2,-68,Modulation_QPSK,Coding_3_4,2,468,16,1872,1404,1,400,390.0},
	{14,3,-65,Modulation_16_QAM,Coding_1_2,4,468,16,3744,1872,1,400,520.0},
	{15,4,-61,Modulation_16_QAM,Coding_3_4,4,468,16,3744,2808,2,400,780.0},
	{16,5,-57,Modulation_64_QAM,Coding_2_3,6,468,16,5616,3744,2,400,1040.0},
	{17,6,-56,Modulation_64_QAM,Coding_3_4,6,468,16,5616,4212,2,400,1170.0},
	{18,7,-55,Modulation_64_QAM,Coding_5_6,6,268,16,5616,4680,3,400,1300.0},
	{19,8,-50,Modulation_256_QAM,Coding_3_4,9,468,16,7488,5616,3,400,1560.0},
	{20,9,-48,Modulation_256_QAM,Coding_5_6,8,468,16,7488,6240,3,400,1733.3}

};

static struct stru_802_11_Phy_Parameters_HT struPhyParameters_160MHz_3NSS[20] =
{
	//IEEE802.11-VHT Phy 160 MHz
	{1,0,-73,Modulation_BPSK,Coding_1_2,1,468,16,1404,702,1,800,175.5},
	{2,1,-70,Modulation_QPSK,Coding_1_2,2,468,16,2808,1404,1,800,351.0},
	{3,2,-68,Modulation_QPSK,Coding_3_4,2,468,16,2808,2106,1,800,526.5},
	{4,3,-65,Modulation_16_QAM,Coding_1_2,4,468,16,5616,2808,2,800,702.0},
	{5,4,-61,Modulation_16_QAM,Coding_3_4,4,468,16,5616,4212,2,800,1053.0},
	{6,5,-57,Modulation_64_QAM,Coding_2_3,6,468,16,8424,5616,3,800,1404.0},
	{7,6,-56,Modulation_64_QAM,Coding_3_4,6,468,16,8424,6318,3,800,1579.5},
	{8,7,-55,Modulation_64_QAM,Coding_5_6,6,268,16,8424,7020,4,800,1755.0},
	{9,8,-50,Modulation_256_QAM,Coding_3_4,8,468,16,11232,8424,4,800,2106.0},
	{10,9,-48,Modulation_256_QAM,Coding_5_6,8,0,0,0,0,0,0,-1},//Not valid
	{11,0,-73,Modulation_BPSK,Coding_1_2,1,468,16,1404,702,1,400,195.0},
	{12,1,-70,Modulation_QPSK,Coding_1_2,2,468,16,2808,1404,1,400,390.0},
	{13,2,-68,Modulation_QPSK,Coding_3_4,2,468,16,2808,2106,1,400,585.0},
	{14,3,-65,Modulation_16_QAM,Coding_1_2,4,468,16,5616,2808,2,400,780.0},
	{15,4,-61,Modulation_16_QAM,Coding_3_4,4,468,16,5616,4212,2,400,1170.0},
	{16,5,-57,Modulation_64_QAM,Coding_2_3,6,468,16,8424,5616,3,400,1560.0},
	{17,6,-56,Modulation_64_QAM,Coding_3_4,6,468,16,8424,6318,3,400,1755.0},
	{18,7,-55,Modulation_64_QAM,Coding_5_6,6,268,16,8424,7020,4,400,1950.0},
	{19,8,-50,Modulation_256_QAM,Coding_3_4,8,468,16,11232,8424,4,400,2340.0},
	{20,9,-48,Modulation_256_QAM,Coding_5_6,8,0,0,0,0,0,0,-1},//Not valid

};

static struct stru_802_11_Phy_Parameters_HT struPhyParameters_160MHz_4NSS[20] =
{
	//IEEE802.11-VHT Phy 160 MHz
	{1,0,-73,Modulation_BPSK,Coding_1_2,1,468,16,1872,936,1,800,234.0},
	{2,1,-70,Modulation_QPSK,Coding_1_2,2,468,16,3744,1872,1,800,468.0},
	{3,2,-68,Modulation_QPSK,Coding_3_4,2,468,16,3744,2808,2,800,702.0},
	{4,3,-65,Modulation_16_QAM,Coding_1_2,4,468,16,7488,3744,2,800,936.0},
	{5,4,-61,Modulation_16_QAM,Coding_3_4,4,468,16,7488,5616,3,800,1404.0},
	{6,5,-57,Modulation_64_QAM,Coding_2_3,6,468,16,11232,7488,4,800,1872.0},
	{7,6,-56,Modulation_64_QAM,Coding_3_4,6,468,16,11232,8424,4,800,2106.0},
	{8,7,-55,Modulation_64_QAM,Coding_5_6,6,468,16,11232,9360,6,800,2340.0},
	{9,8,-50,Modulation_256_QAM,Coding_3_4,8,468,16,14976,11232,6,800,2808.0},
	{10,9,-48,Modulation_256_QAM,Coding_5_6,8,468,16,14976,12480,6,800,3120.0},
	{11,0,-73,Modulation_BPSK,Coding_1_2,1,468,16,1872,936,1,400,260.0},
	{12,1,-70,Modulation_QPSK,Coding_1_2,2,468,16,3744,1872,1,400,520.0},
	{13,2,-68,Modulation_QPSK,Coding_3_4,2,468,16,3744,2808,2,400,780.0},
	{14,3,-65,Modulation_16_QAM,Coding_1_2,4,468,16,7488,3744,2,400,1040.0},
	{15,4,-61,Modulation_16_QAM,Coding_3_4,4,468,16,7488,5616,3,400,1560.0},
	{16,5,-57,Modulation_64_QAM,Coding_2_3,6,468,16,11232,7488,4,400,2080.0},
	{17,6,-56,Modulation_64_QAM,Coding_3_4,6,468,16,11232,8424,4,400,2340.0},
	{18,7,-55,Modulation_64_QAM,Coding_5_6,6,468,16,11232,9360,6,400,2600.0},
	{19,8,-50,Modulation_256_QAM,Coding_3_4,8,468,16,14976,11232,6,400,3120.0},
	{20,9,-48,Modulation_256_QAM,Coding_5_6,8,468,16,14976,12480,6,400,3466.7},

};

static struct stru_802_11_Phy_Parameters_HT struPhyParameters_160MHz_5NSS[20] =
{
	//IEEE802.11-VHT Phy 160 MHz
	{1,0,-73,Modulation_BPSK,Coding_1_2,1,468,16,2340,1170,1,800,292.5},
	{2,1,-70,Modulation_QPSK,Coding_1_2,2,468,16,4680,2340,2,800,585.0},
	{3,2,-68,Modulation_QPSK,Coding_3_4,2,468,16,4680,3510,2,800,877.5},
	{4,3,-65,Modulation_16_QAM,Coding_1_2,4,468,16,9360,4680,3,800,1170.0},
	{5,4,-61,Modulation_16_QAM,Coding_3_4,4,468,16,9360,7020,4,800,1755.0},
	{6,5,-57,Modulation_64_QAM,Coding_2_3,6,468,16,14040,9360,5,800,2340.0},
	{7,6,-56,Modulation_64_QAM,Coding_3_4,6,468,16,14040,10530,5,800,2632.5},
	{8,7,-55,Modulation_64_QAM,Coding_5_6,6,468,16,14040,11700,6,800,2925.0},
	{9,8,-50,Modulation_256_QAM,Coding_3_4,8,468,16,18720,14040,8,800,3510.0},
	{10,9,-48,Modulation_256_QAM,Coding_5_6,8,468,16,18720,15600,8,800,3900.0},
	{11,0,-73,Modulation_BPSK,Coding_1_2,1,468,16,2340,1170,1,400,325.0},
	{12,1,-70,Modulation_QPSK,Coding_1_2,2,468,16,4680,2340,2,400,650.0},
	{13,2,-68,Modulation_QPSK,Coding_3_4,2,468,16,4680,3510,2,400,975.0},
	{14,3,-65,Modulation_16_QAM,Coding_1_2,4,468,16,9360,4680,3,400,1300.0},
	{15,4,-61,Modulation_16_QAM,Coding_3_4,4,468,16,9360,7020,4,400,1950.0},
	{16,5,-57,Modulation_64_QAM,Coding_2_3,6,468,16,14040,9360,5,400,2600.0},
	{17,6,-56,Modulation_64_QAM,Coding_3_4,6,468,16,14040,10530,5,400,2925.0},
	{18,7,-55,Modulation_64_QAM,Coding_5_6,6,468,16,14040,11700,6,400,3250.0},
	{19,8,-50,Modulation_256_QAM,Coding_3_4,8,468,16,18720,14040,8,400,3900.0},
	{20,9,-48,Modulation_256_QAM,Coding_5_6,8,468,16,18720,15600,8,400,4333.3},

};


static struct stru_802_11_Phy_Parameters_HT struPhyParameters_160MHz_6NSS[20] =
{
	//IEEE802.11-VHT Phy 160 MHz
	{1,0,-73,Modulation_BPSK,Coding_1_2,1,468,16,2808,1404,1,800,351.0},
	{2,1,-70,Modulation_QPSK,Coding_1_2,2,468,16,5616,2808,2,800,702.0},
	{3,2,-68,Modulation_QPSK,Coding_3_4,2,468,16,5616,4212,2,800,1053.0},
	{4,3,-65,Modulation_16_QAM,Coding_1_2,4,468,16,11232,5616,3,800,1404.0},
	{5,4,-61,Modulation_16_QAM,Coding_3_4,4,468,16,11232,8424,4,800,2106.0},
	{6,5,-57,Modulation_64_QAM,Coding_2_3,6,468,16,16848,11232,6,800,2808.0},
	{7,6,-56,Modulation_64_QAM,Coding_3_4,6,468,16,16848,12636,6,800,3159.0},
	{8,7,-55,Modulation_64_QAM,Coding_5_6,6,468,16,16848,14040,8,800,3510.0},
	{9,8,-50,Modulation_256_QAM,Coding_3_4,8,468,16,22464,16848,8,800,4212.0},
	{10,9,-48,Modulation_256_QAM,Coding_5_6,8,468,16,22464,18720,9,800,4680.0},
	{11,0,-73,Modulation_BPSK,Coding_1_2,1,468,16,2808,1404,1,400,390.0},
	{12,1,-70,Modulation_QPSK,Coding_1_2,2,468,16,5616,2808,2,400,780.0},
	{13,2,-68,Modulation_QPSK,Coding_3_4,2,468,16,5616,4212,2,400,1170.0},
	{14,3,-65,Modulation_16_QAM,Coding_1_2,4,468,16,11232,5616,3,400,1560.0},
	{15,4,-61,Modulation_16_QAM,Coding_3_4,4,468,16,11232,8424,4,400,2340.0},
	{16,5,-57,Modulation_64_QAM,Coding_2_3,6,468,16,16848,11232,6,400,3120.0},
	{17,6,-56,Modulation_64_QAM,Coding_3_4,6,468,16,16848,12636,6,400,3510.0},
	{18,7,-55,Modulation_64_QAM,Coding_5_6,6,468,16,16848,14040,8,400,3900.0},
	{19,8,-50,Modulation_256_QAM,Coding_3_4,8,468,16,22464,16848,8,400,4680.0},
	{20,9,-48,Modulation_256_QAM,Coding_5_6,8,468,16,22464,18720,9,400,5200.0},

};

static struct stru_802_11_Phy_Parameters_HT struPhyParameters_160MHz_7NSS[20] =
{
	//IEEE802.11-VHT Phy 160 MHz
	{1,0,-73,Modulation_BPSK,Coding_1_2,1,468,16,3276,1638,1,800,409.5},
	{2,1,-70,Modulation_QPSK,Coding_1_2,2,468,16,6552,3276,2,800,819.0},
	{3,2,-68,Modulation_QPSK,Coding_3_4,2,468,16,6552,4914,3,800,1228.5},
	{4,3,-65,Modulation_16_QAM,Coding_1_2,4,468,16,13104,6552,4,800,1638.0},
	{5,4,-61,Modulation_16_QAM,Coding_3_4,4,468,16,13104,9828,6,800,2457.0},
	{6,5,-57,Modulation_64_QAM,Coding_2_3,6,468,16,19656,13104,7,800,3276.0},
	{7,6,-56,Modulation_64_QAM,Coding_3_4,6,468,16,19656,14742,7,800,3685.5},
	{8,7,-55,Modulation_64_QAM,Coding_5_6,6,468,16,19656,16380,9,800,4095.0},
	{9,8,-50,Modulation_256_QAM,Coding_3_4,8,468,16,26208,19656,12,800,4914.0},
	{10,9,-48,Modulation_256_QAM,Coding_5_6,8,468,16,26208,21840,12,800,5460.0},
	{11,0,-73,Modulation_BPSK,Coding_1_2,1,468,16,3276,1638,1,400,455.0},
	{12,1,-70,Modulation_QPSK,Coding_1_2,2,468,16,6552,3276,2,400,910.0},
	{13,2,-68,Modulation_QPSK,Coding_3_4,2,468,16,6552,4914,3,400,1365.0},
	{14,3,-65,Modulation_16_QAM,Coding_1_2,4,468,16,13104,6552,4,400,1820.0},
	{15,4,-61,Modulation_16_QAM,Coding_3_4,4,468,16,13104,9828,6,400,2730.0},
	{16,5,-57,Modulation_64_QAM,Coding_2_3,6,468,16,19656,13104,7,400,3640.0},
	{17,6,-56,Modulation_64_QAM,Coding_3_4,6,468,16,19656,14742,7,400,4095.0},
	{18,7,-55,Modulation_64_QAM,Coding_5_6,6,468,16,19656,16380,9,400,4550.0},
	{19,8,-50,Modulation_256_QAM,Coding_3_4,8,468,16,26208,19656,12,400,5460.0},
	{20,9,-48,Modulation_256_QAM,Coding_5_6,8,468,16,26208,21840,12,400,6066.7},

};

static struct stru_802_11_Phy_Parameters_HT struPhyParameters_160MHz_8NSS[20] =
{
	//IEEE802.11-VHT Phy 160 MHz
	{1,0,-73,Modulation_BPSK,Coding_1_2,1,468,16,3744,1872,1,800,468.0},
	{2,1,-70,Modulation_QPSK,Coding_1_2,2,468,16,7488,3744,2,800,936.0},
	{3,2,-68,Modulation_QPSK,Coding_3_4,2,468,16,7488,5616,3,800,1404.0},
	{4,3,-65,Modulation_16_QAM,Coding_1_2,4,468,16,14976,7488,4,800,1872.0},
	{5,4,-61,Modulation_16_QAM,Coding_3_4,4,468,16,14976,11232,6,800,2808.0},
	{6,5,-57,Modulation_64_QAM,Coding_2_3,6,468,16,22464,14976,8,800,3744.0},
	{7,6,-56,Modulation_64_QAM,Coding_3_4,6,468,16,22464,16848,8,800,4212.0},
	{8,7,-55,Modulation_64_QAM,Coding_5_6,6,468,16,22464,18720,9,800,4680.0},
	{9,8,-50,Modulation_256_QAM,Coding_3_4,8,468,16,29952,22464,12,800,5616.0},
	{10,9,-48,Modulation_256_QAM,Coding_5_6,8,468,16,29952,24960,12,800,6240.0},
	{11,0,-73,Modulation_BPSK,Coding_1_2,1,468,16,3744,1872,1,400,520.0},
	{12,1,-70,Modulation_QPSK,Coding_1_2,2,468,16,7488,3744,2,400,1040.0},
	{13,2,-68,Modulation_QPSK,Coding_3_4,2,468,16,7488,5616,3,400,1560.0},
	{14,3,-65,Modulation_16_QAM,Coding_1_2,4,468,16,14976,7488,4,400,2080.0},
	{15,4,-61,Modulation_16_QAM,Coding_3_4,4,468,16,14976,11232,6,400,3120.0},
	{16,5,-57,Modulation_64_QAM,Coding_2_3,6,468,16,22464,14976,8,400,4160.0},
	{17,6,-56,Modulation_64_QAM,Coding_3_4,6,468,16,22464,16848,8,400,4680.0},
	{18,7,-55,Modulation_64_QAM,Coding_5_6,6,468,16,22464,18720,9,400,5200.0},
	{19,8,-50,Modulation_256_QAM,Coding_3_4,8,468,16,29952,22464,12,4200,6240.0},
	{20,9,-48,Modulation_256_QAM,Coding_5_6,8,468,16,29952,24960,12,4200,6933.3},

};


// Done till here ..................... 
#define MAX_RATE_INDEX_11n_800 7
#define MAX_RATE_INDEX_11n_400 17
#define MAX_RATE_INDEX_11ac_800 9
#define MAX_RATE_INDEX_11ac_400 19
#define MIN_RATE_INDEX_800 0
#define MIN_RATE_INDEX_400 10

#define get_max_index_n(phy) phy->nGuardInterval==400?MAX_RATE_INDEX_11n_400:MAX_RATE_INDEX_11n_800
#define get_max_index_ac(phy) phy->nGuardInterval==400?MAX_RATE_INDEX_11ac_400:MAX_RATE_INDEX_11ac_800
#define get_max_index(phy) phy->PhyProtocol==IEEE_802_11n?get_max_index_n(phy):get_max_index_ac(phy)
//#define MIN_RATE_INDEX pstruPhy->NSS==400?MIN_RATE_INDEX_400:MIN_RATE_INDEX_800;

/*
20.3.21.1 Receiver minimum input sensitivity
The packet error ratio (PER) shall be less than 10% for a PSDU length of 4096 octets
*/
#define TARGET_PEP 0.1
#define REF_PACKET_SIZE 4096 //Bytes

unsigned int get_ht_phy_max_index(int bandwidth,IEEE802_11_PROTOCOL protocol,UINT dGI)
{
	switch(protocol)
	{
	case IEEE_802_11n:
		if(dGI==400)
			return MAX_RATE_INDEX_11n_400;
		else if(dGI==800)
			return  MAX_RATE_INDEX_11n_800;
		else
			fnNetSimError("Unknown Guard Interval %d in %s.",dGI,__FUNCTION__);
		return 0;
	case IEEE_802_11ac:
		if(dGI==400)
			return MAX_RATE_INDEX_11ac_400;
		else if(dGI==800)
			return  MAX_RATE_INDEX_11ac_800;
		else
			fnNetSimError("Unknown Guard Interval %d in %s.",dGI,__FUNCTION__);
		return 0;
	default:
		fnNetSimError("Unknown phy protocol %d in %s.",protocol,__FUNCTION__);
		return 0;
	}
}

unsigned int get_ht_phy_min_index(int bandwidth,IEEE802_11_PROTOCOL protocol,UINT dGI)
{
	switch(protocol)
	{
	case IEEE_802_11n:
		if(dGI==400)
			return MIN_RATE_INDEX_400;
		else if(dGI==800)
			return MIN_RATE_INDEX_800;
		else
			fnNetSimError("Unknown Guard Interval %d in %s.",dGI,__FUNCTION__);
		return 0;
	case IEEE_802_11ac:
		if(dGI==400)
			return MIN_RATE_INDEX_400;
		else if(dGI==800)
			return MIN_RATE_INDEX_800;
		else
			fnNetSimError("Unknown Guard Interval %d in %s.",dGI,__FUNCTION__);
		return 0;
	default:
		fnNetSimError("Unknown phy protocol %d in %s.",protocol,__FUNCTION__);
		return 0;
	}
}

struct stru_802_11_Phy_Parameters_HT* get_phy_parameter_HT(double dChannelBandwidth,UINT NSS)
{
	switch((int)dChannelBandwidth)
	{
	case 20:
		switch(NSS)
		{
		case 1:
			return struPhyParameters_20MHz_1NSS;
		case 2:
			return struPhyParameters_20MHz_2NSS;
		case 3:
			return struPhyParameters_20MHz_3NSS;
		case 4:
			return struPhyParameters_20MHz_4NSS;
		case 5:
			return struPhyParameters_20MHz_5NSS;
		case 6:
			return struPhyParameters_20MHz_6NSS;
		case 7:
			return struPhyParameters_20MHz_7NSS;
		case 8:
			return struPhyParameters_20MHz_8NSS;
		default:
			fnNetSimError("Unknown No: of spatial streams %d in %s\n",NSS,__FUNCTION__);
			return NULL;

		}

	case 40:
		switch(NSS)
		{
		case 1:
			return struPhyParameters_40MHz_1NSS;
		case 2:
			return struPhyParameters_40MHz_2NSS;
		case 3:
			return struPhyParameters_40MHz_3NSS;
		case 4:
			return struPhyParameters_40MHz_4NSS;
		case 5:
			return struPhyParameters_40MHz_5NSS;
		case 6:
			return struPhyParameters_40MHz_6NSS;
		case 7:
			return struPhyParameters_40MHz_7NSS;
		case 8:
			return struPhyParameters_40MHz_8NSS;
		default:
			fnNetSimError("Unknown No: of spatial streams %d in %s\n",NSS,__FUNCTION__);
			return NULL;

		}
	case 80:
		switch(NSS)
		{
		case 1:
			return struPhyParameters_80MHz_1NSS;
		case 2:
			return struPhyParameters_80MHz_2NSS;
		case 3:
			return struPhyParameters_80MHz_3NSS;
		case 4:
			return struPhyParameters_80MHz_4NSS;
		case 5:
			return struPhyParameters_80MHz_5NSS;
		case 6:
			return struPhyParameters_80MHz_6NSS;
		case 7:
			return struPhyParameters_80MHz_7NSS;
		case 8:
			return struPhyParameters_80MHz_8NSS;
		default:
			fnNetSimError("Unknown No: of spatial streams %d in %s\n",NSS,__FUNCTION__);
			return NULL;

		}
	case 160:
		switch(NSS)
		{
		case 1:
			return struPhyParameters_160MHz_1NSS;
		case 2:
			return struPhyParameters_160MHz_2NSS;
		case 3:
			return struPhyParameters_160MHz_3NSS;
		case 4:
			return struPhyParameters_160MHz_4NSS;
		case 5:
			return struPhyParameters_160MHz_5NSS;
		case 6:
			return struPhyParameters_160MHz_6NSS;
		case 7:
			return struPhyParameters_160MHz_7NSS;
		case 8:
			return struPhyParameters_160MHz_8NSS;
		default:
			fnNetSimError("Unknown No: of spatial streams %d in %s\n",NSS,__FUNCTION__);
			return NULL;

		}
	default:
		fnNetSimError("Unknown channel bandwidth %lf in %s\n",dChannelBandwidth,__FUNCTION__);
		return NULL;
	}
}

int fn_NetSim_IEEE802_11_HTPhy_UpdateParameter()
{
	int b;
	int n;
	int i;
	for (b = 20; b <= 160; b*=2)
	{
		for (n = 1; n <= 8; n++)
		{
			struct stru_802_11_Phy_Parameters_HT* pht = get_phy_parameter_HT((double)b, n);
			for (i = 0; i < 20; i++)
				pht[i].dRxSensitivity = propagation_calculateRXSensitivity(TARGET_PEP,
																		   REF_PACKET_SIZE,
																		   pht[i].nModulation,
																		   (double)b);
		}
	}
	return 0;
}

void fn_NetSim_IEEE802_11_HTPhy_SetEDThreshold(PIEEE802_11_PHY_VAR phy)
{
	struct stru_802_11_Phy_Parameters_HT *struPhyParameters = get_phy_parameter_HT(phy->dChannelBandwidth, phy->NSS);
	int MIN_RATE_INDEX = phy->nGuardInterval == 400 ? MIN_RATE_INDEX_400 : MIN_RATE_INDEX_800;
	phy->dEDThreshold = struPhyParameters[MIN_RATE_INDEX].dRxSensitivity + CSRANGEDIFF;
}

/**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
This function is used to set the data rate for a frame. Using this we are 
calculating the transmission time.
Data rate set based on the received power.
Select the receiver sensitivity as per below table assigned 
in PMD_RX_Specification_802_11abgn.c
page 1745 Table 20-23—Receiver minimum input level sensitivity  802.11-2012.pdf
Page 319 Table 20-22—Receiver minimum input level sensitivity 802.11n-2009.pdf 
These Data rates are as per table
page 345 Table 20-29 to  Table 20-36 802.11n-2009.pdf 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/
int fn_NetSim_IEEE802_11_HTPhy_DataRate(NETSIM_ID nDeviceId, NETSIM_ID nInterfaceId, NETSIM_ID nReceiverId,NetSim_PACKET* packet,double time)
{
	int mcs=0;
	PIEEE802_11_MAC_VAR mac = IEEE802_11_MAC(nDeviceId,nInterfaceId);
	PIEEE802_11_PHY_VAR pstruPhy = IEEE802_11_PHY(nDeviceId,nInterfaceId);
	struct stru_802_11_Phy_Parameters_HT *struPhyParameters = get_phy_parameter_HT(pstruPhy->dChannelBandwidth,pstruPhy->NSS);
	int MIN_RATE_INDEX = pstruPhy->nGuardInterval==400?MIN_RATE_INDEX_400:MIN_RATE_INDEX_800;

	if(mac->rate_adaptationAlgo==GENERIC)
	{
		unsigned int index = get_rate_index(nDeviceId,nInterfaceId,nReceiverId);
		if(isIEEE802_11_CtrlPacket(packet))
			goto CONTROL_RATE;
		if(struPhyParameters[index].dDataRate<=0)
			index--;
		if(nReceiverId)
		{
			pstruPhy->PHY_TYPE.ofdmPhy_11n.dDataRate = struPhyParameters[index].dDataRate;
			pstruPhy->dCurrentRxSensitivity_dbm = struPhyParameters[index].dRxSensitivity;
			pstruPhy->PHY_TYPE.ofdmPhy_11n.modulation = struPhyParameters[index].nModulation;
			pstruPhy->PHY_TYPE.ofdmPhy_11n.coding = struPhyParameters[index].nCodingRate;
			pstruPhy->PHY_TYPE.ofdmPhy_11n.nNBPSC = struPhyParameters[index].nNBPSC;
			mcs = struPhyParameters[index].MCS;
		}
		else // Broadcast packets
		{
			goto BROADCAST_RATE;
		}
	}
	else if(mac->rate_adaptationAlgo==MINSTREL)
	{
		struct stru_802_11_Phy_Parameters_HT struPhyParametersi = getTxRate(nDeviceId,nInterfaceId,nReceiverId);
		if(isIEEE802_11_CtrlPacket(packet))
			goto CONTROL_RATE;
		if(struPhyParametersi.dDataRate<=0)
			fnNetSimError("Data rate <= 0 in Minstrel.\n");
		if(nReceiverId)
		{
			pstruPhy->PHY_TYPE.ofdmPhy_11n.dDataRate = struPhyParametersi.dDataRate;
			pstruPhy->dCurrentRxSensitivity_dbm = struPhyParametersi.dRxSensitivity;
			pstruPhy->PHY_TYPE.ofdmPhy_11n.modulation = struPhyParametersi.nModulation;
			pstruPhy->PHY_TYPE.ofdmPhy_11n.coding = struPhyParametersi.nCodingRate;
			pstruPhy->PHY_TYPE.ofdmPhy_11n.nNBPSC = struPhyParametersi.nNBPSC;
			mcs = struPhyParametersi.MCS;
			pstruPhy->dControlFrameDataRate = struPhyParameters[MIN_RATE_INDEX].dDataRate;
		}
		else // Broadcast packets
		{
			goto BROADCAST_RATE;
		}
	}
	else
	{
		if(nReceiverId)
		{
			if(isIEEE802_11_CtrlPacket(packet))
			{
CONTROL_RATE:
				pstruPhy->PHY_TYPE.ofdmPhy_11n.dDataRate = struPhyParameters[MIN_RATE_INDEX].dDataRate;
				pstruPhy->dCurrentRxSensitivity_dbm = struPhyParameters[MIN_RATE_INDEX].dRxSensitivity;
				pstruPhy->PHY_TYPE.ofdmPhy_11n.modulation = struPhyParameters[MIN_RATE_INDEX].nModulation;
				pstruPhy->PHY_TYPE.ofdmPhy_11n.coding = struPhyParameters[MIN_RATE_INDEX].nCodingRate;
				pstruPhy->PHY_TYPE.ofdmPhy_11n.nNBPSC = struPhyParameters[MIN_RATE_INDEX].nNBPSC;
				mcs = struPhyParameters[MIN_RATE_INDEX].MCS;
				pstruPhy->dControlFrameDataRate = struPhyParameters[MIN_RATE_INDEX].dDataRate;
			}
			else
			{				
				NETSIM_ID in = fn_NetSim_Stack_GetConnectedInterface(nDeviceId, nInterfaceId, nReceiverId);

				double power = GET_RX_POWER_dbm(nDeviceId, nInterfaceId, nReceiverId, in, time);
				int i;
				for(i=get_max_index(pstruPhy);i>=MIN_RATE_INDEX;i--)
				{
					if(struPhyParameters[i].dDataRate<=0)
						continue; //Invalid 
					if(power >= struPhyParameters[i].dRxSensitivity || i==MIN_RATE_INDEX)
					{
						pstruPhy->PHY_TYPE.ofdmPhy_11n.dDataRate = struPhyParameters[i].dDataRate;
						pstruPhy->dCurrentRxSensitivity_dbm = struPhyParameters[i].dRxSensitivity;
						pstruPhy->PHY_TYPE.ofdmPhy_11n.modulation = struPhyParameters[i].nModulation;
						pstruPhy->PHY_TYPE.ofdmPhy_11n.coding = struPhyParameters[i].nCodingRate;
						pstruPhy->PHY_TYPE.ofdmPhy_11n.nNBPSC = struPhyParameters[i].nNBPSC;
						mcs = struPhyParameters[i].MCS;
						pstruPhy->dControlFrameDataRate = struPhyParameters[MIN_RATE_INDEX].dDataRate;
						break;
					}
				}
			}
		}
		else // Broadcast packets
		{
BROADCAST_RATE:
			pstruPhy->PHY_TYPE.ofdmPhy_11n.dDataRate = struPhyParameters[MIN_RATE_INDEX].dDataRate;
			pstruPhy->dCurrentRxSensitivity_dbm = struPhyParameters[MIN_RATE_INDEX].dRxSensitivity;
			pstruPhy->PHY_TYPE.ofdmPhy_11n.modulation = struPhyParameters[MIN_RATE_INDEX].nModulation;
			pstruPhy->PHY_TYPE.ofdmPhy_11n.coding = struPhyParameters[MIN_RATE_INDEX].nCodingRate;
			pstruPhy->PHY_TYPE.ofdmPhy_11n.nNBPSC = struPhyParameters[MIN_RATE_INDEX].nNBPSC;
			mcs = struPhyParameters[MIN_RATE_INDEX].MCS;
			pstruPhy->dControlFrameDataRate = struPhyParameters[MIN_RATE_INDEX].dDataRate;
		}
	}
	return 0;
}

double fn_NetSim_IEEE802_11_HTPhy_getCtrlFrameDataRate(PIEEE802_11_PHY_VAR pstruPhy)
{
	struct stru_802_11_Phy_Parameters_HT *struPhyParameters = get_phy_parameter_HT(pstruPhy->dChannelBandwidth,pstruPhy->NSS);
	int MIN_RATE_INDEX = pstruPhy->nGuardInterval==400?MIN_RATE_INDEX_400:MIN_RATE_INDEX_800;
	return struPhyParameters[MIN_RATE_INDEX].dDataRate;
}
