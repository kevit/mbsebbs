/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Archive unpacker
 *
 *****************************************************************************
 * Copyright (C) 1997-2002
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
 * Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *****************************************************************************/

#define	DB_ARCHIVE

#include "../config.h"
#include "libs.h"
#include "memwatch.h"
#include "structs.h"
#include "users.h"
#include "records.h"
#include "clcomm.h"
#include "common.h"


char *unpacker(char *fn)
{
	FILE		*fp;
	unsigned char	buf[8];

	if ((fp = fopen(fn,"r")) == NULL) {
		WriteError("$Could not open file %s", fn);
		return NULL;
	}

	if (fread(buf,1,sizeof(buf),fp) != sizeof(buf)) {
		WriteError("$Could not read head of the file %s", fn);
		fclose(fp);
		return NULL;
	}

	fclose(fp);

	if (memcmp(buf,"PK\003\004",4) == 0)	return (char *)"ZIP";
	if (*buf == 0x1a)			return (char *)"ARC";
	if (memcmp(buf+2,"-l",2) == 0)		return (char *)"LHA";
	if (memcmp(buf,"ZOO",3) == 0)		return (char *)"ZOO";
	if (memcmp(buf,"`\352",2) == 0)		return (char *)"ARJ";
	if (memcmp(buf,"Rar!",4) == 0)		return (char *)"RAR";
	if (memcmp(buf,"UC2\0x1a",4) == 0)	return (char *)"UC2";
	if (memcmp(buf,"BZ",2) == 0)		return (char *)"BZIP";
	if (memcmp(buf,"MSCF",4) == 0)		return (char *)"CAB";   /* M$ CAB files	    */
	if (memcmp(buf,"\037\213",2) == 0)	return (char *)"GZIP";  /* gzip compressed data */
	if (memcmp(buf,"\037\235",2) == 0)	return (char *)"CMP";   /* unix compressed data */
	
	if ((fp = fopen(fn,"r"))) {
	    fseek(fp, 257, SEEK_SET);
	    if (fread(buf,1,sizeof(buf),fp) != sizeof(buf)) {
		WriteError("$Could not read position 257 of the file %s", fn);
		fclose(fp);
		return NULL;
	    }
	    fclose(fp);
	}
	if (memcmp(buf,"ustar",5) == 0)	    return (char *)"TAR";   /* GNU/Posix tar	    */

	Syslog('p', "Unknown compress scheme in file %s", fn);
	return NULL;
}



int getarchiver(char *unarc)
{
	FILE	*fp;
	char	*filename;

	memset(&archiver, 0, sizeof(archiver));
	filename = calloc(PATH_MAX, sizeof(char));
	sprintf(filename, "%s/etc/archiver.data", getenv("MBSE_ROOT"));

	if ((fp = fopen(filename, "r")) == NULL) {
		WriteError("$Can't open %s", filename);
		free(filename);
		return FALSE;
	}
	free(filename);

	fread(&archiverhdr, sizeof(archiverhdr), 1, fp);
	while (fread(&archiver, archiverhdr.recsize, 1, fp) == 1) {
		if ((archiver.available) && (strcmp(archiver.name, unarc) == 0)) {
			fclose(fp);
			return TRUE;
		}
	}

	fclose(fp);
	return FALSE;
}



