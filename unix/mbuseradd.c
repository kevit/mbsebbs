/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: setuid root version of useradd
 *
 *****************************************************************************
 * Copyright (C) 1997-2004
 *   
 * Michiel Broek	FIDO:		2:280/2802
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
#include <stdio.h>
#include <stdlib.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/param.h>
#include <syslog.h>

#include "mbuseradd.h"




int execute(char *cmd, char *file, char *in, char *out, char *err)
{
    char    buf[PATH_MAX], *vector[16];
    int	    i, pid, status = 0, rc = 0;

    sprintf(buf, "%s %s", cmd, file);
    syslog(LOG_WARNING, "Execute: %s", buf);
	
    memset(vector, 0, sizeof(vector));
    i = 0;
    vector[i++] = strtok(buf, " \t\n");
    while ((vector[i++] = strtok(NULL," \t\n")) && (i < 16)) { syslog(LOG_NOTICE, "%s", vector[i]); } ;
    vector[15] = NULL;
    fflush(stdout);
    fflush(stderr);

    if ((pid = fork()) == 0) {
	if (in) {
	    close(0);
	    if (open(in, O_RDONLY) != 0) {
		syslog(LOG_WARNING, "Reopen of stdin to %s failed", in);
		_exit(-1);
	    }
	}
	if (out) {
	    close(1);
	    if (open(out, O_WRONLY | O_APPEND | O_CREAT,0600) != 1) {
		syslog(LOG_WARNING, "Reopen of stdout to %s failed", out);
		_exit(-1);
	    }
	}
	if (err) {
	    close(2);
	    if (open(err, O_WRONLY | O_APPEND | O_CREAT,0600) != 2) {
		syslog(LOG_WARNING, "Reopen of stderr to %s failed", err);
		_exit(-1);
	    }
	}
	rc = execv(vector[0],vector);
	syslog(LOG_WARNING, "Exec \"%s\" returned %d", vector[0], rc);
	_exit(-1);
    }

    do {
	rc = wait(&status);
    } while (((rc > 0) && (rc != pid)) || ((rc == -1) && (errno == EINTR)));

    if (rc == -1) {
	syslog(LOG_WARNING, "Wait returned %d, status %d,%d", rc, status >> 8, status & 0xff);
	return -1;
    }

    return 0;
}



void makedir(char *path, mode_t mode, uid_t owner, gid_t group)
{
    if (mkdir(path, mode) != 0) {
	syslog(LOG_WARNING, "Can't create directory %s", path);
	exit(2);
    }
    if ((chown(path, owner, group)) == -1) {
	syslog(LOG_WARNING, "Unable to change ownership of %s", path);
	exit(2);
    }
}



/*
 * Function will create the users name in the passwd file
 * Note that this function must run setuid root!
 */
