/* ************************************************************************
*   File: act.other.c                                   Part of CircleMUD *
*  Usage: Miscellaneous player-level commands                             *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#define __ACT_OTHER_C__

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "screen.h"
#include "house.h"
#include "constants.h"

/* extern variables */
extern const char *pc_class_types[];
extern const float class_modifiers[NUM_CLASSES];
extern struct room_data *world;
extern struct descriptor_data *descriptor_list;
extern struct spell_info_type spell_info[];
extern struct index_data *mob_index;
extern struct index_data *obj_index; 
extern char *class_abbrevs[];
extern int free_rent;
extern int pt_allowed;
extern int max_filesize;
extern int nameserver_is_slow;
extern int auto_save;
extern int track_through_doors;

/* extern procedures */
void list_skills(struct char_data * ch);
void appear(struct char_data * ch);
void write_aliases(struct char_data *ch);
void perform_immort_vis(struct char_data *ch);
SPECIAL(shop_keeper);
ACMD(do_gen_comm);
void die(struct char_data * ch);
void Crash_rentsave(struct char_data * ch, int cost);

void set_race_specials(struct char_data *ch);
void set_class_specials(struct char_data *ch);
/* local functions */
ACMD(do_memorise);
ACMD(do_disguise);
ACMD(do_pagewidth);
ACMD(do_pagelength);
ACMD(do_remort);
ACMD(do_quit);
ACMD(do_save);
ACMD(do_not_here);
ACMD(do_sneak);
ACMD(do_hide);
ACMD(do_steal);
ACMD(do_practice);
ACMD(do_visible);
ACMD(do_title);
int perform_group(struct char_data *ch, struct char_data *vict);
void print_group(struct char_data *ch);
ACMD(do_group);
ACMD(do_ungroup);
ACMD(do_report);
ACMD(do_split);
ACMD(do_use);
ACMD(do_wimpy);
ACMD(do_display);
ACMD(do_gen_write);
ACMD(do_gen_tog);

/* Charm function for SPECIAL_CHARMER flagged pc's */
void charm_room(struct char_data * ch)
{
   int room = ch->in_room,counter=0;
   struct char_data *vict;
   struct affected_type af;

   if (room == NOWHERE)
	return;

   for(vict = world[room].people; vict; vict = vict->next_in_room)
   {
	if (vict == ch || !IS_NPC(vict) )
		continue;

        if ( vict->in_room != room )
		break;		// Exceeded room, shouldnt' need this, but 
				// i refuse to let it do over 3k loops for no reason
	if (GET_CLASS(vict) == CLASS_DEMIHUMAN && !AFF_FLAGGED(vict, AFF_CHARM) &&
		!MOB_FLAGGED(vict, MOB_NOCHARM)) {
		// Make sure they're not too powerful
		if (GET_LEVEL(vict) > (GET_LEVEL(ch) * 1.10))
			continue;

		if( number(0, 25 - GET_CHA(ch)) == 0)
		{
		   if (vict->master)
			stop_follower(vict);
		   add_follower(vict, ch);
		   af.type = SPELL_CHARM;
		   
		   af.duration = GET_CHA(ch) * 2;
		   af.modifier = 0; 
		   af.location = 0;
		   af.bitvector = AFF_CHARM;
		   affect_to_char(vict, &af);

		   act("$n's charm gets the better of you.", FALSE, ch, 0, vict, TO_VICT);
		   act("Your natural charms influence $N to join your cause.\r\n", FALSE, ch, 0, vict, TO_CHAR);  		   
		   if (IS_NPC(vict))
		   {
	              REMOVE_BIT(MOB_FLAGS(vict), MOB_AGGRESSIVE);
		      REMOVE_BIT(MOB_FLAGS(vict), MOB_SPEC);
		   }
		}
	}
   }
}

ACMD(do_memorise) 
{ 
	struct char_data *tch = NULL;
	struct obj_data *obj;
	if (IS_NPC(ch) )
		return;

	if ( !IS_SET(GET_SPECIALS(ch), SPECIAL_DISGUISE) ) {
		send_to_char("You have no real talent for this.\r\n", ch);
		return;
	}

	one_argument(argument, arg);

	if( !*arg ) {
		send_to_char("Whose features would you like to memorize?\r\n", ch);
		return;
	}

	generic_find(arg, FIND_CHAR_ROOM, ch, &tch, &obj);		

	if (tch == NULL) {
		send_to_char("Noone here by that name.\r\n", ch);
		return; 		
	}

	if (!IS_NPC(tch) || (GET_LEVEL(tch) >= ( GET_LEVEL(ch) * 1.10)) ) {
		send_to_char("Their features are too detailed for you to capture.\r\n", ch);
		return;
	}

	// Do a check here for particular types of mobs. Dragon's can't be mem'ed, etc

	CHAR_MEMORISED(ch) = GET_MOB_VNUM(tch);
	sprintf(buf, "You commit %s's features to memory.\r\n", GET_NAME(tch));
	send_to_char(buf, ch);
}

ACMD(do_disguise) 
{ 
	struct char_data *tch;

	if( IS_NPC(ch))	
		return;

	if (!IS_SET(GET_SPECIALS(ch), SPECIAL_DISGUISE) ) {
		send_to_char("You have no idea how!\r\n", ch );
		return;
	}

	if (!CHAR_MEMORISED(ch)) {
		send_to_char("You have noone memorised!\r\n", ch);
		return;
	}
	
	half_chop(argument, arg, buf1);

	tch = read_mobile(CHAR_MEMORISED(ch), VIRTUAL);
	char_to_room(tch, 0);

	if (strcmp(arg, "on") == 0) { 
		CHAR_DISGUISED(ch) = CHAR_MEMORISED(ch);
		sprintf(buf, "You now appear as %s.\r\n", GET_NAME(tch));
		send_to_char(buf, ch);
	}
	else if (strcmp(arg, "off") == 0 ) {
		if (CHAR_DISGUISED(ch) == 0) {
		   send_to_char("You're not disguised at the moment.\r\n", ch);
		}
		else {
		  CHAR_DISGUISED(ch) = 0;
		  send_to_char("You now appear as yourself.\r\n", ch);
		}
	}
	else {
		sprintf(buf, "Disguise &gon&n or &goff&n! (Currently %s)\r\n", 
			(CHAR_DISGUISED(ch) == 0 ? "off": "on"));
		send_to_char(buf, ch);
	}
	if (tch) 	
		extract_char(tch);
}

void apply_specials(struct char_data *ch, bool initial) {

	struct affected_type af, *afptr;

	// Remove all ability affects from the character
	for( afptr = ch->affected; afptr; afptr = afptr->next)
		if (afptr->duration == CLASS_ABILITY)
		  affect_from_char(ch, afptr->type); 

	// Reapply class abilities that require affs
	if (IS_SET(GET_SPECIALS(ch), SPECIAL_INVIS) )
	{
	  af.type = SPELL_INVISIBLE;
	  af.duration = CLASS_ABILITY;
	  af.modifier = -40;
	  af.location = APPLY_AC;
	  af.bitvector = AFF_INVISIBLE;
	  affect_to_char(ch, &af);
	  if( initial ) 
	    send_to_char("&WYou merge your skills and vanish.&n\r\n", ch);	
	}
	if (IS_SET(GET_SPECIALS(ch), SPECIAL_SNEAK) )
	{
	  af.duration = CLASS_ABILITY;
	  af.modifier = 0;
	  af.location = APPLY_NONE;
	  af.bitvector = AFF_SNEAK;
	  af.type = SKILL_SNEAK;
	  affect_to_char(ch, &af);
	  if( initial )
	    send_to_char("&WYour ability to move quietly becomes second nature.&n\r\n",ch);
	}	
	if (IS_SET(GET_SPECIALS(ch), SPECIAL_FLY))
	{
	  af.duration = CLASS_ABILITY;
	  af.modifier = 0;
	  af.location = APPLY_NONE;
	  af.bitvector = AFF_FLY;
	  af.type = SPELL_FLY;
	  affect_to_char(ch, &af);
	  if( initial )
	    send_to_char("&WYou can now miss the ground when falling.&n\r\n",ch);
	}
	if (IS_SET(GET_SPECIALS(ch), SPECIAL_INFRA))
	{
	  af.duration = CLASS_ABILITY;
	  af.modifier = 0;
	  af.location = APPLY_NONE;
	  af.bitvector = AFF_INFRAVISION;
	  af.type = SPELL_INFRAVISION;
	  affect_to_char(ch, &af);
	  if( initial )
	    send_to_char("&WYour eyes take on a permanant red glow.&n\r\n",ch);
	}

	// Set their str and con to 21 if they're supermen
	if (IS_SET(GET_SPECIALS(ch), SPECIAL_SUPERMAN)) {
		if (GET_STR(ch) < 21)
			GET_STR(ch) = 21;
		if (GET_CON(ch) < 21)
			GET_STR(ch) = 21;
	}
	
}

void set_race_specials(struct char_data *ch) {

	switch(GET_RACE(ch)) {
		case RACE_OGRE:
		  SET_BIT(GET_SPECIALS(ch), SPECIAL_SUPERMAN);
		  break;
		case RACE_CHANGLING:
		  SET_BIT(GET_SPECIALS(ch), SPECIAL_DISGUISE);
		  break;
		case RACE_DWARF:
		  SET_BIT(GET_SPECIALS(ch), SPECIAL_DWARF);
		  SET_BIT(GET_SPECIALS(ch), SPECIAL_ESCAPE);
		  SET_BIT(GET_SPECIALS(ch), SPECIAL_INFRA);
		  break;
		case RACE_ELF:
		  SET_BIT(GET_SPECIALS(ch), SPECIAL_ELF);
		  SET_BIT(GET_SPECIALS(ch), SPECIAL_INFRA);
		  SET_BIT(GET_SPECIALS(ch), SPECIAL_GROUP_SNEAK);
		  break;
		case RACE_KENDA:
		  SET_BIT(GET_SPECIALS(ch), SPECIAL_SNEAK);
		  SET_BIT(GET_SPECIALS(ch), SPECIAL_THIEF);
		  break;
		case RACE_MINOTAUR:
		  SET_BIT(GET_SPECIALS(ch), SPECIAL_GORE);
		  SET_BIT(GET_SPECIALS(ch), SPECIAL_MINOTAUR);
		  break;
		case RACE_ORC:
		  SET_BIT(GET_SPECIALS(ch), SPECIAL_CHARMER);
		  SET_BIT(GET_SPECIALS(ch), SPECIAL_INFRA);
		  break;
		case RACE_PIXIE:
		  SET_BIT(GET_SPECIALS(ch), SPECIAL_FLY);
		  SET_BIT(GET_SPECIALS(ch), SPECIAL_FOREST_HELP);
 		  break;
		case RACE_DEVA:
		case RACE_AVATAR: break;
		default: log("Unknown race type for %s (%d).",GET_NAME(ch), GET_RACE(ch));
			break;
	}
}

void set_class_specials(struct char_data *ch) {
	
	switch(GET_CLASS(ch)) {
		case CLASS_WARRIOR:
		case CLASS_CLERIC:
		case CLASS_THIEF:
		case CLASS_MAGIC_USER: break;
		case CLASS_DRUID:
			SET_BIT(GET_SPECIALS(ch), SPECIAL_FOREST_SPELLS);
			SET_BIT(GET_SPECIALS(ch), SPECIAL_FOREST_HELP);
			break;
		case CLASS_PRIEST:
			SET_BIT(GET_SPECIALS(ch), SPECIAL_HEALER);
			SET_BIT(GET_SPECIALS(ch), SPECIAL_PRIEST);
			break;
		case CLASS_NIGHTBLADE:
			SET_BIT(GET_SPECIALS(ch), SPECIAL_BACKSTAB);
			SET_BIT(GET_SPECIALS(ch), SPECIAL_SNEAK);
			break;
		case CLASS_BATTLEMAGE:
			SET_BIT(GET_SPECIALS(ch), SPECIAL_MULTIWEAPON);
			SET_BIT(GET_SPECIALS(ch), SPECIAL_BATTLEMAGE);
			break;
		case CLASS_SPELLSWORD:
			SET_BIT(GET_SPECIALS(ch), SPECIAL_MANA_THIEF);
			SET_BIT(GET_SPECIALS(ch), SPECIAL_INVIS);
			SET_BIT(GET_SPECIALS(ch), SPECIAL_SNEAK);
			break;
		case CLASS_PALADIN:
			SET_BIT(GET_SPECIALS(ch), SPECIAL_HEALER);
			SET_BIT(GET_SPECIALS(ch), SPECIAL_HOLY);
			break;
		case CLASS_MASTER:
			SET_BIT(GET_SPECIALS(ch), SPECIAL_SNEAK);
			SET_BIT(GET_SPECIALS(ch), SPECIAL_MULTIWEAPON);
			SET_BIT(GET_SPECIALS(ch), SPECIAL_HEALER);
			break;
		default: 
			sprintf(buf, "SYSERR: %s has unknown class type '%d'.", GET_NAME(ch),GET_CLASS(ch));
			mudlog(buf, BRF, LVL_GOD, TRUE);
			break;
	}

}

