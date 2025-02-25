/* CASINO2.C
 *		Author		:	Jason Theofilos
 *		Date Started	: 	10th September, 1999
 		Revised		:	12th September, 1999
 *		Purpose		:	Defines the casino's special procedures
 *
 *		Language	:	C
 *		Location	: 	Churchill, Gippsland, Australia.
 */

// Include CircleMUD's header files
#include "conf.h"
#include "sysdep.h"

/*
// Include system header files

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
 */

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "db.h"

// Own header
#include "casino.h"

// Extern the world for the SPEC_PROC
extern struct room_data *world;

extern struct game_data casino_race;
extern struct blackjack_data casino_blackjack;

/* First set of functions - Those used by outside modules */
// Use this function to maintain all casino games through calls.
void check_games() 
{
  // Check if gambling has been disabled
  if (casino_race.timetogame == NO_GAME)
    return;
  // Pulse all games that are time based
  casino_race.timetogame--;
  // Call functions to handle games
  check_races();
  check_blackjack();
}

// All games that require initialising on mud bootup should be here
void init_games() 
{
  init_race();
  init_blackjack();
}

// Initialises all the blackjack structures 
void init_blackjack() 
{
  int counter = 0, cardcounter =0;
  struct game_item *bjitem;
  // Set players to 0
  casino_blackjack.numplayers = 0;
  // Clear out all storage
  while (counter < MAX_GAME_BETS)
  {
    bjitem = &casino_blackjack.list[counter];
    bjitem->ch = 0;
    bjitem->amount = 0;
    bjitem->type = 0;
    // Clear out the card storage for each player and dealer
    // 6th card is for information (insurance set, playing against player, etc)
    while (cardcounter < 6)
    {
      casino_blackjack.cards[counter][cardcounter] = 0;
      casino_blackjack.cards[counter + MAX_GAME_BETS][cardcounter] = 0;
      cardcounter++;
    }	
    counter++;
  }
}

/* Function to enforce blackjack policies. 
   Currently just makes sure that a player is in a casino, on pulse */
void check_blackjack() 
{
  int counter = 0;
  struct game_item *bjplayer;
  // Loop through BJ players
  while (counter < MAX_GAME_BETS)
  {
    bjplayer = &casino_blackjack.list[counter];
    if ((bjplayer->ch != 0) &&
        (GET_ROOM_SPEC(IN_ROOM(bjplayer->ch)) != casino))
    {
      send_to_char("You forfeit your blackjack hand.\r\n", bjplayer->ch);
      removeBJPlayer(counter);
    }
    counter++;	// Next player
  }
}

/* Initialises the race structures */
void init_race(void) 
{
  int counter = 0;
  struct game_item *raceitem;

  casino_race.timetogame = DECLARE_GAME;
  casino_race.numbets = 0;
  while (counter < MAX_GAME_BETS)
  {
    raceitem = &casino_race.list[counter];
    raceitem->ch = 0;
    raceitem->amount = 0;
    raceitem->type = 0;
    counter++;
  }
}

/* Handles the racing */
void check_races() 
{
  int counter = 0, result, amount, chance;
  struct game_item *raceitem;

  // If we're starting the race, let the participants know
  if (casino_race.timetogame == START_GAME)
  {
    while (counter < MAX_GAME_BETS)
    {
      raceitem = &casino_race.list[counter];
      if(raceitem->ch != 0) 
	send_to_char("Your race has just begun!\r\n", raceitem->ch);
      counter++;
    }
  }
  
  // If the race has finished, poll the results 
  if (casino_race.timetogame == END_GAME)
  {
    // Inform every participant of their result
    while (counter < MAX_GAME_BETS)
    {
      raceitem = &casino_race.list[counter]; 	// Get race item
      if (raceitem->ch != 0)			// If valid character
      {
	send_to_char("Your race just finished ", raceitem->ch);
	// Calculate chances of winning
	if (raceitem->type == 3)		// if long
	  chance = 18;				// 1 in 18 chance of x9 win
	else if(raceitem->type == 2)		// if medium(default)
	  chance = 9;				// 1 in 9 chance of x4 win
	else
	  chance = 3;				// 1 in 3 chance of x2 win
	// Get a random result, based on bet type
	result = number(1, chance);
	// See if we can pick the winner!
	if (result == number(1, chance))
	{
	  amount = raceitem->amount * (raceitem->type * 2);
	  sprintf(buf, "- congratulations! You won %d!!\r\n", amount);
	  // Artus> BTH
	  // GET_BANK_GOLD(raceitem->ch) += amount;
	  GET_GOLD(raceitem->ch) += amount;
	  send_to_char(buf, raceitem->ch);
	} else
	  send_to_char("..better luck next time.\r\n", raceitem->ch);
      } // if valid character
      counter++;				// Next bet
    } // loop every bet
    init_race();				// Reset for next race
  } // END_RACE

  // In case of error
  if ((casino_race.timetogame < END_GAME) ||
      (casino_race.timetogame > DECLARE_GAME))
  {
    basic_mud_log("Gambling Error: Racing : exceeded boundaries");
    init_race(); 		
  }
}

