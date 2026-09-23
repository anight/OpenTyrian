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
#include "episodes.h"
#include "file.h"
#include "lvllib.h"
#include "lvlmast.h"
#include "opentyr.h"
#include "varz.h"


/* MAIN Weapons Data, in flash; see episodes.h */
const JE_WeaponPortItem *weaponPort;
const JE_WeaponType     *weapons;

/* Items */
const JE_PowerItem   *powerSys;
const JE_ShipItem    *ships;
const JE_OptionType  *options;
const JE_ShieldItem  *shields;
const JE_SpecialItem *special;

/* Enemy data */
const JE_EnemyDatItem *enemyDatTable;
JE_EnemyDatItem enemyDat0;

/* EPISODE variables */
JE_byte    initial_episode_num, episodeNum = 0;
JE_boolean episodeAvail[EPISODE_MAX]; /* [1..episodemax] */
char       episode_file[22], cube_file[22];

/* Tells the game whether the level currently loaded is a bonus level. */
JE_boolean bonusLevel;

/* Tells if the game jumped back to Episode 1 */
JE_boolean jumpBackToEpisode1;

static void set_item_tables( const JE_ItemTables *t )
{
	weapons    = t->weapons;
	weaponPort = t->weaponPort;
	special    = t->special;
	powerSys   = t->powerSys;
	ships      = t->ships;
	options    = t->options;
	shields    = t->shields;

	enemyDatTable = t->enemyDat;
	enemyDat0 = t->enemyDat[0];
}

void JE_loadItemDat( void )
{
	const JE_ItemTables *t = ty_items[episodeNum <= 3 ? 0 : 1];

	if (t == NULL)
	{
		fprintf(stderr, "error: episode %d's item data was not converted into this firmware\n", episodeNum);
		JE_tyrianHalt(1);
	}

	set_item_tables(t);
}

void JE_initEpisode( JE_byte newEpisode )
{
	if (newEpisode == episodeNum)
		return;
	
	episodeNum = newEpisode;
	
	sprintf(levelFile,    "tyrian%d.lvl",  episodeNum);
	sprintf(cube_file,    "cubetxt%d.dat", episodeNum);
	sprintf(episode_file, "levels%d.dat",  episodeNum);
	
	JE_analyzeLevel();
	JE_loadItemDat();
}

void JE_scanForEpisodes( void )
{
	/* The item tables are read before any episode is loaded - the player's
	 * starting armour comes from ships[] - and when they were arrays that
	 * read zeros.  Pointers must point somewhere, and the first episode's
	 * set is what every later pass through the title screen sees anyway. */
	set_item_tables(ty_items[0] != NULL ? ty_items[0] : ty_items[1]);

	for (int i = 0; i < EPISODE_MAX; ++i)
	{
		char ep_file[22];
		snprintf(ep_file, sizeof(ep_file), "tyrian%d.lvl", i + 1);
		episodeAvail[i] = dir_file_exists(data_dir(), ep_file);
	}
}

unsigned int JE_findNextEpisode( void )
{
	unsigned int newEpisode = episodeNum;
	
	jumpBackToEpisode1 = false;
	
	while (true)
	{
		newEpisode++;
		
		if (newEpisode > EPISODE_MAX)
		{
			newEpisode = 1;
			jumpBackToEpisode1 = true;
			gameHasRepeated = true;
		}
		
		if (episodeAvail[newEpisode-1] || newEpisode == episodeNum)
		{
			break;
		}
	}
	
	return newEpisode;
}

