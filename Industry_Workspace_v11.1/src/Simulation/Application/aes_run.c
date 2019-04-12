/*
 *  This program demonstrates how to use AES to encrypt/decrypt a 
 *    block (128 bits) of data. To encrypt/decrypt data that are
 *    larger than one block, you need to implement one of the modes of 
 *    AES, such as AES-ECB and AES-CBC.  
 */

#include "aes.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static unsigned char AES_test[16] =
{ 0xF5, 0xBF, 0x8B, 0x37, 0x13, 0x6F, 0x2E, 0x1F,
  0x6B, 0xEC, 0x6F, 0x57, 0x20, 0x21, 0xE3, 0xBA 
};

static unsigned char secret_key[32] =
{ 
    0x4E, 0x46, 0xF8, 0xC5, 0x09, 0x2B, 0x29, 0xE2,
    0x9A, 0x97, 0x1A, 0x0C, 0xD1, 0xF6, 0x10, 0xFB,
    0x1F, 0x67, 0x63, 0xDF, 0x80, 0x7A, 0x7E, 0x70,
    0x96, 0x0D, 0x4C, 0xD3, 0x11, 0x8E, 0x60, 0x1A 
};

    
int aes256( char* str,int* len )
{
    int n;
	int l=*len;
    aes_context ctx;
    unsigned char buf[16];
    unsigned char key[32];

    for (n = 0; n < *len; n+=16,str+=16,l-=16)
    {
       /* Set the plain-text */
       memcpy( buf, str, min(16,l)); 

       /* Set the key */
       memcpy( key, secret_key, 16);
       aes_set_key( &ctx, key, 128 + n * 64);

       aes_encrypt( &ctx, buf, buf );
	   memcpy(str,buf,16);
    }
            
    return( 0 );
}


