/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Basic File I/O
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
 * This toolkit is free software; you can redistribute it and/or modify it
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
 * along with MBTOOL; see the file COPYING.  If not, write to the Free
 * Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *****************************************************************************/

#include "../config.h"
#include "libs.h"
#include "memwatch.h"
#include "structs.h"
#include "clcomm.h"
#include "common.h"


/*
 * Buffered file copy, filetime is preserved.
 */
int file_cp(char *from, char *to)
{
	char	*line;
	FILE	*stfrom, *stto;
	int	dummy, bread;
	static	int error;
	struct	stat sb;
	struct	utimbuf ut;

	stfrom = fopen(from, "r");
	if (stfrom == NULL)
		return errno;

	stto = fopen(to, "w");
	if (stto == NULL) {
		error = errno;
		fclose(stfrom);
		return error;
	}

	line = malloc(16384);

	do {
		bread = fread(line, 1, 16384, stfrom);
		dummy = fwrite(line, 1, bread, stto);
		if (bread != dummy) {
			error = errno;
			fclose(stfrom);
			fclose(stto);
			unlink(to);
			free(line);
			return error;
		}
		Nopper();  // For large files on slow systems
	} while (bread != 0);

	free(line);
	fclose(stfrom);
	if (fclose(stto) != 0) {
		error = errno;
		unlink(to);
		return error;
	}
  
	/*
	 * copy successfull, now copy file- and modification-time
	 */
	if (stat(from, &sb) == 0) {
		ut.actime = mktime(localtime(&sb.st_atime));
		ut.modtime = mktime(localtime(&sb.st_mtime));
		if (utime(to, &ut) != 0) {
			error = errno;
			unlink(to);
			return error;
		}
		chmod(to, sb.st_mode);
	}

	return 0;
}



/*
 * Remove a file
 */
int file_rm(char *path)
{
	if (unlink(path) != 0) 
		return errno;
	return 0;
}



/*
 * Move or rename a file. Not fullproof if using NFS, see
 * man 2 rename. If we are trying to move a file accross
 * filesystems, which is not allowed,  we fall back to simple
 * copy the file and then delete the old file.
 */
int file_mv(char *oldpath, char *newpath)
{
	static	int error;

	if (rename(oldpath, newpath) != 0) {
		error = errno;
		if (error != EXDEV)
			return error;
		/*
		 * We tried cross-device link, now the slow way :-)
		 */
		error = file_cp(oldpath, newpath);
		if (error != 0)
			return error;
		error = file_rm(oldpath);
		return 0;
	}

	return 0;
}



/*
 * Test if the given file exists. The second option is:
 * R_OK - test for Read rights 
 * W_OK - test for Write rights
 * X_OK - test for eXecute rights
 * F_OK - test file presence only
 */ 
int file_exist(char *path, int mode)
{
	if (access(path, mode) != 0)
		return errno;

	return 0;
}



/*
 * Return size of file, or -1 if file doesn't exist
 */
long file_size(char *path)
{
	static struct	stat sb;

	if (stat(path, &sb) == -1)
		return -1;

	return sb.st_size;
}



/*
 * Claclulate the 32 bit CRC of a file. Return -1 if file not found.
 */
long file_crc(char *path, int slow)
{
	static	long crc;
	int	bread;
	FILE	*fp;
	char	*line;

	if ((fp = fopen(path, "r")) == NULL) 
		return -1;

	line = malloc(32768);
	crc = 0xffffffff;

	do {
		bread = fread(line, 1, 32768, fp);
		crc = upd_crc32(line, crc, bread);
		if (slow)
			usleep(1);
		Nopper(); // For large files on slow systems.
	} while (bread > 0);

	free(line);
	fclose(fp);
	return crc ^ 0xffffffff;
}



/*
 * Return time of file, or -1 if file doen't exist, which is
 * the same as 1 second before 1 jan 1970. You may test the 
 * result on -1 since time_t is actualy a long integer.
 */
time_t file_time(char *path)
{
	static struct stat sb;

	if (stat(path, &sb) == -1)
		return -1;

	return sb.st_mtime;
}



/*
 *    Make directory tree, the name must end with a /
 */