void remove_class_specials(struct char_data *ch)
{
	switch(GET_CLASS(ch)) {
		case CLASS_WARRIOR:
		case CLASS_CLERIC:
		case CLASS_THIEF:
		case CLASS_MAGIC_USER: break;
		case CLASS_DRUID:
			REMOVE_BIT(GET_SPECIALS(ch), SPECIAL_FOREST_SPELLS);
			REMOVE_BIT(GET_SPECIALS(ch), SPECIAL_FOREST_HELP);
			break;
		case CLASS_PRIEST:
			REMOVE_BIT(GET_SPECIALS(ch), SPECIAL_HEALER);
			REMOVE_BIT(GET_SPECIALS(ch), SPECIAL_PRIEST);
			break;
		case CLASS_NIGHTBLADE:
			REMOVE_BIT(GET_SPECIALS(ch), SPECIAL_BACKSTAB);
			REMOVE_BIT(GET_SPECIALS(ch), SPECIAL_SNEAK);
			break;
		case CLASS_BATTLEMAGE:
			REMOVE_BIT(GET_SPECIALS(ch), SPECIAL_MULTIWEAPON);
			REMOVE_BIT(GET_SPECIALS(ch), SPECIAL_BATTLEMAGE);
			break;
		case CLASS_SPELLSWORD:
			REMOVE_BIT(GET_SPECIALS(ch), SPECIAL_MANA_THIEF);
			REMOVE_BIT(GET_SPECIALS(ch), SPECIAL_INVIS);
			REMOVE_BIT(GET_SPECIALS(ch), SPECIAL_SNEAK);
			break;
		case CLASS_PALADIN:
			REMOVE_BIT(GET_SPECIALS(ch), SPECIAL_HEALER);
			REMOVE_BIT(GET_SPECIALS(ch), SPECIAL_HOLY);
			break;
		case CLASS_MASTER:
			REMOVE_BIT(GET_SPECIALS(ch), SPECIAL_SNEAK);
			REMOVE_BIT(GET_SPECIALS(ch), SPECIAL_MULTIWEAPON);
			REMOVE_BIT(GET_SPECIALS(ch), SPECIAL_HEALER);
			break;
		default: 
			sprintf(buf, "SYSERR: %s has unknown class type '%d'.", GET_NAME(ch),GET_CLASS(ch));
			mudlog(buf, BRF, LVL_GOD, TRUE);
			break;
	}

}

void remort(struct char_data *ch, int rclass) {

	int pclass = GET_CLASS(ch);
	const char *remort_msg[NUM_CLASSES] = {
		"\n", 	// Mage
		"\n",   // Warrior
 		"\n",   // Cleric
		"\n",   // Thief
		"You combine your abilities into the most magical mix available.\r\nAs a &gDruid&n you hold all arcane powers at your disposal.\r\n",
		"A &yPriest&n combines your abilities into a profitable mix which relies on\r\ncunning and personal strength to survive and prosper.\r\n",
		"You combine your abilities into a deadly mix of cunning and strength,\r\nbecoming a &RNightblade&n.\r\n",
		"Focusing your abilities into offensive skills, as a &gBattlemage&n\r\nyou are widely reknown for your fighting prowess and unstoppable\r\nstrength.\r\n",
		"Combining your abilities for profit, you begin your new life as a\r\n&rSpellSword&n.\r\n",
		"Combining your abilities you become a &gPaladin&n, a holy warrior\r\nof good and pure heart.\r\n", 
		"You take the final step into personal perfection, and rejoin the\r\nmortal realm as a &YMaster&n of all trades. Your abilities and potential\r\nare staggering, and should you achieve Immortality once again, you\r\nshall stand tall with the most powerful in the realm.\r\n" 
	};
	

	/* Perform class adaption checking, ie, thieves cannot become druids */
	switch(pclass) {
	   case CLASS_MAGIC_USER:
		if( rclass != CLASS_SPELLSWORD && rclass != CLASS_BATTLEMAGE &&
		    rclass != CLASS_DRUID ) {
	          send_to_char("Mages cannot specialise there.\r\n", ch);
		  return;
		}
		break;
	   case CLASS_WARRIOR:
		if( rclass != CLASS_PALADIN && rclass != CLASS_BATTLEMAGE &&
		    rclass != CLASS_NIGHTBLADE ) {
	          send_to_char("Warriors cannot specialise there.\r\n", ch);
		  return;
		}
		break;
 	   case CLASS_CLERIC:
		if( rclass != CLASS_PRIEST && rclass != CLASS_PALADIN &&
		    rclass != CLASS_DRUID ) {
	          send_to_char("Clerics cannot specialise there.\r\n", ch);
		  return;
		}
		break;
	   case CLASS_THIEF:
		if( rclass != CLASS_NIGHTBLADE && rclass != CLASS_SPELLSWORD &&
		    rclass != CLASS_PRIEST ) {
	          send_to_char("Thieves cannot specialise there.\r\n", ch);
		  return;
		}
		break;
	  case CLASS_NIGHTBLADE:
	  case CLASS_SPELLSWORD:
	  case CLASS_DRUID:
	  case CLASS_PRIEST:
	  case CLASS_PALADIN:
	  case CLASS_BATTLEMAGE:	break;

	  default:
		sprintf(buf, "Remort Error: %s attempting to remort into class #%d.", 
			GET_NAME(ch), rclass);
		mudlog(buf, NRM, LVL_GOD, TRUE);
		return;
	}

	/* Do checking here for special locations, items, alignment etc */

	/* Apply changing conditions here (Maxmana, etc) */

	/* Set player's level here */
	GET_CLASS(ch) = rclass;
	GET_LEVEL(ch) = 1;
	GET_MODIFIER(ch) = class_modifiers[rclass] + special_modifier(ch);

//        apply_class_specials(ch, TRUE);
	
	set_class_specials(ch);	
	apply_specials(ch, TRUE);	

	sprintf(buf, "%s rejoins the mortal realm as a %s.", GET_NAME(ch),
		pc_class_types[rclass]);
	mudlog(buf, BRF, LVL_IMMORT, TRUE); 

	send_to_char(remort_msg[rclass], ch);
}

/* Remort command for champs */
ACMD(do_remort) {
 
        if( GET_CLASS(ch) == CLASS_MASTER ) {
                send_to_char("You have mastered all classes!\r\n", ch);
                return;
        }
 
        if( GET_LEVEL(ch) < LVL_IMMORT ) {
                send_to_char("You cannot remort until you are an immortal!\r\n", ch);
                return;
        }
 
        one_argument(argument, arg);
 
        if( !*arg ) {
                send_to_char("You must specify what class you would like to evolve into.\r\n", ch);
                return;
        }
 
        if( strcmp(arg, "nightblade") == 0 ) {
          if( GET_CLASS(ch) > CLASS_WARRIOR ) {
             send_to_char("You are too specialised to become a Nightblade.\r\n", ch);
             return;
          }
          remort(ch, CLASS_NIGHTBLADE);
          return;
        }
 
        if( strcmp(arg, "battlemage") == 0 ) {
          if( GET_CLASS(ch) > CLASS_WARRIOR ) {
             send_to_char("You are too specialised to become a Battlemage.\r\n", ch);
             return;
          }
          remort(ch, CLASS_BATTLEMAGE);
          return;
        }
 
        if( strcmp(arg, "spellsword") == 0 ) {
          if( GET_CLASS(ch) > CLASS_WARRIOR ) {
             send_to_char("You are too specialised to become a Spellsword.\r\n", ch);
             return;
          }
          remort(ch, CLASS_SPELLSWORD);
          return;
        }
 
        if( strcmp(arg, "paladin") == 0 ) {
          if( GET_CLASS(ch) > CLASS_WARRIOR ) {
             send_to_char("You are too specialised to become a Paladin.\r\n", ch);
             return;
          }
          remort(ch, CLASS_PALADIN);
          return;
        }
 
        if( strcmp(arg, "druid") == 0 ) {
          if( GET_CLASS(ch) > CLASS_WARRIOR ) {
             send_to_char("You are too specialised to become a Druid.\r\n", ch);
             return;
          }
          remort(ch, CLASS_DRUID);
          return;
        }
 
        if( strcmp(arg, "priest") == 0 ) {
          if( GET_CLASS(ch) > CLASS_WARRIOR ) {
             send_to_char("You are too specialised to become a Priest.\r\n", ch);
             return;
          }
          remort(ch, CLASS_PRIEST);
          return;
        }
 
        if( strcmp(arg, "master") == 0 ) {
            if( GET_CLASS(ch) <= CLASS_WARRIOR ) {
                send_to_char("You are not learned enough to become a master, yet.\r\n",ch);
                return;
            }
            remort(ch, CLASS_MASTER);
            return;
        }
 
        send_to_char("No such class exists!\r\n", ch);
}                                                                                           


ACMD(do_quit)
{
  struct descriptor_data *d, *next_d;
  extern int r_mortal_start_room;

  if (IS_NPC(ch) || !ch->desc)
    return;

  if (subcmd != SCMD_QUIT && GET_LEVEL(ch) < LVL_IMMORT)
    send_to_char("You have to type quit--no less, to quit!\r\n", ch);
  else if (GET_POS(ch) == POS_FIGHTING)
    send_to_char(CHARFIGHTING, ch);
  else if (GET_POS(ch) < POS_STUNNED) {
    send_to_char("You die before your time...\r\n", ch);
    die(ch);
  } else {
    int loadroom = ch->in_room;

    if (!GET_INVIS_LEV(ch))
      act("$n has left the game.", TRUE, ch, 0, 0, TO_ROOM);
    sprintf(buf, "&G%s &ghas quit the game.", GET_NAME(ch));
    mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
    info_channel(buf, ch);
    send_to_char("Goodbye, friend.. Come back soon!\r\n", ch);

    /*
     * kill off all sockets connected to the same player as the one who is
     * trying to quit.  Helps to maintain sanity as well as prevent duping.
     */
    for (d = descriptor_list; d; d = next_d) {
      next_d = d->next;
      if (d == ch->desc)
        continue;
      if (d->character && (GET_IDNUM(d->character) == GET_IDNUM(ch)))
        STATE(d) = CON_DISCONNECT;
    }

    if (free_rent)
      Crash_rentsave(ch, 0);
    loadroom = ch->in_room;

    extract_char(ch);		/* Char is saved in extract char */

    // DM - reset save_room if it is same as mortal start room
    if (loadroom == world[real_room(r_mortal_start_room)].number)
      save_char(ch, loadroom);
      // ENTRY_ROOM(ch,1) = world[loadroom].number;

    /* If someone is quitting in their house, let them load back here */
    if (ROOM_FLAGGED(loadroom, ROOM_HOUSE))
      save_char(ch, loadroom);
  }
}



ACMD(do_save)
{
  if (IS_NPC(ch) || !ch->desc)
    return;

  /* Only tell the char we're saving if they actually typed "save" */
  if (cmd) {
    /*
     * This prevents item duplication by two PC's using coordinated saves
     * (or one PC with a house) and system crashes. Note that houses are
     * still automatically saved without this enabled. This code assumes
     * that guest immortals aren't trustworthy. If you've disabled guest
     * immortal advances from mortality, you may want < instead of <=.
     */
    if (auto_save && GET_LEVEL(ch) <= LVL_IMMORT) {
      send_to_char("Saving aliases.\r\n", ch);
      write_aliases(ch);
      return;
    }
    sprintf(buf, "Saving %s and aliases.\r\n", GET_NAME(ch));
    send_to_char(buf, ch);
  }

  write_aliases(ch);
  save_char(ch, NOWHERE);
  Crash_crashsave(ch);
  if (ROOM_FLAGGED(ch->in_room, ROOM_HOUSE_CRASH))
    House_crashsave(GET_ROOM_VNUM(IN_ROOM(ch)));
}


/* generic function for commands which are normally overridden by
   special procedures - i.e., shop commands, mail commands, etc. */
ACMD(do_not_here)
{
  send_to_char("Sorry, but you cannot do that here!\r\n", ch);
}

