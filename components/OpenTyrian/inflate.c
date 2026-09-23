/*
 * OpenTyrian: A modern cross-platform port of Tyrian
 * Copyright (C) 2007-2009  The OpenTyrian Development Team
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
 */

/*
 * A raw deflate (RFC 1951) decoder into a caller's buffer.
 *
 * It exists for the file layer, which stores the game's data in flash as
 * independently compressed 4 KiB blocks.  Each block decodes into a buffer
 * that holds all of it, so the output is its own window: no sliding window,
 * no allocation, and no state between calls.  What it keeps is two canonical
 * Huffman tables on the stack, about 1.3 KB.
 *
 * Decoding is a bit at a time through canonical code counts, which is the
 * slowest correct way to do it and more than fast enough here: blocks are
 * decoded when a level or a picture loads, never while the game is running.
 *
 * Every read and every copy is bounded, so a corrupt stream fails rather than
 * reading past the input or writing past the output.
 */
#include "inflate.h"

#include <string.h>

#define MAXBITS   15
#define MAXLCODES 286
#define MAXDCODES 30
#define FIXLCODES 288

struct state {
	const uint8_t *in;
	size_t in_len, in_pos;
	uint32_t bitbuf;
	int bitcnt;

	uint8_t *out;
	size_t out_len, out_pos;
};

struct huffman {
	uint16_t count[MAXBITS + 1];  /* codes of each length */
	uint16_t symbol[FIXLCODES];   /* symbols in canonical order */
};

/* Returns -1 at the end of the input rather than inventing zeros. */
static int bits( struct state *s, int need )
{
	uint32_t val = s->bitbuf;

	while (s->bitcnt < need)
	{
		if (s->in_pos == s->in_len)
			return -1;
		val |= (uint32_t)s->in[s->in_pos++] << s->bitcnt;
		s->bitcnt += 8;
	}

	s->bitbuf = val >> need;
	s->bitcnt -= need;

	return (int)(val & ((1u << need) - 1));
}

static int stored( struct state *s )
{
	s->bitbuf = 0;
	s->bitcnt = 0;

	if (s->in_pos + 4 > s->in_len)
		return -1;

	unsigned len = s->in[s->in_pos] | (s->in[s->in_pos + 1] << 8);
	unsigned nlen = s->in[s->in_pos + 2] | (s->in[s->in_pos + 3] << 8);
	s->in_pos += 4;

	if (len != (~nlen & 0xffff) || s->in_pos + len > s->in_len || s->out_pos + len > s->out_len)
		return -1;

	memcpy(s->out + s->out_pos, s->in + s->in_pos, len);
	s->in_pos += len;
	s->out_pos += len;

	return 0;
}

/* Codes are sent most significant bit first, but packed into bytes from the
 * least significant end, so they are assembled a bit at a time. */
static int decode( struct state *s, const struct huffman *h )
{
	int code = 0, first = 0, index = 0;

	for (int len = 1; len <= MAXBITS; len++)
	{
		int b = bits(s, 1);
		if (b < 0)
			return -1;
		code |= b;

		int count = h->count[len];
		if (code - count < first)
			return h->symbol[index + (code - first)];

		index += count;
		first += count;
		first <<= 1;
		code <<= 1;
	}

	return -1;
}

/* Builds the table from code lengths.  An over-subscribed set is an error; an
 * incomplete one is allowed, as zlib allows it for the distance codes. */
static int construct( struct huffman *h, const uint8_t *length, int n )
{
	uint16_t offs[MAXBITS + 1];

	memset(h->count, 0, sizeof(h->count));
	for (int symbol = 0; symbol < n; symbol++)
		h->count[length[symbol]]++;

	if (h->count[0] == n)
		return 0;

	int left = 1;
	for (int len = 1; len <= MAXBITS; len++)
	{
		left <<= 1;
		left -= h->count[len];
		if (left < 0)
			return -1;
	}

	offs[1] = 0;
	for (int len = 1; len < MAXBITS; len++)
		offs[len + 1] = offs[len] + h->count[len];

	for (int symbol = 0; symbol < n; symbol++)
		if (length[symbol] != 0)
			h->symbol[offs[length[symbol]]++] = symbol;

	return left;
}

