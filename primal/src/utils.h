/* ************************************************************************
*   File: utils.h                                       Part of CircleMUD *
*  Usage: header file: utility macros and prototypes of utility funcs     *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */


/* external declarations and prototypes **********************************/

extern struct weather_data weather_info;

/* public functions */
int     is_wearing(struct char_data *ch, int item_type);
int     is_carrying(struct char_data *ch, int item_type);
char	*str_dup(const char *source);
int	str_cmp(char *arg1, char *arg2);
int	strn_cmp(char *arg1, char *arg2, int n);
void	pmlog(char *str);
#define log pmlog
int	touch(char *path);
void	mudlog(char *str, char type, ubyte level, byte file);
void    info_channel( char *str , struct char_data *ch );
void	log_death_trap(struct char_data *ch);
int	MAX(int a, int b);
int	MIN(int a, int b);
int	number(int from, int to);
int	dice(int number, int size);
void	sprintbit(long vektor, char *names[], char *result);
void	sprinttype(int type, char *names[], char *result);
int	get_line(FILE *fl, char *buf);
int	get_filename(char *orig_name, char *filename, int mode);
struct time_info_data age(struct char_data *ch);
void die_clone(struct char_data *ch, struct char_data *killer);
int     replace_str(char **string, char *pattern, char *replacement, int rep_all, int max_size);
void    format_text(char **ptr_string, int mode, struct descriptor_data *d, int maxlen);
int 	num_attacks(struct char_data *ch);

/* in magic.c */
bool	circle_follow(struct char_data *ch, struct char_data * victim);

/* in act.comm.c */
int scan_buffer_for_xword(char* buf);

/* in act.informative.c */
void	look_at_room(struct char_data *ch, int mode);

/* in act.movmement.c */
int	do_simple_move(struct char_data *ch, int dir, int following);
int	perform_move(struct char_data *ch, int dir, int following);

/* fight.c */
void raw_kill(struct char_data *ch, struct char_data *killer);

/* in limits.c */
int	mana_limit(struct char_data *ch);
int	hit_limit(struct char_data *ch);
int	move_limit(struct char_data *ch);
int	mana_gain(struct char_data *ch);
int	hit_gain(struct char_data *ch);
int	move_gain(struct char_data *ch);
void	advance_level(struct char_data *ch);
/* DM_demote */
void	demote_level(struct char_data *ch,int newlevel);
void	set_title(struct char_data *ch, char *title);
void	gain_exp(struct char_data *ch, int gain);
void	gain_exp_regardless(struct char_data *ch, int gain);
void	gain_condition(struct char_data *ch, int condition, int value);
void	check_idling(struct char_data *ch);
void	point_update(void);
void	update_pos(struct char_data *victim);
int has_stats_for_skill(struct char_data *ch, int skillnum);
int 	digits(long);

/* various constants *****************************************************/

/* Spec MOB used mainly in magic.c, moved for access in other modules */
#define MOB_MONSUM_I            130
#define MOB_MONSUM_II           140
#define MOB_MONSUM_III          150
#define MOB_GATE_I              160
#define MOB_GATE_II             170
#define MOB_GATE_III            180
#define MOB_ELEMENTAL_BASE      110
#define MOB_CLONE               22300
#define MOB_ZOMBIE              22301
#define MOB_AERIALSERVANT       10     

/* defines for mudlog() */
#define OFF	0
#define BRF	1
#define NRM	2
#define CMP	3

/* get_filename() */
#define CRASH_FILE	0
#define ETEXT_FILE	1

/* breadth-first searching */
#define BFS_ERROR		-1
#define BFS_ALREADY_THERE	-2
#define BFS_NO_PATH		-3

/* mud-life time */
#define SECS_PER_MUD_HOUR       45	
#define SECS_PER_MUD_DAY	(24*SECS_PER_MUD_HOUR)
#define SECS_PER_MUD_MONTH	(35*SECS_PER_MUD_DAY)
#define SECS_PER_MUD_YEAR	(17*SECS_PER_MUD_MONTH)

/* real-life time (remember Real Life?) */
#define SECS_PER_REAL_MIN	60
#define SECS_PER_REAL_HOUR	(60*SECS_PER_REAL_MIN)
#define SECS_PER_REAL_DAY	(24*SECS_PER_REAL_HOUR)
#define SECS_PER_REAL_YEAR	(365*SECS_PER_REAL_DAY)


