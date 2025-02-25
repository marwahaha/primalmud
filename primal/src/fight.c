/**************************************************************************
*   File: fight.c                                       Part of CircleMUD *
*  Usage: Combat system                                                   *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
/*
   Modifications done by Brett Murphy to introduce character races
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "handler.h"
#include "interpreter.h"
#include "db.h"
#include "spells.h"
#include "screen.h"

/* Structures */
struct char_data *combat_list = NULL;	/* head of l-list of fighting chars */
struct char_data *next_combat_list = NULL;

/* External structures */
extern struct zone_data *zone_table;
extern struct room_data *world;
extern struct message_list fight_messages[MAX_MESSAGES];
extern struct obj_data *object_list;
extern int auto_save;		/* see config.c */
extern int max_exp_gain;	/* see config.c */

/* External procedures */
char *fread_action(FILE * fl, int nr);
char *fread_string(FILE * fl, char *error);
void make_titan_corpse(struct char_data *ch);
void stop_follower(struct char_data * ch);
void stop_assisting(struct char_data *ch);
void stop_assisters(struct char_data *ch);
void mount_violence(struct char_data *rider);
int mag_savingthrow(struct char_data * ch, int type);

SPECIAL(titan);
ACMD(do_flee);
void hit(struct char_data * ch, struct char_data * victim, int type);
void forget(struct char_data * ch, struct char_data * victim);
void remember(struct char_data * ch, struct char_data * victim);
int ok_damage_shopkeeper(struct char_data * ch, struct char_data * victim);

/* Weapon attack texts */
struct attack_hit_type attack_hit_text[] =
{
  {"hit", "hits"},		/* 0 */
  {"sting", "stings"},
  {"whip", "whips"},
  {"slash", "slashes"},
  {"bite", "bites"},
  {"bludgeon", "bludgeons"},	/* 5 */
  {"crush", "crushes"},
  {"pound", "pounds"},
  {"claw", "claws"},
  {"maul", "mauls"},
  {"thrash", "thrashes"},	/* 10 */
  {"pierce", "pierces"},
  {"blast", "blasts"},
  {"punch", "punches"},
  {"stab", "stabs"},
  {"shot", "shoots"}, 	/* 15 - DM: why is this here? */
  {"ERR", "ERRORS"},
  {"ERR", "ERRORS"},
  {"ERR", "ERRORS"},
  {"ERR", "ERRORS"},
  {"shot", "shoot"},	/* 20 - DM: gun attack types are from 20-30 */
  {"shot", "shoot"},
  {"shot", "shoot"},
  {"shot", "shoot"},
  {"shot", "shoot"},
  {"shot", "shoot"},	/* 25 */
  {"shot", "shoot"},
  {"shot", "shoot"},
  {"shot", "shoot"},
  {"shot", "shoot"},
  {"blast", "blast"}	/* 30 */
};

#define IS_WEAPON(type) (((type) >= TYPE_HIT) && ((type) < TYPE_SUFFERING))

/* The Fight related routines */

void appear(struct char_data * ch)
{
  act("$n slowly fades into existence.", FALSE, ch, 0, 0, TO_ROOM);

  if (affected_by_spell(ch, SPELL_INVISIBLE))
    affect_from_char(ch, SPELL_INVISIBLE);

  REMOVE_BIT(AFF_FLAGS(ch), AFF_INVISIBLE | AFF_HIDE);
}



void load_messages(void)
{
  FILE *fl;
  int i, type;
  struct message_type *messages;
  char chk[128];

  if (!(fl = fopen(MESS_FILE, "r"))) {
    sprintf(buf2, "Error reading combat message file %s", MESS_FILE);
    perror(buf2);
    exit(1);
  }
  for (i = 0; i < MAX_MESSAGES; i++) {
    fight_messages[i].a_type = 0;
    fight_messages[i].number_of_attacks = 0;
    fight_messages[i].msg = 0;
  }


  fgets(chk, 128, fl);
  while (!feof(fl) && (*chk == '\n' || *chk == '*'))
    fgets(chk, 128, fl);

  while (*chk == 'M') {
    fgets(chk, 128, fl);
    sscanf(chk, " %d\n", &type);
    for (i = 0; (i < MAX_MESSAGES) && (fight_messages[i].a_type != type) &&
	 (fight_messages[i].a_type); i++);
    if (i >= MAX_MESSAGES) {
      fprintf(stderr, "Too many combat messages.  Increase MAX_MESSAGES and recompile.");
      exit(1);
    }
    CREATE(messages, struct message_type, 1);
    fight_messages[i].number_of_attacks++;
    fight_messages[i].a_type = type;
    messages->next = fight_messages[i].msg;
    fight_messages[i].msg = messages;

    messages->die_msg.attacker_msg = fread_action(fl, i);
    messages->die_msg.victim_msg = fread_action(fl, i);
    messages->die_msg.room_msg = fread_action(fl, i);
    messages->miss_msg.attacker_msg = fread_action(fl, i);
    messages->miss_msg.victim_msg = fread_action(fl, i);
    messages->miss_msg.room_msg = fread_action(fl, i);
    messages->hit_msg.attacker_msg = fread_action(fl, i);
    messages->hit_msg.victim_msg = fread_action(fl, i);
    messages->hit_msg.room_msg = fread_action(fl, i);
    messages->god_msg.attacker_msg = fread_action(fl, i);
    messages->god_msg.victim_msg = fread_action(fl, i);
    messages->god_msg.room_msg = fread_action(fl, i);
    fgets(chk, 128, fl);
    while (!feof(fl) && (*chk == '\n' || *chk == '*'))
      fgets(chk, 128, fl);
  }

  fclose(fl);
}


void update_pos(struct char_data * victim)
{

  if ((GET_HIT(victim) > 0) && (GET_POS(victim) > POS_STUNNED))
    return;
  else if (GET_HIT(victim) > 0)
    GET_POS(victim) = POS_STANDING;
  else if (GET_HIT(victim) <= -11)
    GET_POS(victim) = POS_DEAD;
  else if (GET_HIT(victim) <= -6)
    GET_POS(victim) = POS_MORTALLYW;
  else if (GET_HIT(victim) <= -3)
    GET_POS(victim) = POS_INCAP;
  else
    GET_POS(victim) = POS_STUNNED;
}


void check_killer(struct char_data * ch, struct char_data * vict)
{
  if (!PLR_FLAGGED(vict, PLR_KILLER) && !PLR_FLAGGED(vict, PLR_THIEF)
      && !PLR_FLAGGED(ch, PLR_KILLER) && !IS_NPC(ch) && !IS_NPC(vict) &&
      (ch != vict) && GET_LEVEL(ch)<=LVL_IS_GOD) {
    char buf[256];

    SET_BIT(PLR_FLAGS(ch), PLR_KILLER);
    sprintf(buf, "PC Killer bit set on %s for initiating attack on %s at %s.",
	    GET_NAME(ch), GET_NAME(vict), world[vict->in_room].name);
    mudlog(buf, BRF, LVL_ETRNL1, TRUE);
    send_to_char("If you want to be a PLAYER KILLER, so be it...\r\n", ch);
  }
}


/* start one char fighting another (yes, it is horrible, I know... )  */
void set_fighting(struct char_data * ch, struct char_data * vict)
{
  if (ch == vict)
    return;


  assert(!FIGHTING(ch));

  ch->next_fighting = combat_list;
  combat_list = ch;

  if (IS_AFFECTED(ch, AFF_SLEEP))
    affect_from_char(ch, SPELL_SLEEP);

  FIGHTING(ch) = vict;
  GET_POS(ch) = POS_FIGHTING;

  if (!IS_SET(zone_table[world[ch->in_room].zone].zflag , ZN_PK_ALLOWED))
    check_killer(ch, vict);

}