/* These functions are the actual commands themselves */
ACMD(do_race)
{
  if (IS_NPC(ch))
  {
    send_to_char("We reserve the right to refuse service to you.\r\n", ch);
    return;
  }
  send_to_char("You can't bet on the races here, go find a casino!", ch);
} 

ACMD(do_blackjack) 
{
  if (IS_NPC(ch))
  {
    send_to_char("We reserve the right to refuse service to you.\r\n", ch);
    return;
  }
  send_to_char("You can't play blackjack here, go find a casino!", ch);
}

ACMD(do_slots) 
{

  if (IS_NPC(ch))
  {
    send_to_char("We reserve the right to refuse service to you.\r\n", ch);
    return;
  }
  send_to_char("You can't play slots here, go find a casino!", ch);
}

/* SPECial PROCedures */

/* Main driver function for the casino */
SPECIAL(casino) 
{
  if (!(ch))
    return FALSE;
  if (IS_NPC(ch))
    return (FALSE);
  // Horse Racing...
  if (CMD_IS("race"))
  {
    gamble_race(ch, argument);
    return (TRUE);
  }
  // BlackJack...
  if (CMD_IS("blackjack") || CMD_IS("bj" ))
  {
    gamble_blackjack(ch, argument);
    return (TRUE);
  }
  // Slot Machine...
  if (CMD_IS("slots"))
  {
    gamble_slots(ch, argument);
    return (TRUE);
  }
  return (FALSE);
}


/***** RACING ******/
void gamble_race(struct char_data *ch, char *arg) 
{
  int amount = 0, counter = 0;
  struct game_item *raceitem;

  // Get arguments, second one is allowed to be empty
  two_arguments(arg, buf1, buf2);
  // Check for first argument
  if (!*buf1)
  {
    send_to_char("You need to bet a cash amount.\r\n", ch);
    return;
  }
  // Ensure it's a digit, we're after money, not loaves of bread
  if(!isdigit(*buf1))
  {
    send_to_char("The casino deals strictly in cash.\r\n", ch);
    return;
  }
  // Convert
  amount = atoi(buf1);
  // Check amount is not over max allowed (level * 1000)
  if (amount > (GET_LEVEL(ch) * 1000))
  {
    sprintf(buf, "Your maximum allowed bet is &Y%d&n coins.\r\n", 
	    GET_LEVEL(ch) * 1000);
    send_to_char(buf, ch);
    return;
  }
  // Enforce some kind of minimum
  if (amount < 100)
  {
    send_to_char("Minimum race bet is &Y100&n coins.\r\n", ch);
    return;
  }
  // Don't allow double betting
  while (counter < casino_race.numbets)
  {
    raceitem = &casino_race.list[counter];
    if (ch == raceitem->ch)
    {
      sprintf(buf, "You already have %d riding on this race!\r\n", 
	      raceitem->amount);
      send_to_char(buf, raceitem->ch);
      return;
    }
    counter++;
  }
  // If the race is full
  if (casino_race.numbets >= MAX_GAME_BETS)
  {
    send_to_char("No more bets allowed on this race, wait for the next one.\r\n", ch);
    return;
  }
  // Don't allow late comers 
  if ((casino_race.timetogame == START_GAME) || 
      (casino_race.timetogame == END_GAME))
  {
    send_to_char("You will have to wait for the next race.\r\n", ch);
    return;
  }
  // Okay, ready to bet, see if the player has the cash (uses bank)
  /* Artus> BTH
  if (GET_BANK_GOLD(ch) < amount)
  {
    send_to_char("Your account is a bit light for that wager.\r\n", ch);
    return;
  } */
  if (GET_GOLD(ch) < amount)
  {
    send_to_char("Show me the money!\r\n", ch);
    return;
  }
  // All's well, add the bet
  raceitem = &casino_race.list[casino_race.numbets];
  raceitem->ch = ch;
  raceitem->amount = amount;
  // Calculate what type of bet they desire
  if (*buf2)
  {
    if (strcmp(buf2, "long") == 0) 
      raceitem->type = 3;		// Long call
    else if (strcmp(buf2, "short") == 0)
      raceitem->type = 1;		// Short call
    else
      raceitem->type = 2;		// Medium
  } else
    raceitem->type = 2;			// Default to medium

  casino_race.numbets++;		// Add bet officially
  // Artus> BTH
  // GET_BANK_GOLD(ch) -= amount;	// Lighten players load
  GET_GOLD(ch) -= amount;
  send_to_char("Thank you for your patronage. Good luck!\r\n", ch);
  act("$n places a bet on the next race.", FALSE, ch, 0, 0, TO_ROOM);
  return;
} // gamble_race

/* BLACKJACK Functions */

