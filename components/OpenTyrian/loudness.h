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
#ifndef LOUDNESS_H
#define LOUDNESS_H

#include "opentyr.h"
#include "opl.h"

#include "SDL.h"

#define SFX_CHANNELS 8

/*
 * 22050 Hz: the sound effects are 11025 Hz, so each is held for exactly two
 * frames, and the OPL has nothing above 11 kHz for a higher rate to carry.
 * It halves the mixer's work against 44100 on the core that does nothing else.
 */
#define OUTPUT_QUALITY 2
#define AUDIO_RATE (11025 * OUTPUT_QUALITY)

#define SAMPLE_SCALING OUTPUT_QUALITY

extern float music_volume, sample_volume;

extern unsigned int song_playing;

extern bool audio_disabled, music_disabled, samples_disabled;

bool init_audio( void );
void deinit_audio( void );

void load_music( void );
void play_song( unsigned int song_num );
void restart_song( void );
void stop_song( void );
void fade_song( void );

void set_volume( unsigned int music, unsigned int sample );

void JE_multiSamplePlay( const JE_byte *buffer, JE_word size, JE_byte chan, JE_byte vol );

#endif /* LOUDNESS_H */