/* remove a char from the list of fighting chars */
void stop_fighting(struct char_data * ch)
{
  struct char_data *temp;

  if (ch == next_combat_list)
    next_combat_list = ch->next_fighting;

  REMOVE_FROM_LIST(ch, combat_list, next_fighting);
  ch->next_fighting = NULL;
  FIGHTING(ch) = NULL;
  GET_POS(ch) = POS_STANDING;
  update_pos(ch);
}



struct obj_data *make_corpse(struct char_data * ch)
{
  struct obj_data *corpse, *o;
  struct obj_data *money;
  int i;
  extern int max_npc_corpse_time, max_pc_corpse_time;

  struct obj_data *create_money(int amount);

  corpse = create_obj();

  corpse->item_number = NOTHING;
  corpse->in_room = NOWHERE;
  // Artus> Player Corpses can be 'pcorpse' too.
  if (IS_NPC(ch))
    corpse->name = str_dup("corpse");
  else
    corpse->name = str_dup("corpse pcorpse");

  sprintf(buf2, "The corpse of %s is lying here.", GET_NAME(ch));
  corpse->description = str_dup(buf2);

  sprintf(buf2, "the corpse of %s", GET_NAME(ch));
  corpse->short_description = str_dup(buf2);

  GET_OBJ_TYPE(corpse) = ITEM_CONTAINER;
  GET_OBJ_WEAR(corpse) = ITEM_WEAR_TAKE;
  GET_OBJ_EXTRA(corpse) = ITEM_NODONATE;
  GET_OBJ_VAL(corpse, 0) = 0;	/* You can't store stuff in a corpse */
  GET_OBJ_VAL(corpse, 3) = 1;	/* corpse identifier */
  GET_OBJ_WEIGHT(corpse) = GET_WEIGHT(ch) + IS_CARRYING_W(ch);
  GET_OBJ_COST(corpse) = GET_LEVEL(ch);
  corpse->obj_flags.cost_per_day= 100000;
  if (IS_NPC(ch))
    GET_OBJ_TIMER(corpse) = max_npc_corpse_time;
  else
    GET_OBJ_TIMER(corpse) = max_pc_corpse_time;

  /* transfer character's inventory to the corpse */
  corpse->contains = ch->carrying;
  for (o = corpse->contains; o != NULL; o = o->next_content)
    o->in_obj = corpse;
  object_list_new_owner(corpse, NULL);

  /* transfer character's equipment to the corpse */
  for (i = 0; i < NUM_WEARS; i++)
    if (ch->equipment[i])
      obj_to_obj(unequip_char(ch, i), corpse);

  /* transfer gold */
  if (GET_GOLD(ch) > 0) {
    /* following 'if' clause added to fix gold duplication loophole */
    if (IS_NPC(ch) || (!IS_NPC(ch) && ch->desc)) {
      money = create_money(GET_GOLD(ch));
      obj_to_obj(money, corpse);
    }
    GET_GOLD(ch) = 0;
  }
  ch->carrying = NULL;
  IS_CARRYING_N(ch) = 0;
  IS_CARRYING_W(ch) = 0;

  obj_to_room(corpse, ch->in_room);

  return corpse;
}


/* When ch kills victim */
void change_alignment(struct char_data * ch, struct char_data * victim)
{
  struct affected_type *af, *next;

  /*
   * new alignment change algorithm: if you kill a monster with alignment A,
   * you move 1/16th of the way to having alignment -A.  Simple and fast.
   */
  GET_ALIGNMENT(ch) += (-GET_ALIGNMENT(victim) - GET_ALIGNMENT(ch)) >> 4;

  /* Check alignment restrictions - DM */
  if (affected_by_spell(ch,SPELL_PROT_FROM_GOOD) && !IS_EVIL(ch)) 
      for (af = ch->affected; af; af = next) {
        next = af->next;
        if ((af->type == SPELL_PROT_FROM_GOOD) && (af->duration > 0)) {
          send_to_char("You no longer feel the need for protection from good beings.\r\n", ch);
          affect_remove(ch,af);
        }
      }

  if (affected_by_spell(ch,SPELL_PROT_FROM_EVIL) && !IS_GOOD(ch)) 
      for (af = ch->affected; af; af = next) {
        next = af->next;
        if ((af->type == SPELL_PROT_FROM_EVIL) && (af->duration > 0)) {
          send_to_char("You no longer feel the need for protection from evil beings.\r\n", ch);
          affect_remove(ch,af);
        }
      }
}



void death_cry(struct char_data * ch)
{
  int door, was_in;

  act("Your blood freezes as you hear $n's death cry.", FALSE, ch, 0, 0, TO_ROOM);
  was_in = ch->in_room;

  for (door = 0; door < NUM_OF_DIRS; door++) {
    if (CAN_GO(ch, door)) {
      ch->in_room = world[was_in].dir_option[door]->to_room;
      act("Your blood freezes as you hear someone's death cry.", FALSE, ch, 0, 0, TO_ROOM);
      ch->in_room = was_in;
    }
  }
}

