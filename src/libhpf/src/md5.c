/* md5.c - MD5 hash algorith implementation
 *
 * MIT X11 license, Copyright (c) 2005 by Senko Rasic <senko@senko.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT
 * SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
 * THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * This implementation is taken from Cyfer crypto library.
 */

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* types and structs */
typedef uint32_t u32;
typedef uint64_t u64;
typedef uint8_t u8;
typedef int bool;

#define false 0
#define true (~0)

typedef struct {
        u32 h1, h2, h3, h4;
        u64 length;
        u8 buffer[2*64];
        size_t buflen;
} MD5_CTX;

/* constants and transformations */

static const u32 iv1 = 0x67452301;
static const u32 iv2 = 0xefcdab89;
static const u32 iv3 = 0x98badcfe;
static const u32 iv4 = 0x10325476;

#define	f(u,v,w) ((u & v) | (~u & w))
#define	g(u,v,w) ((u & w) | (v & ~w))
#define	h(u,v,w) ((u ^ v) ^ w)
#define k(u,v,w) (v ^ (u | ~w))
#define rotl(t,s) ((t << s) | (t >> (32 - s)))

static const u32 y[64] = {
	0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee, 0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
	0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be, 0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
	0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa, 0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
	0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed, 0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
	0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c, 0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
	0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05, 0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
	0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039, 0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
	0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1, 0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391
};

static const int z[64] = {
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
	1, 6, 11, 0, 5, 10, 15, 4, 9, 14, 3, 8, 13, 2, 7, 12,
	5, 8, 11, 14, 1, 4, 7, 10, 13, 0, 3, 6, 9, 12, 15, 2,
	0, 7, 14, 5, 12, 3, 10, 1, 8, 15, 6, 13, 4, 11, 2, 9
};

static const int s[64] = {
	7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
	5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20,
	4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
	6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21
};

/* implementation */

static inline u32 little_load32(const u8 *text)
{
        return *((u32 *) text);
}

static inline void little_store32(u32 val, u8 *text)
{
        *((u32 *) text) = val;
}

static void md5_compress_block(MD5_CTX *ctx, bool last)
{
	u32 x[16];
	u32 a, b, c, d, t;
	size_t j;
	size_t len = ctx->buflen;
	u8 *source = ctx->buffer;

	ctx->length += len * 8; 

	if (last) {
		source[len++] = 128;
		if (len > 56) {
			for (j = len; j < 128; j++) source[j] = 0;
			len = 128;
		} else {
			while (len % 64) source[len++] = 0;
		}
		len -= 8;

		little_store32((u32) ctx->length, source + len); len += 4;
		little_store32((u32) (ctx->length >> 32), source + len); len += 4;
	}
	len /= 64;

	while (len) {
		for (j = 0; j < 16; j++) x[j] = little_load32(source + 4 * j);
		a = ctx->h1; b = ctx->h2; c = ctx->h3; d = ctx->h4;
		for (j = 0; j < 16; j++) {
			t = a + f(b, c, d) + x[z[j]] + y[j];
			a = d; d = c; c = b; b += rotl(t, s[j]);
		}
		for (j = 16; j < 32; j++) {
			t = a + g(b, c, d) + x[z[j]] + y[j];
			a = d; d = c; c = b; b += rotl(t, s[j]);
		}
		for (j = 32; j < 48; j++) {
			t = a + h(b, c, d) + x[z[j]] + y[j];
			a = d; d = c; c = b; b += rotl(t, s[j]);
		}
		for (j = 48; j < 64; j++) {
			t = a + k(b, c, d) + x[z[j]] + y[j];
			a = d; d = c; c = b; b += rotl(t, s[j]);
		}
		ctx->h1 += a; ctx->h2 += b; ctx->h3 += c; ctx->h4 += d;
		len--;
		source += 64;
	}
}

/* interface */

static void CYFER_MD5_Init(MD5_CTX *ctx)
{
	ctx->h1 = iv1; ctx->h2 = iv2; ctx->h3 = iv3; ctx->h4 = iv4;
	ctx->length = 0; ctx->buflen = 0;
}

static void CYFER_MD5_Finish(MD5_CTX *ctx, unsigned char *md)
{
	md5_compress_block(ctx, true);
	little_store32(ctx->h1, md);
	little_store32(ctx->h2, md + 4);
	little_store32(ctx->h3, md + 8);
	little_store32(ctx->h4, md + 12);
}

static void CYFER_MD5_Update(MD5_CTX *ctx, const unsigned char *data, size_t len)
{
	while (len--) {
		ctx->buffer[ctx->buflen++] = *data++;
		if (ctx->buflen == 64) {
			md5_compress_block(ctx, false);
			ctx->buflen = 0;
		}
	}
}

void hpf_md5(const unsigned char *data, size_t len, unsigned char *md)
{
	MD5_CTX ctx;

	CYFER_MD5_Init(&ctx);
	CYFER_MD5_Update(&ctx, data, len);
	CYFER_MD5_Finish(&ctx, md);
}

