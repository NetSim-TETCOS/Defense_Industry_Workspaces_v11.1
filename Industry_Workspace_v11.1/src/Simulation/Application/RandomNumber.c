/************************************************************************************
 * Copyright (C) 2014                                                               *
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

 #ifndef _WIN32
 #include "Linux.h"
 #endif
/** This function is used to generate the random number */
_declspec(dllexport) int fnRandomNo(long lm, double *fRandNo, unsigned long *uSeed,unsigned long *uSeed1)
{
	long ldTemp;
	double fy = 0;
	*uSeed
			= (unsigned long) ((40014 * (*uSeed))
					% (unsigned long) (2147483563));
	*uSeed1 = (unsigned long) ((40692 * (*uSeed1))
			% (unsigned long) (2147483399));
	ldTemp = (long) ((*uSeed - *uSeed1) % (long) (2147483562));
	if (ldTemp != 0) {
		fy = (double) ((double) (ldTemp) / (double) (2147483563));
	} else {
		fy = (double) ((double) (2147483562) / (double) (2147483563));
	}
	*fRandNo = fy;
	return 1;
}