void raw_kill(struct char_data *ch, struct char_data *killer)
{
  struct obj_data *corpse, *obj, *next_obj;
  bool found=FALSE, arms_full=FALSE;
  int amount, num=1, share;
  struct char_data *k;
  struct follow_type *f; 

  extern struct index_data *mob_index;

  void perform_get_from_container(struct char_data * ch, struct obj_data * obj,
				       struct obj_data * cont, int mode);
  stop_fighting(ch);

  // Dismount
  if( MOUNTING(ch) ) 
	MOUNTING(MOUNTING(ch)) = NULL;
  if( MOUNTING_OBJ(ch) )
	OBJ_RIDDEN(MOUNTING_OBJ(ch)) = NULL;


  while (ch->affected)
    affect_remove(ch, ch->affected);

  if (PRF_FLAGGED(ch, PRF_MORTALK)){
	REMOVE_BIT(PRF_FLAGS(ch), PRF_MORTALK);
        call_magic(ch,ch,NULL,42,GET_LEVEL(ch),CAST_MAGIC_OBJ);
        GET_HIT(ch)= GET_MAX_HIT(ch);
        GET_MANA(ch) = 100;
        GET_POS(ch) = POS_STUNNED;
  } else {

  /* remove wolf/vamp infections so ya hafta be reinfected after death */
    REMOVE_BIT(PRF_FLAGS(ch),PRF_WOLF | PRF_VAMPIRE);

    if(GET_HIT(ch) > 0)
      GET_HIT(ch) = 0;

    death_cry(ch);

    if (GET_MOB_SPEC(ch) == titan)
    {
      corpse = NULL; // Artus -- Naughty Talisman! -- Titan crash.
      make_titan_corpse(ch);
    }
    else
      corpse = make_corpse(ch);

    /* DM - check to see if someone killed ch, or if we dont have a corpse */
    if (!killer || !corpse) {
      extract_char(ch);
      return;
    }

  /* DM - autoloot/autogold - automatically loot if ch is NPC 
	and they are in the same room */

    if ( (!IS_NPC(killer) && IS_NPC(ch)) &&  
	(killer->in_room == ch->in_room) &&
	(corpse) && (corpse->contains)) /*artus - titan like crashes*/ { 

    /* Auto Loot */
      if (EXT_FLAGGED(killer, EXT_AUTOLOOT)) {
        for (obj = corpse->contains; obj && !arms_full; obj = next_obj) {  
          if (IS_CARRYING_N(killer) >= CAN_CARRY_N(killer)) {
    	    send_to_char("Your arms are already full!\r\n", killer);
            arms_full = TRUE; 
            continue;
          }
	  next_obj = obj->next_content;
          if (CAN_SEE_OBJ(killer, obj) && (GET_OBJ_TYPE(obj) != ITEM_MONEY)) {
            perform_get_from_container(killer, obj, corpse, 0); 
            found = TRUE;
          }
	} 
        if (!found)
          act("$p seems to be empty.", FALSE, killer, corpse, 0, TO_CHAR);
      }

    /* Auto Gold */
      if (EXT_FLAGGED(killer, EXT_AUTOGOLD)) {
        for (obj = corpse->contains; obj; obj = next_obj) {
          next_obj = obj->next_content;
          if ((GET_OBJ_TYPE(obj) == ITEM_MONEY) && (GET_OBJ_VAL(obj, 0) > 0)) { 

	    /* Auto Split */
            if (EXT_FLAGGED(killer, EXT_AUTOSPLIT) && IS_AFFECTED(killer, AFF_GROUP)) {

	      amount = GET_OBJ_VAL(obj, 0);

    	      k = (killer->master ? killer->master : killer);


	/* DM - fix in here when a follower makes kill not in room as others */

              for (f = k->followers; f; f = f->next)
                if (IS_AFFECTED(f->follower, AFF_GROUP) && (!IS_NPC(f->follower)) &&
                      /* (f->follower != killer) && */
                      (f->follower->in_room == killer->in_room))
                  num++;      


              if (num == 1) 
                perform_get_from_container(killer, obj, corpse, 0);  
              else {
                obj_from_obj(obj);
                share = amount / num;

                GET_GOLD(killer) += share;
                sprintf(buf, "You split %s%d%s coins among %d members -- %s%d%s coins each.\r\n",
        		CCGOLD(killer,C_NRM),amount,CCNRM(killer,C_NRM), num,
        		CCGOLD(killer,C_NRM),share,CCNRM(killer,C_NRM));
    			send_to_char(buf, killer);

                if (IS_AFFECTED(k, AFF_GROUP) && (k->in_room == killer->in_room)
                    && !(IS_NPC(k)) && k != killer) {
                  GET_GOLD(k) += share;
                  sprintf(buf, "%s splits %s%d%s coins; you receive %s%d%s.\r\n", GET_NAME(killer),
                           CCGOLD(k,C_NRM),amount,CCNRM(k,C_NRM),CCGOLD(k,C_NRM),
                           share,CCNRM(k,C_NRM));
                  send_to_char(buf, k); 
                }

	        for (f=k->followers; f;f=f->next) {
                  if (IS_AFFECTED(f->follower, AFF_GROUP) &&
                      (!IS_NPC(f->follower)) &&
                      (f->follower->in_room == killer->in_room) &&
                      f->follower != killer) {
                    GET_GOLD(f->follower) += share;
                    sprintf(buf, "%s splits %s%d%s coins; you receive %s%d%s.\r\n",
                    GET_NAME(killer),CCGOLD(f->follower,C_NRM),amount,CCNRM(f->follower,C_NRM),
                    CCGOLD(f->follower,C_NRM), share,CCNRM(f->follower,C_NRM));
                    send_to_char(buf, f->follower); 
                  } 
                } 
              } 
	  /* Not grouped or autosplitting */
	    } else  
              perform_get_from_container(killer, obj, corpse, 0);  
          }  
        }
      } /* End of Autogold */
    } /* End of Auto loot/gold/split able */
    extract_char(ch);
  }
}

void die(struct char_data *ch, struct char_data *killer)
{
  extern int level_exp[LVL_CHAMP+1];

  /* bm changed exp lost to  level minimum and 10% of carried gold*/
  if (PRF_FLAGGED(ch,PRF_MORTALK))
	REMOVE_BIT(PRF_FLAGS(ch), PLR_KILLER | PLR_THIEF);
  else{
  /* DM_exp : If exp is not negative (which it shouldn't be ie level min = 0)
		use (exp this level)/2, otherwise take 10% of level */
  	if (GET_EXP(ch) > 0)
		gain_exp(ch, -(GET_EXP(ch)/2));	
  	else
		gain_exp(ch,-(level_exp[(int)GET_LEVEL(ch)]*0.1));	
  	GET_GOLD(ch)*=0.9;
  	if (!IS_NPC(ch))
  	  REMOVE_BIT(PLR_FLAGS(ch), PLR_KILLER | PLR_THIEF);
      }
	  raw_kill(ch,killer);
}
void perform_group_gain(struct char_data * ch, int base,
			     struct char_data * victim)
{
  int share;

  share = MIN(max_exp_gain, MAX(1, base));
  if (PRF_FLAGGED(ch, PRF_MORTALK))
	return;
  if (PLR_FLAGGED(victim, PLR_KILLER))
	return;
  if (share > 1) {
    sprintf(buf2, "You receive your share of experience -- %s%d%s points.\r\n",
	CCEXP(ch,C_NRM),share,CCNRM(ch,C_NRM));
    send_to_char(buf2, ch);
  } else
    send_to_char("You receive your share of experience -- one measly little point!\r\n", ch);

  gain_exp(ch, share);
  change_alignment(ch, victim);
}


void group_gain(struct char_data * ch, struct char_data * victim)
{
  int tot_members, base;
  struct char_data *k;
  struct follow_type *f;
  int min_level = LVL_IMPL;
  int max_level = 0;  
  long exp;
  int group_level;
  int av_group_level;
  int percent;
  extern struct index_data *mob_index;

  if (!(k = ch->master))
    k = ch;
  if (PRF_FLAGGED(ch, PRF_MORTALK)){
     stop_fighting(ch);
     stop_fighting(victim);
     send_to_char("You are the SUPREME warrior of the MOrtal Kombat arena!\r\n", ch);
     return;
  }
  if (PLR_FLAGGED(victim, PLR_KILLER)){
           send_to_char("You recieve No EXP for killing players with the KILLER flag\r\n", ch);
   	   return;
   }	 

  group_level = GET_LEVEL(k); 
  max_level = min_level = MAX(max_level, GET_LEVEL(k)); 

  if (IS_AFFECTED(k, AFF_GROUP) && (k->in_room == ch->in_room))
    tot_members = 1;
  else
    tot_members = 0;

  for (f = k->followers; f; f = f->next)
    if (IS_AFFECTED(f->follower, AFF_GROUP) && f->follower->in_room == ch->in_room)
  
    /* DM - Dont include CLONES if they are grouped */ 
    if (!IS_CLONE(f->follower))
    {
      tot_members++;
      group_level += GET_LEVEL(f->follower);
      min_level = MIN(min_level, GET_LEVEL(f->follower));
      max_level = MAX(max_level, GET_LEVEL(f->follower));
    }

 av_group_level = group_level / tot_members;

/* cap it to LVL_IMPL */
 group_level = MIN(LVL_IMPL, group_level - tot_members);

  base = (GET_EXP(victim) / 3) + tot_members - 1;

/* DM group gain percentage */
  if (tot_members >= 1) {
    /* base = MAX(1, GET_EXP(victim) / 3) / tot_members; */
    if (tot_members == 2)
        base=(GET_EXP(victim)*80)/100;
    else if (tot_members == 3)
        base=(GET_EXP(victim)*85)/100;
    else if (tot_members == 4)
        base=(GET_EXP(victim)*90)/100;
    else if (tot_members == 5)
        base=(GET_EXP(victim)*70)/100;
    else if (tot_members == 6)
        base=(GET_EXP(victim)*65)/100;
    else if (tot_members == 7)
        base=(GET_EXP(victim)*55)/100;
    else if (tot_members == 8)
        base=(GET_EXP(victim)*20)/100;
    else 
      base=0;
    
  } else
    base = 0;


  if (IS_AFFECTED(k, AFF_GROUP) && k->in_room == ch->in_room)
  {
      if (GET_LEVEL(victim) < av_group_level)
      {
        base = ((GET_EXP(victim) / 3) + tot_members - 1)/tot_members;
        percent = MAX(10, 100 - ((GET_LEVEL(ch) - GET_LEVEL(victim)) * 10));
        exp = (base * percent) / 100;
        sprintf(buf2, "Your opponent was out of your group's league! You don't learn as much.\n\r");
        send_to_char(buf2, k); 
      }
      else
        exp = base;

      if (((max_level - min_level) > 10) && (tot_members > 1))
      {
        if (exp > (((GET_EXP(victim) / 3) + tot_members - 1)/tot_members))
	  exp = base = ((GET_EXP(victim) / 3) + tot_members - 1)/tot_members;
        sprintf(buf2, "The group is unbalanced! You learn much less.\n\r");
        send_to_char(buf2, k);  
        exp = (exp * GET_LEVEL(k)) / group_level;
      } 
      perform_group_gain(k, exp, victim);
   }

  for (f = k->followers; f; f = f->next)
    if (IS_AFFECTED(f->follower, AFF_GROUP) && f->follower->in_room == ch->in_room)
    {
    /* DM - Dont include CLONES if they are grouped * / 
    // if (!IS_CLONE(f->follower)) {
    // Artus - We'll stop NPC's gaining all together. */
    if (!IS_NPC(f->follower)) {

      if (GET_LEVEL(victim) < av_group_level)
      {
        percent = MAX(10, 100 - ((GET_LEVEL(ch) - GET_LEVEL(victim)) * 10));
        exp = (base * percent) / 100;
        sprintf(buf2, "Your opponent was out of your group's league! You don't learn as much.\n\r");
        send_to_char(buf2, f->follower);
      }
      else
        exp = base;
      if ((max_level - min_level) > 10)
      {
        sprintf(buf2, "The group is unbalanced! You learn much less.\n\r");
        send_to_char(buf2, f->follower);  
        exp = (exp * GET_LEVEL(f->follower)) / (group_level*2);
      }
      perform_group_gain(f->follower, exp, victim);
    }
  }
}