/* Controls the blackjack games */
/* Artus> I still don't get why this is coming from bank. Changing it to come
 *        from carried gold instead. Just makes more sense to me.
 *        Changes marked with BTC*/
void gamble_blackjack(struct char_data *ch, char *arg) 
{

  int counter = 0, amount = 0;
  struct game_item *bjitem;

  // Get argument
  one_argument(arg, buf1);

  // If there is no argument
  if (!*buf1)
  {
    // See if player is in the game, if they are, show them their cards
    counter = isInGame(ch, 'B');
    if (counter != 0)
    {
      counter--;
      bjitem = getGamePlayer(ch, 'B');// Get player's data
      showBJHand(bjitem, counter);	// Show them their hand
    } else {				// PRompt them to join 
      send_to_char("Need a cash amount to join a game.\r\n", ch);
    }
    return;						// Done, leave
  } // End-if-no-argument

  // Argument exists, so check if it is a digit, or request for max
  if (isdigit(*buf1) || strcmp(buf1, "max") == 0)
  {
    // Check if we can allow new players into the game
    if (casino_blackjack.numplayers >= MAX_GAME_BETS)
    {
      send_to_char("All the blackjack tables are full at the moment.\r\n", ch);
      return;
    }
    // Check if player is already playing BlackJack
    if (isInGame(ch, 'B'))
    {
      send_to_char("Why don't you finish your current game first?\r\n", ch);
      return;
    }
    // GEt the bet amount
    if (strcmp(buf1, "max") == 0)
      amount = GET_LEVEL(ch) * 2000;
    else
      amount = atoi(buf1);
    // Enforce betting limits
    if (amount < 100)
    {
      send_to_char("Minimum house bet is &Y100&n coins.\r\n", ch);
      return;
    }
    if (amount > GET_LEVEL(ch) * 2000)
    {
      sprintf(buf2, "Your maximum allowable wager is &Y%d&n coins.\r\n", 
	      GET_LEVEL(ch) * 2000 );
      send_to_char(buf2, ch);
      return;
    }
    // Game is open, see if player can put their money where their mouth is
    /* Artus> BTH
    if (GET_BANK_GOLD(ch) < amount)
    {
      send_to_char("Your account is too light for that wager.\r\n", ch);
      return;
    } */
    if (GET_GOLD(ch) < amount)
    {
      send_to_char("Show me the money.\r\n", ch);
      return;
    }
    // Player has the cash, lets play
    bjitem = &casino_blackjack.list[ casino_blackjack.numplayers ];
    bjitem->ch = ch;
    bjitem->amount = amount;
    // Artus> BTH
    //GET_BANK_GOLD(ch) -= amount;			// Get money
    GET_GOLD(ch) -= amount;
    // Give the player the first two cards
    casino_blackjack.cards[casino_blackjack.numplayers][0] = getCard();
    casino_blackjack.cards[casino_blackjack.numplayers][1] = getCard();
    // And the dealer, the card that the player can see
    casino_blackjack.cards[casino_blackjack.numplayers + MAX_GAME_BETS][0] = getCard();
    send_to_char("The dealer passes you two cards.\r\n", bjitem->ch);
    // Set the player's score, and show them their hand
    setBJScore(bjitem, casino_blackjack.numplayers);
    showBJHand(bjitem, casino_blackjack.numplayers);
    if (canSplit(casino_blackjack.numplayers))
      send_to_char("Double card. Use the 'split' option to double up.\r\n", ch);
    if (hasBlackjack(casino_blackjack.numplayers))
    {
      sprintf(buf1, "BLACKJACK! Way to go. You won &Y%d&n coins!!\r\n", 
	      (bjitem->amount * 3));
      send_to_char(buf1, bjitem->ch);
      // Artus> BTH
      // GET_BANK_GOLD(bjitem->ch) += bjitem->amount * 3;
      GET_GOLD(bjitem->ch) += bjitem->amount * 3;
      removeBJPlayer(casino_blackjack.numplayers);
    }	
    // Simply add player to game, required even if BlackJack'ed.
    casino_blackjack.numplayers++;
    return;
  } // End-if-digit-or-max
  // Player gave command, process it
  counter = isInGame(ch, 'B');
  if (!counter)
  {
    send_to_char("You have to be in a game before you can give commands.\r\n", ch);
    return;
  }
  counter--;
  // Get player's details
  bjitem = getGamePlayer(ch, 'B');
  // Check player's command
  if (strcmp(buf1, "hit") == 0)			// Player wants another card
  {
    handle_hit(ch);
    return;
  }
  if (strcmp(buf1, "stay") == 0)		// Player is done
  {
    send_to_char("You pass control of the game.\r\n", bjitem->ch);
    endBJGame(bjitem, counter);
    return;
  }
  if (strcmp(buf1, "split") == 0)		// Player wants to split
  {
    handle_split(counter);
    return;
  }
  // Didn't catch command, tell player their options
  send_to_char("You can either 'hit', 'stay' or 'split'.\r\n", ch);
}

