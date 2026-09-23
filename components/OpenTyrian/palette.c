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
#include "nortsong.h"
#include "opentyr.h"
#include "palette.h"
#include "video.h"

#include <assert.h>

/*
 * What the game believes the palette is.  The fades step it and read it back,
 * so it has to exist as numbers; the display's copy is picosdl's CLUT, and
 * every change here is pushed there, which is what makes a fade 256 register
 * writes rather than a redraw.
 */
static Palette palette;

Palette colors;

static void update_clut( unsigned int first_color, unsigned int last_color )
{
	SDL_SetPaletteColors(PSDL_GlobalPalette(), &palette[first_color], first_color, last_color - first_color + 1);
}

/* The palettes are compiled into flash; there is nothing to load. */
void JE_loadPals( void )
{
}

void set_palette( const Palette colors, unsigned int first_color, unsigned int last_color )
{
	for (uint i = first_color; i <= last_color; ++i)
		palette[i] = colors[i];

	update_clut(first_color, last_color);
}

void set_colors( SDL_Color color, unsigned int first_color, unsigned int last_color )
{
	for (uint i = first_color; i <= last_color; ++i)
		palette[i] = color;

	update_clut(first_color, last_color);
}

void init_step_fade_palette( int diff[256][3], const Palette colors, unsigned int first_color, unsigned int last_color )
{
	for (unsigned int i = first_color; i <= last_color; i++)
	{
		diff[i][0] = (int)colors[i].r - palette[i].r;
		diff[i][1] = (int)colors[i].g - palette[i].g;
		diff[i][2] = (int)colors[i].b - palette[i].b;
	}
}

void init_step_fade_solid( int diff[256][3], SDL_Color color, unsigned int first_color, unsigned int last_color )
{
	for (unsigned int i = first_color; i <= last_color; i++)
	{
		diff[i][0] = (int)color.r - palette[i].r;
		diff[i][1] = (int)color.g - palette[i].g;
		diff[i][2] = (int)color.b - palette[i].b;
	}
}

void step_fade_palette( int diff[256][3], int steps, unsigned int first_color, unsigned int last_color )
{
	assert(steps > 0);
	
	for (unsigned int i = first_color; i <= last_color; i++)
	{
		int delta[3] = { diff[i][0] / steps, diff[i][1] / steps, diff[i][2] / steps };
		
		diff[i][0] -= delta[0];
		diff[i][1] -= delta[1];
		diff[i][2] -= delta[2];
		
		palette[i].r += delta[0];
		palette[i].g += delta[1];
		palette[i].b += delta[2];
	}

	update_clut(first_color, last_color);
}

/* The fades' working state; one, since only one fade runs at a time. */
static int diff[256][3];

void fade_palette(const Palette colors, int steps, unsigned int first_color, unsigned int last_color)
{
    assert(steps > 0);
    
    init_step_fade_palette(diff, colors, first_color, last_color);
    
    for (; steps > 0; steps--)
    {
        setdelay(1);
        
        step_fade_palette(diff, steps, first_color, last_color);
        
        // The panel expands indices through the CLUT as a frame is pushed,
        // so a palette change is invisible until the frame is sent again.
        JE_showVGA();
        
        wait_delay();
    }
}


void fade_solid(SDL_Color color, int steps, unsigned int first_color, unsigned int last_color)
{
    assert(steps > 0);
    
    init_step_fade_solid(diff, color, first_color, last_color);
    
    for (; steps > 0; steps--)
    {
        setdelay(1);
        
        step_fade_palette(diff, steps, first_color, last_color);
        
        // The panel expands indices through the CLUT as a frame is pushed,
        // so a palette change is invisible until the frame is sent again.
        JE_showVGA();
        
        wait_delay();
    }
}


void fade_black( int steps )
{
	SDL_Color black = { 0, 0, 0 };
	fade_solid(black, steps, 0, 255);
}

void fade_white( int steps )
{
	SDL_Color white = { 255, 255, 255 };
	fade_solid(white, steps, 0, 255);
}