/* compare by Misty */
ACMD(do_compare)
{
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  int count1, count2, i, align1, align2;
  float dam1, dam2;
  byte prob, percent;
  struct obj_data *obj1, *obj2;
  struct char_data *temp;
 
  /* functions used for alignment */
  int get_bit(struct obj_data *obj)
  {
    int i = 0;
 
    if(IS_OBJ_STAT(obj,ITEM_ANTI_GOOD)) i += 1;
    if(IS_OBJ_STAT(obj,ITEM_ANTI_NEUTRAL)) i += 2;
    if(IS_OBJ_STAT(obj,ITEM_ANTI_EVIL)) i += 4;
    return i;
  }
 
  /* print alignment messages */
  void print_it(int bit, struct obj_data *obj)
  {
    switch(bit) {
      case 0: /* no alignment restrictions */
        sprintf(buf,"%s can be used by any Tom, Dick or Harry.\r\n",obj->short_description);
        break;
      case 1: /* neutral or evil item */
        sprintf(buf,"%s can be used by neutral or evil players.\r\n",obj->short_description);
        break;      
      case 2: /* good or evil item */
        sprintf(buf,"%s can be used by good or evil players.\r\n",obj->short_description);
        break;
      case 3: /* evil item */
        sprintf(buf,"%s can only be used by evil players.\r\n",obj->short_description);
        break;
      case 4: /* neutral or good item */
        sprintf(buf,"%s can be used by good and neutral players.\r\n",obj->short_description);
        break;
      case 5: /* neutral */
        sprintf(buf,"%s can only be used by neutral players.\r\n",obj->short_description);
        break;
      case 6: /* good */
        sprintf(buf,"%s can only be used by good players.\r\n",obj->short_description);
        break;
      default:
        sprintf(buf,"%s is obviously fucked.\r\n",obj->short_description);
        break;
      }
    CAP(buf);
    send_to_char(buf,ch);
  }
  /* end of functions */
 
  two_arguments(argument, arg1, arg2);
 
  if (!has_stats_for_skill(ch, SKILL_COMPARE))
      return;

  /* Check if 2 objects were specified */
  if (!*arg1 || !*arg2)
  {
      send_to_char("Compare what with what?\r\n", ch);
      return;
  }
 
  /* Find object 1 */
  generic_find(arg1, FIND_OBJ_INV | FIND_OBJ_EQUIP, ch, &temp, &obj1);
  if (!obj1)
  {
    sprintf(buf, "You don't seem to have a %s.\r\n",arg1);
    send_to_char(buf, ch);
    return;
  }
 
  /* Find object 2 */
  generic_find(arg2, FIND_OBJ_INV | FIND_OBJ_EQUIP, ch, &temp, &obj2);
  if (!obj2)
  {
    sprintf(buf, "You don't seem to have a %s.\r\n",arg2);
    send_to_char(buf, ch);
    return;
  }
 
  /* Make sure objects are of type ARMOR or WEAPON */
  if ((GET_OBJ_TYPE(obj1) != ITEM_WEAPON &&
       GET_OBJ_TYPE(obj1) != ITEM_ARMOR) ||
       GET_OBJ_TYPE(obj1) != GET_OBJ_TYPE(obj2))
  {
    sprintf(buf,"You can't compare %s to %s.\r\n",arg1,arg2);
    send_to_char(buf,ch);
    return;
  } 

  percent = number(1, 101);   /* 101% is a complete failure */
  prob = GET_SKILL(ch, SKILL_COMPARE);
 
  if (percent > prob)
  {
  /* vader's lame bit */
    if(GET_OBJ_TYPE(obj1) == ITEM_WEAPON) {
      switch(number(0,4)) {
        case 0:
          sprintf(buf,"Have you seen anyone wielding %s lately?? It's out of fashion!\r\n",
                  obj1->short_description);
          break;
        case 1:
          sprintf(buf,"Get with it! No-one kills with %s anymore.\r\n",
                  obj2->short_description);
          break;
        case 2:
          sprintf(buf,"In the right lighting, %s could really suit you!\r\n",
                  obj1->short_description);
          break;
        case 3:
          sprintf(buf,"I'm sure you'll have many fun hours of slaying newbies with %s.\r\n",
                  obj2->short_description);
          break;
        case 4:
          sprintf(buf,"If you this %s looks good now, wait till its stained with the blood of your enemies!!\r\n",
                  obj1->short_description);
          break;
        case 5:
          sprintf(buf,"Go with the %s. It makes you look tuff. Grrr.\r\n",
                  obj2->short_description);
          break;
        }
    } else {
      switch(number(0,10)) { 
        case 0:
          sprintf(buf,"Oh please darling, %s is soooo last season!\r\n",
                  obj1->short_description);
          break;
        case 1:
          sprintf(buf,"I wouldn't even wear %s to my step mother's funeral!\r\n",
                  obj2->short_description);
          break;
        case 2:
          sprintf(buf,"Loose a few pounds darling. Then you can consider wearing something like %s.\r\n",
                  obj1->short_description);
          break;
        case 3:
          sprintf(buf,"Yes! The floral pattern on %s looks smashing on you!\r\n",
                  obj2->short_description);
          break;
        case 4:
          sprintf(buf,"Come on darling. If you're going to wear %s with %s then you may aswell get \"FASHION VICTIM\"
tattooed on your forehead!\r\n",
                  obj1->short_description,obj2->short_description);
          break;
        case 5:
          sprintf(buf,"If someone's told you that you look good in %s. They lied.\r\n",
                  obj1->short_description);
          break;
        case 6:
          sprintf(buf,"%s goes so well with %s. You'd be a fool not to wear these together!\r\n",
                  obj1->short_description,obj2->short_description);
          break;
        case 7:
          sprintf(buf,"%s really sets off the purple on those socks you're wearing!\r\n",
                  obj1->short_description);
          break;
        case 8:
          sprintf(buf,"%s will look brilliant when its blood stained!\r\n", 
                  obj2->short_description);
          break;
        case 9:
          sprintf(buf,"%s with %s. The perfect look for the modern mage.\r\n",
                  obj1->short_description,obj2->short_description);
          break;
        case 10:
          sprintf(buf,"If we were living in Darask then maybe you'd pull it off. But here? I don't think so!\r\n");
          break;
        }
      }
    CAP(buf);
    send_to_char(buf, ch);
    return;
  }
 
 
  /* Calculate average damage on weapon type */
  if (GET_OBJ_TYPE(obj1) == ITEM_WEAPON)
  {
    dam1 = (((GET_OBJ_VAL(obj1,2) + 1)/2.0) * GET_OBJ_VAL(obj1,1));
    dam2 = (((GET_OBJ_VAL(obj2,2) + 1)/2.0) * GET_OBJ_VAL(obj2,1));
    /* Compare damages and print */
    if (dam1 < dam2)
      sprintf(buf, "%s does more damage than %s.\r\n",
          obj2->short_description,obj1->short_description);
    else
      if (dam1 > dam2)
            sprintf(buf, "%s does more damage than %s.\r\n",
              obj1->short_description,obj2->short_description);
         else
            sprintf(buf, "%s does the same amount of damage as %s.\r\n",
              obj2->short_description,obj1->short_description);
  }
  else /* object is armor */
    /* compare armor class */
    if (GET_OBJ_VAL(obj1,0) < GET_OBJ_VAL(obj2,0))
      sprintf(buf,"%s looks more protective than %s.\r\n",
          obj1->short_description,obj2->short_description);
    else
      if (GET_OBJ_VAL(obj1,0) > GET_OBJ_VAL(obj2,0))
           sprintf(buf,"%s looks more protective than %s.\r\n",
               obj2->short_description,obj1->short_description);
      else
           sprintf(buf,"%s looks just as protective as %s.\r\n",
               obj2->short_description,obj1->short_description);
 
  CAP(buf);
  send_to_char(buf,ch);
 
  /* Count affections */
  count1 = count2 = 0;
  for (i = 0; i < MAX_OBJ_AFFECT; i++)
  {
    if ((obj1->affected[i].modifier != 0))
      count1++;
    if ((obj2->affected[i].modifier != 0))
      count2++;
  }
 
  /* Compare affections and print */
  if (count1 < count2)
  {
    sprintf(buf,"%s has more affections than %s.\r\n",
         obj2->short_description,obj1->short_description);
    CAP(buf);
    send_to_char(buf,ch);
  }
  else
    if (count1 > count2)
    {
      sprintf(buf,"%s has more affections than %s.\r\n",
          obj1->short_description,obj2->short_description);
      CAP(buf);
      send_to_char(buf,ch);
    }
    else /* equal affections - no affections exist in both */
      if (count1 == 0)
      {
        sprintf(buf, "Neither %s or %s have any affections.\r\n",
            obj1->short_description,obj2->short_description);
        CAP(buf);
        send_to_char(buf,ch);
      }
      else /* equal affections exist */
        {
          sprintf(buf, "%s has the same amount of affections as %s.\r\n",
              obj1->short_description,obj2->short_description);
          CAP(buf);
          send_to_char(buf,ch);
        }
 
 
  /* object alignment */
 
 
  /* get align values */
  align1 = get_bit(obj1);
  align2 = get_bit(obj2);
 
  /* compare the values - if not equal , print */
  if(align1 != align2)
  {
    print_it(align1,obj1);
    print_it(align2,obj2); 
  }
 
} 


ACMD(do_sneak)
{
  struct affected_type af;
  byte percent;

  if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_SNEAK)) {
    send_to_char("You have no idea how to do that.\r\n", ch);
    return;
  }
  send_to_char("Okay, you'll try to move silently for a while.\r\n", ch);
  if (AFF_FLAGGED(ch, AFF_SNEAK))
    affect_from_char(ch, SKILL_SNEAK);

  percent = number(1, 101);	/* 101% is a complete failure */

  if (percent > GET_SKILL(ch, SKILL_SNEAK) + dex_app_skill[GET_DEX(ch)].sneak)
    return;

  af.type = SKILL_SNEAK;
  af.duration = GET_LEVEL(ch);
  af.modifier = 0;
  af.location = APPLY_NONE;
  af.bitvector = AFF_SNEAK;
  affect_to_char(ch, &af);
}



ACMD(do_hide)
{
  byte percent;

  if (!has_stats_for_skill(ch, SKILL_HIDE))
    return;

  if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_HIDE)) {
    send_to_char("You have no idea how to do that.\r\n", ch);
    return;
  }

  if (AFF_FLAGGED(ch, AFF_HIDE))
    REMOVE_BIT(AFF_FLAGS(ch), AFF_HIDE);

  percent = number(1, 101);	/* 101% is a complete failure */

  if (percent > GET_SKILL(ch, SKILL_HIDE) + dex_app_skill[GET_DEX(ch)].hide) {
    send_to_char("You unsuccessfully attempt to hide yourself.\r\n", ch);
    return;
  }

  send_to_char("You hide yourself.\r\n", ch);
  SET_BIT(AFF_FLAGS(ch), AFF_HIDE);
}




