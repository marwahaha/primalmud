/* ************************************************************************
*   File: comm.c                                        Part of CircleMUD *
*  Usage: Communication, socket handling, main(), central game loop       *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

/*
   Modified by Brett Murphy
*/

#define __COMM_C__

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <limits.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <netinet/in.h>
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "house.h"
#include "screen.h"

/* externs */
extern int restrict;
extern int mini_mud;
extern int mainten;
extern int no_rent_check;
extern FILE *player_fl;
extern int DFLT_PORT;
extern char *DFLT_DIR;
extern int MAX_PLAYERS;
extern int MAX_DESCRIPTORS_AVAILABLE;

extern struct room_data *world;	/* In db.c */
extern int top_of_world;	/* In db.c */
extern struct time_info_data time_info;	/* In db.c */
extern char help[];

/* local globals */
struct descriptor_data *descriptor_list = NULL;	/* master desc list */
struct txt_block *bufpool = 0;	/* pool of large output buffers */
int buf_largecount = 0;		/* # of large buffers which exist */
int buf_overflows = 0;		/* # of overflows of output */
int buf_switches = 0;		/* # of switches from small to large buf */
int circle_shutdown = 0;	/* clean shutdown */
int circle_reboot = 0;		/* reboot the game after a shutdown */
int no_specials = 0;		/* Suppress ass. of special routines */
int avail_descs = 0;		/* max descriptors available */
int tics = 0;			/* for extern checkpointing */
extern int nameserver_is_slow;	/* see config.c */
extern int auto_save;		/* see config.c */
extern int autosave_time;	/* see config.c */

/* functions in this file */
int get_from_q(struct txt_q * queue, char *dest, int *aliased);
void init_game(int port);
void signal_setup(void);
void game_loop(int mother_desc);
int init_socket(int port);
int new_descriptor(int s);
int get_avail_descs(void);
int process_output(struct descriptor_data * t);
int process_input(struct descriptor_data * t);
void close_socket(struct descriptor_data * d);
struct timeval timediff(struct timeval * a, struct timeval * b);
void flush_queues(struct descriptor_data * d);
void nonblock(int s);
int perform_subst(struct descriptor_data * t, char *orig, char *subst);
int perform_alias(struct descriptor_data * d, char *orig);
void record_usage(void);
void make_prompt(struct descriptor_data * point);
/* JA 3/5/95 ship movement functions */
void move_ship(void);
void move_falcon(void);

/* extern fcnts */
void boot_db(void);
void zone_update(void);
void affect_update(void);	/* In spells.c */
void point_update(void);	/* In limits.c */
void mobile_activity(void);
void string_add(struct descriptor_data * d, char *str);
void perform_violence(void);
void show_string(struct descriptor_data * d, char *input);
int isbanned(char *hostname);
void weather_and_time(int mode);
void do_arrows(); /* for random damage */
void check_games(); /* bloody gamblers */
void rotate_arms(); /* special for the island forever */
void handle_damage_rooms(); // Island forever room specials

void proc_color(char *inbuf, int color, int insize); /* Easy color - DM */

/* *********************************************************************
*  main game loop and related stuff                                    *
********************************************************************* */

int main(int argc, char **argv)
{
  int port;
  char buf[512];
  int pos = 1;
  char *dir;

  port = DFLT_PORT;
  dir = DFLT_DIR;

  while ((pos < argc) && (*(argv[pos]) == '-')) {
    switch (*(argv[pos] + 1)) {
    case 'd':
      if (*(argv[pos] + 2))
	dir = argv[pos] + 2;
      else if (++pos < argc)
	dir = argv[pos];
      else {
	log("Directory arg expected after option -d.");
	exit(1);
      }
      break;
    case 'h':
      mainten = 1;
      log("Maintence Mode: Not accepting any logins.");
      break;
    case 'm':
      mini_mud = 1;
      no_rent_check = 1;
      log("Running in minimized mode & with no rent check.");
      break;
    case 'q':
      no_rent_check = 1;
      log("Quick boot mode -- rent check supressed.");
      break;
    case 'r':
      restrict = 1;
      log("Restricting game -- no new players allowed.");
      break;
    case 's':
      no_specials = 1;
      log("Suppressing assignment of special routines.");
      break;
    default:
      sprintf(buf, "SYSERR: Unknown option -%c in argument string.", *(argv[pos] + 1));
      log(buf);
      break;
    }
    pos++;
  }

  if (pos < argc)
    if (!isdigit(*argv[pos])) {
      fprintf(stderr, "Usage: %s [-h] [-m] [-q] [-r] [-s] [-d pathname] [port #]\n", argv[0]);
      exit(1);
    } else if ((port = atoi(argv[pos])) <= 1024) {
      fprintf(stderr, "Illegal port number.\n");
      exit(1);
    }
  sprintf(buf, "Running game on port %d.", port);
  log(buf);

  if (chdir(dir) < 0) {
    perror("Fatal error changing to data directory");
    exit(1);
  }
  sprintf(buf, "Using %s as data directory.", dir);
  log(buf);

  init_game(port);

  return 0;
}



/* Init sockets, run game, and cleanup sockets */
void init_game(int port)
{
  int mother_desc;

  srandom(time(0));

  log("Opening mother connection.");
  mother_desc = init_socket(port);

  avail_descs = get_avail_descs();

  boot_db();

  log("Signal trapping.");
  signal_setup();

  log("Entering game loop.");

  game_loop(mother_desc);

  log("Closing all sockets.");
  while (descriptor_list)
    close_socket(descriptor_list);

  close(mother_desc);
  fclose(player_fl);

  if (circle_reboot) {
    log("Rebooting.");
    exit(52);			/* what's so great about HHGTTG, anyhow? */
  }
  log("Normal termination of game.");
}



/*
 * init_socket sets up the mother descriptor - creates the socket, sets
 * its options up, binds it, and listens.
 */
int init_socket(int port)
{
  int s, opt;
  struct sockaddr_in sa;

  /*
   * Should the first argument to socket() be AF_INET or PF_INET?  I don't
   * know, take your pick.  PF_INET seems to be more widely adopted, and
   * Comer (_Internetworking with TCP/IP_) even makes a point to say that
   * people erroneously use AF_INET with socket() when they should be using
   * PF_INET.  However, the man pages of some systems indicate that AF_INET
   * is correct; some such as ConvexOS even say that you can use either one.
   * All implementations I've seen define AF_INET and PF_INET to be the same
   * number anyway, so ths point is (hopefully) moot.
   */

  if ((s = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
    perror("Create socket");
    exit(1);
  }
#if defined(SO_SNDBUF)
  opt = LARGE_BUFSIZE + GARBAGE_SPACE;
  if (setsockopt(s, SOL_SOCKET, SO_SNDBUF, (char *) &opt, sizeof(opt)) < 0) {
    perror("setsockopt SNDBUF");
    exit(1);
  }
#endif

#if defined(SO_REUSEADDR)
  opt = 1;
  if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *) &opt, sizeof(opt)) < 0) {
    perror("setsockopt REUSEADDR");
    exit(1);
  }
