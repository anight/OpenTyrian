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
#ifndef FILE_H
#define FILE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

/*
 * The game's files, served from flash.
 *
 * There is no filesystem.  tools/assets/convert.py compiles the data files into
 * the firmware as a table, and this layer opens them by name and reads them the
 * way the game always read its files - seek, read, getc - so the loaders are
 * unchanged except for the type of the handle.
 *
 * Nothing can be written.  Opening for writing fails the way a read-only
 * filesystem would, and so does opening anything not in the table, which is
 * how the configuration, the saved games and the unconverted episodes all
 * come to be absent.  Every caller already copes with a failed open.
 */
typedef struct VFILE VFILE;

extern const char *custom_data_dir;

const char *data_dir( void );

VFILE *dir_fopen( const char *dir, const char *file, const char *mode );
VFILE *dir_fopen_warn( const char *dir, const char *file, const char *mode );
VFILE *dir_fopen_die( const char *dir, const char *file, const char *mode );

bool dir_file_exists( const char *dir, const char *file );

long ftell_eof( VFILE *f );

/*
 * A pointer to the next n bytes of the file, in flash, and the position moved
 * past them - a read that does not copy.  For the data the game would
 * otherwise hold in RAM for a whole level or the whole run: sprite sheets,
 * level tiles, sound effects.  Only files the converter stored raw can do
 * this, and asking of any other file is a mismatch between the converter and
 * the game, so it halts rather than returning something to check.
 */
const void *vfs_map( VFILE *f, size_t n );

// fread/fwrite that die if the expected amount cannot be read/written
size_t eefwrite ( const void * ptr, size_t size, size_t count, VFILE * stream );
int efeof ( VFILE * stream );
int efputc ( int character, VFILE * stream );
int efgetc ( VFILE * stream );
int efclose ( VFILE * stream );
long int eftell ( VFILE * stream );
int efseek( VFILE * stream, long int offset, int origin );
size_t efread( void *buffer, size_t size, size_t num, VFILE *stream );
size_t efwrite( const void *buffer, size_t size, size_t num, VFILE *stream );

/* fread: as many as there are, without dying at the end. */
size_t vfs_read( void *buffer, size_t size, size_t num, VFILE *stream );

#endif // FILE_H