ACMD(do_steal)
{
  extern struct zone_data *zone_table;
  struct char_data *vict;
  struct obj_data *obj;
  char vict_name[MAX_INPUT_LENGTH], obj_name[MAX_INPUT_LENGTH];
  int percent, gold, eq_pos, pcsteal = 0, ohoh = 0;

  if (!has_stats_for_skill(ch, SKILL_STEAL))
    return;

  if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_STEAL)) {
    send_to_char("You have no idea how to do that.\r\n", ch);
    return;
  }
  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL)) {
    send_to_char(PEACEROOM, ch);
    return;
  }

  two_arguments(argument, obj_name, vict_name);

  if (!(vict = get_char_vis(ch, vict_name, FIND_CHAR_ROOM))) {
    send_to_char("Steal what from who?\r\n", ch);
    return;
  } else if (vict == ch) {
    send_to_char("Come on now, that's rather stupid!\r\n", ch);
    return;
  }
  if (MOB_FLAGGED(vict,MOB_NO_STEAL)){
     do_gen_comm(vict, "THIEF!!!.  I'll get you for that!!", 0, SCMD_SHOUT);
     hit(vict, ch, TYPE_UNDEFINED);
     return;
  }
  if (!pt_allowed && GET_LEVEL(ch)<LVL_IMPL) {
    if (!IS_NPC(vict) && !PLR_FLAGGED(vict, PLR_THIEF) &&
        !PLR_FLAGGED(vict, PLR_KILLER) && !PLR_FLAGGED(ch, PLR_THIEF)) {
      /*
       * SET_BIT(ch->specials.act, PLR_THIEF); send_to_char("Okay, you're the
       * boss... you're now a THIEF!\r\n",ch); sprintf(buf, "PC Thief bit set
       * on %s", GET_NAME(ch)); log(buf);
       */
      if (GET_POS(vict) == POS_SLEEPING)
      {
        send_to_char("That isn't very sporting now is it!\n\r", ch);
        act("$n tried to steal something from you!", FALSE, ch, 0, vict, TO_VICT);
        GET_POS(vict) = POS_STANDING;
        act("$n rifles through $N's belongings as they sleep!", FALSE, ch, 0, vict, TO_NOTVICT);
        act("$n wakes and stands up", FALSE, vict, 0, 0, TO_ROOM);
        send_to_char("You are awakened by a thief going through your things!\n\r", vict);
        send_to_char("You get a good look at the scoundrel!\n\r\n\r", vict);
        look_at_char(ch, vict);
        return;
      }
 
      pcsteal = 1;
    }
    if (PLR_FLAGGED(ch, PLR_THIEF))
      pcsteal = 1;
  }
  /* 101% is a complete failure */
  percent = number(1, 101) - dex_app_skill[GET_DEX(ch)].p_pocket;

  // Adjust percent if thief is enhanced
  if (IS_SET(GET_SPECIALS(ch), SPECIAL_THIEF)) {
	percent -= percent * 0.15;
	if (percent <= 0) 
	  percent = 1;
  }

  if (GET_POS(vict) < POS_SLEEPING)
    percent = -1;		/* ALWAYS SUCCESS, unless heavy object. */

  if (!pt_allowed && !IS_NPC(vict))
    pcsteal = 1;

  if (!AWAKE(vict))	/* Easier to steal from sleeping people. */
    percent -= 50;

  /* NO NO With Imp's and Shopkeepers, and if player thieving is not allowed */
  if (GET_LEVEL(vict) >= LVL_IS_GOD || pcsteal ||
      GET_MOB_SPEC(vict) == shop_keeper)
    percent = 101;		/* Failure */

  if (!IS_NPC(vict)) /* always fail on another player character */
    percent=101;
  if (GET_LEVEL(ch)>=LVL_IMPL) /* implementors always succeed! /bm/ */
    percent=-100;

  if (str_cmp(obj_name, "coins") && str_cmp(obj_name, "gold")) {

    if (!(obj = get_obj_in_list_vis(ch, obj_name, vict->carrying))) {

      for (eq_pos = 0; eq_pos < NUM_WEARS; eq_pos++)
	if (GET_EQ(vict, eq_pos) &&
	    (isname(obj_name, GET_EQ(vict, eq_pos)->name)) &&
	    CAN_SEE_OBJ(ch, GET_EQ(vict, eq_pos))) {
	  obj = GET_EQ(vict, eq_pos);
	  break;
	}
      if (!obj) {
	act("$E hasn't got that item.", FALSE, ch, 0, vict, TO_CHAR);
	return;
      } else {			/* It is equipment */
	if ((GET_POS(vict) > POS_STUNNED)) {
	  send_to_char("Steal the equipment now?  Impossible!\r\n", ch);
	  return;
	} else {
	  act("You unequip $p and steal it.", FALSE, ch, obj, 0, TO_CHAR);
	  act("$n steals $p from $N.", FALSE, ch, obj, vict, TO_NOTVICT);
	  obj_to_char(unequip_char(vict, eq_pos), ch);
	}
      }
    } else {			/* obj found in inventory */

      percent += GET_OBJ_WEIGHT(obj);	/* Make heavy harder */

      if (percent > GET_SKILL(ch, SKILL_STEAL)) {
	ohoh = TRUE;
	send_to_char("Oops..\r\n", ch);
	act("$n tried to steal something from you!", FALSE, ch, 0, vict, TO_VICT);
	act("$n tries to steal something from $N.", TRUE, ch, 0, vict, TO_NOTVICT);
      } else {			/* Steal the item */
	if (IS_CARRYING_N(ch) + 1 < CAN_CARRY_N(ch)) {
	  if (IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj) < CAN_CARRY_W(ch)) {
	    obj_from_char(obj);
	    obj_to_char(obj, ch);
	    send_to_char("Got it!\r\n", ch);
	  }
	} else
	  send_to_char("You cannot carry that much.\r\n", ch);
      }
    }
  } else {			/* Steal some coins */
    if (AWAKE(vict) && (percent > GET_SKILL(ch, SKILL_STEAL))) {
      ohoh = TRUE;
      send_to_char("Oops..\r\n", ch);
      act("You discover that $n has $s hands in your wallet.", FALSE, ch, 0, vict, TO_VICT);
      act("$n tries to steal gold from $N.", TRUE, ch, 0, vict, TO_NOTVICT);
    } else {
      /* Steal some gold coins */
      if (IS_SET(zone_table[world[ch->in_room].zone].zflag , ZN_NO_STEAL)&&IS_NPC(vict)){
 
        do_gen_comm(vict, "THIEF!!!.  I'll get you for that!!", 0, SCMD_SHOUT);
        hit(vict, ch, TYPE_UNDEFINED);
        return;
      }
      gold = (int) ((GET_GOLD(vict) * number(1, 10)) / 100);
      gold = MIN(1782, gold);
      if (gold > 0) {
	GET_GOLD(ch) += gold;
	GET_GOLD(vict) -= gold;
        if (gold > 1) {
	  sprintf(buf, "Bingo!  You got %d gold coins.\r\n", gold);
	  send_to_char(buf, ch);
	} else {
	  send_to_char("You manage to swipe a solitary gold coin.\r\n", ch);
	}
      } else {
	send_to_char("You couldn't get any gold...\r\n", ch);
      }
    }
  }

  if (ohoh && IS_NPC(vict) && AWAKE(vict))
    hit(vict, ch, TYPE_UNDEFINED);
}



ACMD(do_practice)
{
  if (IS_NPC(ch))
    return;

  one_argument(argument, arg);

  if (*arg)
    send_to_char("You can only practice skills in your guild.\r\n", ch);
  else
    list_skills(ch);
}



ACMD(do_visible)
{
  if (GET_LEVEL(ch) >= LVL_IMMORT) {
    perform_immort_vis(ch);
    return;
  }

  if AFF_FLAGGED(ch, AFF_INVISIBLE) {
    appear(ch);
    send_to_char("You break the spell of invisibility.\r\n", ch);
  } else
    send_to_char("You are already visible.\r\n", ch);
}



ACMD(do_title)
{
  skip_spaces(&argument);
  delete_doubledollar(argument);

  if (IS_NPC(ch))
    send_to_char("Your title is fine... go away.\r\n", ch);
  else if (PLR_FLAGGED(ch, PLR_NOTITLE))
    send_to_char("You can't title yourself -- you shouldn't have abused it!\r\n", ch);
  else if (strstr(argument, "(") || strstr(argument, ")"))
    send_to_char("Titles can't contain the ( or ) characters.\r\n", ch);
  else if (strstr(argument, "[") || strstr(argument, "]"))
    send_to_char("Titles can't contain the [ or ] characters.\r\n", ch);
  else if (scan_buffer_for_xword(argument))
  {
    sprintf(buf,"WATCHLOG SWEAR (Title): %s title : '%s'", ch->player.name, argument);
    mudlog(buf,NRM,LVL_IMPL,TRUE);
    send_to_char("Please dont swear in your title.\r\n",ch);
    return ;
  }   
  else if ((strlen(argument) > MAX_TITLE_LENGTH) || (strlen(argument) > GET_LEVEL(ch))) {
    sprintf(buf, "Sorry, your titles can't be longer than your level in characters, with a max of %d.\r\n", MAX_TITLE_LENGTH);
    send_to_char(buf, ch);
  } else {
    strcat(argument,"&n");
    set_title(ch, argument);
    sprintf(buf, "Okay, you're now %s %s.\r\n", GET_NAME(ch), GET_TITLE(ch));
    send_to_char(buf, ch);
  }
}


int perform_group(struct char_data *ch, struct char_data *vict)
{
  if (AFF_FLAGGED(vict, AFF_GROUP) || !CAN_SEE(ch, vict))
    return (0);

  SET_BIT(AFF_FLAGS(vict), AFF_GROUP);
  if (ch != vict)
    act("$N is now a member of your group.", FALSE, ch, 0, vict, TO_CHAR);
  act("You are now a member of $n's group.", FALSE, ch, 0, vict, TO_VICT);
  act("$N is now a member of $n's group.", FALSE, ch, 0, vict, TO_NOTVICT);
  return (1);
}

void color_perc(char col[], int curr, int max)
{
  if (curr < max / 4)
    strcpy(col,"&r");
  else if (curr < max / 2)
    strcpy(col,"&R");
  else if (curr < max)
    strcpy(col,"&Y");
  else
    strcpy(col,"&G");
}


void print_group(struct char_data *ch)
{
  struct char_data *k;
  struct follow_type *f;
  char hcol[3], mcol[3], vcol[3];

  if (!AFF_FLAGGED(ch, AFF_GROUP))
    send_to_char("But you are not the member of a group!\r\n", ch);
  else {
    send_to_char("Your group consists of:\r\n", ch);

    k = (ch->master ? ch->master : ch);

    color_perc(hcol,GET_HIT(k),GET_MAX_HIT(k));
    color_perc(mcol,GET_MANA(k),GET_MAX_MANA(k));
    color_perc(vcol,GET_MOVE(k),GET_MAX_MOVE(k));

    if (AFF_FLAGGED(k, AFF_GROUP)) {
      sprintf(buf, "     &B[%s%3d&nH %s%3d&nM %s%3d&nV&B] [&n%2d %s&B]&n $N &R(Head of group)&n",
	      hcol, GET_HIT(k), mcol, GET_MANA(k), mcol, GET_MOVE(k), GET_LEVEL(k), CLASS_ABBR(k));
      act(buf, FALSE, ch, 0, k, TO_CHAR);
    }

    for (f = k->followers; f; f = f->next) {
      if (!AFF_FLAGGED(f->follower, AFF_GROUP))
	continue;

      color_perc(hcol,GET_HIT(f->follower),GET_MAX_HIT(f->follower));
      color_perc(mcol,GET_MANA(f->follower),GET_MAX_MANA(f->follower));
      color_perc(vcol,GET_MOVE(f->follower),GET_MAX_MOVE(f->follower));

      sprintf(buf, "     &B[%s%3d&nH %s%3d&nM %s%3d&nV&B] [&n%2d %s&B]&n $N", hcol, GET_HIT(f->follower),
	      mcol, GET_MANA(f->follower), vcol, GET_MOVE(f->follower),
	      GET_LEVEL(f->follower), CLASS_ABBR(f->follower));
      act(buf, FALSE, ch, 0, f->follower, TO_CHAR);
    }
  }
}

ACMD(do_group)
{
  struct char_data *vict;
  struct follow_type *f;
  int found;

  one_argument(argument, buf);

  if (!*buf) {
    print_group(ch);
    return;
  }

  if (ch->master) {
    act("You can not enroll group members without being head of a group.",
	FALSE, ch, 0, 0, TO_CHAR);
    return;
  }

  if (!str_cmp(buf, "all")) {
    perform_group(ch, ch);
    for (found = 0, f = ch->followers; f; f = f->next)
      found += perform_group(ch, f->follower);
    if (!found)
      send_to_char("Everyone following you is already in your group.\r\n", ch);
    return;
  }

  if (!(vict = get_char_vis(ch, buf, FIND_CHAR_ROOM)))
    send_to_char(NOPERSON, ch);
  else if ((vict->master != ch) && (vict != ch))
    act("$N must follow you to enter your group.", FALSE, ch, 0, vict, TO_CHAR);
  else {
    if (!AFF_FLAGGED(vict, AFF_GROUP))
      perform_group(ch, vict);
    else {
      if (ch != vict)
	act("$N is no longer a member of your group.", FALSE, ch, 0, vict, TO_CHAR);
      act("You have been kicked out of $n's group!", FALSE, ch, 0, vict, TO_VICT);
      act("$N has been kicked out of $n's group!", FALSE, ch, 0, vict, TO_NOTVICT);
      REMOVE_BIT(AFF_FLAGS(vict), AFF_GROUP);
    }
  }
}