#endif

#if defined(SO_REUSEPORT)
  opt = 1;
  if (setsockopt(s, SOL_SOCKET, SO_REUSEPORT, (char *) &opt, sizeof(opt)) < 0) {
    perror("setsockopt REUSEPORT");
    exit(1);
  }
#endif

#if defined(SO_LINGER)
  {
    struct linger ld;

    ld.l_onoff = 0;
    ld.l_linger = 0;
    if (setsockopt(s, SOL_SOCKET, SO_LINGER, &ld, sizeof(ld)) < 0) {
      perror("setsockopt LINGER");
      exit(1);
    }
  }
#endif

  sa.sin_family = AF_INET;
  sa.sin_port = htons(port);
  sa.sin_addr.s_addr = htonl(INADDR_ANY);

  if (bind(s, (struct sockaddr *) & sa, sizeof(sa)) < 0) {
    perror("bind");
    close(s);
    exit(1);
  }
  nonblock(s);
  listen(s, 5);
  return s;
}


int get_avail_descs(void)
{
  int max_descs = 0;

/*
 * First, we'll try using getrlimit/setrlimit.  This will probably work
 * on most systems.
 */
#if defined (RLIMIT_NOFILE) || defined (RLIMIT_OFILE)
#if !defined(RLIMIT_NOFILE)
#define RLIMIT_NOFILE RLIMIT_OFILE
#endif
  {
    struct rlimit limit;

    getrlimit(RLIMIT_NOFILE, &limit);
    if (limit.rlim_max == RLIM_INFINITY)
      max_descs = MAX_PLAYERS + NUM_RESERVED_DESCS;
    else
      max_descs = limit.rlim_max;

    limit.rlim_cur = max_descs;

    setrlimit(RLIMIT_NOFILE, &limit);
  }
#elif defined (OPEN_MAX) || defined(FOPEN_MAX)
#if !defined(OPEN_MAX)
#define OPEN_MAX FOPEN_MAX
#endif
  max_descs = OPEN_MAX;		/* Uh oh.. rlimit didn't work, but we have
				 * OPEN_MAX */
#else
  /*
   * Okay, you don't have getrlimit() and you don't have OPEN_MAX.  Time to
   * use the POSIX sysconf() function.  (See Stevens' _Advanced Programming
   * in the UNIX Environment_).
   */
  errno = 0;
  if ((max_descs = sysconf(_SC_OPEN_MAX)) < 0) {
    if (errno == 0)
      max_descs = MAX_PLAYERS + NUM_RESERVED_DESCS;
    else {
      perror("Error calling sysconf");
      exit(1);
    }
  }
#endif

  max_descs = MIN(MAX_PLAYERS, max_descs - NUM_RESERVED_DESCS);

  if (max_descs <= 0) {
    log("Non-positive max player limit!");
    exit(1);
  }
  sprintf(buf, "Setting player limit to %d", max_descs);
  log(buf);
  return max_descs;
}



/*
 * game_loop contains the main loop which drives the entire MUD.  It
 * cycles once every 0.10 seconds and is responsible for accepting new
 * new connections, polling existing connections for input, dequeueing
 * output and sending it out to players, and calling "heartbeat" functions
 * such as mobile_activity().
 */
int pulse =0;

