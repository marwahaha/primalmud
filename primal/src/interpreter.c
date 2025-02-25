/* ************************************************************************
*   File: interpreter.c                                 Part of CircleMUD *
*  Usage: parse user commands, search for specials, call ACMD functions   *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

/*
   Modifications done by Brett Murphy to introduce character races
*/


#define __INTERPRETER_C__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/time.h>

#include "structs.h"
#include "comm.h"
#include "interpreter.h"
#include "db.h"
#include "utils.h"
#include "spells.h"
#include "handler.h"
#include "mail.h"
#include "screen.h"

extern int level_exp[LVL_CHAMP + 1];

extern char *motd;
extern char *imotd;
extern char *background;
extern char *MENU;
extern char *WELC_MESSG;
extern char *START_MESSG;
extern struct char_data *character_list;
extern struct player_index_element *player_table;
extern int top_of_p_table;
extern int restrict;
extern struct index_data *mob_index;
extern struct index_data *obj_index;
extern struct room_data *world;

/* external functions */
void echo_on(struct descriptor_data * d);
void echo_off(struct descriptor_data * d);
void do_start(struct char_data * ch);
void newbie_equip(struct char_data * ch);
void init_char(struct char_data * ch);
int create_entry(char *name);
int special(struct char_data * ch, int cmd, char *arg);
int isbanned(char *hostname);
int Valid_Name(char *newname);

#undef REIMBURSE_LEVELS	/* Calculate new level based on new exp/level system */
#define REIMBURSE_POINTS	/* Give chars points on reimbursed levels */
void reimburse_levels(struct char_data *ch); 

/* prototypes for all do_x functions. */
ACMD(do_breakin);
ACMD(do_mount);
ACMD(do_dismount);
ACMD(do_lag);
ACMD(do_scream);
ACMD(do_scan);
ACMD(do_action);
ACMD(do_advance);
ACMD(do_alias);
ACMD(do_assist);
ACMD(do_assasinate);
ACMD(do_at);
ACMD(do_autoassist);
ACMD(do_backstab);
ACMD(do_ban);
ACMD(do_bash);
ACMD(do_blackjack);
ACMD(do_retreat);
ACMD(do_board);
ACMD(do_cast);
ACMD(do_change);
ACMD(do_charge);
ACMD(do_clan_table);
ACMD(do_clans);
ACMD(do_close);
ACMD(do_color);
ACMD(do_commands);
ACMD(do_compare);
ACMD(do_consider);
ACMD(do_cream);
ACMD(do_credits);
ACMD(do_date);
ACMD(do_dc);
ACMD(do_deimmort);
ACMD(do_diagnose);
ACMD(do_display);
ACMD(do_drink);
ACMD(do_drop);
ACMD(do_eat);
ACMD(do_echo);
ACMD(do_enter);
ACMD(do_equipment);
ACMD(do_examine);
ACMD(do_exit);
ACMD(do_exits);
ACMD(do_fastrent);
ACMD(do_flee);
ACMD(do_follow);
ACMD(do_force);
ACMD(do_gecho);
ACMD(do_gen_comm);
ACMD(do_gen_ps);
ACMD(do_gen_tog);
ACMD(do_gen_write);
ACMD(do_get);
ACMD(do_give);
ACMD(do_go);
ACMD(do_gold);
ACMD(do_goto);
ACMD(do_grab);
ACMD(do_group);
ACMD(do_gsay);
ACMD(do_hcontrol);
ACMD(do_help);
ACMD(do_hide);
ACMD(do_hit);
ACMD(do_house);
ACMD(do_ignore);
ACMD(do_immort);
ACMD(do_info);
ACMD(do_insult);
ACMD(do_inventory);
ACMD(do_invis);
ACMD(do_join);
ACMD(do_kick);
ACMD(do_kill);
ACMD(do_last);
ACMD(do_laston);
ACMD(do_leave);
ACMD(do_levels);
ACMD(do_load);
ACMD(do_loadweapon);
ACMD(do_lock);
ACMD(do_look);
ACMD(do_mortal_kombat);
ACMD(do_move);
ACMD(do_moon);
ACMD(do_not_here);
ACMD(do_offer);
ACMD(do_open);
ACMD(do_order);
ACMD(do_page);
ACMD(do_pick);
ACMD(do_pinch);
ACMD(do_pkset);
ACMD(do_poofset);
ACMD(do_pour);
ACMD(do_practice);
ACMD(do_purge);
ACMD(do_put);
ACMD(do_qcomm);
ACMD(do_queston);
ACMD(do_questoff);
ACMD(do_quit);
ACMD(do_race);
ACMD(do_reallyquit);
ACMD(do_realtime);
ACMD(do_reboot);
ACMD(do_remove);
ACMD(do_rent);
ACMD(do_reply);
ACMD(do_report);
ACMD(do_rescue);
ACMD(do_rest);
ACMD(do_restore);
ACMD(do_return);
ACMD(do_save);
ACMD(do_say);
ACMD(do_score);
ACMD(do_affects);
ACMD(do_areas);
ACMD(do_exp);
ACMD(do_send);
ACMD(do_set);
ACMD(do_sethelp);
ACMD(do_shoot);
ACMD(do_show);
ACMD(do_shutdown);
ACMD(do_sit);
ACMD(do_slay);
ACMD(do_skillset);
ACMD(do_skillshow);
ACMD(do_sleep);
ACMD(do_slots);
ACMD(do_sneak);
ACMD(do_snoop);
ACMD(do_spec_comm);
ACMD(do_split);
ACMD(do_spy);
ACMD(do_stand);
ACMD(do_stat);
ACMD(do_steal);
ACMD(do_switch);
ACMD(do_syslog);
ACMD(do_tag);
ACMD(do_teleport);
ACMD(do_tell);
ACMD(do_tic);
ACMD(do_throw);
ACMD(do_time);
ACMD(do_title);
ACMD(do_toggle);
ACMD(do_track);
ACMD(do_trans);
ACMD(do_unban);
ACMD(do_ungroup);
ACMD(do_unlock);
ACMD(do_use);
ACMD(do_users);
ACMD(do_visible);
ACMD(do_vnum);
ACMD(do_vstat);
ACMD(do_wake);
ACMD(do_wear);
ACMD(do_weather);
ACMD(do_where);
ACMD(do_who);
ACMD(do_wield);
ACMD(do_wimpy);
ACMD(do_wizlock);
ACMD(do_wiznet);
ACMD(do_wizutil);
ACMD(do_write);
ACMD(do_zreset);

/* clan do_x commands -Hal */
ACMD(do_knight);
ACMD(do_banish);
ACMD(do_demote);
ACMD(do_recruit);
ACMD(do_promote);
ACMD(do_signup);

/* This is the Master Command List(tm).
 *
 * You can put new commands in, take commands out, change the order
 * they appear in, etc.  You can adjust the "priority" of commands
 * simply by changing the order they appear in the command list.
 * (For example, if you want "as" to mean "assist" instead of "ask",
 * just put "assist" above "ask" in the Master Command List(tm).
 *
 * In general, utility commands such as "at" should have high priority;
 * infrequently used and dangerously destructive commands should have low
 * priority.
 */

