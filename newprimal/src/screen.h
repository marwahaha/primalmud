/* ************************************************************************
*   File: screen.h                                      Part of CircleMUD *
*  Usage: header file with ANSI color codes for online color              *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"
#define KNUL  ""
/* NEW COLORS Added ??/01/96 - Vader */
#define KBWHT "\x1B[0;1m"    /* bright white */
#define KBGRY "\x1B[0;1;30m" /* dark grey */
#define KBRED "\x1B[0;1;31m" /* bright red */
#define KBGRN "\x1B[0;1;32m" /* bright green */
#define KBYEL "\x1B[0;1;33m" /* yellow */
#define KBBLU "\x1B[0;1;34m" /* light blue */
#define KBMAG "\x1B[0;1;35m" /* bright purple */
#define KBCYN "\x1B[0;1;36m" /* bright cyan */

/* conditional color.  pass it a pointer to a char_data and a color level. */
#define C_OFF	0
#define C_SPR	1
#define C_NRM	2
#define C_CMP	3
#define _clrlevel(ch) (!IS_NPC(ch) ? (PRF_FLAGGED((ch), PRF_COLOR_1) ? 1 : 0) + \
		       (PRF_FLAGGED((ch), PRF_COLOR_2) ? 2 : 0) : 0)
#define clr(ch,lvl) (_clrlevel(ch) >= (lvl))
#define CCNRM(ch,lvl)  (clr((ch),(lvl))?KNRM:KNUL)
#define CCRED(ch,lvl)  (clr((ch),(lvl))?KRED:KNUL)
#define CCGRN(ch,lvl)  (clr((ch),(lvl))?KGRN:KNUL)
#define CCYEL(ch,lvl)  (clr((ch),(lvl))?KYEL:KNUL)
#define CCBLU(ch,lvl)  (clr((ch),(lvl))?KBLU:KNUL)
#define CCMAG(ch,lvl)  (clr((ch),(lvl))?KMAG:KNUL)
#define CCCYN(ch,lvl)  (clr((ch),(lvl))?KCYN:KNUL)
#define CCWHT(ch,lvl)  (clr((ch),(lvl))?KWHT:KNUL)
/* NEW COLOURS ADDED! - Vader */
#define CCBGRY(ch,lvl) (clr((ch),(lvl))?KBGRY:KNUL)
#define CCBRED(ch,lvl) (clr((ch),(lvl))?KBRED:KNUL)
#define CCBGRN(ch,lvl) (clr((ch),(lvl))?KBGRN:KNUL)
#define CCBYEL(ch,lvl) (clr((ch),(lvl))?KBYEL:KNUL)
#define CCBBLU(ch,lvl) (clr((ch),(lvl))?KBBLU:KNUL)
#define CCBMAG(ch,lvl) (clr((ch),(lvl))?KBMAG:KNUL)
#define CCBCYN(ch,lvl) (clr((ch),(lvl))?KBCYN:KNUL)
#define CCBWHT(ch,lvl) (clr((ch),(lvl))?KBWHT:KNUL)

/* Standard colours - DM */
#define CCEXP(ch,lev)	CCCYN(ch,lev)
#define CCGOLD(ch,lev)  CCYEL(ch,lev)

#define COLOR_LEV(ch) (_clrlevel(ch))

#define QNRM CCNRM(ch,C_SPR)
#define QRED CCRED(ch,C_SPR)
#define QGRN CCGRN(ch,C_SPR)
#define QYEL CCYEL(ch,C_SPR)
#define QBLU CCBLU(ch,C_SPR)
#define QMAG CCMAG(ch,C_SPR)
#define QCYN CCCYN(ch,C_SPR)
#define QWHT CCWHT(ch,C_SPR)
