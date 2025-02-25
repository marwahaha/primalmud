/* ************************************************************************
*   File: act.comm.c                                    Part of CircleMUD *
*  Usage: Player-level communication commands                             *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

/* modified by Brett Murphy to log certain players comms */

#include <stdio.h>
#include <string.h>

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "screen.h"

#define WATCHPLAYER1 3764 
#define WATCHPLAYER2 0 /* Indiana */

/* extern variables */
extern struct room_data *world;
extern struct descriptor_data *descriptor_list;
extern struct char_data *character_list;

/* function prototypes */
int Valid_Name(char *newname);

int scan_buffer_for_xword(char* buf)
{
  char tmpword[MAX_INPUT_LENGTH+65];
  int i,count=-1;
  for (i=0;i<=strlen(buf);i++)
  {
    count++;
    tmpword[count]=buf[i];
    if (tmpword[count]==' ' || tmpword[i]=='\n' || i==strlen(buf))
    {
      tmpword[count]='\0';
      count=-1;
      if (!Valid_Name(tmpword))
        return 1;
    }
  }
  return 0;
}


/* modified do_say to affect speech when drunk.. - Vader */                     
#define DRUNKNESS    ch->player_specials->saved.conditions[DRUNK]
#define PISS_FACTOR  (25 - DRUNKNESS) * 50
#define BASE_SECT(n) ((n) & 0x000f)


ACMD(do_say)
{
/* char s[800];*/ /* zap */
  char *speech;
  int i,j;

  skip_spaces(&argument);

  if (!*argument)
    send_to_char("Yes, but WHAT do you want to say?\r\n", ch);
  else {
    speech = str_dup(argument);                                                 
/* this loop goes thru and randomly drops letters from what was said
 * depanding on how drunk the person is.. 24 is max drunk.. when someone
 * is 24 drunk they pretty much cant be understood.. - Vader
 */
    for(i = 0,j = 0; i < strlen(speech); i++) {
      if(number(1,PISS_FACTOR) > DRUNKNESS) {
        speech[j] = speech[i];
        j++;
        }
      }
      speech[j] = '\0';

      if(!IS_NPC(ch) && ((BASE_SECT(world[ch->in_room].sector_type) == SECT_UNDERWATER))){ 
        send_to_char("Bubbles raise from your mouth as you speak!\r\n",ch);
        act("Bubbles raise from $n's mouth as $e speaks.", TRUE, ch, 0, 0, TO_ROOM);
      } 
    sprintf(buf, "&n$n says, '%s&n'", speech);
    act(buf, FALSE, ch, 0, 0, TO_ROOM);
    if (PRF_FLAGGED(ch, PRF_NOREPEAT))
      send_to_char(OK, ch);
    else {
      sprintf(buf, "&nYou say, '%s&n'", speech);
      act(buf, FALSE, ch, 0, argument, TO_CHAR);
    }
/*
  if (scan_buffer_for_xword(argument))
  {  
    sprintf(s,"WATCHLOG SWEAR: %s said '%s'&n",ch->player.name,argument);
    mudlog(s,NRM,LVL_IMPL,TRUE);
    send_to_char("Please dont swear where other players might hear.\r\n",ch);
return;
 }
*/  
  /* temporary hack to log say from certain players   bm 2/95 
    currently set for ANGMAR */
/*  if (GET_IDNUM(ch)==WATCHPLAYER2)
  {
    sprintf(s,"WATCHLOG PLAYER: %s said '%s'",ch->player.name,argument);
    mudlog(s,NRM,LVL_IMPL,TRUE);
  }
*/
  }
}


