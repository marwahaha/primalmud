/*************************************************************************
*   File: act.offensive.c                               Part of CircleMUD *
*  Usage: player-level commands of an offensive nature                    *
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
#include <string.h>
#include <assert.h>

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "screen.h"

/* extern variables */
extern struct zone_data *zone_table;
extern struct room_data *world;
extern struct descriptor_data *descriptor_list;
extern struct room_data *world;
extern struct index_data *obj_index;

/* extern functions */
/* void raw_kill(struct char_data * ch, struct char_data *killer); */


#define KILL_WOLF_VNUM  22301
#define KILL_VAMP_VNUM  22302

/* command used to kill wolf/vamps in one shot - Vader */
ACMD(do_slay)
{
  struct char_data *vict;
  struct obj_data *wielded = ch->equipment[WEAR_WIELD];
  int exp;
 
  one_argument(argument,arg);
 
  if(!*arg)
    send_to_char("Which vile fiend are you intending to slay??\r\n",ch);
  else if(!(vict = get_char_room_vis(ch,arg)))
    send_to_char("They don't seem to be here...\r\n",ch);
  else if(vict == ch)
    send_to_char("I agree you need to die, but lets let someone else do it, shall we?\r\n",ch);
  else if(IS_AFFECTED(ch,AFF_CHARM) && (ch->master == vict))
    act("How could you even consider slaying $N??",FALSE,ch,0,vict,TO_CHAR);
  else if(!wielded || !((GET_OBJ_VNUM(wielded) == KILL_WOLF_VNUM && PRF_FLAGGED(vict,PRF_WOLF)) ||
          (GET_OBJ_VNUM(wielded) == KILL_VAMP_VNUM && PRF_FLAGGED(vict,PRF_VAMPIRE))))
    send_to_char("You need to wield the right weapon to do a proper job.\r\n",ch);
  else if(!affected_by_spell(vict,SPELL_CHANGED))
    send_to_char("Maybe you should wait til they change before you slay them.\r\n",ch);
  else {
    if(IS_AFFECTED(ch,AFF_CHARM) && !IS_NPC(ch->master) && !IS_NPC(vict))
      return;
 
    if((PRF_FLAGGED(ch,PRF_WOLF) && PRF_FLAGGED(vict,PRF_WOLF)) ||
       (PRF_FLAGGED(ch,PRF_VAMPIRE) && PRF_FLAGGED(vict,PRF_VAMPIRE))) {
      send_to_char("Attempting to kill your own kind! How dare you?!\r\n",ch);
      SET_BIT(PLR_FLAGS(ch),PLR_KILLER);
      }
 
    if(PRF_FLAGGED(vict,PRF_WOLF)) {
      if((number(0,2) < 2) && (GET_LEVEL(vict) < LVL_IS_GOD)) {
        act("You drive $p deep into $N's ribcage!",FALSE,ch,wielded,vict,TO_CHAR);
        act("$n drives $p deep into $N's ribcage!",FALSE,ch,wielded,vict,TO_NOTVICT);
        act("$n drives $p deep into your ribcage, killing the beast within you.",FALSE,ch,wielded,vict,TO_VICT);
        act("$n changes back to normal as $e dies.",FALSE,vict,0,0,TO_ROOM);
        send_to_char("You revert to your original form as you die.\r\n",vict);
        exp = GET_LEVEL(vict) * 1000; /* give em some xp for it */
        gain_exp(ch,exp);
        sprintf(buf,"You receive %s%d%s experience points.\r\n",CCEXP(ch,C_NRM),exp,CCNRM(ch,C_NRM));
        send_to_char(buf,ch);
        sprintf(buf,"%s (werewolf) slain by %s",GET_NAME(vict),GET_NAME(ch));
        mudlog(buf,BRF,LVL_GOD,TRUE);
        gain_exp(vict,-(exp * 4));
        raw_kill(vict,ch);
        send_to_outdoor("The howl of a dying werewolf echoes across the land.\r\n");
      } else {
        act("Too slow! $N sees you, spins round, and rips your face off.",FALSE,ch,wielded,vict,TO_CHAR);
        act("$N dodges $n's attack, spins round, and rips $s face off.",FALSE,ch,wielded,vict,TO_NOTVICT);
        act("You see $n coming from miles away, and quickly do away with $m.",FALSE,ch,wielded,vict,TO_VICT);
        act("$n is dead!  R.I.P.",FALSE,ch,0,0,TO_ROOM);
        send_to_char("You are dead!  Sorry...\r\n",ch);
        exp = GET_LEVEL(ch) * 1000; /* you get xp for dodging */
        gain_exp(vict,exp);
        sprintf(buf,"You receive %s%d%s experience points.\r\n",CCEXP(ch,C_NRM),exp,CCNRM(ch,C_NRM));
        send_to_char(buf,vict);
        sprintf(buf,"%s killed while attempting to slay %s (werewolf)",
                GET_NAME(ch),GET_NAME(vict));
        mudlog(buf,BRF,LVL_GOD,TRUE);
        gain_exp(ch,-(exp * 4));
        raw_kill(ch,vict);
        }
    } else if(PRF_FLAGGED(vict,PRF_VAMPIRE)) {
      if((number(0,2) < 2) && (GET_LEVEL(vict) < LVL_IS_GOD)) {
        act("You stab $p firmly into $N's heart!",FALSE,ch,wielded,vict,TO_CHAR);
        act("$n stabs $p firmly into $N's heart!",FALSE,ch,wielded,vict,TO_NOTVICT);
        act("$n stabs $p firmly into your heart!",FALSE,ch,wielded,vict,TO_VICT);
        act("$n explodes.",FALSE,vict,0,0,TO_ROOM);
        send_to_char("You explode violently as the spell is broken.\r\n",vict);
        exp = GET_LEVEL(vict) * 1000;
        gain_exp(ch,exp);
        sprintf(buf,"You receive %s%d%s experience points.\r\n",CCEXP(ch,C_NRM),exp,CCNRM(ch,C_NRM));
        send_to_char(buf,ch);
        sprintf(buf,"%s (vampire) slain by %s",GET_NAME(vict),GET_NAME(ch));
        mudlog(buf,BRF,LVL_GOD,TRUE);
        gain_exp(vict,-(exp * 4));
        raw_kill(vict,ch);
        send_to_outdoor("You hear the sound of an exploding vampire in the distance.\r\n");
      } else {
        act("$N is suddenly behind you! You feel $S teeth sink into your neck...",FALSE,ch,0,vict,TO_CHAR);
        act("$N dodges $n's attack, grabs $m from behind and bites $s neck!",FALSE,ch,0,vict,TO_NOTVICT);
        act("You dodge $n's attack, grab $m by the neck, and drink.",FALSE,ch,0,vict,TO_VICT);
        act("$n is dead!  R.I.P.",FALSE,ch,0,0,TO_ROOM);
        send_to_char("You are dead!  Sorry...\r\n",ch);
        exp = GET_LEVEL(ch) * 1000;
        gain_exp(vict,exp);
        sprintf(buf,"You receive %s%d%s experience points.\r\n",CCEXP(ch,C_NRM),exp,CCNRM(ch,C_NRM));
        send_to_char(buf,vict);
        sprintf(buf,"%s killed while attempting to slay %s (vampire)",
                GET_NAME(ch),GET_NAME(vict));
        mudlog(buf,BRF,LVL_GOD,TRUE);
        gain_exp(ch,-(exp * 4));
        raw_kill(ch,vict);
        }
      }
  }
}