const struct command_info cmd_info[] = {
  { "RESERVED", 0, 0, 0, 0 },	/* this must be first -- for specprocs */

  /* directions must come before other commands but after RESERVED */
  { "north"    , POS_STANDING, do_move     , 0, SCMD_NORTH },
  { "east"     , POS_STANDING, do_move     , 0, SCMD_EAST },
  { "south"    , POS_STANDING, do_move     , 0, SCMD_SOUTH },
  { "west"     , POS_STANDING, do_move     , 0, SCMD_WEST },
  { "up"       , POS_STANDING, do_move     , 0, SCMD_UP },
  { "down"     , POS_STANDING, do_move     , 0, SCMD_DOWN },

  /* now, the main list */
  { "at"       , POS_DEAD    , do_at       , LVL_ANGEL, 0 },
  { "advance"  , POS_DEAD    , do_advance  , LVL_IMPL, 0 },
  { "afk"      , POS_DEAD    , do_gen_tog  , 0, SCMD_AFK },
  { "alias"    , POS_DEAD    , do_alias    , 0, 0 },
  { "accuse"   , POS_SITTING , do_action   , 0, 0 },
  { "affects"  , POS_DEAD    , do_affects  , 0, 0 },
  { "agree"    , POS_DEAD    , do_action   , 0, 0 },
  { "angnet"   , POS_DEAD    , do_wiznet   , LVL_ANGEL, SCMD_ANGNET },
  { "-"        , POS_DEAD    , do_wiznet   , LVL_ANGEL, SCMD_ANGNET },
  { "angel"    , POS_RESTING , do_action   , LVL_ANGEL, 0 },
  { "apologise", POS_RESTING , do_action   , 0, 0 },
  { "applaud"  , POS_RESTING , do_action   , 0, 0 },
  { "areas"    , POS_DEAD    , do_gen_ps   , 0, SCMD_AREAS },
  { "assist"   , POS_FIGHTING, do_assist   , 1, 0 },
  { "assasinate", POS_FIGHTING, do_not_here , 1, 0 },
  { "ask"      , POS_RESTING , do_spec_comm, 0, SCMD_ASK },
  { "auction"  , POS_SLEEPING, do_gen_comm , 0, SCMD_AUCTION },
  { "autoassist", POS_RESTING, do_autoassist, 0, 0 }, 
  { "autoexit" , POS_DEAD    , do_gen_tog  , 0, SCMD_AUTOEXIT },
  { "autogold" , POS_DEAD    , do_gen_tog  , 0, SCMD_AUTOGOLD }, 
  { "autoloot" , POS_DEAD    , do_gen_tog  , 0, SCMD_AUTOLOOT }, 
  { "autosplit", POS_DEAD    , do_gen_tog  , 0, SCMD_AUTOSPLIT }, 

  { "backstab" , POS_STANDING, do_backstab , 1, 0 },
  { "ban"      , POS_DEAD    , do_ban      , LVL_IMPL, 0 },
  { "banish"   , POS_RESTING , do_banish   , 0, 0 },
  { "balance"  , POS_STANDING, do_not_here , 1, 0 },
  { "bark"     , POS_RESTING , do_action   , 0, 0 },
  { "bash"     , POS_FIGHTING, do_bash     , 1, 0 },
  { "bat"      , POS_RESTING , do_action   , 0, 0 },
  { "bearhug"  , POS_RESTING , do_action   , 0, 0 },
  { "bay"      , POS_RESTING , do_action   , 0, 0 },
  { "beat"     , POS_RESTING , do_action   , 0, 0 },
  { "beg"      , POS_RESTING , do_action   , 0, 0 },
  { "behead"   , POS_RESTING , do_action   , 0, 0 },
  { "bj"       , POS_RESTING , do_blackjack, 0, 0 },
  { "blackjack", POS_RESTING , do_blackjack, 0, 0 },
  { "blast"    , POS_FIGHTING, do_hit      , 0, SCMD_HIT},
  { "bleed"    , POS_RESTING , do_action   , 0, 0 },
  { "blink"    , POS_RESTING , do_action   , 0, 0 },
  { "blush"    , POS_RESTING , do_action   , 0, 0 },
  { "bounce"   , POS_STANDING, do_action   , 0, 0 },
  { "bow"      , POS_STANDING, do_action   , 0, 0 },
  { "brb"      , POS_RESTING , do_action   , 0, 0 },
//  { "breakin"  , POS_STANDING, do_breakin  , 0, 0 },
  { "brief"    , POS_DEAD    , do_gen_tog  , 0, SCMD_BRIEF },
  { "burp"     , POS_RESTING , do_action   , 0, 0 },
  { "buy"      , POS_STANDING, do_not_here , 0, 0 },
  { "bug"      , POS_DEAD    , do_gen_write, 0, SCMD_BUG },
  { "bet"      , POS_RESTING , do_not_here , 0, 0 },

  { "cast"     , POS_SITTING , do_cast     , 1, 0 },
  { "cackle"   , POS_RESTING , do_action   , 0, 0 },
  { "cape"     , POS_RESTING , do_action   , 0, 0 },
  { "challenge", POS_DEAD    , do_action   , 0, 0 },
  { "change"   , POS_FIGHTING, do_change   , 0, 0 },
//  { "charge"   , POS_FIGHTING, do_charge   , 0, 0 },
  { "chase"    , POS_RESTING , do_action   , 0, 0 },
  { "check"    , POS_STANDING, do_not_here , 1, 0 },
  { "cheer"    , POS_DEAD    , do_action   , 0, 0 },
  { "chuckle"  , POS_RESTING , do_action   , 0, 0 },
  { "clans"    , POS_DEAD    , do_clans    , 0, 0 },
  { "clantable", POS_DEAD    , do_clan_table,0, 0 }, 
  { "clap"     , POS_RESTING , do_action   , 0, 0 },
  { "clear"    , POS_DEAD    , do_gen_ps   , 0, SCMD_CLEAR },
  { "close"    , POS_SITTING , do_close    , 0, 0 },
  { "cls"      , POS_DEAD    , do_gen_ps   , 0, SCMD_CLEAR },
  { "compare"  , POS_SITTING , do_compare  , 0, 0 },
  { "consider" , POS_RESTING , do_consider , 0, 0 },
  { "color"    , POS_DEAD    , do_color    , 0, 0 },
  { "comb"     , POS_RESTING , do_action   , 0, 0 },
  { "comfort"  , POS_RESTING , do_action   , 0, 0 },
  { "commands" , POS_DEAD    , do_commands , 0, SCMD_COMMANDS },
  { "compact"  , POS_DEAD    , do_gen_tog  , 0, SCMD_COMPACT },
  { "cough"    , POS_RESTING , do_action   , 0, 0 },
  { "cream"    , POS_STANDING, do_cream    , LVL_GOD, 0 },
  { "credits"  , POS_DEAD    , do_gen_ps   , 0, SCMD_CREDITS },
  { "cringe"   , POS_RESTING , do_action   , 0, 0 },
  { "cripple"  , POS_RESTING , do_action   , 0, 0 },
  { "cry"      , POS_RESTING , do_action   , 0, 0 },
  { "ctalk"    , POS_SLEEPING, do_gen_comm , 0, SCMD_CTALK },
  { "cuddle"   , POS_RESTING , do_action   , 0, 0 },
  { "curse"    , POS_RESTING , do_action   , 0, 0 },
  { "curtsey"  , POS_STANDING, do_action   , 0, 0 },


  { "dance"    , POS_STANDING, do_action   , 0, 0 },
  { "date"     , POS_DEAD    , do_date     , LVL_ETRNL1, SCMD_DATE },
  { "daydream" , POS_SLEEPING, do_action   , 0, 0 },
  { "dc"       , POS_DEAD    , do_dc       , LVL_GOD, 0 },
  { "deposit"  , POS_STANDING, do_not_here , 1, 0 },
  { "deimmort" , POS_SITTING , do_deimmort , LVL_GRGOD, 0 },
  { "demote"   , POS_STANDING, do_demote   , 0, 0 },
  { "diagnose" , POS_RESTING , do_diagnose , 0, 0 },
  { "die"      , POS_SLEEPING, do_action   , 0, 0 },
//  { "dismount" , POS_RESTING , do_dismount , 0, 0}, 
  { "display"  , POS_DEAD    , do_display , 0, 0 },
  { "doh"      , POS_DEAD    , do_action   , 0, 0 },
  { "donate"   , POS_RESTING , do_drop     , 0, SCMD_DONATE },
  { "drink"    , POS_RESTING , do_drink    , 0, SCMD_DRINK },
  { "drop"     , POS_RESTING , do_drop     , 0, SCMD_DROP },
  { "drool"    , POS_RESTING , do_action   , 0, 0 },
  { "duck"     , POS_DEAD    , do_action   , 0, 0 },

  { "eat"      , POS_RESTING , do_eat      , 0, SCMD_EAT },
  { "echo"     , POS_SLEEPING, do_echo     , LVL_ETRNL7, SCMD_ECHO },
  { "emote"    , POS_RESTING , do_echo     , 10, SCMD_EMOTE },
  { "elvis"    , POS_DEAD    , do_action   , 0, 0 },
  { ":"        , POS_RESTING, do_echo      , 1, SCMD_EMOTE },
  { "ehelp"    , POS_SLEEPING,do_commands  , LVL_ETRNL1, SCMD_WIZHELP },
  { "embrace"  , POS_STANDING, do_action   , 0, 0 },
  { "enter"    , POS_STANDING, do_enter    , 0, 0 },
  { "equipment", POS_SLEEPING, do_equipment, 0, 0 },
  { "eskimo"   , POS_RESTING , do_action   , 0, 0 },
  { "exits"    , POS_RESTING , do_exits    , 0, 0 },
  { "examine"  , POS_SITTING , do_examine  , 0, 0 },
  { "exp"      , POS_DEAD    , do_exp      , 0, 0 },

  { "force"    , POS_SLEEPING, do_force    , LVL_GRGOD, 0 },
  { "fart"     , POS_RESTING , do_action   , 0, 0 },
  { "faint"    , POS_RESTING , do_action   , 0, 0 },
/*  { "fastrent" , POS_RESTING , do_fastrent , 0, 0 }, */
  { "fatality" , POS_RESTING , do_action   , LVL_ETRNL5, 0 },
  { "feed"     , POS_RESTING , do_action   , 0, 0 },
  { "fill"     , POS_STANDING, do_pour     , 0, SCMD_FILL },
  { "flee"     , POS_FIGHTING, do_flee     , 1, 0 },
  { "flip"     , POS_STANDING, do_action   , 0, 0 },
  { "flirt"    , POS_RESTING , do_action   , 0, 0 },
  { "follow"   , POS_RESTING , do_follow   , 0, 0 },
  { "fondle"   , POS_RESTING , do_action   , 0, 0 },
  { "freez"    , POS_DEAD    , do_quit     , LVL_FREEZE, SCMD_FREEZE },
  { "freeze"   , POS_DEAD    , do_wizutil  , LVL_FREEZE, SCMD_FREEZE },
  { "french"   , POS_RESTING , do_action   , 0, 0 },
  { "frown"    , POS_RESTING , do_action   , 0, 0 },
  { "fume"     , POS_RESTING , do_action   , 0, 0 },

  { "get"      , POS_RESTING , do_get      , 0, 0 },
  { "gasp"     , POS_RESTING , do_action   , 0, 0 },
  { "gecho"    , POS_DEAD    , do_gecho    , LVL_GOD, 0 },
  { "gglow"    , POS_RESTING , do_action   , 0, 0 },
  { "give"     , POS_RESTING , do_give     , 0, 0 },
  { "giggle"   , POS_RESTING , do_action   , 0, 0 },
  { "glare"    , POS_RESTING , do_action   , 0, 0 },
  { "glaze"    , POS_RESTING , do_action   , 0, 0 },
  { "go"       , POS_STANDING, do_go       , 0, 0 },
  { "goto"     , POS_STANDING, do_goto     , LVL_CHAMP, 0 },
  { "gold"     , POS_DEAD    , do_gold     , 0, 0 },
  { "goose"    , POS_DEAD    , do_action   , 0, 0 },
  { "gossip"   , POS_SLEEPING, do_gen_comm , 0, SCMD_GOSSIP },
  { "group"    , POS_RESTING , do_group    , 1, 0 },
  { "grope"    , POS_DEAD    , do_action   , 0, 0 },
  { "grab"     , POS_RESTING , do_grab     , 0, 0 },
  { "grats"    , POS_SLEEPING, do_gen_comm , 0, SCMD_GRATZ },
  { "greet"    , POS_RESTING , do_action   , 0, 0 },
  { "grin"     , POS_RESTING , do_action   , 0, 0 },
  { "groan"    , POS_RESTING , do_action   , 0, 0 },
  { "grovel"   , POS_RESTING , do_action   , 0, 0 },
  { "growl"    , POS_RESTING , do_action   , 0, 0 },
  { "grumble"  , POS_DEAD    , do_action   , 0, 0 },
  { "gsay"     , POS_SLEEPING, do_gsay     , 0, 0 },
  { "gtell"    , POS_SLEEPING, do_gsay     , 0, 0 },
  { "guide"    , POS_DEAD    , do_action   , 0, 0 },

  { "happy"    , POS_DEAD    , do_action   , 0, 0 },
  { "harp"     , POS_RESTING , do_action   , 0, 0 },
  { "help"     , POS_DEAD    , do_help     , 0, 0 },
  { "handbook" , POS_DEAD    , do_gen_ps   , LVL_ANGEL, SCMD_HANDBOOK },
  { "hcontrol" , POS_DEAD    , do_hcontrol , LVL_GRGOD, 0 },
  { "hickey"   , POS_DEAD    , do_action   , 0, 0 },
  { "hiccup"   , POS_RESTING , do_action   , 0, 0 },
  { "hide"     , POS_RESTING , do_hide     , 1, 0 },
  { "high5"    , POS_DEAD    , do_action   , 0, 0 },
  { "hit"      , POS_FIGHTING, do_hit      , 0, SCMD_HIT },
  { "hold"     , POS_RESTING , do_grab     , 1, 0 },
  { "holler"   , POS_RESTING , do_gen_comm , 1, SCMD_HOLLER },
  { "holylight", POS_DEAD    , do_gen_tog  , LVL_ETRNL4, SCMD_HOLYLIGHT },
  { "hop"      , POS_RESTING , do_action   , 0, 0 },
  { "house"    , POS_RESTING , do_house    , 0, 0 },
  { "hug"      , POS_RESTING , do_action   , 0, 0 },
  { "hum"      , POS_DEAD    , do_action   , 0, 0 },
  { "hunt"     , POS_STANDING, do_track    , 0, SCMD_HUNT },

  { "inventory", POS_DEAD    , do_inventory, 0, 0 },
  { "idea"     , POS_DEAD    , do_gen_write, 0, SCMD_IDEA },

  { "ignore"   , POS_SLEEPING, do_ignore   , 0, 0 },
  { "imotd"    , POS_DEAD    , do_gen_ps   , LVL_CHAMP, SCMD_IMOTD },
  { "immlist"  , POS_DEAD    , do_gen_ps   , 0, SCMD_IMMLIST },
  { "immnet"   , POS_DEAD    , do_wiznet   , LVL_ETRNL1, SCMD_IMMNET },
  { "/"        , POS_DEAD    , do_wiznet   , LVL_ETRNL1, SCMD_IMMNET },
  { "immort"   , POS_SITTING , do_immort   , LVL_GRGOD, 0 },
  { "info"     , POS_SLEEPING, do_gen_ps   , 0, SCMD_INFO },
  { "insult"   , POS_RESTING , do_insult   , 0, 0 },
  { "invis"    , POS_DEAD    , do_invis    , LVL_ANGEL, 0 },
  { "iwhistle" , POS_DEAD    , do_action   , 0, 0 },

  { "junk"     , POS_RESTING , do_drop     , 0, SCMD_JUNK },
  { "join"     , POS_SITTING , do_join     , 0, 0 },
  { "jeer"     , POS_DEAD    , do_action   , 0, 0 },

  { "kill"     , POS_FIGHTING, do_kill     , 0, 0 },
  { "kick"     , POS_FIGHTING, do_kick     , 1, 0 },
  { "kiss"     , POS_RESTING , do_action   , 0, 0 },
  { "knight"   , POS_DEAD    , do_knight   , 0, 0 },
  
  { "reload"   , POS_FIGHTING,  do_loadweapon, 0, 0},
  { "look"     , POS_RESTING , do_look     , 0, SCMD_LOOK },
  { "laugh"    , POS_RESTING , do_action   , 0, 0 },
  { "last"     , POS_DEAD    , do_last     , LVL_GOD, 0 },
  { "laston"   , POS_DEAD    , do_laston   , 25, 0},
  { "lag"      , POS_DEAD    , do_lag      , LVL_GRGOD, 0 },
  { "leave"    , POS_STANDING, do_leave    , 0, 0 },
  { "levels"   , POS_DEAD    , do_levels   , 0, 0 },
  { "lie"      , POS_DEAD    , do_action   , 0, 0 },
  { "list"     , POS_STANDING, do_not_here , 0, 0 },
  { "lick"     , POS_RESTING , do_action   , 0, 0 },
  { "lock"     , POS_SITTING , do_lock     , 0, 0 },
  { "load"     , POS_DEAD    , do_load     , LVL_GOD, 0 },
  { "lol"      , POS_RESTING , do_action   , 0, 0 },
  { "love"     , POS_RESTING , do_action   , 0, 0 },
  { "lure"     , POS_RESTING , do_action   , 0, 0 },

  { "massage"  , POS_RESTING , do_action   , 0, 0 },
  { "mgrin"    , POS_RESTING , do_action   , 0, 0 },
  { "moan"     , POS_RESTING , do_action   , 0, 0 },
  { "moon"     , POS_DEAD    , do_moon     , 0, 0 },
  { "mortalkombat", POS_STANDING, do_mortal_kombat, 0, 0 },
  { "motd"     , POS_DEAD    , do_gen_ps   , 0, SCMD_MOTD },
//  { "mount",     POS_STANDING, do_mount    , 0, 0 },
  { "mail"     , POS_STANDING, do_not_here , 1, 0 },
  { "mute"     , POS_DEAD    , do_wizutil  , LVL_GOD, SCMD_SQUELCH },
  { "murder"   , POS_FIGHTING, do_hit      , 0, SCMD_MURDER },

  { "nestle"   , POS_DEAD    , do_action   , 0, 0 },
  { "newbie"   , POS_SLEEPING, do_gen_comm , 0, SCMD_NEWBIE },
  { "news"     , POS_SLEEPING, do_gen_ps   , 0, SCMD_NEWS },
  { "nibble"   , POS_RESTING , do_action   , 0, 0 },
  { "nod"      , POS_RESTING , do_action   , 0, 0 },
  { "noauction", POS_DEAD    , do_gen_tog  , 0, SCMD_NOAUCTION },
  { "noctalk"  , POS_DEAD    , do_gen_tog  , 0, SCMD_NOCTALK },
  { "noimmnet" , POS_DEAD    , do_gen_tog  , LVL_ETRNL1, SCMD_NOIMMNET },
  { "noinfo"   , POS_DEAD    , do_gen_tog  , 0 , SCMD_NOINFO },
  { "nogossip" , POS_DEAD    , do_gen_tog  , 0, SCMD_NOGOSSIP },
  { "nograts"  , POS_DEAD    , do_gen_tog  , 0, SCMD_NOGRATZ },
  { "nohassle" , POS_DEAD    , do_gen_tog  , LVL_CHAMP, SCMD_NOHASSLE },
  { "nonewbie" , POS_DEAD    , do_gen_tog  , 0, SCMD_NONEWBIE },
  { "norepeat" , POS_DEAD    , do_gen_tog  , 0, SCMD_NOREPEAT },
  { "noshout"  , POS_SLEEPING, do_gen_tog  , 1, SCMD_DEAF },
  { "nosummon" , POS_DEAD    , do_gen_tog  , 1, SCMD_NOSUMMON },
  { "notell"   , POS_DEAD    , do_gen_tog  , 1, SCMD_NOTELL },
  { "notitle"  , POS_DEAD    , do_wizutil  , LVL_GOD, SCMD_NOTITLE },
  { "nowiz"    , POS_DEAD    , do_gen_tog  , LVL_IS_GOD, SCMD_NOWIZ },
  { "nudge"    , POS_RESTING , do_action   , 0, 0 },
  { "nuzzle"   , POS_RESTING , do_action   , 0, 0 },

  { "order"    , POS_RESTING , do_order    , 1, 0 },
  { "offer"    , POS_STANDING, do_not_here , 1, 0 },
  { "open"     , POS_SITTING , do_open     , 0, 0 },

  { "put"      , POS_RESTING , do_put      , 0, 0 },
  { "pant"     , POS_RESTING , do_action   , 0, 0 },
  { "pat"      , POS_RESTING , do_action   , 0, 0 },
  { "page"     , POS_DEAD    , do_page     , LVL_GOD, 0 },
  { "pardon"   , POS_DEAD    , do_wizutil  , LVL_GOD, SCMD_PARDON },
  { "pea"      , POS_DEAD    , do_action   , 0, 0 },
  { "peer"     , POS_RESTING , do_action   , 0, 0 },
  { "picard"   , POS_DEAD    , do_action   , 0, 0 },
  { "pick"     , POS_STANDING, do_pick     , 1, 0 },
  { "pinch"    , POS_DEAD    , do_action   , 0, 0 },
  { "pkset"    , POS_DEAD    , do_pkset    , LVL_GOD, 0 },
  { "point"    , POS_RESTING , do_action   , 0, 0 },
  { "poke"     , POS_RESTING , do_action   , 0, 0 },
  { "policy"   , POS_DEAD    , do_gen_ps   , 0, SCMD_POLICIES },
  { "ponder"   , POS_RESTING , do_action   , 0, 0 },
  { "poofin"   , POS_DEAD    , do_poofset  , LVL_CHAMP, SCMD_POOFIN },
  { "poofout"  , POS_DEAD    , do_poofset  , LVL_CHAMP, SCMD_POOFOUT },
  { "pour"     , POS_STANDING, do_pour     , 0, SCMD_POUR },
  { "pout"     , POS_RESTING , do_action   , 0, 0 },
  { "pray"     , POS_RESTING , do_action   , 0, 0 },
  { "prod"     , POS_DEAD    , do_action   , 0, 0 },
  { "prompt"   , POS_DEAD    , do_display  , 0, 0 },
  { "promote"  , POS_STANDING, do_promote  , 0, 0 }, 
  { "practice" , POS_DEAD    , do_practice , 1, 0 },
  { "primal"   , POS_STANDING, do_scream   , 0, 0 },
  { "puke"     , POS_RESTING , do_action   , 0, 0 },
  { "punch"    , POS_RESTING , do_action   , 0, 0 },
  { "puppy"    , POS_DEAD    , do_action   , 0, 0 },
  { "purr"     , POS_RESTING , do_action   , 0, 0 },
  { "purge"    , POS_DEAD    , do_purge    , LVL_GOD, 0 },
  { "play"     , POS_RESTING , do_not_here , 0, 0 },

  { "quaff"    , POS_RESTING , do_use      , 0, SCMD_QUAFF },
  { "qecho"    , POS_DEAD    , do_qcomm    , 0, SCMD_QECHO },
  { "quest"    , POS_DEAD    , do_gen_tog  , 0, SCMD_QUEST },
  { "queston"  , POS_DEAD    , do_queston  , LVL_ANGEL, 0 },
  { "questoff" , POS_DEAD    , do_questoff , LVL_ANGEL, 0 },
  { "qui"      , POS_DEAD    , do_quit     , 0, SCMD_QUIT },
  { "quit"     , POS_DEAD    , do_quit     , 0, SCMD_QUIT },
  { "quitreally"     , POS_DEAD    , do_reallyquit     , 0, SCMD_QUIT },
  { "qsay"     , POS_RESTING , do_qcomm    , 0, SCMD_QSAY },

  { "race"     , POS_RESTING , do_race     , 0, 0 },
  { "reply"    , POS_SLEEPING, do_reply    , 0, 0 },
  { "rest"     , POS_RESTING , do_rest     , 0, 0 },
  { "read"     , POS_SLEEPING, do_look     , 0, SCMD_READ },
  { "realtime" , POS_SLEEPING, do_realtime , 0, 0 },
  { "reread"   , POS_DEAD    , do_reboot   , LVL_IMPL, 0 },
  { "recite"   , POS_RESTING , do_use      , 0, SCMD_RECITE },
  { "receive"  , POS_STANDING, do_not_here , 1, 0 },
  { "recruit"  , POS_RESTING , do_recruit  , 0, 0 },
  { "rejoice"  , POS_RESTING , do_action   , 0, 0 },
  { "remove"   , POS_RESTING , do_remove   , 0, 0 },
  { "rent"     , POS_STANDING, do_not_here , 1, 0 },
  { "report"   , POS_DEAD    , do_report   , 0, 0 },
  { "reroll"   , POS_DEAD    , do_wizutil  , LVL_GRGOD, SCMD_REROLL },
  { "rescue"   , POS_FIGHTING, do_rescue   , 1, 0 },
  { "restore"  , POS_DEAD    , do_restore  , LVL_GOD, 0 },
  { "retreat"  , POS_FIGHTING, do_retreat  , 1, 0 },
  { "return"   , POS_DEAD    , do_return   , 0, 0 },
  { "rglow"    , POS_SITTING , do_action   , 0, 0 },
  { "rofl"     , POS_SITTING , do_action   , 0, 0 },
  { "roll"     , POS_RESTING , do_action   , 0, 0 },
  { "roomflags", POS_DEAD    , do_gen_tog  , LVL_ETRNL2, SCMD_ROOMFLAGS },
  { "roses"    , POS_RESTING , do_action   , 0, 0 },
  { "ruffle"   , POS_STANDING, do_action   , 0, 0 },

  { "say"      , POS_RESTING , do_say      , 0, 0 },
  { "'"        , POS_RESTING , do_say      , 0, 0 },
  { "save"     , POS_SLEEPING, do_save     , 0, 0 },
  { "score"    , POS_DEAD    , do_score    , 0, 0 },
  { "signup"   , POS_RESTING , do_signup   , 0, 0 },

  { "salute"   , POS_DEAD    , do_action   , 0, 0 },
  { "scan"     , POS_STANDING, do_scan     , 0, 0 },
  { "scratch"  , POS_DEAD    , do_action   , 0, 0 },
  { "scream"   , POS_RESTING , do_action   , 0, 0 },
  { "sell"     , POS_STANDING, do_not_here , 0, 0 },
  { "send"     , POS_SLEEPING, do_send     , LVL_GOD, 0 },
  { "serve"    , POS_SLEEPING, do_action   , 0, 0 },
  { "set"      , POS_DEAD    , do_set      , LVL_GRGOD, 0 },
  { "sethelp"  , POS_DEAD    , do_sethelp  , LVL_GRGOD, 0 }, 
  { "shoot"    , POS_FIGHTING, do_shoot    , 0, SCMD_HIT },
  { "shout"    , POS_RESTING , do_gen_comm , 0, SCMD_SHOUT },
  { "shake"    , POS_RESTING , do_action   , 0, 0 },
  { "shandshake" , POS_SITTING , do_action   , 0, 0 },
  { "shed"     , POS_SITTING , do_action   , 0, 0 },
  { "shiver"   , POS_RESTING , do_action   , 0, 0 },
  { "show"     , POS_DEAD    , do_show     , LVL_ETRNL6, 0 },
  { "showoff"  , POS_SITTING , do_action   , LVL_CHAMP, 0 },
  { "shrug"    , POS_RESTING , do_action   , 0, 0 },
  { "shudder"  , POS_DEAD    , do_action   , 0, 0 },
  { "shutdow"  , POS_DEAD    , do_shutdown , LVL_IMPL, 0 },
  { "shutdown" , POS_DEAD    , do_shutdown , LVL_IMPL, SCMD_SHUTDOWN },
  { "sigh"     , POS_RESTING , do_action   , 0, 0 },
  { "sing"     , POS_RESTING , do_action   , 0, 0 },
  { "sink"     , POS_STANDING, do_action   , 0, 0 },
  { "sip"      , POS_RESTING , do_drink    , 0, SCMD_SIP },
  { "sit"      , POS_RESTING , do_sit      , 0, 0 },
  { "sitn"     , POS_DEAD    , do_action   , 0, 0 },
  { "skillset" , POS_SLEEPING, do_skillset , LVL_GRGOD, 0 },
  { "skillshow", POS_SLEEPING, do_skillshow, LVL_GOD, 0 },
  { "slay"     , POS_STANDING, do_slay     , 0, 0 },
  { "sleep"    , POS_SLEEPING, do_sleep    , 0, 0 },
  { "slap"     , POS_RESTING , do_action   , 0, 0 },
  { "slots"    , POS_RESTING , do_slots    , 0, 0 },
  { "slowns"   , POS_DEAD    , do_gen_tog  , LVL_IMPL, SCMD_SLOWNS },
  { "smile"    , POS_RESTING , do_action   , 0, 0 },
  { "smirk"    , POS_RESTING , do_action   , 0, 0 },
  { "smooch"   , POS_DEAD    , do_action   , 0, 0 },
  { "snap"     , POS_RESTING , do_action   , 0, 0 },
  { "snarl"    , POS_RESTING , do_action   , 0, 0 },
  { "sneeze"   , POS_RESTING , do_action   , 0, 0 },
  { "sneak"    , POS_STANDING, do_sneak    , 1, 0 },
  { "snicker"  , POS_RESTING , do_action   , 0, 0 },
  { "sniff"    , POS_RESTING , do_action   , 0, 0 },
  { "snore"    , POS_SLEEPING, do_action   , 0, 0 },
  { "snort"    , POS_RESTING , do_action   , 0, 0 },
  { "snowball" , POS_STANDING, do_action   , LVL_ETRNL1, 0 },
  { "snoop"    , POS_DEAD    , do_snoop    , LVL_GRGOD, 0 },
  { "snuggle"  , POS_RESTING , do_action   , 0, 0 },
  { "socials"  , POS_DEAD    , do_commands , 0, SCMD_SOCIALS },
  { "split"    , POS_SITTING , do_split    , 1, 0 },
  { "spank"    , POS_RESTING , do_action   , 0, 0 },
  { "spin"     , POS_RESTING , do_action   , 0, 0 },
  { "spit"     , POS_STANDING, do_action   , 0, 0 },
  { "spy"      , POS_STANDING, do_spy	   , 0, 0 },
  { "squeeze"  , POS_RESTING , do_action   , 0, 0 },
  { "stand"    , POS_RESTING , do_stand    , 0, 0 },
  { "stake"    , POS_STANDING, do_slay     , 0, 0 },
  { "stare"    , POS_RESTING , do_action   , 0, 0 },
  { "stat"     , POS_DEAD    , do_stat     , LVL_ETRNL8, 0 },
  { "steal"    , POS_STANDING, do_steal    , 1, 0 },
  { "steam"    , POS_RESTING , do_action   , 0, 0 },
  { "strangle" , POS_RESTING , do_action   , 0, 0 },
  { "stretch"  , POS_RESTING , do_action   , 0, 0 },
  { "strip"    , POS_DEAD    , do_action   , 0, 0 },
  { "stroke"   , POS_RESTING , do_action   , 0, 0 },
  { "strut"    , POS_STANDING, do_action   , 0, 0 },
  { "sulk"     , POS_RESTING , do_action   , 0, 0 },
  { "switch"   , POS_DEAD    , do_switch   , LVL_ANGEL, 0 },
  { "syslog"   , POS_DEAD    , do_syslog   , LVL_GOD, 0 },

  { "tell"     , POS_DEAD    , do_tell     , 0, 0 },
  { "tag"      , POS_STANDING, do_tag      , 0, 0 },
  { "tackle"   , POS_RESTING , do_action   , 0, 0 },
  { "take"     , POS_RESTING , do_get      , 0, 0 },
  { "tango"    , POS_STANDING, do_action   , 0, 0 },
  { "taunt"    , POS_RESTING , do_action   , 0, 0 },
  { "taste"    , POS_RESTING , do_eat      , 0, SCMD_TASTE },
  { "teleport" , POS_DEAD    , do_teleport , LVL_GOD, 0 },
  { "thank"    , POS_RESTING , do_action   , 0, 0 },
  { "think"    , POS_RESTING , do_action   , 0, 0 },
  { "throw"    , POS_STANDING, do_throw    , 0, 0 },
  { "thaw"     , POS_DEAD    , do_wizutil  , LVL_FREEZE, SCMD_THAW },
  { "title"    , POS_DEAD    , do_title    , 0, 0 },
  { "tic"      , POS_DEAD    , do_tic      , LVL_IMPL, 0 },
  { "tickle"   , POS_RESTING , do_action   , 0, 0 },
  { "tictac"   , POS_RESTING , do_action   , 0, 0 },
  { "time"     , POS_DEAD    , do_time     , 0, 0 },
  { "toggle"   , POS_DEAD    , do_toggle   , 0, 0 },
  { "tongue"   , POS_DEAD    , do_action   , 0, 0 },
  { "track"    , POS_STANDING, do_track    , 0, 0 },
  { "train"    , POS_STANDING, do_not_here , 0, 0 },
  { "transfer" , POS_SLEEPING, do_trans    , LVL_GOD, 0 },
  { "tug"      , POS_RESTING , do_action   , 0, 0 },
  { "twiddle"  , POS_RESTING , do_action   , 0, 0 },
  { "typo"     , POS_DEAD    , do_gen_write, 0, SCMD_TYPO },

  { "unlock"   , POS_SITTING , do_unlock   , 0, 0 },
  { "ungroup"  , POS_DEAD    , do_ungroup  , 0, 0 },
  { "unban"    , POS_DEAD    , do_unban    , LVL_IMPL, 0 },
  { "unaffect" , POS_DEAD    , do_wizutil  , LVL_GOD, SCMD_UNAFFECT },
  { "uptime"   , POS_DEAD    , do_date     , LVL_ETRNL1, SCMD_UPTIME },
  { "use"      , POS_SITTING , do_use      , 1, SCMD_USE },
  { "users"    , POS_DEAD    , do_users    , LVL_GOD, 0 },

  { "value"    , POS_STANDING, do_not_here , 0, 0 },
  { "version"  , POS_DEAD    , do_gen_ps   , 0, SCMD_VERSION },
  { "visible"  , POS_RESTING , do_visible  , 1, 0 },
  { "vnum"     , POS_DEAD    , do_vnum     , LVL_IMMORT, 0 },
  { "vstat"    , POS_DEAD    , do_vstat    , LVL_IMMORT, 0 },
  { "vulcan"   , POS_SITTING , do_pinch    , LVL_GOD, 0 },

  { "wake"     , POS_SLEEPING, do_wake     , 0, 0 },
  { "wave"     , POS_RESTING , do_action   , 0, 0 },
  { "wear"     , POS_RESTING , do_wear     , 0, 0 },
  { "wash"     , POS_RESTING , do_action   , 0, 0 },
  { "weather"  , POS_RESTING , do_weather  , 0, 0 },
  { "wedgie"   , POS_DEAD    , do_action   , 0, 0 },
  { "wetwilly" , POS_DEAD    , do_action   , 0, 0 },
  { "who"      , POS_DEAD    , do_who      , 0, 0 },
  { "whoami"   , POS_DEAD    , do_gen_ps   , 0, SCMD_WHOAMI },
  { "where"    , POS_RESTING , do_where    , 1, 0 },
  { "whap"     , POS_DEAD    , do_action   , 0, 0 },
  { "whisper"  , POS_RESTING , do_spec_comm, 0, SCMD_WHISPER },
  { "whine"    , POS_RESTING , do_action   , 0, 0 },
  { "whistle"  , POS_RESTING , do_action   , 0, 0 },
  { "wield"    , POS_RESTING , do_wield    , 0, 0 },
  { "wiggle"   , POS_STANDING, do_action   , 0, 0 },
  { "wimpy"    , POS_DEAD    , do_wimpy    , 0, 0 },
  { "wink"     , POS_RESTING , do_action   , 0, 0 },
  { "withdraw" , POS_STANDING, do_not_here , 1, 0 },
  { "wiznet"   , POS_DEAD    , do_wiznet   , LVL_GOD, 0 },
  { ";"        , POS_DEAD    , do_wiznet   , LVL_GOD, 0 },
  { "wizhelp"  , POS_SLEEPING, do_commands , LVL_CHAMP, SCMD_WIZHELP },
  { "wizlist"  , POS_DEAD    , do_gen_ps   , 0, SCMD_WIZLIST },
  { "wizlock"  , POS_DEAD    , do_wizlock  , LVL_IMPL, 0 },
  { "woohoo"  , POS_RESTING , do_action   , 0, 0 },
  { "worship"  , POS_RESTING , do_action   , 0, 0 },
  { "write"    , POS_SLEEPING, do_write    , 1, 0 },

  { "yawn"     , POS_RESTING , do_action   , 0, 0 },
  { "yodel"    , POS_RESTING , do_action   , 0, 0 },

  { "zreset"   , POS_DEAD    , do_zreset   , LVL_GOD, 0 },

  { "\n", 0, 0, 0, 0 } };	/* this must be last */
  
  
