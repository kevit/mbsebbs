/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Terminal output routines.
 *
 *****************************************************************************
 * Copyright (C) 1997-2004
 *   
 * Michiel Broek		FIDO:	2:280/2802
 * Beekmansbos 10
 * 1971 BV IJmuiden
 * the Netherlands
 *
 * This file is part of MBSE BBS.
 *
 * This BBS is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 *
 * MBSE BBS is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with MBSE BBS; see the file COPYING.  If not, write to the Free
 * Software Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 *****************************************************************************/


#include "../config.h"
#include "mbselib.h"
#include "users.h"


int termmode;			/* 0 = tty, 1 = ANSI			   */
int termx = 80;
int termy = 24;


void TermInit(int mode, int x, int y)
{
    termmode = mode;
    termx = x;
    termy = y;
}



/*
 * Function will print about of enters specified
 */
void Enter(int num)
{
    int i;

    for (i = 0; i < num; i++)
	fprintf(stdout, "\n");
    fflush(stdout);
}




void pout(int fg, int bg, char *Str)
{
    colour(fg, bg);
    fprintf(stdout, Str);
    fflush(stdout);
}



void poutCenter(int fg, int bg, char *Str)
{
    colour(fg, bg);
    Center(Str);
}



void poutCR(int fg, int bg, char *Str)
{
    colour(fg, bg);
    fputs(Str, stdout);
    fprintf(stdout, "\n");
    fflush(stdout);
}



/*
 * Changes ansi background and foreground color
 */
void colour(int fg, int bg)
{
    if (termmode == 1) {
  
	int att=0, fore=37, back=40;

	if (fg<0 || fg>31 || bg<0 || bg>7) {
	    fprintf(stdout, "ANSI: Illegal colour specified: %i, %i\n", fg, bg);
	    fflush(stdout);
	    return; 
	}

	fprintf(stdout, "[");
	if ( fg > WHITE) {
	    fprintf(stdout, "5;");
	    fg-= 16;
	}
	if (fg > LIGHTGRAY) {
	    att=1;
	    fg=fg-8;
	}

	if      (fg == BLACK)   fore=30;
	else if (fg == BLUE)    fore=34;
	else if (fg == GREEN)   fore=32;
	else if (fg == CYAN)    fore=36;
	else if (fg == RED)     fore=31;
	else if (fg == MAGENTA) fore=35;
	else if (fg == BROWN)   fore=33;
	else                    fore=37;

	if      (bg == BLUE)      back=44;
	else if (bg == GREEN)     back=42;
	else if (bg == CYAN)      back=46;
	else if (bg == RED)       back=41;
	else if (bg == MAGENTA)   back=45;
	else if (bg == BROWN)     back=43;
	else if (bg == LIGHTGRAY) back=47;
	else                      back=40;
		
	fprintf(stdout, "%d;%d;%dm", att, fore, back);
	fflush(stdout);
    }
}



void Center(char *string)
{
    int	    Strlen;
    int	    Maxlen = termx;
    int	    i, x, z;
    char    *Str;

    Str = calloc(1024, sizeof(char));
    Strlen = strlen(string);

    if (Strlen == Maxlen)
	fprintf(stdout, "%s\n", string);
    else {
	x = Maxlen - Strlen;
	z = x / 2;
	for (i = 0; i < z; i++)
	    strcat(Str, " ");
	strcat(Str, string);
	fprintf(stdout, "%s\n", Str);
    }

    fflush(stdout);
    free(Str);
}



void clear()
{
    if (termmode == 1) {
	colour(LIGHTGRAY, BLACK);
	fprintf(stdout, ANSI_HOME);
	fprintf(stdout, ANSI_CLEAR);
	fflush(stdout);
    } else
	Enter(1); 
}



/*
 * Moves cursor to specified position
 */
void locate(int y, int x)
{
    if (termmode > 0) {
	if (y > termy || x > termx) {
	    fprintf(stdout, "ANSI: Invalid screen coordinates: %i, %i\n", y, x);
	    fflush(stdout);
	    return; 
	}
	fprintf(stdout, "\x1B[%i;%iH", y, x);
	fflush(stdout);
    }
}



void fLine(int	Len)
{
    int	x;

    if (termmode == 0)
	for (x = 0; x < Len; x++)
	    fprintf(stdout, "-");

    if (termmode == 1)
	for (x = 0; x < Len; x++)
	    fprintf(stdout, "%c", 196);

    fprintf(stdout, " \n");
    fflush(stdout);
}




void  sLine()
{
    fLine(termx -1);
}



/*
 * curses compatible functions
 */
void mvprintw(int y, int x, const char *format, ...)
{
    char	*outputstr;
    va_list	va_ptr;

    outputstr = calloc(2048, sizeof(char));

    va_start(va_ptr, format);
    vsprintf(outputstr, format, va_ptr);
    va_end(va_ptr);

    locate(y, x);
    fprintf(stdout, outputstr);
    free(outputstr);
    fflush(stdout);
}