/* Gun defines */
/* #define BASE_GUN_TYPE		20
#define MAX_GUN_TYPES		30

#define OBJ_IS_GUN(x)			((GET_OBJ_VAL((x),3) & 0x7fff) >= BASE_GUN_TYPE \
													  && (GET_OBJ_VAL((x),3) & 0x7fff) <= \
													 (BASE_GUN_TYPE + MAX_GUN_TYPES))

*/

ACMD(do_shoot)
{
  char shoot_message[] = "$n aims a $p at $N and starts shooting.\n\r";
  char shooter_message[] = "You aim a $p at $N and start shooting\n\r";
  char finger_mess[] = "$n points their finger at $N and says \"BANG!\".\n\r";
  char fingerer[] = "You point you finger at $N and pretend to shoot them.\n\r";
  struct obj_data *wielded = ch->equipment[WEAR_HOLD];
  struct char_data *vict;
  int w_type;
  SPECIAL(receptionist);
  SPECIAL(postmaster);
  extern struct index_data *mob_index;

  one_argument(argument, arg);

  if (!*arg)
    send_to_char("Shoot who?\r\n", ch);
  else if (!(vict = get_char_room_vis(ch, arg)))
    send_to_char("They don't seem to be here.\r\n", ch);
  else if ((GET_LEVEL(ch) < LVL_GOD)&&(GET_MOB_SPEC(vict) == postmaster))      
        send_to_char("Try SHOOTING someone who cares!!.\r\n", ch);
  else if ((GET_LEVEL(ch) < LVL_GOD)&&(GET_MOB_SPEC(vict) == receptionist))
        send_to_char("Try SHOOTING someone who cares!!.\r\n", ch);     
  else if (vict == ch) {
		if (wielded)
		{
 	   w_type = GET_OBJ_VAL(wielded, 3) & 0x7fff;
/* 	   if ((w_type >= BASE_GUN_TYPE) && (w_type < (BASE_GUN_TYPE + MAX_GUN_TYPES))) */
		if (OBJ_IS_GUN(wielded))
 	   {
 	     send_to_char("You shoot yourself...OUCH!.\r\n", ch);
 	     act("$n shoots $mself, and dies!", FALSE, ch, 0, vict, TO_ROOM);
 	     raw_kill(ch,NULL);
 	   }
		}
    else
    {
      send_to_char("You point your finger at your head and shout \"POW!\".", ch);
      act("$n points a finger at their head and shouts \"POW!\"", FALSE, ch, 0, vict, TO_ROOM);
    }  
  } else if (IS_AFFECTED(ch, AFF_CHARM) && (ch->master == vict))
    act("$N is just such a good friend, you simply can't shoot $M.", FALSE, ch, 0, vict, TO_CHAR);
  else {
    if (!IS_SET(zone_table[world[ch->in_room].zone].zflag , ZN_PK_ALLOWED)) {
      if (!IS_NPC(vict) && !IS_NPC(ch) && (subcmd != SCMD_MURDER)) {
	send_to_char("Use 'murder' to shoot another player.\r\n", ch);
	return;
      }
      if (IS_AFFECTED(ch, AFF_CHARM) && !IS_NPC(ch->master) && !IS_NPC(vict))
	return;			/* you can't order a charmed pet to attack a
				 * player */
    }
    if ((GET_POS(ch) == POS_STANDING) && (vict != FIGHTING(ch))) {
      if (!wielded)
      {
        act(finger_mess, TRUE, ch, 0, vict, TO_ROOM);
	act(fingerer, TRUE, ch, 0, vict, TO_CHAR);
        return;
      }
      w_type = GET_OBJ_VAL(wielded, 3) & 0x7fff;
      if ((w_type < BASE_GUN_TYPE) || (w_type > (BASE_GUN_TYPE + MAX_GUN_TYPES)))
      {
        send_to_char("You can't shoot that!\n\r", ch);
        return;
      }  

      act(shoot_message, TRUE, ch, wielded, vict, TO_ROOM);
      act(shooter_message, TRUE, ch, wielded, vict, TO_CHAR);
      hit(ch, vict, TYPE_UNDEFINED);
      WAIT_STATE(ch, PULSE_VIOLENCE + 2);
    } else
      send_to_char("You do the best you can!\r\n", ch);
  }

}


