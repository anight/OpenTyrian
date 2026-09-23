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
#define HELPTEXT_NO_ALIASES  // this file fills the fields by name

#include "episodes.h"
#include "file.h"
#include "opentyr.h"
#include "helptext.h"
#include "palette.h"

#include <assert.h>

/*
 * The game's loaders for the data it reads once and never writes: the item
 * and enemy definitions, and the palettes.  They are the originals, except
 * that each fills what it is given rather than a global.
 *
 * The firmware does not call them.  tools/assets/gamedata_gen.c runs them on
 * the host against the converted resources, at build time, and compiles what
 * they read into flash, where the game's globals point.
 */
void JE_readItemDat( VFILE *f, JE_ItemTables *t, JE_byte episode )
{
	(void)episode;  // only the Tyrian 2000 layout needs it

	JE_word itemNum[7]; /* [1..7] */
	efread(&itemNum, sizeof(JE_word), 7, f);
	for (int i = 0; i < WEAP_NUM + 1; ++i)
	{
		efread(&t->weapons[i].drain,           sizeof(JE_word), 1, f);
		efread(&t->weapons[i].shotrepeat,      sizeof(JE_byte), 1, f);
		efread(&t->weapons[i].multi,           sizeof(JE_byte), 1, f);
		efread(&t->weapons[i].weapani,         sizeof(JE_word), 1, f);
		efread(&t->weapons[i].max,             sizeof(JE_byte), 1, f);
		efread(&t->weapons[i].tx,              sizeof(JE_byte), 1, f);
		efread(&t->weapons[i].ty,              sizeof(JE_byte), 1, f);
		efread(&t->weapons[i].aim,             sizeof(JE_byte), 1, f);
		efread(&t->weapons[i].attack,          sizeof(JE_byte), 8, f);
		efread(&t->weapons[i].del,             sizeof(JE_byte), 8, f);
		efread(&t->weapons[i].sx,              sizeof(JE_shortint), 8, f);
		efread(&t->weapons[i].sy,              sizeof(JE_shortint), 8, f);
		efread(&t->weapons[i].bx,              sizeof(JE_shortint), 8, f);
		efread(&t->weapons[i].by,              sizeof(JE_shortint), 8, f);
		efread(&t->weapons[i].sg,              sizeof(JE_word), 8, f);
		efread(&t->weapons[i].acceleration,    sizeof(JE_shortint), 1, f);
		efread(&t->weapons[i].accelerationx,   sizeof(JE_shortint), 1, f);
		efread(&t->weapons[i].circlesize,      sizeof(JE_byte), 1, f);
		efread(&t->weapons[i].sound,           sizeof(JE_byte), 1, f);
		efread(&t->weapons[i].trail,           sizeof(JE_byte), 1, f);
		efread(&t->weapons[i].shipblastfilter, sizeof(JE_byte), 1, f);
	}

#ifdef TYRIAN2000
	if (episode <= 3) efseek(f, 0x252A4, SEEK_SET);
	if (episode == 4) efseek(f, 0xC1F5E, SEEK_SET);
	if (episode == 5) efseek(f, 0x5C5B8, SEEK_SET);
#endif
	for (int i = 0; i < PORT_NUM + 1; ++i)
	{
		efseek(f, 1, SEEK_CUR); /* skip string length */
		efread(&t->weaponPort[i].name,        1, 30, f);
		t->weaponPort[i].name[30] = '\0';
		efread(&t->weaponPort[i].opnum,       sizeof(JE_byte), 1, f);
		for (int j = 0; j < 2; ++j)
		{
			efread(&t->weaponPort[i].op[j],   sizeof(JE_word), 11, f);
		}
		efread(&t->weaponPort[i].cost,        sizeof(JE_word), 1, f);
		efread(&t->weaponPort[i].itemgraphic, sizeof(JE_word), 1, f);
		efread(&t->weaponPort[i].poweruse,    sizeof(JE_word), 1, f);
	}

	int specials_count = SPECIAL_NUM;
#ifdef TYRIAN2000
	if (episode <= 3) efseek(f, 0x2662E, SEEK_SET);
	if (episode == 4) efseek(f, 0xC32E8, SEEK_SET);
	if (episode == 5) efseek(f, 0x5D942, SEEK_SET);
	if (episode >= 4) specials_count = SPECIAL_NUM + 8; /*this ugly hack will need a fix*/
#endif
	
	for (int i = 0; i < specials_count + 1; ++i)
	{
		efseek(f, 1, SEEK_CUR); /* skip string length */
		efread(&t->special[i].name,        1, 30, f);
		t->special[i].name[30] = '\0';
		efread(&t->special[i].itemgraphic, sizeof(JE_word), 1, f);
		efread(&t->special[i].pwr,         sizeof(JE_byte), 1, f);
		efread(&t->special[i].stype,       sizeof(JE_byte), 1, f);
		efread(&t->special[i].wpn,         sizeof(JE_word), 1, f);
	}

#ifdef TYRIAN2000
	if (episode <= 3) efseek(f, 0x26E21, SEEK_SET);
	if (episode == 4) efseek(f, 0xC3ADB, SEEK_SET);
	if (episode == 5) efseek(f, 0x5E135, SEEK_SET);
#endif
		
	for (int i = 0; i < POWER_NUM + 1; ++i)
	{
		efseek(f, 1, SEEK_CUR); /* skip string length */
		efread(&t->powerSys[i].name,        1, 30, f);
		t->powerSys[i].name[30] = '\0';
		efread(&t->powerSys[i].itemgraphic, sizeof(JE_word), 1, f);
		efread(&t->powerSys[i].power,       sizeof(JE_shortint), 1, f);
		efread(&t->powerSys[i].speed,       sizeof(JE_byte), 1, f);
		efread(&t->powerSys[i].cost,        sizeof(JE_word), 1, f);
	}

#ifdef TYRIAN2000
	if (episode <= 3) efseek(f, 0x26F24, SEEK_SET);
	if (episode == 4) efseek(f, 0xC3BDE, SEEK_SET);
	if (episode == 5) efseek(f, 0x5E238, SEEK_SET);
#endif
	
	for (int i = 0; i < SHIP_NUM + 1; ++i)
	{
		efseek(f, 1, SEEK_CUR); /* skip string length */
		efread(&t->ships[i].name,           1, 30, f);
		t->ships[i].name[30] = '\0';
		efread(&t->ships[i].shipgraphic,    sizeof(JE_word), 1, f);
		efread(&t->ships[i].itemgraphic,    sizeof(JE_word), 1, f);
		efread(&t->ships[i].ani,            sizeof(JE_byte), 1, f);
		efread(&t->ships[i].spd,            sizeof(JE_shortint), 1, f);
		efread(&t->ships[i].dmg,            sizeof(JE_byte), 1, f);
		efread(&t->ships[i].cost,           sizeof(JE_word), 1, f);
		efread(&t->ships[i].bigshipgraphic, sizeof(JE_byte), 1, f);
	}

#ifdef TYRIAN2000
	if (episode <= 3) efseek(f, 0x2722F, SEEK_SET);
	if (episode == 4) efseek(f, 0xC3EE9, SEEK_SET);
	if (episode == 5) efseek(f, 0x5E543, SEEK_SET); 
#endif
	for (int i = 0; i < OPTION_NUM + 1; ++i)
	{
		efseek(f, 1, SEEK_CUR); /* skip string length */
		efread(&t->options[i].name,        1, 30, f);
		t->options[i].name[30] = '\0';
		efread(&t->options[i].pwr,         sizeof(JE_byte), 1, f);
		efread(&t->options[i].itemgraphic, sizeof(JE_word), 1, f);
		efread(&t->options[i].cost,        sizeof(JE_word), 1, f);
		efread(&t->options[i].tr,          sizeof(JE_byte), 1, f);
		efread(&t->options[i].option,      sizeof(JE_byte), 1, f);
		efread(&t->options[i].opspd,       sizeof(JE_shortint), 1, f);
		efread(&t->options[i].ani,         sizeof(JE_byte), 1, f);
		efread(&t->options[i].gr,          sizeof(JE_word), 20, f);
		efread(&t->options[i].wport,       sizeof(JE_byte), 1, f);
		efread(&t->options[i].wpnum,       sizeof(JE_word), 1, f);
		efread(&t->options[i].ammo,        sizeof(JE_byte), 1, f);
		efread(&t->options[i].stop,        1, 1, f); /* override sizeof(JE_boolean) */
		efread(&t->options[i].icongr,      sizeof(JE_byte), 1, f);
	}

#ifdef TYRIAN2000
	if (episode <= 3) efseek(f, 0x27EF3, SEEK_SET);
	if (episode == 4) efseek(f, 0xC4BAD, SEEK_SET);
	if (episode == 5) efseek(f, 0x5F207, SEEK_SET);
#endif
		
	for (int i = 0; i < SHIELD_NUM + 1; ++i)
	{
		efseek(f, 1, SEEK_CUR); /* skip string length */
		efread(&t->shields[i].name,        1, 30, f);
		t->shields[i].name[30] = '\0';
		efread(&t->shields[i].tpwr,        sizeof(JE_byte), 1, f);
		efread(&t->shields[i].mpwr,        sizeof(JE_byte), 1, f);
		efread(&t->shields[i].itemgraphic, sizeof(JE_word), 1, f);
		efread(&t->shields[i].cost,        sizeof(JE_word), 1, f);
	}
	for (int i = 0; i < ENEMY_NUM + 1; ++i)
	{
		efread(&t->enemyDat[i].ani,           sizeof(JE_byte), 1, f);
		efread(&t->enemyDat[i].tur,           sizeof(JE_byte), 3, f);
		efread(&t->enemyDat[i].freq,          sizeof(JE_byte), 3, f);
		efread(&t->enemyDat[i].xmove,         sizeof(JE_shortint), 1, f);
		efread(&t->enemyDat[i].ymove,         sizeof(JE_shortint), 1, f);
		efread(&t->enemyDat[i].xaccel,        sizeof(JE_shortint), 1, f);
		efread(&t->enemyDat[i].yaccel,        sizeof(JE_shortint), 1, f);
		efread(&t->enemyDat[i].xcaccel,       sizeof(JE_shortint), 1, f);
		efread(&t->enemyDat[i].ycaccel,       sizeof(JE_shortint), 1, f);
		efread(&t->enemyDat[i].startx,        sizeof(JE_integer), 1, f);
		efread(&t->enemyDat[i].starty,        sizeof(JE_integer), 1, f);
		efread(&t->enemyDat[i].startxc,       sizeof(JE_shortint), 1, f);
		efread(&t->enemyDat[i].startyc,       sizeof(JE_shortint), 1, f);
		efread(&t->enemyDat[i].armor,         sizeof(JE_byte), 1, f);
		efread(&t->enemyDat[i].esize,         sizeof(JE_byte), 1, f);
		efread(&t->enemyDat[i].egraphic,      sizeof(JE_word), 20, f);
		efread(&t->enemyDat[i].explosiontype, sizeof(JE_byte), 1, f);
		efread(&t->enemyDat[i].animate,       sizeof(JE_byte), 1, f);
		efread(&t->enemyDat[i].shapebank,     sizeof(JE_byte), 1, f);
		efread(&t->enemyDat[i].xrev,          sizeof(JE_shortint), 1, f);
		efread(&t->enemyDat[i].yrev,          sizeof(JE_shortint), 1, f);
		efread(&t->enemyDat[i].dgr,           sizeof(JE_word), 1, f);
		efread(&t->enemyDat[i].dlevel,        sizeof(JE_shortint), 1, f);
		efread(&t->enemyDat[i].dani,          sizeof(JE_shortint), 1, f);
		efread(&t->enemyDat[i].elaunchfreq,   sizeof(JE_byte), 1, f);
		efread(&t->enemyDat[i].elaunchtype,   sizeof(JE_word), 1, f);
		efread(&t->enemyDat[i].value,         sizeof(JE_integer), 1, f);
		efread(&t->enemyDat[i].eenemydie,     sizeof(JE_word), 1, f);
	}
	
}

