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
#include "lds_play.h"
#include "loudness.h"
#include "musmast.h"
#include "nortsong.h"
#include "opentyr.h"
#include "params.h"

#include <string.h>

float music_volume = 0, sample_volume = 0;

bool music_stopped = true;
static volatile bool music_loading = false;  // the player is being replaced; see load_song()
unsigned int song_playing = 0;

bool audio_disabled = false, music_disabled = false, samples_disabled = false;

/* SYN: These shouldn't be used outside this file. Hands off! */
static VFILE *music_file = NULL;
static Uint32 song_offset[MUSIC_NUM + 1];
static Uint16 song_count = 0;

/*
 * The mixer runs in picosdl's audio callback, on core 1, and everything it
 * touches that the game also touches is guarded by SDL_LockAudio().
 *
 * Integer throughout.  The game's volumes are 0..255 and become Q8 gains when
 * they are set, so the callback multiplies and shifts and never converts.
 */
static int music_gain;   // Q8: 1.5 at full volume, as the float version was
static int sample_gain;  // 0..255

/*
 * A sound effect is played where it lies, in flash: 8-bit signed samples at
 * 11025 Hz, each held for SAMPLE_SCALING output frames.  That is what the
 * original expansion into a malloc'd buffer did, without the buffer.
 */
static struct
{
	const Sint8 *pos;
	Uint32 left;    // source samples still to play
	Uint8 phase;    // output frames already given to *pos
	Uint8 vol;      // 1..8, the game's channel volume + 1
} channel[SFX_CHANNELS];

static void audio_cb( void *userdata, Uint8 *stream, int len );

/*
 * What the mixer notices about its own output, always on because it is cheap
 * and because the fault it is for came and went with the build.  Written on
 * the mixer's core, read and reset on the game's; a torn read costs one wrong
 * figure in a diagnostic.
 *
 *   late    blocks that took longer to make than they take to play
 *   spikes  music samples that jump further from the last than any song
 *           does - 13405 is the most in all 41, rendered on the host
 *   clipped mixed samples that had to be clamped to 16 bits
 */
volatile Uint32 audio_stat_max_us, audio_stat_late, audio_stat_blocks;
volatile Uint32 audio_stat_spikes, audio_stat_clipped;

#define MUSIC_SPIKE 16000

void load_song( unsigned int song_num );

bool init_audio( void )
{
	if (audio_disabled)
		return false;

	SDL_AudioSpec ask, got;

	memset(&ask, 0, sizeof(ask));
	ask.freq = AUDIO_RATE;
	ask.format = AUDIO_S16SYS;
	ask.channels = 2;  // all picosdl offers; the mix is mono and goes to both
	ask.samples = 256;
	ask.callback = audio_cb;

	printf("\trequested %d Hz, %d channels, %d samples\n", ask.freq, ask.channels, ask.samples);

	if (SDL_OpenAudio(&ask, &got) == -1)
	{
		fprintf(stderr, "error: failed to initialize SDL audio: %s\n", SDL_GetError());
		audio_disabled = true;
		return false;
	}

	printf("\tobtained  %d Hz, %d channels, %d samples\n", got.freq, got.channels, got.samples);

	opl_init();

	SDL_PauseAudio(0); // unpause

	return true;
}

/* Mono, 16-bit, n frames of music at the current volume: the OPL clocked at
 * the rate the song's player expects to be called, which is REFRESH Hz. */
static void render_music( Sint32 *mix, int n )
{
	static long ct = 0;
	static Bit16s music[256];  // static: core 1's stack is 2 KB, and this runs there

	int pos = 0;
	while (pos < n)
	{
		while (ct < 0)
		{
			ct += AUDIO_RATE;
			lds_update();
		}

		/* SYN: ct represents the margin between play time (in samples) and
		 * the song's tick rate, which do not synchronise exactly; generate up
		 * to the next tick and no further. */
		long i = (long)((ct / REFRESH) + 4) & ~3;
		if (i > n - pos)
			i = n - pos;

		opl_update(music, (int)i);
		for (long k = 0; k < i; k++)
		{
			static int last;
			int d = music[k] - last;
			if (d > MUSIC_SPIKE || d < -MUSIC_SPIKE)
				audio_stat_spikes++;
			last = music[k];

			mix[pos + k] = (music[k] * music_gain) >> 8;
		}

		pos += i;
		ct -= (long)(REFRESH * i);
	}
}

static void mix_samples( Sint32 *mix, int n )
{
	for (int ch = 0; ch < SFX_CHANNELS; ch++)
	{
		if (channel[ch].left == 0)
			continue;

		/* SYN's float volume, sample_volume * vol / SFX_CHANNELS on a sample
		 * shifted up by 8, as one integer: full scale at both maxima. */
		const int gain = (256 * sample_gain * channel[ch].vol) / (255 * SFX_CHANNELS);

		const Sint8 *pos = channel[ch].pos;
		Uint32 left = channel[ch].left;
		unsigned phase = channel[ch].phase;

		for (int i = 0; i < n && left > 0; i++)
		{
			mix[i] += *pos * gain;

			if (++phase == SAMPLE_SCALING)
			{
				phase = 0;
				pos++;
				left--;
			}
		}

		channel[ch].pos = pos;
		channel[ch].left = left;
		channel[ch].phase = phase;
	}
}

