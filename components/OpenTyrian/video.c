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
#include "opentyr.h"
#include "video.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

SDL_Surface *VGAScreen, *VGAScreenSeg;
SDL_Surface *VGAScreen2;
SDL_Surface *game_screen;

/*
 * The three 320x200 screens the game draws with, 187.5 KB together and most of
 * the RAM the port spends.  All three are live during play - game_screen is
 * where the play field is drawn, VGAScreen2 holds the background layer and
 * VGAScreenSeg is the frame with the sidebar - so none can be folded into
 * another.
 *
 * picosdl allocates no pixels: these are ours, and the library wraps them.
 */
static Uint8 screen_pixels[3][vga_width * vga_height];

static SDL_Window *window;

void init_video( void )
{
	if (SDL_Init(SDL_INIT_VIDEO) != 0)
	{
		fprintf(stderr, "error: failed to initialize SDL video: %s\n", SDL_GetError());
		exit(EXIT_FAILURE);
	}

	window = PSDL_CreateWindow(screen_pixels[0], vga_width, vga_height, vga_width);
	VGAScreen = VGAScreenSeg = SDL_GetWindowSurface(window);

	VGAScreen2 = SDL_CreateRGBSurfaceFrom(screen_pixels[1], vga_width, vga_height, 8, vga_width, 0, 0, 0, 0);
	game_screen = SDL_CreateRGBSurfaceFrom(screen_pixels[2], vga_width, vga_height, 8, vga_width, 0, 0, 0, 0);

	if (VGAScreen == NULL || VGAScreen2 == NULL || game_screen == NULL)
	{
		fprintf(stderr, "error: failed to create the screen surfaces: %s\n", SDL_GetError());
		exit(EXIT_FAILURE);
	}

	SDL_FillRect(VGAScreen, NULL, 0);
}

void deinit_video( void )
{
	SDL_FreeSurface(VGAScreen2);
	SDL_FreeSurface(game_screen);
	SDL_DestroyWindow(window);

	SDL_Quit();
}

void JE_clr256( SDL_Surface * screen)
{
	memset(screen->pixels, 0, screen->pitch * screen->h);
}

void JE_showVGA( void ) { scale_and_flip(VGAScreen); }

/*
 * Whatever VGAScreen points at goes to the panel - it is not always the window
 * surface, the game swaps it to game_screen or a scratch surface while it
 * composes - so this presents the buffer rather than the window.
 *
 * The present is asynchronous and the game draws into the same buffer as soon
 * as this returns, with no notion that the panel is still reading it.  So it
 * waits for the transfer: correct, at the cost of the drawing that could have
 * overlapped it.
 */
/* Something the firmware does once a frame, which the host build has nothing
 * to do for: src/tyrian_main.c reports the stack from here. */
__attribute__((weak)) void picotyrian_frame_hook( void )
{
}

void scale_and_flip( SDL_Surface *src_surface )
{
	PSDL_PresentBuffer(src_surface->pixels, src_surface->w, src_surface->h, src_surface->pitch);
	PSDL_PresentSync();

	picotyrian_frame_hook();
}
