/* ************************************************************************
*   File: ban.c                                         Part of CircleMUD *
*  Usage: banning/unbanning/checking sites and player names               *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h> 
#include "conf.h"
#include "sysdep.h"


#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"

struct ban_list_element *ban_list = NULL;
extern struct descriptor_data *descriptor_list;

/* local functions */
void load_banned(void);
int isbanned(char *hostname);
int isipbanned(struct in_addr *inaddr, const int nameserver_is_slow);
int parse_ip(const char *addr, struct in_addr *inaddr);
void _write_one_node(FILE * fp, struct ban_list_element * node);
void write_ban_list(void);
ACMD(do_ban);
ACMD(do_unban);
void Read_Invalid_List(void);

const char *ban_types[] =
{
  "no",
  "new",
  "select",
  "all",
  "ERROR"
};


void load_banned(void)
{
  FILE *fl;
  int i, date;
  char site_name[BANNED_SITE_LENGTH + 1], ban_type[100];
  char name[MAX_NAME_LENGTH + 1];
  struct ban_list_element *next_node;
  struct in_addr addy;

  ban_list = 0;

  if (!(fl = fopen(BAN_FILE, "r")))
  {
    if (errno != ENOENT)
      basic_mud_log("SYSERR: Unable to open banfile '%s': %s", BAN_FILE, strerror(errno));
    else
      basic_mud_log("   Ban file '%s' doesn't exist.", BAN_FILE);
    return;
  }
  while (fscanf(fl, " %s %s %d %s ", ban_type, site_name, &date, name) == 4)
  {
    CREATE(next_node, struct ban_list_element, 1);
    strncpy(next_node->site, site_name, BANNED_SITE_LENGTH);
    next_node->site[BANNED_SITE_LENGTH] = '\0';
    if (parse_ip(site_name, &addy) == 0)
      basic_mud_log("SYSERR: Banned site [%s] failed parse_ip()", site_name);
    strncpy(next_node->name, name, MAX_NAME_LENGTH);
    next_node->s_addr = addy.s_addr;
    next_node->name[MAX_NAME_LENGTH] = '\0';
    next_node->date = date;

    for (i = BAN_NOT; i <= BAN_ALL; i++)
      if (!strcmp(ban_type, ban_types[i]))
	next_node->type = i;

    next_node->next = ban_list;
    ban_list = next_node;
  }

  fclose(fl);
}

// DM - check ips, not hostnames
int isipbanned(struct in_addr in, const int nameserver_is_slow) 
{
  int i = BAN_NOT;
  for (struct ban_list_element *b = ban_list; b; b = b->next)
    if ((b->s_addr & 0xff000000) == 0)
    {
      if ((in.s_addr & 0x00ffffff) == b->s_addr)
	i = MAX(i, b->type);
    } else if (in.s_addr == b->s_addr)
      i = MAX(i, b->type);
  return i;
}	
		
int isbanned(char *hostname)
{
  int i;
  struct ban_list_element *banned_node;
  char *nextchar;

  if (!hostname || !*hostname)
    return (0);

  i = 0;
  for (nextchar = hostname; *nextchar; nextchar++)
    *nextchar = LOWER(*nextchar);

  for (banned_node = ban_list; banned_node; banned_node = banned_node->next)
    if (strstr(hostname, banned_node->site))	/* if hostname is a substring */
      i = MAX(i, banned_node->type);

  return (i);
}


void _write_one_node(FILE * fp, struct ban_list_element * node)
{
  if (node)
  {
    _write_one_node(fp, node->next);
    fprintf(fp, "%s %s %ld %s\n", ban_types[node->type],
	    node->site, (long) node->date, node->name);
  }
}



void write_ban_list(void)
{
  FILE *fl;

  if (!(fl = fopen(BAN_FILE, "w")))
  {
    perror("SYSERR: Unable to open '" BAN_FILE "' for writing");
    return;
  }
  _write_one_node(fl, ban_list);/* recursively write from end to start */
  fclose(fl);
  return;
}