ACMD(do_loadweapon)
{
  char load_message[] = "$n reloads $p";
  int w_type, gun_ammo, max_gun_ammo, ammo, ammo_needed;
  int successful = 0;
  struct obj_data *proto_gun, *obj, *next_obj, *wielded = ch->equipment[WEAR_WIELD];
  
  if (wielded && GET_OBJ_TYPE(wielded) == ITEM_WEAPON)
  {
    w_type = GET_OBJ_VAL(wielded, 3) & 0x7fff;
    proto_gun = read_object(GET_OBJ_RNUM(wielded), REAL);
    gun_ammo = GET_OBJ_VAL(wielded, 0);
    max_gun_ammo = GET_OBJ_VAL(proto_gun, 0);
    ammo_needed = max_gun_ammo - gun_ammo;

    if ((w_type >= BASE_GUN_TYPE) && (w_type < (BASE_GUN_TYPE + MAX_GUN_TYPES)))
    {
      if (ammo_needed <= 0)
      {
	send_to_char("The weapon is already fully loaded.\n\r", ch);
        return;
      }

/* look thru the players inventory and try to load the weapon */
      for (obj=ch->carrying;obj;obj=next_obj)
      {
        next_obj = obj->next_content;
  
        if (GET_OBJ_VAL(obj, 3) & 0x4000)  /* is it ammo */
          if ((GET_OBJ_VAL(obj, 3) & 0x2fff) == w_type)
          {                                /* does it belong to this weapon */
            ammo = GET_OBJ_VAL(obj, 0);
            if (ammo < ammo_needed) /* just load as much as I can into the gun */
            {                          
              GET_OBJ_VAL(wielded, 0) += ammo;
              obj_from_char(obj);
              extract_obj(obj);
              successful += ammo;
            }
	    else        /* load it up full */
            {
              GET_OBJ_VAL(obj, 0) -= ammo_needed;
              GET_OBJ_VAL(wielded, 0) = max_gun_ammo;
              act(load_message, TRUE, ch, wielded, 0, TO_ROOM);
              send_to_char("The weapon is fully loaded now\n\r", ch);
              if (GET_OBJ_VAL(obj, 0) <= 0)
              {
                obj_from_char(obj);
                extract_obj(obj);
              }
              return;
            }
          } 
      } 
      if (!successful)
        send_to_char("You don't seem to have the right ammo for this weapon.\n\r", ch);    
      else
      {
        act(load_message, TRUE, ch, wielded, 0, TO_ROOM);   
	if (ammo_needed)
          send_to_char("You didn't have enuough ammo to fully load the weapon.\n\r", ch);
        else
          send_to_char("The weapon is fully loaded now.\n\r", ch);
      }
    }     
  }
  else
    send_to_char("You cannot reload that!\n\r", ch);
}

ACMD(do_assist)
{
  struct char_data *helpee, *opponent;

  if (FIGHTING(ch)) {
    send_to_char("You're already fighting!  How can you assist someone else?\r\n", ch);
    return;
  }
  one_argument(argument, arg);

  if (!*arg)
    send_to_char("Whom do you wish to assist?\r\n", ch);
  else if (!(helpee = get_char_room_vis(ch, arg)))
    send_to_char(NOPERSON, ch);
  else if (helpee == ch)
    send_to_char("You can't help yourself any more than this!\r\n", ch);
  else {
    for (opponent = world[ch->in_room].people; opponent &&
	 (FIGHTING(opponent) != helpee); opponent = opponent->next_in_room);

    if (!opponent)
      act("But nobody is fighting $M!", FALSE, ch, 0, helpee, TO_CHAR);
    else if (!CAN_SEE(ch, opponent))
      act("You can't see who is fighting $M!", FALSE, ch, 0, helpee, TO_CHAR);
    else if (!IS_SET(zone_table[world[ch->in_room].zone].zflag,ZN_PK_ALLOWED) && !IS_NPC(opponent))	/* prevent accidental pkill */
      act("Use 'murder' if you really want to attack $N.", FALSE,
	  ch, 0, opponent, TO_CHAR);
    else {
      send_to_char("You join the fight!\r\n", ch);
      act("$N assists you!", 0, helpee, 0, ch, TO_CHAR);
      act("$n assists $N.", FALSE, ch, 0, helpee, TO_NOTVICT);
      hit(ch, opponent, TYPE_UNDEFINED);
    }
  }
}


ACMD(do_hit)
{
  struct char_data *vict;
  int w_type;
  struct obj_data *wielded = ch->equipment[WEAR_WIELD];
  SPECIAL(postmaster);
  SPECIAL(receptionist);
  extern struct index_data *mob_index;

  one_argument(argument, arg);

  // Check that no mounted critter is trying to initiate an attack
  if( IS_NPC(ch) && MOUNTING(ch) && !FIGHTING(MOUNTING(ch)) )
	return;

  if(IS_AFFECTED(ch,AFF_CHARM) && !ch->master)
    REMOVE_BIT(AFF_FLAGS(ch),AFF_CHARM);

  if (!*arg)
    send_to_char("Hit who?\r\n", ch);
  else if (!(vict = get_char_room_vis(ch, arg)))
    send_to_char("They don't seem to be here.\r\n", ch);
  else if (vict == ch) {
    send_to_char("You hit yourself...OUCH!.\r\n", ch);
    act("$n hits $mself, and says OUCH!", FALSE, ch, 0, vict, TO_ROOM);
  } else if (IS_AFFECTED(ch, AFF_CHARM) && (ch->master == vict))
    act("$N is just such a good friend, you simply can't hit $M.", FALSE, ch, 0, vict, TO_CHAR);
  else if ((GET_LEVEL(ch) < LVL_GOD)&&(GET_MOB_SPEC(vict) == postmaster))    
        send_to_char("You cannot attack the postmaster!!\r\n", ch);
  else if ((GET_LEVEL(ch) < LVL_GOD)&&(GET_MOB_SPEC(vict) == receptionist))    
        send_to_char("You cannot attack the Receptionist!\r\n", ch);
  else {
    // Attacking a mount is effectively attacking the rider
    if( MOUNTING(vict) && IS_NPC(vict) )
	vict = MOUNTING(vict);

    if (!IS_SET(zone_table[world[ch->in_room].zone].zflag , ZN_PK_ALLOWED)) {
      if (!IS_NPC(vict) && !IS_NPC(ch) && (subcmd != SCMD_MURDER)) {
	send_to_char("Use 'murder' to hit another player.\r\n", ch);
	return;
      }
      if (IS_AFFECTED(ch, AFF_CHARM) && !IS_NPC(ch->master) && !IS_NPC(vict))
	return;			/* you can't order a charmed pet to attack a
				 * player */
    }
    if ((GET_POS(ch) == POS_STANDING) && (vict != FIGHTING(ch))) {
			if (wielded)
			{
      	w_type = GET_OBJ_VAL(wielded, 3) & 0x7fff;
      	if ((w_type >= BASE_GUN_TYPE) && (w_type < (BASE_GUN_TYPE + MAX_GUN_TYPES)))
      	{
      	  send_to_char("You would do better to shoot this weapon!\n\r", ch);
					return;
  	    }  
			}
      hit(ch, vict, TYPE_UNDEFINED);
      WAIT_STATE(ch, PULSE_VIOLENCE + 2);
      // Order mount to attack
      if( !IS_NPC(ch) && MOUNTING(ch) ) {
	send_to_char("You order your mount to attack.\r\n", ch);
	act("$n orders $s mount to attack.\r\n", FALSE,ch, 0, 0, TO_ROOM);
	do_hit(MOUNTING(ch), arg, 0, SCMD_HIT);
      }
    } else
      send_to_char("You do the best you can!\r\n", ch);
  }
}



