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
#include "config.h"
#include "destruct.h"
#include "editship.h"
//extern "C" {
	#include "episodes.h"
//}
#include "file.h"
#include "font.h"
#include "helptext.h"
#include "hg_revision.h"
#include "joystick.h"
#include "jukebox.h"
#include "keyboard.h"
#include "loudness.h"
#include "mainint.h"
#include "mtrand.h"
#include "musmast.h"
#include "network.h"
#include "nortsong.h"
#include "opentyr.h"
#include "params.h"
#include "picload.h"
#include "scroller.h"
#include "setup.h"
#include "sprite.h"
#include "tyrian2.h"
#include "xmas.h"
#include "varz.h"
#include "vga256d.h"
#include "video.h"

#include "SDL.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

const char *opentyrian_str = "OpenTyrian",
           *opentyrian_version = HG_REV;

void opentyrian_menu( void )
{
	/* Fullscreen and the scalers are gone: the panel is 320x240 and the game
	 * is drawn to it pixel for pixel. */
	typedef enum
	{
		MENU_ABOUT = 0,
		MENU_JUKEBOX,
		MENU_RETURN,
		MenuOptions_MAX
	} MenuOptions;

	static const char *menu_items[] =
	{
		"About OpenTyrian",
		"Jukebox",
		"Return to Main Menu",
	};
	bool menu_items_disabled[] =
	{
		false,
		false,
		false,
	};
	
	assert(COUNTOF(menu_items) == MenuOptions_MAX);
	assert(COUNTOF(menu_items_disabled) == MenuOptions_MAX);

	fade_black(10);
	JE_loadPic(VGAScreen, 13, false);

	draw_font_hv(VGAScreen, VGAScreen->w / 2, 5, opentyrian_str, large_font, centered, 15, -3);

	memcpy(VGAScreen2->pixels, VGAScreen->pixels, VGAScreen2->pitch * VGAScreen2->h);

	JE_showVGA();

	play_song(36); // A Field for Mag

	MenuOptions sel = (MenuOptions)0;

	bool fade_in = true, quit = false;
	do
	{
		memcpy(VGAScreen->pixels, VGAScreen2->pixels, VGAScreen->pitch * VGAScreen->h);

		for (int i = 0; i < (int)MenuOptions_MAX; i++)
		{
			const char *text = menu_items[i];

			int y = i != MENU_RETURN ? i * 16 + 32 : 118;
			draw_font_hv(VGAScreen, VGAScreen->w / 2, y, text, normal_font, centered, 15, menu_items_disabled[i] ? -8 : i != sel ? -4 : -2);
		}

		JE_showVGA();

		if (fade_in)
		{
			fade_in = false;
			fade_palette(colors, 20, 0, 255);
			wait_noinput(true, false, false);
		}

		tempW = 0;
		JE_textMenuWait(&tempW, false);

		if (newkey)
		{
			switch (lastkey_sym)
			{
			case SDL_SCANCODE_UP:
				do
				{
					if ((int)sel-1 == 0)
						sel = (MenuOptions_MAX - 1);
					else
						sel = ((int)sel-1);
				}
				while (menu_items_disabled[sel]);
				
				JE_playSampleNum(S_CURSOR);
				break;
			case SDL_SCANCODE_DOWN:
				do
				{
					if ((int)sel+1 >= MenuOptions_MAX)
						sel = (0);
					else
						sel = ((int)sel+1);
				}
				while (menu_items_disabled[sel]);
				
				JE_playSampleNum(S_CURSOR);
				break;
				
			case SDL_SCANCODE_RETURN:
				switch (sel)
				{
				case MENU_ABOUT:
					JE_playSampleNum(S_SELECT);

					scroller_sine(about_text);

					memcpy(VGAScreen->pixels, VGAScreen2->pixels, VGAScreen->pitch * VGAScreen->h);
					JE_showVGA();
					fade_in = true;
					break;
					
				case MENU_JUKEBOX:
					JE_playSampleNum(S_SELECT);

					fade_black(10);
					jukebox();

					memcpy(VGAScreen->pixels, VGAScreen2->pixels, VGAScreen->pitch * VGAScreen->h);
					JE_showVGA();
					fade_in = true;
					break;
					
				case MENU_RETURN:
					quit = true;
					JE_playSampleNum(S_SPRING);
					break;
					
				case MenuOptions_MAX:
					assert(false);
					break;
				}
				break;
				
			case SDL_SCANCODE_ESCAPE:
				quit = true;
				JE_playSampleNum(S_SPRING);
				break;
				
			default:
				break;
			}
		}
	} while (!quit);
}

/*
 * The game's main(), under another name: the firmware's own main() has to set
 * the clock and bring up the console first, and the host build's passes its
 * arguments straight through.
 */
int opentyrian_main( int argc, char *argv[] )
{

	mt_srand(time(NULL));
	printf("\nWelcome to... >> %s %s <<\n\n", opentyrian_str, opentyrian_version);

	printf("Copyright (C) 2007-2013 The OpenTyrian Development Team\n\n");

	printf("This program comes with ABSOLUTELY NO WARRANTY.\n");
	printf("This is free software, and you are welcome to redistribute it\n");
	printf("under certain conditions.  See the file GPL.txt for details.\n\n");

	init_video();

	JE_loadConfiguration();

	xmas = xmas_time();  // arg handler may override

	JE_paramCheck(argc, argv);

	JE_scanForEpisodes();

	init_joysticks();

	if (xmas && (!dir_file_exists(data_dir(), "tyrianc.shp") || !dir_file_exists(data_dir(), "voicesc.snd")))
	{
		xmas = false;

		fprintf(stderr, "warning: Christmas is missing.\n");
	}

	JE_loadPals();
	JE_loadMainShapeTables(xmas ? "tyrianc.shp" : "tyrian.shp");

	if (xmas && !xmas_prompt())
	{
		xmas = false;
		free_main_shape_tables();
		JE_loadMainShapeTables("tyrian.shp");
	}


	/* Default Options */
	youAreCheating = false;
	smoothScroll = true;
	loadDestruct = false;

	if (!audio_disabled)
	{
		printf("initializing SDL audio...\n");
		init_audio();

		load_music();

		JE_loadSndFile("tyrian.snd", xmas ? "voicesc.snd" : "voices.snd");
		printf("audio loaded\n");
	}
	else
	{
		printf("audio disabled\n");
	}

	if (record_demo)
		printf("demo recording enabled (input limited to keyboard)\n");

	JE_loadExtraShapes();  /*Editship*/

	JE_loadHelpText();

	if (isNetworkGame)
	{
#ifdef WITH_NETWORK
		if (network_init())
		{
			network_tyrian_halt(3, false);
		}
#else
		fprintf(stderr, "OpenTyrian was compiled without networking support.");
		JE_tyrianHalt(5);
#endif
	}

#ifdef NDEBUG
	if (!isNetworkGame)
		intro_logos();
#endif

	for (; ; )
	{
		JE_initPlayerData();
		JE_sortHighScores();

		if (JE_titleScreen(true))
			break;  // user quit from title screen

#ifdef WITH_DESTRUCT
		if (loadDestruct)
		{
			JE_destructGame();
			loadDestruct = false;
		}
		else
#endif
		{
			JE_main();
		}
	}

	JE_tyrianHalt(0);

	return 0;
}

