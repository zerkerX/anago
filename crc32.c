/*
GZIP file format specification version 4.3
*/
#include "type.h"
#include "crc32.h"
#include "crctable.h"
/*
   Update a running crc with the bytes buf[0..len-1] and return
 the updated crc. The crc should be initialized to zero. Pre- and
 post-conditioning (one's complement) is performed within this
 function so it shouldn't be done by the caller. Usage example:

   unsigned long crc = 0L;

   while (read_buffer(buffer, length) != EOF) {
     crc = update_crc(crc, buffer, length);
   }
   if (crc != original_crc) error();
*/
static uint32_t update_crc(uint32_t crc, const uint8_t *buf, int len)
{
	uint32_t c = crc ^ 0xffffffffUL;
	int n;

	for (n = 0; n < len; n++) {
		c = CRCTABLE[(c ^ buf[n]) & 0xff] ^ (c >> 8);
	}
	return c ^ 0xffffffffUL;
}

/* Return the CRC of the bytes buf[0..len-1]. */
//uint32_t crc(uint8_t *buf, int len) //変数名とかぶるのでかえる
uint32_t crc32_get(const uint8_t *buf, int len)
{
	return update_crc(0UL, buf, len);
}