static void audio_cb( void *userdata, Uint8 *stream, int len )
{
	(void)userdata;

	Sint16 *out = (Sint16 *)stream;
	int frames = len / (2 * (int)sizeof(Sint16));

	const Uint32 budget_us = (Uint32)((Uint64)frames * 1000000u / AUDIO_RATE);
	Uint64 t0 = SDL_GetPerformanceCounter();

	/* Static, as are the buffers below it: this runs on core 1, whose stack
	 * picosdl gives 2 KB, and these three alone would be 2.5. */
	static Sint32 mix[256];

	while (frames > 0)
	{
		int n = frames < 256 ? frames : 256;

		if (!music_disabled && !music_stopped && !music_loading)
			render_music(mix, n);
		else
			memset(mix, 0, n * sizeof(*mix));

		if (!samples_disabled)
			mix_samples(mix, n);

		for (int i = 0; i < n; i++)
		{
			Sint32 s = mix[i];
			if (s > 32767 || s < -32768)
				audio_stat_clipped++;
			Sint16 c = s > 32767 ? 32767 : s < -32768 ? -32768 : (Sint16)s;
			out[2 * i] = out[2 * i + 1] = c;
		}

		out += 2 * n;
		frames -= n;
	}

	Uint32 us = (Uint32)(SDL_GetPerformanceCounter() - t0);
	if (us > audio_stat_max_us)
		audio_stat_max_us = us;
	if (us > budget_us)
		audio_stat_late++;
	audio_stat_blocks++;
}

void deinit_audio( void )
{
	if (audio_disabled)
		return;

	SDL_PauseAudio(1); // pause

	SDL_CloseAudio();

	memset(channel, 0, sizeof(channel));

	lds_free();
}


void load_music( void )
{
	if (music_file == NULL)
	{
		music_file = dir_fopen_die(data_dir(), "music.mus", "rb");

		efread(&song_count, sizeof(song_count), 1, music_file);
		if (song_count > MUSIC_NUM)
		{
			fprintf(stderr, "error: music.mus has %u songs, more than %d\n", song_count, MUSIC_NUM);
			song_count = MUSIC_NUM;
		}

		efread(song_offset, 4, song_count, music_file);
		song_offset[song_count] = ftell_eof(music_file);
	}
}

void load_song( unsigned int song_num )
{
	if (audio_disabled)
		return;

	/*
	 * Not under SDL_LockAudio() for the length of the load, which is how this
	 * used to be done and what put static on the music.
	 *
	 * Decoding a song out of the compressed file layer takes up to 16 ms on
	 * the board, and the mixer's interrupt takes the same lock first thing:
	 * held that long, the interrupt waits past the end of the next 11.6 ms
	 * block, two DMA completions arrive as one, and picosdl's I2S double buffer
	 * - which alternates by counting interrupts - is left filling the half the
	 * DAC is playing.  Every block after that is torn, until another long
	 * load happens to flip it back.
	 *
	 * So the lock is held only to say the music is being replaced: the mixer
	 * runs whole under it, so once it has been taken and released no render
	 * is under way, and every one after that sees music_loading and leaves the
	 * player alone.  The effects keep playing while the song loads.
	 */
	SDL_LockAudio();
	music_loading = true;
	SDL_UnlockAudio();

	if (song_num < song_count)
	{
		unsigned int song_size = song_offset[song_num + 1] - song_offset[song_num];
		lds_load(music_file, song_offset[song_num], song_size);
	}
	else
	{
		fprintf(stderr, "warning: failed to load song %d\n", song_num + 1);
	}

	SDL_LockAudio();
	music_loading = false;
	SDL_UnlockAudio();
}

void play_song( unsigned int song_num )
{
	if (song_num != song_playing)
	{
		load_song(song_num);
		song_playing = song_num;
	}

	music_stopped = false;
}

void restart_song( void )
{
	unsigned int temp = song_playing;
	song_playing = -1;
	play_song(temp);
}

void stop_song( void )
{
	music_stopped = true;
}

void fade_song( void )
{
	/* STUB: we have no implementation of this to port */
}

void set_volume( unsigned int music, unsigned int sample )
{
	music_volume = music * (1.5f / 255.0f);
	sample_volume = sample * (1.0f / 255.0f);

	/* Not under SDL_LockAudio(): this is first called by the configuration
	 * loader, before the audio is open, and picosdl's lock does not exist
	 * until then - taking it hangs the board.  Nor does it need it: each is
	 * one aligned word, stored whole, which the mixer reads once a block. */
	music_gain = (int)(music * 384 / 255);
	sample_gain = (int)(sample > 255 ? 255 : sample);
}

void JE_multiSamplePlay( const JE_byte *buffer, JE_word size, JE_byte chan, JE_byte vol )
{
	if (audio_disabled || samples_disabled || chan >= SFX_CHANNELS)
		return;

	SDL_LockAudio();

	channel[chan].pos = (const Sint8 *)buffer;
	channel[chan].left = size;
	channel[chan].phase = 0;
	channel[chan].vol = vol + 1;

	SDL_UnlockAudio();
}
