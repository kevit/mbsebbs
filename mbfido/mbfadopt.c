/*****************************************************************************
 *
 * $Id$
 * Purpose: File Database Maintenance - Adopt file
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
#include "../lib/structs.h"
#include "../lib/records.h"
#include "../lib/common.h"
#include "../lib/clcomm.h"
#include "../lib/dbcfg.h"
#include "virscan.h"
#include "mbfutil.h"
#include "mbflist.h"



extern int	do_quiet;		/* Supress screen output	    */
extern int	do_annon;		/* Supress announce file	    */


void AdoptFile(int Area, char *File, char *Description)
{
    FILE		*pAreas, *fp;
    char		*sAreas, *temp, *temp2, *unarc, *pwd;
    char		Desc[256], TDesc[256];
    int			IsArchive = FALSE, MustRearc = FALSE, UnPacked = FALSE;
    int			IsVirus = FALSE, File_Id = FALSE;
    int			i, j, k, lines = 0, File_id_cnt = 0;
    struct FILERecord	fdb;

    Syslog('-', "Adopt(%d, %s, %s)", Area, MBSE_SS(File), MBSE_SS(Description));

    if (!do_quiet)
	colour(CYAN, BLACK);

    sAreas = calloc(PATH_MAX, sizeof(char));

    sprintf(sAreas, "%s/etc/fareas.data", getenv("MBSE_ROOT"));
    if ((pAreas = fopen (sAreas, "r")) == NULL) {
	WriteError("$Can't open %s", sAreas);
	if (!do_quiet)
	    printf("Can't open %s\n", sAreas);
	die(0);
    }

    fread(&areahdr, sizeof(areahdr), 1, pAreas);
    if (fseek(pAreas, ((Area - 1) * areahdr.recsize) + areahdr.hdrsize, SEEK_SET)) {
	WriteError("$Can't seek record %d in %s", Area, sAreas);
	if (!do_quiet)
	    printf("Can't seek record %d in %s\n", Area, sAreas);
	fclose(pAreas);
	free(sAreas);
	die(0);
    }

    if (fread(&area, areahdr.recsize, 1, pAreas) != 1) {
	WriteError("$Can't read record %d in %s", Area, sAreas);
	if (!do_quiet)
	    printf("Can't read record %d in %s\n", Area, sAreas);
	fclose(pAreas);
	free(sAreas);
	die(0);
    }

    if (area.Available) {
	temp   = calloc(PATH_MAX, sizeof(char));
	temp2  = calloc(PATH_MAX, sizeof(char));
	pwd    = calloc(PATH_MAX, sizeof(char));

	if (CheckFDB(Area, area.Path))
	    die(0);
	getcwd(pwd, PATH_MAX);

	if (!do_quiet) {
	    printf("Adopt file: %s ", File);
	    printf("Unpacking \b\b\b\b\b\b\b\b\b\b");
	    fflush(stdout);
	}

	sprintf(temp, "%s/%s", pwd, File);
	if ((unarc = unpacker(File)) == NULL) {
	    Syslog('+', "No known archive: %s", File);
	    sprintf(temp2, "%s/tmp/arc/%s", getenv("MBSE_ROOT"), File);
	    mkdirs(temp2);
	    if (file_cp(temp, temp2)) {
		WriteError("Can't copy file to %s", temp2);
		if (!do_quiet)
		    printf("Can't copy file to %s\n", temp2);
		die(0);
	    } else {
		if (!do_quiet) {
		    printf("Virscan   \b\b\b\b\b\b\b\b\b\b");
		    fflush(stdout);
		}
		IsVirus = VirScan();
		if (IsVirus) {
		    DeleteVirusWork();
		    chdir(pwd);
		    WriteError("Virus found");
		    if (!do_quiet)
			printf("Virus found\n");
		    die(0);
		}
	    }
	} else {
	    IsArchive = TRUE;
	    if (strlen(area.Archiver) && (strcmp(unarc, area.Archiver) == 0))
		MustRearc = TRUE;
	    UnPacked = UnpackFile(temp);
	    if (!UnPacked)
		die(0);

            if (!do_quiet) {
                printf("Virscan   \b\b\b\b\b\b\b\b\b\b");
                fflush(stdout);
            }

	    IsVirus = VirScan();
            if (IsVirus) {
		DeleteVirusWork();
                chdir(pwd);
		WriteError("Virus found");
		if (!do_quiet)
		    printf("Virus found\n");
		die(0);
            }
	}

        if (!do_quiet) {
	    printf("Checking  \b\b\b\b\b\b\b\b\b\b");
	    fflush(stdout);
        }

        memset(&fdb, 0, sizeof(fdb));
	strcpy(fdb.Uploader, CFG.sysop_name);
	fdb.UploadDate = time(NULL);
	if (do_annon)
	    fdb.Announced = TRUE;
	
	if (UnPacked) {
	    /*
	     * Try to get a FILE_ID.DIZ
	     */
            sprintf(temp, "%s/tmp/arc/FILE_ID.DIZ", getenv("MBSE_ROOT"));
            sprintf(temp2, "%s/tmp/FILE_ID.DIZ", getenv("MBSE_ROOT"));
            if (file_cp(temp, temp2) == 0) {
                File_Id = TRUE;
	    } else {
		sprintf(temp, "%s/tmp/arc/file_id.diz", getenv("MBSE_ROOT"));
		if (file_cp(temp, temp2) == 0)
		    File_Id = TRUE;
	    }

	    if (File_Id) {
		Syslog('-', "FILE_ID.DIZ found");
		if ((fp = fopen(temp2, "r"))) {
		    /*
		     * Read no more then 25 lines
		     */
		    while (((fgets(Desc, 255, fp)) != NULL) && (File_id_cnt < 25)) {
			lines++;
			/*
			 * Check if the FILE_ID.DIZ is in a normal layout.
			 * This should be max. 10 lines of max. 48 characters.
			 * We check at 51 characters and if the lines are longer,
			 * we discard the FILE_ID.DIZ file.
			 */
			if (strlen(Desc) > 51) {
			    File_id_cnt = 0;
			    File_Id = FALSE;
			    Syslog('!', "Discarding illegal formated FILE_ID.DIZ");
			    break;
			}

			if (strlen(Desc)) {
			    if (strlen(Desc) > 48)
				Desc[48] = '\0';
			    j = 0;
			    for (i = 0; i < strlen(Desc); i++) {
				if ((Desc[i] >= ' ') || (Desc[i] < 0)) {
				    fdb.Desc[File_id_cnt][j] = Desc[i];
				    j++;
				}
			    }
			    File_id_cnt++;
			}
		    }
		    fclose(fp);
		    unlink(temp2);

		    /*
		     * Strip empty lines at end of FILE_ID.DIZ
		     */
		    while ((strlen(fdb.Desc[File_id_cnt-1]) == 0) && (File_id_cnt))
			File_id_cnt--;

		    Syslog('f', "Got %d FILE_ID.DIZ lines", File_id_cnt);
		    for (i = 0; i < File_id_cnt; i++)
			Syslog('f', "\"%s\"", fdb.Desc[i]);
		}
	    }
	}

	if (!File_id_cnt) {
	    if (Description == NULL) {
		WriteError("No FILE_ID.DIZ and no description on the commandline");
		if (!do_quiet)
		    printf("No FILE_ID.DIZ and no description on the commandline\n");
		DeleteVirusWork();
		die(0);
	    } else {
		/*
		 * Create description from the commandline.
		 */
		if (strlen(Description) < 48) {
		    /*
		     * Less then 48 chars, copy and ready.
		     */
		    strcpy(fdb.Desc[0], Description);
		    File_id_cnt++;
		} else {
		    /*
		     * More then 48 characters, break into multiple
		     * lines not longer then 48 characters.
		     */
		    memset(&TDesc, 0, sizeof(TDesc));
		    strcpy(TDesc, Description);
		    while (strlen(TDesc) > 48) {
			j = 48;
			while (TDesc[j] != ' ')
			    j--;
			strncat(fdb.Desc[File_id_cnt], TDesc, j);
			File_id_cnt++;
			k = strlen(TDesc);
			j++; /* Correct space */
			for (i = 0; i <= k; i++, j++)
			    TDesc[i] = TDesc[j];
		    }
		    strcpy(fdb.Desc[File_id_cnt], TDesc);
		    File_id_cnt++;
		}
	    }
	}

	/*
	 * Import the file.
	 */
	chdir(pwd);
	DeleteVirusWork();
	if (strlen(File) < 13) {
	    strcpy(fdb.Name, File);
	    for (i = 0; i < strlen(File); i++)
		fdb.Name[i] = toupper(fdb.Name[i]);
	} else {
	    WriteError("Long filename conversion not supported");
	    if (!do_quiet)
		printf("Long filename conversion not supported\n");
	    die(0);
	}
	strcpy(fdb.LName, File);
	fdb.Size = file_size(File);
	fdb.Crc32 = file_crc(File, TRUE);
	fdb.FileDate = file_time(File);
	sprintf(temp2, "%s/%s", area.Path, File);

	if (!do_quiet) {
	    printf("Adding    \b\b\b\b\b\b\b\b\b\b");
	    fflush(stdout);
	}

	if (AddFile(fdb, Area, temp2, File) == FALSE) {
	    die(0);
	}
	Syslog('+', "File %s added to area %d", File, Area);

	if (MustRearc) {
	    /* Here we should call the rearc function */
	}

	free(pwd);
	free(temp2);
	free(temp);
    } else {
	WriteError("Area %d is not available", Area);
	if (!do_quiet)
	    printf("Area %d is not available\n", Area);
    }

    fclose(pAreas);
    if (!do_quiet) {
	printf("\r                                                              \r");
	fflush(stdout);
    }

    free(sAreas);
}