ACMD(do_ungroup)
{
  struct follow_type *f, *next_fol;
  struct char_data *tch;

  one_argument(argument, buf);

  if (!*buf) {
    if (ch->master || !(AFF_FLAGGED(ch, AFF_GROUP))) {
      send_to_char("But you lead no group!\r\n", ch);
      return;
    }
    sprintf(buf2, "%s has disbanded the group.\r\n", GET_NAME(ch));
    for (f = ch->followers; f; f = next_fol) {
      next_fol = f->next;
      if (AFF_FLAGGED(f->follower, AFF_GROUP)) {
	REMOVE_BIT(AFF_FLAGS(f->follower), AFF_GROUP);
	send_to_char(buf2, f->follower);
        if (!AFF_FLAGGED(f->follower, AFF_CHARM))
	  stop_follower(f->follower);
      }
    }

    REMOVE_BIT(AFF_FLAGS(ch), AFF_GROUP);
    send_to_char("You disband the group.\r\n", ch);
    return;
  }
  if (!(tch = get_char_vis(ch, buf, FIND_CHAR_ROOM))) {
    send_to_char("There is no such person!\r\n", ch);
    return;
  }
  if (tch->master != ch) {
    send_to_char("That person is not following you!\r\n", ch);
    return;
  }

  if (!AFF_FLAGGED(tch, AFF_GROUP)) {
    send_to_char("That person isn't in your group.\r\n", ch);
    return;
  }

  REMOVE_BIT(AFF_FLAGS(tch), AFF_GROUP);

  act("$N is no longer a member of your group.", FALSE, ch, 0, tch, TO_CHAR);
  act("You have been kicked out of $n's group!", FALSE, ch, 0, tch, TO_VICT);
  act("$N has been kicked out of $n's group!", FALSE, ch, 0, tch, TO_NOTVICT);
 
  if (!AFF_FLAGGED(tch, AFF_CHARM))
    stop_follower(tch);
}




ACMD(do_report)
{
  struct char_data *k;
  struct follow_type *f;
  char hcol[5], mcol[5], vcol[6];

  if (!AFF_FLAGGED(ch, AFF_GROUP)) {
    send_to_char("But you are not a member of any group!\r\n", ch);
    return;
  }

  color_perc(hcol, GET_HIT(ch), GET_MAX_HIT(ch));
  color_perc(mcol, GET_MANA(ch), GET_MAX_MANA(ch));
  color_perc(vcol, GET_MOVE(ch), GET_MAX_MOVE(ch));

  sprintf(buf, "%s reports: %s%d&n/&G%d&nH, %s%d&n/&G%d&nM, %s%d&n/&G%d&nV\r\n",
	  GET_NAME(ch), 
	  hcol, GET_HIT(ch), GET_MAX_HIT(ch),
	  mcol, GET_MANA(ch), GET_MAX_MANA(ch),
	  vcol, GET_MOVE(ch), GET_MAX_MOVE(ch));

  CAP(buf);

  k = (ch->master ? ch->master : ch);

  for (f = k->followers; f; f = f->next)
    if (AFF_FLAGGED(f->follower, AFF_GROUP) && f->follower != ch)
      send_to_char(buf, f->follower);
  if (k != ch)
    send_to_char(buf, k);
  send_to_char("You report to the group.\r\n", ch);
}



ACMD(do_split)
{
  int amount, num, share, rest;
  struct char_data *k;
  struct follow_type *f;

  if (IS_NPC(ch))
    return;

  one_argument(argument, buf);

  if (is_number(buf)) {
    amount = atoi(buf);
    if (amount <= 0) {
      send_to_char("Sorry, you can't do that.\r\n", ch);
      return;
    }
    if (amount > GET_GOLD(ch)) {
      send_to_char("You don't seem to have that much gold to split.\r\n", ch);
      return;
    }
    k = (ch->master ? ch->master : ch);

    if (AFF_FLAGGED(k, AFF_GROUP) && (k->in_room == ch->in_room))
      num = 1;
    else
      num = 0;

    for (f = k->followers; f; f = f->next)
      if (AFF_FLAGGED(f->follower, AFF_GROUP) &&
	  (!IS_NPC(f->follower)) &&
	  (f->follower->in_room == ch->in_room))
	num++;

    if (num && AFF_FLAGGED(ch, AFF_GROUP)) {
      share = amount / num;
      rest = amount % num;
    } else {
      send_to_char("With whom do you wish to share your gold?\r\n", ch);
      return;
    }

    GET_GOLD(ch) -= share * (num - 1);

    sprintf(buf, "%s splits &Y%d&n coins; you receive &Y%d&n.\r\n", GET_NAME(ch),
            amount, share);
    if (rest) {
      sprintf(buf + strlen(buf), "&Y%d&n coin%s %s not splitable, so %s "
              "keeps the money.\r\n", rest,
              (rest == 1) ? "" : "s",
              (rest == 1) ? "was" : "were",
              GET_NAME(ch));
    }
    if (AFF_FLAGGED(k, AFF_GROUP) && (k->in_room == ch->in_room)
	&& !(IS_NPC(k)) && k != ch) {
      GET_GOLD(k) += share;
      send_to_char(buf, k);
    }
    for (f = k->followers; f; f = f->next) {
      if (AFF_FLAGGED(f->follower, AFF_GROUP) &&
	  (!IS_NPC(f->follower)) &&
	  (f->follower->in_room == ch->in_room) &&
	  f->follower != ch) {
	GET_GOLD(f->follower) += share;
	send_to_char(buf, f->follower);
      }
    }
    sprintf(buf, "You split &Y%d&n coins among %d members -- &Y%d&n coins each.\r\n",
	    amount, num, share);
    if (rest) {
      sprintf(buf + strlen(buf), "&Y%d&n coin%s %s not splitable, so you keep "
                                 "the money.\r\n", rest,
                                 (rest == 1) ? "" : "s",
                                 (rest == 1) ? "was" : "were");
      GET_GOLD(ch) += rest;
    }
    send_to_char(buf, ch);
  } else {
    send_to_char("How many coins do you wish to split with your group?\r\n", ch);
    return;
  }
}



ACMD(do_use)
{
  struct obj_data *mag_item;

  half_chop(argument, arg, buf);
  if (!*arg) {
    sprintf(buf2, "What do you want to %s?\r\n", CMD_NAME);
    send_to_char(buf2, ch);
    return;
  }
  mag_item = GET_EQ(ch, WEAR_HOLD);

  if (!mag_item || !isname(arg, mag_item->name)) {
    switch (subcmd) {
    case SCMD_RECITE:
    case SCMD_QUAFF:
      if (!(mag_item = get_obj_in_list_vis(ch, arg, ch->carrying))) {
	sprintf(buf2, "You don't seem to have %s %s.\r\n", AN(arg), arg);
	send_to_char(buf2, ch);
	return;
      }
      break;
    case SCMD_USE:
      sprintf(buf2, "You don't seem to be holding %s %s.\r\n", AN(arg), arg);
      send_to_char(buf2, ch);
      return;
    default:
      log("SYSERR: Unknown subcmd %d passed to do_use.", subcmd);
      return;
    }
  }
  switch (subcmd) {
  case SCMD_QUAFF:
    if (GET_OBJ_TYPE(mag_item) != ITEM_POTION) {
      send_to_char("You can only quaff potions.\r\n", ch);
      return;
    }
    break;
  case SCMD_RECITE:
    if (GET_OBJ_TYPE(mag_item) != ITEM_SCROLL) {
      send_to_char("You can only recite scrolls.\r\n", ch);
      return;
    }
    break;
  case SCMD_USE:
    if ((GET_OBJ_TYPE(mag_item) != ITEM_WAND) &&
	(GET_OBJ_TYPE(mag_item) != ITEM_STAFF)) {
      send_to_char("You can't seem to figure out how to use it.\r\n", ch);
      return;
    }
    break;
  }

  mag_objectmagic(ch, mag_item, buf);
}



ACMD(do_wimpy)
{
  int wimp_lev;

  /* 'wimp_level' is a player_special. -gg 2/25/98 */
  if (IS_NPC(ch))
    return;

  one_argument(argument, arg);

  if (!*arg) {
    if (GET_WIMP_LEV(ch)) {
      sprintf(buf, "Your current wimp level is %d hit points.\r\n",
	      GET_WIMP_LEV(ch));
      send_to_char(buf, ch);
      return;
    } else {
      send_to_char("At the moment, you're not a wimp.  (sure, sure...)\r\n", ch);
      return;
    }
  }
  if (isdigit(*arg)) {
    if ((wimp_lev = atoi(arg)) != 0) {
      if (wimp_lev < 0)
	send_to_char("Heh, heh, heh.. we are jolly funny today, eh?\r\n", ch);
      else if (wimp_lev > GET_MAX_HIT(ch))
	send_to_char("That doesn't make much sense, now does it?\r\n", ch);
      else if (wimp_lev > (GET_MAX_HIT(ch) / 2))
	send_to_char("You can't set your wimp level above half your hit points.\r\n", ch);
      else {
	sprintf(buf, "Okay, you'll wimp out if you drop below %d hit points.\r\n",
		wimp_lev);
	send_to_char(buf, ch);
	GET_WIMP_LEV(ch) = wimp_lev;
      }
    } else {
      send_to_char("Okay, you'll now tough out fights to the bitter end.\r\n", ch);
      GET_WIMP_LEV(ch) = 0;
    }
  } else
    send_to_char("Specify at how many hit points you want to wimp out at.  (0 to disable)\r\n", ch);
}


ACMD(do_display)
{
  size_t i;

  if (IS_NPC(ch)) {
    send_to_char("Mosters don't need displays.  Go away.\r\n", ch);
    return;
  }
  skip_spaces(&argument);

  if (!*argument) {
    send_to_char("Usage: prompt { H | M | V | X | L | all | none }\r\n", ch);
    return;
  }
  if ((!str_cmp(argument, "on")) || (!str_cmp(argument, "all")))
    SET_BIT(PRF_FLAGS(ch), PRF_DISPHP | PRF_DISPMANA | PRF_DISPMOVE | PRF_DISPEXP | PRF_DISPALIGN);
  else {
    REMOVE_BIT(PRF_FLAGS(ch), PRF_DISPHP | PRF_DISPMANA | PRF_DISPMOVE | PRF_DISPEXP | PRF_DISPALIGN);
 
    for (i = 0; i < strlen(argument); i++) {
      switch (LOWER(argument[i])) {
      case 'h':
        SET_BIT(PRF_FLAGS(ch), PRF_DISPHP);
        break;
      case 'm':
        SET_BIT(PRF_FLAGS(ch), PRF_DISPMANA);
        break;
      case 'v':
        SET_BIT(PRF_FLAGS(ch), PRF_DISPMOVE);
        break;
      case 'x':
        SET_BIT(PRF_FLAGS(ch), PRF_DISPEXP);
        break;
      case 'l':
        SET_BIT(PRF_FLAGS(ch), PRF_DISPALIGN);
        break;
 
      }
    }
  } 

  send_to_char(OK, ch);
}



ACMD(do_gen_write)
{
  FILE *fl;
  char *tmp, buf[MAX_STRING_LENGTH];
  const char *filename;
  struct stat fbuf;
  time_t ct;

  switch (subcmd) {
  case SCMD_BUG:
    filename = BUG_FILE;
    break;
  case SCMD_TYPO:
    filename = TYPO_FILE;
    break;
  case SCMD_IDEA:
    filename = IDEA_FILE;
    break;
  default:
    return;
  }

  ct = time(0);
  tmp = asctime(localtime(&ct));

  if (IS_NPC(ch)) {
    send_to_char("Monsters can't have ideas - Go away.\r\n", ch);
    return;
  }

  skip_spaces(&argument);
  delete_doubledollar(argument);

  if (!*argument) {
    send_to_char("That must be a mistake...\r\n", ch);
    return;
  }
  sprintf(buf, "%s %s: %s", GET_NAME(ch), CMD_NAME, argument);
  mudlog(buf, CMP, LVL_IMMORT, FALSE);

  if (stat(filename, &fbuf) < 0) {
    perror("SYSERR: Can't stat() file");
    return;
  }
  if (fbuf.st_size >= max_filesize) {
    send_to_char("Sorry, the file is full right now.. try again later.\r\n", ch);
    return;
  }
  if (!(fl = fopen(filename, "a"))) {
    perror("SYSERR: do_gen_write");
    send_to_char("Could not open the file.  Sorry.\r\n", ch);
    return;
  }
  fprintf(fl, "%-8s (%6.6s) [%5d] %s\n", GET_NAME(ch), (tmp + 4),
	  GET_ROOM_VNUM(IN_ROOM(ch)), argument);
  fclose(fl);
  send_to_char("Okay.  Thanks!\r\n", ch);
}



#define TOG_OFF 0
#define TOG_ON  1

#define PRF_TOG_CHK(ch,flag) ((TOGGLE_BIT(PRF_FLAGS(ch), (flag))) & (flag))