int mkdirs(char *name, mode_t mode)
{
	char	buf[PATH_MAX], *p, *q;
	int	rc, last = 0, oldmask;

	memset(&buf, 0, sizeof(buf));
	strncpy(buf, name, sizeof(buf)-1);
	buf[sizeof(buf)-1] = '\0';

	p = buf+1;

	oldmask = umask(000);
	while ((q = strchr(p, '/'))) {
		*q = '\0';
		rc = mkdir(buf, mode);
		last = errno;
		*q = '/';
		p = q+1;
	}

	umask(oldmask);

	if ((last == 0) || (last == EEXIST)) {
		return TRUE;
	} else {
		WriteError("$mkdirs(%s)", name);
		return FALSE;
	}
}



/*
 *  Check free diskspace on most filesystems. Exclude check on floppyies,
 *  CD's and /boot partition. The amount of needed space is given in MBytes.
 */
int diskfree(int needed)
{
    int		    RetVal = TRUE;
    struct statfs   sfs;
    unsigned long   temp;
#if defined(__linux__)
    char	    *mtab, *dev, *fs, *type;
    FILE	    *fp;
#elif defined(__FreeBSD__) || defined(__NetBSD__)
    struct statfs   *mntbuf;
    long	    mntsize;
    int		    i;
#endif

    if (! needed)
	return TRUE;

#if defined(__linux__)

    if ((fp = fopen((char *)"/etc/mtab", "r")) == 0) {
	WriteError("$Can't open /etc/mtab");
	return TRUE;
    }

    mtab = calloc(PATH_MAX, sizeof(char));
    while (fgets(mtab, PATH_MAX, fp)) {
	dev  = strtok(mtab, " \t");
	fs   = strtok(NULL, " \t");
	type = strtok(NULL, " \t");
	if (strncmp((char *)"/dev/", dev, 5) == 0) {
	    /*
	     *  Filter out unwanted filesystems, floppy.
	     *  Also filter out the /boot file system.
	     */
	    if (strncmp((char *)"/dev/fd", dev, 7) && strncmp((char *)"/boot", fs, 5) &&
	        (!strncmp((char *)"ext2", type, 4) || !strncmp((char *)"reiserfs", type, 8) ||
		 !strncmp((char *)"vfat", type, 4) || !strncmp((char *)"msdos", type, 5) ||
		 !strncmp((char *)"ext3", type, 4))) {
		if (statfs(fs, &sfs) == 0) {
		    temp = (unsigned long)(sfs.f_bsize / 512L);
		    if (((unsigned long)(sfs.f_bavail * temp) / 2048L) < needed) {
			RetVal = FALSE;
			WriteError("On \"%s\" only %d kb left, need %d kb", fs, 
			    (sfs.f_bavail * sfs.f_bsize) / 1024, needed * 1024);
		    }
		}
	    }
	}
    }
    fclose(fp);
    free(mtab);

#elif defined(__FreeBSD__) || defined(__NetBSD__)

    if ((mntsize = getmntinfo(&mntbuf, MNT_NOWAIT)) == 0) {
	WriteError("Huh, no filesystems mounted??");
	return TRUE;
    }

    for (i = 0; i < mntsize; i++) {
	/*
	 *  Don't check kernfs (NetBSD) and procfs (FreeBSD), floppy and CD.
	 */
	if ((strncmp(mntbuf[i].f_fstypename, (char *)"kernfs", 6)) &&
	    (strncmp(mntbuf[i].f_fstypename, (char *)"procfs", 6)) &&
	    (strncmp(mntbuf[i].f_mntfromname, (char *)"/dev/fd", 7)) &&
	    (strncmp(mntbuf[i].f_fstypename, (char *)"cd9660", 6)) &&
	    (strncmp(mntbuf[i].f_fstypename, (char *)"msdos", 5)) &&
	    (statfs(mntbuf[i].f_mntonname, &sfs) == 0)) {
	    temp = (unsigned long)(sfs.f_bsize / 512L);
	    if (((unsigned long)(sfs.f_bavail * temp) / 2048L) < needed) {
		RetVal = FALSE;
		WriteError("On \"%s\" only %d kb left, need %d kb", mntbuf[i].f_mntonname,
		    (sfs.f_bavail * sfs.f_bsize) / 1024, needed * 1024);
	    }
	}
    }

#endif

    return RetVal;
}