/* JE_loadPals(): every palette in palette.dat, rescaled from the VGA's six
 * bits per component to eight. */
int JE_readPals( VFILE *f, Palette *out, int max )
{
	int count = ftell_eof(f) / (256 * 3);
	if (count > max)
		count = max;

	for (int p = 0; p < count; ++p)
	{
		for (int i = 0; i < 256; ++i)
		{
			// The VGA hardware palette used only 6 bits per component, so the values need to be rescaled to
			// 8 bits. The naive way to do this is to simply do (c << 2), padding it with 0's, however this
			// makes the maximum value 252 instead of the proper 255. A trick to fix this is to use the upper 2
			// bits of the original value instead. This ensures that the value goes to 255 as the original goes
			// to 63.

			int c = efgetc(f);
			out[p][i].r = (c << 2) | (c >> 4);
			c = efgetc(f);
			out[p][i].g = (c << 2) | (c >> 4);
			c = efgetc(f);
			out[p][i].b = (c << 2) | (c >> 4);
			out[p][i].a = SDL_ALPHA_OPAQUE;
		}
	}

	return count;
}

static void decrypt_pascal_string( char *s, int len )
{
	static const unsigned char crypt_key[] = { 204, 129, 63, 255, 71, 19, 25, 62, 1, 99 };

	for (int i = len - 1; i >= 0; --i)
	{
		s[i] ^= crypt_key[i % sizeof(crypt_key)];
		if (i > 0)
			s[i] ^= s[i - 1];
	}
}

