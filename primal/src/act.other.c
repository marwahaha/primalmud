/* ************************************************************************
*   File: act.other.c                                   Part of CircleMUD *
*  Usage: Miscellaneous player-level commands                             *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "screen.h"
#include "house.h"

#define BATTERY_MANA 0

/* extern variables */
extern struct str_app_type str_app[];
extern struct room_data *world;
extern struct descriptor_data *descriptor_list;
extern struct room_data *world;
extern struct dex_skill_type dex_app_skill[];
extern struct spell_info_type spell_info[];
extern struct index_data *mob_index;
extern struct index_data *obj_index;
extern char *class_abbrevs[];
extern FILE *player_fl;

/* extern procedures */
SPECIAL(shop_keeper);
extern int get_world(struct char_data *ch);
extern int check_clan(struct char_data *ch, struct char_data *vict, 
			int sub_allow);
void look_at_char(struct char_data * i, struct char_data * ch);
void die(struct char_data *ch, struct char_data *killer);
int scan_buffer_for_xword(char* buf);

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
        if(!IS_NPC(vict)) {
          sprintf(buf,"%s killed by an arrow at %s",GET_NAME(vict),world[vict->in_room].name);
          mudlog(buf,NRM,LVL_GOD,0);
        }
        act("$n falls to the ground. As dead as something that isn't alive.",FALSE,vict,0,0,TO_ROOM);
        send_to_char("The arrow seems to have done permenant damage. You're dead.\r\n",vict);
        die(vict,NULL);
        }
      }
    }
}

/* Saves a character - used by the do_lag once flag is toggled */
void lag_save_char(struct char_data *ch, long pid) {

	struct char_file_u tmp;
	
	char_to_store(ch, &tmp);
	fseek(player_fl, (pid) * sizeof(struct char_file_u), SEEK_SET);
	fwrite(&tmp, sizeof(struct char_file_u), 1, player_fl);

}

/* lag - Pacifist (And quite cruel) way of pissing players you don't like,
         right off */