ACMD(do_kill)
{
  struct char_data *vict;

  if ((GET_LEVEL(ch) < LVL_IMPL) || IS_NPC(ch)) {
    do_hit(ch, argument, cmd, subcmd);
    return;
  }
  one_argument(argument, arg);

  if (!*arg) {
    send_to_char("Kill who?\r\n", ch);
  } else {
    if (!(vict = get_char_room_vis(ch, arg)))
      send_to_char("They aren't here.\r\n", ch);
    else if (ch == vict)
      send_to_char("Your mother would be so sad.. :(\r\n", ch);
    else {
      act("You chop $M to pieces!  Ah!  The blood!", FALSE, ch, 0, vict, TO_CHAR);
      act("$N chops you to pieces!", FALSE, vict, 0, ch, TO_CHAR);
      act("$n brutally slays $N!", FALSE, ch, 0, vict, TO_NOTVICT);
      raw_kill(vict,ch);
    }
  }
}



ACMD(do_backstab)
{
  struct char_data *vict;
  byte percent, prob;
  SPECIAL(receptionist);
  SPECIAL(postmaster);
  extern struct index_data *mob_index;

  one_argument(argument, buf);

	if (!has_stats_for_skill(ch, SKILL_BACKSTAB))
		return;

  if (!(vict = get_char_room_vis(ch, buf))) {
    send_to_char("Backstab who?\r\n", ch);
    return;
  }
  if (vict == ch) {
    send_to_char("How can you sneak up on yourself?\r\n", ch);
    return;
  }
  if (!ch->equipment[WEAR_WIELD]) {
    send_to_char("You need to wield a weapon to make it a success.\r\n", ch);
    return;
  }
  if (GET_OBJ_VAL(ch->equipment[WEAR_WIELD], 3) != TYPE_PIERCE - TYPE_HIT) {
    send_to_char("Only piercing weapons can be used for backstabbing.\r\n", ch);
    return;
  }
  if (FIGHTING(vict)) {
    send_to_char("You can't backstab a fighting person -- they're too alert!\r\n", ch);
    return;
  }
  if( MOUNTING(vict) && IS_NPC(vict) ) {
	send_to_char("You can't backstab someone's mount!\r\n", ch);
	return;
  }
  if (IS_CLONE(vict)) {
    send_to_char("You can't backstab a clone!\r\n", ch);
    return;
  }
  
  if (MOB_FLAGGED(vict, MOB_AWARE)){
    switch (number(1,4)){
    case 1:
        send_to_char("Your weapon hits an invisible barrier and fails to penetrate it.\r\n", ch);
        break;
    case 2:
	act("$N skillfuly blocks your backstab and attacks with rage!", FALSE, ch, 0 , vict, TO_CHAR);
        break;
    case 3:
	act("$N cleverly avoids your backstab.", FALSE, ch, 0 , vict, TO_CHAR);
        break;
    case 4:
	act("$N steps to the side avoiding your backstab!", FALSE, ch, 0 , vict, TO_CHAR);
        break;
    }
    hit(vict, ch, TYPE_UNDEFINED);
    return;
  }
  if ((GET_LEVEL(ch) < LVL_GOD)&&(GET_MOB_SPEC(vict) == postmaster)) {  
        send_to_char("The postmaster easily dodges your pitiful attempt to backstab him.\r\n", ch);
        act("$N laughs in $n's face as $E dances out of the way of $n's backstab.", FALSE, ch,0,vict,TO_ROOM); 
        return;
  }
  if ((GET_LEVEL(ch) < LVL_GOD)&&(GET_MOB_SPEC(vict) == receptionist)){
        send_to_char("The Receptionist easily dodges your pitiful attempt to backstab her.\r\n", ch);
        act("$N laughs in $n's face as $E dances out of the way of $n's backstab.", FALSE, ch,0,vict,TO_ROOM); 
        return;
  }

  percent = number(1, 101);	/* 101% is a complete failure */
  prob = GET_SKILL(ch, SKILL_BACKSTAB);

  if (AWAKE(vict) && (percent > prob))
    damage(ch, vict, 0, SKILL_BACKSTAB);
  else
    hit(ch, vict, SKILL_BACKSTAB);
}