void read_encrypted_pascal_string( char *s, int size, VFILE *f )
{
	int len = efgetc(f);
	if (len != EOF)
	{
		int skip = MAX((len + 1) - size, 0);
		assert(skip == 0);

		len -= skip;
		efread(s, 1, len, f);
		if (size > 0)
			s[len] = '\0';
		efseek(f, skip, SEEK_CUR);

		decrypt_pascal_string(s, len);
	}
}

void skip_pascal_string( VFILE *f )
{
	int len = efgetc(f);
	efseek(f, len, SEEK_CUR);
}

/* JE_loadHelpText(): every table of text in tyrian.hdt, f positioned at its
 * start. */
void JE_readHelpText( VFILE *f, JE_HelpText *h )
{
#ifdef TYRIAN2000
	const unsigned int menuInt_entries[MENU_MAX + 1] = { -1, 7, 9, 9, -1, -1, 11, -1, -1, -1, 7, 4, 6, 7, 5 };
#else
	const signed int menuInt_entries[MENU_MAX + 1] = { -1, 7, 9, 8, -1, -1, 11, -1, -1, -1, 6, 4, 6, 7, 5 };
#endif
	
	JE_longint episode1DataLoc;  // where the item data starts; read, not needed here
	efread(&episode1DataLoc, sizeof(JE_longint), 1, f);

	/*Online Help*/
	skip_pascal_string(f);
	for (unsigned int i = 0; i < COUNTOF(h->helpTxt); ++i)
		read_encrypted_pascal_string(h->helpTxt[i], sizeof(h->helpTxt[i]), f);
	skip_pascal_string(f);

	/*Planet names*/
	skip_pascal_string(f);
	for (unsigned int i = 0; i < COUNTOF(h->pName); ++i)
		read_encrypted_pascal_string(h->pName[i], sizeof(h->pName[i]), f);
	skip_pascal_string(f);

	/*Miscellaneous text*/
	skip_pascal_string(f);
	for (unsigned int i = 0; i < COUNTOF(h->miscText); ++i)
		read_encrypted_pascal_string(h->miscText[i], sizeof(h->miscText[i]), f);
	skip_pascal_string(f);

	/*Little Miscellaneous text*/
	skip_pascal_string(f);
	for (unsigned int i = 0; i < COUNTOF(h->miscTextB); ++i)
		read_encrypted_pascal_string(h->miscTextB[i], sizeof(h->miscTextB[i]), f);
	skip_pascal_string(f);

	/*Key names*/
	skip_pascal_string(f);
	for (unsigned int i = 0; i < menuInt_entries[6]; ++i)
		read_encrypted_pascal_string(h->menuInt[6][i], sizeof(h->menuInt[6][i]), f);
	skip_pascal_string(f);

	/*Main Menu*/
	skip_pascal_string(f);
	for (unsigned int i = 0; i < COUNTOF(h->menuText); ++i)
		read_encrypted_pascal_string(h->menuText[i], sizeof(h->menuText[i]), f);
	skip_pascal_string(f);

	/*Event text*/
	skip_pascal_string(f);
	for (unsigned int i = 0; i < COUNTOF(h->outputs); ++i)
		read_encrypted_pascal_string(h->outputs[i], sizeof(h->outputs[i]), f);
	skip_pascal_string(f);

	/*Help topics*/
	skip_pascal_string(f);
	for (unsigned int i = 0; i < COUNTOF(h->topicName); ++i)
		read_encrypted_pascal_string(h->topicName[i], sizeof(h->topicName[i]), f);
	skip_pascal_string(f);

	/*Main Menu Help*/
	skip_pascal_string(f);
	for (unsigned int i = 0; i < COUNTOF(h->mainMenuHelp); ++i)
		read_encrypted_pascal_string(h->mainMenuHelp[i], sizeof(h->mainMenuHelp[i]), f);
	skip_pascal_string(f);

	/*Menu 1 - Main*/
	skip_pascal_string(f);
	for (unsigned int i = 0; i < menuInt_entries[1]; ++i)
		read_encrypted_pascal_string(h->menuInt[1][i], sizeof(h->menuInt[1][i]), f);
	skip_pascal_string(f);

	/*Menu 2 - Items*/
	skip_pascal_string(f);
	for (unsigned int i = 0; i < menuInt_entries[2]; ++i)
		read_encrypted_pascal_string(h->menuInt[2][i], sizeof(h->menuInt[2][i]), f);
	skip_pascal_string(f);

	/*Menu 3 - Options*/
	skip_pascal_string(f);
	for (unsigned int i = 0; i < menuInt_entries[3]; ++i)
		read_encrypted_pascal_string(h->menuInt[3][i], sizeof(h->menuInt[3][i]), f);
	skip_pascal_string(f);

	/*InGame Menu*/
	skip_pascal_string(f);
	for (unsigned int i = 0; i < COUNTOF(h->inGameText); ++i)
		read_encrypted_pascal_string(h->inGameText[i], sizeof(h->inGameText[i]), f);
	skip_pascal_string(f);

	/*Detail Level*/
	skip_pascal_string(f);
	for (unsigned int i = 0; i < COUNTOF(h->detailLevel); ++i)
		read_encrypted_pascal_string(h->detailLevel[i], sizeof(h->detailLevel[i]), f);
	skip_pascal_string(f);

	/*Game speed text*/
	skip_pascal_string(f);
	for (unsigned int i = 0; i < COUNTOF(h->gameSpeedText); ++i)
		read_encrypted_pascal_string(h->gameSpeedText[i], sizeof(h->gameSpeedText[i]), f);
	skip_pascal_string(f);

	// episode names
	skip_pascal_string(f);
	for (unsigned int i = 0; i < COUNTOF(h->episode_name); ++i)
		read_encrypted_pascal_string(h->episode_name[i], sizeof(h->episode_name[i]), f);
	skip_pascal_string(f);

	// difficulty names
	skip_pascal_string(f);
	for (unsigned int i = 0; i < COUNTOF(h->difficulty_name); ++i)
		read_encrypted_pascal_string(h->difficulty_name[i], sizeof(h->difficulty_name[i]), f);
	skip_pascal_string(f);

	// gameplay mode names
	skip_pascal_string(f);
	for (unsigned int i = 0; i < COUNTOF(h->gameplay_name); ++i)
		read_encrypted_pascal_string(h->gameplay_name[i], sizeof(h->gameplay_name[i]), f);
	skip_pascal_string(f);

	/*Menu 10 - 2Player Main*/
	skip_pascal_string(f);
	for (unsigned int i = 0; i < menuInt_entries[10]; ++i)
		read_encrypted_pascal_string(h->menuInt[10][i], sizeof(h->menuInt[10][i]), f);
	skip_pascal_string(f);

	/*Input Devices*/
	skip_pascal_string(f);
	for (unsigned int i = 0; i < COUNTOF(h->inputDevices); ++i)
		read_encrypted_pascal_string(h->inputDevices[i], sizeof(h->inputDevices[i]), f);
	skip_pascal_string(f);

	/*Network text*/
	skip_pascal_string(f);
	for (unsigned int i = 0; i < COUNTOF(h->networkText); ++i)
		read_encrypted_pascal_string(h->networkText[i], sizeof(h->networkText[i]), f);
	skip_pascal_string(f);

	/*Menu 11 - 2Player Network*/
	skip_pascal_string(f);
	for (unsigned int i = 0; i < menuInt_entries[11]; ++i)
		read_encrypted_pascal_string(h->menuInt[11][i], sizeof(h->menuInt[11][i]), f);
	skip_pascal_string(f);

	/*HighScore Difficulty Names*/
	skip_pascal_string(f);
	for (unsigned int i = 0; i < COUNTOF(h->difficultyNameB); ++i)
		read_encrypted_pascal_string(h->difficultyNameB[i], sizeof(h->difficultyNameB[i]), f);
	skip_pascal_string(f);

	/*Menu 12 - Network Options*/
	skip_pascal_string(f);
	for (unsigned int i = 0; i < menuInt_entries[12]; ++i)
		read_encrypted_pascal_string(h->menuInt[12][i], sizeof(h->menuInt[12][i]), f);
	skip_pascal_string(f);

	/*Menu 13 - Joystick*/
	skip_pascal_string(f);
	for (unsigned int i = 0; i < menuInt_entries[13]; ++i)
		read_encrypted_pascal_string(h->menuInt[13][i], sizeof(h->menuInt[13][i]), f);
	skip_pascal_string(f);

	/*Joystick Button Assignments*/
	skip_pascal_string(f);
	for (unsigned int i = 0; i < COUNTOF(h->joyButtonNames); ++i)
		read_encrypted_pascal_string(h->joyButtonNames[i], sizeof(h->joyButtonNames[i]), f);
	skip_pascal_string(f);

	/*SuperShips - For Super Arcade Mode*/
	skip_pascal_string(f);
	for (unsigned int i = 0; i < COUNTOF(h->superShips); ++i)
		read_encrypted_pascal_string(h->superShips[i], sizeof(h->superShips[i]), f);
	skip_pascal_string(f);

	/*SuperShips - For Super Arcade Mode*/
	skip_pascal_string(f);
	for (unsigned int i = 0; i < COUNTOF(h->specialName); ++i)
		read_encrypted_pascal_string(h->specialName[i], sizeof(h->specialName[i]), f);
	skip_pascal_string(f);

	/*Secret DESTRUCT game*/
	skip_pascal_string(f);
	for (unsigned int i = 0; i < COUNTOF(h->destructHelp); ++i)
		read_encrypted_pascal_string(h->destructHelp[i], sizeof(h->destructHelp[i]), f);
	skip_pascal_string(f);

	/*Secret DESTRUCT weapons*/
	skip_pascal_string(f);
	for (unsigned int i = 0; i < COUNTOF(h->weaponNames); ++i)
		read_encrypted_pascal_string(h->weaponNames[i], sizeof(h->weaponNames[i]), f);
	skip_pascal_string(f);

	/*Secret DESTRUCT modes*/
	skip_pascal_string(f);
	for (unsigned int i = 0; i < COUNTOF(h->destructModeName); ++i)
		read_encrypted_pascal_string(h->destructModeName[i], sizeof(h->destructModeName[i]), f);
	skip_pascal_string(f);

	/*NEW: Ship Info*/
	skip_pascal_string(f);
	for (unsigned int i = 0; i < COUNTOF(h->shipInfo); ++i)
	{
		read_encrypted_pascal_string(h->shipInfo[i][0], sizeof(h->shipInfo[i][0]), f);
		read_encrypted_pascal_string(h->shipInfo[i][1], sizeof(h->shipInfo[i][1]), f);
	}
	skip_pascal_string(f);
	
#ifndef TYRIAN2000
	/*Menu 12 - Network Options*/
	skip_pascal_string(f);
	for (unsigned int i = 0; i < menuInt_entries[14]; ++i)
		read_encrypted_pascal_string(h->menuInt[14][i], sizeof(h->menuInt[14][i]), f);
#endif
	
}