/* Handles the hit command for blackjack */
void handle_hit(struct char_data *ch) 
{

  int cardcounter = 0, card, playernum = 0;
  struct game_item *player;

  // Find the player's number
  while (playernum < MAX_GAME_BETS)
  {
    player = &casino_blackjack.list[playernum];
    if ((player->ch != 0) && (player->ch == ch))
      break;
    playernum++;
  }
  // Find the next available slot for the card
  while (cardcounter < 5)
  {
    card = casino_blackjack.cards[playernum][cardcounter];
    if (card == CARD_NONE)		// Found a blank
    {
      card = getCard();			// Get a new card
      if (card == CARD_ACE) 
	send_to_char("The dealer gives you an Ace.\r\n", ch);
      else
      {
	sprintf(buf2, "The dealer gives you a %s.\r\n", getCardName(card));
	send_to_char(buf2, ch);
      }
      casino_blackjack.cards[playernum][cardcounter] = card;	
      break;
    }
    cardcounter++;
  }
  // Update the player's score
  setBJScore(player, playernum);
  // Check if the player has gone bust with the new card
  if (player->type > 21)
  {
    endBJGame(player, playernum);
    return;
  }
  // Check if we've reached the last card
  if (cardcounter >= 4)
  {
    if (player->type < 22)		// Autowin, 5 cards, and legal
    {
      send_to_char("You have 5 cards, and are still under the limit.\r\n",player->ch);
      sprintf(buf2, "You get 1.5 times your money, winning &Y%d&n coins.\r\n",
	      (player->amount + (player->amount >> 1)));
      send_to_char(buf2, player->ch);
      // Artus> BTH
      // GET_BANK_GOLD(player->ch) += (player->amount * 5) / 2;
      GET_GOLD(player->ch) += (player->amount << 1) + (player->amount >> 1);
      removeBJPlayer(playernum);
      return;
    }	
    endBJGame(player, playernum);
  }
  // Artus> Moved below five card check.
  // Automatically shift control on reaching 21
  if (player->type == 21)
  {
    send_to_char("21, passing control to dealer.\r\n", ch);
    endBJGame(player, playernum);
    return;
  }
}

/* Handles the split command for blackjack */
void handle_split(int counter) 
{
  int newcounter = 0, splitcount = 0;
  struct game_item *bjitem = &casino_blackjack.list[counter], *bjitemnew;

  // See if we can split
  if (!canSplit(counter))
  {
    send_to_char("You can't split this game.\r\n", bjitem->ch);
    return;
  }
  // Check that there is a game available to split into
  while (newcounter < MAX_GAME_BETS)
  {
    bjitemnew = &casino_blackjack.list[newcounter];
    if(bjitemnew->ch == 0)
      break;			// Found a spot
    newcounter++;
  }
  if (newcounter >= MAX_GAME_BETS)
  {
    send_to_char("Can't split, house is at maximum allowable games.\r\n", 
	         bjitem->ch);
    return;
  }
  // Check that player can afford to split
  /* Artus> BTH
  if (GET_BANK_GOLD(bjitem->ch) < bjitem->amount) */
  if (GET_GOLD(bjitem->ch) < bjitem->amount)
  {
    send_to_char("You don't have enough cash to split this game.\r\n", 
	         bjitem->ch);
    return;
  }
  // Restrict splitting to one split / game
  // Firstly, see if player has the BJ_HAS_SPLIT flag
  while ((splitcount = ((casino_blackjack.cards[counter][5] - splitcount) % 2)) != 0)
    if (splitcount == BJ_HAS_SPLIT)
    {
      send_to_char("you have already split this game.\r\n", bjitem->ch);
      return;
    }
  // Set the split flag, for this game, and the new
  casino_blackjack.cards[counter][5] += BJ_HAS_SPLIT;	
  casino_blackjack.cards[ newcounter ][5] += BJ_HAS_SPLIT;
  // Do the split
  send_to_char("Splitting game. Next game will follow this one. You are passed a new card.\r\n", bjitem->ch);
  // Create the newgame -- bypasses double play protection
  bjitemnew = &casino_blackjack.list[ newcounter ];
  bjitemnew->ch = bjitem->ch;
  bjitemnew->amount = bjitem->amount;
  // Halve original score, and set the new one
  bjitem->type /= 2;
  bjitemnew->type = bjitem->type;
  // Copy the actual second card from the old game to the new
  casino_blackjack.cards[newcounter][0] = casino_blackjack.cards[counter][1];
  // Get a new card for the original game
  casino_blackjack.cards[counter][1] = getCard();
  // Get a new card for the NEW game, and the dealer of the new game gets the 
  // original dealer
  casino_blackjack.cards[ newcounter ][1] = getCard();
  casino_blackjack.cards[newcounter + MAX_GAME_BETS][0] = casino_blackjack.cards[counter + MAX_GAME_BETS][0];
  sprintf(buf2, "Cards - %d : %d ", casino_blackjack.cards[counter][1], 
          casino_blackjack.cards[newcounter][1]);
  send_to_char(buf2, bjitem->ch);
  // Update both hands
  setBJScore(bjitem, counter);
  setBJScore(bjitemnew, newcounter);
  // Add the new player
  casino_blackjack.numplayers++;
  // Show the player his new hand
  showBJHand(bjitem, counter);
  // Check for blackjack on original game
  if(hasBlackjack(counter))
  {
    sprintf(buf2, "BLACKJACK! Way to go. You win &Y%d&n coins!!\r\n", 
	    bjitem->amount * 3);
    send_to_char(buf2, bjitem->ch);
    // Artus> BTH
    // GET_BANK_GOLD(bjitem->ch) += bjitem->amount * 3;
    GET_GOLD(bjitem->ch) += bjitem->amount * 3;
    send_to_char("Next game.\r\n", bjitem->ch);
    showBJHand(bjitemnew, newcounter);
    removeBJPlayer(counter);
  }
  if (hasBlackjack(newcounter))
  {
    sprintf(buf2, "BLACKJACK! Way to go. You win &Y%d&n coins!!\r\n", 
	    bjitemnew->amount * 3);
    send_to_char(buf2, bjitemnew->ch);
    // Artus> BTH
    // GET_BANK_GOLD(bjitemnew->ch) += bjitemnew->amount * 3;
    GET_GOLD(bjitemnew->ch) += bjitemnew->amount * 3;
    removeBJPlayer(newcounter);
  }
}

