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
#ifndef OPL_H
#define OPL_H

/*
 * The OPL2 the music player drives, emulated by DBOPL.
 *
 * DBOPL is C++ and the game is C, so this header is the wall between them:
 * dbopl_adapter.cpp implements these three and nothing C++ leaks through.
 */

#include <stdint.h>

typedef int16_t Bit16s;

#ifdef __cplusplus
extern "C" {
#endif

/* Resets the chip and sets the rate opl_update() generates at. */
void opl_init( void );

void opl_write( int reg, int val );

/* Generates n mono samples.  Call only with the audio locked or from the
 * audio callback: the chip is not safe to write and render concurrently. */
void opl_update( Bit16s *buf, int n );

#ifdef __cplusplus
}
#endif

#endif /* OPL_H */