char *fill[] =
{
  "in",
  "from",
  "with",
  "the",
  "on",
  "at",
  "to",
  "\n"
};

char *reserved[] =
{
  "self",
  "me",
  "all",
  "room",
  "someone",
  "something",
  "\n"
};

#define BASE_SECT(n) ((n) & 0x000f)

/*
 * This is the actual command interpreter called from game_loop() in comm.c
 * It makes sure you are the proper level and position to execute the command,
 * then calls the appropriate function.
 */
void command_interpreter(struct char_data * ch, char *argument)
{
  int cmd, length;
  extern int no_specials;
  char *line;
  int room;

  /* Affect player lag */
  if( PLR_FLAGGED(ch, PLR_LAGGED) )
	WAIT_STATE(ch, PULSE_VIOLENCE * number(2, 6) );

  REMOVE_BIT(AFF_FLAGS(ch), AFF_HIDE);

/* this is so that the AFK thing goes away when ya type.. lots of people
 * play for hours with it on... - Vader 
 */
  REMOVE_BIT(PRF_FLAGS(ch), PRF_AFK);

/* think this is the best place to make ya fall if fly wears off - Vader */
  if(!IS_NPC(ch) && !IS_AFFECTED(ch,AFF_FLY) &&
     (BASE_SECT(world[ch->in_room].sector_type) == SECT_FLYING) && 
     CAN_GO(ch,DOWN)) {
    room = world[ch->in_room].dir_option[DOWN]->to_room;
    act("$n realises $e can no longer fly and falls.",FALSE,ch,0,0,TO_ROOM);
    send_to_char("You can fight gravity no longer. You fall.\r\n\r\n",ch);
    char_from_room(ch);
    char_to_room(ch,room);
    GET_HIT(ch) -= GET_WEIGHT(ch); /* damage depends on weight */
    look_at_room(ch,0);
    act("$n falls in from above.",FALSE,ch,0,0,TO_ROOM);
    }

  line = any_one_arg(argument, arg);

  /* just drop to next line for hitting CR */
  if (!*arg)
    return;

    for (length = strlen(arg), cmd = 0; *cmd_info[cmd].command != '\n'; cmd++)
      if (!strncmp(cmd_info[cmd].command, arg, length))
        if (GET_LEVEL(ch) >= cmd_info[cmd].minimum_level)
	  break;

  if (*cmd_info[cmd].command == '\n')
    send_to_char("Huh?!?\r\n", ch);
  else if (PLR_FLAGGED(ch, PLR_FROZEN) && GET_LEVEL(ch) < LVL_IMPL)
    send_to_char("You try, but the mind-numbing cold prevents you...\r\n", ch);
  else if (cmd_info[cmd].command_pointer == NULL)
    send_to_char("Sorry, that command hasn't been implemented yet.\r\n", ch);
  else if (IS_NPC(ch) && cmd_info[cmd].minimum_level >= LVL_IS_GOD && cmd_info[cmd].minimum_level < LVL_IMPL)
    send_to_char("You can't use immortal commands while switched.\r\n", ch);
  else if (GET_POS(ch) < cmd_info[cmd].minimum_position && GET_LEVEL(ch)<LVL_IMPL)
    switch (GET_POS(ch)) {
    case POS_DEAD:
      send_to_char("Lie still; you are DEAD!!! :-(\r\n", ch);
      break;
    case POS_INCAP:
    case POS_MORTALLYW:
      send_to_char("You are in a pretty bad shape, unable to do anything!\r\n", ch);
      break;
    case POS_STUNNED:
      send_to_char("All you can do right now is think about the stars!\r\n", ch);
      break;
    case POS_SLEEPING:
      send_to_char("In your dreams, or what?\r\n", ch);
      break;
    case POS_RESTING:
      send_to_char("Nah... You feel too relaxed to do that..\r\n", ch);
      break;
    case POS_SITTING:
      send_to_char("Maybe you should get on your feet first?\r\n", ch);
      break;
    case POS_FIGHTING:
      send_to_char("No way!  You're fighting for your life!\r\n", ch);
      break;
    }
  else if (no_specials || !special(ch, cmd, line))
    ((*cmd_info[cmd].command_pointer) (ch, line, cmd, cmd_info[cmd].subcmd));
}

