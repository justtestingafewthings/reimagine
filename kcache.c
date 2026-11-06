/*-
 * Copyright 2013, winocm <winocm@icloud.com>
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * $Id$
 */

#include <sys/types.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"
#include "kcache.h"

struct compressed_kernel_header
{
	u_int32_t signature;
	u_int32_t compress_type;
	u_int32_t adler32;
	u_int32_t uncompressed_size;
	u_int32_t compressed_size;
	u_int32_t reserved[11];
	char platform_name[64];
	char root_path[256];
	u_int8_t data[0];
};

unsigned long
Adler32 (unsigned char *buffer, long length)
{
	long cnt;
	unsigned long result, lowHalf, highHalf;

	lowHalf = 1;
	highHalf = 0;

	for (cnt = 0; cnt < length; cnt++) {
		if ((cnt % 5000) == 0) {
			lowHalf %= 65521L;
			highHalf %= 65521L;
		}
		lowHalf += buffer[cnt];
		highHalf += lowHalf;
	}

	lowHalf %= 65521L;
	highHalf %= 65521L;

	result = (highHalf << 16) | lowHalf;
	return result;
}

/**************************************************************
 LZSS.C -- A Data Compression Program
***************************************************************
	4/6/1989 Haruhiko Okumura
	Use, distribute, and modify this program freely.
	Please send me your improved versions.
		PC-VAN	  SCIENCE
		NIFTY-Serve PAF01022
		CompuServe  74050,1022

**************************************************************/
#define N		 4096			/* size of ring buffer - must be power of 2 */
#define F		 18				/* upper limit for match_length */
#define THRESHOLD 2				/* encode string into position and length
								 * if match_length is greater than this */
#define NIL	   N				/* index for root of binary search trees */

int
decompress_lzss (uint8_t * dst, uint8_t * src, uint32_t srclen)
{
	/*
	 * ring buffer of size N, with extra F-1 bytes to aid string comparison
	 */
	uint8_t text_buf[N + F - 1];
	uint8_t *dststart = dst;
	uint8_t *srcend = src + srclen;
	int i, j, k, r, c;
	unsigned int flags;

	dst = dststart;
	srcend = src + srclen;
	for (i = 0; i < N - F; i++)
		text_buf[i] = ' ';
	r = N - F;
	flags = 0;
	for (;;) {
		if (((flags >>= 1) & 0x100) == 0) {
			if (src < srcend)
				c = *src++;
			else
				break;
			flags = c | 0xFF00;	/* uses higher byte cleverly */
		}						/* to count eight */
		if (flags & 1) {
			if (src < srcend)
				c = *src++;
			else
				break;
			*dst++ = c;
			text_buf[r++] = c;
			r &= (N - 1);
		}
		else {
			if (src < srcend)
				i = *src++;
			else
				break;
			if (src < srcend)
				j = *src++;
			else
				break;
			i |= ((j & 0xF0) << 4);
			j = (j & 0x0F) + THRESHOLD;
			for (k = 0; k <= j; k++) {
				c = text_buf[(i + k) & (N - 1)];
				*dst++ = c;
				text_buf[r++] = c;
				r &= (N - 1);
			}
		}
	}

	return dst - dststart;
}

