/*        
MODULE_START
================================================================================
FILE
		
MODULE
	
DESCRIPTION

NOTES

================================================================================
MODULE_END
*/          

#include <stdio.h>
#include <stdarg.h>
#include <fcntl.h>
#include <assert.h>
#include <stdlib.h>  
#include <io.h>          
#include <string.h>
#include <time.h>

#include "debug.h"
    
#define MAX_DEBUG_BUF 256
#define MAX_DATE_LEN  25
#define PATH_SEPARATOR '\\'
                       
static int DebugInitialised = 0;                          
FILE *debug_fd;

/*        
FUNC_START
================================================================================
NAME
	
DESCRIPTION
    
ARGUMENTS

SIDE EFFECTS

RETURNS

LIMITS

================================================================================
FUNC_END
*/     
void TimeStamp(FILE *fd)
{

	time_t t;           
	char date_buf[MAX_DATE_LEN];
	char *date_str = date_buf;

  	t = time(NULL);      
  	strcpy(date_buf, ctime(&t));
  	/*
  	** strip out the '\n'
  	*/                       
  	while (*date_str != '\n')
  		date_str++;
  	*date_str = '\0';
   
	fprintf(fd, "%s:", date_buf);
}
    
    /*        
FUNC_START
================================================================================
NAME
	
DESCRIPTION
    
ARGUMENTS

SIDE EFFECTS

RETURNS

LIMITS

================================================================================
FUNC_END
*/                            
void DebugInit(char *fname)         
{                 
	char buf[MAX_DEBUG_BUF];
	
	if (!DebugInitialised)
	{
		if ((debug_fd = fopen(fname, "w")) != NULL)
		{                                     
			TimeStamp(debug_fd);
			fprintf(debug_fd, "Debug file %s initialised\n\r", fname);
			DebugInitialised = 1;
		}
	}
	else
	{
		fprintf(debug_fd, buf, "Debug file already initialised\n\r");
	}
}

void DebugClose()
{
	if (DebugInitialised)
	{
		fclose(debug_fd);
	}
}	
    
/*        
FUNC_START
================================================================================
NAME
	
DESCRIPTION
    
ARGUMENTS

SIDE EFFECTS

RETURNS

LIMITS

================================================================================
FUNC_END
*/     
void WriteDebug(char * SourceFile, long SourceLine, char *fmt, ...)
{                         
	va_list args;
	char *fname_ptr = SourceFile;                 

	if (!DebugInitialised)
		return;
	else
	{                   
		TimeStamp(debug_fd);         
		/*
		** strip the path from the file name            
		*/                              
		fname_ptr = SourceFile + strlen(SourceFile);
		while(*fname_ptr != PATH_SEPARATOR)
			fname_ptr--; 
		fname_ptr++;
		
		fprintf(debug_fd, "%s : %d ", fname_ptr, SourceLine);
		va_start(args, fmt);
		vfprintf(debug_fd, fmt, args);
		fprintf(debug_fd, "\n\r");
	}
}