ACMD(do_gsay)
{
  struct char_data *k;
  struct follow_type *f;

  skip_spaces(&argument);

  if (!IS_AFFECTED(ch, AFF_GROUP)) {
    send_to_char("But you are not the member of a group!\r\n", ch);
    return;
  }
  if (!*argument)
    send_to_char("Yes, but WHAT do you want to group-say?\r\n", ch);
  else {
    if (ch->master)
      k = ch->master;
    else
      k = ch;

    sprintf(buf, "&R$n &rtells the group, '%s&r'&n", argument);

    if (IS_AFFECTED(k, AFF_GROUP) && (k != ch))
      act(buf, FALSE, ch, 0, k, TO_VICT | TO_SLEEP);
    for (f = k->followers; f; f = f->next)
      if (IS_AFFECTED(f->follower, AFF_GROUP) && (f->follower != ch))
	act(buf, FALSE, ch, 0, f->follower, TO_VICT | TO_SLEEP);

    if (PRF_FLAGGED(ch, PRF_NOREPEAT))
      send_to_char(OK, ch);
    else {
      sprintf(buf, "&rYou tell the group, '%s&r'&n", argument);
      act(buf, FALSE, ch, 0, 0, TO_CHAR | TO_SLEEP);
    }
  }
}


void perform_tell(struct char_data *ch, struct char_data *vict, char *arg)
{
/* char s[800];*/  /* zap */
//  send_to_char(CCRED(vict, C_NRM), vict);
  sprintf(buf, "&R$n &rtells you, '%s&r'&n", arg);
  act(buf, FALSE, ch, 0, vict, TO_VICT | TO_SLEEP);
//  send_to_char(CCNRM(vict, C_NRM), vict);

  if (PRF_FLAGGED(ch, PRF_NOREPEAT))
    send_to_char(OK, ch);
  else {
//    send_to_char(CCRED(ch, C_CMP), ch);
    sprintf(buf, "&rYou tell &R$N&r, '%s&r'&n", arg);
    act(buf, FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
//    send_to_char(CCNRM(ch, C_CMP), ch);
  }
/*
  if (scan_buffer_for_xword(arg))
  {  
    sprintf(s,"WATCHLOG SWEAR: %s told %s '%s'",ch->player.name,vict->player.name,arg);
    mudlog(s,NRM,LVL_IMPL,TRUE);
    send_to_char("Please dont swear at other players.\r\n",ch);
return;
  }
*/  
  /* temporary hack to log tells from certain players   bm 2/95 */
/*  if (GET_IDNUM(ch)==WATCHPLAYER2)
  {
    sprintf(s,"WATCHLOG PLAYER: %s told %s '%s'",ch->player.name,vict->player.name,arg);
    mudlog(s,NRM,LVL_IMPL,TRUE); 
  }
*/
  GET_LAST_TELL(vict) = ch;
}

/*
 * Yes, do_tell probably could be combined with whisper and ask, but
 * called frequently, and should IMHO be kept as tight as possible.
 */

ACMD(do_tell)
{
  struct char_data *vict;

  half_chop(argument, buf, buf2);

  if (!*buf || !*buf2)
    send_to_char("Who do you wish to tell what??\r\n", ch);
  else if (!(vict = get_char_vis(ch, buf,TRUE)))
    send_to_char(NOPERSON, ch);
  else if (ch == vict)
    send_to_char("You try to tell yourself something.\r\n", ch);
  else if (PRF_FLAGGED(ch, PRF_NOTELL))
    send_to_char("You can't tell other people while you have notell on.\r\n", ch);
  else if (!IS_NPC(vict) && !vict->desc)	/* linkless */
    act("$E's linkless at the moment.", FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
  else if (PLR_FLAGGED(vict, PLR_WRITING))
    act("$E's writing a message right now; try again later.",
	FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
  else if (PRF_FLAGGED(vict, PRF_NOTELL))
        act("$E's deaf to your tells; try again later.",
        FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
  else if (PRF_FLAGGED(vict, PRF_AFK))
        act("$E's is marked (AFK); try again later.",
        FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
  else if ( ( GET_IGN1(vict) == GET_IDNUM(ch) ||
             GET_IGN2(vict) == GET_IDNUM(ch) || 
             GET_IGN3(vict) == GET_IDNUM(ch) ||
             GET_IGN_LEVEL(vict) >= GET_LEVEL(ch)) && 
             !IS_NPC(ch) )
            act("$E's ignoring you.", FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP );
  else if (PRF_FLAGGED(ch, PRF_AFK))
    send_to_char("It is unfair to tell people things while marked AFK.  They can't reply!!.\r\n", ch);
  else
    perform_tell(ch, vict, buf2);
}


ACMD(do_reply)
{
  struct char_data *tch = character_list;

  skip_spaces(&argument);

  if (GET_LAST_TELL(ch) == NULL)
    send_to_char("You have no-one to reply to!\r\n", ch);
  else if (!*argument)
    send_to_char("What is your reply?\r\n", ch);
  else {
    /* Make sure the person you're replying to is still playing by searching
     * for them.  Note, this will break in a big way if I ever implement some
     * scheme where it keeps a pool of char_data structures for reuse.
     */
				     
    while (tch != NULL && tch != GET_LAST_TELL(ch))
      tch = tch->next;

    if (tch == NULL)
      send_to_char("They are no longer playing.\r\n", ch);
    else 
      perform_tell(ch, GET_LAST_TELL(ch), argument);
  }
}


ACMD(do_spec_comm)
{
  struct char_data *vict;
  char *action_sing, *action_plur, *action_others;

  if (subcmd == SCMD_WHISPER) {
    action_sing = "whisper to";
    action_plur = "whispers to";
    action_others = "$n whispers something to $N.";
  } else {
    action_sing = "ask";
    action_plur = "asks";
    action_others = "$n asks $N a question.";
  }

  half_chop(argument, buf, buf2);

  if (!*buf || !*buf2) {
    sprintf(buf, "Whom do you want to %s.. and what??\r\n", action_sing);
    send_to_char(buf, ch);
  } else if (!(vict = get_char_room_vis(ch, buf)))
    send_to_char(NOPERSON, ch);
  else if (vict == ch)
    send_to_char("You can't get your mouth close enough to your ear...\r\n", ch);
  else {
    sprintf(buf, "&C$n &c%s you, '%s&c'&n", action_plur, buf2);
    act(buf, FALSE, ch, 0, vict, TO_VICT);
    if (PRF_FLAGGED(ch, PRF_NOREPEAT))
      send_to_char(OK, ch);
    else {
      sprintf(buf, "&cYou %s &C%s&c, '%s&c'&n\r\n", action_sing, GET_NAME(vict), buf2);
      act(buf, FALSE, ch, 0, 0, TO_CHAR);
    }
    act(action_others, FALSE, ch, 0, vict, TO_NOTVICT);
  }
}



#define MAX_NOTE_LENGTH 1000	/* arbitrary */

ACMD(do_write)
{
  struct obj_data *paper = 0, *pen = 0;
  char *papername, *penname;

  papername = buf1;
  penname = buf2;

  two_arguments(argument, papername, penname);

  if (!ch->desc)
    return;

  if (!*papername) {		/* nothing was delivered */
    send_to_char("Write?  With what?  ON what?  What are you trying to do?!?\r\n", ch);
    return;
  }
  if (*penname) {		/* there were two arguments */
    if (!(paper = get_obj_in_list_vis(ch, papername, ch->carrying))) {
      sprintf(buf, "You have no %s.\r\n", papername);
      send_to_char(buf, ch);
      return;
    }
    if (!(pen = get_obj_in_list_vis(ch, penname, ch->carrying))) {
      sprintf(buf, "You have no %s.\r\n", papername);
      send_to_char(buf, ch);
      return;
    }
  } else {			/* there was one arg.. let's see what we can
				 * find */
    if (!(paper = get_obj_in_list_vis(ch, papername, ch->carrying))) {
      sprintf(buf, "There is no %s in your inventory.\r\n", papername);
      send_to_char(buf, ch);
      return;
    }
    if (GET_OBJ_TYPE(paper) == ITEM_PEN) {	/* oops, a pen.. */
      pen = paper;
      paper = 0;
    } else if (GET_OBJ_TYPE(paper) != ITEM_NOTE) {
      send_to_char("That thing has nothing to do with writing.\r\n", ch);
      return;
    }
    /* One object was found.. now for the other one. */
    if (!ch->equipment[WEAR_HOLD]) {
      sprintf(buf, "You can't write with %s %s alone.\r\n", AN(papername),
	      papername);
      send_to_char(buf, ch);
      return;
    }
    if (!CAN_SEE_OBJ(ch, ch->equipment[WEAR_HOLD])) {
      send_to_char("The stuff in your hand is invisible!  Yeech!!\r\n", ch);
      return;
    }
    if (pen)
      paper = ch->equipment[WEAR_HOLD];
    else
      pen = ch->equipment[WEAR_HOLD];
  }


  /* ok.. now let's see what kind of stuff we've found */
  if (GET_OBJ_TYPE(pen) != ITEM_PEN) {
    act("$p is no good for writing with.", FALSE, ch, pen, 0, TO_CHAR);
  } else if (GET_OBJ_TYPE(paper) != ITEM_NOTE) {
    act("You can't write on $p.", FALSE, ch, paper, 0, TO_CHAR);
  } else if (paper->action_description)
    send_to_char("There's something written on it already.\r\n", ch);
  else {
    /* we can write - hooray! */
     /* this is the PERFECT code example of how to set up:
      * a) the text editor with a message already loaed
      * b) the abort buffer if the player aborts the message
      */
     ch->desc->backstr = NULL;
     send_to_char("Write your note.  (/s saves /h for help)\r\n", ch);
     /* ok, here we check for a message ALREADY on the paper */
     if (paper->action_description) {
      /* we str_dup the original text to the descriptors->backstr */
      ch->desc->backstr = str_dup(paper->action_description);
      /* send to the player what was on the paper (cause this is already */
      /* loaded into the editor) */
      send_to_char(paper->action_description, ch);
     }
    act("$n begins to jot down a note.", TRUE, ch, 0, 0, TO_ROOM);
     /* assign the descriptor's->str the value of the pointer to the text */
     /* pointer so that we can reallocate as needed (hopefully that made */
     /* sense :>) */ 
    ch->desc->str = &paper->action_description;
    ch->desc->max_str = MAX_NOTE_LENGTH;
  }
}



ACMD(do_page)
{
  struct descriptor_data *d;
  struct char_data *vict;

  half_chop(argument, arg, buf2);

  if (IS_NPC(ch))
    send_to_char("Monsters can't page.. go away.\r\n", ch);
  else if (!*arg)
    send_to_char("Whom do you wish to page?\r\n", ch);
  else {
    sprintf(buf, "\007\007*%s* %s\r\n", GET_NAME(ch), buf2);
    if (!str_cmp(arg, "all")) {
      if (GET_LEVEL(ch) > LVL_GOD) {
	for (d = descriptor_list; d; d = d->next)
	  if (!d->connected && d->character)
	    act(buf, FALSE, ch, 0, d->character, TO_VICT);
      } else
	send_to_char("You will never be godly enough to do that!\r\n", ch);
      return;
    }
    if ((vict = get_char_vis(ch, arg,TRUE)) != NULL) {
      act(buf, FALSE, ch, 0, vict, TO_VICT);
      if (PRF_FLAGGED(ch, PRF_NOREPEAT))
	send_to_char(OK, ch);
      else
	act(buf, FALSE, ch, 0, vict, TO_CHAR);
      return;
    } else
      send_to_char("There is no such person in the game!\r\n", ch);
  }
}


/**********************************************************************
 * generalized communication func, originally by Fred C. Merkel (Torg) *
  *********************************************************************/

ACMD(do_gen_comm)
{
  extern int level_can_shout;
  extern int holler_move_cost;
  struct descriptor_data *i;
  char color_on[24]; 
  char s[800];/* zap */ 
  char *speech;
  int l,j;

  extern int same_world(struct char_data *ch,struct char_data *ch2);

  /* Array of flags which must _not_ be set in order for comm to be heard */
  static int channels[] = {
    0,
    PRF_DEAF,
    PRF_NOGOSS,
    PRF_NOAUCT,
    PRF_NOGRATZ,
    EXT_NONEWBIE,
    EXT_NOCTALK,
    0
  };

  /*
   * com_msgs: [0] Message if you can't perform the action because of noshout
   * [1] name of the action [2] message if you're not on the channel [3] a
   * color string.
   */
  static char *com_msgs[][4] = {
    {"You cannot holler!!\r\n",
      "holler",
      "",
    KYEL},

    {"You cannot shout!!\r\n",
      "shout",
      "Turn off your noshout flag first!\r\n",
    KYEL},

    {"You cannot gossip!!\r\n",
      "gossip",
      "You aren't even on the channel!\r\n",
    KYEL},

    {"You cannot auction!!\r\n",
      "auction",
      "You aren't even on the channel!\r\n",
    KMAG},

    {"You cannot congratulate!\r\n",
      "congrat",
      "You aren't even on the channel!\r\n",
    KGRN},

    {"You cannot use the newbie channel!\r\n",
      "newbie",      
      "You aren't even on the channel!\r\n",    
    KCYN},

    {"You cannot use clan talk!\r\n",
      "clan talk",
      "You aren't even on the channel!\r\n",
    KWHT}  



  };

  /* to keep pets, etc from being ordered to shout */
  /* changed so mobs can shout and holler in spec procs - Vader */
  if (!ch->desc && subcmd != SCMD_SHOUT && subcmd != SCMD_HOLLER)
    return;

  if (!IS_NPC(ch) && PLR_FLAGGED(ch, PLR_NOSHOUT)) {
    send_to_char(com_msgs[subcmd][0], ch);
    return;
  }
  if (ROOM_FLAGGED(ch->in_room, ROOM_SOUNDPROOF)) {
    send_to_char("The walls seem to absorb your words.\r\n", ch);
    return;
  }
  /* level_can_shout defined in config.c */
  
   if ((subcmd != SCMD_NEWBIE) && GET_LEVEL(ch) < level_can_shout) {
     sprintf(buf1, "You must be at least level %d before you can %s.\r\n",
	    level_can_shout, com_msgs[subcmd][1]);
     send_to_char(buf1, ch);
     return;
   }

  /* make sure the char is on the channel */
  /* check the PRF flags - Hal */
  if (PRF_FLAGGED(ch, channels[subcmd]) && subcmd <= SCMD_GRATZ )
  {
    send_to_char(com_msgs[subcmd][2], ch);
    return;
  }

  /* check the EXT flags - Hal */
  if (EXT_FLAGGED(ch, channels[subcmd]) && subcmd >= SCMD_NEWBIE )
  {
    send_to_char(com_msgs[subcmd][2], ch);
    return;
  }

  /* skip leading spaces */
  skip_spaces(&argument);

  /* make sure that there is something there to say! */
  if (!*argument) {
    sprintf(buf1, "Yes, %s, fine, %s we must, but WHAT???\r\n",
	    com_msgs[subcmd][1], com_msgs[subcmd][1]);
    send_to_char(buf1, ch);
    return;
  }
  if (subcmd == SCMD_HOLLER && !IS_NPC(ch)) {
    if (GET_MOVE(ch) < holler_move_cost) {
      send_to_char("You're too exhausted to holler.\r\n", ch);
      return;
    } else
      GET_MOVE(ch) -= holler_move_cost;
  }
 
  /* check that there not in a clan if using clan talk */
  if ( subcmd == SCMD_CTALK && GET_CLAN_NUM(ch) == 0 )
  {
   send_to_char("Your not even in a clan.\n\r" , ch); 
   return;
  }
 
  if (scan_buffer_for_xword(argument))
  {  
    sprintf(s,"WATCHLOG SWEAR: %s %ss, '%s&n'", ch->player.name,com_msgs[subcmd][1], argument);
    mudlog(s,NRM,LVL_IMPL,TRUE);
    send_to_char("&nPlease dont swear on the open channels.\r\n",ch);
    return ;
  }  

    speech = str_dup(argument);
/* this loop goes thru and randomly drops letters from what was said 
 * depanding on how drunk the person is.. 24 is max drunk.. when someone
 * is 24 drunk they pretty much cant be understood.. - Vader
 */
    for(l = 0,j = 0; l < strlen(speech); l++) {
      if(number(1,PISS_FACTOR) > DRUNKNESS) {
        speech[j] = speech[l];
        j++;
        }
      }
      speech[j] = '\0';


  /* set up the color on code */
  strcpy(color_on, com_msgs[subcmd][3]);

  /* first, set up strings to be given to the communicator */
  if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOREPEAT))
    send_to_char(OK, ch);
  else {
    // was C_CMP - DM
    if (COLOR_LEV(ch) >= C_NRM)
      sprintf(buf1, "%sYou %s, '%s%s'&n", color_on, com_msgs[subcmd][1],
	      speech, color_on);
    else
      sprintf(buf1, "You %s, '%s&n'", com_msgs[subcmd][1], speech);
    act(buf1, FALSE, ch, 0, 0, TO_CHAR | TO_SLEEP);
  }


  /* temporary hack to log comms from certain players   bm 2/95 
    currently set for ANGMAR */
/*
  if (GET_IDNUM(ch)==WATCHPLAYER2)
  {
    sprintf(s,"WATCHLOG PLAYER: %s %ss, '%s'", ch->player.name,com_msgs[subcmd][1], argument);
    mudlog(s,NRM,LVL_IMPL,TRUE);
  }
*/



  /* now send all the strings out */
  for (i = descriptor_list; i; i = i->next) {
    if (!i->connected && i != ch->desc && i->character &&
    /*    same_world(i->character,ch) && */
       ((!PRF_FLAGGED(i->character, channels[subcmd]) && subcmd <= SCMD_GRATZ )||
       (!EXT_FLAGGED(i->character, channels[subcmd]) && subcmd >= SCMD_NEWBIE ))&&
	!PLR_FLAGGED(i->character, PLR_WRITING) &&
	!ROOM_FLAGGED(i->character->in_room, ROOM_SOUNDPROOF)) {

      if (subcmd == SCMD_SHOUT &&
	  ((world[ch->in_room].zone != world[i->character->in_room].zone) ||
	   GET_POS(i->character) < POS_RESTING))
	continue;
 
      /* stops others from hearing the clan channels */
       if (subcmd == SCMD_CTALK &&
           (GET_CLAN_NUM(i->character) != GET_CLAN_NUM(ch)))
        continue;

      if (COLOR_LEV(i->character) >= C_NRM)
        sprintf(buf, "%s$n %ss, '%s%s'&n", color_on, com_msgs[subcmd][1], speech, color_on);
      else
        sprintf(buf, "$n %ss, '%s'&n", com_msgs[subcmd][1], speech);
   
     // if (COLOR_LEV(i->character) >= C_NRM)
     //	send_to_char(color_on, i->character);
      act(buf, FALSE, ch, 0, i->character, TO_VICT | TO_SLEEP);
     // if (COLOR_LEV(i->character) >= C_NRM)
     //	send_to_char(KNRM, i->character);
    }
  }
}


ACMD(do_qcomm)
{
  struct descriptor_data *i;

  if (!PRF_FLAGGED(ch, PRF_QUEST)) {
    send_to_char("You aren't even part of the quest!\r\n", ch);
    return;
  }
  skip_spaces(&argument);

  if (!*argument) {
    sprintf(buf, "%s?  Yes, fine, %s we must, but WHAT??\r\n", CMD_NAME,
	    CMD_NAME);
    CAP(buf);
    send_to_char(buf, ch);
  } else {
    if (PRF_FLAGGED(ch, PRF_NOREPEAT))
      send_to_char(OK, ch);
    else {
      if (subcmd == SCMD_QSAY)
	sprintf(buf, "You quest-say, '%s&n'", argument);
      else
	strcpy(buf, argument);
      act(buf, FALSE, ch, 0, argument, TO_CHAR);
    }

    if (subcmd == SCMD_QSAY)
      sprintf(buf, "&C$n&n quest-says, '%s&n'", argument);
    else
      strcpy(buf, argument);

    for (i = descriptor_list; i; i = i->next)
      if (!i->connected && i != ch->desc &&
	  PRF_FLAGGED(i->character, PRF_QUEST))
	act(buf, 0, ch, 0, i->character, TO_VICT | TO_SLEEP);
  }
}
