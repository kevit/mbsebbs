/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Display Userlist
 *
 *****************************************************************************
 * Copyright (C) 1997-2001 
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

#include "../lib/libs.h"
#include "../lib/mbse.h"
#include "../lib/structs.h"
#include "../lib/records.h"
#include "../lib/common.h"
#include "../lib/clcomm.h"
#include "userlist.h"
#include "language.h"
#include "input.h"
#include "timeout.h"



void UserList(char *OpData)
{                                                                        
	FILE	*pUsrConfig;
	int	LineCount = 2;
	int	iFoundName = FALSE;
	int	iNameCount = 0;
	char	*Name, *sTemp, *User;
	char	*temp;
	struct	userhdr	uhdr;
	struct	userrec	u;

	temp  = calloc(PATH_MAX, sizeof(char));
	Name  = calloc(37, sizeof(char));
	sTemp = calloc(81, sizeof(char));
	User  = calloc(81, sizeof(char));

	clear();
	/* User List */
	language(WHITE, BLACK, 126);
	Enter(1);
	LineCount = 1;

	sprintf(temp, "%s/etc/users.data", getenv("MBSE_ROOT"));
	if ((pUsrConfig = fopen(temp, "rb")) == NULL) {
		WriteError("UserList: Can't open file: %s", temp);
		return;
	}
	fread(&uhdr, sizeof(uhdr), 1, pUsrConfig);

	/* Enter Username search string or (Enter) for all users: */
	language(WHITE, BLACK, 127);
	colour(CFG.InputColourF, CFG.InputColourB);
	alarm_on();
	GetstrC(Name, 35);
	clear();

	/* Name         Location                   Last On    Calls */
	language(WHITE, BLACK, 128);
	Enter(1);

	colour(GREEN, BLACK);
	fLine(79);

	colour(CYAN, BLACK);
	while (fread(&u, uhdr.recsize, 1, pUsrConfig) == 1) {
		if ((strcmp(Name,"")) != 0) {
			if((strcmp(OpData, "/H")) == 0)
				sprintf(User, "%s", u.sHandle);
			else
				sprintf(User, "%s", u.sUserName);

			if ((strstr(tl(User), tl(Name)) != NULL)) {
				if ((!u.Hidden) && (!u.Deleted)) {
					if((strcmp(OpData, "/H")) == 0) {
						if((strcmp(u.sHandle, "") != 0 && *(u.sHandle) != ' '))
							printf("%-25s", u.sHandle);
						else
							printf("%-25s", u.sUserName);
					} else 
						printf("%-25s", u.sUserName);

					printf("%-30s%-14s%-11d", u.sLocation, StrDateDMY(u.tLastLoginDate), u.iTotalCalls);
					iFoundName = TRUE;
					LineCount++;
					iNameCount++; 
				}
			}
		} else
			if ((!u.Hidden) && (!u.Deleted) && (strlen(u.sUserName) > 0)) {
				if((strcmp(OpData, "/H")) == 0) {
					if((strcmp(u.sHandle, "") != 0 && *(u.sHandle) != ' '))
						printf("%-25s", u.sHandle);
					else
						printf("%-25s", u.sUserName);
				} else
					printf("%-25s", u.sUserName);

	   	    		printf("%-30s%-14s%-11d", u.sLocation, StrDateDMY(u.tLastLoginDate), u.iTotalCalls);
				iFoundName = TRUE;
				LineCount++;
				iNameCount++;
				Enter(1);
			}

		if (LineCount >= exitinfo.iScreenLen - 2) {
			LineCount = 0;
			Pause();
			colour(CYAN, BLACK);
		}
	}

	if(!iFoundName) {
		language(CYAN, BLACK, 129);
		Enter(1);
	}

	fclose(pUsrConfig);

	colour(GREEN, BLACK);
	fLine(79); 

	free(temp);
	free(Name);
	free(sTemp);
	free(User);

	Pause();
}



