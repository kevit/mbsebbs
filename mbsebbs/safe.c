/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Safe Door
 *
 *****************************************************************************
 * Copyright (C) 1997-2002
 *   
 * Michiel Broek		FIDO:		2:280/2802
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
 * Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *****************************************************************************/

#include "../config.h"
#include "../lib/libs.h"
#include "../lib/memwatch.h"
#include "../lib/mbse.h"
#include "../lib/structs.h"
#include "../lib/users.h"
#include "../lib/records.h"
#include "../lib/clcomm.h"
#include "../lib/common.h"
#include "whoson.h"
#include "dispfile.h"
#include "input.h"
#include "misc.h"
#include "safe.h"
#include "timeout.h"
#include "language.h"



FILE *pSafe;

int iLoop, iFirst, iSecond, iThird;
char sFirst[4], sSecond[4], sThird[4];
int cracked;
int tries;

int  getdigits(void);
int  SafeCheckUser(int);



void Safe(void)
{
	long	isize;
	int	i;

	isize = sizeof(int);
	srand(Time_Now);
	cracked = FALSE;
	tries = 0;

	WhosDoingWhat(SAFE);

	Syslog('+', "User starts Safe Cracker Door");

	Enter(1);
	/* Safe Cracker Door */
	pout(WHITE, BLACK, (char *) Language(86));
	Enter(1);

	clear();

	DisplayFile(CFG.sSafeWelcome);

	if (SafeCheckUser(TRUE) == TRUE)
		return;

	/* In the safe lies */
	pout(WHITE, BLACK, (char *) Language(87));

	fflush(stdout);
	alarm_on();
	Getone();

	clear();

	Enter(2);
	pout(LIGHTGREEN, BLACK, (char *) Language(88));
	Enter(2);

	colour(LIGHTMAGENTA, BLACK);
	printf("%s", CFG.sSafePrize);

	Enter(2);
	/* Do you want to open the safe ? [Y/n]: */
	pout(WHITE, BLACK, (char *) Language(102));

	fflush(stdout);
	alarm_on();
	i = toupper(Getone());

	if (i == Keystroke(102, 1)) {
		Syslog('+', "User exited Safe Cracker Door");
		return;
	}

	/*
	 * Loop until the safe is opened, maximum trys
	 * exceeded or the user is tired of this door.
	 */
	while (TRUE) {
		/* Get digits, TRUE if safe cracked. */
		if (getdigits() == TRUE) {
		    SafeCheckUser(FALSE);
		    break;
		}

		Enter(1);
		/* Do you want to try again ? [Y/n]: */
		pout(LIGHTRED, BLACK, (char *) Language(101));
		fflush(stdout);

		alarm_on();
		i = toupper(Getone());
		if (i == Keystroke(101, 1)) {
		    SafeCheckUser(FALSE);
		    break;
		}

		if (SafeCheckUser(FALSE) == TRUE)
			break;
	}
	Syslog('+', "User exited Safe Cracker Door");
}



/*
 * Ask use for digits, returns TRUE if the safe is cracked.
 */