void game_loop(int mother_desc)
{
  fd_set input_set, output_set, exc_set;
  struct timeval last_time, now, timespent, timeout, null_time, opt_time;
  char comm[MAX_INPUT_LENGTH];
  struct descriptor_data *d, *next_d, *i;
  int mins_since_crashsave = 0, maxdesc, aliased;
  struct obj_data *tob; /* toboggan */
  extern struct index_data *obj_index; /* for toboggans */

  /* initialize various time values */
  null_time.tv_sec = 0;
  null_time.tv_usec = 0;
  opt_time.tv_usec = OPT_USEC;
  opt_time.tv_sec = 0;
  gettimeofday(&last_time, (struct timezone *) 0);

  /* The Main Loop.  The Big Cheese.  The Top Dog.  The Head Honcho.  The.. */
  while (!circle_shutdown) {

    /* Sleep if we don't have any connections */
    if (descriptor_list == NULL) {
      log("No connections.  Going to sleep.");
      FD_ZERO(&input_set);
      FD_SET(mother_desc, &input_set);
      if (select(mother_desc + 1, &input_set, (fd_set *) 0, (fd_set *) 0, NULL) < 0) {
	if (errno == EINTR)
	  log("Waking up to process signal.");
	else
	  perror("Select coma");
      } else
	log("New connection.  Waking up.");
      gettimeofday(&last_time, (struct timezone *) 0);
    }
    /* Set up the input, output, and exception sets for select(). */
    FD_ZERO(&input_set);
    FD_ZERO(&output_set);
    FD_ZERO(&exc_set);
    FD_SET(mother_desc, &input_set);
    maxdesc = mother_desc;
    for (d = descriptor_list; d; d = d->next) {
      if (d->descriptor > maxdesc)
	maxdesc = d->descriptor;
      FD_SET(d->descriptor, &input_set);
      FD_SET(d->descriptor, &output_set);
      FD_SET(d->descriptor, &exc_set);
    }

    /*
     * At this point, the original Diku code set up a signal mask to avoid
     * block all signals from being delivered.  I believe this was done in
     * order to prevent the MUD from dying with an "interrupted system call"
     * error in the event that a signal be received while the MUD is dormant.
     * However, I think it is easier to check for an EINTR error return from
     * this select() call rather than to block and unblock signals.
     */
    do {
      errno = 0;		/* clear error condition */

      /* figure out for how long we have to sleep */
      gettimeofday(&now, (struct timezone *) 0);
      timespent = timediff(&now, &last_time);
      timeout = timediff(&opt_time, &timespent);

      /* sleep until the next 0.1 second mark */
      if (select(0, (fd_set *) 0, (fd_set *) 0, (fd_set *) 0, &timeout) < 0)
	if (errno != EINTR) {
	  perror("Select sleep");
	  exit(1);
	}
    } while (errno);

    /* record the time for the next pass */
    gettimeofday(&last_time, (struct timezone *) 0);

    /* poll (without blocking) for new input, output, and exceptions */
    if (select(maxdesc + 1, &input_set, &output_set, &exc_set, &null_time) < 0) {
      perror("Select poll");
      return;
    }
    /* New connection? */
    if (FD_ISSET(mother_desc, &input_set))
      new_descriptor(mother_desc);

    /* kick out the freaky folks in the exception set */
    for (d = descriptor_list; d; d = next_d) 
		{
      next_d = d->next;
      if (FD_ISSET(d->descriptor, &exc_set)) 
			{
				FD_CLR(d->descriptor, &input_set);
				FD_CLR(d->descriptor, &output_set);
				close_socket(d);
      }
    }

    /* process descriptors with input pending */
    for (d = descriptor_list; d; d = next_d) 
		{
      next_d = d->next;
			/* check timeout first */
			/* only certain states in the login should be timed out */
			if ((d->connected == CON_GET_NAME)  || 
					(d->connected == CON_PASSWORD) ||
					(d->connected == CON_MENU) ||
					(d->connected == CON_RMOTD))
				if ((time(0) - d->login_time) > 200)
				{
					close_socket(d);
					continue;
				}
     	if (FD_ISSET(d->descriptor, &input_set))
				if (process_input(d) < 0)
  				close_socket(d);
    }

    /* process commands we just read from process_input */
    for (d = descriptor_list; d; d = next_d) {
      next_d = d->next;

      if ((--(d->wait) <= 0) && get_from_q(&d->input, comm, &aliased)) {
	if (d->character) {
	  d->character->char_specials.timer = 0;
	  if (!d->connected && GET_WAS_IN(d->character) != NOWHERE) {
	    if (d->character->in_room != NOWHERE)
	      char_from_room(d->character);
	    char_to_room(d->character, GET_WAS_IN(d->character));
	    GET_WAS_IN(d->character) = NOWHERE;
	    act("$n has returned.", TRUE, d->character, 0, 0, TO_ROOM);
	  }
	}
	d->wait = 1;
	d->prompt_mode = 1;
        
	if (d->connected==CON_OLE)
	  command_interpreter(d->character, comm);

       /* reversed these top 2 if checks so that you can use the page_string */
       /* function in the editor */
        else if (d->showstr_count)   /* reading something w/ pager     */
          show_string(d, comm);
        else if (d->str) {                /* writing boards, mail, etc.     */
          string_add(d, strcat(comm,"&n"));
	} else if (d->connected != CON_PLAYING)
  	nanny(d, comm);
	else {
	  if (aliased)
	    d->prompt_mode = 0;
	  else {
	    if (perform_alias(d, comm))
	      get_from_q(&d->input, comm, &aliased);
	  }
	  command_interpreter(d->character, comm);
	}
      }
    }

    /* send queued output out to the operating system */
    for (d = descriptor_list; d; d = next_d) {
      next_d = d->next;
      if (FD_ISSET(d->descriptor, &output_set) && *(d->output))
	if (process_output(d) < 0)
	  close_socket(d);
	else
	  d->prompt_mode = 1;
    }

    /* kick out folks in the CON_CLOSE state */
    for (d = descriptor_list; d; d = next_d) {
      next_d = d->next;
      if (STATE(d) == CON_CLOSE)
	close_socket(d);
    }

    /* give each descriptor an appropriate prompt */
    for (d = descriptor_list; d; d = d->next) {
      if (d->prompt_mode) {
	make_prompt(d);
	d->prompt_mode = 0;
      }
    }

    /* handle heartbeat stuff */
    /* Note: pulse now changes every 0.10 seconds  */

    pulse++;

    if (!(pulse % PULSE_CASINO)) {
      check_games();	// Tali's gambling.
      rotate_arms();	// Tali's island forever spec.	
    }

    if (!(pulse % PULSE_ZONE))
      zone_update();
    if (!(pulse % PULSE_MOBILE))
      mobile_activity();
    if (!(pulse % PULSE_VIOLENCE)) {
      handle_damage_rooms(); 	// Island forever room specials
      perform_violence();
    }


/* pulse to test random damage - Vader */
    if (!(pulse % (PULSE_RANDAM * 3))) {
      do_arrows();
      }

/* pulse to move the toboggans along so they move each second - Vader */
    if (!(pulse % PULSE_TOB)) {
      for(i = descriptor_list; i ; i = i->next) {
        if(i->character && (tob = GET_EQ(i->character,WEAR_HOLD)))
          if(GET_OBJ_VNUM(tob) == 22327 && GET_OBJ_VAL(tob,0) == 1) {
            GET_OBJ_SPEC(tob) (i->character,tob,0,"");
            }
        }
      }
/* pulse to move rollerbladers - Vader */
    if(!(pulse % PULSE_ROLLER)) {
      for(i = descriptor_list; i ; i = i->next) {
        if(i->character && (tob = GET_EQ(i->character,WEAR_FEET)))
          if(GET_OBJ_VNUM(tob) == 22328)
            if(GET_OBJ_VAL(tob,0) == 1)
              GET_OBJ_SPEC(tob) (i->character,tob,0,"");
            else if(number(0,25) == 1 && GET_POS(i->character) == POS_STANDING) {
              GET_POS(i->character) = POS_SITTING;
              act("$n's feet roll out from under $m!",FALSE,i->character,0,0,TO_ROOM);
              act("$e lands on $s butt with a thud.",FALSE,i->character,0,0,TO_ROOM);
              send_to_char("Your feet seem to roll out from under you!\r\n",i->character);
              }
        }
      }
 

    if (!(pulse % (SECS_PER_MUD_HOUR * PASSES_PER_SEC))) {
      weather_and_time(1);
      affect_update();
      point_update();
      fflush(player_fl);
    }
    if (auto_save)
      if (!(pulse % (60 * PASSES_PER_SEC)))	/* 1 minute */
	if (++mins_since_crashsave >= autosave_time) {
	  mins_since_crashsave = 0;
	  Crash_save_all();
	  House_save_all();
	}
    if (!(pulse % (300 * PASSES_PER_SEC)))	/* 5 minutes */
      record_usage();

    if (!(pulse % (120 * PASSES_PER_SEC)))      /* 2 minutes */ 
    {   
      move_ship();
      move_falcon();
    }
 
    if (pulse >= (30 * 60 * PASSES_PER_SEC)) {	/* 30 minutes */
      pulse = 0;
    }
    tics++;			/* tics since last checkpoint signal */
  }
}



/* ******************************************************************
*  general utility stuff (for local use)                            *
****************************************************************** */

struct timeval timediff(struct timeval * a, struct timeval * b)
{
  struct timeval rslt, tmp;

  tmp = *a;

