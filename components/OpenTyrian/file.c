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
#include "file.h"

#include "inflate.h"
#include "opentyr.h"
#include "tyrian_assets.h"
#include "varz.h"

#include <ctype.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

const char *custom_data_dir = NULL;

/*
 * Handles are a fixed pool.  The most the game holds open at once is the
 * music file, which stays open for the whole run, plus a level, its episode
 * script and its tile set while a level loads.
 */
#define VFS_MAX_OPEN 8

struct VFILE
{
	const struct ty_asset *asset;  // NULL when the slot is free
	uint32_t pos;
	bool eof;                      // a read ran off the end, as feof() means it
};

static VFILE open_files[VFS_MAX_OPEN];

/*
 * Decoded blocks of the compressed files, shared by every handle.
 *
 * Three, because a level load interleaves reads from the level file and the
 * episode script, and the music file can be read in the middle of either.
 * A handle reading forward uses one slot; the others keep its neighbours from
 * evicting it.  12 KB in all, against the megabyte and a half these files
 * decompress to.
 */
#define VFS_CACHE_SLOTS 3

static struct
{
	const struct ty_asset *asset;
	uint32_t block;
	uint32_t used;                 // for least-recently-used replacement
	uint8_t data[TY_ASSET_BLOCK];
} cache[VFS_CACHE_SLOTS];

static uint32_t cache_clock;

static void vfs_die( const char *what, const VFILE *f )
{
	fprintf(stderr, "error: %s: %s\n", f && f->asset ? f->asset->name : "(no file)", what);
	JE_tyrianHalt(1);
}

static const uint8_t *cached_block( const struct ty_asset *a, uint32_t block )
{
	unsigned victim = 0;

	for (unsigned i = 0; i < VFS_CACHE_SLOTS; ++i)
	{
		if (cache[i].asset == a && cache[i].block == block)
		{
			cache[i].used = ++cache_clock;
			return cache[i].data;
		}
		if (cache[i].used < cache[victim].used)
			victim = i;
	}

	uint32_t want = a->size - block * TY_ASSET_BLOCK;
	if (want > TY_ASSET_BLOCK)
		want = TY_ASSET_BLOCK;

	long got = inflate_raw(cache[victim].data, TY_ASSET_BLOCK,
	                       a->data + a->offsets[block],
	                       a->offsets[block + 1] - a->offsets[block]);
	if (got != (long)want)
	{
		// Flash is not supposed to change under us; if it has, nothing read
		// from this file can be trusted.
		cache[victim].asset = NULL;
		fprintf(stderr, "error: %s: block %u is corrupt\n", a->name, (unsigned)block);
		JE_tyrianHalt(1);
	}

	cache[victim].asset = a;
	cache[victim].block = block;
	cache[victim].used = ++cache_clock;
	return cache[victim].data;
}

static const struct ty_asset *find_asset( const char *file )
{
	char name[32];
	size_t i;

	for (i = 0; file[i] != '\0' && i < sizeof(name) - 1; ++i)
		name[i] = tolower((unsigned char)file[i]);
	name[i] = '\0';

	unsigned lo = 0, hi = ty_asset_count;
	while (lo < hi)
	{
		unsigned mid = (lo + hi) / 2;
		int cmp = strcmp(name, ty_assets[mid].name);
		if (cmp == 0)
			return &ty_assets[mid];
		if (cmp < 0)
			hi = mid;
		else
			lo = mid + 1;
	}
	return NULL;
}

// the data is wherever the table says it is
const char *data_dir( void )
{
	return "";
}

// the directory is ignored: there is one namespace, and it is the table
VFILE *dir_fopen( const char *dir, const char *file, const char *mode )
{
	(void)dir;

	if (strpbrk(mode, "wa+") != NULL)
	{
		errno = EROFS;
		return NULL;
	}

	const struct ty_asset *a = find_asset(file);
	if (a == NULL)
	{
		errno = ENOENT;
		return NULL;
	}

	for (unsigned i = 0; i < VFS_MAX_OPEN; ++i)
	{
		if (open_files[i].asset == NULL)
		{
			open_files[i].asset = a;
			open_files[i].pos = 0;
			open_files[i].eof = false;
			return &open_files[i];
		}
	}

	fprintf(stderr, "error: more than %d files open at once opening '%s'\n", VFS_MAX_OPEN, file);
	JE_tyrianHalt(1);
	return NULL;
}