/* string utils **********************************************************/


#define YESNO(a) ((a) ? "YES" : "NO")
#define ONOFF(a) ((a) ? "ON" : "OFF")

#define LOWER(c)   (((c)>='A'  && (c) <= 'Z') ? ((c)+('a'-'A')) : (c))
#define UPPER(c)   (((c)>='a'  && (c) <= 'z') ? ((c)+('A'-'a')) : (c) )

#define ISNEWL(ch) ((ch) == '\n' || (ch) == '\r') 
#define IF_STR(st) ((st) ? (st) : "\0")
#define CAP(st)  (*(st) = UPPER(*(st)), st)

#define AN(string) (strchr("aeiouAEIOU", *string) ? "an" : "a")


/* memory utils **********************************************************/


#define CREATE(result, type, number)  do {\
	if (!((result) = (type *) calloc ((number), sizeof(type))))\
		{ perror("malloc failure"); abort(); } } while(0)

#define RECREATE(result,type,number) do {\
  if (!((result) = (type *) realloc ((result), sizeof(type) * (number))))\
		{ perror("realloc failure"); abort(); } } while(0)

/* the source previously used the same code in many places to remove an item
 * from a list: if it's the list head, change the head, else traverse the
 * list looking for the item before the one to be removed.  Now, we have a
 * macro to do this.  To use, just make sure that there is a variable 'temp'
 * declared as the same type as the list to be manipulated.  BTW, this is
 * a great application for C++ templates but, alas, this is not C++.  Maybe
 * CircleMUD 4.0 will be...
 */
#define REMOVE_FROM_LIST(item, head, next)	\
   if ((item) == (head))		\
      head = (item)->next;		\
   else {				\
      temp = head;			\
      while (temp && (temp->next != (item))) \
	 temp = temp->next;		\
      if (temp)				\
         temp->next = (item)->next;	\
   }					\


/* basic bitvector utils *************************************************/


#define IS_SET(flag,bit)  ((flag) & (bit))
#define SET_BIT(var,bit)  ((var) |= (bit))
#define REMOVE_BIT(var,bit)  ((var) &= ~(bit))
#define TOGGLE_BIT(var,bit) ((var) = (var) ^ (bit))

#define MOB_FLAGS(ch) ((ch)->char_specials.saved.act)
#define PLR_FLAGS(ch) ((ch)->char_specials.saved.act)
#define PRF_FLAGS(ch) ((ch)->player_specials->saved.pref)
#define AFF_FLAGS(ch) ((ch)->char_specials.saved.affected_by)
#define EXT_FLAGS(ch) ((ch)->player_specials->saved.ext_flag)
#define ROOM_FLAGS(loc) (world[(loc)].room_flags)

#define IS_NPC(ch)  (IS_SET(MOB_FLAGS(ch), MOB_ISNPC))
#define IS_MOB(ch)  (IS_NPC(ch) && ((ch)->nr >-1))

/* DM - clone utils */
#define IS_CLONE(ch) (IS_NPC(ch) && (GET_MOB_VNUM(ch) == MOB_CLONE))
#define IS_CLONE_ROOM(ch) (IS_CLONE(ch) && \
		((ch)->in_room == (ch->master)->in_room))

#define MOB_FLAGGED(ch, flag) (IS_NPC(ch) && IS_SET(MOB_FLAGS(ch), (flag)))
#define PLR_FLAGGED(ch, flag) (!IS_NPC(ch) && IS_SET(PLR_FLAGS(ch), (flag)))
#define AFF_FLAGGED(ch, flag) (IS_SET(AFF_FLAGS(ch), (flag)))
#define PRF_FLAGGED(ch, flag) (IS_SET(PRF_FLAGS(ch), (flag)))
#define EXT_FLAGGED(ch, flag) (IS_SET(EXT_FLAGS(ch), (flag)))
#define ROOM_FLAGGED(loc, flag) (IS_SET(ROOM_FLAGS(loc), (flag)))

/* IS_AFFECTED for backwards compatibility */
#define IS_AFFECTED(ch, skill) (AFF_FLAGGED((ch), (skill)))

