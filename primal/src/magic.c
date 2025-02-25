/*
   Modifications done by Brett Murphy to introduce character races
*/
/*
  newmagic.c
  Written by Fred Merkel
  Part of JediMUD
  Copyright (C) 1993 Trustees of The Johns Hopkins Unversity
  All Rights Reserved.
  Based on DikuMUD, Copyright (C) 1990, 1991.
*/

#include <stdio.h>
#include <string.h>
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "spells.h"
#include "handler.h"
#include "db.h"

extern struct zone_data *zone_table;
extern struct room_data *world;
extern struct obj_data *object_list;
extern struct char_data *character_list;
extern struct cha_app_type cha_app[];
extern struct int_app_type int_app[];
extern struct index_data *obj_index;
extern struct index_data *mob_index;

extern struct weather_data weather_info;
extern struct descriptor_data *descriptor_list;
extern struct zone_data *zone_table;

extern int mini_mud;

extern struct default_mobile_stats *mob_defaults;
extern char weapon_verbs[];
extern int *max_ac_applys;
extern struct apply_mod_defaults *apmd;

void clearMemory(struct char_data * ch);
void act(char *str, int i, struct char_data * c, struct obj_data * o,
	      void *vict_obj, int j);

void damage(struct char_data * ch, struct char_data * victim,
	         int damage, int weapontype);

void weight_change_object(struct obj_data * obj, int weight);
void add_follower(struct char_data * ch, struct char_data * leader);
int dice(int number, int size);
extern struct spell_info_type spell_info[];


struct char_data *read_mobile(int, int);


/*
 * Saving throws for:
 * MCTW
 *   PARA, ROD, PETRI, BREATH, SPELL
 *     Levels 0-40
 */