// warn when dir_fopen fails
VFILE *dir_fopen_warn(  const char *dir, const char *file, const char *mode )
{
	VFILE *f = dir_fopen(dir, file, mode);

	if (f == NULL)
		fprintf(stderr, "warning: failed to open '%s': %s\n", file, strerror(errno));

	return f;
}

// die when dir_fopen fails
VFILE *dir_fopen_die( const char *dir, const char *file, const char *mode )
{
	VFILE *f = dir_fopen(dir, file, mode);

	if (f == NULL)
	{
		fprintf(stderr, "error: failed to open '%s': %s\n", file, strerror(errno));
		fprintf(stderr, "error: '%s' was not converted into this firmware - see tools/assets/convert.py\n", file);
		JE_tyrianHalt(1);
	}

	return f;
}

// check if file can be opened for reading
bool dir_file_exists( const char *dir, const char *file )
{
	(void)dir;
	return find_asset(file) != NULL;
}

// returns end-of-file position
long ftell_eof( VFILE *f )
{
	return f->asset->size;
}

const void *vfs_map( VFILE *f, size_t n )
{
	const struct ty_asset *a = f->asset;

	if (a->blocks != 0)
		vfs_die("mapped, but the converter compressed it; it must be stored raw", f);
	if (f->pos > a->size || n > a->size - f->pos)
		vfs_die("mapped past its end", f);

	const void *p = a->data + f->pos;
	f->pos += n;
	return p;
}

size_t vfs_read( void *buffer, size_t size, size_t num, VFILE *f )
{
	const struct ty_asset *a = f->asset;

	if (size == 0 || num == 0)
		return 0;

	size_t avail = f->pos < a->size ? a->size - f->pos : 0;
	size_t want = size * num;
	if (want > avail)
	{
		want = avail - avail % size;  // only whole items, as fread does
		f->eof = true;
	}

	Uint8 *dst = buffer;
	size_t left = want;
	while (left > 0)
	{
		size_t chunk;

		if (a->blocks == 0)
		{
			chunk = left;
			memcpy(dst, a->data + f->pos, chunk);
		}
		else
		{
			uint32_t block = f->pos / TY_ASSET_BLOCK, offset = f->pos % TY_ASSET_BLOCK;
			chunk = TY_ASSET_BLOCK - offset;
			if (chunk > left)
				chunk = left;
			memcpy(dst, cached_block(a, block) + offset, chunk);
		}

		dst += chunk;
		f->pos += chunk;
		left -= chunk;
	}

	return want / size;
}

int efeof ( VFILE * stream )
{
	return stream->eof;
}

int efputc ( int character, VFILE * stream )
{
	(void)character;
	(void)stream;
	return EOF;
}

int efgetc ( VFILE * stream )
{
	Uint8 c;
	return vfs_read(&c, 1, 1, stream) == 1 ? c : EOF;
}

size_t eefwrite ( const void * ptr, size_t size, size_t count, VFILE * stream )
{
	(void)ptr;
	(void)size;
	(void)count;
	(void)stream;
	return 0;
}

int efclose ( VFILE * stream )
{
	if (stream != NULL)
		stream->asset = NULL;
	return 0;
}

long int eftell ( VFILE * stream )
{
	return stream->pos;
}

int efseek( VFILE * stream, long int offset, int origin )
{
	long base;

	switch (origin)
	{
	case SEEK_SET: base = 0;                   break;
	case SEEK_CUR: base = stream->pos;         break;
	case SEEK_END: base = stream->asset->size; break;
	default:       return -1;
	}

	if (base + offset < 0)
		return -1;

	// Past the end is allowed, as fseek allows it; the next read says EOF.
	stream->pos = base + offset;
	stream->eof = false;
	return 0;
}

// fread that dies if the expected amount cannot be read
size_t efread( void *buffer, size_t size, size_t num, VFILE *stream )
{
	size_t num_read = vfs_read(buffer, size, num, stream);

	if (num_read != num)
	{
		fprintf(stderr, "error: An unexpected problem occurred while reading from a file.\n");
		fprintf(stderr, "%s: read %u of %u at offset %ld\n", stream->asset->name,
		        (unsigned)num_read, (unsigned)num, (long)stream->pos);
		JE_tyrianHalt(1);
	}

	return num_read;
}

// nothing can be opened for writing, so nothing can reach here with a file
size_t efwrite( const void *buffer, size_t size, size_t num, VFILE *stream )
{
	(void)buffer;
	(void)size;
	(void)num;
	vfs_die("written, but files are read-only", stream);
	return 0;
}