ACMD(do_ban)
{
  char flag[MAX_INPUT_LENGTH], site[MAX_INPUT_LENGTH],
	format[MAX_INPUT_LENGTH], *nextchar, *timestr;
  int i;
  struct ban_list_element *ban_node;
  struct in_addr addy;
#ifdef NO_LOCALTIME
  struct tm lt;
#endif

  *buf = '\0';

  if (!*argument)
  {
    if (!ban_list)
    {
      send_to_char("No sites are banned.\r\n", ch);
      return;
    }
    strcpy(format, "%-25.25s  %-8.8s  %-10.10s  %-16.16s\r\n");
    sprintf(buf, format,
	    "Banned Site Name",
	    "Ban Type",
	    "Banned On",
	    "Banned By");
    send_to_char(buf, ch);
    sprintf(buf, format,
	    "---------------------------------",
	    "---------------------------------",
	    "---------------------------------",
	    "---------------------------------");
    send_to_char(buf, ch);

    for (ban_node = ban_list; ban_node; ban_node = ban_node->next) {
      if (ban_node->date) 
      {
#ifndef NO_LOCALTIME
	timestr = asctime(localtime(&(ban_node->date)));
#else
	if (jk_localtime(&lt, ban_node->date))
	  strcpy(site, "Error");
	else 
	  timestr = asctime(&lt);
	*(timestr + 10) = 0;
#endif
	strcpy(site, timestr);
      } else {
	strcpy(site, "Unknown");
      }
      sprintf(buf, format, ban_node->site, ban_types[ban_node->type], site,
	      ban_node->name);
      send_to_char(buf, ch);
    }
    return;
  }
  two_arguments(argument, flag, site);
  if (!*site || !*flag)
  {
    send_to_char("Usage: ban {all | select | new} site_name\r\n", ch);
    return;
  }
  if (!(!str_cmp(flag, "select") || 
	!str_cmp(flag, "all") || 
	!str_cmp(flag, "new"))) {
    send_to_char("Flag must be ALL, SELECT, or NEW.\r\n", ch);
    return;
  }
  if (parse_ip(site, &addy) == 0)
  {
    send_to_char("That is not a valid IP address.\r\n", ch);
    return;
  }
  for (ban_node = ban_list; ban_node; ban_node = ban_node->next)
  {
    if (!str_cmp(ban_node->site, site) || (addy.s_addr == ban_node->s_addr))
    {
      send_to_char("That site has already been banned -- unban it to change the ban type.\r\n", ch);
      return;
    }
  }

  CREATE(ban_node, struct ban_list_element, 1);
  strncpy(ban_node->site, site, BANNED_SITE_LENGTH);
  for (nextchar = ban_node->site; *nextchar; nextchar++)
    *nextchar = LOWER(*nextchar);
  ban_node->site[BANNED_SITE_LENGTH] = '\0';
  ban_node->s_addr = addy.s_addr;
  strncpy(ban_node->name, GET_NAME(ch), MAX_NAME_LENGTH);
  ban_node->name[MAX_NAME_LENGTH] = '\0';
  ban_node->date = time(0);

  for (i = BAN_NEW; i <= BAN_ALL; i++)
    if (!str_cmp(flag, ban_types[i]))
      ban_node->type = i;

  ban_node->next = ban_list;
  ban_list = ban_node;

  sprintf(buf, "%s has banned %s for %s players.", GET_NAME(ch), site,
	  ban_types[ban_node->type]);
  mudlog(buf, NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);
  send_to_char("Site banned.\r\n", ch);
  write_ban_list();
}


ACMD(do_unban)
{
  char site[MAX_INPUT_LENGTH];
  struct ban_list_element *ban_node, *temp;
  int found = 0;

  one_argument(argument, site);
  if (!*site) {
    send_to_char("A site to unban might help.\r\n", ch);
    return;
  }
  ban_node = ban_list;
  while (ban_node && !found)
  {
    if (!str_cmp(ban_node->site, site))
      found = 1;
    else
      ban_node = ban_node->next;
  }

  if (!found)
  {
    send_to_char("That site is not currently banned.\r\n", ch);
    return;
  }
  REMOVE_FROM_LIST(ban_node, ban_list, next);
  send_to_char("Site unbanned.\r\n", ch);
  sprintf(buf, "%s removed the %s-player ban on %s.",
	  GET_NAME(ch), ban_types[ban_node->type], ban_node->site);
  mudlog(buf, NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);

  free(ban_node);
  write_ban_list();
}


/**************************************************************************
 *  Code to check for invalid names (i.e., profanity, etc.)		  *
 *  Written by Sharon P. Goza						  *
 **************************************************************************/

#define MAX_INVALID_NAMES	200

char *invalid_list[MAX_INVALID_NAMES];
int num_invalid = 0;

int Valid_Name(char *newname, bool desc_check)
{
  int i;
  struct descriptor_data *dt;
  char tempname[MAX_INPUT_LENGTH];

  /*
   * Make sure someone isn't trying to create this same name.  We want to
   * do a 'str_cmp' so people can't do 'Bob' and 'BoB'.  The creating login
   * will not have a character name yet and other people sitting at the
   * prompt won't have characters yet.
   */
  if (desc_check) 
    for (dt = descriptor_list; dt; dt = dt->next)
      if (dt->character && GET_NAME(dt->character) && !str_cmp(GET_NAME(dt->character), newname))
	switch (STATE(dt))
	{
	  case CON_EXDESC:
	  case CON_MEDIT:
	  case CON_OEDIT:
	  case CON_REDIT:
	  case CON_SEDIT:
	  case CON_TEDIT:
	  case CON_ZEDIT:
	  case CON_REPORT_ADD:
	  case CON_REPORT_EDIT:
	  case CON_TRIGEDIT:
	    // Do we need to do anything here?..
	  case CON_DELCNF1:
	  case CON_DELCNF2:
	  case CON_RMOTD:
	  case CON_MENU:
	  case CON_PLAYING:
	    return (1);
	  default: 
	    return (0);
	}

  /* return valid if list doesn't exist */
  if (!invalid_list || num_invalid < 1)
    return (1);

  /* change to lowercase */
  strcpy(tempname, newname);
  for (i = 0; tempname[i]; i++)
    tempname[i] = LOWER(tempname[i]);

  /* Does the desired name contain a string in the invalid list? */
  for (i = 0; i < num_invalid; i++)
    if (strstr(tempname, invalid_list[i]))
      return (0);

  return (1);
}


void Read_Invalid_List(void)
{
  FILE *fp;
  char temp[256];

  if (!(fp = fopen(XNAME_FILE, "r")))
  {
    perror("SYSERR: Unable to open '" XNAME_FILE "' for reading");
    return;
  }

  num_invalid = 0;
  while (get_line(fp, temp) && num_invalid < MAX_INVALID_NAMES)
    invalid_list[num_invalid++] = str_dup(temp);

  if (num_invalid >= MAX_INVALID_NAMES)
  {
    basic_mud_log("SYSERR: Too many invalid names; change MAX_INVALID_NAMES in ban.c");
    exit(1);
  }

  fclose(fp);
}