/* Returns character's data, given char, and game */
struct game_item *getGamePlayer(struct char_data *ch, char game) 
{
  int counter  = 0;
  struct game_item *player;

  if (game == 'B')
    while (counter < MAX_GAME_BETS)
    {
      player = &casino_blackjack.list[counter];
      if ((player->ch != 0) && (player->ch == ch))
	return player;
      counter++;
    }

  if (game == 'R')
    while (counter < MAX_GAME_BETS)
    {
      player = &casino_race.list[counter];
      if ((player->ch != 0) && (player->ch == ch))
	return player;
      counter++;
    }

  return 0;
}

/* Function to determine a hand's score. Complicated by ACE issues in BJ */
void setBJScore(struct game_item *bjitem, int playernum) 
{
  int counter = 0, innercounter = 0, total = 0, card, acecount = 0;

  // Look through hand, for ACEs
  while (counter < 5)
  {
    card = casino_blackjack.cards[playernum][counter];
    if (card == CARD_ACE)
      acecount++;
    counter++;
  }
	
  // Reset counter
  counter = 0;

  // If there are aces, scoring is handled differently
  if (acecount)
  {
    do
    { // While aces
      counter = 0;
      total = 0;
      // Go through every card, totalling values
      while (counter < 5)
      {
	card = casino_blackjack.cards[playernum][counter];
	switch(card)
	{
	  case CARD_CONV_ACE:
	    total += 1;
	    break;
	  case CARD_ACE:
	    total += 11;
	    break;
	  case CARD_KING:
	  case CARD_QUEEN:
	  case CARD_JACK:
	  case CARD_TEN:
	    total += CARD_TEN;
	    break;
	  case CARD_NINE:
	  case CARD_EIGHT:
	  case CARD_SEVEN:
	  case CARD_SIX:
	  case CARD_FIVE:
	  case CARD_FOUR:
	  case CARD_THREE:
	  case CARD_TWO:
	  case CARD_NONE:
	    total += card;
	    break;
	  default: 
	    basic_mud_log("Gambling : Card Error : Not a valid card");
	    break;
	} // Switch (card)
	counter++;
      }
      if (total <= 21)
	break;		// The score is fine, break out
      innercounter = 5;	// Set counter to last possible card
			// Score needs reduction, convert last ace to a one
      while (innercounter > 0)
      {
	card = casino_blackjack.cards[playernum][innercounter - 1];
	if (card == CARD_ACE)	// Found ace 
	{
	  // Change it to a converted ace
	  casino_blackjack.cards[playernum][innercounter -1] = CARD_CONV_ACE;
	  acecount--;	// Reduce ace count
	  break;
	}
	innercounter--;
      } // while not at ace
    } while (acecount + 1); // While aces exist
  } /* If aces in hand */ else {
    while (counter < 5)
    {
      card = casino_blackjack.cards[playernum][counter]; 
      if (card != CARD_CONV_ACE)// If card is not an ace that is now a 1
      {
	if ((card == CARD_KING) || (card == CARD_QUEEN) || (card == CARD_JACK))
	  total += CARD_TEN;	// Valued at ten points
	else if (card == CARD_ACE)
	  total += 11;		// Ace value
	else
	  total += card;	        // Add score
      } else	
	total += CARD_CONV_ACE;      	// Ace now counts as one
      counter++;
    }
  } // No aces
  bjitem->type = total;
}