#define PLR_TOG_CHK(ch,flag) ((TOGGLE_BIT(PLR_FLAGS(ch), (flag))) & (flag))
#define PRF_TOG_CHK(ch,flag) ((TOGGLE_BIT(PRF_FLAGS(ch), (flag))) & (flag))
#define EXT_TOG_CHK(ch,flag) ((TOGGLE_BIT(EXT_FLAGS(ch), (flag))) & (flag))

/* room utils ************************************************************/


#define IS_DARK(room)  ( !world[room].light && \
                         (IS_SET(world[room].room_flags, ROOM_DARK) || \
                          ( ( (world[room].sector_type & 0x0f) != SECT_INSIDE && \
                              (world[room].sector_type & 0x0f) != SECT_CITY ) && \
                            (weather_info.sunlight == SUN_SET || \
			     weather_info.sunlight == SUN_DARK)) ) )

#define IS_LIGHT(room)  (!IS_DARK(room))

#define GET_ROOM_SPEC(room) ((room) >= 0 ? world[(room)].func : NULL)

/* char utils ************************************************************/


#define ENTRY_ROOM(ch,wrld)  ((ch)->player_specials->saved.world_entry[wrld-1])
#define IN_ROOM(ch)	((ch)->in_room)
#define GET_WAS_IN(ch)	((ch)->was_in_room)
#define GET_AGE(ch)     (age(ch).year)

#define GET_NAME(ch)    (IS_NPC(ch) ? \
			 (ch)->player.short_descr : (ch)->player.name)
#define GET_TITLE(ch)   ((ch)->player.title)
#define GET_LEVEL(ch)   ((ch)->player.level)

/*
 * I wonder if this definition of GET_REAL_LEVEL should be the definition
 * of GET_LEVEL?  JE
 */
#define GET_REAL_LEVEL(ch) \
   (ch->desc && ch->desc->original ? GET_LEVEL(ch->desc->original) : \
    GET_LEVEL(ch))

#define GET_CLASS(ch)   ((ch)->player.class)
#define GET_HOME(ch)	((ch)->player.hometown)
#define GET_HEIGHT(ch)	((ch)->player.height)
#define GET_WEIGHT(ch)	((ch)->player.weight)
#define GET_SEX(ch)	((ch)->player.sex)

#define GET_REAL_STR(ch)     ((ch)->real_abils.str)
#define GET_REAL_ADD(ch)     ((ch)->real_abils.str_add)
#define GET_REAL_DEX(ch)     ((ch)->real_abils.dex)
#define GET_REAL_INT(ch)     ((ch)->real_abils.intel)
#define GET_REAL_WIS(ch)     ((ch)->real_abils.wis)
#define GET_REAL_CON(ch)     ((ch)->real_abils.con)
#define GET_REAL_CHA(ch)     ((ch)->real_abils.cha)

#define GET_AFF_STR(ch)     ((ch)->aff_abils.str)
#define GET_AFF_ADD(ch)     ((ch)->aff_abils.str_add)
#define GET_AFF_DEX(ch)     ((ch)->aff_abils.dex)
#define GET_AFF_INT(ch)     ((ch)->aff_abils.intel)
#define GET_AFF_WIS(ch)     ((ch)->aff_abils.wis)
#define GET_AFF_CON(ch)     ((ch)->aff_abils.con)
#define GET_AFF_CHA(ch)     ((ch)->aff_abils.cha)

#define GET_EXP(ch)	  ((ch)->points.exp)
#define GET_AC(ch)        ((ch)->points.armor)
#define GET_HIT(ch)	  ((ch)->points.hit)
#define GET_MAX_HIT(ch)	  ((ch)->points.max_hit)
#define GET_MOVE(ch)	  ((ch)->points.move)
#define GET_MAX_MOVE(ch)  ((ch)->points.max_move)
#define GET_MANA(ch)	  ((ch)->points.mana)
#define GET_MAX_MANA(ch)  ((ch)->points.max_mana)
#define GET_GOLD(ch)	  ((ch)->points.gold)
#define GET_BANK_GOLD(ch) ((ch)->points.bank_gold)
#define GET_HITROLL(ch)	  ((ch)->points.hitroll)
#define GET_DAMROLL(ch)   ((ch)->points.damroll)