ACMD(do_order)
{
  char name[100], message[MAX_INPUT_LENGTH], clone_order[MAX_INPUT_LENGTH];
  char buf[256];
  bool found = FALSE;
  int org_room;
  struct char_data *vict;
  struct follow_type *k, *j;
  extern struct index_data *mob_index;

  half_chop(argument, name, message);

  one_argument(message, clone_order);

  if (!*name || !*message)
    send_to_char("Order who to do what?\r\n", ch);
  /* DM - add check for order clones */
  else if ((!(vict = get_char_room_vis(ch, name)) && !is_abbrev(name, "followers")) && !is_abbrev(name, "clones")) { 
/*    if (!is_abbrev(name, "clones")) */ 
      send_to_char("That person isn't here.\r\n", ch);
  } else if (ch == vict)
    send_to_char("You obviously suffer from skitzofrenia.\r\n", ch);

  else {
    if (IS_AFFECTED(ch, AFF_CHARM)) {
      send_to_char("Your superior would not aprove of you giving orders.\r\n", ch);
      return;
    }
    if (vict) {
      if ((GET_LEVEL(vict) > LVL_ANGEL) && (GET_LEVEL(ch) < GET_LEVEL(vict)))
      { /* ARTUS - Another bugfix.. Boz could switch into carolyn after she'd
         * charmed me and order me to advance him. */
        send_to_char("Their mind is to strong for you.\r\n", ch);
        return;
      }
      sprintf(buf, "$N orders you to '%s'", message);
      act(buf, FALSE, vict, 0, ch, TO_CHAR);
      act("$n gives $N an order.", FALSE, ch, 0, vict, TO_ROOM);

      if ((vict->master != ch) || !IS_AFFECTED(vict, AFF_CHARM))
	act("$n has an indifferent look.", FALSE, vict, 0, 0, TO_ROOM);
      else {
	send_to_char(OK, ch);
	command_interpreter(vict, message);
      }
    } else {			/* This is order "followers"/"clones" */
      sprintf(buf, "$n issues the order '%s'.", message);
      act(buf, FALSE, ch, 0, vict, TO_ROOM);

      org_room = ch->in_room;

      /* DM - changed "order followers" to a do while in the case that the follower
		dies whilst performing the command - CLONES (order followers die)
	moved die_clone in here now due to problems */ 
      if ((k = ch->followers))
      do {
        j=k->next; 
	if ((org_room == k->follower->in_room) || IS_CLONE(k->follower)) 
	  if (IS_AFFECTED(k->follower, AFF_CHARM)) {
	    found = TRUE;
            if (IS_CLONE(k->follower) && !(str_cmp(clone_order,"die"))) 
              die_clone(k->follower,NULL);
            else 
	      command_interpreter(k->follower, message);
	  }
        k=j; 
      } while (k); 

/*      for (k = ch->followers; k; k = k->next) {
	if (org_room == k->follower->in_room)
	  if (IS_AFFECTED(k->follower, AFF_CHARM)) {
	    found = TRUE;
	    command_interpreter(k->follower, message);
	  }
      } */

      if (found)
	send_to_char(OK, ch);
      else
	send_to_char("Nobody here is a loyal subject of yours!\r\n", ch);
    }
  }
}



ACMD(do_flee)
{
  int i, attempt, loss;

  if (IS_AFFECTED(ch, AFF_PARALYZED)){
     send_to_char("PANIC! Your legs refuse to move!!\r\n", ch);
     return; 
  }
	
  for (i = 0; i < 6; i++) {
    attempt = number(0, NUM_OF_DIRS - 1);	/* Select a random direction */
    if (CAN_GO(ch, attempt) &&
	!IS_SET(ROOM_FLAGS(EXIT(ch, attempt)->to_room), ROOM_DEATH)) {
      act("$n panics, and attempts to flee!", TRUE, ch, 0, 0, TO_ROOM);
      if (do_simple_move(ch, attempt, TRUE)) {
	send_to_char("You flee head over heels.\r\n", ch);
	if (FIGHTING(ch)) {
	  if (!IS_NPC(ch)) {
	    if (!(ch->desc))
              loss=0;
            else
              loss = GET_MAX_HIT(FIGHTING(ch)) - GET_HIT(FIGHTING(ch));
	    loss *= GET_LEVEL(FIGHTING(ch));
	    if (loss>300000)
		loss = 300000;
	    gain_exp(ch, -loss);
	  }
	  if (FIGHTING(FIGHTING(ch)) == ch)
	    stop_fighting(FIGHTING(ch));
	  stop_fighting(ch);
	}
      } else {
	act("$n tries to flee, but can't!", TRUE, ch, 0, 0, TO_ROOM);
      }
      return;
    }
  }
  send_to_char("PANIC!  You couldn't escape!\r\n", ch);
}

ACMD(do_retreat)
{
  int i, attempt, testroll;
 
  if (IS_AFFECTED(ch, AFF_PARALYZED)){
     send_to_char("PANIC! Your legs refuse to move!!\r\n", ch);
     return; 
  }
        
  for (i = 0; i < 6; i++) {
    attempt = number(0, NUM_OF_DIRS - 1);       /* Select a random direction */
    if (CAN_GO(ch, attempt) &&
        !IS_SET(ROOM_FLAGS(EXIT(ch, attempt)->to_room), ROOM_DEATH)) {
        testroll = number(0,100);
	if (testroll>=50){
        	if (do_simple_move(ch, attempt, TRUE)) {
			if (FIGHTING(ch)){
        			if (FIGHTING(FIGHTING(ch)) == ch)
            			  stop_fighting(FIGHTING(ch));
          		stop_fighting(ch);
      			act("$n strategicaly withdraws from the battle!", TRUE, ch, 0, 0, TO_ROOM);
        		send_to_char("You strategicaly withdraw from the battle.\r\n", ch);
 			}else{
      			act("$n strategicaly withdraws from the room!", TRUE, ch, 0, 0, TO_ROOM);
        		send_to_char("You strategicaly withdraw from the room.\r\n", ch);
			}	
        	}
        }else{
              act("$n tries to retreat, but fails!", TRUE, ch, 0, 0, TO_ROOM);
              send_to_char("Your retreat is blocked off. PANIC!\r\n", ch);
             }
      } else {
        act("$n tries to retreat, but fails!", TRUE, ch, 0, 0, TO_ROOM);
        send_to_char("Your retreat is blocked off. PANIC!\r\n", ch);
      }
      return;
  }
}