/**************************************************************************
 * Routines to handle aliasing                                             *
  **************************************************************************/


struct alias *find_alias(struct alias * alias_list, char *str)
{
  while (alias_list != NULL) {
    if (*str == *alias_list->alias)	/* hey, every little bit counts :-) */
      if (!strcmp(str, alias_list->alias))
	return alias_list;

    alias_list = alias_list->next;
  }

  return NULL;
}


void free_alias(struct alias * a)
{
  if (a->alias)
    free(a->alias);
  if (a->replacement)
    free(a->replacement);
  free(a);
}


/* The interface: do_alias */
ACMD(do_alias)
{
  char *repl;
  char alias_buf[MAX_STRING_LENGTH*2], temp_buf[512];
  int total;
  struct alias *a, *temp;

  if (IS_NPC(ch))
    return;

  repl = any_one_arg(argument, arg);
  total = 0;
  /*strcat(alias_buf,"");*/
  *alias_buf = '\0';
  if (!*arg) {
    send_to_char("Currently defined aliases:\r\n", ch);
    if ((a = GET_ALIASES(ch)) == NULL)
      send_to_char(" None.\r\n", ch);
    else {
      while (a != NULL) {
	sprintf(temp_buf, "%-15s %s%s%s\r\n", a->alias, CCCYN(ch, C_NRM), a->replacement, CCNRM(ch, C_NRM));
	strcat(alias_buf,temp_buf);
	/*send_to_char(buf, ch);*/
	a = a->next;
	total++;
      }
	sprintf(temp_buf, "Total Aliases: %d\r\n", total);
	strcat(alias_buf,temp_buf);
	page_string(ch->desc, alias_buf, 1);
    }
  } else {
    if ((a = find_alias(GET_ALIASES(ch), arg)) != NULL) {
      REMOVE_FROM_LIST(a, GET_ALIASES(ch), next);
      free_alias(a);
    }
    if (!*repl) {
      if (a == NULL)
	send_to_char("No such alias.\r\n", ch);
      else
	send_to_char("Alias deleted.\r\n", ch);
    } else {
      if (!str_cmp(arg, "alias")) {
	send_to_char("You can't alias 'alias'.\r\n", ch);
	return;
      }
      CREATE(a, struct alias, 1);
      a->alias = str_dup(arg);
      delete_doubledollar(repl);
      a->replacement = str_dup(repl);
      if (strchr(repl, ALIAS_SEP_CHAR) || strchr(repl, ALIAS_VAR_CHAR))
	a->type = ALIAS_COMPLEX;
      else
	a->type = ALIAS_SIMPLE;
      a->next = GET_ALIASES(ch);
      GET_ALIASES(ch) = a;
      send_to_char("Alias added.\r\n", ch);
    }
  }
}

