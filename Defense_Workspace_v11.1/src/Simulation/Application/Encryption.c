/************************************************************************************
 * Copyright (C) 2015                                                               *
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

#include <stdint.h>
//#define _ENCRYPT
#define _XOR_ENCRYPT

char xor_encrypt(char ch,long key)
{
	return (char)(ch^key);
}

/* encrypt
 *   Encrypt 64 bits with a 128 bit key using TEA
 *   From http://en.wikipedia.org/wiki/Tiny_Encryption_Algorithm
 * Arguments:
 *   v - array of two 32 bit uints to be encoded in place
 *   k - array of four 32 bit uints to act as key
 * Returns:
 *   v - encrypted result
 * Side effects:
 *   None
 */
void encrypt (uint32_t* v, uint32_t* k) {
	uint32_t v0=v[0], v1=v[1], sum=0, i;           /* set up */
	uint32_t delta=0x9e3779b9;                     /* a key schedule constant */
	uint32_t k0=k[0], k1=k[1], k2=k[2], k3=k[3];   /* cache key */
	for (i=0; i < 32; i++) {                       /* basic cycle start */
		sum += delta;
		v0 += ((v1<<4) + k0) ^ (v1 + sum) ^ ((v1>>5) + k1);
		v1 += ((v0<<4) + k2) ^ (v0 + sum) ^ ((v0>>5) + k3);  
	}                                              /* end cycle */
	v[0]=v0; v[1]=v1;
}

void encryptBlock(uint8_t * data, uint32_t * len, uint32_t * key)
{
   uint32_t blocks, i;
   uint32_t * data32;

   // treat the data as 32 bit unsigned integers
   data32 = (uint32_t *) data;

   // Find the number of 8 byte blocks, add one for the length
   blocks = (((*len) + 7) / 8) + 1;

   // Set the last block to the original data length
   data32[(blocks*2) - 1] = *len;

   // Set the encrypted data length
   *len = blocks * 8;

   for(i = 0; i< blocks; i++)
   {
	  encrypt(&data32[i*2], key);
   }
}