int main(int argc, char *argv[])
{
    char	    *PassEnt, *temp, *shell;
    int		    i;
    struct passwd   *pwent, *pwuser;

    if (argc != 5)
	Help();

    /*
     *  First simple check for argument overflow
     */
    for (i = 1; i < 5; i++) {
	if (strlen(argv[i]) > 80) {
	    fprintf(stderr, "mbuseradd: Argument %d is too long\n", i);
	    exit(1);
	}
    }

    PassEnt = calloc(PATH_MAX, sizeof(char));
    temp    = calloc(PATH_MAX, sizeof(char));
    shell   = calloc(PATH_MAX, sizeof(char));

    if (setuid(0) == -1 || setgid(1) == -1) {
        perror("");
        fprintf(stderr, "mbuseradd: Unable to setuid(root) or setgid(root)\n");
        fprintf(stderr, "Make sure that this program is set to -rwsr-sr-x\n");
        fprintf(stderr, "Owner must be root and group root\n");
        exit(1);
    }
    umask(0000);

    /*
     * We don't log into MBSE BBS logfiles but to the system logfiles,
     * because we are modifying system files not belonging to MBSE BBS.
     */
    openlog("mbuseradd", LOG_PID|LOG_CONS|LOG_NOWAIT, LOG_AUTH);
    syslog(LOG_WARNING, "mbuseradd %s %s %s %s", argv[1], argv[2], argv[3], argv[4]);

    /*
     * Build command to add user entry to the /etc/passwd and /etc/shadow
     * files. We use the systems own useradd program.
     */
#if defined(__linux__) || defined(__NetBSD__)
    if ((access("/usr/bin/useradd", R_OK)) == 0)
	strcpy(temp, "/usr/bin/useradd");
    else if ((access("/bin/useradd", R_OK)) == 0)
	strcpy(temp, "/bin/useradd");
    else if ((access("/usr/sbin/useradd", R_OK)) == 0)
	strcpy(temp, "/usr/sbin/useradd");
    else if ((access("/sbin/useradd", R_OK)) == 0)
	strcpy(temp, "/sbin/useradd");
    else {
	syslog(LOG_WARNING, "Can't find useradd");
	exit(1);
    }
#elif __FreeBSD__
    if ((access("/usr/sbin/pw", X_OK)) == 0)
	strcpy(temp, "/usr/sbin/pw");
    else if ((access("/sbin/pw", X_OK)) == 0)
	strcpy(temp, "/sbin/pw");
    else {
	syslog(LOG_WARNING, "Can't find pw");
	exit(1);
    }
#else
    syslog(LOG_WARNING, "Don't know how to add a user on this OS");
    exit(1);
#endif

    sprintf(shell, "%s/bin/mbsebbs", getenv("MBSE_ROOT"));

#if defined(__linux__) || defined(__NetBSD__)
    sprintf(PassEnt, "%s -c \"%s\" -d %s/%s -g %s -s %s %s", temp, argv[3], argv[4], argv[2], argv[1], shell, argv[2]);
#endif
#ifdef __FreeBSD__
    sprintf(PassEnt, "%s useradd %s -c \"%s\" -d %s/%s -g %s -s %s", temp, argv[2], argv[3], argv[4], argv[2], argv[1], shell);
#endif

    syslog(LOG_WARNING, "system(%s)", PassEnt);
    if (system(PassEnt) != 0) {
	syslog(LOG_WARNING, "Failed to create unix account");
	exit(1);
    } 

    /*
     * Now create directories and files for this user.
     */
    if ((pwent = getpwnam((char *)"mbse")) == NULL) {
	syslog(LOG_WARNING, "Can't get password entry for \"mbse\"");
	exit(2);
    }

    /*
     *
     *  Check bbs users base home directory
     */
    if ((access(argv[4], R_OK)) != 0)
	makedir(argv[4], 0770, pwent->pw_uid, pwent->pw_gid);

    /*
     * Now create users home directory. Check for an existing directory,
     * some systems have already created a home directory. If one is found
     * it is removed to create a fresh one.
     */
    sprintf(temp, "%s/%s", argv[4], argv[2]);
    if ((access(temp, R_OK)) == 0) {
	if ((access("/bin/rm", X_OK)) == 0)
	    strcpy(shell, "/bin/rm");
	else if ((access("/usr/bin/rm", X_OK)) == 0)
	    strcpy(shell, "/usr/bin/rm");
	else {
	    syslog(LOG_WARNING, "Can't find rm");
	    exit(2);
	}
	sprintf(PassEnt, " -Rf %s", temp);
	i = execute(shell, PassEnt, (char *)"/dev/tty", (char *)"/dev/tty", (char *)"/dev/tty");
	if (i != 0) {
	    syslog(LOG_WARNING, "Unable remove old home directory");
	    exit(2);
	}
    }

    /*
     *  Create users home directory.
     */
    if ((pwuser = getpwnam(argv[2])) == NULL) {
	syslog(LOG_WARNING, "Can't get passwd entry for %s", argv[2]);
	exit(2);
    }
    makedir(temp, 0770, pwuser->pw_uid, pwent->pw_gid);

    /*
     *  Create Maildir and subdirs for Qmail.
     */
    sprintf(temp, "%s/%s/Maildir", argv[4], argv[2]);
    makedir(temp, 0700, pwuser->pw_uid, pwent->pw_gid);
    sprintf(temp, "%s/%s/Maildir/cur", argv[4], argv[2]);
    makedir(temp, 0700, pwuser->pw_uid, pwent->pw_gid);
    sprintf(temp, "%s/%s/Maildir/new", argv[4], argv[2]);
    makedir(temp, 0700, pwuser->pw_uid, pwent->pw_gid);
    sprintf(temp, "%s/%s/Maildir/tmp", argv[4], argv[2]);
    makedir(temp, 0700, pwuser->pw_uid, pwent->pw_gid);

#ifdef _VPOPMAIL_PATH
    sprintf(temp, "%s/vadduser %s %s", _VPOPMAIL_PATH, argv[2], argv[2]);

    if (system(temp) != 0) {
	syslog(LOG_WARNING, "Failed to create vpopmail account");
    }
#endif

    free(shell);
    free(PassEnt);
    free(temp);
    syslog(LOG_WARNING, "Added system account for user\"%s\"", argv[2]);
    exit(0);
}



void Help()
{
    fprintf(stderr, "\nmbuseradd commandline:\n\n");
    fprintf(stderr, "mbuseradd [gid] [name] [comment] [usersdir]\n");
    exit(1);
}