  if ((rslt.tv_usec = tmp.tv_usec - b->tv_usec) < 0) {
    rslt.tv_usec += 1000000;
    --(tmp.tv_sec);
  }
  if ((rslt.tv_sec = tmp.tv_sec - b->tv_sec) < 0) {
    rslt.tv_usec = 0;
    rslt.tv_sec = 0;
  }
  return rslt;
}

void record_usage(void)
{
  int sockets_connected = 0, sockets_playing = 0;
  struct descriptor_data *d;
  char buf[256];

  for (d = descriptor_list; d; d = d->next) {
    sockets_connected++;
    if (!d->connected)
      sockets_playing++;
  }

  sprintf(buf, "nusage: %-3d sockets connected, %-3d sockets playing",
	  sockets_connected, sockets_playing);
  log(buf);

#ifdef RUSAGE
  {
    struct rusage ru;

    getrusage(0, &ru);
    sprintf(buf, "rusage: %d %d %d %d %d %d %d",
	    ru.ru_utime.tv_sec, ru.ru_stime.tv_sec, ru.ru_maxrss,
	    ru.ru_ixrss, ru.ru_ismrss, ru.ru_idrss, ru.ru_isrss);
    log(buf);
  }
#endif

}


/* modified to add colour prompts n Health-o-Meter(tm) - Vader */
void make_prompt(struct descriptor_data * d)
{
  extern int level_exp[LVL_CHAMP + 1];
  float health;
  double temp;
  int bar,i;
  char prompt[MAX_INPUT_LENGTH];


  if (d->connected==CON_OLE)
    write_to_descriptor(d->descriptor, "");

   /* reversed these top 2 if checks so that page_string() would work in */
   /* the editor */
  else if (d->showstr_count) {
     sprintf(prompt,
            "\r\n\r[ Return to continue, (q)uit, (r)efresh, (b)ack, or page number (%d/%d) ]",
            d->showstr_page, d->showstr_count);
      write_to_descriptor(d->descriptor, prompt);
  }
  else if (d->str)
    write_to_descriptor(d->descriptor, "] ");
  else if (!d->connected) {

    *prompt = '\0';

    if (GET_INVIS_LEV(d->character)) {
	// Char spec invis?
	if( GET_INVIS_TYPE(d->character) == -2 )
	   sprintf(prompt, "i-%s ", get_name_by_id(GET_INVIS_LEV(d->character)));
	// Level specific?
	else if(GET_INVIS_TYPE(d->character) == -1 )
	   sprintf(prompt, "i%ds ", GET_INVIS_LEV(d->character));
	// Standard invis
	else if( GET_INVIS_TYPE(d->character) == 0 )
      	   sprintf(prompt, "i%d ", GET_INVIS_LEV(d->character));
        else
	   sprintf(prompt, "i%d-%d ", GET_INVIS_LEV(d->character),
	        GET_INVIS_TYPE(d->character) );
     }
/* modifications for cooly, frilly, golly, sticky prompties! - Vader */
    if (PRF_FLAGGED(d->character, PRF_DISPHP)) {
      if(GET_HIT(d->character) < GET_MAX_HIT(d->character) / 4)
        sprintf(buf,"%s",CCRED(d->character,C_NRM));
      else if(GET_HIT(d->character) < GET_MAX_HIT(d->character) / 2)
             sprintf(buf,"%s",CCBRED(d->character,C_NRM));
      else if(GET_HIT(d->character) < GET_MAX_HIT(d->character))
             sprintf(buf,"%s",CCBYEL(d->character,C_NRM));
      else sprintf(buf,"%s",CCBGRN(d->character,C_NRM));
      sprintf(prompt, "%s%s%d%sH ",prompt,buf,GET_HIT(d->character),
              CCWHT(d->character,C_NRM));
      }
 
    if (PRF_FLAGGED(d->character, PRF_DISPMANA)) {
      if(GET_MANA(d->character) < GET_MAX_MANA(d->character) / 4)
        sprintf(buf,"%s",CCRED(d->character,C_NRM));
      else if(GET_MANA(d->character) < GET_MAX_MANA(d->character) / 2)
             sprintf(buf,"%s",CCBRED(d->character,C_NRM));
      else if(GET_MANA(d->character) < GET_MAX_MANA(d->character))
             sprintf(buf,"%s",CCBYEL(d->character,C_NRM));
      else sprintf(buf,"%s",CCBGRN(d->character,C_NRM));
      sprintf(prompt, "%s%s%d%sM ",prompt,buf,GET_MANA(d->character),
              CCWHT(d->character,C_NRM));
      }
 
    if (PRF_FLAGGED(d->character, PRF_DISPMOVE)) {
      if(GET_MOVE(d->character) < GET_MAX_MOVE(d->character) / 4)
        sprintf(buf,"%s",CCRED(d->character,C_NRM));
      else if(GET_MOVE(d->character) < GET_MAX_MOVE(d->character) / 2)
             sprintf(buf,"%s",CCBRED(d->character,C_NRM));
      else if(GET_MOVE(d->character) < GET_MAX_MOVE(d->character))
             sprintf(buf,"%s",CCBYEL(d->character,C_NRM));
      else sprintf(buf,"%s",CCBGRN(d->character,C_NRM));
      sprintf(prompt, "%s%s%d%sV ",prompt,buf,GET_MOVE(d->character),
              CCWHT(d->character,C_NRM));
      }

      if (PRF_FLAGGED(d->character, PRF_DISPEXP)) 
	if (GET_LEVEL(d->character) >= LVL_CHAMP)
            sprintf(prompt,"%sX:%s*%s ", prompt, CCBGRY(d->character,C_NRM), CCBGRY(d->character,C_NRM));
        else{
	    temp = ((double)((level_exp[GET_LEVEL(d->character)]) - GET_EXP(d->character))/ 1000000);
	    if (temp>=1)
	       sprintf(prompt, "%sX:%s%3.1fm%s ", prompt, CCBGRN(d->character,C_NRM), temp, CCBGRY(d->character,C_NRM));	
	    else{
		temp = ((double)((level_exp[GET_LEVEL(d->character)]) - GET_EXP(d->character))/ 1000);
	        if (temp>=1)
	            sprintf(prompt, "%sX:%s%3.1fk%s ", prompt, CCBGRN(d->character,C_NRM), temp, CCBGRY(d->character,C_NRM));	
                else
	          sprintf(prompt, "%sX:%s%d%s ", prompt, CCBGRN(d->character,C_NRM), (level_exp[GET_LEVEL(d->character)]) - GET_EXP(d->character), CCBGRY(d->character,C_NRM)); 
            }
         }

     if (PRF_FLAGGED(d->character, PRF_DISPALIGN)){
        if (GET_ALIGNMENT(d->character) > 350)
          sprintf(buf, "%s", CCCYN(d->character, C_NRM));
	else
	   if (GET_ALIGNMENT(d->character) < -350)
  	     sprintf(buf, "%s", CCRED(d->character, C_NRM));
	else
	   sprintf(buf, "%s", CCWHT(d->character, C_NRM));

	sprintf(prompt, "%s%sAl:%s%d%s ", prompt, CCBRED(d->character, C_NRM), buf, GET_ALIGNMENT(d->character), CCBGRY(d->character, C_NRM));
     }

/* modifications to add the Health-o-Meter(tm) to the prompt - Vader */
    if(FIGHTING(d->character))
      if(GET_HIT(FIGHTING(d->character)) > 0) {
        health = ((float)GET_HIT(FIGHTING(d->character)) /  
                  (float)GET_MAX_HIT(FIGHTING(d->character))) * 100;
        bar = health / 5;
        sprintf(prompt,"%s%s[%s",prompt,CCBGRY(d->character,C_NRM),
                CCGRN(d->character,C_NRM));
        for(i=0;i<=bar && i<=19;i++) {
          sprintf(prompt,"%s*",prompt);
          }
        if(bar < 19) {
          sprintf(prompt,"%s%s",prompt,CCRED(d->character,C_NRM));
          for(i=bar;i<19;i++) {
            sprintf(prompt,"%s-",prompt);
            }
          }
        sprintf(prompt,"%s%s] ",prompt,CCBGRY(d->character,C_NRM));
      } else {
        sprintf(prompt,"%s%s[%s--------------------%s] ",prompt,
                CCBGRY(d->character,C_NRM),CCRED(d->character,C_NRM),
                CCBGRY(d->character,C_NRM));
        }
    
    sprintf(prompt,"%s%s%s",prompt,CCWHT(d->character, C_NRM)
            ,CCNRM(d->character, C_NRM));
    strcat(prompt, "> ");
    write_to_descriptor(d->descriptor, prompt);
  }
}