/*
 * Valid numeric replacements are only &1 .. &9 (makes parsing a little
 * easier, and it's not that much of a limitation anyway.)  Also valid
 * is "&*", which stands for the entire original line after the alias.
 * ";" is used to delimit commands.
 */
#define NUM_TOKENS       9

void perform_complex_alias(struct txt_q *input_q, char *orig, struct alias *a)
{
  struct txt_q temp_queue;
  char *tokens[NUM_TOKENS], *temp, *write_point;
  int num_of_tokens = 0, num;

  /* First, parse the original string */
  temp = strtok(strcpy(buf2, orig), " ");
  while (temp != NULL && num_of_tokens < NUM_TOKENS) {
    tokens[num_of_tokens++] = temp;
    temp = strtok(NULL, " ");
  }

  /* initialize */
  write_point = buf;
  temp_queue.head = temp_queue.tail = NULL;

  /* now parse the alias */
  for (temp = a->replacement; *temp; temp++) {
    if (*temp == ALIAS_SEP_CHAR) {
      *write_point = '\0';
      write_to_q(buf, &temp_queue, 1);
      write_point = buf;
    } else if (*temp == ALIAS_VAR_CHAR) {
      temp++;
      if ((num = *temp - '1') < num_of_tokens && num >= 0) {
	strcpy(write_point, tokens[num]);
	write_point += strlen(tokens[num]);
      } else if (*temp == ALIAS_GLOB_CHAR) {
	strcpy(write_point, orig);
	write_point += strlen(orig);
      } else
	if ((*(write_point++) = *temp) == '$') /* redouble $ for act safety */
	  *(write_point++) = '$';
    } else
      *(write_point++) = *temp;
  }

  *write_point = '\0';
  write_to_q(buf, &temp_queue, 1);

  /* push our temp_queue on to the _front_ of the input queue */
  if (input_q->head == NULL)
    *input_q = temp_queue;
  else {
    temp_queue.tail->next = input_q->head;
    input_q->head = temp_queue.head;
  }
}


/*
 * Given a character and a string, perform alias replacement on it.
 *
 * Return values:
 *   0: String was modified in place; call command_interpreter immediately.
 *   1: String was _not_ modified in place; rather, the expanded aliases
 *      have been placed at the front of the character's input queue.
 */
int perform_alias(struct descriptor_data * d, char *orig)
{
  char first_arg[MAX_INPUT_LENGTH], *ptr;
  struct alias *a, *tmp;

  /* bail out immediately if the guy doesn't have any aliases */
  if ((tmp = GET_ALIASES(d->character)) == NULL)
    return 0;

  /* find the alias we're supposed to match */
  ptr = any_one_arg(orig, first_arg);

  /* bail out if it's null */
  if (!*first_arg)
    return 0;

  /* if the first arg is not an alias, return without doing anything */
  if ((a = find_alias(tmp, first_arg)) == NULL)
    return 0;

  if (a->type == ALIAS_SIMPLE) {
    strcpy(orig, a->replacement);
    return 0;
  } else {
    perform_complex_alias(&d->input, ptr, a);
    return 1;
  }
}



/***************************************************************************
 * Various other parsing utilities                                         *
 **************************************************************************/

/*
 * searches an array of strings for a target string.  "exact" can be
 * 0 or non-0, depending on whether or not the match must be exact for
 * it to be returned.  Returns -1 if not found; 0..n otherwise.  Array
 * must be terminated with a '\n' so it knows to stop searching.
 */
int search_block(char *arg, char **list, bool exact)
{
  register int i, l;

  /* Make into lower case, and get length of string */
  for (l = 0; *(arg + l); l++)
    *(arg + l) = LOWER(*(arg + l));

  if (exact) {
    for (i = 0; **(list + i) != '\n'; i++)
      if (!strcmp(arg, *(list + i)))
	return (i);
  } else {
    if (!l)
      l = 1;			/* Avoid "" to match the first available
				 * string */
    for (i = 0; **(list + i) != '\n'; i++)
      if (!strncmp(arg, *(list + i), l))
	return (i);
  }

  return -1;
}


int is_number(char *str)
{
  while (*str)
    if (!isdigit(*(str++)))
      return 0;

  return 1;
}


void skip_spaces(char **string)
{
  for (; **string && isspace(**string); (*string)++);
}


char *delete_doubledollar(char *string)
{
  char *read, *write;

  if ((write = strchr(string, '$')) == NULL)
    return string;

  read = write;

  while (*read)
    if ((*(write++) = *(read++)) == '$')
      if (*read == '$')
	read++;

  *write = '\0';

  return string;
}


int fill_word(char *argument)
{
  return (search_block(argument, fill, TRUE) >= 0);
}


int reserved_word(char *argument)
{
  return (search_block(argument, reserved, TRUE) >= 0);
}
/*

char *one_arg_strn(char *argument, char *first_arg)
{
  char *begin = first_arg;
  int counter = 0;

  do {
    while(isspace(*argument)) argument++;
    while (!isspace(*argument))
    {
      argument++;
      counter++;
    }
  } while (fill_word(begin));
  strncpy(first_arg,begin,counter);
  return argument;
}

 * copy the first non-fill-word, space-delimited argument of 'argument'
 * to 'first_arg'; return a pointer to the remainder of the string.
 */