/* Shoes a player their hand */
void showBJHand(struct game_item *bjitem, int playernum) 
{
  int cardcounter = 0, card = 0;
  sprintf(buf1, "Your hand:\r\n----------\r\n");
  while (cardcounter < 5)
  {
    card = casino_blackjack.cards[playernum][cardcounter];
    if (card)
    {
      sprintf(buf2, "- %s\r\n", getCardName(card));
      strcat(buf1, buf2);
    }			
    cardcounter++;			
  }
  sprintf(buf2, "\r\nCurrent hand score is %d.\r\n", bjitem->type);
  strcat(buf1, buf2);
  sprintf(buf2, "The dealer's card is: %s \r\n", 
	  getCardName(casino_blackjack.cards[playernum+MAX_GAME_BETS][0]));
  strcat(buf1, buf2);
  send_to_char(buf1, bjitem->ch);	
  return;
}

/* Returns letter name of a card */
// Artus> Any reason this isn't a const?...
char *getCardName(int card) 
{
  switch(card)
  {
    case CARD_CONV_ACE: return "Ace(1)";
    case CARD_ACE: 	return "Ace";
    case CARD_KING: 	return "King";
    case CARD_QUEEN: 	return "Queen"; 
    case CARD_JACK: 	return "Jack"; 
    case CARD_TEN: 	return "10"; 
    case CARD_NINE: 	return "9"; 
    case CARD_EIGHT: 	return "8"; 
    case CARD_SEVEN: 	return "7"; 
    case CARD_SIX: 	return "6";
    case CARD_FIVE: 	return "5";
    case CARD_FOUR: 	return "4"; 
    case CARD_THREE: 	return "3"; 
    case CARD_TWO: 	return "2"; 
    default: 
      basic_mud_log("Gambling Error : Blackjack : card out of bounds"); 
      break;
  }
  return 0;
}

/* Handles the end of a game, once control has passed to the dealer */
void endBJGame(struct game_item *bjitem, int playernum) 
{
  struct game_item dealer;
  int notwon = 1, dealernum = playernum + MAX_GAME_BETS, card1, card2;
  int counter = 2;		// Counts dealer's cards

  // SEe if player has gone bust
  if (bjitem->type > 21)
  {
    send_to_char("Bust! Better luck next time.\r\n", bjitem->ch);
    removeBJPlayer(playernum);
    return;
  }	
  // Set dealer's score to the card we gave them
  dealer.type = casino_blackjack.cards[playernum][5];
  // Player is not bust, play dealers hand
  casino_blackjack.cards[dealernum][1] = getCard();	// 2nd card
  sprintf(buf1, "\r\nDealer draws: %s\r\n", 
          getCardName(casino_blackjack.cards[dealernum][1]));
  send_to_char(buf1, bjitem->ch);
  setBJScore(&dealer,dealernum);
  card1 = casino_blackjack.cards[dealernum][0];
  card2 = casino_blackjack.cards[dealernum][1];
  // See if dealer has BJ
  if (hasBlackjack(dealernum))
  {
    send_to_char("The dealer got BLACKJACK! Bad luck.\r\n", bjitem->ch);
    removeBJPlayer(playernum);
    return;
  }
  setBJScore(&dealer, dealernum);
  while (notwon)
  { // Loop while game is undecided
    // Check if dealer is bust 
    if (dealer.type > 21)
    {
      sprintf(buf1, "Dealer goes bust. You win &Y%d&n coins!!\r\n", 
	      bjitem->amount * 2); 
      send_to_char(buf1, bjitem->ch);
      // Artus> BTH
      // GET_BANK_GOLD(bjitem->ch) += bjitem->amount * 2;
      GET_GOLD(bjitem->ch) += bjitem->amount << 1;
      removeBJPlayer(playernum);
      return;
    }
    // Check if dealer is within threshold
    if (dealer.type >= 17 && dealer.type <= 21)
    {
      sprintf(buf1, "Dealer stays on %d.\r\n", dealer.type);
      send_to_char(buf1, bjitem->ch);
      notwon = 0;		
    }
    // Check if dealer requires another card
    if (dealer.type < 17)
    {
      casino_blackjack.cards[dealernum][counter] = getCard();
      sprintf(buf1, "Dealer draws: %s\r\n",
	      getCardName(casino_blackjack.cards[dealernum][counter]));
      send_to_char(buf1, bjitem->ch);
      counter++; 			
    }
    // Check that dealer doens't have max cards
    setBJScore(&dealer,dealernum);
    if (counter >= 5 && dealer.type < 22)
    {
      send_to_char("Dealer has 5 cards, and is still valid. You lose.\r\n",
		   bjitem->ch);
      removeBJPlayer(playernum);
      return;
    }
  } // While(notwon)
  // Settle winner
  if (bjitem->type == dealer.type)
  {
    send_to_char("Push! You got your money back.\r\n", bjitem->ch);
    // Artus> BTH
    // GET_BANK_GOLD( bjitem->ch ) += bjitem->amount;
    GET_GOLD(bjitem->ch) += bjitem->amount;
  } else if (bjitem->type < dealer.type) {
    send_to_char("Dealer wins. Better luck next time.\r\n", bjitem->ch);
  } else {
    sprintf(buf1, "Congratulations, you won &Y%d&n coins!\r\n", 
	    bjitem->amount << 1);
    // Artus> BTH
    // GET_BANK_GOLD(bjitem->ch) += bjitem->amount * 2;
    GET_GOLD(bjitem->ch) += bjitem->amount << 1;
    send_to_char(buf1, bjitem->ch);
  }
  removeBJPlayer(playernum);
}

