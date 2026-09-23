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
#ifndef EPISODES_H
#define EPISODES_H

#include "opentyr.h"

#include "file.h"
#include "lvlmast.h"


/* Episodes and general data */

#define FIRST_LEVEL 1
#define EPISODE_MAX 5
#ifdef TYRIAN2000
#define EPISODE_AVAILABLE 5
#else
#define EPISODE_AVAILABLE 4
#endif

typedef struct
{
	JE_word     drain;
	JE_byte     shotrepeat;
	JE_byte     multi;
	JE_word     weapani;
	JE_byte     max;
	JE_byte     tx, ty, aim;
	JE_byte     attack[8], del[8]; /* [1..8] */
	JE_shortint sx[8], sy[8]; /* [1..8] */
	JE_shortint bx[8], by[8]; /* [1..8] */
	JE_word     sg[8]; /* [1..8] */
	JE_shortint acceleration, accelerationx;
	JE_byte     circlesize;
	JE_byte     sound;
	JE_byte     trail;
	JE_byte     shipblastfilter;
} JE_WeaponType;

typedef struct
{
	char    name[31]; /* string [30] */
	JE_byte opnum;
	JE_word op[2][11]; /* [1..2, 1..11] */
	JE_word cost;
	JE_word itemgraphic;
	JE_word poweruse;
} JE_WeaponPortItem;
typedef JE_WeaponPortItem JE_WeaponPortType[PORT_NUM + 1]; /* [0..portnum] */

typedef struct
{
	char        name[31]; /* string [30] */
	JE_word     itemgraphic;
	JE_byte     power;
	JE_shortint speed;
	JE_word     cost;
} JE_PowerItem;
typedef JE_PowerItem JE_PowerType[POWER_NUM + 1]; /* [0..powernum] */

typedef struct
{
	char    name[31]; /* string [30] */
	JE_word itemgraphic;
	JE_byte pwr;
	JE_byte stype;
	JE_word wpn;
} JE_SpecialItem;
typedef JE_SpecialItem JE_SpecialType[SPECIAL_NUM + 1]; /* [0..specialnum] */

typedef struct
{
	char        name[31]; /* string [30] */
	JE_byte     pwr;
	JE_word     itemgraphic;
	JE_word     cost;
	JE_byte     tr, option;
	JE_shortint opspd;
	JE_byte     ani;
	JE_word     gr[20]; /* [1..20] */
	JE_byte     wport;
	JE_word     wpnum;
	JE_byte     ammo;
	JE_boolean  stop;
	JE_byte     icongr;
} JE_OptionType;

typedef struct
{
	char    name[31]; /* string [30] */
	JE_byte tpwr;
	JE_byte mpwr;
	JE_word itemgraphic;
	JE_word cost;
} JE_ShieldItem;
typedef JE_ShieldItem JE_ShieldType[SHIELD_NUM + 1]; /* [0..shieldnum] */

typedef struct
{
	char        name[31]; /* string [30] */
	JE_word     shipgraphic;
	JE_word     itemgraphic;
	JE_byte     ani;
	JE_shortint spd;
	JE_byte     dmg;
	JE_word     cost;
	JE_byte     bigshipgraphic;
} JE_ShipItem;
typedef JE_ShipItem JE_ShipType[SHIP_NUM + 1]; /* [0..shipnum] */

/* EnemyData */
typedef struct
{
	JE_byte     ani;
	JE_byte     tur[3]; /* [1..3] */
	JE_byte     freq[3]; /* [1..3] */
	JE_shortint xmove;
	JE_shortint ymove;
	JE_shortint xaccel;
	JE_shortint yaccel;
	JE_shortint xcaccel;
	JE_shortint ycaccel;
	JE_integer  startx;
	JE_integer  starty;
	JE_shortint startxc;
	JE_shortint startyc;
	JE_byte     armor;
	JE_byte     esize;
	JE_word     egraphic[20];  /* [1..20] */
	JE_byte     explosiontype;
	JE_byte     animate;       /* 0:Not Yet   1:Always   2:When Firing Only */
	JE_byte     shapebank;     /* See LEVELMAK.DOC */
	JE_shortint xrev, yrev;
	JE_word     dgr;
	JE_shortint dlevel;
	JE_shortint dani;
	JE_byte     elaunchfreq;
	JE_word     elaunchtype;
	JE_integer  value;
	JE_word     eenemydie;
} JE_EnemyDatItem;
typedef JE_EnemyDatItem JE_EnemyDatType[ENEMY_NUM + 1]; /* [0..enemynum] */

/*
 * The item and enemy definitions, one set for episodes 1 to 3 and another for
 * episode 4.
 *
 * Nothing writes them once they are loaded, so they are not loaded: the build
 * runs JE_readItemDat() on the host and compiles what it read into flash
 * (tools/assets/gamedata_gen.c), and JE_initEpisode() points these at the set
 * the episode uses.  140 KB that would otherwise be RAM.
 */
typedef struct
{
	JE_WeaponType     weapons[WEAP_NUM + 1];
	JE_WeaponPortType weaponPort;
	JE_SpecialType    special;
	JE_PowerType      powerSys;
	JE_ShipType       ships;
	JE_OptionType     options[OPTION_NUM + 1];
	JE_ShieldType     shields;
	JE_EnemyDatType   enemyDat;
} JE_ItemTables;

extern const JE_WeaponPortItem *weaponPort;
extern const JE_WeaponType     *weapons;
extern const JE_PowerItem      *powerSys;
extern const JE_ShipItem       *ships;
extern const JE_OptionType     *options;
extern const JE_ShieldItem     *shields;
extern const JE_SpecialItem    *special;

/*
 * enemyDat[0] is the exception.  The events that make an enemy to order write
 * its armour and graphic into entry 0 and then create it from there, so that
 * one entry is kept in RAM and reads go through here to find it.
 */
extern JE_EnemyDatItem enemyDat0;
extern const JE_EnemyDatItem *enemyDatTable;

static inline const JE_EnemyDatItem *enemy_dat( unsigned int i )
{
	return i == 0 ? &enemyDat0 : &enemyDatTable[i];
}

/* Reads an episode's item data from f, positioned at its start, into t: the
 * game's loader, which now runs on the host at build time. */
void JE_readItemDat( VFILE *f, JE_ItemTables *t, JE_byte episode );

/* The sets tools/assets/gamedata_gen.c compiled: [0] for episodes 1 to 3,
 * [1] for episode 4, NULL where the episode was not converted. */
extern const JE_ItemTables *const ty_items[2];

extern JE_byte initial_episode_num, episodeNum;
extern JE_boolean episodeAvail[EPISODE_MAX];

extern char episode_file[22], cube_file[22];

extern JE_boolean bonusLevel;
extern JE_boolean jumpBackToEpisode1;

void JE_loadItemDat( void );
void JE_initEpisode( JE_byte newEpisode );
unsigned int JE_findNextEpisode( void );
void JE_scanForEpisodes( void );

#endif /* EPISODES_H */