#define GET_POS(ch)	  ((ch)->char_specials.position)
#define GET_IDNUM(ch)	  ((ch)->char_specials.saved.idnum)
#define IS_CARRYING_W(ch) ((ch)->char_specials.carry_weight)
#define IS_CARRYING_N(ch) ((ch)->char_specials.carry_items)
#define FIGHTING(ch)	  ((ch)->char_specials.fighting)
#define HUNTING(ch)	  ((ch)->char_specials.hunting)
#define MOUNTING(ch)	  ((ch)->char_specials.mounting)
#define MOUNTING_OBJ(ch)  ((ch)->char_specials.mounting_obj)
#define GET_SAVE(ch, i)	  ((ch)->char_specials.saved.apply_saving_throw[i])
#define GET_ALIGNMENT(ch) ((ch)->char_specials.saved.alignment)

/* DM - Autoassist (who this char is autoassisting, max 1)
      - Autoassisted (who is autoassisting this char, list) */
#define AUTOASSIST(ch)	  ((ch)->char_specials.autoassist)
#define AUTOASSISTED(ch)  ((ch)->char_specials.autoassisted)

#define GET_COND(ch, i)		((ch)->player_specials->saved.conditions[(i)])
#define GET_LOADROOM(ch)	((ch)->player_specials->saved.load_room)
#define GET_PRACTICES(ch)	((ch)->player_specials->saved.spells_to_learn)
#define GET_INVIS_LEV(ch)	((ch)->player_specials->saved.invis_level)
#define GET_INVIS_TYPE(ch)	((ch)->player_specials->saved.invis_type_flag)
#define GET_WIMP_LEV(ch)	((ch)->player_specials->saved.wimp_level)
#define GET_FREEZE_LEV(ch)	((ch)->player_specials->saved.freeze_level)
#define GET_BAD_PWS(ch)		((ch)->player_specials->saved.bad_pws)
#define GET_TALK(ch, i)		((ch)->player_specials->saved.talks[i])
#define POOFIN(ch)		((ch)->player_specials->poofin)
#define POOFOUT(ch)		((ch)->player_specials->poofout)
#define GET_LAST_OLC_TARG(ch)	((ch)->player_specials->last_olc_targ)
#define GET_LAST_OLC_MODE(ch)	((ch)->player_specials->last_olc_mode)
#define GET_ALIASES(ch)		((ch)->player_specials->aliases)
#define GET_LAST_TELL(ch)	((ch)->player_specials->last_tell)
#define GET_CHAR_WAIT(ch) 	((ch)->player_specials->char_wait_state)
#define GET_IGN1(ch)		((ch)->player_specials->saved.ignore1)
#define GET_IGN2(ch)		((ch)->player_specials->saved.ignore2)
#define GET_IGN3(ch)		((ch)->player_specials->saved.ignore3)
#define GET_IGN_LEVEL(ch)	((ch)->player_specials->saved.ignorelev)
#define GET_IGN_NUM(ch)		((ch)->player_specials->saved.ignorenum)
#define GET_CLAN_NUM(ch)        ((ch)->player_specials->saved.clan_num)
#define GET_CLAN_LEV(ch)        ((ch)->player_specials->saved.clan_level)
#define GET_SKILL(ch, i)	((ch)->player_specials->saved.skills[i])
#define SET_SKILL(ch, i, pct)	{ (ch)->player_specials->saved.skills[i] = pct; }

#define GET_MOB_SPEC(ch) (IS_MOB(ch) ? (mob_index[(ch->nr)].func) : NULL)
#define GET_MOB_RNUM(mob)	((mob)->nr)
#define GET_MOB_VNUM(mob)	(IS_MOB(mob) ? \
				 mob_index[GET_MOB_RNUM(mob)].virtual : -1)

#define MEMORY(ch)		((ch)->mob_specials.memory)
#define GET_MOB_WAIT(ch)	((ch)->mob_specials.wait_state)
#define GET_DEFAULT_POS(ch)	((ch)->mob_specials.default_pos)

#define STRENGTH_REAL_APPLY_INDEX(ch) \
        ( ((GET_REAL_ADD(ch)==0) || (GET_REAL_STR(ch) != 18)) ? GET_REAL_STR(ch) :\
          (GET_REAL_ADD(ch) <= 50) ? 26 :( \
          (GET_REAL_ADD(ch) <= 75) ? 27 :( \
          (GET_REAL_ADD(ch) <= 90) ? 28 :( \
          (GET_REAL_ADD(ch) <= 99) ? 29 :  30 ) ) )                   \
        )