char *one_argument(char *argument, char *first_arg)
{
  char *begin = first_arg;

  do {
    skip_spaces(&argument);

    first_arg = begin;
    while (*argument && !isspace(*argument)) {
      *(first_arg++) = LOWER(*argument);
      argument++;
    }

    *first_arg = '\0';
  } while (fill_word(begin));

  return argument;
}


/* same as one_argument except that it doesn't ignore fill words */
char *any_one_arg(char *argument, char *first_arg)
{
  skip_spaces(&argument);

  while (*argument && !isspace(*argument)) {
    *(first_arg++) = LOWER(*argument);
    argument++;
  }

  *first_arg = '\0';

  return argument;
}


/*
 * Same as one_argument except that it takes two args and returns the rest;
 * ignores fill words
 */
char *two_arguments(char *argument, char *first_arg, char *second_arg)
{
  return one_argument(one_argument(argument, first_arg), second_arg);	/* :-) */
}


/*
 * determine if a given string is an abbreviation of another
 */
int is_abbrev(char *arg1, char *arg2)
{
  if (!*arg1)
    return 0;

  for (; *arg1 && *arg2; arg1++, arg2++)
    if (LOWER(*arg1) != LOWER(*arg2))
      return 0;

 if (!*arg1)
    return 1;
  else
    return 0;
}



/* return first space-delimited token in arg1; remainder of string in arg2 */
void half_chop(char *string, char *arg1, char *arg2)
{
  char *temp;

  temp = any_one_arg(string, arg1);
  skip_spaces(&temp);
  strcpy(arg2, temp);
}



/* Used in specprocs, mostly.  (Exactly) matches "command" to cmd number */
int find_command(char *command)
{
  int cmd;

  for (cmd = 0; *cmd_info[cmd].command != '\n'; cmd++)
    if (!strcmp(cmd_info[cmd].command, command))
      return cmd;

  return -1;
}


int special(struct char_data * ch, int cmd, char *arg)
{
  register struct obj_data *i;
  register struct char_data *k;
  int j;

  /* special in room? */
  if (GET_ROOM_SPEC(ch->in_room) != NULL)
    if (GET_ROOM_SPEC(ch->in_room) (ch, world + ch->in_room, cmd, arg))
      return 1;

  /* special in equipment list? */
  for (j = 0; j < NUM_WEARS; j++)
    if (ch->equipment[j] && GET_OBJ_SPEC(ch->equipment[j]) != NULL)
      if (GET_OBJ_SPEC(ch->equipment[j]) (ch, ch->equipment[j], cmd, arg))
	return 1;

  /* special in inventory? */
  for (i = ch->carrying; i; i = i->next_content)
    if (GET_OBJ_SPEC(i) != NULL)
      if (GET_OBJ_SPEC(i) (ch, i, cmd, arg))
	return 1;

  /* special in mobile present? */
  for (k = world[ch->in_room].people; k; k = k->next_in_room)
    if (GET_MOB_SPEC(k) != NULL)
      if (GET_MOB_SPEC(k) (ch, k, cmd, arg))
	return 1;

  /* special in object present? */
  for (i = world[ch->in_room].contents; i; i = i->next_content)
    if (GET_OBJ_SPEC(i) != NULL)
      if (GET_OBJ_SPEC(i) (ch, i, cmd, arg))
	return 1;

  return 0;
}



/* *************************************************************************
*  Stuff for controlling the non-playing sockets (get name, pwd etc)       *
************************************************************************* */


/* locate entry in p_table with entry->name == name. -1 mrks failed search */
int find_name(char *name)
{
  int i;

  for (i = 0; i <= top_of_p_table; i++) {
    if (!str_cmp((player_table + i)->name, name))
      return i;
  }

  return -1;
}


int _parse_name(char *arg, char *name)
{
  int i;

  if (strlen(arg) > MAX_NAME_LENGTH-1)
    return(1);

  /* skip whitespaces */
  for (; isspace(*arg); arg++);

  for (i = 0; (*name = *arg); arg++, i++, name++)
    if (!isalpha(*arg))
      return 1;

  if (!i)
    return 1;

  return 0;
}