ACMD(do_lag) {

	struct char_data *tch;
	struct char_file_u tmp;
	int char_loaded = 0;
	long pid = 0;

	if( IS_NPC(ch) )
		return;

	two_arguments(argument, arg, buf1);

	if( !*arg ) {
	   send_to_char("Usage: lag <victim> | lag file <victim>\r\n",ch);
	   return;
	}

	if( strcmp(arg, "file") == 0 ) {
	   if( (pid = load_char(buf1, &tmp)) < 0 ) {
	     send_to_char("No such player exists.\r\n", ch);
	     return;
	   }
           CREATE(tch, struct char_data, 1);
	   clear_char(tch);
	   store_to_char(&tmp, tch);
	   char_to_room(tch, 0);
	   char_loaded = 1;
	}
	else { // Char is online, or so they think
	   if( !(tch = get_player_vis(ch, arg)) ) {
	      send_to_char("No such player visible at the moment.\r\n", ch);
	      return;
	   }
	}

	// By now we have a character, either way
	if( GET_LEVEL(tch) > LVL_IMMORT ) {
	  send_to_char("Gods cannot be lagged.\r\n", ch);
	  if( char_loaded )
		extract_char(tch);
	  return;
	}
	
	if( PLR_FLAGGED(tch, PLR_LAGGED) ) {
	  act("You remove $N's induced lag.", FALSE, ch, 0, tch, TO_CHAR);
	  REMOVE_BIT(PLR_FLAGS(tch), PLR_LAGGED);
	  if(char_loaded)
	    lag_save_char(tch, pid);
	  return;
	}

	act("You give $N some well deserved lag.", FALSE, ch, 0, tch, TO_CHAR);
	SET_BIT(PLR_FLAGS(tch), PLR_LAGGED);
	if( char_loaded )
	   lag_save_char(tch, pid);
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
        act("Okay, from now on you will autoassist $N.",TRUE,ch,0,test,TO_CHAR);
        AUTOASSIST(ch) = test;
 
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

ACMD(do_charge)
{
  /* A "battery" stores anything of an integer value */
  /* Value 0: the battery type */
  /* Value 1: max size */
  /* Value 2: current size */
  /* Value 3: charge ratio */
 
  struct obj_data *battery, *dummy;
  struct char_data *vict;
  int charge_type = 0, charge_max = 0, charge_current = 0, charge_ratio = 0;
  int charge_from = 0, charge_to = 0, charge_remaining = 0;
  char *charge_names[] = {"Mana"};
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
 
  two_arguments(argument,arg1,arg2);
 
  if (!*arg1) {
    send_to_char("Umm, charge what exactly?.\r\n",ch);
    return;
  }
 
  /* Find the battery */
  generic_find(arg1, FIND_OBJ_INV, ch, &vict, &battery);
 
  if (!battery) {
    sprintf(buf, "You don't seem to have a %s.\r\n",arg1);
    send_to_char(buf,ch);
    return;
  }
 
  /* Now check the item is a battery */
  if (!(GET_OBJ_TYPE(battery) == ITEM_BATTERY)) {
    send_to_char("You can't charge that.\r\n",ch);
    return;
  } else {
    /* find out the battery details */
    charge_type = GET_OBJ_VAL(battery, 0);
    charge_max = GET_OBJ_VAL(battery, 1);
    charge_current = GET_OBJ_VAL(battery, 2);
    charge_ratio = GET_OBJ_VAL(battery, 3);
 
    charge_remaining = charge_max - charge_current;
 
    /* DISPLAY BATTERY INFO */
    if (!*arg2) {
      sprintf(buf,"Battery Information:\r\n--------------------\r\nCharge Stored: %s\r\nMax Charge: %d\r\nCurrent Charge: %d\r\nCharge Ratio: %d/1\r\n",charge_names[charge_type],charge_max,charge_current,charge_ratio);
      send_to_char(buf,ch);
      return;
    }
 
    switch (charge_type) {
      case BATTERY_MANA :
 
        /* ADD MANA TO OBJECT */
        if (isdigit(*arg2)) {
          charge_to=atoi(arg2);
 
          /* Full Battery */
          if (charge_remaining == 0) {
            send_to_char("This battery is full.\r\n",ch);
            return;
          }
 
          /* Adding 0 or a negative number */
          if (charge_to <= 0) {
            send_to_char("Yep, your a comedian.\r\n",ch);
            return;
          }
 
          /* Adjust for (given charge > charge remaining) */
          if (charge_to > charge_remaining)
            charge_to = charge_remaining;
 
          charge_from = charge_ratio * charge_to; 
 
          if (GET_MANA(ch) < charge_from) {
            charge_to = GET_MANA(ch)/charge_ratio;
            charge_from = charge_ratio * charge_to;
          }
 
          if (charge_from == 0) {
            send_to_char("You haven't the energy to perform the charge.\r\n",ch);
            return;
          }
 
          GET_MANA(ch) -= charge_from;
          GET_OBJ_VAL(battery,2) += charge_to;
          sprintf(buf,"You give the battery a charge of %d at an expense of %d mana.\r\n",charge_to,charge_from);
         send_to_char(buf,ch);
         //   send_to_char("You charge the battery.\r\n",ch);
/* OLD
           Check available mana
          if (charge_from > GET_MANA(ch)) {
 
            if ((GET_MANA(ch)/charge_ratio) > 0) {
              return;
            } else {
            }
          } else {
            GET_MANA(ch) -= charge_from;
          }
*/
        /* EXTRACT MANA FROM OBJECT */
        } else {
          if (!str_cmp("self",arg2))
            vict=ch;
          else
            generic_find(arg2, FIND_CHAR_ROOM, ch, &vict, &dummy);
 
          if (!vict) {
            send_to_char("If only they where here.\r\n",ch);
            return;
          }
 
          /* Flat Battery */
          if (charge_current == 0) {
            send_to_char("This battery is flat.\r\n",ch);
            return;
          }
 
          /* Calculate victims mana */
          charge_to = GET_MAX_MANA(vict) - GET_MANA(vict);
 
          /* Victim has full mana */
          if (GET_MANA(vict) == GET_MAX_MANA(vict)) {
            send_to_char("Now that'd be a waste wouldn't it?\r\n",ch);
            return;
          }
 
          /* Use full battery */
          if (charge_to > charge_current)
            charge_to = charge_current;
 
          GET_MANA(vict) += charge_to;
          GET_OBJ_VAL(battery,2) -= charge_to;
 
          if (ch == vict) {
            sprintf(buf,"You extract %d mana from the battery.\r\n",charge_to);
            send_to_char(buf,ch);
          } else {
            sprintf(buf,"You extract %d mana to %s.\r\n",charge_to,GET_NAME(vict));
            send_to_char(buf,ch);
          }
        }
 
        break;
 
      default :
        send_to_char("This battery doesn't seem to be working.\r\n",ch);
        sprintf(buf,"%s using battery: %d, with undetermined battery type.\r\n",GET_NAME(ch),GET_OBJ_VNUM(battery));
        mudlog(buf, NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);
        break;
    }
  }
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
          sprintf(buf,"Have you seen anyone wielding %s latly?? It's out of fashion!\r\n",
                  obj1->short_description);
          break;
        case 1:
          sprintf(buf,"Get with it! Noone kills with %s anymore.\r\n",
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
          sprintf(buf,"If you this %s looks good now, wait til its stained with the blood of your enemies!!\r\n",
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
          sprintf(buf,"Lose afew pounds darling. Then you can consider wearing something like %s.\r\n",
                  obj1->short_description);
          break;
        case 3:
          sprintf(buf,"Yes! The floral pattern on %s looks smashing on you!\r\n",
                  obj2->short_description);
          break;
        case 4:
          sprintf(buf,"Come on darling. If you're going to wear %s with %s then you may aswell get \"FASHION VICTIM\" tattooed on your forehead!\r\n",
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

/* command displays the mud local time - Hal */
ACMD(do_realtime)
{
    
	char *buf ;
	time_t ct;
	
	ct = time(0);
    buf = asctime(localtime(&ct));
	
    send_to_char( buf , ch);
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
      send_to_char("You slap yourself and yell, 'You're it!'\r\n\r\n",ch);
      act("$n slap $mself and yells, 'You're it!!'",FALSE,ch,0,0,TO_ROOM);
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
  void char_to_room(struct char_data * ch, int room);
 
  arena_room = number(4557, 4560);
  if (FIGHTING(ch)){
	send_to_char("\n\rNo way you are fighting for you life!!", ch);
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



ACMD(do_quit)
{
  // Artus: This is dodgy ! :o)
  if (subcmd != SCMD_FREEZE)
  {
    send_to_char("Type 'quitreally' if you want to quit.\r\n",ch);
  } else {
    send_to_char("Type 'freeze' if you really want to freeze someone.\r\nDamn lazy immortals.\r\n", ch);
  }
}

ACMD(do_reallyquit)
{
  void Crash_rentsave(struct char_data * ch, int cost);
  extern int free_rent;
  extern int r_mortal_start_room;

  sh_int save_room;
  int i;
	/* int load_room = ENTRY_ROOM(ch,get_world(ch)); */
  struct obj_data *obj;

  if (IS_NPC(ch) || !ch->desc)
    return;

  if (subcmd != SCMD_QUIT && GET_LEVEL(ch) < LVL_IS_GOD)
    send_to_char("You have to type quit - no less, to quit!\r\n", ch);
  else if (GET_POS(ch) == POS_FIGHTING)
    send_to_char("No way!  You're fighting for your life!\r\n", ch);
  else if (GET_POS(ch) < POS_STUNNED) {
    send_to_char("You die before your time...\r\n", ch);
    die(ch,NULL);
  } 
  else 
	{
    sprintf(buf, "%s has quit the game.", GET_NAME(ch));
    if (free_rent || GET_LEVEL(ch) >=LVL_IS_GOD)
      Crash_rentsave(ch, 0);
    save_room = ch->in_room;

    // DM - reset save_room if it is same as mortal start room
    if (save_room == world[real_room(r_mortal_start_room)].number)
      ENTRY_ROOM(ch,1) = world[save_room].number;

    if(ROOM_FLAGGED(save_room,ROOM_HOUSE)) 
		{
      ENTRY_ROOM(ch,get_world(ch)) = world[save_room].number;
      if(!House_crashsave(world[save_room].number,FALSE))
			{
				return;
			}
      while(ch->carrying) 
			{
				obj = ch->carrying;
				obj_from_char(obj);
				obj_to_room(obj,save_room);
      }
      for(i=0; i<NUM_WEARS; i++)
				if(ch->equipment[i])
	  			obj_to_room(unequip_char(ch,i),save_room);

/* this is to make it check ya house when ya quit - Vader */
/*      if(!House_crashsave(load_room,FALSE))
        return;
*/    }
    if(!GET_INVIS_LEV(ch))
      act("$n has left the game.",TRUE,ch,0,0,TO_ROOM);
    mudlog(buf,NRM,MAX(LVL_ETRNL1,GET_INVIS_LEV(ch)),TRUE);
    info_channel ( buf , ch  ) ;    
    send_to_char("Goodbye, friend... Come back soon!\r\n",ch);
    extract_char(ch);
  }
}


ACMD(do_fastrent)
{
  void Crash_rentsave(struct char_data * ch, int cost);
  int Crash_offer_rent(struct char_data *ch,struct char_data *receptionist,
		       int display, int factor);
  int Crash_rent_deadline(struct char_data *ch,struct char_data *receptionist,
			  long cost);
  extern int rent_per_day;
  int cost=0;
  struct char_data *recep;
  int save_room=NOWHERE;
  
  if (IS_NPC(ch) || !ch->desc)
    return;

  if (GET_POS(ch) == POS_FIGHTING)
    send_to_char("No way!  You're fighting for your life!\r\n", ch);
  else if (GET_POS(ch) < POS_STUNNED) {
    send_to_char("You die before your time...\r\n", ch);
    die(ch,NULL);
  } 
  else {
    /* 5 = receptionist */
    recep = read_mobile(5, VIRTUAL);
 
    if (!(cost = Crash_offer_rent(ch, recep, FALSE,2)))
      return;
    sprintf(buf, "$n tells you, 'Rent will cost you %s%d%s gold coins %s.'",
        CCGOLD(ch,C_NRM),cost,CCNRM(ch,C_NRM),(rent_per_day ? " per day" : ""));
    act(buf, FALSE, recep, 0, ch, TO_VICT);
    if (cost > GET_GOLD(ch)) 
    {
      act("$n tells you, '...which I see you can't afford.'",
	  FALSE, recep, 0, ch, TO_VICT);
      return;
    }
    if (cost && (rent_per_day))
      Crash_rent_deadline(ch, recep, cost);
    
    act("$n stores your belongings and helps you into your private chamber.",
	  FALSE, recep, 0, ch, TO_VICT);
    Crash_rentsave(ch, cost);
    sprintf(buf, "%s has fastrented (%d%s, %d tot.)", GET_NAME(ch),
	      cost, (rent_per_day ? "/day" : ""), GET_GOLD(ch) + GET_BANK_GOLD(ch)); 

    mudlog(buf, NRM, MAX(LVL_ETRNL1, GET_INVIS_LEV(ch)), TRUE);
    act("$n helps $N into $S private chamber.", FALSE, recep, 0, ch, TO_NOTVICT);
    save_room = ch->in_room;
    extract_char(ch);
    ch->in_room = world[save_room].number;
    save_char(ch, ch->in_room);
  }
}



ACMD(do_save)
{
  if (IS_NPC(ch) || !ch->desc)
    return;

  if (cmd) {
    sprintf(buf, "Saving %s.\r\n", GET_NAME(ch));
    send_to_char(buf, ch);
  }
  save_char(ch, NOWHERE);
  Crash_crashsave(ch);
  if (ROOM_FLAGGED(ch->in_room, ROOM_HOUSE_CRASH))
    (void) House_crashsave(world[ch->in_room].number,TRUE);
}


/* generic function for commands which are normally overridden by
   special procedures - i.e., shop commands, mail commands, etc. */
ACMD(do_not_here)
{
  send_to_char("Sorry, but you cannot do that here!\r\n", ch);
}



ACMD(do_sneak)
{
  struct affected_type af;
  byte percent;


	if (!has_stats_for_skill(ch, SKILL_SNEAK))
		return;

  send_to_char("Okay, you'll try to move silently for a while.\r\n", ch);
  if (IS_AFFECTED(ch, AFF_SNEAK))
    affect_from_char(ch, SKILL_SNEAK);

  percent = number(1, 101);     /* 101% is a complete failure */

  if (percent > GET_SKILL(ch, SKILL_SNEAK) + dex_app_skill[GET_AFF_DEX(ch)].sneak)
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

  send_to_char("You attempt to hide yourself.\r\n", ch);

  if (IS_AFFECTED(ch, AFF_HIDE))
    REMOVE_BIT(AFF_FLAGS(ch), AFF_HIDE);

  percent = number(1, 101);     /* 101% is a complete failure */

  if (percent > GET_SKILL(ch, SKILL_HIDE) + dex_app_skill[GET_REAL_DEX(ch)].hide)
    return;

  SET_BIT(AFF_FLAGS(ch), AFF_HIDE);
}




ACMD(do_steal)
{
  struct char_data *vict;
  struct obj_data *obj;
  char vict_name[240];
  char obj_name[240];
  int percent, gold, eq_pos, pcsteal = 0;
  extern int pt_allowed;
  extern struct zone_data *zone_table;
  bool ohoh = FALSE;

  ACMD(do_gen_comm);

  argument = one_argument(argument, obj_name);
  one_argument(argument, vict_name);

	if (!has_stats_for_skill(ch, SKILL_STEAL))
		return;

  if (!(vict = get_char_room_vis(ch, vict_name))) {
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

    /*
     * We'll try something different... instead of having a thief flag, just
     * have PC Steals fail all the time.
     */
  }
  /* 101% is a complete failure */
  percent = number(1, 101) - dex_app_skill[GET_AFF_DEX(ch)].p_pocket;

  if (GET_POS(vict) < POS_SLEEPING)
    percent = -1;               /* ALWAYS SUCCESS */

  /* NO NO With Imp's and Shopkeepers! */
  if ((GET_LEVEL(vict) >= LVL_IS_GOD) || pcsteal ||
      GET_MOB_SPEC(vict) == shop_keeper)
    percent = 101;              /* Failure */

  if (!IS_NPC(vict)) /* always fail on another player character */
    percent=101;
  if (GET_LEVEL(ch)==LVL_IMPL) /* implementors always succeed! /bm/ */
    percent=-100;
  if (str_cmp(obj_name, "coins") && str_cmp(obj_name, "gold")) {

    if (!(obj = get_obj_in_list_vis(vict, obj_name, vict->carrying))) {

      for (eq_pos = 0; eq_pos < NUM_WEARS; eq_pos++)
	if (vict->equipment[eq_pos] &&
	    (isname(obj_name, vict->equipment[eq_pos]->name)) &&
	    CAN_SEE_OBJ(ch, vict->equipment[eq_pos])) {
	  obj = vict->equipment[eq_pos];
	  break;
	}
      if (!obj) {
	act("$E hasn't got that item.", FALSE, ch, 0, vict, TO_CHAR);
	return;
      } else {                  /* It is equipment */
	if ((GET_POS(vict) > POS_STUNNED) && (GET_LEVEL(ch)<LVL_IMPL)) {
	  send_to_char("Steal the equipment now?  Impossible!\r\n", ch);
	  return;
	} else {
	  act("You unequip $p and steal it.", FALSE, ch, obj, 0, TO_CHAR);
	  act("$n steals $p from $N.", FALSE, ch, obj, vict, TO_NOTVICT);
	  obj_to_char(unequip_char(vict, eq_pos), ch);
	}
      }
    } else {                    /* obj found in inventory */

      percent += GET_OBJ_WEIGHT(obj);   /* Make heavy harder */

      if (AWAKE(vict) && (percent > GET_SKILL(ch, SKILL_STEAL))) {
	ohoh = TRUE;
	act("Oops..", FALSE, ch, 0, 0, TO_CHAR);
	act("$n tried to steal something from you!", FALSE, ch, 0, vict, TO_VICT);
	act("$n tries to steal something from $N.", TRUE, ch, 0, vict, TO_NOTVICT);
      } else {                  /* Steal the item */
	if ((IS_CARRYING_N(ch) + 1 < CAN_CARRY_N(ch))) {
	  if ((IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj)) < CAN_CARRY_W(ch)) {
	    obj_from_char(obj);
	    obj_to_char(obj, ch);
	    send_to_char("Got it!\r\n", ch);
	  }
	} else
	  send_to_char("You cannot carry that much.\r\n", ch);
      }
    }
  } else {                      /* Steal some coins */
    if (AWAKE(vict) && (percent > GET_SKILL(ch, SKILL_STEAL))) {
      ohoh = TRUE;
      act("Oops..", FALSE, ch, 0, 0, TO_CHAR);
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
	sprintf(buf, "Bingo!  You got %s%d%s gold coins.\r\n",
		CCGOLD(ch,C_NRM), gold, CCNRM(ch,C_NRM));
	send_to_char(buf, ch);
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
  void list_skills(struct char_data * ch);

  one_argument(argument, arg);

  if (*arg)
    send_to_char("You can only practice skills in your guild.\r\n", ch);
  else
    list_skills(ch);
}



ACMD(do_visible)
{
  void appear(struct char_data * ch);

  if IS_AFFECTED
    (ch, AFF_INVISIBLE) {
    appear(ch);
    send_to_char("You break the spell of invisibility.\r\n", ch);
  } else
    send_to_char("You are already visible.\r\n", ch);
}

int title_no_good(const char *title)
{
	char tmp[4];
	char *cp;

	strncpy(tmp, title, 3);
	cp = tmp;
	while (*cp)
	{
		*cp = toupper(*cp);
		cp++;
	}

	if (strcmp(tmp, "THE"))
		return(1);

	return(0);
}

ACMD(do_title)
{
  char tmp_title[MAX_TITLE_LENGTH];

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
  else if (strlen(argument) > MAX_TITLE_LENGTH-4) {
    sprintf(buf, "Sorry, titles can't be longer than %d characters.\r\n",
	    MAX_TITLE_LENGTH);
    send_to_char(buf, ch);
  } else {
/*    strcpy(tmp_title, "the "); */
		strcpy(tmp_title, "");
    strcat(tmp_title, argument);
    strcat(tmp_title,"&n");
    set_title(ch, tmp_title);
    sprintf(buf, "Okay, you're now %s %s.\r\n", GET_NAME(ch), GET_TITLE(ch));
    send_to_char(buf, ch);
  }
}


ACMD(do_group)
{
  struct char_data *vict, *k;
  struct follow_type *f;
  bool found;

  one_argument(argument, buf);

  if (!*buf) {
    if (!IS_AFFECTED(ch, AFF_GROUP)) {
      send_to_char("But you are not the member of a group!\r\n", ch);
    } else {
      send_to_char("Your group consists of:\r\n", ch);

      k = (ch->master ? ch->master : ch);

      if (IS_AFFECTED(k, AFF_GROUP)) {
	sprintf(buf, "     [%3dH %3dM %3dV] [%2d %s] $N (Head of group)",
	 GET_HIT(k), GET_MANA(k), GET_MOVE(k), GET_LEVEL(k), CLASS_ABBR(k));
	act(buf, FALSE, ch, 0, k, TO_CHAR);
      }
      for (f = k->followers; f; f = f->next)
	if (IS_AFFECTED(f->follower, AFF_GROUP)) {
	  sprintf(buf, "     [%3dH %3dM %3dV] [%2d %s] $N",
		  GET_HIT(f->follower), GET_MANA(f->follower),
		  GET_MOVE(f->follower), GET_LEVEL(f->follower),
		  CLASS_ABBR(f->follower));
	  act(buf, FALSE, ch, 0, f->follower, TO_CHAR);
	}
    }

    return;
  }
  if (ch->master) {
    act("You can not enroll group members without being head of a group.",
	FALSE, ch, 0, 0, TO_CHAR);
    return;
  }
  if (!str_cmp(buf, "all")) {
    found = FALSE;
    SET_BIT(AFF_FLAGS(ch), AFF_GROUP);
    for (f = ch->followers; f; f = f->next) {
      vict = f->follower;
      if (!IS_AFFECTED(vict, AFF_GROUP)) {
	found = TRUE;
	if (ch != vict)
	  act("$N is now a member of your group.", FALSE, ch, 0, vict, TO_CHAR);
	act("You are now a member of $n's group.", FALSE, ch, 0, vict, TO_VICT);
	act("$N is now a member of $n's group.", FALSE, ch, 0, vict, TO_NOTVICT);
	SET_BIT(AFF_FLAGS(vict), AFF_GROUP);
      }
    }

    if (!found)
      send_to_char("Everyone following you is already in your group.\r\n", ch);

    return;
  }
  if (!(vict = get_char_room_vis(ch, buf))) {
    send_to_char(NOPERSON, ch);
  } else {
    found = FALSE;

    if (vict == ch)
      found = TRUE;
    else {
      for (f = ch->followers; f; f = f->next) {
	if (f->follower == vict) {
	  found = TRUE;
	  break;
	}
      }
    }

    if (found) {
      if (IS_AFFECTED(vict, AFF_GROUP)) {
	if (ch != vict)
	  act("$N is no longer a member of your group.", FALSE, ch, 0, vict, TO_CHAR);
	act("You have been kicked out of $n's group!", FALSE, ch, 0, vict, TO_VICT);
	act("$N has been kicked out of $n's group!", FALSE, ch, 0, vict, TO_NOTVICT);
	REMOVE_BIT(AFF_FLAGS(vict), AFF_GROUP);
      } else {
	if (ch != vict)
	  act("$N is now a member of your group.", FALSE, ch, 0, vict, TO_CHAR);
	act("You are now a member of $n's group.", FALSE, ch, 0, vict, TO_VICT);
	act("$N is now a member of $n's group.", FALSE, ch, 0, vict, TO_NOTVICT);
	SET_BIT(AFF_FLAGS(vict), AFF_GROUP);
      }
    } else
      act("$N must follow you to enter your group.", FALSE, ch, 0, vict, TO_CHAR);
  }
}


ACMD(do_ungroup)
{
  struct follow_type *f, *next_fol;
  struct char_data *tch;
  void stop_follower(struct char_data * ch);

  one_argument(argument, buf);

  if (!*buf) {
    if (ch->master || !(IS_AFFECTED(ch, AFF_GROUP))) {
      send_to_char("But you lead no group!\r\n", ch);
      return;
    }
    sprintf(buf2, "%s has disbanded the group.\r\n", GET_NAME(ch));
    for (f = ch->followers; f; f = next_fol) {
      next_fol = f->next;
      if (IS_AFFECTED(f->follower, AFF_GROUP)) {
	REMOVE_BIT(AFF_FLAGS(f->follower), AFF_GROUP);
        /* DM - clones always follow master */
        if (!IS_CLONE(f->follower)) {
	  send_to_char(buf2, f->follower);
	  stop_follower(f->follower);
        }
      }
    }

    send_to_char("You have disbanded the group.\r\n", ch);
    REMOVE_BIT(AFF_FLAGS(ch), AFF_GROUP);
    return;
  }
  if (!(tch = get_char_room_vis(ch, buf))) {
    send_to_char("There is no such person!\r\n", ch);
    return;
  }
  if (tch->master != ch) {
    send_to_char("That person is not following you!\r\n", ch);
    return;
  }
  if (IS_AFFECTED(tch, AFF_GROUP))
    REMOVE_BIT(AFF_FLAGS(tch), AFF_GROUP);

  act("$N is no longer a member of your group.", FALSE, ch, 0, tch, TO_CHAR);
  act("You have been kicked out of $n's group!", FALSE, ch, 0, tch, TO_VICT);
  act("$N has been kicked out of $n's group!", FALSE, ch, 0, tch, TO_NOTVICT);
  stop_follower(tch);
}




ACMD(do_report)
{
  struct char_data *k;
  struct follow_type *f;

  if (!IS_AFFECTED(ch, AFF_GROUP)) {
    send_to_char("But you are not a member of any group!\r\n", ch);
    return;
  }
  sprintf(buf, "%s reports: %d/%dH, %d/%dM, %d/%dV\r\n",
	  GET_NAME(ch), GET_HIT(ch), GET_MAX_HIT(ch),
	  GET_MANA(ch), GET_MAX_MANA(ch),
	  GET_MOVE(ch), GET_MAX_MOVE(ch));

  CAP(buf);

  k = (ch->master ? ch->master : ch);

  for (f = k->followers; f; f = f->next)
    if (IS_AFFECTED(f->follower, AFF_GROUP) && f->follower != ch)
      send_to_char(buf, f->follower);
  if (k != ch)
    send_to_char(buf, k);
  send_to_char("You report to the group.\r\n", ch);
}



ACMD(do_split)
{
  int amount, num, share;
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

    if (IS_AFFECTED(k, AFF_GROUP) && (k->in_room == ch->in_room))
      num = 1;
    else
      num = 0;

    for (f = k->followers; f; f = f->next)
      if (IS_AFFECTED(f->follower, AFF_GROUP) &&
	  (!IS_NPC(f->follower)) &&
	  (f->follower->in_room == ch->in_room))
	num++;

    if (num && IS_AFFECTED(ch, AFF_GROUP))
      share = amount / num;
    else {
      send_to_char("With whom do you wish to share your gold?\r\n", ch);
      return;
    }

    GET_GOLD(ch) -= share * (num - 1);

    if (IS_AFFECTED(k, AFF_GROUP) && (k->in_room == ch->in_room)
	&& !(IS_NPC(k)) && k != ch) {
      GET_GOLD(k) += share;
      sprintf(buf, "%s splits %s%d%s coins; you receive %s%d%s.\r\n", GET_NAME(ch),
	      CCGOLD(k,C_NRM),amount,CCNRM(k,C_NRM),CCGOLD(k,C_NRM),
	      share,CCNRM(k,C_NRM));
      send_to_char(buf, k);
    }
    for (f = k->followers; f; f = f->next) {
      if (IS_AFFECTED(f->follower, AFF_GROUP) &&
	  (!IS_NPC(f->follower)) &&
	  (f->follower->in_room == ch->in_room) &&
	  f->follower != ch) {
	GET_GOLD(f->follower) += share;
	sprintf(buf, "%s splits %s%d%s coins; you receive %s%d%s.\r\n", 
		GET_NAME(ch),CCGOLD(f->follower,C_NRM),amount,CCNRM(f->follower,C_NRM),
		CCGOLD(f->follower,C_NRM), share,CCNRM(f->follower,C_NRM));
	send_to_char(buf, f->follower);
      }
    }
    sprintf(buf, "You split %s%d%s coins among %d members -- %s%d%s coins each.\r\n", 
	CCGOLD(ch,C_NRM),amount,CCNRM(ch,C_NRM), num, 
	CCGOLD(ch,C_NRM),share,CCNRM(ch,C_NRM));
    send_to_char(buf, ch);
  } else {
    send_to_char("How many coins do you wish to split with your group?\r\n", ch);
    return;
  }
}



ACMD(do_use)
{
  struct obj_data *mag_item;
  int equipped = 1;

  half_chop(argument, arg, buf);
  if (!*arg) {
    sprintf(buf2, "What do you want to %s?\r\n", CMD_NAME);
    send_to_char(buf2, ch);
    return;
  }
  mag_item = ch->equipment[WEAR_HOLD];

  if (!mag_item || !isname(arg, mag_item->name)) {
    switch (subcmd) {
    case SCMD_RECITE:
    case SCMD_QUAFF:
      equipped = 0;
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
      break;
    default:
      log("SYSERR: Unknown subcmd passed to do_use");
      return;
      break;
    }
  }
  switch (subcmd) {
  case SCMD_QUAFF:
    if (GET_OBJ_TYPE(mag_item) != ITEM_POTION) {
      send_to_char("You can only quaff potions.", ch);
      return;
    }
    break;
  case SCMD_RECITE:
    if (GET_OBJ_TYPE(mag_item) != ITEM_SCROLL) {
      send_to_char("You can only recite scrolls.", ch);
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
    if ((wimp_lev = atoi(arg))) {
      if (wimp_lev < 0)
	send_to_char("Heh, heh, heh.. we are jolly funny today, eh?\r\n", ch);
      else if (wimp_lev > GET_MAX_HIT(ch))
	send_to_char("That doesn't make much sense, now does it?\r\n", ch);
      else if (wimp_lev > (GET_MAX_HIT(ch) >> 1))
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

  return;

}


ACMD(do_display)
{
  int i;

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
  char *tmp, *filename;
  struct stat fbuf;
  extern int max_filesize;
  long ct;

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
  mudlog(buf, CMP, LVL_ETRNL1, FALSE);

  if (stat(filename, &fbuf) < 0) {
    perror("Error statting file");
    return;
  }
  if (fbuf.st_size >= max_filesize) {
    send_to_char("Sorry, the file is full right now.. try again later.\r\n", ch);
    return;
  }
  if (!(fl = fopen(filename, "a"))) {
    perror("do_gen_write");
    send_to_char("Could not open the file.  Sorry.\r\n", ch);
    return;
  }
  fprintf(fl, "%-8s (%6.6s) [%5d] %s\n", GET_NAME(ch), (tmp + 4),
	  world[ch->in_room].number, argument);
  fclose(fl);
  send_to_char("Okay.  Thanks!\r\n", ch);
}



#define TOG_OFF 0
#define TOG_ON  1

#define PRF_TOG_CHK(ch,flag) ((TOGGLE_BIT(PRF_FLAGS(ch), (flag))) & (flag))

ACMD(do_gen_tog)
{
  long result;
  extern int nameserver_is_slow;

  char *tog_messages[][2] = {
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
    "You will now autosplit gold when grouped.\r\n"} 
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
 case SCMD_NOIMMNET:
    result = PRF_TOG_CHK(ch, PRF_NOIMMNET);
    break;
  case SCMD_QUEST:
    /*
    result = PRF_TOG_CHK(ch, PRF_QUEST);
    */
    send_to_char("Please tell current QuestMaster you want in on the quest.\r\n", ch);
    return;
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

 default:
    log("SYSERR: Unknown subcmd in do_gen_toggle");
    return;
    break;
  }

  if (result)
    send_to_char(tog_messages[subcmd][TOG_ON], ch);
  else
    send_to_char(tog_messages[subcmd][TOG_OFF], ch);

  return;
}

/* This is the ignore cammand - Hal */

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

/* DM - modified spy skill written by Eric V. Bahmer */
ACMD(do_spy)
{
  int percent, prob, spy_type, return_room;
  char direction[MAX_INPUT_LENGTH];
  extern char *dirs[];

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
              if (ROOM_FLAGGED(world[ch->in_room].dir_option[spy_type]->to_room, ROOM_HOUSE | ROOM_PRIVATE)) {
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
/***************************************************************************/
/* Clan functions -Hal     - reformatted DM                                */
/***************************************************************************/

ACMD(do_signup)
{

  if (GET_LEVEL(ch) < 10) {
    send_to_char("You can't join a clan till level 10!\r\n", ch);
    return;
  }

  if (GET_CLAN_NUM(ch) > 0) {
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

  if (GET_CLAN_NUM(vict) > 0) {
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
  sprintf(buf, "%s is now a member of the %s.\r\n", GET_NAME(vict), 
      get_clan_disp(vict));
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
      sprintf(buf, "%s has been striped of %s knighthood.\r\n", GET_NAME(vict),
          HSHR(vict));
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
    sprintf(buf, "You have been banished from the %s.\r\n", 
        get_clan_disp(vict));
    send_to_char(buf, vict);
    sprintf(buf, "%s has been banished from the %s.\r\n", GET_NAME(vict), 
        get_clan_disp(vict));   
    GET_CLAN_NUM(vict) = 0;
   
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
      GET_CLAN_LEV(vict) -- ;
      sprintf(buf, "You have been demoted to the rank of %s.\r\n", 
          get_clan_rank(vict));
      send_to_char(buf, vict);

      sprintf(buf, "%s has been demoted to the rank of %s.\r\n", GET_NAME(vict),
          get_clan_rank(vict));
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
   
      sprintf(buf, "You have been promoted to the rank of %s.\r\n", 
          get_clan_rank(vict));
      send_to_char(buf, vict);

      sprintf(buf, "%s has been promoted to the rank of %s.\r\n",GET_NAME(vict),
          get_clan_rank(vict));
      send_to_char(buf, ch);
    } else {
      sprintf(buf, "%s has aready reached the highest rank.\r\n", 
          GET_NAME(vict));
      send_to_char(buf, ch);
    }
  }
}