void write_to_q(char *txt, struct txt_q * queue, int aliased)
{
  struct txt_block *new;

  CREATE(new, struct txt_block, 1);
  CREATE(new->text, char, strlen(txt) + 1);
  strcpy(new->text, txt);
  new->aliased = aliased;

  /* queue empty? */
  if (!queue->head) {
    new->next = NULL;
    queue->head = queue->tail = new;
  } else {
    queue->tail->next = new;
    queue->tail = new;
    new->next = NULL;
  }
}



int get_from_q(struct txt_q * queue, char *dest, int *aliased)
{
  struct txt_block *tmp;

  /* queue empty? */
  if (!queue->head)
    return 0;

  tmp = queue->head;
  strcpy(dest, queue->head->text);
  *aliased = queue->head->aliased;
  queue->head = queue->head->next;

  free(tmp->text);
  free(tmp);

  return 1;
}



/* Empty the queues before closing connection */
void flush_queues(struct descriptor_data * d)
{
  int dummy;

  if (d->large_outbuf) {
    d->large_outbuf->next = bufpool;
    bufpool = d->large_outbuf;
  }
  while (get_from_q(&d->input, buf2, &dummy));
}


void write_to_output(const char *txt, struct descriptor_data * t)
{
  int size;

  size = strlen(txt);

  /* if we're in the overflow state already, ignore this new output */
  if (t->bufptr < 0)
    return;

  /* if we have enough space, just write to buffer and that's it! */
  if (t->bufspace >= size) {
    strcpy(t->output + t->bufptr, txt);
    t->bufspace -= size;
    t->bufptr += size;
  } else {			/* otherwise, try switching to a lrg buffer */
    if (t->large_outbuf || ((size + strlen(t->output)) > LARGE_BUFSIZE)) {
      /*
       * we're already using large buffer, or even the large buffer isn't big
       * enough -- switch to overflow state
       */
      t->bufptr = -1;
      buf_overflows++;
      return;
    }
    buf_switches++;

    /* if the pool has a buffer in it, grab it */
    if (bufpool != NULL) {
      t->large_outbuf = bufpool;
      bufpool = bufpool->next;
    } else {			/* else create a new one */
      CREATE(t->large_outbuf, struct txt_block, 1);
      CREATE(t->large_outbuf->text, char, LARGE_BUFSIZE);
      buf_largecount++;
    }

    strcpy(t->large_outbuf->text, t->output);	/* copy to big buffer */
    t->output = t->large_outbuf->text;	/* make big buffer primary */
    strcat(t->output, txt);	/* now add new text */

    /* calculate how much space is left in the buffer */
    t->bufspace = LARGE_BUFSIZE - 1 - strlen(t->output);

    /* set the pointer for the next write */
    t->bufptr = strlen(t->output);
  }
}





/* ******************************************************************
*  socket handling                                                  *
****************************************************************** */


int new_descriptor(int s)
{
  int desc, sockets_connected = 0;
  unsigned long addr;
  int i;
  static int last_desc = 0;	/* last descriptor number */
  struct descriptor_data *newd;
  struct sockaddr_in peer;
  struct hostent *from;
  extern char *GREETINGS, *MAINTEN;

  /* accept the new connection */
  i = sizeof(peer);
  if ((desc = accept(s, (struct sockaddr *) & peer, &i)) < 0) {
    perror("Accept");
    return -1;
  }
  /* keep it from blocking */
  nonblock(desc);

  if (mainten) {
    write_to_descriptor(desc, MAINTEN);
    close(desc);
    return 0;
  } else

  /* make sure we have room for it */
  for (newd = descriptor_list; newd; newd = newd->next)
    sockets_connected++;

  if (sockets_connected >= avail_descs) {
    write_to_descriptor(desc, "Sorry, CircleMUD is full right now... try again later!  :-)\r\n");
    close(desc);
    return 0;
  }
  /* create a new descriptor */
  CREATE(newd, struct descriptor_data, 1);
  memset((char *) newd, 0, sizeof(struct descriptor_data));

  /* find the sitename */
  if (nameserver_is_slow || !(from = gethostbyaddr((char *) &peer.sin_addr,
					 sizeof(peer.sin_addr), AF_INET))) {
    if (!nameserver_is_slow)
      perror("gethostbyaddr");
    addr = ntohl(peer.sin_addr.s_addr);
    sprintf(newd->host, "%d.%d.%d.%d", (int)((addr & 0xFF000000) >> 24),
	    (int)((addr & 0x00FF0000) >> 16), (int)((addr & 0x0000FF00) >> 8),
	    (int)((addr & 0x000000FF)));
  } else {
    strncpy(newd->host, from->h_name, HOST_LENGTH);
    *(newd->host + HOST_LENGTH) = '\0';
  }

  /* determine if the site is banned */
  if (isbanned(newd->host) == BAN_ALL) {
    close(desc);
    sprintf(buf2, "Connection attempt denied from [%s]", newd->host);
    mudlog(buf2, CMP, LVL_GOD, TRUE);
    free(newd);
    return 0;
  }

#if 0
  /* Log new connections - probably unnecessary, but you may want it */
  sprintf(buf2, "New connection from [%s]", newd->host);
  mudlog(buf2, CMP, LVL_GOD, FALSE);
#endif

  /* initialize descriptor data */
  newd->descriptor = desc;
  newd->connected = CON_GET_NAME;
  newd->pos = -1;
  newd->wait = 1;
  newd->output = newd->small_outbuf;
  newd->bufspace = SMALL_BUFSIZE - 1;
  newd->next = descriptor_list;
  newd->login_time = time(0);

  if (++last_desc == 1000)
    last_desc = 1;
  newd->desc_num = last_desc;

  /* prepend to list */
  descriptor_list = newd;

    SEND_TO_Q(GREETINGS, newd);

  return 0;
}