/* deal with newcomers and other non-playing sockets */
void nanny(struct descriptor_data * d, char *arg)
{ 
  char buf[100];
  int player_i, load_result;
  char tmp_name[MAX_NAME_LENGTH];
  struct char_file_u tmp_store;
  struct char_data *tmp_ch;
  struct descriptor_data *k, *next;
  extern struct descriptor_data *descriptor_list;
  extern sh_int world_start_room[3];
//  extern sh_int r_mortal_start_room;
  extern sh_int r_immort_start_room;
  extern sh_int r_frozen_start_room;
  extern char class_menu;
    extern const char *class_stat_menu;
  extern char class_info; 
  extern void build_class_menu(); 
  int class_num=0;
  sh_int load_room;
  int i,j, result;

  int load_char(char *name, struct char_file_u * char_element);
  int parse_class(char *arg);

  skip_spaces(&arg);

	/* reset the login time at each stage of the login */
	d->login_time = time(0);

  switch (STATE(d)) {
  case CON_GET_NAME:		/* wait for input of name */
    if (d->character == NULL) {
      CREATE(d->character, struct char_data, 1);
      clear_char(d->character);
      CREATE(d->character->player_specials, struct player_special_data, 1);
      d->character->desc = d;
    }
    if (!*arg)
      close_socket(d);
    else {
      if ((_parse_name(arg, tmp_name)) || strlen(tmp_name) < 2 ||
	  strlen(tmp_name) > MAX_NAME_LENGTH ||
	  fill_word(strcpy(buf, tmp_name)) || reserved_word(buf)) {
	SEND_TO_Q("Invalid name, please try another.\r\n"
		  "Name: ", d);
	return;
      }
      if ((player_i = load_char(tmp_name, &tmp_store)) > -1) {
	d->pos = player_i;
	store_to_char(&tmp_store, d->character);

	if (PLR_FLAGGED(d->character, PLR_DELETED)) {
	  free_char(d->character);
	  CREATE(d->character, struct char_data, 1);
	  clear_char(d->character);
	  d->character->desc = d;
	  CREATE(d->character->player.name, char, strlen(tmp_name) + 1);
	  strcpy(d->character->player.name, CAP(tmp_name));
	  sprintf(buf, "Did I get that right, %s (Y/N)? ", tmp_name);
	  SEND_TO_Q(buf, d);
	  STATE(d) = CON_NAME_CNFRM;
	} else {
	  strcpy(d->pwd, tmp_store.pwd);

	  /* undo it just in case they are set */
	  REMOVE_BIT(PLR_FLAGS(d->character),
		     PLR_WRITING | PLR_MAILING | PLR_CRYO);

	  SEND_TO_Q("Password: ", d);
	  echo_off(d);

	  STATE(d) = CON_PASSWORD;
	}
      } else {
	/* player unknown -- make new character */

	if (!Valid_Name(tmp_name)) {
	  SEND_TO_Q("Invalid name, please try another.\r\n", d);
	  SEND_TO_Q("Name: ", d);
	  return;
	}
	CREATE(d->character->player.name, char, strlen(tmp_name) + 1);
	strcpy(d->character->player.name, CAP(tmp_name));

	sprintf(buf, "Did I get that right, %s (Y/N)? ", tmp_name);
	SEND_TO_Q(buf, d);
	STATE(d) = CON_NAME_CNFRM;
      }
    }
    break;
  case CON_NAME_CNFRM:		/* wait for conf. of new name	 */
    if (*arg == 'y' || *arg == 'Y') {
      if (isbanned(d->host) >= BAN_NEW) {
	sprintf(buf, "Request for new char %s denied from [%s] (siteban)",
		GET_NAME(d->character), d->host);
	mudlog(buf, NRM, LVL_GOD, TRUE);
	SEND_TO_Q("Sorry, new characters are not allowed from your site!\r\n", d);
	SEND_TO_Q("Please email abuse@mud.alphalink.com.au for an explanation.\r\n", d);
	STATE(d) = CON_CLOSE;
	return;
      }
      if (restrict) {
	SEND_TO_Q("Sorry, new players can't be created at the moment.\r\n", d);
	sprintf(buf, "Request for new char %s denied from %s (wizlock)",
		GET_NAME(d->character), d->host);
	mudlog(buf, NRM, LVL_GOD, TRUE);
	STATE(d) = CON_CLOSE;
	return;
      }
      SEND_TO_Q("New character.\r\n", d);
      sprintf(buf, "Give me a password for %s: ", GET_NAME(d->character));
      SEND_TO_Q(buf, d);
      echo_off(d);
      STATE(d) = CON_NEWPASSWD;
    } else if (*arg == 'n' || *arg == 'N') {
      SEND_TO_Q("Okay, what IS it, then? ", d);
      free(d->character->player.name);
      d->character->player.name = NULL;
      STATE(d) = CON_GET_NAME;
    } else {
      SEND_TO_Q("Please type Yes or No: ", d);
    }
    break;
  case CON_PASSWORD:		/* get pwd for known player	 */
    /* turn echo back on */
    echo_on(d);

    if (!*arg)
      close_socket(d);
    else {
      if (strncmp(CRYPT(arg, d->pwd), d->pwd, MAX_PWD_LENGTH)) {
	sprintf(buf, "Bad PW: %s [%s]", GET_NAME(d->character), d->host);
	if (GET_LEVEL(d->character) > LVL_OWNER)
	  mudlog(buf, BRF, 201, TRUE);
	else
	  mudlog(buf, BRF, LVL_GOD, TRUE);
	GET_BAD_PWS(d->character)++;
	save_char(d->character, NOWHERE);
	if (++(d->bad_pws) >= 3) {	/* 3 strikes and you're out. */
	  SEND_TO_Q("Wrong password... disconnecting.\r\n", d);
	  STATE(d) = CON_CLOSE;
	} else {
	  SEND_TO_Q("Wrong password.\r\nPassword: ", d);
	  echo_off(d);
	}
	return;
      }
      load_result = GET_BAD_PWS(d->character);
      GET_BAD_PWS(d->character) = 0;
      save_char(d->character, NOWHERE);

      if (isbanned(d->host) == BAN_SELECT &&
	  !PLR_FLAGGED(d->character, PLR_SITEOK)) {
	SEND_TO_Q("Sorry, this char has not been cleared for login from your site!\r\n", d);
	SEND_TO_Q("Please email abuse@mud.alphalink.com.au for details.\r\n", d);
	STATE(d) = CON_CLOSE;
	sprintf(buf, "Connection attempt for %s denied from %s",
		GET_NAME(d->character), d->host);
	mudlog(buf, NRM, LVL_GOD, TRUE);
	return;
      }
      if (GET_LEVEL(d->character) < restrict) {
	SEND_TO_Q("The game is temporarily restricted.. try again later.\r\n", d);
	STATE(d) = CON_CLOSE;
	sprintf(buf, "Request for login denied for %s [%s] (wizlock)",
		GET_NAME(d->character), d->host);
	mudlog(buf, NRM, LVL_GOD, TRUE);
	return;
      }
      /* first, check for switched characters */
      for (tmp_ch = character_list; tmp_ch; tmp_ch = tmp_ch->next)
	if (IS_NPC(tmp_ch) && tmp_ch->desc && tmp_ch->desc->original &&
	    GET_IDNUM(tmp_ch->desc->original) == GET_IDNUM(d->character)) {
	  SEND_TO_Q("Disconnecting.", tmp_ch->desc);
	  free_char(d->character);
	  d->character = tmp_ch->desc->original;
	  d->character->desc = d;
	  tmp_ch->desc->character = NULL;
	  tmp_ch->desc->original = NULL;
	  STATE(tmp_ch->desc) = CON_CLOSE;
	  d->character->char_specials.timer = 0;
	  SEND_TO_Q("Reconnecting to unswitched char.", d);
	  REMOVE_BIT(PLR_FLAGS(d->character), PLR_MAILING | PLR_WRITING);
	  STATE(d) = CON_PLAYING;
          
          if (GET_LEVEL(d->character)<LVL_IMPL)
           {
	    sprintf(buf, "%s [%s] has reconnected.",
		  GET_NAME(d->character), d->host);
	    mudlog(buf, NRM, MAX(LVL_GOD, GET_INVIS_LEV(d->character)),TRUE);

           }
        

        return;
	}
      /* now check for linkless and usurpable */
      for (tmp_ch = character_list; tmp_ch; tmp_ch = tmp_ch->next)
	if (!IS_NPC(tmp_ch) &&
	    GET_IDNUM(d->character) == GET_IDNUM(tmp_ch)) {
	  if (!tmp_ch->desc) {
	    SEND_TO_Q("Reconnecting.\r\n", d);
	    act("$n has reconnected.", TRUE, tmp_ch, 0, 0, TO_ROOM);
if (GET_LEVEL(d->character)<LVL_IMPL)
{
	    sprintf(buf, "%s [%s] has reconnected.", GET_NAME(d->character), d->host);
	    mudlog(buf, NRM, MAX(LVL_GOD, GET_INVIS_LEV(d->character)), TRUE);
            sprintf(buf, "%s has reconnected.", GET_NAME(d->character));
	    info_channel ( buf , d->character ) ;	    

}
	  } else {
if (GET_LEVEL(d->character)<LVL_IMPL)
{
	    sprintf(buf, "%s has re-logged in ... disconnecting old socket.",
		    GET_NAME(tmp_ch));
	    mudlog(buf, NRM, MAX(LVL_GOD, GET_INVIS_LEV(tmp_ch)), TRUE);
}
	    SEND_TO_Q("This body has been usurped!\r\n", tmp_ch->desc);
	    STATE(tmp_ch->desc) = CON_CLOSE;
	    tmp_ch->desc->character = NULL;
	    tmp_ch->desc = NULL;
	    SEND_TO_Q("You take over your own body, already in use!\r\n", d);
	    act("$n suddenly keels over in pain, surrounded by a white aura...\r\n"
		"$n's body has been taken over by a new spirit!",
		TRUE, tmp_ch, 0, 0, TO_ROOM);
	  }

	  free_char(d->character);
	  tmp_ch->desc = d;
	  d->character = tmp_ch;
	  tmp_ch->char_specials.timer = 0;
	  REMOVE_BIT(PLR_FLAGS(d->character), PLR_MAILING | PLR_WRITING);
	  STATE(d) = CON_PLAYING;
          if (has_mail(GET_IDNUM(d->character)))
            send_to_char("You have mail waiting.\r\n",d->character);
	  return;
	}
      if (GET_LEVEL(d->character) >= LVL_IS_GOD)
	SEND_TO_Q(imotd, d);
      else
	SEND_TO_Q(motd, d);
if (GET_LEVEL(d->character)<LVL_IMPL)
{
      sprintf(buf, "%s [%s] has connected.", GET_NAME(d->character), d->host);
      mudlog(buf, BRF, MAX(LVL_GOD, GET_INVIS_LEV(d->character)), TRUE);
}
      if (load_result) {
	sprintf(buf, "\r\n\r\n\007\007\007"
		"%s%d LOGIN FAILURE%s SINCE LAST SUCCESSFUL LOGIN.%s\r\n",
		CCRED(d->character, C_SPR), load_result,
		(load_result > 1) ? "S" : "", CCNRM(d->character, C_SPR));
	SEND_TO_Q(buf, d);
      }
      SEND_TO_Q("\r\n\n&g***&n PRESS RETURN: ", d);
      STATE(d) = CON_RMOTD;
    }
    break;

  case CON_NEWPASSWD:
  case CON_CHPWD_GETNEW:
    if (!*arg || strlen(arg) > MAX_PWD_LENGTH || strlen(arg) < 3 ||
	!str_cmp(arg, GET_NAME(d->character))) {
      SEND_TO_Q("\r\nIllegal password.\r\n", d);
      SEND_TO_Q("Password: ", d);
      return;
    }
    strncpy(d->pwd, CRYPT(arg, d->character->player.name), MAX_PWD_LENGTH);
    *(d->pwd + MAX_PWD_LENGTH) = '\0';

    SEND_TO_Q("\r\nPlease retype password: ", d);
    if (STATE(d) == CON_NEWPASSWD)
      STATE(d) = CON_CNFPASSWD;
    else
      STATE(d) = CON_CHPWD_VRFY;

    break;

  case CON_CNFPASSWD:
  case CON_CHPWD_VRFY:
    if (strncmp(CRYPT(arg, d->pwd), d->pwd, MAX_PWD_LENGTH)) {
      SEND_TO_Q("\r\nPasswords don't match... start over.\r\n", d);
      SEND_TO_Q("Password: ", d);
      if (STATE(d) == CON_CNFPASSWD)
	STATE(d) = CON_NEWPASSWD;
      else
	STATE(d) = CON_CHPWD_GETNEW;
      return;
    }
    echo_on(d);

    if (STATE(d) == CON_CNFPASSWD) {
      SEND_TO_Q("What is your sex (M/F)? ", d);
      STATE(d) = CON_QSEX;
    } else {
      save_char(d->character, NOWHERE);
      echo_on(d);
      SEND_TO_Q("\r\nDone.\n\r", d);
      SEND_TO_Q(MENU, d);
      STATE(d) = CON_MENU;
    }

    break;
    
  case CON_QSEX:		/* query sex of new user	 */
    switch (*arg) {
    case 'm':
    case 'M':
      d->character->player.sex = SEX_MALE;
      break;
    case 'f':
    case 'F':
      d->character->player.sex = SEX_FEMALE;
      break;
    default:
      SEND_TO_Q("That is not a sex..\r\n"
		"What IS your sex? ", d);
      return;
      break;
    }
    build_class_menu();
    SEND_TO_Q(&class_menu, d);
    SEND_TO_Q("\r\nRace: ", d);
    STATE(d) = CON_QRACE;
    break;

  case CON_QRACE:
   if (!strcmp(arg,"?"))
   {
      build_class_menu();
      SEND_TO_Q(&class_info, d);
      SEND_TO_Q("\r\nRace: ", d);
      return; 
   }  
   if ((GET_CLASS(d->character) = parse_class(arg)) == CLASS_UNDEFINED) {
      SEND_TO_Q("\r\nThat's not a race.\r\nRace: ", d);
      return;
    }
    if (d->pos < 0)
      d->pos = create_entry(GET_NAME(d->character));
    init_char(d->character);
/* JA shouldn't save it here, seems to leave stats undefined if
   the player goes linkless before getting to the next state */
/*    save_char(d->character, NOWHERE); */

    sprintf(buf, "%s: new player.", GET_NAME(d->character));
    mudlog(buf, NRM, LVL_ETRNL1, TRUE);
    for (i=0;i<6;i++)
      d->character->player_specials->saved.stat_order[i]=0;  
    d->character->desc->stat_count=0;
    SEND_TO_Q(class_stat_menu, d);
    SEND_TO_Q("\r\nSelect one of the above [S,C,T,M,W]: ", d);
    STATE(d) = CON_QCLASS;
    break;

 case CON_QCLASS:
      *arg = LOWER(*arg);
      switch (*arg) {
      case 'm':
        class_num=1;
        break;
      case 'c':
        class_num=2;
        break;
      case 'w':
        class_num=3;
        break;
      case 't':
        class_num=4;
        break;
      case 's':
	class_num=5;
        break;    
      default:
        class_num=0;
        break;
    }

    if (class_num == 0) {
      SEND_TO_Q("\r\nThat's not a valid stat order selection.\r\nSelect one of the above [S,C,T,M,W]: ", d);
      return;
    }
 
    switch (class_num)
    {
      case 1:
        d->character->player_specials->saved.stat_order[0]=4;
	d->character->player_specials->saved.stat_order[1]=5;
	d->character->player_specials->saved.stat_order[2]=3;
	d->character->player_specials->saved.stat_order[3]=1;
	d->character->player_specials->saved.stat_order[4]=2;
	d->character->player_specials->saved.stat_order[5]=6;
	break;
      case 2:
        d->character->player_specials->saved.stat_order[0]=5;
	d->character->player_specials->saved.stat_order[1]=4;
	d->character->player_specials->saved.stat_order[2]=1;
	d->character->player_specials->saved.stat_order[3]=3;
	d->character->player_specials->saved.stat_order[4]=2;
	d->character->player_specials->saved.stat_order[5]=6;
	break;
     case 3:
        d->character->player_specials->saved.stat_order[0]=1;
	d->character->player_specials->saved.stat_order[1]=3;
	d->character->player_specials->saved.stat_order[2]=2;
	d->character->player_specials->saved.stat_order[3]=5;
	d->character->player_specials->saved.stat_order[4]=4;
	d->character->player_specials->saved.stat_order[5]=6;
	break;

     case 4:
        d->character->player_specials->saved.stat_order[0]=3;
	d->character->player_specials->saved.stat_order[1]=1;
	d->character->player_specials->saved.stat_order[2]=2;
	d->character->player_specials->saved.stat_order[3]=4;
	d->character->player_specials->saved.stat_order[4]=5;
	d->character->player_specials->saved.stat_order[5]=6;
	break;
	
    default:      
        SEND_TO_Q("\r\nChoose first stat 1-STR 2-CON 3-DEX 4-INT 5-WIS 6-CHA :", d);
        STATE(d) = CON_QSTAT;
        return;	
    }  
/* JA save here now since it was leaving undef stats if the player
   went linkless in previous state */
    save_char(d->character, NOWHERE);
/*    SEND_TO_Q(buff, d);
    SEND_TO_Q("\r\n1 Reroll Stats\r\n2 Accept Stats.\r\nMake youy selection[1,2]> ", d);
    STATE(d) = CON_REROLL;*/
    SEND_TO_Q(motd, d);
    SEND_TO_Q("\r\n\n&g***&n PRESS RETURN: ", d);
    STATE(d) = CON_RMOTD;
    break;

  case CON_QSTAT:
    if (is_number(arg)) 
    {
      d->character->player_specials->saved.stat_order[d->character->desc->stat_count] = atoi(arg);
      if ( (d->character->player_specials->saved.stat_order[d->character->desc->stat_count] > 0) && 
	  (d->character->player_specials->saved.stat_order[d->character->desc->stat_count] < 7) )
      {
        j=0;
        for (i=0;i<6;i++)
          if (i != d->character->desc->stat_count)
            if (d->character->player_specials->saved.stat_order[d->character->desc->stat_count]==d->character->player_specials->saved.stat_order[i])
              j=1;
        if (j==0)
        { 
          if (++d->character->desc->stat_count > 5)
          {
	    SEND_TO_Q(motd,d);
            SEND_TO_Q("\r\n\n&g*****&n PRESS RETURN:",d);
            STATE(d) = CON_RMOTD;
            return;
          }
          sprintf(buf,"\r\nChoose stat[%d]:",d->character->desc->stat_count+1);
          SEND_TO_Q(buf,d); 
          return;
        }
      }   
    }
    sprintf(buf,"\r\nInvalid. Stat[%d] :",d->character->desc->stat_count+1);
    SEND_TO_Q(buf,d);
    d->character->player_specials->saved.stat_order[d->character->desc->stat_count] = 0;
    break;

  case CON_RMOTD:		/* read CR after printing motd	 */
    SEND_TO_Q("\r\n",d);
    do_date(d->character,"",1,1);
    SEND_TO_Q(MENU, d);
    STATE(d) = CON_MENU;
    break;

  case CON_MENU:		/* get selection from main menu	 */
    switch (*arg) {
    case '0':
      close_socket(d);
      break;

    case '1':

      /* this code is to prevent people from multiply logging in */
      for (k = descriptor_list; k; k = next) {
	next = k->next;
	if (!k->connected && k->character &&
	    !str_cmp(GET_NAME(k->character), GET_NAME(d->character))) {
	  SEND_TO_Q("Your character has been deleted.\r\n", d);
	  STATE(d) = CON_CLOSE;
	  return;
	}
      }
      reset_char(d->character);
      if (PLR_FLAGGED(d->character, PLR_INVSTART)) {
	GET_INVIS_LEV(d->character) = GET_LEVEL(d->character);
        GET_INVIS_TYPE(d->character) = 0; /* default to normal invis */
      }
      /* if ((load_result = Crash_load(d->character)))
	 d->character->in_room = NOWHERE;  */
      save_char(d->character, NOWHERE);
      send_to_char(WELC_MESSG, d->character);
      d->character->next = character_list;
      character_list = d->character;

      // DM - Used to load to entry (vnum 501), changed to skip entry world
      // See SPECIAL(recep_guard) in spec_procs.c for old details 

      if(real_room(ENTRY_ROOM(d->character,1)) < 0)
        ENTRY_ROOM(d->character,1) = world_start_room[0];
      if(real_room(ENTRY_ROOM(d->character,2)) < 0)
        ENTRY_ROOM(d->character,2) = world_start_room[1];
      if(real_room(ENTRY_ROOM(d->character,3)) < 0)
        ENTRY_ROOM(d->character,3) = world_start_room[2];

      if (GET_LEVEL(d->character) >= LVL_ANGEL) 
        load_room = r_immort_start_room;
      else if (PLR_FLAGGED(d->character, PLR_FROZEN))
        load_room = r_frozen_start_room;
      else 
        load_room = real_room(ENTRY_ROOM(d->character,1)); 
        // load_room = r_mortal_start_room;
	
      sprintf ( buf , "%s has entered the game." , GET_NAME (d->character) ); 
      info_channel ( buf , d->character );

      char_to_room(d->character, load_room);
      act("$n has entered the game.", TRUE, d->character, 0, 0, TO_ROOM);

      save_char(d->character,NOWHERE);

      STATE(d) = CON_PLAYING;

      // New Chars
      if (!GET_LEVEL(d->character)) {
	do_start(d->character);

/* JA Set some flags to help newbies */
        GET_GOLD(d->character) = 2500;
        GET_WIMP_LEV(d->character) = 7;
        result = PRF_TOG_CHK(d->character, PRF_AUTOEXIT);
        SET_BIT(PRF_FLAGS(d->character), PRF_DISPHP | PRF_DISPMANA | PRF_DISPMOVE);
        obj_to_char(read_object(251, VIRTUAL), d->character);

        newbie_equip(d->character);
	send_to_char(START_MESSG, d->character);
      } else { 

	// DM - Load the eq
        if (Crash_load(d->character) == 2) {
          send_to_char("You could not afford rent here!\r\n"
                   "Your possessions have been donated to the Salvation Army!\r\n",d->character);
        }   
      }
      look_at_room(d->character, 0);

      if (has_mail(GET_IDNUM(d->character)))
	send_to_char("You have mail waiting.\r\n", d->character);

      /* DM_exp check if using old exp system on login */
      if (GET_EXP(d->character) > level_exp[GET_LEVEL(d->character)]) {
        #ifdef REIMBURSE_LEVELS
 	  reimburse_levels(d->character);
        #endif
        GET_EXP(d->character) = 0;
        save_char(d->character,NOWHERE);
      }

      /*if (load_result == 2) {	 rented items lost 
	send_to_char("\r\n\007You could not afford your rent!\r\n"
	     "Your possesions have been donated to the Salvation Army!\r\n",
		     d->character);
      }*/
      d->prompt_mode = 1;
      break;

    case '2':
      SEND_TO_Q("Enter the text you'd like others to see when they look at you.\r\n", d);
      SEND_TO_Q("(/s saves /h for help)\r\n", d);
      if (d->character->player.description) {
      SEND_TO_Q("Current description:\r\n", d);
        SEND_TO_Q(d->character->player.description, d);
      /* don't free this now... so that the old description gets loaded */
      /* as the current buffer in the editor */
      /* free(d->character->player.description); */
      /* d->character->player.description = NULL; */
      /* BUT, do setup the ABORT buffer here */
      d->backstr = str_dup(d->character->player.description);
      } 
      d->str = &d->character->player.description;
      d->max_str = EXDSCR_LENGTH;
      STATE(d) = CON_EXDESC;
      break;

    case '3':
      page_string(d, background, 0);
      STATE(d) = CON_RMOTD;
      break;

    case '4':
      SEND_TO_Q("\r\nEnter your old password: ", d);
      echo_off(d);
      STATE(d) = CON_CHPWD_GETOLD;
      break;

    case '5':
      SEND_TO_Q("\r\nEnter your password for verification: ", d);
      echo_off(d);
      STATE(d) = CON_DELCNF1;
      break;

    default:
      SEND_TO_Q("\r\nThat's not a menu choice!\r\n", d);
      SEND_TO_Q(MENU, d);
      break;
    }

    break;

  case CON_CHPWD_GETOLD:
    if (strncmp(CRYPT(arg, d->pwd), d->pwd, MAX_PWD_LENGTH)) {
      SEND_TO_Q("\r\nIncorrect password.\r\n", d);
      echo_on(d);
      SEND_TO_Q(MENU, d);
      STATE(d) = CON_MENU;
      return;
    } else {
      SEND_TO_Q("\r\nEnter a new password: ", d);
      STATE(d) = CON_CHPWD_GETNEW;
      return;
    }
    break;

  case CON_DELCNF1:
    echo_on(d);
    if (strncmp(CRYPT(arg, d->pwd), d->pwd, MAX_PWD_LENGTH)) {
      SEND_TO_Q("\r\nIncorrect password.\r\n", d);
      SEND_TO_Q(MENU, d);
      STATE(d) = CON_MENU;
    } else {
      SEND_TO_Q("\r\nYOU ARE ABOUT TO DELETE THIS CHARACTER PERMANENTLY.\r\n"
		"ARE YOU ABSOLUTELY SURE?\r\n\r\n"
		"Please type \"yes\" to confirm: ", d);
      STATE(d) = CON_DELCNF2;
    }
    break;

  case CON_DELCNF2:
    if (!strcmp(arg, "yes") || !strcmp(arg, "YES")) {
      if (PLR_FLAGGED(d->character, PLR_FROZEN)) {
	SEND_TO_Q("You try to kill yourself, but the ice stops you.\r\n", d);
	SEND_TO_Q("Character not deleted.\r\n\r\n", d);
	STATE(d) = CON_CLOSE;
	return;
      }
      if (GET_LEVEL(d->character) < LVL_GRGOD)
	SET_BIT(PLR_FLAGS(d->character), PLR_DELETED);
      save_char(d->character, NOWHERE);
      Crash_delete_file(GET_NAME(d->character));
      Alias_delete_file(GET_NAME(d->character));
      sprintf(buf, "Character '%s' deleted!\r\n"
	      "Goodbye.\r\n", GET_NAME(d->character));
      SEND_TO_Q(buf, d);
      sprintf(buf, "%s (lev %d) has self-deleted.", GET_NAME(d->character),
	      GET_LEVEL(d->character));
      mudlog(buf, NRM, LVL_GOD, TRUE);
      STATE(d) = CON_CLOSE;
      return;
    } else {
      SEND_TO_Q("\r\nCharacter not deleted.\r\n", d);
      SEND_TO_Q(MENU, d);
      STATE(d) = CON_MENU;
    }
    break;

  case CON_CLOSE:
    close_socket(d);
    break;

  default:
    log("SYSERR: Nanny: illegal state of con'ness; closing connection");
    close_socket(d);
    break;
  }
}