ACMD(do_bash)
{
  struct char_data *vict;
  byte percent, prob;
  SPECIAL(postmaster);
  SPECIAL(receptionist);
  extern struct index_data *mob_index;

  one_argument(argument, arg);

	if (!has_stats_for_skill(ch, SKILL_BASH))
		return;

  if (!(vict = get_char_room_vis(ch, arg))) {
    if (FIGHTING(ch)) {
      vict = FIGHTING(ch);
    } else {
      send_to_char("Bash who?\r\n", ch);
      return;
    }
  }
  // check if victim is mounted and not already fighting it
  if( IS_NPC(vict) && MOUNTING(vict) && !(FIGHTING(ch) == vict) ) {
	send_to_char("That's someone's mount. Use 'murder' to attack another player.\r\n", ch);
	return;
  }

  if (vict == ch) {
    send_to_char("Aren't we funny today...\r\n", ch);
    return;
  }

  if (!ch->equipment[WEAR_WIELD]) {
    send_to_char("You need to wield a weapon to make it a success.\r\n", ch);
    return;
  }
  if (ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL)) {
    send_to_char("This room has a nice peaceful feeling.\r\n", ch);
    return;
  }
  if ((GET_LEVEL(ch) < LVL_GOD)&&(GET_MOB_SPEC(vict) == postmaster)) {
        send_to_char("You cannot bash the postmaster!!\r\n", ch);
	return;
  }
  if ((GET_LEVEL(ch) < LVL_GOD)&&(GET_MOB_SPEC(vict) == receptionist)){
        send_to_char("You cannot bash the Receptionist!\r\n", ch);
	return;
  }
  if (!IS_SET(zone_table[world[ch->in_room].zone].zflag , ZN_PK_ALLOWED) && !IS_NPC(vict))
  {
    send_to_char("Player Killing is not allowed in this Zone.", ch);
    act("$n just tried to bash $N, and fell flat on their face!", FALSE, ch, 0, vict, TO_NOTVICT);
    GET_POS(ch) = POS_SITTING;
    return;
  }

  if (GET_POS(vict) < POS_FIGHTING){
	send_to_char("Your victim is already down!.\r\n", ch);
	return;
  }

  percent = number(1, 111);  /* 101% is a complete failure */
  prob = GET_SKILL(ch, SKILL_BASH);

  if (MOB_FLAGGED(vict, MOB_NOBASH))
	percent= 101;
  if (PRF_FLAGGED(ch,PRF_MORTALK))
	percent=101;

  if (percent > prob) {
    damage(ch, vict, 0, SKILL_BASH);
    GET_POS(ch) = POS_SITTING;
  } else {
    damage(ch, vict, GET_LEVEL(ch), SKILL_BASH);
    GET_POS(vict) = POS_SITTING;
    WAIT_STATE(vict, PULSE_VIOLENCE * 3);
  }
  WAIT_STATE(ch, PULSE_VIOLENCE * 3);
}


ACMD(do_rescue)
{
  struct char_data *vict, *tmp_ch;
  byte percent, prob;

  one_argument(argument, arg);

	if (!has_stats_for_skill(ch, SKILL_RESCUE))
		return;

  if (!(vict = get_char_room_vis(ch, arg))) {
    send_to_char("Who do you want to rescue?\r\n", ch);
    return;
  }
  if (vict == ch) {
    send_to_char("What about fleeing instead?\r\n", ch);
    return;
  }
  if (FIGHTING(ch) == vict) {
    send_to_char("How can you rescue someone you are trying to kill?\r\n", ch);
    return;
  }
  for (tmp_ch = world[ch->in_room].people; tmp_ch &&
       (FIGHTING(tmp_ch) != vict); tmp_ch = tmp_ch->next_in_room);

  if (!tmp_ch) {
    act("But nobody is fighting $M!", FALSE, ch, 0, vict, TO_CHAR);
    return;
  }
    percent = number(1, 101);	/* 101% is a complete failure */
    prob = GET_SKILL(ch, SKILL_RESCUE);

    if (percent > prob) {
      send_to_char("You fail the rescue!\r\n", ch);
      return;
    }
    send_to_char("Banzai!  To the rescue...\r\n", ch);
    act("You are rescued by $N, you are confused!", FALSE, vict, 0, ch, TO_CHAR);
    act("$n heroically rescues $N!", FALSE, ch, 0, vict, TO_NOTVICT);

    if (FIGHTING(vict) == tmp_ch)
      stop_fighting(vict);
    if (FIGHTING(tmp_ch))
      stop_fighting(tmp_ch);
    if (FIGHTING(ch))
      stop_fighting(ch);

    set_fighting(ch, tmp_ch);
    set_fighting(tmp_ch, ch);

    WAIT_STATE(vict, 2 * PULSE_VIOLENCE);
  

}



ACMD(do_kick)
{
  struct char_data *vict;
  byte percent, prob;
  SPECIAL(receptionist);
  SPECIAL(postmaster);
  extern struct index_data *mob_index;

  one_argument(argument, arg);

	if (!has_stats_for_skill(ch, SKILL_KICK))
		return;

  if (!(vict = get_char_room_vis(ch, arg))) {
    if (FIGHTING(ch)) {
      vict = FIGHTING(ch);
    } else {
      send_to_char("Kick who?\r\n", ch);
      return;
    }
  }
  // check if victim is mounted and not already fighting it
  if( IS_NPC(vict) && MOUNTING(vict) && !(FIGHTING(ch) == vict) ) {
	send_to_char("That's someone's mount. Use 'murder' to attack another player.\r\n", ch);
	return;
  }
  if (vict == ch) {
    send_to_char("Aren't we funny today...\r\n", ch);
    return;
  }
  if ((GET_LEVEL(ch) < LVL_GOD)&&(GET_MOB_SPEC(vict) == postmaster)) {  
        send_to_char("The postmaster is too fast for you to kick.\r\n", ch);
        return;
  }
  if ((GET_LEVEL(ch) < LVL_GOD)&&(GET_MOB_SPEC(vict) == receptionist)){
        send_to_char("The receptionist is too fast for you to kick.\r\n", ch);
        return;
  }

  if (!IS_SET(zone_table[world[ch->in_room].zone].zflag , ZN_PK_ALLOWED) && !IS_NPC(vict))
  {
    send_to_char("Player Killing is not allowed in this Zone.", ch);
    act("$n tries to kick $N, and fails miserably!",FALSE, ch, 0, vict, TO_NOTVICT);
    return;
  }

  percent = ((10 - (GET_AC(vict) / 10)) << 1) + number(1, 101);	/* 101% is a complete
								 * failure */
  prob = GET_SKILL(ch, SKILL_KICK);

  if (percent > prob) {
    damage(ch, vict, 0, SKILL_KICK);
  } else
    damage(ch, vict, GET_LEVEL(ch) >> 1, SKILL_KICK);
  WAIT_STATE(ch, PULSE_VIOLENCE * 3);
}

/* primal scream skill. hits all enemies in room. can only be used at start
 * not actually during the fight.. - Vader
 */