static int codes( struct state *s, const struct huffman *lencode, const struct huffman *distcode )
{
	static const uint16_t lbase[29] = {
		3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31,
		35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258 };
	static const uint8_t lext[29] = {
		0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2,
		3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0 };
	static const uint16_t dbase[30] = {
		1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193,
		257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145,
		8193, 12289, 16385, 24577 };
	static const uint8_t dext[30] = {
		0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6,
		7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13 };

	for (;;)
	{
		int symbol = decode(s, lencode);
		if (symbol < 0)
			return -1;

		if (symbol < 256)
		{
			if (s->out_pos == s->out_len)
				return -1;
			s->out[s->out_pos++] = (uint8_t)symbol;
		}
		else if (symbol == 256)
		{
			return 0;
		}
		else
		{
			symbol -= 257;
			if (symbol >= 29)
				return -1;
			int e = bits(s, lext[symbol]);
			if (e < 0)
				return -1;
			size_t len = lbase[symbol] + e;

			symbol = decode(s, distcode);
			if (symbol < 0 || symbol >= 30)
				return -1;
			e = bits(s, dext[symbol]);
			if (e < 0)
				return -1;
			size_t dist = dbase[symbol] + e;

			if (dist > s->out_pos || s->out_pos + len > s->out_len)
				return -1;

			/* Byte by byte: the source may overlap what is being written,
			 * which is how deflate spells a run. */
			uint8_t *dst = s->out + s->out_pos;
			const uint8_t *src = dst - dist;
			for (size_t i = 0; i < len; i++)
				dst[i] = src[i];
			s->out_pos += len;
		}
	}
}

static int fixed( struct state *s )
{
	static struct huffman lencode, distcode;
	static int built = 0;

	if (!built)
	{
		uint8_t lengths[FIXLCODES];
		int symbol = 0;

		for (; symbol < 144; symbol++) lengths[symbol] = 8;
		for (; symbol < 256; symbol++) lengths[symbol] = 9;
		for (; symbol < 280; symbol++) lengths[symbol] = 7;
		for (; symbol < FIXLCODES; symbol++) lengths[symbol] = 8;
		construct(&lencode, lengths, FIXLCODES);

		for (symbol = 0; symbol < MAXDCODES; symbol++) lengths[symbol] = 5;
		construct(&distcode, lengths, MAXDCODES);

		built = 1;
	}

	return codes(s, &lencode, &distcode);
}

static int dynamic( struct state *s )
{
	static const uint8_t order[19] = {
		16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15 };

	uint8_t lengths[MAXLCODES + MAXDCODES];
	struct huffman lencode, distcode;

	int nlen = bits(s, 5), ndist = bits(s, 5), ncode = bits(s, 4);
	if (nlen < 0 || ndist < 0 || ncode < 0)
		return -1;
	nlen += 257;
	ndist += 1;
	ncode += 4;
	if (nlen > MAXLCODES || ndist > MAXDCODES)
		return -1;

	int index;
	for (index = 0; index < ncode; index++)
	{
		int len = bits(s, 3);
		if (len < 0)
			return -1;
		lengths[order[index]] = (uint8_t)len;
	}
	for (; index < 19; index++)
		lengths[order[index]] = 0;

	if (construct(&lencode, lengths, 19) != 0)
		return -1;

	index = 0;
	while (index < nlen + ndist)
	{
		int symbol = decode(s, &lencode);
		if (symbol < 0)
			return -1;

		if (symbol < 16)
		{
			lengths[index++] = (uint8_t)symbol;
			continue;
		}

		int len = 0, repeat;
		if (symbol == 16)
		{
			if (index == 0)
				return -1;
			len = lengths[index - 1];
			repeat = bits(s, 2);
			if (repeat < 0)
				return -1;
			repeat += 3;
		}
		else if (symbol == 17)
		{
			repeat = bits(s, 3);
			if (repeat < 0)
				return -1;
			repeat += 3;
		}
		else
		{
			repeat = bits(s, 7);
			if (repeat < 0)
				return -1;
			repeat += 11;
		}

		if (index + repeat > nlen + ndist)
			return -1;
		while (repeat--)
			lengths[index++] = (uint8_t)len;
	}

	if (lengths[256] == 0)
		return -1;

	/* Incomplete codes are only allowed when a single length is used. */
	int err = construct(&lencode, lengths, nlen);
	if (err < 0 || (err > 0 && nlen - lencode.count[0] != 1))
		return -1;

	err = construct(&distcode, lengths + nlen, ndist);
	if (err < 0 || (err > 0 && ndist - distcode.count[0] != 1))
		return -1;

	return codes(s, &lencode, &distcode);
}

long inflate_raw( void *out, size_t out_len, const void *in, size_t in_len )
{
	struct state s = {
		.in = in, .in_len = in_len,
		.out = out, .out_len = out_len,
	};

	int last;
	do
	{
		last = bits(&s, 1);
		int type = bits(&s, 2);
		if (last < 0 || type < 0)
			return -1;

		int err;
		switch (type)
		{
		case 0:  err = stored(&s);  break;
		case 1:  err = fixed(&s);   break;
		case 2:  err = dynamic(&s); break;
		default: err = -1;          break;
		}
		if (err != 0)
			return -1;
	}
	while (!last);

	return (long)s.out_pos;
}