ACMD(do_gen_tog)
{
  long result;

  const char *tog_messages[][2] = {
    {"You are now safe from summoning by other players.\r\n",
    "You may now be summoned by other players.\r\n"},
    {"Nohassle disabled.\r\n",
    "Nohassle enabled.\r\n"},
    {"Brief mode off.\r\n",
    "Brief mode on.\r\n"},
    {"Compact mode off.\r\n",
    "Compact mode on.\r\n"},
    {"You can now hear tells.\r\n",
    "You are now deaf to tells.\r\n"},
    {"You can now hear auctions.\r\n",
    "You are now deaf to auctions.\r\n"},
    {"You can now hear shouts.\r\n",
    "You are now deaf to shouts.\r\n"},
    {"You can now hear gossip.\r\n",
    "You are now deaf to gossip.\r\n"},
    {"You can now hear the congratulation messages.\r\n",
    "You are now deaf to the congratulation messages.\r\n"},
    {"You can now hear the Wiz-channel.\r\n",
    "You are now deaf to the Wiz-channel.\r\n"},
    {"You are no longer part of the Quest.\r\n",
    "Okay, you are part of the Quest!\r\n"},
    {"You will no longer see the room flags.\r\n",
    "You will now see the room flags.\r\n"},
    {"You will now have your communication repeated.\r\n",
    "You will no longer have your communication repeated.\r\n"},
    {"HolyLight mode off.\r\n",
    "HolyLight mode on.\r\n"},
    {"Nameserver_is_slow changed to NO; IP addresses will now be resolved.\r\n",
    "Nameserver_is_slow changed to YES; sitenames will no longer be resolved.\r\n"},
    {"Autoexits disabled.\r\n",
    "Autoexits enabled.\r\n"},
    {"You can now hear the Immnet Channel.\r\n",
    "You are now deaf to the Immnet Channel.\r\n"},
    {"You are no longer marked as (AFK).\r\n",
     "You are now marked as (AFK).  You will recieve no tells!\r\n"},
    {"You can see the info channel now.\r\n",
     "You can't see the info channel now.\r\n"},
    {"You can hear newbies now.\r\n",
    "You are now deaf to newbies.\r\n"},
    {"You can hear clan talk now.\r\n",
    "You are now deaf to your fellow clan members.\r\n"},
    {"You will no longer autoloot corpses.\r\n",
    "You will autoloot corpses now.\r\n"},
    {"You will no longer loot gold from corpses.\r\n",
    "You will now loot gold from corpses.\r\n"},
    {"You will no longer autosplit gold when grouped.\r\n",
    "You will now autosplit gold when grouped.\r\n"}, 
    {"Will no longer track through doors.\r\n",
    "Will now track through doors.\r\n"}
  };


  if (IS_NPC(ch))
    return;

  switch (subcmd) {
  case SCMD_NOSUMMON:
    result = PRF_TOG_CHK(ch, PRF_SUMMONABLE);
    break;
  case SCMD_NOHASSLE:
    result = PRF_TOG_CHK(ch, PRF_NOHASSLE);
    break;
  case SCMD_BRIEF:
    result = PRF_TOG_CHK(ch, PRF_BRIEF);
    break;
  case SCMD_COMPACT:
    result = PRF_TOG_CHK(ch, PRF_COMPACT);
    break;
  case SCMD_NOTELL:
    result = PRF_TOG_CHK(ch, PRF_NOTELL);
    break;
  case SCMD_NOAUCTION:
    result = PRF_TOG_CHK(ch, PRF_NOAUCT);
    break;
  case SCMD_DEAF:
    result = PRF_TOG_CHK(ch, PRF_DEAF);
    break;
  case SCMD_NOGOSSIP:
    result = PRF_TOG_CHK(ch, PRF_NOGOSS);
    break;
  case SCMD_NOGRATZ:
    result = PRF_TOG_CHK(ch, PRF_NOGRATZ);
    break;
  case SCMD_NOWIZ:
    result = PRF_TOG_CHK(ch, PRF_NOWIZ);
    break;
  case SCMD_QUEST:
    send_to_char("Please tell current QuestMaster you want in on the quest.\r\n", ch);
    return;
//    result = PRF_TOG_CHK(ch, PRF_QUEST);
//    break;
  case SCMD_ROOMFLAGS:
    result = PRF_TOG_CHK(ch, PRF_ROOMFLAGS);
    break;
  case SCMD_NOREPEAT:
    result = PRF_TOG_CHK(ch, PRF_NOREPEAT);
    break;
  case SCMD_HOLYLIGHT:
    result = PRF_TOG_CHK(ch, PRF_HOLYLIGHT);
    break;
  case SCMD_SLOWNS:
    result = (nameserver_is_slow = !nameserver_is_slow);
    break;
  case SCMD_AUTOEXIT:
    result = PRF_TOG_CHK(ch, PRF_AUTOEXIT);
    break;
  case SCMD_AFK:
    result = PRF_TOG_CHK(ch, PRF_AFK);
    break;
 case SCMD_NOINFO:
    result = PRF_TOG_CHK(ch, PRF_NOINFO);
    break;
 case SCMD_NONEWBIE:
    result = EXT_TOG_CHK(ch, EXT_NONEWBIE);
    break;
 case SCMD_NOCTALK:
    result = EXT_TOG_CHK(ch, EXT_NOCTALK);
    break;
 case SCMD_AUTOLOOT:
    result = EXT_TOG_CHK(ch, EXT_AUTOLOOT);
    break;
 case SCMD_AUTOGOLD:
    result = EXT_TOG_CHK(ch, EXT_AUTOGOLD);
    break;
 case SCMD_AUTOSPLIT:
    result = EXT_TOG_CHK(ch, EXT_AUTOSPLIT);
    break;

  case SCMD_TRACK:
    result = (track_through_doors = !track_through_doors);
    break;
  default:
    log("SYSERR: Unknown subcmd %d in do_gen_toggle.", subcmd);
    return;
  }

  if (result)
    send_to_char(tog_messages[subcmd][TOG_ON], ch);
  else
    send_to_char(tog_messages[subcmd][TOG_OFF], ch);

  return;
}

/* DM - modified spy skill written by Eric V. Bahmer */
ACMD(do_spy)
{
  int percent, prob, spy_type, return_room;
  char direction[MAX_INPUT_LENGTH];
  extern const char *dirs[];
 
  if (!has_stats_for_skill(ch, SKILL_SPY))
    return;
 
  half_chop(argument, direction, buf);
 
  /* 101% is a complete failure */
  percent = number(1, 101);
  prob = GET_SKILL(ch, SKILL_SPY);
  spy_type = search_block(direction, dirs, FALSE);
 
  if (spy_type < 0 || !EXIT(ch, spy_type) || EXIT(ch, spy_type)->to_room == NOWHERE) {
    send_to_char("Spy where?\r\n", ch);
    return;
  } else {
     if (!(GET_MOVE(ch) >= 7)) {
        send_to_char("You don't have enough movement points.\r\n", ch);
     }
     else {
        if (percent > prob) {
           send_to_char("You miserably attempt to spy.\r\n", ch);
           GET_MOVE(ch) = MAX(0, MIN(GET_MAX_MOVE(ch), GET_MOVE(ch) - 2));
        }
        else {
           if (IS_SET(EXIT(ch, spy_type)->exit_info, EX_CLOSED) && EXIT(ch, spy_type)->keyword) {
              sprintf(buf, "The %s is closed.\r\n", fname(EXIT(ch, spy_type)->keyword));
              send_to_char(buf, ch);
              GET_MOVE(ch) = MAX(0, MIN(GET_MAX_MOVE(ch), GET_MOVE(ch) - 2));
           }
           else {
              if (ROOM_FLAGGED(world[ch->in_room].dir_option[spy_type]->to_room, ROOM_HOUSE
| ROOM_PRIVATE)) {
                 send_to_char("Stick your nosy nose somewhere else!\r\n",ch);
                 return;
              }
              GET_MOVE(ch) = MAX(0, MIN(GET_MAX_MOVE(ch), GET_MOVE(ch) - 5));
              return_room = ch->in_room;
              char_from_room(ch);
              char_to_room(ch, world[return_room].dir_option[spy_type]->to_room);
              send_to_char("You spy into the next room and see: \r\n\r\n", ch);
              look_at_room(ch, 1);
              char_from_room(ch);
              char_to_room(ch, return_room);
              act("$n peeks into the next room.", TRUE, ch, 0, 0, TO_NOTVICT);
          }
        }
     }
  }
}  

ACMD(do_pagewidth)
{
  int page_width;

  one_argument(argument, arg);
 
  if (!*arg) {
    sprintf(buf, "Your current page width is &c%d&n chars.\r\n",
              GET_PAGE_WIDTH(ch));
    send_to_char(buf, ch);
    return;
  } else {
    if (isdigit(*arg)) {
      if ((page_width = atoi(arg))) {
        if ((page_width < 40) || (page_width > 250)) {
          send_to_char("Page Width must be between &c40&n-&c250&n chars.\r\n", ch);
          return;
        } else {
          GET_PAGE_WIDTH(ch) = page_width;
          sprintf(buf,"Page Width changed to &c%d&n chars.\r\n",GET_PAGE_WIDTH(ch));
          send_to_char(buf,ch);
          save_char(ch,NOWHERE);
        }
      } else {
        send_to_char("Yeah, right.\r\n",ch);
        return;
      }
    } else {
      sprintf(buf, "Your current page width is &c%d&n chars.\r\n",
              GET_PAGE_WIDTH(ch));
      send_to_char(buf, ch);
    }
  }
}

ACMD(do_pagelength)
{
  int page_length;

  one_argument(argument, arg);
 
  if (!*arg) {
    sprintf(buf, "Your current page length is &c%d&n lines.\r\n",
              GET_PAGE_LENGTH(ch));
    send_to_char(buf, ch);
    return;
  } else {
    if (isdigit(*arg)) {
      if ((page_length = atoi(arg))) {
        if ((page_length < 10) || (page_length > 100)) {
          send_to_char("Page Length must be between &c10&n-&c100&n lines.\r\n", ch);
          return;
        } else {
          GET_PAGE_LENGTH(ch) = page_length;
          sprintf(buf,"Page Length changed to &c%d&n lines.\r\n",GET_PAGE_LENGTH(ch));
          send_to_char(buf,ch);
          save_char(ch,NOWHERE);
        }
      } else {
        send_to_char("Yeah, right.\r\n",ch);
        return;
      }
    } else {
      sprintf(buf, "Your current page length is &c%d&n lines.\r\n",
              GET_PAGE_LENGTH(ch));
      send_to_char(buf, ch);
    }
  }  
}

/***************************************************************************/
/* Clan functions -Hal, cleaned up by DM                                   */
/***************************************************************************/
 
ACMD(do_signup)
{
 
  if (GET_LEVEL(ch) < 10) {
    send_to_char("You can't join a clan till level 10!\r\n", ch);
    return;
  }
 
  if (GET_CLAN_NUM(ch) >= 0) {
    send_to_char("You are already in a clan!\r\n", ch);
    return;
  }
 
  if (EXT_FLAGGED(ch, EXT_CLAN)) {
    send_to_char("You don't want to join a clan now.\r\n", ch);
    REMOVE_BIT(EXT_FLAGS(ch), EXT_CLAN);
  } else {
    send_to_char("You want to join a clan now.\r\n", ch );
    SET_BIT(EXT_FLAGS(ch), EXT_CLAN);
  }
}
 
ACMD(do_recruit)
{
  struct char_data *vict;
  char vict_name[MAX_INPUT_LENGTH];
 
  one_argument(argument, vict_name);
 
  if (!EXT_FLAGGED(ch, EXT_LEADER) && !EXT_FLAGGED(ch, EXT_SUBLEADER)) {
    send_to_char("You are not a clan leader or a knight.\r\n", ch);
    return;
  }
 
  if (!(vict = get_char_room_vis(ch, vict_name))) { 
    send_to_char("There is no one here by that name to recruit\r\n", ch);
    return;
  }
 
  if (IS_NPC(vict)) {
    sprintf(buf, "You can't recruit %s!\r\n", HMHR(vict));
    send_to_char(buf, ch);
    return;
  }
 
  if (GET_CLAN_NUM(vict) >= 0) {
    sprintf(buf, "%s is already in a clan!\r\n", HSSH(vict));
    buf[0] = toupper(buf[0]);
    send_to_char(buf, ch);
    return;
  }
 
  if (!(EXT_FLAGGED(vict, EXT_CLAN))) {
    sprintf(buf, "%s does not want to be in a clan.\r\n", GET_NAME(vict));
    send_to_char(buf, ch);
    return;
  }
 
  GET_CLAN_NUM(vict)=GET_CLAN_NUM(ch);
  sprintf(buf, "You are now a member of the %s.\r\n", get_clan_disp(vict));
  send_to_char(buf, vict);
  sprintf(buf, "%s is now a member of the %s.\r\n", GET_NAME(vict), get_clan_disp(vict));
  send_to_char(buf, ch);
}
 