const byte saving_throws[2][5][41] = {

  {				/* Good (Players) */
    {90, 70, 69, 68, 67, 66, 65, 63, 61, 60, 59,	/* 0 - 10 */
       /* PARA */ 57, 55, 54, 53, 53, 52, 51, 50, 48, 46,	/* 11 - 20 */
      45, 44, 42, 40, 38, 36, 34, 32, 30, 28,	/* 21 - 30 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0},	/* 31 - 40 */

    {90, 55, 53, 51, 49, 47, 45, 43, 41, 40, 39,	/* 0 - 10 */
       /* ROD */ 37, 35, 33, 31, 30, 29, 27, 25, 23, 21,	/* 11 - 20 */
      20, 19, 17, 15, 14, 13, 12, 11, 10, 9,	/* 21 - 30 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0},	/* 31 - 40 */

    {90, 65, 63, 61, 59, 57, 55, 53, 51, 50, 49,	/* 0 - 10 */
       /* PETRI */ 47, 45, 43, 41, 40, 39, 37, 35, 33, 31,	/* 11 - 20 */
      30, 29, 27, 25, 23, 21, 19, 17, 15, 13,	/* 21 - 30 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0},	/* 31 - 40 */

    {90, 75, 73, 71, 69, 67, 65, 63, 61, 60, 59,	/* 0 - 10 */
       /* BREATH */ 57, 55, 53, 51, 50, 49, 47, 45, 43, 41,	/* 11 - 20 */
      40, 39, 37, 35, 33, 31, 29, 27, 25, 23,	/* 21 - 30 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0},	/* 31 - 40 */

    {90, 60, 58, 56, 54, 52, 50, 48, 46, 45, 44,	/* 0 - 10 */
       /* SPELL */ 42, 40, 38, 36, 35, 34, 32, 30, 28, 26,	/* 11 - 20 */
      25, 24, 22, 20, 18, 16, 14, 12, 10, 8,	/* 21 - 30 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0},	/* 31 - 40 */
  },


  {				/* Bad (NPC's) */
    {90, 70, 68, 67, 65, 62, 58, 55, 53, 52, 50,	/* 0 - 10 */
       /* PARA */ 47, 43, 40, 38, 37, 35, 32, 28, 25, 24,	/* 11 - 20 */
      23, 22, 20, 19, 17, 16, 15, 14, 13, 12,	/* 21 - 30 */
    11, 10, 9, 8, 7, 6, 5, 4, 3, 2},	/* 31 - 40 */

    {90, 80, 78, 77, 75, 72, 68, 65, 63, 62, 60,	/* 0 - 10 */
       /* ROD */ 57, 53, 50, 48, 47, 45, 42, 38, 35, 34,	/* 11 - 20 */
      33, 32, 30, 29, 27, 26, 25, 24, 23, 22,	/* 21 - 30 */
    20, 18, 16, 14, 12, 10, 8, 6, 5, 4},	/* 31 - 40 */

    {90, 75, 73, 72, 70, 67, 63, 60, 58, 57, 55,	/* 0 - 10 */
       /* PETRI */ 52, 48, 45, 43, 42, 40, 37, 33, 30, 29,	/* 11 - 20 */
      28, 26, 25, 24, 23, 21, 20, 19, 18, 17,	/* 21 - 30 */
    16, 15, 14, 13, 12, 11, 10, 9, 8, 7},	/* 31 - 40 */

    {90, 85, 83, 82, 80, 75, 70, 65, 63, 62, 60,	/* 0 - 10 */
       /* BREATH */ 55, 50, 45, 43, 42, 40, 37, 33, 30, 29,	/* 11 - 20 */
      28, 26, 25, 24, 23, 21, 20, 19, 18, 17,	/* 21 - 30 */
    16, 15, 14, 13, 12, 11, 10, 9, 8, 7},	/* 31 - 40 */

    {90, 85, 83, 82, 80, 77, 73, 70, 68, 67, 65,	/* 0 - 10 */
       /* SPELL */ 62, 58, 55, 53, 52, 50, 47, 43, 40, 39,	/* 11 - 20 */
      38, 36, 35, 34, 33, 31, 30, 29, 28, 27,	/* 21 - 30 */
    25, 23, 21, 19, 17, 15, 13, 11, 9, 7}	/* 31 - 40 */
  }
};


int mag_savingthrow(struct char_data * ch, int type)
{
  int save;

  /* negative apply_saving_throw values make saving throws better! */

  if (IS_NPC(ch))
    save = saving_throws[1][type][(int) GET_LEVEL(ch)];
  else
    save = saving_throws[0][type][(int) GET_LEVEL(ch)];

  save += GET_SAVE(ch, type);

  /* throwing a 0 is always a failure */
  if (MAX(1, save) < number(0, 99))
    return TRUE;
  else
    return FALSE;
}


/* affect_update: called from comm.c (causes spells to wear off) */
void affect_update(void)
{
  static struct affected_type *af, *next;
  static struct char_data *i;
  extern char *spell_wear_off_msg[];

  int found = 0,eq;

  for (i = character_list; i; i = i->next)
    for (af = i->affected; af; af = next) {
      next = af->next;
      if (af->duration >= 1)
	af->duration--;
      else if (af->duration == -1)
	/* No action */
	af->duration = -1;	/* GODs only! unlimited */
      else {
/* modification to check if magic eq is still worn, if not remove
 * its spell. - Vader
 */
        found = 0; /* 0 found or nothing wears off! */
        for(eq = 0; eq < NUM_WEARS; eq++)
          if(i->equipment[eq] != NULL)
            if((GET_OBJ_TYPE(i->equipment[eq]) == ITEM_MAGIC_EQ) &&
               ((GET_OBJ_VAL(i->equipment[eq],0) == af->type) ||
                (GET_OBJ_VAL(i->equipment[eq],1) == af->type) ||
                (GET_OBJ_VAL(i->equipment[eq],2) == af->type))) {
              found = 1;
              break;
              }
        if(found)
          continue;

/* quick fix by JA, spell_wear_off messages are not defined for
   the new values
 */

/* if ((af->type > 0) && (af->type <= MAX_SPELLS)) */
	if ((af->type > 0) && (af->type <= 129))
	  if (!af->next || (af->next->type != af->type) ||
	      (af->next->duration > 0))
	    if (*spell_wear_off_msg[af->type]) {
              /* DM - NOHASSLE spell set PRF_NOHASSLE off */
              if (af->type == SPELL_NOHASSLE)
                REMOVE_BIT(PRF_FLAGS(i), PRF_NOHASSLE);
	      send_to_char(spell_wear_off_msg[af->type], i);
	      send_to_char("\r\n", i);
	    }
	affect_remove(i, af);
      }
    }
}


/*
  mag_materials:
  Checks for up to 3 vnums in the player's inventory.
*/

int mag_materials(struct char_data * ch, int item0, int item1, int item2,
		      int extract, int verbose)
{
  struct obj_data *tobj;
  struct obj_data *obj0, *obj1, *obj2;

  for (tobj = ch->carrying; tobj; tobj = tobj->next_content) { 
    if ((item0 > 0) && (GET_OBJ_VNUM(tobj) == item0)) {
      obj0 = tobj;
      item0 = -1;
    } else if ((item1 > 0) && (GET_OBJ_VNUM(tobj) == item1)) {
      obj1 = tobj;
      item1 = -1;
    } else if ((item2 > 0) && (GET_OBJ_VNUM(tobj) == item2)) {
      obj2 = tobj;
      item2 = -1;
    }
  }
  if ((item0 > 0) || (item1 > 0) || (item2 > 0)) {
    if (verbose) {
      switch (number(0, 2)) {
      case 0:
	send_to_char("A wart sprouts on your nose.\r\n", ch);
	break;
      case 1:
	send_to_char("Your hair falls out in clumps.\r\n", ch);
	break;
      case 2:
	send_to_char("A huge corn develops on your big toe.\r\n", ch);
	break;
      }
    }
    return (FALSE);
  }
  if (extract) {
    if (item0 < 0) {
      obj_from_char(obj0);
      extract_obj(obj0);
    }
    if (item1 < 0) {
      obj_from_char(obj1);
      extract_obj(obj1);
    }
    if (item2 < 0) {
      obj_from_char(obj2);
      extract_obj(obj2);
    }
  }
  if (verbose) {
    send_to_char("A puff of smoke rises from your pack.\r\n", ch);
    act("A puff of smoke rises from $n's pack.", TRUE, ch, NULL, NULL, TO_ROOM);
  }
  return (TRUE);
}




/*
  Every spell that does damage comes through here.  This calculates the
  amount of damage, adds in any modifiers, determines what the saves are,
  tests for save and calls damage();
*/


void mag_damage(int level, struct char_data * ch, struct char_data * victim,
		     int spellnum, int savetype)
{
  int dam = 0;
  int tmpdam = 0;

  if (victim == NULL || ch == NULL)
    return;


  switch (spellnum) {
    /* Mostly mages */
  case SPELL_MAGIC_MISSILE:
  case SPELL_CHILL_TOUCH:	/* chill touch also has an affect */
      dam = dice(1, 8) + 1;
    break;
  case SPELL_BURNING_HANDS:
      dam = dice(3, 8) + 3;
    break;
  case SPELL_SHOCKING_GRASP:
      dam = dice(5, 8) + 5;
    break;
  case SPELL_LIGHTNING_BOLT:
      dam = dice(7, 8) + 7;
    break;
  case SPELL_COLOR_SPRAY:
      dam = dice(9, 8) + 9;
    break;
  case SPELL_FIREBALL:
      dam = dice(11, 8) + 11;
    break;
  case SPELL_PLASMA_BLAST:
      dam = dice(10, 10) + (level/2);
    break;
  case SPELL_WRAITH_TOUCH:

      if (!IS_SET(zone_table[world[ch->in_room].zone].zflag , ZN_PK_ALLOWED) && !IS_NPC(victim)){
	send_to_char("You cannot attack players in a NO_PKILL zone!\r\n", ch);
	return;
      }
      dam = dice(20,25) + (level*0.75);

      if(IS_AFFECTED(victim, AFF_SANCTUARY))
	tmpdam = dam/2;
      else
	tmpdam= dam;
      if ((GET_HIT(ch)+tmpdam)>GET_MAX_HIT(ch))
        GET_HIT(ch) = GET_MAX_HIT(ch);
      else
	GET_HIT(ch)+=tmpdam;
      break;

    /* Mostly clerics */
  case SPELL_DISPEL_EVIL:
    dam = dice(6, 8) + 6;
    if (IS_EVIL(ch)) {
      victim = ch;
      dam = GET_HIT(ch) - 1;
    } else if (IS_GOOD(victim)) {
      act("The gods protect $N.", FALSE, ch, 0, victim, TO_CHAR);
      dam = 0;
      return;
    }
    break;
  case SPELL_DISPEL_GOOD:
    dam = dice(6, 8) + 6;
    if (IS_GOOD(ch)) {
      victim = ch;
      dam = GET_HIT(ch) - 1;
    } else if (IS_EVIL(victim)) {
      act("The gods protect $N.", FALSE, ch, 0, victim, TO_CHAR);
      dam = 0;
      return;
    }
    break;


  case SPELL_CALL_LIGHTNING:
    dam = dice(7, 8) + 7;
    break;

  case SPELL_HARM:
    dam = dice(8, 8) + 8;
    break;

  case SPELL_FINGERDEATH:
    dam = dice(10, 20);
    break;

  case SPELL_ENERGY_DRAIN:
    if (GET_LEVEL(victim) <= 2)
      dam = 100;
    else
      dam = dice(1, 10);
    break;

    /* Area spells */
  case SPELL_EARTHQUAKE:
    dam = dice(2, 8) + level;
    break;

  case SPELL_WHIRLWIND:
    dam = (dice(1, 3) + 4) *  level;
    break;

  case SPELL_METEOR_SWARM:
    dam = dice(11, 8) + (level*0.75);
    break;

  case SPELL_CLOUD_KILL:
		if (GET_LEVEL(victim) <=20)
		{	
			GET_HIT(victim)=0;
			dam = 50;
		}
		else
			dam = (dice(50,16) + (level*0.75));
	break;
  }				/* switch(spellnum) */


  if (mag_savingthrow(victim, savetype))
    dam >>= 1;

  damage(ch, victim, dam, spellnum);
}


/*
  Every spell that does an affect comes through here.  This determines
  the effect, whether it is added or replacement, whether it is legal or
  not, etc.

  affect_join(vict, aff, add_dur, avg_dur, add_mod, avg_mod)
*/

void mag_affects(int level, struct char_data * ch, struct char_data * victim,
		      int spellnum, int savetype)
{

  struct affected_type af  ;
	int tmp_duration;

  if (victim == NULL || ch == NULL)
    return;

  af.type = spellnum ;
  af.bitvector = 0;
  af.modifier = 0;
  af.location = APPLY_NONE;

  switch (spellnum) {
  case SPELL_CHILL_TOUCH:
    if (mag_savingthrow(victim, savetype))
      af.duration = 1;
    else
      af.duration = 4;
    af.type = SPELL_CHILL_TOUCH;
    af.modifier = -1;
    af.location = APPLY_STR;
    affect_join(victim, &af, TRUE, FALSE, FALSE, FALSE);
    send_to_char("You feel your strength wither!\r\n", victim);
    break;
  case SPELL_ARMOR:
    af.duration = 12; 
    send_to_char("You feel someone protecting you.\r\n", victim);

    if (affected_by_spell(victim, SPELL_ARMOR))
    	af.duration = 6; 

    af.type = SPELL_ARMOR;
    af.modifier = -15;
    af.location = APPLY_AC;

    affect_join(victim, &af, TRUE, FALSE, FALSE, FALSE);
    break;
  case SPELL_SPIRIT_ARMOR:
    if (affected_by_spell(victim, SPELL_SPIRIT_ARMOR))
    {
      send_to_char("Nothing seems to happen!\r\n", ch);
      return;
    }
    af.type = SPELL_SPIRIT_ARMOR;
    af.duration = 8; 
    af.modifier = -30;
    af.location = APPLY_AC;

    affect_join(victim, &af, TRUE, FALSE, FALSE, FALSE);
    send_to_char("You feel divine forces protecting you.\r\n", victim);
    break;

  case SPELL_STONESKIN:
    if (affected_by_spell(victim, SPELL_STONESKIN))
    {
      send_to_char("Nothing seems to happen!\r\n", ch);
      return;
    }
    af.type = SPELL_STONESKIN;
    af.duration = 9;
    af.modifier = -40;
    af.location = APPLY_AC;
 
    affect_join(victim, &af, TRUE, FALSE, FALSE, FALSE);
    send_to_char("A skin of stone creates itself around you.\r\n", victim);
    break;

  case SPELL_LIGHT_SHIELD:
    if (affected_by_spell(victim, SPELL_LIGHT_SHIELD) ||
       affected_by_spell(victim, SPELL_FIRE_SHIELD) ||
       affected_by_spell(victim, SPELL_FIRE_WALL)) 
    {
      send_to_char("Nothing seems to happen!\r\n", ch);
      return;
    }
    af.type = SPELL_LIGHT_SHIELD;
    af.duration = 5;
    af.modifier = -15;
    af.location = APPLY_AC;
 
    affect_join(victim, &af, TRUE, FALSE, FALSE, FALSE);
    send_to_char("Your shield jolts as lighting bolts quiver around it's surface!\r\n", victim);
    break;

  case SPELL_FIRE_SHIELD:
    if (affected_by_spell(victim, SPELL_LIGHT_SHIELD) ||
       affected_by_spell(victim, SPELL_FIRE_SHIELD) ||
       affected_by_spell(victim, SPELL_FIRE_WALL)) 
    {
      send_to_char("Nothing seems to happen!\r\n", ch);
      return;
    }
    af.type = SPELL_FIRE_SHIELD;
    af.duration = 6;
    af.modifier = -20;
    af.location = APPLY_AC;

    affect_join(victim, &af, TRUE, FALSE, FALSE, FALSE);
    send_to_char("You feel a wave of heat as fire consumes the surface of your shield!\r\n", victim);
    break;

  case SPELL_FIRE_WALL:
    if (affected_by_spell(victim, SPELL_LIGHT_SHIELD) ||
       affected_by_spell(victim, SPELL_FIRE_SHIELD) ||
       affected_by_spell(victim, SPELL_FIRE_WALL)) 
    {
      send_to_char("Nothing seems to happen!\r\n", ch);
      return;
    }
    af.type = SPELL_FIRE_WALL;
    af.duration = 7;
    af.modifier = -20;
    af.location = APPLY_AC;

    affect_join(victim, &af, TRUE, FALSE, FALSE, FALSE);
    send_to_char("You feel a wave of heat as a wall of fire surrounds yourself!\r\n", victim);
    break;
  
  case SPELL_HASTE:
    if (affected_by_spell(victim, SPELL_HASTE))
    {
      send_to_char("Nothing seems to happen!\n\r", ch);
      return;
    }

    af.type = SPELL_HASTE;
    af.duration = 6;
		af.bitvector = AFF_HASTE;
    affect_join(victim, &af, TRUE, FALSE, FALSE, FALSE);
		send_to_char("Whoooooahhhh, what a RUSH!!!! You are speeding!\r\n", ch);
		break;
  case SPELL_BLESS:
    tmp_duration = 6;
   	send_to_char("You feel righteous.\r\n", victim);
    if (affected_by_spell(victim, SPELL_BLESS))
    	tmp_duration = 2;

    af.type = SPELL_BLESS;
    af.location = APPLY_SAVING_SPELL;
    af.modifier = -1;
		af.duration = tmp_duration;
    affect_join(victim, &af, TRUE, FALSE, FALSE, FALSE);

    af.location = APPLY_HITROLL;
    af.modifier = 2;
    af.bitvector = 0;
		af.duration = tmp_duration;
    affect_join(victim, &af, TRUE, FALSE, FALSE, FALSE);

    af.location = APPLY_DAMROLL;
    af.modifier = 1;
    af.bitvector = 0;
		af.duration = tmp_duration;
    affect_join(victim, &af, TRUE, FALSE, FALSE, FALSE);

/*    af.location = APPLY_SAVING_SPELL;
    af.modifier = -1;
		af.duration = tmp_duration;
    affect_join(victim, &af, TRUE, FALSE, FALSE, FALSE);*/

    break;
	case SPELL_HOLY_AID:
    if (affected_by_spell(victim, SPELL_HOLY_AID))
    {
      send_to_char("Nothing seems to happen!\n\r", ch);
      return;
    }
    af.type = SPELL_HOLY_AID;
    af.location = APPLY_AC;
    af.modifier = -10;
    af.duration = 7;
    affect_join(victim, &af, TRUE, FALSE, FALSE, FALSE);

    af.location = APPLY_HITROLL;
    af.modifier = 3;
    af.bitvector = 0;
    affect_join(victim, &af, TRUE, FALSE, FALSE, FALSE);

    af.location = APPLY_DAMROLL;
    af.modifier = 4;
    af.bitvector = 0;
    affect_join(victim, &af, TRUE, FALSE, FALSE, FALSE);
	
		send_to_char("Your God hears your prayer and assists you.\r\n", victim);
		break;

	case SPELL_DIVINE_PROTECTION:
		if (affected_by_spell(victim, SPELL_DIVINE_PROTECTION))
		{
      send_to_char("Nothing seems to happen!\n\r", ch);
      return;
    }
    af.type = SPELL_DIVINE_PROTECTION;
    af.location = APPLY_AC;
    af.modifier = -40;
    af.duration = 6;
    affect_join(victim, &af, TRUE, FALSE, FALSE, FALSE);
		send_to_char("You feel you deity protecting you\r\n", victim);
		break;		
		
  case SPELL_BLINDNESS:
    if (IS_AFFECTED(victim, AFF_BLIND)) {
      send_to_char("Nothing seems to happen.\r\n", ch);
      return;
    }
    if (!IS_SET(zone_table[world[ch->in_room].zone].zflag , ZN_PK_ALLOWED) && !IS_NPC(victim))
    {
      send_to_char("You are not allowed to Blind other players in this Zone.", ch);
      return;
    }
    if (MOB_FLAGGED(victim, MOB_NOBLIND)){
        send_to_char("Your victim resists.\r\n", ch);
        return;
    }

    if (mag_savingthrow(victim, savetype)) {
      send_to_char("You fail.\r\n", ch);
      return;
    }
    act("$n seems to be blinded!", TRUE, victim, 0, 0, TO_ROOM);
    send_to_char("You have been blinded!\r\n", victim);

    af.type = SPELL_BLINDNESS;
    af.location = APPLY_HITROLL;
    af.modifier = -4;
    af.duration = 2;
    af.bitvector = AFF_BLIND;
    affect_join(victim, &af, FALSE, FALSE, FALSE, FALSE);

    af.location = APPLY_AC;
    af.modifier = 40;
    affect_join(victim, &af, FALSE, FALSE, FALSE, FALSE);
    break;

  case SPELL_CURSE:
    if (mag_savingthrow(victim, savetype))
      return;
    af.type = SPELL_CURSE;
    af.duration = 1 + (GET_LEVEL(ch) >> 1);
    af.modifier = -1;
    af.location = APPLY_HITROLL;
    af.bitvector = AFF_CURSE;
    affect_join(victim, &af, TRUE, FALSE, TRUE, TRUE);

    af.modifier = -1;
    af.location = APPLY_DAMROLL;
    affect_join(victim, &af, TRUE, FALSE, TRUE, TRUE);

    act("$n briefly glows red!", FALSE, victim, 0, 0, TO_ROOM);
    act("You feel very uncomfortable.", FALSE, victim, 0, 0, TO_CHAR);
    break;

  case SPELL_DETECT_ALIGN:
    af.type = SPELL_DETECT_ALIGN;
    af.duration = 12 + level;
    af.bitvector = AFF_DETECT_ALIGN;
    affect_join(victim, &af, TRUE, FALSE, FALSE, FALSE);
    send_to_char("Your eyes tingle.\r\n", victim);
    break;

  case SPELL_DETECT_INVIS:
    af.type = SPELL_DETECT_INVIS;
    af.duration = 12 + level;
    af.bitvector = AFF_DETECT_INVIS;
    affect_join(victim, &af, TRUE, FALSE, FALSE, FALSE);
    send_to_char("Your eyes tingle.\r\n", victim);
    break;

  case SPELL_DETECT_MAGIC:
    af.type = SPELL_DETECT_MAGIC;
    af.duration = 12 + level;
    af.bitvector = AFF_DETECT_MAGIC;
    affect_join(victim, &af, TRUE, FALSE, FALSE, FALSE);
    send_to_char("Your eyes tingle.\r\n", victim);
    break;

  case SPELL_DETECT_POISON:
    if (victim == ch)
      if (IS_AFFECTED(victim, AFF_POISON))
	send_to_char("You can sense poison in your blood.\r\n", ch);
      else
	send_to_char("You feel healthy.\r\n", ch);
    else if (IS_AFFECTED(victim, AFF_POISON))
      act("You sense that $E is poisoned.", FALSE, ch, 0, victim, TO_CHAR);
    else
      act("You sense that $E is healthy.", FALSE, ch, 0, victim, TO_CHAR);
    break;

  case SPELL_INFRAVISION:
    if (IS_AFFECTED(victim, AFF_INFRAVISION))
    {
      send_to_char("Nothing seems to happen!\n\r", ch);
      return;
    }
    af.type = SPELL_INFRAVISION;
    af.duration = 36 + level;
    af.bitvector = AFF_INFRAVISION;
    affect_join(victim, &af, TRUE, FALSE, FALSE, FALSE);
    send_to_char("Your eyes glow red.\r\n", victim);
    act("$n's eyes glow red.", TRUE, victim, 0, 0, TO_ROOM);
    break;

  case SPELL_INVISIBLE:
    if (IS_AFFECTED(victim, AFF_INVISIBLE))
    {
      send_to_char("Nothing seems to happen!\n\r", ch);
      return;
    }
    if (!victim)
      victim = ch;
    act("$n slowly fades out of existence.", TRUE, victim, 0, 0, TO_NOTVICT);
    send_to_char("You vanish.\r\n", victim);

    af.type = SPELL_INVISIBLE;
    af.duration = 24 + (GET_LEVEL(ch) >> 2);
    af.modifier = -40;
    af.location = APPLY_AC;
    af.bitvector = AFF_INVISIBLE;
    affect_join(victim, &af, TRUE, FALSE, FALSE, FALSE);
    break;

  case SPELL_POISON:
    if (!mag_savingthrow(victim, savetype)) {
      af.type = SPELL_POISON;
      af.duration = GET_LEVEL(ch);
      af.modifier = -2;
      af.location = APPLY_STR;
      af.bitvector = AFF_POISON;
      affect_join(victim, &af, FALSE, FALSE, FALSE, FALSE);
      send_to_char("You feel very sick.\r\n", victim);
      act("$N gets violently ill!", TRUE, ch, NULL, victim, TO_NOTVICT);
    }
    break;

  case SPELL_PROT_FROM_GOOD:
    if (affected_by_spell(victim, SPELL_PROT_FROM_GOOD))
    {
      send_to_char("Nothing seems to happen!\n\r", ch);
      return;
    }   
    af.type = SPELL_PROT_FROM_GOOD;
    af.duration = 24;
    af.bitvector = AFF_PROTECT_GOOD;
    affect_join(victim, &af, TRUE, FALSE, FALSE, FALSE);
    send_to_char("You feel invulnerable!\r\n", victim);
    break;

  case SPELL_PROT_FROM_EVIL:
    if (affected_by_spell(victim, SPELL_PROT_FROM_EVIL))
    {
      send_to_char("Nothing seems to happen!\n\r", ch);
      return;
    }   
    af.type = SPELL_PROT_FROM_EVIL;
    af.duration = 24;
    af.bitvector = AFF_PROTECT_EVIL;
    affect_join(victim, &af, TRUE, FALSE, FALSE, FALSE);
    send_to_char("You feel invulnerable!\r\n", victim);
    break;

  case SPELL_SANCTUARY:
    act("$n is surrounded by a white aura.", TRUE, victim, 0, 0, TO_ROOM);
    act("You start glowing.", TRUE, victim, 0, 0, TO_CHAR);
    af.duration = 4;
    if (IS_AFFECTED(victim, AFF_SANCTUARY))
			af.duration = 1;

    af.type = SPELL_SANCTUARY;
    af.bitvector = AFF_SANCTUARY;
    affect_join(victim, &af, TRUE, FALSE, FALSE, FALSE);
    break;

case SPELL_DRAGON:

     if ( affected_by_spell ( victim , SPELL_DRAGON ) )
     {
      send_to_char("Nothing seems to happen!\n\r", ch);
      return;
     }

     if ( GET_LEVEL(ch) < 40 )
     {
      if ( GET_ALIGNMENT (ch) > -350 && GET_ALIGNMENT (ch) < 350 )
      {
       af.location = APPLY_AC;
       af.duration = 8;
       af.modifier = -10;
       affect_join(victim, &af, FALSE , FALSE, FALSE, FALSE);
      }
      
      if ( GET_ALIGNMENT (ch) >= 350 )
      {
       af.location = APPLY_HITROLL;
       af.duration = 8;
       af.modifier = 1;
       affect_join(victim, &af, FALSE, FALSE, FALSE, FALSE); 
      }

      if ( GET_ALIGNMENT (ch) <= -350 )
      {
       af.location = APPLY_DAMROLL;
       af.duration = 8;
       af.modifier = 1;     
       affect_join(victim, &af, FALSE  , FALSE, FALSE, FALSE);
      }
   
    }

     if ( GET_LEVEL(ch) >= 40 && GET_LEVEL( ch ) <= 60 )
     {
      if ( GET_ALIGNMENT (ch) > -350 && GET_ALIGNMENT (ch) < 350 )
      {
       af.location = APPLY_AC;
       af.duration = 9;
       af.modifier = -15;
       affect_join(victim, &af, FALSE , FALSE, FALSE, FALSE);
      }

      if ( GET_ALIGNMENT (ch) >= 350 )
      {
       af.location = APPLY_HITROLL;
       af.duration = 9;
       af.modifier = 2;
       affect_join(victim, &af , FALSE, FALSE, FALSE, FALSE);
      } 

      if ( GET_ALIGNMENT (ch) <= -350 )
      {
       af.location = APPLY_DAMROLL;
       af.duration = 9;
       af.modifier = 2;
       affect_join(victim, &af, FALSE, FALSE, FALSE, FALSE);
      }
     }

     if (  GET_LEVEL( ch ) > 60 )
     {
      if ( GET_ALIGNMENT (ch) > -350 && GET_ALIGNMENT (ch) < 350 )
      {
       af.location = APPLY_AC;
       af.modifier = -20;
       af.duration = 10;
       af.bitvector = AFF_FLY;
       affect_join(victim, &af ,FALSE , FALSE, FALSE, FALSE);
      }

      if ( GET_ALIGNMENT (ch) >= 350 )
      {
       af.location = APPLY_HITROLL;
       af.modifier = 3;
       af.duration = 10;
       af.bitvector = AFF_FLY;
       affect_join(victim, &af, FALSE , FALSE ,FALSE, FALSE);
      }

      if ( GET_ALIGNMENT (ch) <= -350 )
      {      
       af.location = APPLY_DAMROLL;
       af.modifier = 3;
       af.duration = 10;
       af.bitvector = AFF_FLY;
       affect_join(victim, &af, FALSE, FALSE, FALSE, FALSE);
      }

    }

    send_to_char ( "You feel the blood of a dragon coursing through your veins.\n\r" , ch ) ;
    act("$n looks more like a dragon now.", TRUE, victim, 0, 0, TO_ROOM);

     break ;

  case SPELL_SLEEP:
    if (!IS_SET(zone_table[world[ch->in_room].zone].zflag , ZN_PK_ALLOWED) && !IS_NPC(victim))
      return;
    if (MOB_FLAGGED(victim, MOB_NOSLEEP)){
	send_to_char("Your victim resists.\r\n", ch);
	return;
    }
    if (mag_savingthrow(victim, savetype))
      return;

    af.type = SPELL_SLEEP;
    af.duration = 4 + (GET_LEVEL(ch) >> 2);
    af.location = APPLY_NONE;
    af.bitvector = AFF_SLEEP;
    affect_join(victim, &af, FALSE, FALSE, FALSE, FALSE);
    if (GET_POS(victim) > POS_SLEEPING) {
      act("You feel very sleepy...zzzzzz", FALSE, victim, 0, 0, TO_CHAR);
      act("$n goes to sleep.", TRUE, victim, 0, 0, TO_ROOM);
      GET_POS(victim) = POS_SLEEPING;
    }
    break;

  case SPELL_STRENGTH:
    send_to_char("You feel stronger!\r\n", victim);
    af.type = SPELL_STRENGTH;
    af.duration = (GET_LEVEL(ch) >> 1) + 4;
    af.modifier = 1 + (level > 18);
    af.location = APPLY_STR;
    affect_join(victim, &af, TRUE, TRUE, FALSE, FALSE);
    break;

  case SPELL_SENSE_LIFE:
    if (IS_AFFECTED(victim, AFF_SENSE_LIFE))
    {
      send_to_char("Nothing seems to happen!\n\r", ch);
      return;
    }
    if (!victim)
    victim = ch;
    af.type = SPELL_SENSE_LIFE;
    af.duration = GET_LEVEL(ch)/2;
    af.bitvector = AFF_SENSE_LIFE;
    affect_join(victim, &af, TRUE, FALSE, FALSE, FALSE);
    send_to_char("Your feel your awareness improve.\r\n", victim);
    act("$n's awareness improves.", TRUE, victim, 0, 0, TO_ROOM);
    break;

  case SPELL_WATERWALK:
    if (IS_AFFECTED(victim, AFF_WATERWALK))
    {
      send_to_char("Nothing seems to happen!\n\r", ch);
      return;
    }
    af.type = SPELL_WATERWALK;
    af.duration = 24;
    af.bitvector = AFF_WATERWALK;
    affect_join(victim, &af, TRUE, FALSE, FALSE, FALSE);
    send_to_char("You feel webbing between your toes.\r\n", victim);
    break;

  case SPELL_SERPENT_SKIN:
    if(IS_AFFECTED(victim, AFF_REFLECT)) {
      send_to_char("Nothing seems to happen!\r\n",ch);
      return;
      }
    if(mag_materials(ch,8400,0,0,1,1)) {
      af.type = SPELL_SERPENT_SKIN;
      af.duration = 3;
      af.bitvector = AFF_REFLECT;
      act("Your skin begins to sparkle!",FALSE,victim,0,0,TO_CHAR);
      act("Shiny scales appear on $n's skin!",FALSE,victim,0,0,TO_ROOM);
      affect_join(victim,&af,TRUE,FALSE,FALSE,FALSE);
    } else {
      send_to_char("You seem to missing a major ingredient...\r\n",ch);
      return;
      }
    break;
 
  case SPELL_NOHASSLE:
    if (IS_AFFECTED(victim, AFF_NOHASSLE)) {
      send_to_char("Nothing seems to happen!\r\n",ch);
      return;
    }
    af.type = SPELL_NOHASSLE;
    af.duration = 10;
    af.bitvector = AFF_NOHASSLE;
    SET_BIT(PRF_FLAGS(ch),PRF_NOHASSLE);
    send_to_char("You start to feel untouchable.\r\n",ch);
    act("$n starts to look untouchable.",FALSE,ch,0,0,TO_ROOM);
    affect_join(victim,&af,TRUE,FALSE,FALSE,FALSE);
    break;

  case SPELL_FLY:
    if(IS_AFFECTED(victim,AFF_FLY)) {
      send_to_char("Nothing seems to happen.\r\n",ch);
      return;
      }
    af.type = SPELL_FLY;
    af.duration = 8;
    af.bitvector = AFF_FLY;
    send_to_char("You begin to float off the ground.\r\n",victim);
    act("$n begins to float off the ground.",FALSE,victim,0,0,TO_ROOM);
    affect_join(victim,&af,TRUE,FALSE,FALSE,FALSE);
    break;

  case SPELL_PARALYZE:
    if (!IS_SET(zone_table[world[ch->in_room].zone].zflag , ZN_PK_ALLOWED) && !IS_NPC(victim))
      return;

    if (IS_AFFECTED(victim, AFF_PARALYZED))
    {
      send_to_char("They are already paralized!.\r\n", ch);
      return;
    }
    if (!mag_savingthrow(victim, savetype)) {
      af.type = SPELL_PARALYZE;
      af.duration = 5;
      af.bitvector = AFF_PARALYZED;
      affect_join(victim, &af, FALSE, FALSE, FALSE, FALSE);
      send_to_char("You feel your muscles tense up and lock.  You are paralyzed!!\r\n", victim);
      act("$n's legs completely freeze up.  $n looks pretty paralyzed!", FALSE, victim, 0, 0, TO_ROOM);

    }
    break;
  case SPELL_WATERBREATHE:
    if(IS_AFFECTED(victim,AFF_WATERBREATHE)) {
      send_to_char("Nothing seems to happen.\r\n",ch);
      return;
      }
    af.type = SPELL_WATERBREATHE;
    af.duration = GET_LEVEL(ch)/2;
    af.bitvector = AFF_WATERBREATHE;
    send_to_char("A pair of magical gills appear on your neck.\r\n",victim);
    act("A pair of magical gills appear on $n's neck.", FALSE,victim,0,0,TO_ROOM);
    affect_join(victim,&af,TRUE,FALSE,FALSE,FALSE);
    break;

  }
}


void mag_group_switch(int level, struct char_data * ch, struct char_data * tch,
		           int spellnum, int savetype)
{
  switch (spellnum) {
    case SPELL_GROUP_HEAL:
    mag_points(level, ch, tch, SPELL_HEAL, savetype);
    break;
  case SPELL_GROUP_ARMOR:
    mag_affects(level, ch, tch, SPELL_ARMOR, savetype);
    break;
  case SPELL_GROUP_RECALL:
    spell_recall(level, ch, tch, NULL);
    break;
  case SPELL_GROUP_SANCTUARY:
    mag_affects(level, ch, tch, SPELL_SANCTUARY, savetype);
    break;
  }
}

/*
  Every spell that affects the group should run through here
  mag_group_switch contains the switch statement to send us to the right
  magic.

  group spells affect everyone grouped with the caster who is in the room.
*/

void mag_groups(int level, struct char_data * ch, int spellnum, int savetype)
{
  struct char_data *tch, *k;
  struct follow_type *f, *f_next;

  if (ch == NULL)
    return;

  if (!IS_AFFECTED(ch, AFF_GROUP))
    return;
  if (ch->master != NULL)
    k = ch->master;
  else
    k = ch;
  for (f = k->followers; f; f = f_next) {
    f_next = f->next;
    tch = f->follower;
    if (tch->in_room != ch->in_room)
      continue;
    if (!IS_AFFECTED(tch, AFF_GROUP))
      continue;
    if(tch == ch)
      continue;
    mag_group_switch(level, ch, tch, spellnum, savetype);
  }
  if ((k != ch) && IS_AFFECTED(k, AFF_GROUP))
    mag_group_switch(level, ch, k, spellnum, savetype);
  mag_group_switch(level, ch, ch, spellnum, savetype);
}


/*
  mass spells affect every creature in the room except the caster.
*/

void mag_masses(int level, struct char_data * ch, int spellnum, int savetype)
{
  struct char_data *tch, *tch_next;

  for (tch = world[ch->in_room].people; tch; tch = tch_next) {
    tch_next = tch->next_in_room;
    if (tch == ch)
      continue;

    switch (spellnum) {
    }
  }
}


/*
  Every spell that affects an area (room) runs through here.  These are
  generally offensive spells.  This calls mag_damage to do the actual
  damage.

  area spells have limited targets within the room.
*/

void mag_areas(byte level, struct char_data * ch, int spellnum, int savetype)
{
  struct char_data *tch, *next_tch, *m;
  int skip;
  SPECIAL(postmaster);
  SPECIAL(receptionist);
  extern struct index_data *mob_index;


  if (ch == NULL)
    return;


  if (ch->master != NULL)
    m = ch->master;
  else
    m = ch;

  for (tch = world[ch->in_room].people; tch; tch = next_tch) {
    next_tch = tch->next_in_room;


    /*
     * The skips: 1) immortals && self 2) mobs only hit charmed mobs 3)
     * players can only hit players in CRIMEOK rooms 4) players can only hit
     * charmed mobs in CRIMEOK rooms
     */

    skip = 0;
    if (tch == ch)
      skip = 1;
    if ((GET_LEVEL(ch) < LVL_GOD)&&(GET_MOB_SPEC(tch) == postmaster))
      skip = 1;
    if ((GET_LEVEL(ch) < LVL_GOD)&&(GET_MOB_SPEC(tch) == receptionist))
      skip = 1;
    if (IS_NPC(ch) && IS_NPC(tch) && !IS_AFFECTED(tch, AFF_CHARM))
      skip = 1;
    if (!IS_NPC(tch) && GET_LEVEL(tch) >= LVL_IS_GOD)
      skip = 1;
    if (!IS_NPC(ch) && !IS_NPC(tch))
      skip = 1;
    if (!IS_NPC(ch) && IS_NPC(tch) && IS_AFFECTED(tch, AFF_CHARM))
      skip = 1;

    if (skip)
      continue;

    switch (spellnum) {
    case SPELL_EARTHQUAKE:
      mag_damage(GET_LEVEL(ch), ch, tch, spellnum, 1);
      break;
    case SPELL_WHIRLWIND:
      mag_damage(GET_LEVEL(ch), ch, tch, spellnum, 1);
      break;
    case SPELL_METEOR_SWARM:
      mag_damage(GET_LEVEL(ch), ch, tch, spellnum, 1);
      break;
    case SPELL_CLOUD_KILL:
      mag_damage(GET_LEVEL(ch), ch, tch, spellnum, 1);
      break;
    default:
      return;
    }
  }
}


/*
  Every spell which summons/gates/conjours a mob comes through here.
*/

static char *mag_summon_msgs[] = {
  "\r\n",
  "$n makes a strange magical gesture; you feel a strong breeze!",
  "$n animates a corpse!",
  "$N appears from a cloud of thick blue smoke!",
  "$N appears from a cloud of thick green smoke!",
  "$N appears from a cloud of thick red smoke!",
  "$N disappears in a thick black cloud!"
  "As $n makes a strange magical gesture, you feel a strong breeze.",
  "As $n makes a strange magical gesture, you feel a searing heat.",
  "As $n makes a strange magical gesture, you feel a sudden chill.",
  "As $n makes a strange magical gesture, you feel the dust swirl.",
  "With a hideous tearing sound, $n splits in two!",
  "$p jumps to life!"
};

static char *mag_summon_fail_msgs[] = {
  "\r\n",
  "There are no such creatures.\r\n",
  "Uh oh...\r\n",
  "Oh dear.\r\n",
  "Oh shit!\r\n",
  "The elements resist!\r\n",
  "You failed.\r\n",
  "That aint no corpse!"
};

void mag_summons(int level, struct char_data * ch, struct obj_data * obj,
		      int spellnum, int savetype)
{
  struct char_data *mob;
  struct obj_data *tobj, *next_obj;
  struct follow_type *fol;
  int pfail = 0;
  int msg = 0, fmsg = 6;
  int num = 1;
  int i;
  int counter = 0;
  int mob_num = 0;
  int handle_corpse = 0;
  char buf[MAX_STRING_LENGTH];

  if (ch == NULL)
    return;

  switch (spellnum) {
  case SPELL_ANIMATE_DEAD:
    if ((obj == NULL) || (GET_OBJ_TYPE(obj) != ITEM_CONTAINER) ||
	(!GET_OBJ_VAL(obj, 3)) || (GET_OBJ_VNUM(obj) == 22300)) {
      act(mag_summon_fail_msgs[7], FALSE, ch, 0, 0, TO_CHAR);
      return;
    }
    handle_corpse = 1;
    msg = 11;
    mob_num = MOB_ZOMBIE;
/*    a = number(0, 5);
    if (a)
      mob_num++;
*/    pfail = 8;
    break;

  case SPELL_CLONE:
    handle_corpse = 0;
    msg = 10;
    mob_num = MOB_CLONE;
    pfail = 8;
/* for(temp = world[ch->in_room].people; temp; temp = temp->next_in_room) { */

    /* DM - Check follower list for number of clones - they always fol master */
    for (fol = ch->followers; fol; fol = fol->next) {
      if(IS_CLONE(fol->follower) && fol->follower->master == ch)
	counter++;
      }
    if(counter >= 2) {
      send_to_char("You dont wanna risk fiddling with your DNA any further...\r\n",ch);
      return;
      }
    break;

  default:
    return;
  }

  if(spellnum == SPELL_ANIMATE_DEAD)
    if(!strcmp("the zombie",&obj->short_description[14])) {
      send_to_char("Hmmm, I think this corpse has seen enough action already...\r\n",ch);
      return;
      }

  if (IS_AFFECTED(ch, AFF_CHARM)) {
    send_to_char("You are too giddy to have any followers!\r\n", ch);
    return;
  }
  if (number(0, 101) < pfail) {
    send_to_char(mag_summon_fail_msgs[fmsg], ch);
    return;
  }
  for (i = 0; i < num; i++) {
  switch (spellnum) {
  case SPELL_ANIMATE_DEAD:
    sprintf(buf,"The zombie of %s is standing here, looking undead.\r\n",&obj->short_description[14]);
    mob = read_mobile(MOB_ZOMBIE, VIRTUAL);
    mob->player.long_descr = str_dup(buf);
    GET_LEVEL(mob) = GET_OBJ_COST(obj);
    GET_MAX_HIT(mob) = dice(10,GET_LEVEL(mob)) + GET_LEVEL(mob);
    GET_HIT(mob) = GET_MAX_HIT(mob);
    mob->char_specials.timer = number(1,5);
    break;
  case SPELL_CLONE:
    sprintf(buf,"%s %s &yis standing here.\r\n",GET_NAME(ch),ch->player.title);
    mob = read_mobile(MOB_CLONE, VIRTUAL);
    mob->player.long_descr = str_dup(buf);
    mob->player.short_descr = str_dup(GET_NAME(ch));
    sprintf(buf,"If you didn't know any better you'd think this was the real %s...\r\n",GET_NAME(ch));
    mob->player.description = str_dup(buf);
    GET_SEX(mob) = GET_SEX(ch);
    GET_MAX_HIT(mob) = GET_MAX_HIT(ch);
    GET_HIT(mob) = GET_MAX_HIT(mob);
    GET_LEVEL(mob) = GET_LEVEL(ch);
    GET_AFF_STR(mob) = GET_AFF_STR(ch);
    GET_AFF_DEX(mob) = GET_AFF_DEX(ch);
    GET_HITROLL(mob) = GET_HITROLL(ch);
    GET_DAMROLL(mob) = GET_DAMROLL(ch);
    GET_AC(mob) = GET_AC(ch);
    SET_BIT(MOB_FLAGS(ch), MOB_NOCHARM);
    break;
  default:
    return;
  }

    act(mag_summon_msgs[msg], FALSE, ch, obj, mob, TO_ROOM);
    char_to_room(mob, ch->in_room);
    IS_CARRYING_W(mob) = 0;
    IS_CARRYING_N(mob) = 0;
    SET_BIT(AFF_FLAGS(mob), AFF_CHARM);
    add_follower(mob, ch);
/*    if (spellnum == SPELL_CLONE) {
      strcpy(GET_NAME(mob), GET_NAME(ch));
      strcpy(mob->player.short_descr, GET_NAME(ch));
    }
*/  }
  if (handle_corpse) {
    for (tobj = obj->contains; tobj; tobj = next_obj) {
      next_obj = tobj->next_content;
      obj_from_obj(tobj);
      obj_to_char(tobj, mob);
    }
    extract_obj(obj);
  }
}


void mag_points(int level, struct char_data * ch, struct char_data * victim,
		     int spellnum, int savetype)
{
  int hit = 0;
  int move = 0;
  int mana = 0;

  if (victim == NULL)
    return;

  switch (spellnum) {
  case SPELL_CURE_LIGHT:
    hit = dice(1, 8) + 1 + (level >> 2);
    send_to_char("You feel better.\r\n", victim);
    break;
  case SPELL_CURE_CRITIC:
    hit = dice(3, 8) + 3 + (level >> 2);
    send_to_char("You feel a lot better!\r\n", victim);
    break;
  case SPELL_HEAL:
    hit = 100 + dice(3, 8);
    send_to_char("A warm feeling floods your body.\r\n", victim);
    break;
  case SPELL_ADV_HEAL:
    hit = 200 + level;
    send_to_char("You feel your wounds heal!\r\n", victim);
    break;
  case SPELL_DIVINE_HEAL:
    hit = 300 + level;
    send_to_char("A divine feeling floods your body!\r\n",victim);
    break;
  case SPELL_REFRESH:
    move = 50 + dice(3, 8);
    send_to_char("You feel refreshed.\r\n", victim);
    break;
  case SPELL_MANA:
    mana = 100 + dice(3, 8);
    send_to_char("Your skin tingles as magical energy surges through your body.\r\n", victim);
    break;
  }
  GET_HIT(victim) = MIN(GET_MAX_HIT(victim), GET_HIT(victim) + hit);
  GET_MOVE(victim) = MIN(GET_MAX_MOVE(victim), GET_MOVE(victim) + move);
  GET_MANA(victim) = MIN(GET_MAX_MANA(victim), GET_MANA(victim) + mana);
}


void mag_unaffects(int level, struct char_data * ch, struct char_data * victim,
			int spellnum, int type)
{
  int spell = 0;
  char *to_vict = NULL, *to_room = NULL;

  if (victim == NULL)
    return;

  switch (spellnum) {
  case SPELL_CURE_BLIND:
    spell = SPELL_BLINDNESS;
    to_vict = "Your vision returns!";
    break;
  case SPELL_HEAL:
    spell = SPELL_BLINDNESS;
    break;
  case SPELL_ADV_HEAL:
    spell = SPELL_BLINDNESS;
    break;
  case SPELL_DIVINE_HEAL:
    spell = SPELL_BLINDNESS;
    break;
  case SPELL_REMOVE_POISON:
    spell = SPELL_POISON;
    to_vict = "A warm feeling runs through your body!";
    to_room = "$n looks better.";
    break;
  case SPELL_REMOVE_CURSE:
    spell = SPELL_CURSE;
    to_vict = "You don't feel so unlucky.";
    break;
  case SPELL_REMOVE_PARA:
    spell = SPELL_PARALYZE;
    to_vict = "Your muscles suddenly relax you feel like u can move again!";
    to_room = "$n stumbles slightly as $s legs start working again.";
    break;
  default:
    sprintf(buf, "SYSERR: unknown spellnum %d passed to mag_unaffects", spellnum);
    log(buf);
    return;
    break;
  }

  if (affected_by_spell(victim, spell)) {
    affect_from_char(victim, spell);
    if (to_vict != NULL) {
      send_to_char(to_vict, victim);
      send_to_char("\r\n", victim);
    }
    if (to_room != NULL)
      act(to_room, TRUE, victim, NULL, NULL, TO_ROOM);
  } else if (to_vict != NULL)
    send_to_char("Nothing seems to happen.\r\n", ch);
}


void mag_alter_objs(int level, struct char_data * ch, struct obj_data * obj,
		         int spellnum, int savetype)
{
  char *to_char = NULL;
  char *to_room = NULL;
 
  if (obj == NULL)
    return;

  switch (spellnum) {

  case SPELL_REMOVE_CURSE:
      if (IS_OBJ_STAT(obj, ITEM_NODROP)) {
        REMOVE_BIT(obj->obj_flags.extra_flags, ITEM_NODROP);
        if (GET_OBJ_TYPE(obj) == ITEM_WEAPON)
          GET_OBJ_VAL(obj, 2)++;
        to_char = "$p briefly glows blue.";
      }
      break;
  }
if (to_char == NULL)
         send_to_char(NOEFFECT, ch);
       else     
    act(to_char, TRUE, ch, obj, 0, TO_CHAR);                                                                  
  if (to_room != NULL)
         act(to_room, TRUE, ch, obj, 0, TO_ROOM);

  else if (to_char != NULL)
         act(to_char, TRUE, ch, obj, 0, TO_ROOM);
  
}


void mag_objects(int level, struct char_data * ch, struct obj_data * obj,
		      int spellnum)
{
  switch (spellnum) {
    case SPELL_CREATE_WATER:
      if (GET_OBJ_TYPE(obj) != ITEM_DRINKCON)
      {  
        send_to_char("You cant fill that with water!\r\n",ch);
        return;
      }   
      GET_OBJ_VAL(obj,1)=MIN(GET_OBJ_VAL(obj,1)+dice(2,5)+10,GET_OBJ_VAL(obj,0)); 
      GET_OBJ_VAL(obj,2)=0;
    break;
  }
}



void mag_creations(int level, struct char_data * ch, int spellnum)
{
  struct obj_data *tobj;
  int z;

  if (ch == NULL)
    return;
  level = MAX(MIN(level, LVL_IMPL), 1);

  switch (spellnum) {
  case SPELL_CREATE_FOOD:
    z = 10016;
    break;
  default:
    send_to_char("Spell unimplemented, it would seem.\r\n", ch);
    return;
    break;
  }

  if (!(tobj = read_object(z, VIRTUAL))) {
    send_to_char("I seem to have goofed.\r\n", ch);
    sprintf(buf, "SYSERR: spell_creations, spell %d, obj %d: obj not found",
	    spellnum, z);
    log(buf);
    return;
  }
  obj_to_char(tobj, ch);
  act("$n creates $p.", FALSE, ch, tobj, 0, TO_ROOM);
  act("You create $p.", FALSE, ch, tobj, 0, TO_CHAR);
}