int
kcache_decompress_kernel (void *compressedKernel, void *decompressedKernel, int *decompressed_size)
{
	struct compressed_kernel_header *kernel_header = (struct compressed_kernel_header *) compressedKernel;
	u_int32_t uncompressed_size, size;

	if (kernel_header->signature != __builtin_bswap32 ('comp')) {
		printf ("decompress_kernel: bad file magic\n");
		return -1;
	}

	if (kernel_header->signature == __builtin_bswap32 ('comp')) {
		if (kernel_header->compress_type != __builtin_bswap32 ('lzss')) {
			printf ("decompress_kernel: kernel compression is bad\n");
			return -1;
		}
	}

	uncompressed_size = __builtin_bswap32 (kernel_header->uncompressed_size);
	if (!decompressedKernel) {
		*decompressed_size = uncompressed_size;
		return 0;
	}

	size = decompress_lzss ((u_int8_t *) (decompressedKernel), &kernel_header->data[0], __builtin_bswap32 (kernel_header->compressed_size));

	if (uncompressed_size != size) {
		printf ("decompress_kernel: size mismatch from lzss: %x\n", size);
		return -1;
	}

	if (__builtin_bswap32 (kernel_header->adler32) != Adler32 (decompressedKernel, uncompressed_size)) {
		printf ("decompress_kernel: adler mismatch\n");
		return -1;
	}

	printf ("decompress_kernel: kernelcache decompressed to %p\n", decompressedKernel);

	*decompressed_size = size;
	return 0;
}

/*
 * LZSS encoder (Okumura), the inverse of decompress_lzss above. Binary-search
 * tree over the ring buffer; emits the same literal/match bitstream the decoder
 * reads back.
 */
static uint8_t lzss_text_buf[N + F - 1];
static int lzss_match_position, lzss_match_length;
static int lzss_lson[N + 1], lzss_rson[N + 257], lzss_dad[N + 1];

static void
lzss_init_tree (void)
{
	int i;
	for (i = N + 1; i <= N + 256; i++)
		lzss_rson[i] = NIL;
	for (i = 0; i < N; i++)
		lzss_dad[i] = NIL;
}

static void
lzss_insert_node (int r)
{
	int i, p, cmp;
	uint8_t *key;

	cmp = 1;
	key = &lzss_text_buf[r];
	p = N + 1 + key[0];
	lzss_rson[r] = lzss_lson[r] = NIL;
	lzss_match_length = 0;
	for (;;) {
		if (cmp >= 0) {
			if (lzss_rson[p] != NIL)
				p = lzss_rson[p];
			else {
				lzss_rson[p] = r;
				lzss_dad[r] = p;
				return;
			}
		}
		else {
			if (lzss_lson[p] != NIL)
				p = lzss_lson[p];
			else {
				lzss_lson[p] = r;
				lzss_dad[r] = p;
				return;
			}
		}
		for (i = 1; i < F; i++)
			if ((cmp = key[i] - lzss_text_buf[p + i]) != 0)
				break;
		if (i > lzss_match_length) {
			lzss_match_position = p;
			if ((lzss_match_length = i) >= F)
				break;
		}
	}
	lzss_dad[r] = lzss_dad[p];
	lzss_lson[r] = lzss_lson[p];
	lzss_rson[r] = lzss_rson[p];
	lzss_dad[lzss_lson[p]] = r;
	lzss_dad[lzss_rson[p]] = r;
	if (lzss_rson[lzss_dad[p]] == p)
		lzss_rson[lzss_dad[p]] = r;
	else
		lzss_lson[lzss_dad[p]] = r;
	lzss_dad[p] = NIL;
}

static void
lzss_delete_node (int p)
{
	int q;

	if (lzss_dad[p] == NIL)
		return;
	if (lzss_rson[p] == NIL)
		q = lzss_lson[p];
	else if (lzss_lson[p] == NIL)
		q = lzss_rson[p];
	else {
		q = lzss_lson[p];
		if (lzss_rson[q] != NIL) {
			do {
				q = lzss_rson[q];
			} while (lzss_rson[q] != NIL);
			lzss_rson[lzss_dad[q]] = lzss_lson[q];
			lzss_dad[lzss_lson[q]] = lzss_dad[q];
			lzss_lson[q] = lzss_lson[p];
			lzss_dad[lzss_lson[p]] = q;
		}
		lzss_rson[q] = lzss_rson[p];
		lzss_dad[lzss_rson[p]] = q;
	}
	lzss_dad[q] = lzss_dad[p];
	if (lzss_rson[lzss_dad[p]] == p)
		lzss_rson[lzss_dad[p]] = q;
	else
		lzss_lson[lzss_dad[p]] = q;
	lzss_dad[p] = NIL;
}