char *replace_string(char *str, char *weapon_singular, char *weapon_plural)
{
  static char buf[256];
  char *cp;

  cp = buf;

  for (; *str; str++) {
    if (*str == '#') {
      switch (*(++str)) {
      case 'W':
	for (; *weapon_plural; *(cp++) = *(weapon_plural++));
	break;
      case 'w':
	for (; *weapon_singular; *(cp++) = *(weapon_singular++));
	break;
      default:
	*(cp++) = '#';
	break;
      }
    } else
      *(cp++) = *str;

    *cp = 0;
  }				/* For */

  return (buf);
}


/* message for doing damage with a weapon */
void dam_message(int dam, struct char_data * ch, struct char_data * victim,
		      int w_type)
{
  char *buf;
  int msgnum;
  /* int percent; */     /* JA not needed since vader disabled messages */
  
  static struct dam_weapon_type {
    char *to_room;
    char *to_char;
    char *to_victim;
  } dam_weapons[] = {

    /* use #w for singular (i.e. "slash") and #W for plural (i.e. "slashes") */

    {
      "$n tries to #w $N, but misses.",	/* 0: 0     */
      "You try to #w $N, but miss.",
      "$n tries to #w you, but misses."
    },

    {
      "$n tickles $N as $e #W $M.",	/* 1: 1..2  */
      "You tickle $N as you #w $M.",
      "$n tickles you as $e #W you."
    },

    {
      "$n barely #W $N.",		/* 2: 3..4  */
      "You barely #w $N.",
      "$n barely #W you."
    },

    {
      "$n #W $N.",			/* 3: 5..6  */
      "You #w $N.",
      "$n #W you."
    },

    {
      "$n #W $N hard.",			/* 4: 7..10  */
      "You #w $N hard.",
      "$n #W you hard."
    },

    {
      "$n #W $N very hard.",		/* 5: 11..14  */
      "You #w $N very hard.",
      "$n #W you very hard."
    },

    {
      "$n #W $N extremely hard.",	/* 6: 15..19  */
      "You #w $N extremely hard.",
      "$n #W you extremely hard."
    },

    {
      "$n causes internal BLEEDING on $N with $s #w!!",	/* 8: 23..30   */
      "You cause internal BLEEDING on $N  with your #w!!",
      "$n causes internal BLEEDING on you with $s #w!!"
    },

    {
      "$n severely injures $N with $s #w!!",	/* 8: 23..30   */
      "You severely injure $N with your #w!!",
      "$n severely injures you with $s #w!!"
    },

    {
      "$n wipes $N out with $s deadly #w!!",	/* 8: 23..30   */
      "You wipe out $N with your deadly #w!!",
      "$n wipes you with $s #w!!"
    },
  
    {
      "$n massacres $N to small fragments with $s #w.",	/* 7: 19..23 */
      "You massacre $N to small fragments with your #w.",
      "$n massacres you to small fragments with $s #w."
    },


    {
      "$n SHATTERS $N with $s deadly #w!!",	/* 8: 23..30   */
      "You SHATTER $N with your deadly #w!!",
      "$n SHATTERS you with $s deadly #w!!"
    },

    {
      "$n OBLITERATES $N with $s deadly #w!!",	/* 8: 23..30   */
      "You OBLITERATE $N with your deadly #w!!",
      "$n OBLITERATES you with $s deadly #w!!"
    },

    {
      "$n DESTROYS $N with $s deadly #w!!",	/* 9: > 30   */
      "You DESTROY $N with your deadly #w!!",
      "$n DESTROYS you with $s deadly #w!!"
    },
    {
      "$n FUBARS $N with $s deadly #w!!",
      "You FUBAR $N with your deadly #w!!",
      "$n FUBARS you with $s deadly #w!!"
    },
    {
      "$n NUKES $N with $s deadly #w!!",
      "You NUKE $N with your deadly #w!!",
      "$n NUKES you with $s deadly #w!!"
    },
    {
      "$n does UNSPEAKABLE things to $N with $s deadly #w!!",
      "You do UNSPEAKABLE things to $N with your deadly #w!!",
      "$n does UNSPEAKABLE things to you with $s deadly #w!!"
    }
  };

  w_type -= TYPE_HIT;		/* Change to base of table with text */

  if (dam == 0)		msgnum = 0;
  else if (dam <= 2)    msgnum = 1;
  else if (dam <= 8)    msgnum = 2;
  else if (dam <= 14)   msgnum = 3;
  else if (dam <= 20)   msgnum = 4;
  else if (dam <= 25)   msgnum = 5;
  else if (dam <= 30)   msgnum = 6;
  else if (dam <= 35)   msgnum = 7;
  else if (dam <= 40)   msgnum = 8;
  else if (dam <= 45)   msgnum = 9;
  else if (dam <= 50)   msgnum = 10;
  else if (dam <= 60)   msgnum = 11; 
  else if (dam <= 70)   msgnum = 12; 
  else if (dam <= 80)   msgnum = 13; 
  else if (dam <= 90)   msgnum = 14;
  else if (dam <=100)   msgnum = 15;
  else			msgnum = 16; 

  /* damage message to onlookers */
  buf = replace_string(dam_weapons[msgnum].to_room,
	  attack_hit_text[w_type].singular, attack_hit_text[w_type].plural);
  act(buf, FALSE, ch, NULL, victim, TO_NOTVICT);


  /* damage message to damagee */
	if (!PRF_FLAGGED(victim, PRF_BRIEF))
	{
  	send_to_char(CCRED(victim, C_CMP), victim);
  	buf = replace_string(dam_weapons[msgnum].to_victim,
		  attack_hit_text[w_type].singular, attack_hit_text[w_type].plural);
  	act(buf, FALSE, ch, NULL, victim, TO_VICT | TO_SLEEP);
  	send_to_char(CCNRM(victim, C_CMP), victim);
	}

	
  /* damage message to damager */
	if (!PRF_FLAGGED(ch, PRF_BRIEF))
	{
  	send_to_char(CCYEL(ch, C_CMP), ch);
  	buf = replace_string(dam_weapons[msgnum].to_char,
		  attack_hit_text[w_type].singular, attack_hit_text[w_type].plural);
  	act(buf, FALSE, ch, NULL, victim, TO_CHAR);
  	send_to_char(CCNRM(ch, C_CMP), ch);
 	}
	else
		send_to_char("You are fighting\r\n", ch);

/* section below commented out to remove damage messages ??/01/96 - Vader */
/*
      if (GET_MAX_HIT(victim) > 0)
        percent = (100 * GET_HIT(victim)) / GET_MAX_HIT(victim);
      else
        percent = -1;		
      strcpy(buf, PERS(victim, ch));
      CAP(buf);
      if (percent >= 100)
        strcat(buf, " is in excellent condition.");
      else if (percent >= 90)
        strcat(buf, " has a few scratches.");
      else if (percent >= 75)
        strcat(buf, " has some small wounds and bruises.");
      else if (percent >= 50)
        strcat(buf, " has quite a few wounds.");
      else if (percent >= 30)
        strcat(buf, " has some big nasty wounds and scratches.");
      else if (percent >= 15)
        strcat(buf, " looks pretty hurt.");
      else if (percent >= 0)
       strcat(buf, " is in awful condition.");
      else
        strcat(buf, " is bleeding awfully from big wounds.");
      strcat(buf,"\r\n");

  send_to_char(buf,ch);  
*/
}