int getdigits(void)
{
    int	    i;
    char    temp[81];

    colour(WHITE, BLACK);
    /* Please enter three numbers consisting from 1 to */
    printf("\n\n%s%d\n", (char *) Language(89), CFG.iSafeMaxNumber);
    /* Please enter three combinations. */
    printf("%s", (char *) Language(90));

    while (TRUE) {
	Enter(2);
	/* 1st Digit */
	pout(LIGHTRED, BLACK, (char *) Language(91));
	colour(LIGHTBLUE, BLACK);
	fflush(stdout);
	Getnum(sFirst, 2);
	sprintf(temp, "1st: %s", sFirst);
	if((strcmp(sFirst, "")) != 0) {
	    iFirst=atoi(sFirst);
	}

	if((iFirst > CFG.iSafeMaxNumber) || (iFirst <= 0) || (strcmp(sFirst, "") == 0)) {
	    colour(WHITE, BLUE);
	    /* Please try again! You must input a number greater than Zero and less than */
	    printf("\n%s%d.", (char *) Language(92), CFG.iSafeMaxNumber);
	} else 
	    break;
    }

    while (TRUE) {
	Enter(1);
	/* 2nd digit: */
	pout(LIGHTRED, BLACK, (char *) Language(93));
	colour(LIGHTBLUE, BLACK);
	fflush(stdout);
	Getnum(sSecond, 2);
	sprintf(temp, "2nd: %s", sSecond);
	if((strcmp(sSecond, "")) != 0) {
	    iSecond=atoi(sSecond);
	}

	if((iSecond > CFG.iSafeMaxNumber) || (iSecond <= 0) || (strcmp(sSecond, "") == 0)) {
	    colour(WHITE, BLUE);
	    /* Please try again! You must input a number greater than Zero and less than */
	    printf("\n%s%d.\n", (char *) Language(92), CFG.iSafeMaxNumber);
	} else
	    break;
    }

    while (TRUE) {
	Enter(1);
	pout(LIGHTRED, BLACK, (char *) Language(94));
	colour(LIGHTBLUE, BLACK);
	fflush(stdout);
	Getnum(sThird, 2);
	sprintf(temp, "3rd: %s", sThird);
	if((strcmp(sThird, "")) != 0) {
	    iThird=atoi(sThird);
	}

	if((iThird > CFG.iSafeMaxNumber) || (iThird <= 0) || (strcmp(sThird, "") == 0)) {
	    colour(WHITE, BLUE);
	    /* Please try again! You must input a number greater than Zero and less than */
	    printf("\n%s%d.\n", (char *) Language(92), CFG.iSafeMaxNumber);
	} else
	    break;
    }

    /* Left: */
    Enter(1);
    pout(LIGHTRED, BLACK, (char *) Language(95));
    poutCR(LIGHTBLUE, BLACK, sFirst);

    /* Right: */
    pout(LIGHTRED, BLACK, (char *) Language(96));
    poutCR(LIGHTBLUE, BLACK, sSecond);

    /* Left: */
    pout(LIGHTRED, BLACK, (char *) Language(95));
    poutCR(LIGHTBLUE, BLACK, sThird);

    Enter(1);
    /* Attempt to open safe with this combination [Y/n]: */
    pout(LIGHTRED, BLACK, (char *) Language(97));
    fflush(stdout);
    alarm_on();
    i = toupper(Getone());
    sprintf(temp, "%c", i);

    if ((i == Keystroke(97, 0)) || (i == 13)) {
	printf("\n\n");
	tries++;
	Syslog('+', "Attempt %d with combination %d %d %d", tries, iFirst, iSecond, iThird);

	/* Left: */
	pout(LIGHTRED, BLACK, (char *) Language(95));
	for (iLoop = 0; iLoop < iFirst; iLoop++) {
	    pout(YELLOW, BLACK, (char *)".");
	    fflush(stdout);
	    usleep(100000);
	}
	poutCR(LIGHTBLUE, BLACK, sFirst);

	/* Right: */
	pout(LIGHTRED, BLACK, (char *) Language(96));
	for (iLoop = 0; iLoop < iSecond; iLoop++) {
	    pout(YELLOW, BLACK, (char *)".");
	    fflush(stdout);
	    usleep(100000);
	}
	poutCR(LIGHTBLUE, BLACK, sSecond);

	/* Left: */
	pout(LIGHTRED, BLACK, (char *) Language(95));
	for (iLoop = 0; iLoop < iThird; iLoop++) {
	    pout(YELLOW, BLACK, (char *)"."); 
	    fflush(stdout);
	    usleep(100000);
	}
	poutCR(LIGHTBLUE, BLACK, sThird);

	if(CFG.iSafeNumGen) {
	    CFG.iSafeFirstDigit  = (rand() % CFG.iSafeMaxNumber) + 1;
	    CFG.iSafeSecondDigit = (rand() % CFG.iSafeMaxNumber) + 1;
	    CFG.iSafeThirdDigit  = (rand() % CFG.iSafeMaxNumber) + 1;
	}

	if ((CFG.iSafeFirstDigit == iFirst) && (CFG.iSafeSecondDigit == iSecond) && (CFG.iSafeThirdDigit == iThird)) {

	    DisplayFile(CFG.sSafeOpened);
	    cracked = TRUE;

	    Enter(1);
	    /* You have won the following... */
	    pout(LIGHTRED, BLACK, (char *) Language(98));
	    Enter(2);
	    poutCR(LIGHTMAGENTA, BLACK, CFG.sSafePrize);
	    Enter(1);

	    Syslog('!', "User opened Safe Cracker Door");

	    Pause();
	    return TRUE;
	}

	Enter(1);
	pout(LIGHTGREEN, BLACK, (char *) Language(99));
	Enter(1);

	if(CFG.iSafeNumGen) {
	    Enter(1);
	    /* The safe code was: */
	    pout(LIGHTRED, BLACK, (char *) Language(100));
	    Enter(2);
	    colour(LIGHTRED, BLACK);

	    /* Left: */
	    printf("%s%d\n", (char *) Language(95), CFG.iSafeFirstDigit);

	    /* Right */
	    printf("%s%d\n", (char *) Language(96), CFG.iSafeSecondDigit);

	    /* Left */
	    printf("%s%d\n", (char *) Language(95), CFG.iSafeThirdDigit);
	}

	Enter(1);
	/* Please press key to continue */
	pout(LIGHTGREEN, BLACK, (char *) Language(87));
	alarm_on();
	getchar();
    }
    return FALSE;
}