#define STRENGTH_AFF_APPLY_INDEX(ch) \
        ( ((GET_AFF_ADD(ch)==0) || (GET_AFF_STR(ch) != 18)) ? GET_AFF_STR(ch) :\
          (GET_AFF_ADD(ch) <= 50) ? 26 :( \
          (GET_AFF_ADD(ch) <= 75) ? 27 :( \
          (GET_AFF_ADD(ch) <= 90) ? 28 :( \
          (GET_AFF_ADD(ch) <= 99) ? 29 :  30 ) ) )                   \
        )

#define CAN_CARRY_W(ch) (str_app[STRENGTH_AFF_APPLY_INDEX(ch)].carry_w)
#define CAN_CARRY_N(ch) (5 + (GET_AFF_DEX(ch) >> 1) + (GET_LEVEL(ch) >> 1))
#define AWAKE(ch) (GET_POS(ch) > POS_SLEEPING)
#define CAN_SEE_IN_DARK(ch) \
   (AFF_FLAGGED(ch, AFF_INFRAVISION) || PRF_FLAGGED(ch, PRF_HOLYLIGHT))

#define IS_GOOD(ch)    (GET_ALIGNMENT(ch) >= 350)
#define IS_EVIL(ch)    (GET_ALIGNMENT(ch) <= -350)
#define IS_NEUTRAL(ch) (!IS_GOOD(ch) && !IS_EVIL(ch))


/* descriptor-based utils ************************************************/


#define WAIT_STATE(ch, cycle) { \
				if ((ch)->desc) {(ch)->desc->wait = (cycle); \
						 GET_CHAR_WAIT(ch) = (cycle);} \
				else if (IS_NPC(ch)) GET_MOB_WAIT(ch) = (cycle); }

#define CHECK_WAIT(ch)	(((ch)->desc) ? ((ch)->desc->wait > 1) : 0)
#define STATE(d)	((d)->connected)
#define GET_EQ(ch, i)           ((ch)->equipment[i])

/* object utils **********************************************************/

#define OBJ_RIDDEN(obj)		((obj)->ridden_by)
#define GET_OBJ_NAME(obj)       ((obj)->*name)
#define GET_OBJ_TYPE(obj)	((obj)->obj_flags.type_flag)
#define GET_OBJ_COST(obj)	((obj)->obj_flags.cost)
#define GET_OBJ_RENT(obj)	((obj)->obj_flags.cost_per_day)
#define GET_OBJ_EXTRA(obj)	((obj)->obj_flags.extra_flags)
#define GET_OBJ_WEAR(obj)	((obj)->obj_flags.wear_flags)
#define GET_OBJ_VAL(obj, val)	((obj)->obj_flags.value[(val)])
#define GET_OBJ_WEIGHT(obj)	((obj)->obj_flags.weight)
#define GET_OBJ_TIMER(obj)	((obj)->obj_flags.timer)
#define GET_OBJ_RNUM(obj)	((obj)->item_number)
#define GET_OBJ_VNUM(obj)	(GET_OBJ_RNUM(obj) >= 0 ? \
				 obj_index[GET_OBJ_RNUM(obj)].virtual : -1)
#define IS_OBJ_STAT(obj,stat)	(IS_SET((obj)->obj_flags.extra_flags,stat))

#define GET_OBJ_SPEC(obj) ((obj)->item_number >= 0 ? \
	(obj_index[(obj)->item_number].func) : NULL)

#define CAN_WEAR(obj, part) (IS_SET((obj)->obj_flags.wear_flags, (part)))
#define IS_PROTECT_GEAR(obj) (((obj)->obj_flags.type_flag >= BASE_PROTECT_GEAR) \
                             & ((obj)->obj_flags.type_flag < MAX_PROTECT_GEAR))
#define GET_OBJ_LR(obj)	(((obj)->obj_flags.extra_flags & 0x00fe0000) >> 17)

/* compound utilities and other macros **********************************/


