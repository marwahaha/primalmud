/* ************************************************************************
*  file: listrent.c					Part of CircleMUD *
*  Usage: list player rent files                                          *
*  Written by Jeremy Elson                                                *
*  All Rights Reserved                                                    *
*  Copyright (C) 1993 The Trustees of The Johns Hopkins University        *
************************************************************************* */

#include <stdio.h>
#include "../structs.h"

struct obj_file_elem_new {
   obj_num item_number;

   int  value[4];
   long  extra_flags;
   int  weight;
   int  timer;
   long bitvector;
   struct obj_affected_type affected[MAX_OBJ_AFFECT];
};

void	main(int argc, char **argv) 
{
   int	x;

   for (x = 1; x < argc; x++)
      Crash_listrent(argv[x]);
}


int	Crash_listrent(char *fname) 
{
   FILE * fl, *flout;
   char	buf[MAX_STRING_LENGTH];
   struct obj_file_elem object;
   struct obj_file_elem_new newobject;
   struct obj_data *obj;
   struct rent_info rent;
   int i;

   if (!(fl = fopen(fname, "rb"))) {
      sprintf(buf, "%s has no rent file.\n\r", fname);
      printf("%s", buf);
      return;
   }
   sprintf(buf,"tmp/%s",fname);
   if (!(flout = fopen(buf, "w"))) {
     sprintf(buf,"Could not create new object file.\n\r");
     printf("%s",buf);
     return;
   }
   sprintf(buf, "%s\n\r", fname);
   if (!feof(fl))
      fread(&rent, sizeof(struct rent_info ), 1, fl);
   fwrite( &rent, sizeof(struct rent_info), 1, flout);
   switch (rent.rentcode) {
   case RENT_RENTED: 
      strcat(buf, "Rent\n\r");     
      break;
   case RENT_CRASH:  
      strcat(buf, "Crash\n\r");    
      break;
   case RENT_CRYO:   
      strcat(buf, "Cryo\n\r");     
      break;
   case RENT_TIMEDOUT:
   case RENT_FORCED: 
      strcat(buf, "TimedOut\n\r"); 
      break;
   default:          
      strcat(buf, "Undef\n\r");    
      break;
   }
   while (!feof(fl)) {
      fread(&object, sizeof(struct obj_file_elem ), 1, fl);
      if (ferror(fl)) {
	 fclose(fl);
	 return;
      }
      newobject.item_number = object.item_number; 
      for (i=0;i<4;i++) 
        newobject.value[i] = object.value[i];
      newobject.extra_flags=object.extra_flags;
      newobject.weight=object.weight;
      newobject.timer=object.timer;
      for (i=0;i<MAX_OBJ_AFFECT;i++)
      {
        newobject.affected[i].location=object.affected[i].location;
        newobject.affected[i].modifier=object.affected[i].modifier;
      } 
      if (!feof(fl))
       {
         fwrite(&newobject, sizeof(struct obj_file_elem_new), 1, flout);
	 sprintf(buf, "%s[%5d] %s\n", buf, object.item_number, fname);
      }
   }
   printf("%s", buf);
   fclose(fl);
   fclose (flout);
}