/*
 * message for doing damage with a spell or skill
 *  C3.0: Also used for weapon damage on miss and death blows
 */
int skill_message(int dam, struct char_data * ch, struct char_data * vict,
		      int attacktype)
{
  int i, j, nr;
  struct message_type *msg;

  struct obj_data *weap = ch->equipment[WEAR_WIELD];

  for (i = 0; i < MAX_MESSAGES; i++) {
    if (fight_messages[i].a_type == attacktype) {
      nr = dice(1, fight_messages[i].number_of_attacks);
      for (j = 1, msg = fight_messages[i].msg; (j < nr) && msg; j++)
	msg = msg->next;

      if (!IS_NPC(vict) && (GET_LEVEL(vict) >= LVL_IS_GOD)) {
	act(msg->god_msg.attacker_msg, FALSE, ch, weap, vict, TO_CHAR);
	act(msg->god_msg.victim_msg, FALSE, ch, weap, vict, TO_VICT);
	act(msg->god_msg.room_msg, FALSE, ch, weap, vict, TO_NOTVICT);
      } else if (dam != 0) {
	if (GET_POS(vict) == POS_DEAD) {
	  send_to_char(CCYEL(ch, C_CMP), ch);
	  act(msg->die_msg.attacker_msg, FALSE, ch, weap, vict, TO_CHAR);
	  send_to_char(CCNRM(ch, C_CMP), ch);

	  send_to_char(CCRED(vict, C_CMP), vict);
	  act(msg->die_msg.victim_msg, FALSE, ch, weap, vict, TO_VICT | TO_SLEEP);
	  send_to_char(CCNRM(vict, C_CMP), vict);

	  act(msg->die_msg.room_msg, FALSE, ch, weap, vict, TO_NOTVICT);
	} else {
	  send_to_char(CCYEL(ch, C_CMP), ch);
	  act(msg->hit_msg.attacker_msg, FALSE, ch, weap, vict, TO_CHAR);
	  send_to_char(CCNRM(ch, C_CMP), ch);

	  send_to_char(CCRED(vict, C_CMP), vict);
	  act(msg->hit_msg.victim_msg, FALSE, ch, weap, vict, TO_VICT | TO_SLEEP);
	  send_to_char(CCNRM(vict, C_CMP), vict);

	  act(msg->hit_msg.room_msg, FALSE, ch, weap, vict, TO_NOTVICT);
	}
      } else if (ch != vict) {	/* Dam == 0 */
	send_to_char(CCYEL(ch, C_CMP), ch);
	act(msg->miss_msg.attacker_msg, FALSE, ch, weap, vict, TO_CHAR);
	send_to_char(CCNRM(ch, C_CMP), ch);

	send_to_char(CCRED(vict, C_CMP), vict);
	act(msg->miss_msg.victim_msg, FALSE, ch, weap, vict, TO_VICT | TO_SLEEP);
	send_to_char(CCNRM(vict, C_CMP), vict);

	act(msg->miss_msg.room_msg, FALSE, ch, weap, vict, TO_NOTVICT);
      }
      return 1;
    }
  }
  return 0;
}