#define HSHR(ch) (GET_SEX(ch) ? (GET_SEX(ch)==SEX_MALE ? "his":"her") :"its")
#define HSSH(ch) (GET_SEX(ch) ? (GET_SEX(ch)==SEX_MALE ? "he" :"she") : "it")
#define HMHR(ch) (GET_SEX(ch) ? (GET_SEX(ch)==SEX_MALE ? "him":"her") : "it")

#define ANA(obj) (strchr("aeiouyAEIOUY", *(obj)->name) ? "An" : "A")
#define SANA(obj) (strchr("aeiouyAEIOUY", *(obj)->name) ? "an" : "a")


/* Various macros building up to CAN_SEE */

#define LIGHT_OK(sub)	(!IS_AFFECTED(sub, AFF_BLIND) && \
   (IS_LIGHT((sub)->in_room) || IS_AFFECTED((sub), AFF_INFRAVISION)))

#define INVIS_OK(sub, obj) \
 ((!IS_AFFECTED((obj),AFF_INVISIBLE) || IS_AFFECTED(sub,AFF_DETECT_INVIS)) && \
 (!IS_AFFECTED((obj), AFF_HIDE) || IS_AFFECTED(sub, AFF_SENSE_LIFE)))

#define MORT_CAN_SEE(sub, obj) (LIGHT_OK(sub) && INVIS_OK(sub, obj))

#define IMM_CAN_SEE(sub, obj) \
   (MORT_CAN_SEE(sub, obj) || PRF_FLAGGED(sub, PRF_HOLYLIGHT))

#define SELF(sub, obj)  ((sub) == (obj))

#define INVIS_RANGE(sub, obj) (GET_INVIS_TYPE(obj) == 0 ? GET_INVIS_LEV(obj) : \
	(GET_INVIS_TYPE(obj) >= 1 ? (GET_REAL_LEVEL(sub) > GET_INVIS_LEV(obj) ? \
	 (GET_REAL_LEVEL(sub) < GET_INVIS_TYPE(obj) ? GET_LEVEL(obj) : 0 ) : 0) : \
	(GET_INVIS_TYPE(obj) == -2 ? ( GET_IDNUM(sub) == GET_INVIS_LEV(obj) ? \
	 GET_LEVEL(obj) : 0) : \
	(GET_REAL_LEVEL(sub) == GET_INVIS_LEV(obj) ? GET_LEVEL(obj) : 0 ))))

/* Can subject see character "obj"? */
#define CAN_SEE(sub, obj) (SELF(sub, obj) || \
   ((GET_REAL_LEVEL(sub) >= INVIS_RANGE(sub, obj)) && IMM_CAN_SEE(sub, obj)))

/* End of CAN_SEE */


#define INVIS_OK_OBJ(sub, obj) \
  (!IS_OBJ_STAT((obj), ITEM_INVISIBLE) || IS_AFFECTED((sub), AFF_DETECT_INVIS))

#define MORT_CAN_SEE_OBJ(sub, obj) (LIGHT_OK(sub) && INVIS_OK_OBJ(sub, obj))

#define CAN_SEE_OBJ(sub, obj) \
   (MORT_CAN_SEE_OBJ(sub, obj) || PRF_FLAGGED((sub), PRF_HOLYLIGHT))

#define CAN_CARRY_OBJ(ch,obj)  \
   (((IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj)) <= CAN_CARRY_W(ch)) &&   \
    ((IS_CARRYING_N(ch) + 1) <= CAN_CARRY_N(ch)))

#define CAN_GET_OBJ(ch, obj)   \
   (CAN_WEAR((obj), ITEM_WEAR_TAKE) && CAN_CARRY_OBJ((ch),(obj)) && \
    CAN_SEE_OBJ((ch),(obj)))


#define PERS(ch, vict)   (CAN_SEE(vict, ch) ? GET_NAME(ch) : "someone")

#define OBJS(obj, vict) (CAN_SEE_OBJ((vict), (obj)) ? \
	(obj)->short_description  : "something")

#define OBJN(obj, vict) (CAN_SEE_OBJ((vict), (obj)) ? \
	fname((obj)->name) : "something")


#define EXIT(ch, door)  (world[(ch)->in_room].dir_option[door])

#define CAN_GO(ch, door) (EXIT(ch,door) && \
			 (EXIT(ch,door)->to_room != NOWHERE) && \
			 !IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED))