ACMD(do_scream)
{
  byte percent, prob;
  struct char_data *vict, *next_vict;
  room_num room;
  int door,dam,skip = 0;
  SPECIAL(postmaster);
  SPECIAL(receptionist);
  extern struct index_data *mob_index;

	if (!has_stats_for_skill(ch, SKILL_PRIMAL_SCREAM))
		return;

  prob = GET_SKILL(ch, SKILL_PRIMAL_SCREAM);
  percent = number(1,101); /* 101 is a complete failure */

  if(FIGHTING(ch)) {
    send_to_char("You can't prepare yourself properly while fighting!\r\n",ch);
    WAIT_STATE(ch, PULSE_VIOLENCE * 3);
  } else if(percent > prob) {
	   act("$n lets out a feeble little wimper as $e attempts a primal scream.",FALSE,ch,0,0,TO_ROOM);
	   act("You let out a sad little wimper.",FALSE,ch,0,0,TO_CHAR);
	 } else {
	   act("$n inhales deeply and lets out an ear shattering scream!!\r\n",FALSE,ch,0,0,TO_ROOM);
	   act("You fill your lungs to capacity and let out an ear shattering scream!!",FALSE,ch,0,0,TO_CHAR);
	   room = ch->in_room;
	  for(door=0; door<NUM_OF_DIRS; door++) 
		{ /* this shood make it be heard in a 4 room radius */
	  	if (!world[ch->in_room].dir_option[door])
				continue;

	  	ch->in_room = world[ch->in_room].dir_option[door]->to_room;
	  	if(room != ch->in_room && ch->in_room != NOWHERE)
	  	  act("You hear a frightening scream coming from somewhere nearby...",FALSE,ch,0,0,TO_ROOM);
	  	ch->in_room = room;
	  }
	   for (vict = world[ch->in_room].people; vict; vict = next_vict) {
	     next_vict = vict->next_in_room;

	     skip = 0;
	     if(vict == ch)
	       skip = 1; /* ch is the victim skip to next person */
	     if(IS_NPC(ch) && IS_NPC(vict) && !IS_AFFECTED(vict, AFF_CHARM))
	       skip = 1; /* if ch is a mob only hit other mobs if they are charmed */
	     if(!IS_NPC(vict) && GET_LEVEL(vict) >= LVL_IS_GOD)
	       skip = 1; /* dont bother gods with it */
     	     if ((GET_LEVEL(ch) < LVL_GOD)&&(GET_MOB_SPEC(vict) == postmaster)) 
	 	skip = 1; 
             if ((GET_LEVEL(ch) < LVL_GOD)&&(GET_MOB_SPEC(vict) == receptionist))
		skip = 1;
	     if(!IS_NPC(ch) && !IS_NPC(vict))
	       skip = 1; /* dont hit players with it */
	     if(!IS_NPC(ch) && IS_NPC(vict) && IS_AFFECTED(vict, AFF_CHARM))
	       skip = 1; /* dont hit charmed mobs */

	     if(!(skip)) {
	       dam = dice(4, GET_AFF_DEX(ch)) + 6; /* max dam of around 90 */
	       damage(ch,vict,dam,SKILL_PRIMAL_SCREAM);
	       }
	     }
	     WAIT_STATE(ch, PULSE_VIOLENCE * 3);
	 }
}

/* looks around for people nearby. cant be used indoors. how far you see
 * depends on height - Vader
 */
ACMD(do_scan)
{
  byte prob, percent;
  int dist,door,sqcount,none = TRUE;
  struct room_direction_data *pexit;
  struct char_data * rch;
  char dirs[NUM_OF_DIRS][7];

	if (!has_stats_for_skill(ch, SKILL_SCAN))
		return;
  sprintf(dirs[NORTH],"north");
  sprintf(dirs[SOUTH],"south");
  sprintf(dirs[EAST],"east");
  sprintf(dirs[WEST],"west");
  sprintf(dirs[UP],"up");
  sprintf(dirs[DOWN],"down");

  *buf = '\0';

  prob = GET_SKILL(ch, SKILL_SCAN);
  percent = number(1,101);

  if(FIGHTING(ch)) {
    send_to_char("Wouldn't stopping to have a look round now be a bit stupid??\r\n",ch);
    return;
    }

  if(percent < prob) {

    if(IS_AFFECTED(ch, AFF_BLIND)) {
      send_to_char("You can't see a thing. You are blind!\r\n",ch);
      return;
    }

    if((IS_DARK(ch->in_room)) && (!CAN_SEE_IN_DARK(ch))) {
      send_to_char ("Too dark to tell.",ch);
      return;
    }

    *buf2 = '\0';

    send_to_char("You scan your surroundings and see the following:\r\n\n",ch);

    dist = GET_HEIGHT(ch) / 50; /* calc how far they can see */
    if(dist < 2)
      dist = 1; /* make sure that they can at least see 1 square */

    if(IS_SET(ROOM_FLAGS(ch->in_room),ROOM_INDOORS))
      dist = 0; /* if indoors you can only see the current room */

    if (world[ch->in_room].people != NULL)  {
      for(rch=world[ch->in_room].people; rch!=NULL; rch=rch->next_in_room) {
	if((rch==ch) || (!CAN_SEE(ch,rch)))
	  continue;
	sprintf (buf,"\t\t\t  - %s\r\n",GET_NAME(rch));
	strcat(buf2,buf);
	}

      if(*buf != '\0') {
	none = FALSE;
	send_to_char("Right here you see: \r\n",ch);
	send_to_char(buf2,ch);
	}
      }

    for(door=0; door<NUM_OF_DIRS; door++) {
      pexit = EXIT(ch,door);
      for(sqcount=1; sqcount<dist+1; sqcount++) {
	if(pexit && pexit->to_room != NOWHERE && !IS_SET(pexit->exit_info,EX_CLOSED) )  {
	  if(world[pexit->to_room].people == NULL)
	    continue;

	*buf2 = '\0';

	  for(rch=world[pexit->to_room].people; rch!=NULL; rch=rch->next_in_room) {
	    if((rch==ch) || (!CAN_SEE(ch,rch)))
	      continue;
	    sprintf (buf,"\t\t\t  - %s\r\n",GET_NAME(rch));
	    strcat (buf2,buf);
	  }

	  if(*buf2 != '\0') {
	    none = FALSE;
	    switch (sqcount) {
	      case 1: sprintf(buf, "Just %s of here you see:\r\n",dirs[door]);
		      break;
	      case 2: sprintf(buf, "Nearby %s of here you see:\r\n",dirs[door]);
		      break;
	      case 3: sprintf(buf, "In the distance %s of here you see:\r\n",dirs[door]);
		      break;
	      case 4: sprintf(buf, "Very far %s of here:\r\n",dirs[door]);
		      break;
	      default: sprintf(buf,"Somewhere far, far, %s of here you see:\r\n",dirs[door]);
		       break;
	      }

	    send_to_char(buf,ch);
	    send_to_char(buf2,ch);
	  }

	  pexit = world[pexit->to_room].dir_option[door];
	}
	else {
	  break;
	}
      }
    }

    if (none == TRUE)
      send_to_char ("You can't see anyone.\r\n",ch);

    act("$n scans $s surroundings.",FALSE,ch,0,0,TO_ROOM);
  } else send_to_char("Your not sure how to do that.\r\n",ch);
}