void damage(struct char_data * ch, struct char_data * victim, int dam,int attacktype)
{
  int exp;
  int percent;
  extern struct index_data *mob_index;

  if (IS_NPC(victim))
    if(MOB_FLAGGED(victim,MOB_QUEST))  
    {
      send_to_char("Sorry, they are part of a quest.\r\n",ch);
      return;
    }
 
  if (GET_POS(victim) <= POS_DEAD) {
    log("SYSERR: Attempt to damage a corpse.");
    return;			/* -je, 7/7/92 */
  }
  /* You can't damage an immortal! */
  if (!IS_NPC(victim) && (GET_LEVEL(victim) >= LVL_ANGEL))
    dam = 0;

	/* shopkeeper protection */
	if (!ok_damage_shopkeeper(ch, victim) && ch != NULL)
	{
		if(!IS_NPC(ch)){
                   send_to_char("Kill a shopkeeper.  I THINK NOT!!\r\n", ch);
                   GET_HIT(ch)=0;
		   GET_POS(ch)=POS_STUNNED;
	        }
		return;
	}

  /* Mounts can't attack riders (Might modify for a chance to throw them off) */
  if( victim == MOUNTING(ch) && IS_NPC(ch) ) {
	send_to_char("You decide against attacking your rider.\r\n", ch);
	return;
  }	
  /* Can't attack your own mount */
  if( victim == MOUNTING(ch) ) {
	send_to_char("Better dismount first.\r\n", ch);	
	return;
  }

  if (IS_CLONE(victim) && !IS_NPC(ch)) {
    act("The clone of $N knocks $n over.", TRUE, ch, 0, victim, TO_ROOM);
    sprintf(buf, "The clone of %s knocks you over. OUCH!!\r\n", GET_NAME(victim));
    send_to_char(buf, ch);
    GET_HIT(ch) = MAX(1, GET_HIT(ch)/2);
    stop_fighting(ch);
    stop_fighting(victim);
    GET_POS(ch) = POS_STUNNED;
    return;
  }

  if (victim != ch) {
    if (GET_POS(ch) > POS_STUNNED) {
      if (!(FIGHTING(ch)))
	set_fighting(ch, victim);

      if (IS_NPC(ch) && IS_NPC(victim) && victim->master &&
	  !number(0, 10) && IS_AFFECTED(victim, AFF_CHARM) &&
	  (victim->master->in_room == ch->in_room)) {
	if (FIGHTING(ch))
	  stop_fighting(ch);
	hit(ch, victim->master, TYPE_UNDEFINED);
	return;
      }
    }
    if (GET_POS(victim) > POS_STUNNED && !FIGHTING(victim)) {
      set_fighting(victim, ch);
      if (MOB_FLAGGED(victim, MOB_MEMORY) && !IS_NPC(ch) &&
	  (GET_LEVEL(ch) < LVL_IS_GOD))
	remember(victim, ch);
    }
  }
  if (victim->master == ch)
    stop_follower(victim);

  if (AUTOASSIST(victim) == ch)
    stop_assisting(victim);

  if (IS_AFFECTED(ch, AFF_INVISIBLE | AFF_HIDE))
    appear(ch);

  if (IS_AFFECTED(victim, AFF_SANCTUARY))
    dam >>= 1;		/* 1/2 damage when sanctuary */

  if (!IS_SET(zone_table[world[ch->in_room].zone].zflag , ZN_PK_ALLOWED)) {
    check_killer(ch, victim);

    if (PLR_FLAGGED(ch, PLR_KILLER))
      dam = 0;
  }

  dam = MAX(MIN(dam, 1000), 0);
  GET_HIT(victim) -= dam;

  // Gain exp for hit... Unless npc vs npc (Art)...
  if ((ch != victim) && (!IS_NPC(ch) || !IS_NPC(victim)))
    if (!PRF_FLAGGED(ch, PRF_MORTALK))
      gain_exp(ch, GET_LEVEL(victim) * dam);

  update_pos(victim);

  /*
   * skill_message sends a message from the messages file in lib/misc.
   * dam_message just sends a generic "You hit $n extremely hard.".
   * skill_message is preferable to dam_message because it is more
   * descriptive.
   * 
   * If we are _not_ attacking with a weapon (i.e. a spell), always use
   * skill_message. If we are attacking with a weapon: If this is a miss or a
   * death blow, send a skill_message if one exists; if not, default to a
   * dam_message. Otherwise, always send a dam_message.
   */
  if (!IS_WEAPON(attacktype))
    skill_message(dam, ch, victim, attacktype);
  else {
    if (GET_POS(victim) == POS_DEAD || dam == 0) {
      if (!skill_message(dam, ch, victim, attacktype))
	dam_message(dam, ch, victim, attacktype);
    } else
      dam_message(dam, ch, victim, attacktype);
  }

  /* Use send_to_char -- act() doesn't send message if you are DEAD. */
  switch (GET_POS(victim)) {
  case POS_MORTALLYW:
    act("$n is mortally wounded, and will die soon, if not aided.", TRUE, victim, 0, 0, TO_ROOM);
    send_to_char("You are mortally wounded, and will die soon, if not aided.\r\n", victim);
    break;
  case POS_INCAP:
    act("$n is incapacitated and will slowly die, if not aided.", TRUE, victim, 0, 0, TO_ROOM);
    send_to_char("You are incapacitated an will slowly die, if not aided.\r\n", victim);
    break;
  case POS_STUNNED:
    act("$n is stunned, but will probably regain consciousness again.", TRUE, victim, 0, 0, TO_ROOM);
    send_to_char("You're stunned, but will probably regain consciousness again.\r\n", victim);
    break;
  case POS_DEAD:
    act("$n is dead!  R.I.P.", FALSE, victim, 0, 0, TO_ROOM);
    send_to_char("You are dead!  Sorry...\r\n", victim);
    break;

  default:			/* >= POSITION SLEEPING */
    if (dam > (GET_MAX_HIT(victim) >> 2))
      act("That really did HURT!", FALSE, victim, 0, 0, TO_CHAR);

    if (GET_HIT(victim) < (GET_MAX_HIT(victim) >> 2)) {
      sprintf(buf2, "%sYou wish that your wounds would stop BLEEDING so much!%s\r\n",
	      CCRED(victim, C_SPR), CCNRM(victim, C_SPR));
      send_to_char(buf2, victim);
      if (MOB_FLAGGED(victim, MOB_WIMPY))
	do_flee(victim, "", 0, 0);
    }
    if (!IS_NPC(victim) && GET_WIMP_LEV(victim) && victim != ch &&
	GET_HIT(victim) < GET_WIMP_LEV(victim)) {
      send_to_char("You wimp out, and attempt to flee!\r\n", victim);
      do_flee(victim, "", 0, 0);
    }
    break;
  }

  if (!IS_NPC(victim) && !(victim->desc)) {
    do_flee(victim, "", 0, 0);
    if (!FIGHTING(victim)) {
      act("$n is rescued by divine forces.", FALSE, victim, 0, 0, TO_ROOM);
      GET_WAS_IN(victim) = victim->in_room;
      char_from_room(victim);
      char_to_room(victim, 0);
    }
  }

  if (!AWAKE(victim))
    if (FIGHTING(victim))
      stop_fighting(victim);

  if (GET_POS(victim) == POS_DEAD) {
  
    /* fix for poisoned players getting exp, and their group! bm */
    if (!IS_NPC(victim) && ch==victim)
      if (attacktype==SPELL_POISON || attacktype==TYPE_SUFFERING)
      {
        sprintf(buf2, "%s died at %s (lost %d exp)", GET_NAME(ch),world[victim->in_room].name,  (GET_EXP(victim)/2));
        mudlog(buf2, BRF, LVL_ETRNL1, TRUE);
        if (MOB_FLAGGED(ch, MOB_MEMORY))
          forget(ch, victim);
        die(victim,ch);
        return;
      } 

    if (IS_NPC(victim) || victim->desc)

      /* DM - if CLONE makes kill in group - only give exp to CLONES master */
      if (IS_AFFECTED(ch, AFF_GROUP) && !IS_CLONE(ch)) {
        group_gain(ch, victim);
      } else {
	exp = MIN(max_exp_gain, GET_EXP(victim) / 3);

	/* Calculate level-difference bonus */
	if (IS_NPC(ch) && !IS_CLONE(ch) && !MOUNTING(ch) ) // And its not mounted
	  exp += MAX(0, (exp * MIN(4, (GET_LEVEL(victim) - GET_LEVEL(ch)))) >> 3);
	else
	{
/*          exp += MAX(0, (exp * MIN(8, (GET_LEVEL(victim) - GET_LEVEL(ch)))) >> 3); */

	  // Give exp to rider
	  if( MOUNTING(ch) && IS_NPC(ch) && !IS_CLONE(ch)) 
		ch = MOUNTING(ch);
     
/* JA new level difference code */
          if (GET_LEVEL(victim) < GET_LEVEL(ch))
          {
            percent = MAX(10, 100 - ((GET_LEVEL(ch) - GET_LEVEL(victim)) * 10));
            percent = MIN(percent, 100);
            exp = (exp * percent) / 50;
            if (percent <= 60) 
            {
              sprintf(buf2, "Your opponent was out of your league! You don't learn as much.\n\r");
              if (IS_CLONE(ch) && ch->master == NULL) {
                  die_clone(ch, NULL);
                  return;
              } 
              if (IS_CLONE(ch) && ch->master == NULL) {
                die_clone(ch, NULL);
                return;
              }
              if (IS_CLONE(ch) && IS_CLONE_ROOM(ch))
                send_to_char(buf2, ch->master);
              else
                send_to_char(buf2, ch);
            }
          }
       }

/* this is to stop people abusing the no exp loss below your level code
 * and it adds a punishment for killing in a non-pk zone - Vader
 */
#ifdef JOHN_DISABLE
        if(GET_EXP(victim) <= level_exp[(int)GET_LEVEL(victim)] + 10000 ||
           !IS_SET(zone_table[world[ch->in_room].zone].zflag,ZN_PK_ALLOWED))
          exp = -(GET_LEVEL(victim) * 10000);
        else
#endif
  	  exp = MAX(exp, 1);
	if (PLR_FLAGGED(victim, PLR_KILLER)){
           if (IS_CLONE(ch) && ch->master == NULL) {
             die_clone(ch, NULL);
             return;
           }
           if (IS_CLONE(ch) && IS_CLONE_ROOM(ch))
	     send_to_char("You recieve No EXP for killing players with the KILLER flag!.\r\n", ch->master);
           else
	     send_to_char("You recieve No EXP for killing players with the KILLER flag!.\r\n", ch);
	   exp = 0;
        } else if (PRF_FLAGGED(ch, PRF_MORTALK)) {
           if (IS_CLONE(ch) && ch->master == NULL) {
             die_clone(ch, NULL);
             return;
           }
           if (IS_CLONE(ch) && IS_CLONE_ROOM(ch))
             send_to_char("You are the SUPREME winner of Mortal Kombat!!\r\n", ch->master);
           else
             send_to_char("You are the SUPREME winner of Mortal Kombat!!\r\n", ch);
           exp = 0;
        } else if (exp > 1) {
          if (IS_CLONE(ch) && ch->master == NULL) {
             die_clone(ch, NULL);
             return;
          }
          if (IS_CLONE(ch) && IS_CLONE_ROOM(ch)) {
	    sprintf(buf2, "You receive %s%d%s experience points.\r\n", 
		CCEXP(ch->master,C_NRM),exp,CCNRM(ch->master,C_NRM));
	    send_to_char(buf2, ch->master);
          } else {
	    sprintf(buf2, "You receive %s%d%s experience points.\r\n", 
		CCEXP(ch,C_NRM),exp,CCNRM(ch,C_NRM));
	    send_to_char(buf2, ch);
          }
	} else {
          if (IS_CLONE(ch) && ch->master == NULL) {
             die_clone(ch, NULL);
             return;
          }
          if (IS_CLONE(ch) && IS_CLONE_ROOM(ch))
	    send_to_char("You receive one lousy experience point.\r\n", ch->master);
          else
	    send_to_char("You receive one lousy experience point.\r\n", ch);
	}

        if (IS_CLONE(ch) && ch->master == NULL) {
          die_clone(ch, NULL);
          return;
        }
        /* DM - dont let clones steal exp */
        if (IS_CLONE_ROOM(ch)) {
          gain_exp(ch->master, exp);
	  change_alignment(ch->master, victim);
        } else {
	  gain_exp(ch, exp);
          if (!IS_CLONE(ch)) 
	    change_alignment(ch, victim);
        }
      } 
    if (!IS_NPC(victim)) {
      sprintf(buf2, "%s killed by %s at %s (lost %d exp)", GET_NAME(victim), GET_NAME(ch),
	      world[victim->in_room].name, GET_EXP(ch)/2);
      mudlog(buf2, BRF, LVL_ETRNL1, TRUE);
      if (MOB_FLAGGED(ch, MOB_MEMORY))
	forget(ch, victim);
    }
    die(victim,ch);
  }
}


