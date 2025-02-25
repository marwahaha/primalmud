#define MAX_HOUSES	500
#define MAX_GUESTS	4

#define HOUSE_PRIVATE	0


struct house_control_rec {
   sh_int vnum;			/* vnum of this house		*/
   sh_int atrium;		/* vnum of house's atrium	*/
   long built_on;		/* date this house was built	*/
   int mode;			/* mode of ownership		*/
   long owner;			/* idnum of house's owner	*/
   int num_of_guests;		/* how many guests for house	*/
   long guests[MAX_GUESTS];	/* idnums of house's guests	*/
   long last_payment;		/* date of last house payment   */
   long spare0;
   long spare1;
   long spare2;
   long spare3;
   long spare4;
   long spare5;
   long spare6;
   long spare7;
};


struct house_control_rec_old {
   sh_int vnum;			/* vnum of this house		*/
   sh_int atrium;		/* vnum of house's atrium	*/
   long built_on;		/* date this house was built	*/
   int mode;			/* mode of ownership		*/
   long owner;			/* idnum of house's owner	*/
   int num_of_guests;		/* how many guests for house	*/
   long guests[10];	/* idnums of house's guests	*/
   long last_payment;		/* date of last house payment   */
   long spare0;
   long spare1;
   long spare2;
   long spare3;
   long spare4;
   long spare5;
   long spare6;
   long spare7;
};

   
#define TOROOM(room, dir) (world[room].dir_option[dir] ? \
			    world[room].dir_option[dir]->to_room : NOWHERE)

#define MAX_HOUSE_CONTENT 50

void	House_listrent(struct char_data *ch, int vnum);
void	House_boot(void);
void	House_save_all(void);
int	House_can_enter(struct char_data *ch, sh_int house);
int	House_crashsave(int vnum,int saveall);