/* Invalidates position in BlackJack game */
void removeBJPlayer(int playernum) 
{
  int counter = 0, dealernum = playernum + MAX_GAME_BETS;		
  struct game_item *bjitem = &casino_blackjack.list[playernum];
  while (counter < 6)
  {
    casino_blackjack.cards[playernum][counter] = 0;
    casino_blackjack.cards[dealernum][counter] = 0;
    counter++;
  }
  bjitem->ch = 0;
  bjitem->amount = 0;
  bjitem->type = 0;
  casino_blackjack.numplayers--;	
  return;
}

/* Given a player's number, checks if they can split into
   multiple games */
int canSplit(int playernum) 
{
  int card1 = casino_blackjack.cards[ playernum ][0],
      card2 = casino_blackjack.cards[ playernum ][1],
      card3 = casino_blackjack.cards[ playernum ][2];
  if (card3 != 0)
    return 0;	// Tsk, can't split with 3 cards		
  if (((card1 >= CARD_TEN  && card1 <= CARD_KING) &&
       (card2 >= CARD_TEN  && card2 <= CARD_KING)) ||
      (card1 <= CARD_NINE && card1 == card2))
    return 1;	// Eligible
  return 0;	// No can do
}

/* Given a player's number in the game, checks their cards
   to see if they have blackjack */
int hasBlackjack(int playernum) 
{
  int card1 = casino_blackjack.cards[ playernum ][0],
      card2 = casino_blackjack.cards[ playernum ][1],
      card3 = casino_blackjack.cards[ playernum ][2];
  if (card3 != 0)
    return 0;	// Can't have blackjack with 3 cards		
  if ((card1 == CARD_ACE && (card2 >= CARD_TEN && card2 <= CARD_KING)) ||
      (card2 == CARD_ACE && (card1 >= CARD_TEN && card1 <= CARD_KING)))
    return 1;	// Blackjack!
  return 0;	// No blackjack
}

/* Function to see if a player is in a game (returns 1 higher than the player, if found)*/
int isInGame(struct char_data *ch, char game) 
{
  int counter = 0;
  struct game_item *gameitem;
  // Checking the races
  if (game == 'R')
    while (counter < casino_race.numbets)
    {
      gameitem = &casino_race.list[counter];
      if ((gameitem->ch != 0) && (ch == gameitem->ch)) 	// If this is the one
	return counter+1;		// Indicate found player
      counter++;			// Next player
    } 
  // Checking BlackJack
  if (game == 'B')
    while (counter < MAX_GAME_BETS)
    {
      gameitem = &casino_blackjack.list[counter];
      if ((gameitem->ch != 0) && (ch == gameitem->ch))
	  return counter+1;
      counter++;
    } 
  // Not a valid game, or player not found
  return 0;
}

/* Just randomly give a card. Theoretically possible to get 5+ of a kind.
   Improve if it becomes an issue or problem */
int getCard() 
{
  return number(CARD_TWO, CARD_ACE);
}

/**** SLOTS ****/

void gamble_slots(struct char_data *ch, char *arg) 
{
  int amount = 0;

  one_argument(arg, buf1);
  // Check that player has given us something to do
  if (!*buf1)
  {
    send_to_char("Type 'slots <amount>' to play.\r\n", ch);
    return;
  }
  // What is it with these players that want to stuff items everywhere
  if (!isdigit(*buf1) && strcmp(buf1, "max") != 0)
  {
    send_to_char("You hopelessly try to play slots without money.\r\n", ch);
    return;
  }
  // Get gambling amount
  if (strcmp(buf1, "max") == 0)
    amount = GET_LEVEL(ch) * 1000;
  else
    amount = atoi(buf1);
  // Enforce casino limits
  if (amount < 100)
  {
    send_to_char("House minimum is &Y100&n coins.\r\n", ch);
    return;
  }
  if (amount > GET_LEVEL(ch) * 1000)
  {
    sprintf(buf2, "Your maximum allowable wager is &Y%d&n coins.\r\n", 
	    GET_LEVEL(ch) *1000 );
    send_to_char(buf2, ch);
    return;
  }
  // Check person's gold
  if (GET_GOLD(ch) < amount)
  {
    send_to_char("You don't have enough cash on you for that wager.\r\n", ch);
    return; 
  }	
  send_to_char("You put the money into the machine, and watch the reels spin.\r\n", ch);
  act("$n puts some money into a slot machine.", FALSE, ch, 0, 0, TO_ROOM);
  GET_GOLD(ch) -= amount;			
  // Fine. Lets gamble.
  play_slots(ch, amount);
}