ACMD(do_throw)
{
  struct char_data *vict = NULL, *tmp_char, *tch;
  struct obj_data *weap;
  char target[MAX_INPUT_LENGTH], weapon[MAX_INPUT_LENGTH];
  char direction[MAX_INPUT_LENGTH];
  int dir_num, found, calc_thaco, dam, diceroll, prob, percent;
  extern char *dirs[];
  extern int rev_dir[];
  extern struct str_app_type str_app[];
  extern int pk_allowed;
  extern int thaco(struct char_data *ch);
  void check_killer(struct char_data *ch, struct char_data *vict);

  half_chop(argument, weapon, buf);

  if (!has_stats_for_skill(ch, SKILL_THROW))
        return;

  if (!generic_find(weapon, FIND_OBJ_INV, ch, &tmp_char, &weap)) {
	send_to_char("Throw what? At whom? In which direction?\r\n", ch);
	return;	}
  two_arguments(buf, direction, target);
  dir_num = search_block(direction, dirs, FALSE);
  if (dir_num < 0)	{
	send_to_char("Throw in which direction?\r\n", ch);
	return;	}
  if (GET_OBJ_WEIGHT(weap) > str_app[GET_AFF_STR(ch)].wield_w/2) {
	send_to_char("It's too heavy to throw!\r\n", ch);
	return;	}
  found = 0;
  if (!CAN_GO(ch, dir_num))	{
    send_to_char("You can't throw things through walls!\r\n", ch);
    return;	}

if(ROOM_FLAGGED(world[ch->in_room].dir_option[dir_num]->to_room,ROOM_HOUSE))
{
    send_to_char("Go egg ya own house.\r\n",ch);
    return; }

  for (tch =
world[world[ch->in_room].dir_option[dir_num]->to_room].people;
	tch != NULL; tch = tch->next_in_room)	{
    if (isname(target, (tch)->player.name) && CAN_SEE(ch, tch)) {
	found = 1;
	vict = tch;	}
    }
  obj_from_char(weap);
  if (found == 0)	{
    sprintf(buf, "$n throws $p %s.", dirs[dir_num]);
    act(buf, FALSE, ch, weap, 0, TO_ROOM);
    sprintf(buf, "You throw $p %s.", dirs[dir_num]);
    act(buf, FALSE, ch, weap, 0, TO_CHAR);
    sprintf(buf, "%s is thrown in from the %s exit.\r\n",
	weap->short_description, dirs[rev_dir[dir_num]]);
    send_to_room(buf, world[ch->in_room].dir_option[dir_num]->to_room);
    obj_to_room(weap, world[ch->in_room].dir_option[dir_num]->to_room);
    return; }
  sprintf(buf, "You throw $p %s at $N.", dirs[dir_num]);
  act(buf, FALSE, ch, weap, vict, TO_CHAR);
  sprintf(buf, "$n throws $p %s.", dirs[dir_num]);
  act(buf, FALSE, ch, weap, vict, TO_ROOM);
  sprintf(buf, "$p is thrown in from the %s exit.",
dirs[rev_dir[dir_num]]);
  act(buf, FALSE, vict, weap, 0, TO_ROOM);

  percent = number(1, 101);
  prob = GET_SKILL(ch, SKILL_THROW);

  calc_thaco = thaco(ch);
  calc_thaco -= GET_AFF_DEX(ch)/5;

  dam = GET_OBJ_WEIGHT(weap) + str_app[GET_AFF_STR(ch)].todam/2;
  if (GET_OBJ_TYPE(weap) == ITEM_WEAPON)
	dam += dice(GET_OBJ_VAL(weap, 1), GET_OBJ_VAL(weap, 2))/4;
  diceroll = number(1, 20);
  if (((calc_thaco - diceroll) > GET_AC(vict)) || (diceroll == 1)
	|| (GET_LEVEL(ch) < dam) || percent > prob) {
    send_to_char("You missed!\r\n", ch);
    obj_to_room(weap, vict->in_room);
    act("$n threw $p at you and missed.", FALSE, ch, weap, vict, TO_VICT);
    return;
    }
  act("You strike $N with $p.", FALSE, ch, weap, vict, TO_CHAR);
  act("$n is struck by $p.", FALSE, vict, weap, 0, TO_ROOM);

  if (IS_AFFECTED(vict, AFF_SANCTUARY))
	dam >>= 1;
  if (!pk_allowed || 
(!IS_SET(zone_table[world[ch->in_room].zone].zflag, ZN_PK_ALLOWED)) || 
(!IS_SET(zone_table[world[vict->in_room].zone].zflag, ZN_PK_ALLOWED) && 
!IS_NPC(vict))) {
    check_killer(ch, vict);
    if (PLR_FLAGGED(ch, PLR_KILLER) && (ch != vict))
      dam = 0;
  }
  sprintf(buf, "$n threw $p at you from the %s and hit for %d damage.",
		dirs[rev_dir[dir_num]], dam);
  act(buf, FALSE, ch, weap, vict, TO_VICT);
  GET_HIT(vict) -= dam;
  obj_to_char(weap, vict);
  if (MOB_FLAGGED(vict, MOB_MEMORY) && !IS_NPC(ch))
	remember(vict, ch);
  update_pos(vict);
  if (GET_POS(vict) == POS_DEAD)
	die(vict,ch);
}