#define CLASS_ABBR(ch) (IS_NPC(ch) ? "--" : class_abbrevs[(int)GET_CLASS(ch)])

#define IS_MAGIC_USER(ch)	(!IS_NPC(ch) && \
				(GET_CLASS(ch) == CLASS_MAGIC_USER))
#define IS_CLERIC(ch)		(!IS_NPC(ch) && \
				(GET_CLASS(ch) == CLASS_CLERIC))
#define IS_THIEF(ch)		(!IS_NPC(ch) && \
				(GET_CLASS(ch) == CLASS_THIEF))
#define IS_WARRIOR(ch)		(!IS_NPC(ch) && \
				(GET_CLASS(ch) == CLASS_WARRIOR))

#define OUTSIDE(ch) (!ROOM_FLAGGED((ch)->in_room, ROOM_INDOORS))


/* OS compatibility ******************************************************/


/* there could be some strange OS which doesn't have NULL... */
#ifndef NULL
#define NULL (void *)0
#endif

#if !defined(FALSE)
#define FALSE 0
#endif

#if !defined(TRUE)
#define TRUE  (!FALSE)
#endif

/* defines for fseek */
#ifndef SEEK_SET
#define SEEK_SET	0
#define SEEK_CUR	1
#define SEEK_END	2
#endif

/*
 * Some systems such as Sun's don't have prototyping in their header files.
 * Thus, we try to compensate for them.
 *
 * Much of this is from Merc 2.2, used with permission.
 */

#if defined(_AIX)
char	*crypt(const char *key, const char *salt);
#endif

#if defined(apollo)
int	atoi (const char *string);
void	*calloc( unsigned nelem, size_t size);
char	*crypt( const char *key, const char *salt);
#endif

#if defined(hpux)
char	*crypt(char *key, const char *salt);
#define srandom srand
#define random rand
#endif

#if defined(linux)
char	*crypt( const char *key, const char *salt);
#endif

#if defined(MIPS_OS)
char	*crypt(const char *key, const char *salt);
#endif

#if defined(NeXT)
char	*crypt(const char *key, const char *salt);
#endif

#if defined(sequent)
char	*crypt(const char *key, const char *salt);
int	fclose(FILE *stream);
int	fprintf(FILE *stream, const char *format, ... );
int	fread(void *ptr, int size, int n, FILE *stream);
int	fseek(FILE *stream, long offset, int ptrname);
void	perror(const char *s);
int	ungetc(int c, FILE *stream);
#endif

#if defined(sun)
#include <memory.h>
void	bzero(char *b, int length);
char	*crypt(const char *key, const char *salt);
int	fclose(FILE *stream);
int	fflush(FILE *stream);
int	rewind(FILE *stream);
int	sscanf(char *s, const char *format, ... );
int	fprintf(FILE *stream, const char *format, ... );
int	fscanf(FILE *stream, const char *format, ... );
int	fseek(FILE *stream, long offset, int ptrname);
int	fread(void *ptr, int size, int n, FILE *stream);
int	fwrite(void *ptr, int size, int n, FILE *stream);
void	perror(const char *s);
int	ungetc(int c, FILE *stream);
time_t	time(time_t *tloc);
int	srandom(int seed);
long	random();
int	system(char *string);
#endif

#if defined(ultrix)
char	*crypt(const char *key, const char *salt);
#endif

#if defined(DGUX_TARGET)
#include <crypt.h>
#define bzero(a, b) memset((a), 0, (b))
#endif

/*
 * The crypt(3) function is not available on some operating systems.
 * In particular, the U.S. Government prohibits its export from the
 *   United States to foreign countries.
 * Turn on NOCRYPT to keep passwords in plain text.
 */
#ifdef NOCRYPT
#define CRYPT(a,b) (a)
#else
#define CRYPT(a,b) ((char *) crypt((a),(b)))
#endif

/* Gun defines */
#define BASE_GUN_TYPE		20
#define MAX_GUN_TYPES		30

#define OBJ_IS_GUN(x)			((GET_OBJ_VAL((x),3) & 0x7fff) >= BASE_GUN_TYPE \
													  && (GET_OBJ_VAL((x),3) & 0x7fff) <= \
													 (BASE_GUN_TYPE + MAX_GUN_TYPES))