/*
 * Returns true when safe already cracked or maximum trys exceeded
 */
int SafeCheckUser(int init)
{
    char *File, *Name, *Date;

    File = calloc(PATH_MAX, sizeof(char));
    Name = calloc(50, sizeof(char));
    Date = calloc(50, sizeof(char));

    sprintf(Name, "%s", exitinfo.sUserName);
    sprintf(Date, "%s", (char *) GetDateDMY());
    sprintf(File, "%s/etc/safe.data", getenv("MBSE_ROOT"));

    if ((pSafe = fopen(File, "r+")) == NULL) {
	if ((pSafe = fopen(File, "w")) != NULL) {
	    safehdr.hdrsize = sizeof(safehdr);
	    safehdr.recsize = sizeof(safe);
	    fwrite(&safehdr, sizeof(safehdr), 1, pSafe);
	    sprintf(safe.Date, "%s", (char *) GetDateDMY());
	    sprintf(safe.Name, "%s", Name);
	    safe.Trys   = 0;
	    safe.Opened = FALSE;
	    fwrite(&safe, sizeof(safe), 1, pSafe);
	    fclose(pSafe);
	    chmod(File, 0660);
	}
    } else {
	fread(&safehdr, sizeof(safehdr), 1, pSafe);
	/*
	 * Check if safe already cracked
	 */
	while (fread(&safe, safehdr.recsize, 1, pSafe) == 1) {
	    if (safe.Opened) {
		fclose(pSafe);
		Syslog('+', "Safe is currently LOCKED - exiting door.");

		/* THE SAFE IS CURRENTLY LOCKED */
		poutCR(WHITE, RED, (char *) Language(103));
		Enter(1);
		colour(LIGHTRED, BLACK);

		/* has cracked the safe. */
		printf("%s, %s\n", safe.Name, (char *) Language(104));

		/* The safe will remain locked until the sysop rewards the user. */
		pout(LIGHTGREEN, BLACK, (char *) Language(105));
		Enter(2);
		Pause();
		free(File);
		free(Name);
		free(Date);
		return TRUE;
	    }
	}
	fseek(pSafe, safehdr.hdrsize, SEEK_SET);

	/*
	 * Check if this user is already in the database
	 */
	while (fread(&safe, safehdr.recsize, 1, pSafe) == 1) {
	    if ((strcmp(Name, safe.Name)) == 0) {
		if ((strcmp(Date, safe.Date)) != 0) {
		    /*
		     * User found, but last time used is not today.
		     * Reset this user.
		     */
		    fseek(pSafe, - safehdr.recsize, SEEK_CUR);
		    sprintf(safe.Date, "%s", (char *) GetDateDMY());
		    safe.Trys = 0;
		    tries = 0;
		    safe.Opened = FALSE;
		    fwrite(&safe, safehdr.recsize, 1, pSafe);
		    fclose(pSafe);
		    free(File);
		    free(Name);
		    free(Date);
		    return FALSE;
		} else {
		    /*
		     * User found, last time is today, check attempts
		     */
		    fseek(pSafe, - safehdr.recsize, SEEK_CUR);
		    if (init)
			tries = safe.Trys;
		    else {
			safe.Trys = tries;
		    }
		    safe.Opened = cracked;
		    fwrite(&safe, safehdr.recsize, 1, pSafe);
		    fclose(pSafe);
		    free(File);
		    free(Name);
		    free(Date);
		    if (safe.Trys >= CFG.iSafeMaxTrys) {
			Syslog('+', "Maximum trys per day exceeded");
			Enter(2);
			/* Maximum trys per day exceeded */
			pout(WHITE, BLACK, (char *) Language(106));
			Enter(1);
			sleep(3);
			return TRUE;
		    }
		    return FALSE;
		}
	    }
	}
	
	/*
	 * User not found, append new record
	 */
	fclose(pSafe);
	if ((pSafe = fopen(File, "a")) == NULL) {
	    WriteError("Can't append to %s", File);
	    free(File);
	    free(Name);
	    free(Date);
	    return TRUE;
	}
	fseek(pSafe, 0, SEEK_END);
	memset(&safe, 0, sizeof(safe));
	sprintf(safe.Date, "%s", (char *) GetDateDMY());
	sprintf(safe.Name, "%s", Name);
	safe.Trys   = 0;
	safe.Opened = FALSE;
	tries = 0;
	fwrite(&safe, sizeof(safe), 1, pSafe);
	fclose(pSafe);
	Syslog('+', "Append new safe.data record");
    }

    free(File);
    free(Name);
    free(Date);
    return FALSE;
}


