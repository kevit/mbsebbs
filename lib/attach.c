/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Attach files to outbound
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



/*
 * Attach a file to the real outbound fo a given node.
 * If fdn == TRUE, the the file is a forwarded tic file, then
 * make sure to see if there was an old file with the same name
 * that the old attach is removed including the .tic file so
 * that we will send the new file with the right .tic file.
 */
int attach(faddr noden, char *ofile, int mode, char flavor, int fdn)
{
    FILE    *fp;
    char    *flofile, *thefile;
    int	    rc;

    if (ofile == NULL)
	return FALSE;

    if ((rc = file_exist(ofile, R_OK))) {
	WriteError("attach: file %s failed, %s", ofile, strerror(rc));
	return FALSE;
    }

    /*
     * Check if we attach a file with the same name
     */
    if ((fdn) && (un_attach(noden, flavor, ofile) == FALSE)) {
	WriteError("attach: can't un_attach %s, %s", ofile, strerror(rc));
	return FALSE;
    }

    flofile = calloc(PATH_MAX, sizeof(char));
    thefile = calloc(PATH_MAX, sizeof(char));
    sprintf(flofile, "%s", floname(&noden, flavor));

    /*
     * Check if outbound directory exists and 
     * create if it doesn't exist.
     */
    mkdirs(ofile, 0770);

    /*
     *  Attach file to .flo
     *
     *  Note that mbcico when connected to a node opens the file "r+",
     *  locks it with fcntl(F_SETLK), F_RDLCK, whence=0, start=0L, len=0L.
     *  It seems that this lock is released after the files in the .flo
     *  files are send. I don't know what will happen if we add entries
     *  to the .flo files, this must be tested!
     */
    if ((fp = fopen(flofile, "a+")) == NULL) {
	WriteError("$Can't open %s", flofile);
	WriteError("May be locked by mbcico");
	free(flofile);
	free(thefile);
	return FALSE;
    }

    switch (mode) {
	case LEAVE:
	    if (strlen(CFG.dospath)) {
		if (CFG.leavecase)
		    sprintf(thefile, "@%s", Unix2Dos(ofile));
		else
		    sprintf(thefile, "@%s", tu(Unix2Dos(ofile)));
	    } else {
		sprintf(thefile, "@%s", ofile);
	    }
	    break;

	case KFS:
	    if (strlen(CFG.dospath)) {
		if (CFG.leavecase)
		    sprintf(thefile, "^%s", Unix2Dos(ofile));
		else
		    sprintf(thefile, "^%s", tu(Unix2Dos(ofile)));
	    } else {
		sprintf(thefile, "^%s", ofile);
	    }
	    break;

	case TFS:
	    if (strlen(CFG.dospath)) {
		if (CFG.leavecase)
		    sprintf(thefile, "#%s", Unix2Dos(ofile));
		else
		    sprintf(thefile, "#%s", tu(Unix2Dos(ofile)));
	    } else {
		sprintf(thefile, "#%s", ofile);
	    }
	    break;
    }

    fseek(fp, 0, SEEK_END);
    fprintf(fp, "%s\r\n", thefile);
    fclose(fp);
    free(flofile);
    free(thefile);
    return TRUE;
}



/*
 * Remove a file from the flofile, also search for a .tic file.
 */
int un_attach(faddr node, char flavor, char *filename)
{
    char    *flofile, *buf;
    FILE    *fp;

    Syslog('p', "un_attach: %s %s", ascfnode(&node, 0x1f), filename);

    buf = calloc(PATH_MAX+3, sizeof(char));
    flofile = calloc(PATH_MAX, sizeof(char));
    sprintf(flofile, "%s", floname(&noden, flavor));

    if ((fp = fopen(flofile, "r+"))) {
	while (fgets(buf, sizeof(buf -1), fp)) {
	    Striplf(buf);
	    Syslog('p',  "flo: \"%s\"", buf);

	}
	fclose(flofile);
    }

    free(flofile);
    free(buf);

    return TRUE;
}

