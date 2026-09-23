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
#ifndef INFLATE_H
#define INFLATE_H

#include <stddef.h>
#include <stdint.h>

/* Decodes a complete raw deflate stream into out, which must hold all of it.
 * Returns the number of bytes written, or -1 if the stream is corrupt,
 * truncated, or would not fit. */
long inflate_raw( void *out, size_t out_len, const void *in, size_t in_len );

#endif /* INFLATE_H */
