#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ENCRYPT 1
#define DECRYPT 0
#define uchar unsigned char
#define uint unsigned int

void key_schedule(uchar key[], uchar schedule[][6], uint mode);
void des_crypt(uchar in[], uchar out[], uchar key[][6]);

void printtext(unsigned char hash[])
{
   int i;
   for (i=0; i < 8; i++)
      printf("%02x ",hash[i]);
   printf("\n");
}

int des(char* buf,int* len)
{
	unsigned char text[8];
   unsigned char key1[8]={0x01,0x23,0x45,0x67,0x89,0xAB,0xCD,0xEF},
         key2[8]={0x13,0x34,0x57,0x79,0x9B,0xBC,0xDF,0xF1},
         three_key1[24]={0x01,0x23,0x45,0x67,0x89,0xAB,0xCD,0xEF,
                         0x01,0x23,0x45,0x67,0x89,0xAB,0xCD,0xEF,
                         0x01,0x23,0x45,0x67,0x89,0xAB,0xCD,0xEF},
         three_key2[24]={0x01,0x23,0x45,0x67,0x89,0xAB,0xCD,0xEF,
                         0xFE,0xDC,0xBA,0x98,0x76,0x54,0x32,0x10,
                         0x01,0x23,0x45,0x67,0x89,0xAB,0xCD,0xEF},
         out[8],schedule[16][6];

   int n;
   int l=*len;
   for(n=0;n<*len;n+=8,buf+=8,l-=8)
   {
	   memcpy(text,buf,min(8,l));
	   key_schedule(key1,schedule,ENCRYPT);
	   des_crypt(text,out,schedule);
	   memcpy(buf,out,min(8,l));
   }
   /*printtext(out);

   key_schedule(key1,schedule,DECRYPT);
   des_crypt(out,text1,schedule);
   printtext(text1);

   key_schedule(key2,schedule,ENCRYPT);
   des_crypt(text2,out,schedule);
   printtext(out);

   key_schedule(key2,schedule,DECRYPT);
   des_crypt(out,text2,schedule);
   printtext(text2);

   three_des_key_schedule(three_key1,three_schedule,ENCRYPT);
   three_des_crypt(text1,out,three_schedule);
   printtext(out);

   three_des_key_schedule(three_key2,three_schedule,ENCRYPT);
   three_des_crypt(text1,out,three_schedule);
   printtext(out);

   getchar();*/
   return 0;
}
