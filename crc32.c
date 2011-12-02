/*
 * crc32.c
 */
#include "crc32.h"
#include <stdlib.h>
/* Return a 32-bit CRC of the contents of the buffer. */
unsigned long crc32(char * s, unsigned int len)
{
  unsigned int i;
  unsigned long crc32val;
//  unsigned char * s = (unsigned char *) malloc (len);
//  s = & ss;
  crc32val = 0;
  for (i = 0;  i < len;  i ++)
    {
      crc32val =
	crc32_tab[(crc32val ^ s[i]) & 0xff] ^
	  (crc32val >> 8);
    }
  return crc32val;
}