/* modified by Vader for multiattack.. */
void hit(struct char_data * ch, struct char_data * victim, int type)
{
  struct obj_data *wielded = ch->equipment[WEAR_WIELD];
  int w_type;
  int victim_ac, calc_thaco;
  int dam;
  byte diceroll;
  struct char_data *real_vict = victim;
  struct char_data *char_room;

  extern int thaco(struct char_data *);
  extern struct str_app_type str_app[];
  extern struct dex_app_type dex_app[];

  int attacktype;
  int reloadable; 
  
  /* char s[100]; */

/* JA code to randomly select a player to hit */
/* to stop the tank from getting hit all the time */

  /* pseudo-randomly choose someone in the room who is fighting me */
  if (IS_NPC(ch))
    for (real_vict = world[ch->in_room].people; real_vict; real_vict = real_vict->next_in_room)
      if (FIGHTING(real_vict) == ch && !number(0, 3))
        break;

  if (real_vict == NULL)
    real_vict = victim;
	assert(real_vict);

  if (ch->in_room != real_vict->in_room) 
  {
  /*  sprintf(s,"SYSERR: NOT SAME ROOM WHEN FIGHTING! (ch=%s, victim=%s)",
              ch->player.name,real_vict->player.name);
    log(s); */
    if (FIGHTING(ch) && FIGHTING(ch) == victim)
      stop_fighting(ch);
    return;
  }
  if (wielded && GET_OBJ_TYPE(wielded) == ITEM_WEAPON)
    attacktype = GET_OBJ_VAL(wielded, 3) + TYPE_HIT;
  else 
  {
    if (IS_NPC(ch) && (ch->mob_specials.attack_type != 0))
      attacktype = ch->mob_specials.attack_type + TYPE_HIT;
    else
      attacktype = TYPE_HIT;
  }



/* JA ------------------------- ammo code  */
  w_type = attacktype & 0x7fff;
  reloadable = attacktype & 0x8000;

/* changed so mobs dont use ammo - Vader */
  if (reloadable && !IS_NPC(ch))
  {
    if (GET_OBJ_VAL(wielded, 0) <= 0)
    {
      send_to_char("*CLICK*\n\r", ch);  
      return;
       /* out of ammo */
    }
    else
      MAX(0, --GET_OBJ_VAL(wielded, 0));
  }

  /* Calculate the raw armor including magic armor.  Lower AC is better. */

  calc_thaco = thaco(ch) - ch->points.hitroll;

  diceroll = number(1, 20);

  victim_ac = GET_AC(real_vict) / 20;

  if (AWAKE(real_vict))
    victim_ac += dex_app[GET_REAL_DEX(real_vict)].defensive;

  victim_ac = MAX(-10, victim_ac);	/* -10 is lowest */

  if ((((diceroll < 20) && AWAKE(real_vict)) &&
       ((diceroll == 1) || ((calc_thaco - diceroll) > victim_ac)))) {
    if (type == SKILL_BACKSTAB)
      damage(ch, real_vict, 0, SKILL_BACKSTAB);
    else if(type == SKILL_2ND_ATTACK || type == SKILL_3RD_ATTACK)
	   damage(ch, real_vict, 0, type);  /* thisll make it use the rite messages for missing */
	 else
	   damage(ch, real_vict, 0, w_type);
  } else {

    dam = str_app[STRENGTH_REAL_APPLY_INDEX(ch)].todam;
    dam += GET_DAMROLL(ch);

    if (wielded)
      dam += dice(GET_OBJ_VAL(wielded, 1), GET_OBJ_VAL(wielded, 2));
    else {
      if (IS_NPC(ch)) {
	dam += dice(ch->mob_specials.damnodice, ch->mob_specials.damsizedice);
      } else
	dam += number(0, 2);	/* Max. 2 dam with bare hands */
    }

    if (GET_POS(real_vict) < POS_FIGHTING)
      dam *= 1 + (POS_FIGHTING - GET_POS(real_vict)) / 3;
    /* Position  sitting  x 1.33 */
    /* Position  resting  x 1.66 */
    /* Position  sleeping x 2.00 */
    /* Position  stunned  x 2.33 */
    /* Position  incap    x 2.66 */
    /* Position  mortally x 3.00 */

    /* Check for protection from evil/good on victim - DM */
    if (((IS_GOOD(ch) && affected_by_spell(real_vict,SPELL_PROT_FROM_GOOD)) ||
        (IS_EVIL(ch) && affected_by_spell(real_vict,SPELL_PROT_FROM_EVIL))) &&
        (GET_LEVEL(ch) >= PROTECT_LEVEL) ) {

      dam -= ( GET_LEVEL(real_vict) / MAX(2,num_attacks(real_vict)) ); 
    }


    dam = MAX(1, dam);		/* Not less than 0 damage */

    if(type == SKILL_2ND_ATTACK && dam > 1)
      dam *= 0.666; /* 2 3rds damage on second attack */
    if(type == SKILL_3RD_ATTACK && dam > 1)
      dam *= 0.333; /* 1/3 damage if on 3rd attack */

    if (type == SKILL_BACKSTAB) {
      dam *= (1+GET_LEVEL(ch)/10);
      damage(ch, real_vict, dam, SKILL_BACKSTAB);
    } else
      damage(ch, real_vict, dam, w_type);
/* if they have reflect then reflect */
    if(IS_AFFECTED(real_vict,AFF_REFLECT) && dam > 0 && GET_POS(real_vict)> POS_MORTALLYW)
      damage(real_vict,ch,MIN(dam,MAX(GET_HIT(ch) - 1,0)),SPELL_SERPENT_SKIN);

/* offensive/defensive spells - Lighting/Fire Shield, Fire Wall */
    if (affected_by_spell(real_vict,SPELL_LIGHT_SHIELD)) 
      if (mag_savingthrow(real_vict,SAVING_SPELL))
        damage(real_vict,ch,MIN(GET_LEVEL(real_vict)/4,MAX(GET_HIT(ch)-1,0)),SPELL_LIGHT_SHIELD);
    if (affected_by_spell(real_vict,SPELL_FIRE_SHIELD))
      if (mag_savingthrow(real_vict,SAVING_SPELL))
        damage(real_vict,ch,MIN(GET_LEVEL(real_vict)/3,MAX(GET_HIT(ch)-1,0)),SPELL_FIRE_SHIELD);
    if (affected_by_spell(real_vict,SPELL_FIRE_WALL))
      if (mag_savingthrow(real_vict,SAVING_SPELL))
        damage(real_vict,ch,MIN(GET_LEVEL(real_vict)/2,MAX(GET_HIT(ch)-1,0)),SPELL_FIRE_WALL);

/* if its a magic weapon decided whether to cast or not then do it - Vader */
    if(wielded && FIGHTING(ch))
      if(IS_OBJ_STAT(wielded,ITEM_MAGIC) && (GET_OBJ_VAL(wielded,0) > 0))
        if(number(0,3))
          call_magic(ch,real_vict,NULL,GET_OBJ_VAL(wielded,0),2*GET_LEVEL(ch),CAST_MAGIC_OBJ);
  }

  /* DM - autoassist check */
if (GET_POS(victim) == POS_FIGHTING)
  for (char_room=world[ch->in_room].people;char_room;char_room=char_room->next_in_room) {
    if (!IS_NPC(char_room))
      if (AUTOASSIST(char_room) == ch)

        /* Ensure all in room and vict fighting 
        if ((world[ch->in_room].number == world[victim->in_room].number) && 
	    (world[ch->in_room].number == world[char_room->in_room].number) && 
	    (GET_POS(victim) = POS_FIGHTING)) */

        if (IS_NPC(victim) && 
	  (GET_POS(victim) == POS_FIGHTING) && 
          (world[char_room->in_room].number == world[victim->in_room].number) &&
	  (!FIGHTING(char_room)) && 
	  CAN_SEE(char_room,ch) && 
	  CAN_SEE(char_room,victim)) {

          act("$n assists $N!", FALSE, char_room, 0, ch, TO_NOTVICT);
          act("$N assists you!", FALSE, ch, 0, char_room, TO_CHAR);
          sprintf(buf,"You assist %s!\n\r",GET_NAME(ch));
          send_to_char(buf,char_room);     
          hit(char_room,victim,TYPE_UNDEFINED);
        }
  }
}

