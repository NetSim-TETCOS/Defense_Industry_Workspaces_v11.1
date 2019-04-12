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
#ifndef _NETSIM_CDMA_H_
#define _NETSIM_CDMA_H_

//Function prototype
_declspec(dllexport) int fn_NetSim_CDMA_Configure_F(void** var);
_declspec(dllexport) int fn_NetSim_CDMA_Init_F(struct stru_NetSim_Network *NETWORK_Formal,
													NetSim_EVENTDETAILS *pstruEventDetails_Formal,
													char *pszAppPath_Formal,
													char *pszWritePath_Formal,
													int nVersion_Type,
													void **fnPointer);
_declspec(dllexport) int fn_NetSim_CDMA_Metrics_F(char* szMetrics);
#endif