int process_output(struct descriptor_data * t)
{
  static char i[LARGE_BUFSIZE + GARBAGE_SPACE];
  static int result;

  /* we may need this \r\n for later -- see below */
  strcpy(i, "\r\n");

  /* now, append the 'real' output */
  strcpy(i + 2, t->output);

  /* if we're in the overflow state, notify the user */
  if (t->bufptr < 0)
    strcat(i, "**OVERFLOW**");

  /* add the extra CRLF if the person isn't in compact mode */
  if (!t->connected && t->character && !PRF_FLAGGED(t->character, PRF_COMPACT))
    strcat(i + 2, "\r\n");

  /* Easy color - DM */
  if (t->character)
    if (GET_LEVEL(t->character) > 0)
      proc_color(i, (clr(t->character, C_NRM)), sizeof(i)-GARBAGE_SPACE);
    else 
      proc_color(i, C_OFF, sizeof(i)-GARBAGE_SPACE);

  /*
   * now, send the output.  If this is an 'interruption', use the prepended
   * CRLF, otherwise send the straight output sans CRLF.
   */
  if (!t->prompt_mode)		/* && !t->connected) */
    result = write_to_descriptor(t->descriptor, i);
  else
    result = write_to_descriptor(t->descriptor, i + 2);

  /* handle snooping: prepend "% " and send to snooper */
  if (t->snoop_by) {
    SEND_TO_Q("% ", t->snoop_by);
    SEND_TO_Q(t->output, t->snoop_by);
    SEND_TO_Q("%%", t->snoop_by);
  }
  /*
   * if we were using a large buffer, put the large buffer on the buffer pool
   * and switch back to the small one
   */
  if (t->large_outbuf) {
    t->large_outbuf->next = bufpool;
    bufpool = t->large_outbuf;
    t->large_outbuf = NULL;
    t->output = t->small_outbuf;
  }
  /* reset total bufspace back to that of a small buffer */
  t->bufspace = SMALL_BUFSIZE - 1;
  t->bufptr = 0;
  *(t->output) = '\0';

  return result;
}



int write_to_descriptor(int desc, char *txt)
{
  int total, bytes_written;

  total = strlen(txt);

  do {
    if ((bytes_written = write(desc, txt, total)) < 0) {
#ifdef EWOULDBLOCK
      if (errno == EWOULDBLOCK)
	errno = EAGAIN;
#endif
      if (errno == EAGAIN)
	log("process_output: socket write would block, about to close");
      else
	perror("Write to socket");
      return -1;
    } else {
      txt += bytes_written;
      total -= bytes_written;
    }
  } while (total > 0);

  return 0;
}


/*
 * ASSUMPTION: There will be no newlines in the raw input buffer when this
 * function is called.  We must maintain that before returning.
 */
int process_input(struct descriptor_data * t)
{
  int buf_length, bytes_read, space_left, failed_subst;
  char *ptr, *read_point, *write_point, *nl_pos = NULL;
  char tmp[MAX_INPUT_LENGTH + 8];

  /* first, find the point where we left off reading data */
  buf_length = strlen(t->inbuf);
  read_point = t->inbuf + buf_length;

  do {
    if ((bytes_read = read(t->descriptor, read_point,
			   MAX_RAW_INPUT_LENGTH - buf_length - 1)) < 0) {
#ifdef EWOULDBLOCK
      if (errno == EWOULDBLOCK)
	errno = EAGAIN;
#endif
      if (errno != EAGAIN) {
	perror("process_input: about to lose connection");
	return -1;		/* some error condition was encountered on
				 * read */
      } else
	return 0;		/* the read would have blocked: just means no
				 * data there */
    } else if (bytes_read == 0) {
      log("EOF on socket read (connection broken by peer)");
      return -1;
    }
    /* at this point, we know we got some data from the read */

    *(read_point + bytes_read) = '\0';	/* terminate the string */

    /* search for a newline in the data we just read */
    for (ptr = read_point; *ptr && !nl_pos; ptr++)
      if (ISNEWL(*ptr))
	nl_pos = ptr;

    read_point += bytes_read;
  } while (nl_pos == NULL);

  /*
   * okay, at this point we have at least one newline in the string; now we
   * can copy the formatted data to a new array for further processing.
   */

  read_point = t->inbuf;

  while (nl_pos != NULL) {
    write_point = tmp;
    space_left = MAX_INPUT_LENGTH - 3;

    for (ptr = read_point; (space_left > 0) && (ptr < nl_pos); ptr++) {
      if (*ptr == '\b') {	/* handle backspacing */
	if (write_point > tmp) {
	  if (*(--write_point) == '$') {
	    write_point--;
	    space_left += 2;
	  } else
	    space_left++;
	}
      } else if (isascii(*ptr) && isprint(*ptr)) {
	if ((*(write_point++) = *ptr) == '$') {	/* copy one character */
	  *(write_point++) = '$';	/* if it's a $, double it */
	  space_left -= 2;
	} else
	  space_left--;
      }
    }

    *write_point = '\0';

    if ((space_left <= 0) && (ptr < nl_pos)) {
      char buffer[MAX_INPUT_LENGTH + 64];
    /* Artus - This is being too nice. And crashing is not prevented.
     * Let's disconnect them, instead.
      sprintf(buffer, "Line too long.  Truncated to:\r\n%s\r\n", tmp);
      if (write_to_descriptor(t->descriptor, buffer) < 0)
	return -1; */
      write_to_descriptor(t->descriptor, "Line too long, please break it up before sending again.\r\n");
      if (t->snoop_by)
        SEND_TO_Q("Line too long, please break it up before sending again.\r\n", t->snoop_by);
      return -1;
    }
    if (t->snoop_by) {
      SEND_TO_Q("% ", t->snoop_by);
      SEND_TO_Q(tmp, t->snoop_by);
      SEND_TO_Q("\r\n", t->snoop_by);
    }
    failed_subst = 0;

    if (*tmp == '!')
      strcpy(tmp, t->last_input);
    else if (*tmp == '^') {
      if (!(failed_subst = perform_subst(t, t->last_input, tmp)))
	strcpy(t->last_input, tmp);
    } else
      strcpy(t->last_input, tmp);

    if (!failed_subst)
      write_to_q(tmp, &t->input, 0);

    /* find the end of this line */
    while (ISNEWL(*nl_pos))
      nl_pos++;

    /* see if there's another newline in the input buffer */
    read_point = ptr = nl_pos;
    for (nl_pos = NULL; *ptr && !nl_pos; ptr++)
      if (ISNEWL(*ptr))
	nl_pos = ptr;
  }

  /* now move the rest of the buffer up to the beginning for the next pass */
  write_point = t->inbuf;
  while (*read_point)
    *(write_point++) = *(read_point++);
  *write_point = '\0';

  return 1;
}