static uint32_t
compress_lzss (uint8_t * dst, uint8_t * src, uint32_t srclen)
{
	uint8_t *dststart = dst;
	uint8_t *srcend = src + srclen;
	int i, c, len, r, s, last_match_length, code_buf_ptr;
	uint8_t code_buf[17], mask;

	lzss_init_tree ();
	code_buf[0] = 0;
	code_buf_ptr = mask = 1;
	s = 0;
	r = N - F;
	for (i = s; i < r; i++)
		lzss_text_buf[i] = ' ';
	for (len = 0; len < F && src < srcend; len++)
		lzss_text_buf[r + len] = *src++;
	if (len == 0)
		return 0;
	for (i = 1; i <= F; i++)
		lzss_insert_node (r - i);
	lzss_insert_node (r);
	do {
		if (lzss_match_length > len)
			lzss_match_length = len;
		if (lzss_match_length <= THRESHOLD) {
			lzss_match_length = 1;
			code_buf[0] |= mask;
			code_buf[code_buf_ptr++] = lzss_text_buf[r];
		}
		else {
			code_buf[code_buf_ptr++] = (uint8_t) lzss_match_position;
			code_buf[code_buf_ptr++] = (uint8_t)
				(((lzss_match_position >> 4) & 0xf0)
				 | (lzss_match_length - (THRESHOLD + 1)));
		}
		if ((mask <<= 1) == 0) {
			for (i = 0; i < code_buf_ptr; i++)
				*dst++ = code_buf[i];
			code_buf[0] = 0;
			code_buf_ptr = mask = 1;
		}
		last_match_length = lzss_match_length;
		for (i = 0; i < last_match_length && src < srcend; i++) {
			lzss_delete_node (s);
			c = *src++;
			lzss_text_buf[s] = c;
			if (s < F - 1)
				lzss_text_buf[s + N] = c;
			s = (s + 1) & (N - 1);
			r = (r + 1) & (N - 1);
			lzss_insert_node (r);
		}
		while (i++ < last_match_length) {
			lzss_delete_node (s);
			s = (s + 1) & (N - 1);
			r = (r + 1) & (N - 1);
			if (--len)
				lzss_insert_node (r);
		}
	} while (len > 0);
	if (code_buf_ptr > 1) {
		for (i = 0; i < code_buf_ptr; i++)
			*dst++ = code_buf[i];
	}

	return (uint32_t) (dst - dststart);
}

int
kcache_compress_kernel (void *uncompressedKernel, int uncompressed_size, void **compressedKernel, int *compressed_size)
{
	struct compressed_kernel_header *hdr;
	uint8_t *lzss;
	uint32_t clen;
	int total;

	/* lzss never expands by more than ~1/8 (one flag bit per 8 literals) */
	lzss = _xmalloc (uncompressed_size + (uncompressed_size / 8) + 64);
	clen = compress_lzss (lzss, (uint8_t *) uncompressedKernel, uncompressed_size);

	total = sizeof (struct compressed_kernel_header) + clen;
	hdr = _xmalloc (total);
	hdr->signature = __builtin_bswap32 ('comp');
	hdr->compress_type = __builtin_bswap32 ('lzss');
	hdr->adler32 = __builtin_bswap32 (Adler32 (uncompressedKernel, uncompressed_size));
	hdr->uncompressed_size = __builtin_bswap32 (uncompressed_size);
	hdr->compressed_size = __builtin_bswap32 (clen);
	memcpy (hdr->data, lzss, clen);
	free (lzss);

	printf ("compress_kernel: kernelcache compressed to 0x%x bytes\n", clen);

	*compressedKernel = hdr;
	*compressed_size = total;
	return 0;
}