ACMD(do_knight)
{
  struct char_data *vict;
  char vict_name[MAX_INPUT_LENGTH];
 
  one_argument(argument, vict_name);
 
  if (!(vict = get_char_room_vis(ch, vict_name))) {
    send_to_char("There is no one here by that name to knight.\r\n", ch);
    return; 
  }
 
  if (check_clan(ch, vict, 0)) {
    if (EXT_FLAGGED(vict, EXT_LEADER)) {
      send_to_char("But they are aready a leader.\r\n", ch);
      return;
    }
 
    if (EXT_FLAGGED(vict, EXT_SUBLEADER)) {
      sprintf(buf, "%s has been striped of %s knighthood.\r\n", GET_NAME(vict), HSHR(vict));
      send_to_char(buf, ch);
      sprintf(buf, "Your knighthood has been taken from you.\r\n" );
      send_to_char(buf, vict);
      REMOVE_BIT(EXT_FLAGS(vict), EXT_SUBLEADER);
    } else {
      sprintf(buf, "%s is now a knight.\r\n", GET_NAME(vict));
      send_to_char(buf, ch);
      sprintf(buf, "You are now a knight.\r\n");
      send_to_char(buf, vict);
      SET_BIT(EXT_FLAGS(vict), EXT_SUBLEADER);
    }
  }
}
 
ACMD(do_banish)
{
  struct char_data *vict;  char
  vict_name[MAX_INPUT_LENGTH];
 
  one_argument(argument, vict_name);
 
  if (!(vict = get_char_room_vis(ch, vict_name))) {
    send_to_char("There is no one here by that name to banish\r\n", ch);
    return;
  }
 
  if (check_clan(ch, vict, 0)) {
    sprintf(buf, "You have been banished from the %s.\r\n", get_clan_disp(vict));
    send_to_char(buf, vict);
    sprintf(buf, "%s has been banished from the %s.\r\n", GET_NAME(vict), get_clan_disp(vict));
    GET_CLAN_NUM(vict) = -1;
 
    GET_CLAN_LEV(vict) = 0;
    REMOVE_BIT(EXT_FLAGS(vict), EXT_CLAN);
 
    if (EXT_FLAGGED(vict, EXT_SUBLEADER))
      REMOVE_BIT(EXT_FLAGS(vict), EXT_SUBLEADER);
 
    if (EXT_FLAGGED(vict, EXT_LEADER))
      REMOVE_BIT(EXT_FLAGS(vict), EXT_LEADER);
 
    send_to_char(buf, ch);
  }
}
 
ACMD(do_demote)
{
  struct char_data *vict;
  char vict_name[MAX_INPUT_LENGTH];
 
  one_argument(argument, vict_name);
 
  if (!(vict = get_char_room_vis(ch, vict_name))) {
    send_to_char("There is no one here by that name to demote\r\n", ch);
    return;
  }
 
  if (check_clan(ch, vict, 0)) {
    if (GET_CLAN_LEV(vict) != 0) {
      GET_CLAN_LEV(vict)--;
      sprintf(buf, "You have been demoted to the rank of %s.\r\n", get_clan_rank(vict));
      send_to_char(buf, vict);
 
      sprintf(buf, "%s has been demoted to the rank of %s.\r\n", GET_NAME(vict), get_clan_rank(vict));
      send_to_char(buf, ch);
    } else {
      sprintf(buf, "%s aready has the lowest rank.\r\n", GET_NAME(vict));
      send_to_char(buf, ch);
    }
  } 
}
 
ACMD(do_promote)
{
  struct char_data *vict;
  char vict_name[MAX_INPUT_LENGTH];
 
  one_argument(argument, vict_name);
 
  if (!(vict = get_char_room_vis(ch, vict_name))) {
    send_to_char("There is no one here by that name to promote\r\n", ch);
    return;
  }
 
  if (check_clan(ch, vict, 0)) {
    if (GET_CLAN_LEV(vict) != (RANK_NUM - 1)) {
      GET_CLAN_LEV(vict) ++ ;
 
      sprintf(buf, "You have been promoted to the rank of %s.\r\n", get_clan_rank(vict));
      send_to_char(buf, vict);
 
      sprintf(buf, "%s has been promoted to the rank of %s.\r\n", GET_NAME(vict), get_clan_rank(vict));
      send_to_char(buf, ch);
    } else {
      sprintf(buf, "%s has aready reached the highest rank.\r\n", GET_NAME(vict));
      send_to_char(buf, ch);
    }
  }
}

/* Autoassist - DM */
ACMD(do_autoassist)
{
  struct char_data *vict, *test;
  struct assisters_type *k;
  char assistname[MAX_INPUT_LENGTH];
 
  if (IS_NPC(ch)) {
    send_to_char("I'm a mob, I ain't autoassisting anyone!\r\n",ch);
    return;
  }
 
  one_argument(argument, assistname);
  vict=AUTOASSIST(ch);
 
  /* No argument */
  if (!*assistname) {
    if (vict) {
      stop_assisting(ch);
    } else {
      act("Autoassist who?",FALSE,ch,0,0,TO_CHAR);
    }
 
  /* Argument given */
  } else {
    test=get_char_room_vis(ch,assistname);
  /* Already autoassisting */
    if (vict) {
 
      /* Couldn't find target */
      if (!(test)) {
        act(NOPERSON,FALSE,ch,0,0,TO_CHAR);
 
      /* Found different Target */
      } else if (test!=vict) {
        act("You are already assisting $N.",TRUE,ch,0,vict,TO_CHAR);
 
      /* Found same Target */
      } else {
        act("You are already autoassisting $M.",TRUE,ch,0,vict,TO_CHAR);
     }
    } else {
    /* Dont have victim */
      if ((ch!=test) && (test)) {
        act("$n starts autoassisting you.",FALSE,ch,0,test,TO_VICT);
        act("$n starts autoassisting $N.",FALSE,ch,0,test,TO_NOTVICT);
        act("Okay, from now on you will autoassist $N.",TRUE,ch,0,test,TO_CHAR);        AUTOASSIST(ch) = test;
 
        /* Add to head of targets assisters_type struct */
        CREATE(k, struct assisters_type, 1);
        k->assister=ch;
        k->next=test->autoassisters;
        test->autoassisters=k;
 
      } else {
        if (ch==test)
          act("Yeah, right.",FALSE,ch,0,0,TO_CHAR);
        else
          act(NOPERSON,TRUE,ch,0,0,TO_CHAR);
      }
    }
  }
} 
/* this is the command used to transform into a wolf/vamp - Vader */
ACMD(do_change)
{
  ACMD(do_gen_comm);
  struct affected_type af;
  float wfactor = 0,vfactor = 0;
 
  if(ch == NULL)
    return;
 
  if(affected_by_spell(ch,SPELL_CHANGED)) {
    send_to_char("Your already changed!\r\n",ch);
    return;
    }
 
  af.type = SPELL_CHANGED;
  af.duration = 7;
  af.bitvector = 0;
  af.modifier = 0;
  af.location = APPLY_NONE;
 
/* are we infected? */
  if(!(PRF_FLAGGED(ch,PRF_WOLF) || PRF_FLAGGED(ch,PRF_VAMPIRE))) {
    send_to_char("Your not even infected! Think yourself lucky!\r\n",ch);
    return;
    }
/* is it dark? */
  if(weather_info.sunlight != SUN_DARK) {
    send_to_char("You can't draw any power from the moon during the day.\r\n",ch);
    return;
    }
/* is there a moon? */
  if(weather_info.moon == MOON_NONE) {
    send_to_char("There is no moon to draw power from tonight.\r\n",ch);
    return;
    }
 
/* change amount of benefits depending on moon */
  switch(weather_info.moon) {
    case MOON_1ST_QTR:   
    case MOON_FINAL_QTR:
      wfactor = 1.25;
      vfactor = 1.125;
      break;
    case MOON_HALF:
    case MOON_2ND_HALF:
      wfactor = 1.5;
      vfactor = 1.25;
      break;
    case MOON_3RD_QTR:
    case MOON_2ND_3RD_QTR:
      wfactor = 1.75;
      vfactor = 1.375;
      break;
    default:
      wfactor = 2;
      vfactor = 1.5;
      break;
    }
 
/* if wolf: go wolfy else is vamp: vampout */
  if(PRF_FLAGGED(ch, PRF_WOLF)) {
    send_to_char("You scream in agony as the transformation takes place...\r\n",ch);
    act("A bloody pentangle appears on $n's hand!",FALSE,ch,0,0,TO_ROOM);
    act("$n screams in pain as $e transforms into a werewolf!",FALSE,ch,0,0,TO_ROOM);
    GET_MOVE(ch) += 50; /* make sure they can holler */
    do_gen_comm(ch,"HHHHHHOOOOOOWWWWWWLLLLLL!!!!!!",0,SCMD_HOLLER);
 
    af.modifier = 2;
    af.location = APPLY_STR;
    affect_join(ch,&af,0,0,0,0);
    GET_HIT(ch) *= wfactor;
    GET_MOVE(ch) *= wfactor;
    if(weather_info.moon == MOON_FULL)
      af.bitvector = AFF_INFRAVISION;
  } else if(PRF_FLAGGED(ch,PRF_VAMPIRE)) {
      send_to_char("You smile as your fangs extend...\r\n",ch);
      act("You catch a glipse of $n's fangs in the moonlight.",FALSE,ch,0,0,TO_ROOM);
 
      af.modifier = 2;     
     af.location = APPLY_WIS;
      affect_join(ch,&af,0,0,0,0);
      GET_MANA(ch) *= vfactor;
      GET_MOVE(ch) *= vfactor;
      if(weather_info.moon == MOON_FULL)
        af.bitvector = AFF_INFRAVISION;
      }
 
}  

ACMD(do_ignore)
{
 
 int id , i , num , level  ;
 int del , done , already ;
 int ign [3] ;
 char buf[MAX_STRING_LENGTH];
 
 one_argument(argument, arg);
 
 ign[0] = 0 ;
 ign[1] = 0 ;
 ign[2] = 0 ;
 
 level = 0 ;
 already = 0 ;
 done = 0 ;
 del = 0;
 i = 0;
 num = 0 ;
 
 if ( GET_IGN1(ch) != 0 )
 {
  ign[i] = GET_IGN1(ch) ;
  i++;
 }
 
 if ( GET_IGN2(ch) != 0 )
 {
  ign[i] = GET_IGN2(ch) ;
  i++;
 }
 
 if ( GET_IGN3(ch) != 0 )
  ign[i] = GET_IGN3(ch) ;
 
 num = GET_IGN_NUM(ch) ;
 level = GET_IGN_LEVEL(ch) ;
 
 if (PLR_FLAGGED(ch, PLR_NOIGNORE ))
 {
  send_to_char ( "\n\rThe Gods do not like you using this command.\n\r" ,ch );
  return ;
 }
 
 if ( !*arg )
 {
  if ( num == 0 && level == 0  )
  send_to_char ( "\n\rYou are not ignoring anyone.\n\r" , ch );
  else
  {
   if ( num > 0 )
   {
    sprintf ( buf , "\n\rYou are ignoring :\n\r" );
    for ( i = 0 ; i < 3 ; i++ )
     if ( ign[i] != 0 )
      sprintf ( buf, "%s%s\n\r" , buf , get_name_by_id ( ign[i] )  );
      send_to_char ( buf , ch ) ;
   }
   if ( level > 0 )
   {
    sprintf ( buf , "\n\rYou are also ignoring everbody under level %d.\n\r" , level ) ;
    send_to_char ( buf , ch ) ;
   }
  }
 }
 else
 if ( isdigit(*arg) )
  {
 
   if ( atoi(arg) > GET_LEVEL(ch) )
     send_to_char ( "\n\rIgnore level can not be higher than your level.\n\r" , ch ) ;
 
   if ( atoi(arg) <= GET_LEVEL(ch) )
     {
      level = atoi (arg);
      GET_IGN_LEVEL(ch) = level ;
      sprintf ( buf , "\n\rYou are now ignoring everyone under level %d.\n\r" , level );                       
      send_to_char ( buf , ch ) ;
     }
   }
   else
   {
 
   id = get_id_by_name ( arg );
 
   if ( id < 0 && !isdigit(*arg) )
    {
     sprintf ( buf , "\n\rThat player does not exist.\n\r");
     send_to_char ( buf , ch ) ;
     return ;
    }
 
   if ( id == GET_IDNUM(ch) )
    {
     sprintf ( buf , "\n\rYou can't ignore your self.\n\r" ) ;
     send_to_char ( buf , ch ) ;
     return ;
    }
 
   for ( i = 0 ; i < 3 ; i++ )
     if ( id == ign[i]  )
       {
        del = 1;
        sprintf ( buf , "\n\rYou are now listening to %s.\n\r" , arg );
        send_to_char ( buf , ch ) ;
        ign[i] = 0 ;
        num -- ;
       }
 
   if ( num >= 3 )
     {
      sprintf ( buf , "\n\rYou are already ignoring three people!\n\r" );
      send_to_char ( buf , ch ) ;
      return ;
     }
 
   if ( !del )  
     {
        for ( i = 0 ; i < 3 ; i++ )
         if ( ign[i] == id )
         {
          already = 1 ;
          sprintf ( buf,  "\n\rYou are already ignoring %s.\n\r"  , arg );
          send_to_char ( buf , ch ) ;
         }
 
        if ( !already )
        {
         for ( i = 0 ; i < 3 ; i++ )
          if ( ign[i] == 0 && !done )
          {
           done = 1 ;
           ign[i] = id ;
           num++ ;
           sprintf (buf ,"\n\rYou are now ignoring %s.\n\r" , arg );
           send_to_char ( buf , ch ) ;
          }
         }
      }
 
   GET_IGN1(ch) = ign [0] ;
   GET_IGN2(ch) = ign [1] ;
   GET_IGN3(ch) = ign [2] ;
   GET_IGN_NUM(ch) = num ;
  }
 
 return ;
 
}  