/* perform substitution for the '^..^' csh-esque syntax */
int perform_subst(struct descriptor_data * t, char *orig, char *subst)
{
  char new[MAX_INPUT_LENGTH + 5];

  char *first, *second, *strpos;

  first = subst + 1;
  if (!(second = strchr(first, '^'))) {
    SEND_TO_Q("Invalid substitution.\r\n", t);
    return 1;
  }
  *(second++) = '\0';

  if (!(strpos = strstr(orig, first))) {
    SEND_TO_Q("Invalid substitution.\r\n", t);
    return 1;
  }
  strncpy(new, orig, (strpos - orig));
  new[(strpos - orig)] = '\0';
  strcat(new, second);
  if (((strpos - orig) + strlen(first)) < strlen(orig))
    strcat(new, strpos + strlen(first));
  strcpy(subst, new);

  return 0;
}



void close_socket(struct descriptor_data * d)
{
  struct descriptor_data *temp;
  char buf[100];

  close(d->descriptor);
  flush_queues(d);

  /* Forget snooping */
  if (d->snooping)
    d->snooping->snoop_by = NULL;

  if (d->snoop_by) {
    SEND_TO_Q("Your victim is no longer among us.\r\n", d->snoop_by);
    d->snoop_by->snooping = NULL;
  }
  if ((d->character) && (GET_LEVEL(d->character) <= LVL_OWNER)) {
    if (d->connected == CON_PLAYING) {
      save_char(d->character, NOWHERE);
      act("$n has lost $s link.", TRUE, d->character, 0, 0, TO_ROOM);
      sprintf(buf, "Closing link to: %s.", GET_NAME(d->character));
      mudlog(buf, NRM, MAX(LVL_ETRNL1, GET_INVIS_LEV(d->character)), TRUE);
      d->character->desc = NULL;
      sprintf(buf, "%s has lost %s link.", GET_NAME(d->character) ,HSHR(d->character));
      info_channel(buf , d->character );
     } else {
      sprintf(buf, "Losing player: %s.",
	      GET_NAME(d->character) ? GET_NAME(d->character) : "<null>");
      mudlog(buf, CMP, LVL_ETRNL1, TRUE);
      /* this was crashing when a deleted char dropped link b4 confirm of passwd. 
         crashed in freeing aliases somewhere 
      free_char(d->character); */
    }
  } else
    mudlog("Losing descriptor without char.", CMP, LVL_GOD, TRUE);

  REMOVE_FROM_LIST(d, descriptor_list, next);

  if (d->showstr_head)
    free(d->showstr_head);
  free(d);
}

/*
 * I tried to universally convert Circle over to POSIX compliance, but,
 * alas, some systems are still straggling behind and don't have all the
 * appropriate defines.  In particular, NeXT 2.x defines O_NDELAY but not
 * O_NONBLOCK.  Krusty old NeXT machines!  (Thanks to Michael Jones for
 * this and various other NeXT fixes.)
 */
#ifndef O_NONBLOCK
#define O_NONBLOCK O_NDELAY
#endif

void nonblock(int s)
{
  if (fcntl(s, F_SETFL, O_NONBLOCK) < 0) {
    perror("Fatal error executing nonblock (comm.c)");
    exit(1);
  }
}


/* ******************************************************************
*  signal-handling functions (formerly signals.c)                   *
****************************************************************** */


void checkpointing()
{
  if (!tics) {
    log("SYSERR: CHECKPOINT shutdown: tics not updated");
    abort();
  } else
    tics = 0;
}


void reread_wizlists()
{
  void reboot_wizlists(void);

  mudlog("Rereading wizlists.", CMP, LVL_GOD, FALSE);
  reboot_wizlists();
}


void unrestrict_game()
{
  extern struct ban_list_element *ban_list;
  extern int num_invalid;

  mudlog("Received SIGUSR2 - completely unrestricting game (emergent)",
	 BRF, LVL_GOD, TRUE);
  ban_list = NULL;
  restrict = 0;
  num_invalid = 0;
}


void hupsig()
{
  log("Received SIGHUP, SIGINT, or SIGTERM.  Shutting down...");
  exit(0);			/* perhaps something more elegant should
				 * substituted */
}


/*
 * This is an implementation of signal() using sigaction() for portability.
 * (sigaction() is POSIX; signal() is not.)  Taken from Stevens' _Advanced
 * Programming in the UNIX Environment_.  We are specifying that all system
 * calls _not_ be automatically restarted because BSD systems do not restart
 * select(), even if SA_RESTART is used.
 *
 * Note that NeXT 2.x is not POSIX and does not have sigaction; therefore,
 * I just define it to be the old signal.  If your system doesn't have
 * sigaction either, you can use the same fix.
 */

#if defined(NeXT)
#define my_dignal(signo, func) signal(signo, func)
#else
sigfunc *my_signal(int signo, sigfunc * func)
{
  struct sigaction act, oact;

  act.sa_handler = func;
  sigemptyset(&act.sa_mask);
  act.sa_flags = 0;
#ifdef SA_INTERRUPT
  act.sa_flags |= SA_INTERRUPT;	/* SunOS */
#endif

  if (sigaction(signo, &act, &oact) < 0)
    return SIG_ERR;

  return oact.sa_handler;
}
#endif /* NeXT */