/* DM_exp - reimburse levels for new level system */
void reimburse_levels(struct char_data *ch)
{
  int i, newlevel, oldlevel=GET_LEVEL(ch);
 
  switch (GET_LEVEL(ch)) {
    case 200:
      GET_LEVEL(ch) = newlevel = LVL_OWNER;
      break;
    case 110:
      GET_LEVEL(ch) = newlevel = LVL_GRIMPL;
      break;
    case 100:
      GET_LEVEL(ch) = newlevel = LVL_IMPL;
      break;
    case 99:
      GET_LEVEL(ch) = newlevel = LVL_GRGOD;
      break;
    case 98:
      GET_LEVEL(ch) = newlevel = LVL_GOD;
      break;
    case 80:
      GET_LEVEL(ch) = newlevel = LVL_ANGEL;
      break;
    case 75:           
      GET_LEVEL(ch) = newlevel = LVL_IMMORT;
      break;
    case 70:
      GET_LEVEL(ch) = newlevel = LVL_CHAMP;
      break;
    default:
      /* Check to see chars not above level 70 */ 
      if (GET_LEVEL(ch) > 70) {
	sprintf(buf,"\r\nYou don't seem to fit in for reimbursement due to the new exp/level system.\r\nAsk a god about reimbursement.\r\n\r\n");
        send_to_char(buf,ch);

        sprintf(buf,"Reimburse: denied %s, reason: above level 70.",GET_NAME(ch));
        mudlog(buf,NRM,LVL_GOD,TRUE);
        return;
      }

      newlevel=(LVL_CHAMP*oldlevel)/70;

      /* Gain points */
      #ifdef REIMBURSE_POINTS
        for (i=oldlevel; i<newlevel; i++)
          advance_level(ch);
      #endif

      GET_LEVEL(ch) = newlevel;
      break;
  }
 
  sprintf(buf,"\r\nThis appears to be the first time you have logged onto Primal Mud\r\n");
  sprintf(buf2, "with the new level/exp system. You have been reimbursed from\r\nlevel %d to %d.\r\n\r\n",
        oldlevel,newlevel);
  strcat(buf,buf2);
  strcat(buf, "If you have any problems with this please mudmail or speak to one \r\n");
  strcat(buf, "of the gods.\r\n\r\n");
 
  send_to_char(buf,ch);
 
  sprintf(buf,"Reimburse: %s from %d to %d",GET_NAME(ch), oldlevel, newlevel);
  mudlog(buf,NRM,MAX(LVL_GOD, GET_INVIS_LEV(ch)),TRUE);

  sprintf(buf,"%s reimbursed from level %d to level %d", GET_NAME(ch), oldlevel, newlevel);
  info_channel(buf,ch);       
}