/* Special extra attacks granted by mounts to rider */
void mount_violence(struct char_data *rider ) {

	if( MOUNTING(rider) && !FIGHTING( MOUNTING(rider) ) ) {
	   send_to_char("Your mount joins the fray!\r\n", rider);
	   do_hit(MOUNTING(rider), (FIGHTING(rider))->player.name, 0, SCMD_HIT);
        }
	// Extra attack for item mounts
        if( MOUNTING_OBJ(rider) ) {
		send_to_char("Your mount lends you greater speed in combat.\r\n", rider);
		act("$n's mount lends $m greater speed against $s opponent.", FALSE,
			rider, 0,0, TO_ROOM);
		hit(rider, FIGHTING(rider), TYPE_UNDEFINED);
        }

}


/* modified by vader for multiattack.. */
/* control the fights going on.  Called every 2 seconds from comm.c. */
void perform_violence(void)
{
  struct char_data *ch;
  extern struct index_data *mob_index;
  int second = 0,third = 0, i, loop;

  for (ch = combat_list; ch; ch = next_combat_list) {
    next_combat_list = ch->next_fighting;

    loop = 1;

    if (FIGHTING(ch) == NULL || ch->in_room != FIGHTING(ch)->in_room)
      stop_fighting(ch);
    else {
      if (affected_by_spell(ch, SPELL_HASTE)) {
        loop = 2;
      }

      for (i=0; i < loop; i++) {
        if (i == 1)
          send_to_char("You are hastened and get more attacks\r\n", ch);
				
        if (FIGHTING(ch))	
          hit(ch, FIGHTING(ch), TYPE_UNDEFINED);

  /* the below bit is added to allow multiple attacks per turn - VADER */
        if(IS_NPC(ch)) 
          second = third = 100;
        else {
          second = GET_SKILL(ch, SKILL_2ND_ATTACK);
          third  = GET_SKILL(ch, SKILL_3RD_ATTACK);
        }
		
        if (((!IS_NPC(ch) && second) || 
             (IS_NPC(ch) && MOB_FLAGGED(ch,MOB_2ND_ATTACK))) && FIGHTING(ch))
          if((second + dice(3,(ch->aff_abils.dex))) > number(5,175))
            hit(ch, FIGHTING(ch), SKILL_2ND_ATTACK);
          else 
            damage(ch, FIGHTING(ch), 0, SKILL_2ND_ATTACK);

        if (((!IS_NPC(ch) && third) || 
            (IS_NPC(ch) && MOB_FLAGGED(ch,MOB_3RD_ATTACK))) && FIGHTING(ch))
          if((third  + dice(3,(ch->aff_abils.dex))) > number(5,200))
            hit(ch, FIGHTING(ch), SKILL_3RD_ATTACK);
          else 
            damage(ch, FIGHTING(ch), 0, SKILL_3RD_ATTACK);

/* vampires and wolfs get an extra attack when changed - Vader */
        if (affected_by_spell(ch,SPELL_CHANGED) && 
               (number(0,4) == 0) && FIGHTING(ch))
          if (PRF_FLAGGED(ch,PRF_WOLF))
            damage(ch,FIGHTING(ch),dice(4,GET_LEVEL(ch)),TYPE_CLAW);
          else if(PRF_FLAGGED(ch,PRF_VAMPIRE))
            damage(ch,FIGHTING(ch),dice(4,GET_LEVEL(ch)),TYPE_BITE);
      }
    }

    if (IS_NPC(ch)) {

      // DM - quick fix for clones which are dead at this stage ...
      // dead clones escaping through hit() or damage() ??
      if (IS_CLONE(ch) && GET_HIT(ch) < 0) {
        die_clone(ch, NULL);
        continue;
      }

      if (GET_MOB_WAIT(ch) > 0) {
        GET_MOB_WAIT(ch) -= PULSE_VIOLENCE;
        continue;
      }
      GET_MOB_WAIT(ch) = 0;
      if (GET_POS(ch) < POS_FIGHTING) {
        GET_POS(ch) = POS_FIGHTING;
        act("$n scrambles to $s feet!", TRUE, ch, 0, 0, TO_ROOM);
      }
    }

    if (!IS_NPC(ch)) {
      if (GET_CHAR_WAIT(ch) > 0) {
        GET_CHAR_WAIT(ch) -= PULSE_VIOLENCE;
        continue;
      }
      GET_CHAR_WAIT(ch) = 0;
      if (GET_POS(ch) < POS_FIGHTING) {
        GET_POS(ch) = POS_FIGHTING;
        act("$n scrambles to $s feet!", TRUE, ch, 0, 0, TO_ROOM);
			  send_to_char("You scramble to your feet\r\n", ch);
      }

      if( MOUNTING(ch) || MOUNTING_OBJ(ch) )
	mount_violence(ch);
    }

    if (MOB_FLAGGED(ch, MOB_SPEC) && mob_index[GET_MOB_RNUM(ch)].func != NULL)
      (mob_index[GET_MOB_RNUM(ch)].func) (ch, ch, 0, "");
  }
}
