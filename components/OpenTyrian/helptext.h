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
#ifndef HELPTEXT_H
#define HELPTEXT_H

#include "opentyr.h"
#include "file.h"
#include "menus.h"

#include <stdio.h>

#define MENU_MAX 14

#define DESTRUCT_MODES 5

extern const JE_byte menuHelp[MENU_MAX][11];   /* [1..14, 1..11] */

extern JE_byte verticalHeight;
extern JE_byte helpBoxColor, helpBoxBrightness, helpBoxShadeType;

#ifdef TYRIAN2000
#define HELPTEXT_MISCTEXT_COUNT 72
#define HELPTEXT_MISCTEXTB_COUNT 8
#define HELPTEXT_MISCTEXTB_SIZE 12
#define HELPTEXT_MENUTEXT_SIZE 29
#define HELPTEXT_MAINMENUHELP_COUNT 37
#define HELPTEXT_NETWORKTEXT_COUNT 5
#define HELPTEXT_NETWORKTEXT_SIZE 33
#define HELPTEXT_SUPERSHIPS_COUNT 13
#define HELPTEXT_SPECIALNAME_COUNT 11
#define HELPTEXT_SHIPINFO_COUNT 20
#define HELPTEXT_MENUINT3_COUNT 9
#define HELPTEXT_MENUINT12_COUNT 7
#else
#define HELPTEXT_MISCTEXT_COUNT 68
#define HELPTEXT_MISCTEXTB_COUNT 5
#define HELPTEXT_MISCTEXTB_SIZE 11
#define HELPTEXT_MENUTEXT_SIZE 21
#define HELPTEXT_MAINMENUHELP_COUNT 34
#define HELPTEXT_NETWORKTEXT_COUNT 4
#define HELPTEXT_NETWORKTEXT_SIZE 22
#define HELPTEXT_SUPERSHIPS_COUNT 11
#define HELPTEXT_SPECIALNAME_COUNT 9
#define HELPTEXT_SHIPINFO_COUNT 13
#endif

/*
 * The game's text, from tyrian.hdt.  Nearly all of it is read and never
 * written, so it is not loaded: tools/assets/gamedata_gen.c runs
 * JE_readHelpText() on the host and compiles the result into flash as
 * ty_help, and the names the game uses are that, read-only.  24 KB less RAM.
 *
 * menuInt and menuText are the exceptions - the level and item menus rewrite
 * rows of menuInt, and the title screen puts OpenTyrian into menuText - so
 * those two are RAM, initialised from the flash copy by JE_loadHelpText().
 */
typedef struct
{
	char helpTxt[39][231];
	char pName[21][16];
	char miscText[HELPTEXT_MISCTEXT_COUNT][42];
	char miscTextB[HELPTEXT_MISCTEXTB_COUNT][HELPTEXT_MISCTEXTB_SIZE];
	char menuText[7][HELPTEXT_MENUTEXT_SIZE];
	char outputs[9][31];
	char topicName[6][21];
	char mainMenuHelp[HELPTEXT_MAINMENUHELP_COUNT][66];
	char inGameText[6][21];
	char detailLevel[6][13];
	char gameSpeedText[5][13];
	char episode_name[6][31];
	char difficulty_name[7][21];
	char gameplay_name[GAMEPLAY_NAME_COUNT][26];
	char inputDevices[3][13];
	char networkText[HELPTEXT_NETWORKTEXT_COUNT][HELPTEXT_NETWORKTEXT_SIZE];
	char difficultyNameB[11][21];
	char joyButtonNames[5][21];
	char superShips[HELPTEXT_SUPERSHIPS_COUNT][26];
	char specialName[HELPTEXT_SPECIALNAME_COUNT][10];
	char destructHelp[25][22];
	char weaponNames[17][17];
	char destructModeName[DESTRUCT_MODES][13];
	char shipInfo[HELPTEXT_SHIPINFO_COUNT][2][256];
	char menuInt[MENU_MAX+1][11][18];
} JE_HelpText;

extern const JE_HelpText ty_help;

/* The loader names the fields themselves, so it asks not to have these. */
#ifndef HELPTEXT_NO_ALIASES
#define helpTxt          (ty_help.helpTxt)
#define pName            (ty_help.pName)
#define miscText         (ty_help.miscText)
#define miscTextB        (ty_help.miscTextB)
#define outputs          (ty_help.outputs)
#define topicName        (ty_help.topicName)
#define mainMenuHelp     (ty_help.mainMenuHelp)
#define inGameText       (ty_help.inGameText)
#define detailLevel      (ty_help.detailLevel)
#define gameSpeedText    (ty_help.gameSpeedText)
#define episode_name     (ty_help.episode_name)
#define difficulty_name  (ty_help.difficulty_name)
#define gameplay_name    (ty_help.gameplay_name)
#define inputDevices     (ty_help.inputDevices)
#define networkText      (ty_help.networkText)
#define difficultyNameB  (ty_help.difficultyNameB)
#define joyButtonNames   (ty_help.joyButtonNames)
#define superShips       (ty_help.superShips)
#define specialName      (ty_help.specialName)
#define destructHelp     (ty_help.destructHelp)
#define weaponNames      (ty_help.weaponNames)
#define destructModeName (ty_help.destructModeName)
#define shipInfo         (ty_help.shipInfo)
#endif

extern char menuText[7][HELPTEXT_MENUTEXT_SIZE];
extern char menuInt[MENU_MAX+1][11][18];

/* The game's loader, filling h: run on the host at build time. */
void JE_readHelpText( VFILE *f, JE_HelpText *h );

void read_encrypted_pascal_string( char *s, int size, VFILE *f );
void skip_pascal_string( VFILE *f );

void JE_helpBox( SDL_Surface *screen, int x, int y, const char *message, unsigned int boxwidth );
void JE_HBox( SDL_Surface *screen, int x, int y, unsigned int  messagenum, unsigned int boxwidth );
void JE_loadHelpText( void );

#endif /* HELPTEXT_H */

