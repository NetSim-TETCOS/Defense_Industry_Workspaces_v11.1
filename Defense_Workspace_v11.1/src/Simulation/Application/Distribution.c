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
 * Author:    Shashi Kant Suman                                                       *
 *                                                                                  *
 * ---------------------------------------------------------------------------------*/
#include "Application.h"
static long double ErrorFun(double p);
/** This function is used to run the distributions */
_declspec(dllexport) int fnDistribution(DISTRIBUTION nDistributionType, double *fDistOut,
		unsigned long *uSeed, unsigned long *uSeed1,double* args)
{
	double fFirstArg = 0.0;
	double fSecondArg = 0.0;
	double fRand = 0.0;
	double fRandomNumber = 0.0;
	int nRandOut = 0;

	switch (nDistributionType)
	{
	case Distribution_Exponential: /*Exponential Distribution Function*/
		fFirstArg = args[0];
		nRandOut = fnRandomNo(10000000, &fRand, uSeed, uSeed1);
		fRandomNumber = (double) (fRand);
		fFirstArg = 1 / fFirstArg;
		*fDistOut = (double) -(1 / fFirstArg)
				* (double) logl(1 - fRandomNumber);
		break;
	case Distribution_Uniform: /*Uniform distribution Function*/
		fSecondArg = args[1];
		fFirstArg = args[0];

		nRandOut = fnRandomNo(10000000, &fRand, uSeed, uSeed1);
		fRandomNumber = (double) (fRand);
		*fDistOut = fFirstArg + (fSecondArg - fFirstArg) * fRandomNumber;
		break;
	case Distribution_Triangular: /*Triangular Distribution Function*/
		fFirstArg = args[0];
		fSecondArg = args[1];
		nRandOut = fnRandomNo(10000000, &fRand, uSeed, uSeed1);
		fRandomNumber = (double) (fRand);
		if (0 <= fRandomNumber && fRandomNumber <= 0.5) {
			*fDistOut = (double) sqrt(2 * (double) fRandomNumber);
		} else if (fRandomNumber > 0.5 && fRandomNumber <= 1) {
			*fDistOut = 2 - (double) sqrt(2 * (1 - (double) fRandomNumber));
		}
		break;
	case Distribution_Weibull: /*Weibull Distribution Function*/
		fFirstArg = args[0];
		fSecondArg = args[1];
		nRandOut = fnRandomNo(10000000, &fRand, uSeed, uSeed1);
		fRandomNumber = (double) (fRand);
		*fDistOut = (fFirstArg * (double) pow((-(double) logl(1 - (1
				- fRandomNumber))), 1 / fSecondArg));
		break;
	case Distribution_Constant: /*Constant Distribution function*/
		fFirstArg = args[0];
		*fDistOut = fFirstArg;
		break;
	case Distribution_Backlog:/*BackLog Distribution. Not used as of NetSim v7 */ 
		*fDistOut = 0.0;
		break;

	case Distribution_Normal:/*Normal Distribution Function on 25-12-2007 by Shashi kant*/
		fFirstArg = args[0]; 
		fSecondArg = args[1];
		nRandOut = fnRandomNo(10000000, &fRand, uSeed, uSeed1);
		fRandomNumber = (double) (fRand);
		*fDistOut = (double) (fFirstArg + fSecondArg * sqrt(2) * ErrorFun(2
				* fRandomNumber - 1));
		break;
	}
	return 1;
}
/** Error inverse Function for Normal Distribution */
static long double ErrorFun(double p)
{
	return (1.0968 * (sqrt(3.1415) * 0.5 * (p + (3.1415 * pow(p, 3)) / 12 + (7
			* pow(3.1415, 2) * pow(p, 5)) / 480 + (127 * pow(3.1415, 3) * pow(
			p, 7)) / 40320)));
}