/* command to join 2 objs together - Vader */
ACMD(do_join)
{
  char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  struct obj_data *part1, *part2, *joined;
  struct char_data *dummy;
 
  two_arguments(argument,arg1,arg2);
 
  if(!*arg1 || !*arg2) {
    send_to_char("Join what??\r\n",ch);
    return;
    }
 
  generic_find(arg1, FIND_OBJ_INV, ch, &dummy, &part1);
  if(!part1) {
    sprintf(buf, "You don't seem to have a %s.\r\n",arg1);
    send_to_char(buf,ch);
    return;
    }
  generic_find(arg2, FIND_OBJ_INV, ch, &dummy, &part2);
  if(!part2) {
    sprintf(buf, "You don't seem to have a %s.\r\n",arg2);
    send_to_char(buf,ch);
    return;
    }
 
  if(GET_OBJ_TYPE(part1) != ITEM_JOINABLE &&
     GET_OBJ_TYPE(part2) != ITEM_JOINABLE) {
    sprintf(buf,"%s and %s don't seem to fit together.\r\n",part1->short_description, part2->short_description);
    CAP(buf);
    send_to_char(buf,ch);
    return;
    }
  if((GET_OBJ_VAL(part1,0) != GET_OBJ_VNUM(part2)) ||
     (GET_OBJ_VAL(part2,0) != GET_OBJ_VNUM(part1))) {
    sprintf(buf,"%s and %s don't seem to fit together.\r\n",part1->short_description, part2->short_description);
    CAP(buf);  
    send_to_char(buf,ch);
    return;
    }
 
  joined = read_object(GET_OBJ_VAL(part1,3),VIRTUAL);
  sprintf(buf,"%s and %s fit together perfectly to make %s.\r\n",part1->short_description,part2->short_description,joined->short_description);
  CAP(buf);
  send_to_char(buf,ch);
  sprintf(buf,"%s joins %s and %s together to make %s.\r\n",GET_NAME(ch),part1->short_description,part2->short_description,joined->short_description);
  CAP(buf);
  act(buf,FALSE,ch,0,0,TO_ROOM);
 
  obj_to_char(joined,ch);
  extract_obj(part1);
  extract_obj(part2);
}
 
 
/* this command is for the tag game - Vader */
ACMD(do_tag)
{
  struct char_data *vict;
 
  one_argument(argument,arg);
 
  if(!*arg)
    send_to_char("Tag who?\r\n",ch);
  else if(!(vict = get_char_room_vis(ch,arg)))
    send_to_char("They don't seem to be here...\r\n",ch);
  else if(IS_NPC(vict))
    send_to_char("Errrrr I don't think Mobs play tag...\r\n",ch);
  else if(!PRF_FLAGGED(ch,PRF_TAG))
    send_to_char("You're not even playing tag!\r\n",ch);
  else if(!PRF_FLAGGED(vict,PRF_TAG))
    send_to_char("They arn't even playing tag!\r\n",ch);
  else {
    if(vict == ch) {
      send_to_char("You slap yourself and yell, 'You're it!'\r\n\r\n",ch);                                           act("$n slap $mself and yells, 'You're it!!'",FALSE,ch,0,0,TO_ROOM);
      sprintf(buf,"%s has tagged themself out of the game.",GET_NAME(ch));
    } else {
      act("You tag $N! Well done!",FALSE,ch,0,vict,TO_CHAR);
      act("You have been tagged by $n!\r\n",FALSE,ch,0,vict,TO_VICT);
      act("$n has tagged $N!",FALSE,ch,0,vict,TO_NOTVICT);
      sprintf(buf,"%s has been tagged by %s",GET_NAME(vict),GET_NAME(ch));
      }
    mudlog(buf,BRF,LVL_GOD,FALSE);
    REMOVE_BIT(PRF_FLAGS(vict),PRF_TAG);
    call_magic(ch,vict,NULL,42,GET_LEVEL(ch),CAST_MAGIC_OBJ);
    }
}
 
ACMD(do_mortal_kombat)
{
  struct descriptor_data *pt;
  int arena_room;
  char mybuf[256];
 
  arena_room = number(4557, 4560);
  if (FIGHTING(ch)){
        send_to_char(CHARFIGHTING,ch);
        return;
  }
  strcpy(mybuf, "");
  sprintf(mybuf, "\r\n[NOTE] %s has entered the Mortal Kombat ARENA!!\r\n", GET_NAME(ch));
  if (!PRF_FLAGGED(ch, PRF_MORTALK))
  {
  if (GET_GOLD(ch) < 10000)
        send_to_char("You don't have the gold to enter the ARENA!", ch);
  else{
        GET_GOLD(ch) -= 10000;
        char_from_room(ch);
        char_to_room(ch,real_room(arena_room));
        for (pt = descriptor_list; pt; pt = pt->next)
          if (!pt->connected && pt->character && pt->character != ch)
            send_to_char(mybuf, pt->character);
        if (PRF_FLAGGED(ch, PRF_NOREPEAT))   
          send_to_char(OK, ch);
        else
          send_to_char(mybuf, ch);
        SET_BIT(PRF_FLAGS(ch), PRF_MORTALK);
        look_at_room(ch, 0);
        strcpy(buf,"");
        send_to_char("\r\nWelcome to the Mortal Kombat ARENA.  Prepare for Combat!", ch);
 
   }
  }else
        send_to_char("You are already in the Mortal Kombat ARENA!!!", ch);
}    

/* command displays the mud local time - Hal */
ACMD(do_realtime)
{
        char *buf ;
        time_t ct;
 
        ct = time(0);
    buf = asctime(localtime(&ct));
 
    send_to_char( buf , ch);
}

/* proc to do the damage for Ruprect's indian attacks - Vader */
void do_arrows() {
  struct char_data *vict;
  int t,i;
  int dam;
  char limbs[5][10] = {
    "hand",
    "arm",
    "thigh",
    "shoulder",
    "back"
  };
  int rooms[72] = {
13600,13605,13615,13626,13640,13650,13664,13706,13716,13725,
13732,13740,13748,13749,13796,13797,13798,13693,13694,13695,
13696,13697,13698,13699,13700,13701,13674,13675,13676,13677,
13678,13679,13680,13734,13735,13736,13737,13738,13741,13742,
13743,13744,13745,13750,13751,13752,13753,13754,13755,13759,
13760,13761,13762,13764,13765,13771,13772,13773,13774,13775,
13776,13777,13778,13779,13780,13781,13782,13783,13784,13785,
13786,13787
  };
 
for(i=0;i<72;i++)
  for(vict = world[real_room(rooms[i])].people; vict; vict =vict->next_in_room) {
    if(!(number(0,8)) && GET_LEVEL(vict) < LVL_IS_GOD) {
      t = number(0,4);  
      sprintf(buf,"$n is hit in the %s by an arrow!",limbs[t]);
      act(buf,FALSE,vict,0,0,TO_ROOM);
      sprintf(buf,"You are hit in the %s by an arrow! It feels really pointy...\r\n",limbs[t]);
      send_to_char(buf,vict);
      dam = dice((t+1)*2,GET_LEVEL(vict));
      GET_HIT(vict) -= dam;
      update_pos(vict);
      if(GET_POS(vict) == POS_DEAD) {
        sprintf(buf,"%s killed by an arrow at %s",GET_NAME(vict),world[vict->in_room].name);
       if(!IS_NPC(vict))
        mudlog(buf,NRM,LVL_GOD,0);
        act("$n falls to the ground. As dead as something that isn't alive.",FALSE,vict,0,0,TO_ROOM);
        send_to_char("The arrow seems to have done permenant damage. You're dead.\r\n",vict);
        die(vict);
        }
      }
    }
}   

ACMD(do_attend_wounds)
{
  struct timer_type tim, *hjp;
  struct char_data *vict;
  int prob;
  bool found = FALSE; 

  one_argument(argument, arg);

  prob = GET_SKILL(ch, SKILL_ATTEND_WOUNDS);
  
  if (!prob) {
    send_to_char(UNFAMILIARSKILL,ch);
    return;  
  }

  if (!has_stats_for_skill(ch, SKILL_ATTEND_WOUNDS))
      return;

  if (!*arg) {
    vict = ch;
  } else {
    if (!(vict = get_char_vis(ch, arg, FIND_CHAR_ROOM))) {
      send_to_char(NOPERSON,ch);
      return;
    }
  }

  if ((hjp = affected_by_timer(ch, TIMER_HEALING_SKILLS))) 
    tim = *hjp;
  else {
    tim.type = TIMER_HEALING_SKILLS;
    tim.uses = 0;
    tim.max_uses = 2;
    tim.duration = 1;  
    tim.next = NULL;
    timer_to_char(ch,&tim);
    if (!(hjp = affected_by_timer(ch, TIMER_HEALING_SKILLS))) {
      mudlog("SYSERR: Timer not found for healing skills", BRF, LVL_IMPL, TRUE);
      return;
    }
  }

  hjp->uses++; 
  if (hjp->uses <= hjp->max_uses) {
    if (ch == vict) {
      act("You attend to your wounds (have to add the heal formula).", FALSE, ch, 0, vict, TO_CHAR);
      act("$n attends to his wounds.", TRUE, ch, 0, vict, TO_NOTVICT);
    } else {
      act("$n attends to your wounds.", FALSE, ch, 0, vict, TO_VICT);
      act("$n attends to $N's wounds.", TRUE, ch, 0, vict, TO_NOTVICT);
    }
  } else {
    send_to_char(RESTSKILL,ch);
    return;
  }
}

ACMD(do_adrenaline)
{
  struct affected_type af;   
  struct timer_type tim, *hjp;
  int prob;
  bool found = FALSE;
    
  if (affected_by_spell(ch, SKILL_ADRENALINE)) {
    send_to_char("Your already adrenalised.\r\n",ch);   
    return;
  }

  prob = GET_SKILL(ch, SKILL_ADRENALINE);
  if (!prob) {
    send_to_char(UNFAMILIARSKILL,ch);
    return;
  }

  if (!has_stats_for_skill(ch, SKILL_ADRENALINE))
      return;
       
  if ((hjp = affected_by_timer(ch, TIMER_ADRENALINE)))
    tim = *hjp;
  else {
    tim.type = TIMER_ADRENALINE;
    tim.uses = 0;
    tim.max_uses = 1;
    tim.duration = 5;
    tim.next = NULL;
    timer_to_char(ch,&tim);
    if (!(hjp = affected_by_timer(ch, TIMER_ADRENALINE))) {
      mudlog("SYSERR: Timer not found for adrenaline", BRF, LVL_IMPL, TRUE);
      return;
    }
  }
  
  hjp->uses++;
  if (hjp->uses <= hjp->max_uses) {
    af.type = SKILL_ADRENALINE;
    af.duration = 2;
    af.modifier = 0;
    af.location = APPLY_NONE;
    af.bitvector = AFF_ADRENALINE;

    affect_join(ch, &af, FALSE, FALSE, FALSE, FALSE);
    act("Your feel your adrenaline flow.", FALSE, ch, 0, ch, TO_CHAR);
    act("$n is pumped by adrenaline.", TRUE, ch, 0, ch, TO_NOTVICT);
  } else {
    send_to_char(RESTSKILL,ch);
    return;
  }
}
