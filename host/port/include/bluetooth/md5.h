#include <stdint.h>
#include "types.h"


typedef struct xMD5Context {
	uint32_t buf[4];
	uint32_t bytes[2];
	uint32_t in[16];
} MD5_CTX;

void xMD5Init(MD5_CTX *context);
void xMD5Update(MD5_CTX *context, uint8_t const *buf, int len);
void xMD5Final(uint8_t digest[16], MD5_CTX *context);
void xMD5Transform(uint32_t buf[4], uint32_t const in[16]);