void play_slots(struct char_data *ch, int amount) 
{
  int reel[5]; 		// 5 reels 
  int counter = 0;

  while (counter < 5)
  {
    // System to favour lower numbers
    reel[counter] = getCard();
    if (reel[counter] >= CARD_FIVE)
    {
      reel[counter] = getCard();
      if (reel[counter] >= CARD_TEN)
	reel[counter] = getCard();
    }
    counter++;	
  }
  // Artus> Maybe have some fruits, in stead of cards.
  sprintf(buf2,"The reels slowly stop..\r\n %-5s %-5s %-5s %-5s %-5s\r\n", 
	  getCardName(reel[0]), getCardName(reel[1]), getCardName(reel[2]), 
	  getCardName(reel[3]), getCardName(reel[4]));
  send_to_char(buf2, ch);
  calcSlotResult(ch, reel, amount);
}

void calcSlotResult(struct char_data *ch, int reel[5], int amount)
{
  int counter = 0, cardcounter = 0;
  double payout = 0;
  // check if all reels are the same
  if ((reel[0] == reel[1]) && (reel[1] == reel[2]) && (reel[2] == reel[3]) &&
      (reel[3] == reel[4]))
  {
    send_to_char("Five of a kind! You're a winner!\r\n", ch);
    if (reel[0] == CARD_ACE)
    {	// JACKPOT!
      send_to_char("ACES! You won the JACKPOT!!!\r\n", ch);
      payout = amount * 50 * CARD_ACE;
      // Artus> BTH
      // GET_BANK_GOLD(ch) += amount * 50 * CARD_ACE;
      GET_GOLD(ch) += (int)(payout * 2);
      sprintf(buf2, "Your bank has been credited for &Y%d&n coins.\r\n", 
	      (int)(payout));
      act("$n just won the JACKPOT!!\r\n", FALSE, ch, 0, 0, TO_ROOM);
      return;
    }
    payout = (amount * 15) * reel[0];
    sprintf(buf2, "You win &Y%d&n coins.\r\n", (int)payout);
    send_to_char(buf2, ch);
    GET_GOLD(ch) += (int)(payout * 2);
    return;	
  }
  counter = 1;
  // See if they got four of a kind.
  while (counter <= CARD_ACE)
  { 		// loop through every card
    if (reel[0] == counter) 
      cardcounter++;
    if (reel[1] == counter) 
      cardcounter++;
    if (reel[2] == counter)
      cardcounter++;
    if (reel[3] == counter)
      cardcounter++;
    if (reel[4] == counter)
      cardcounter++;
    if (cardcounter == 4)
    {
      sprintf(buf2, "You got 4 %s's!!\r\n", getCardName(counter));
      send_to_char(buf2,ch);
      payout = amount * 5 * counter / 2;
      sprintf(buf2, "You win &Y%d&n coins.\r\n", (int)(payout));

      send_to_char(buf2, ch);
      GET_GOLD(ch) += (int)(payout * 2);
      return;
    }
    cardcounter = 0;
    counter++;
  }
  counter = 1;
  // See if they got three of a kind
  while (counter <= CARD_ACE)
  {
    if (reel[0] == counter)
      cardcounter++;
    if (reel[1] == counter)
      cardcounter++;
    if (reel[2] == counter)
      cardcounter++;
    if (reel[3] == counter)
      cardcounter++;
    if (reel[4] == counter)
      cardcounter++;
    if (cardcounter == 3)
    {
      payout = (amount * counter);
      sprintf(buf2, "You got 3 %s's!\r\nYou win &Y%d&n coins.\r\n",
	      getCardName(counter), (int)(payout)); 
      send_to_char(buf2, ch);
      GET_GOLD(ch) += (int)(payout*2);
      return;
    }
    cardcounter = 0;
    counter++;
  }
  // See if they got a straight -- win free spins
  if ((reel[0] <= CARD_TEN) &&
      ((reel[1] == (reel[0] + 1)) && (reel[2] == (reel[1] + 1)) &&
       (reel[3] == (reel[2] + 1)) && (reel[4] == (reel[3] + 1))))
  {
    send_to_char("You got a straight! You win 5 free spins.\r\n", ch);
    counter = 0;
    while (counter < 5)
    {
      sprintf(buf2, "Free spin #%d. Good luck.\r\n", counter);
      send_to_char(buf2, ch); 
      // Clear the reel
      reel[0] = reel[1] = reel[2] = reel[3] = reel[4] = 0;
      play_slots(ch, amount);
      counter++;
    }
    send_to_char("Free spins over!\r\n", ch);
    return;			
  }
  send_to_char("Nothing. Try again.\r\n", ch);
}

/* ROULETTE */

