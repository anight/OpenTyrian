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
 * DBOPL behind opl.h, which is the interface the music player was written
 * against when the emulator was a C one.
 */
#include <new>
#include <string.h>

#include "dbopl_types.h"
#include "dbopl.h"
#include "opl.h"

extern "C" {
#include "loudness.h"
}

/*
 * Storage for the chip, so nothing allocates.
 */
alignas(DBOPL::Chip) static unsigned char chip_storage[sizeof(DBOPL::Chip)];
static DBOPL::Chip *chip;

/*
 * Gain from DBOPL's output to the music player's: none.  At unity DBOPL is
 * within 0.3 dB of the emulator OpenTyrian shipped with on every song tried -
 * song 2 at -16.30 against -16.52 dBFS RMS, with the same peaks and nothing
 * clipped - so the game's volume settings mean what they always did.
 * `make -C tools/tests opl-level` measures it.  (picowolf's x4 matches MAME's
 * emulator, which is a different reference; here it clips on every loud bar.)
 */
#ifndef DBOPL_GAIN_SHIFT
#define DBOPL_GAIN_SHIFT 0
#endif

/*
 * A fresh chip, as the music player asks for at the start of every song.
 *
 * Two things here come from picopop, which found both the hard way.
 *
 * InitTables() is called explicitly because nothing else calls it: DOSBox did
 * so from DBOPL::Handler::Init, which is not in this copy, and without it every
 * wave table is zero and the chip plays silence.  It guards itself, so it runs
 * once.
 *
 * And Setup() runs once per rate, not per song.  It is not a table load: it
 * calibrates the envelope rates by simulating envelopes sample by sample, which
 * picopop measured at 17 ms on an x86-64 and puts at the order of a second on
 * the M33 - a stall before every song, since lds_rewind() resets the chip each
 * time.  The result depends only on the rate, so the chip is set up once and a
 * pristine copy restored afterwards.  The chip still starts every song fresh,
 * which is the point of resetting it: a note from the last song cannot hang.
 */
void opl_init( void )
{
	static unsigned char pristine[sizeof(DBOPL::Chip)];
	static bool have_pristine;

	if (have_pristine)
	{
		memcpy(chip_storage, pristine, sizeof(chip_storage));
		return;
	}

	DBOPL::InitTables();

	memset(chip_storage, 0, sizeof(chip_storage));
	chip = new (chip_storage) DBOPL::Chip(false);  // an OPL2, as the game had
	chip->Setup(AUDIO_RATE);

	memcpy(pristine, chip_storage, sizeof(pristine));
	have_pristine = true;
}

void opl_write( int reg, int val )
{
	chip->WriteReg((uint32_t)reg, (uint8_t)val);
}

void opl_update( Bit16s *buf, int n )
{
	static int32_t block[256];  // static: this runs on core 1, with a 2 KB stack

	while (n > 0)
	{
		int chunk = n < 256 ? n : 256;

		chip->GenerateBlock2((Bitu)chunk, block);

		for (int i = 0; i < chunk; i++)
		{
			int32_t s = block[i] * (1 << DBOPL_GAIN_SHIFT);
			buf[i] = s > 32767 ? 32767 : s < -32768 ? -32768 : (Bit16s)s;
		}

		buf += chunk;
		n -= chunk;
	}
}