void signal_setup(void)
{
  struct itimerval itime;
  struct timeval interval;

  /* user signal 1: reread wizlists.  Used by autowiz system. */
  my_signal(SIGUSR1, reread_wizlists);

  /*
   * user signal 2: unrestrict game.  Used for emergencies if you lock
   * yourself out of the MUD somehow.  (Duh...)
   */
  my_signal(SIGUSR2, unrestrict_game);

  /*
   * set up the deadlock-protection so that the MUD aborts itself if it gets
   * caught in an infinite loop for more than 3 minutes
   */
  interval.tv_sec = 180;
  interval.tv_usec = 0;
  itime.it_interval = interval;
  itime.it_value = interval;
  setitimer(ITIMER_VIRTUAL, &itime, NULL);
  my_signal(SIGVTALRM, checkpointing);

  /* just to be on the safe side: */
  my_signal(SIGHUP, hupsig);
  my_signal(SIGINT, hupsig);
  my_signal(SIGTERM, hupsig);
  my_signal(SIGPIPE, SIG_IGN);
  my_signal(SIGALRM, SIG_IGN);
}

/* ****************************************************************
*       Public routines for system-to-player-communication        *
*******************************************************************/



void send_to_char(char *messg, struct char_data * ch)
{
  if (ch->desc && messg)
    SEND_TO_Q(messg, ch->desc);
}


void send_to_all(char *messg)
{
  struct descriptor_data *i;

  if (messg)
    for (i = descriptor_list; i; i = i->next)
      if (!i->connected)
	SEND_TO_Q(messg, i);
}


void send_to_outdoor(char *messg)
{
  struct descriptor_data *i;

  if (!messg || !*messg)
    return;

  for (i = descriptor_list; i; i = i->next)
    if (!i->connected && i->character && AWAKE(i->character) &&
	OUTSIDE(i->character))
      SEND_TO_Q(messg, i);
}



void send_to_room(char *messg, int room)
{
  struct char_data *i;

  if (messg)
    for (i = world[room].people; i; i = i->next_in_room)
      if (i->desc)
	SEND_TO_Q(messg, i->desc);
}



char *ACTNULL = "<NULL>";

#define CHECK_NULL(pointer, expression) \
  if ((pointer) == NULL) i = ACTNULL; else i = (expression);


/* higher-level communication: the act() function */
void perform_act(char *orig, struct char_data * ch, struct obj_data * obj,
		      void *vict_obj, struct char_data * to)
{
  register char *i, *buf;
  static char lbuf[MAX_STRING_LENGTH];

  buf = lbuf;

  for (;;) {
    if (*orig == '$') {
      switch (*(++orig)) {
      case 'n':
	i = PERS(ch, to);
	break;
      case 'N':
	CHECK_NULL(vict_obj, PERS((struct char_data *) vict_obj, to));
	break;
      case 'm':
	i = HMHR(ch);
	break;
      case 'M':
	CHECK_NULL(vict_obj, HMHR((struct char_data *) vict_obj));
	break;
      case 's':
	i = HSHR(ch);
	break;
      case 'S':
	CHECK_NULL(vict_obj, HSHR((struct char_data *) vict_obj));
	break;
      case 'e':
	i = HSSH(ch);
	break;
      case 'E':
	CHECK_NULL(vict_obj, HSSH((struct char_data *) vict_obj));
	break;
      case 'o':
	CHECK_NULL(obj, OBJN(obj, to));
	break;
      case 'O':
	CHECK_NULL(vict_obj, OBJN((struct obj_data *) vict_obj, to));
	break;
      case 'p':
	CHECK_NULL(obj, OBJS(obj, to));
	break;
      case 'P':
	CHECK_NULL(vict_obj, OBJS((struct obj_data *) vict_obj, to));
	break;
      case 'a':
	CHECK_NULL(obj, SANA(obj));
	break;
      case 'A':
	CHECK_NULL(vict_obj, SANA((struct obj_data *) vict_obj));
	break;
      case 'T':
	CHECK_NULL(vict_obj, (char *) vict_obj);
	break;
      case 'F':
	CHECK_NULL(vict_obj, fname((char *) vict_obj));
	break;
      case '$':
	i = "$";
	break;
      default:
	log("SYSERR: Illegal $-code to act():");
	strcpy(buf1, "SYSERR: ");
	strcat(buf1, orig);
	log(buf1);
	break;
      }
      while ((*buf = *(i++)))
	buf++;
      orig++;
    } else if (!(*(buf++) = *(orig++)))
      break;
  }

  *(--buf) = '\r';
  *(++buf) = '\n';
  *(++buf) = '\0';

  SEND_TO_Q(CAP(lbuf), to->desc);
}


#define SENDOK(ch) ((ch)->desc && (AWAKE(ch) || sleep) && \
		    !PLR_FLAGGED((ch), PLR_WRITING))

void act(char *str, int hide_invisible, struct char_data * ch,
	      struct obj_data * obj, void *vict_obj, int type)
{
  struct char_data *to;
  static int sleep;

  if (!str || !*str)
    return;

  /*
   * Warning: the following TO_SLEEP code is a hack.
   * 
   * I wanted to be able to tell act to deliver a message regardless of sleep
   * without adding an additional argument.  TO_SLEEP is 128 (a single bit
   * high up).  It's ONLY legal to combine TO_SLEEP with one other TO_x
   * command.  It's not legal to combine TO_x's with each other otherwise.
   */

  /* check if TO_SLEEP is there, and remove it if it is. */
  if ((sleep = (type & TO_SLEEP)))
    type &= ~TO_SLEEP;


  if (type == TO_CHAR) {
    if (ch && SENDOK(ch))
      perform_act(str, ch, obj, vict_obj, ch);
    return;
  }
  if (type == TO_VICT) {
    if ((to = (struct char_data *) vict_obj) && SENDOK(to))
      perform_act(str, ch, obj, vict_obj, to);
    return;
  }
  /* ASSUMPTION: at this point we know type must be TO_NOTVICT or TO_ROOM */

  if (ch && ch->in_room != NOWHERE)
    to = world[ch->in_room].people;
  else if (obj && obj->in_room != NOWHERE)
    to = world[obj->in_room].people;
  else {
    log("SYSERR: no valid target to act()!");
    return;
  }

  for (; to; to = to->next_in_room)
    if (SENDOK(to) && !(hide_invisible && ch && !CAN_SEE(to, ch)) &&
	(to != ch) && (type == TO_ROOM || (to != vict_obj)))
      perform_act(str, ch, obj, vict_obj, to);
}

/* ****************************************************************
*       Misc. routines 					          *
*******************************************************************/
