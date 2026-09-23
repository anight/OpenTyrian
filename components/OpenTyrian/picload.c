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
#include "opentyr.h"
#include "palette.h"
#include "pcxmast.h"
#include "picload.h"
#include "video.h"


#include <string.h>

void JE_loadPic(SDL_Surface *screen, JE_byte PCXnumber, JE_boolean storepal )
{
	PCXnumber--;

	VFILE *f = dir_fopen_die(data_dir(), "tyrian.pic", "rb");

	static bool first = true;
	if (first)
	{
		first = false;

		Uint16 temp;
		efread(&temp, sizeof(Uint16), 1, f);
		for (int i = 0; i < PCX_NUM; i++)
		{
			efread(&pcxpos[i], sizeof(JE_longint), 1, f);
		}

		pcxpos[PCX_NUM] = ftell_eof(f);
	}

	/* Decoded as it is read: the RLE stream is walked once, front to back, so
	 * there is no reason to hold it anywhere first. */
	efseek(f, pcxpos[PCXnumber], SEEK_SET);

	Uint8 *s; /* screen pointer, 8-bit specific */

	s = (Uint8 *)screen->pixels;

	for (int i = 0; i < 320 * 200; )
	{
		int c = efgetc(f);
		if (c == EOF)
			break;

		if ((c & 0xc0) == 0xc0)
		{
			int run = c & 0x3f;
			int value = efgetc(f);
			if (value == EOF)
				break;
			i += run;
			memset(s, value, run);
			s += run;
		} else {
			i++;
			*s = c;
			s++;
		}
		if (i && (i % 320 == 0))
		{
			s += screen->pitch - 320;
		}
	}

	efclose(f);

	memcpy(colors, palettes[pcxpal[PCXnumber]], sizeof(colors));

	if (storepal)
		set_palette(colors, 0, 255);
}

