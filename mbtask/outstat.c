/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: mbtask - Scan mail outbound status
 *
 *****************************************************************************
 * Copyright (C) 1997-2001
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

#include "libs.h"
#include "../lib/structs.h"
#include "taskutil.h"
#include "taskstat.h"
#include "scanout.h"
#include "nodelist.h"
#include "callstat.h"
#include "outstat.h"


extern int		do_quiet;
extern int		internet;	    /* Internet status		*/
extern struct sysconfig	CFG;		    /* Main configuration	*/
extern struct taskrec	TCFG;		    /* Task configuration	*/
int			nxt_hour, nxt_min;  /* Time of next event	*/
int			inet_calls;	    /* Internet calls to make	*/
int			isdn_calls;	    /* ISDN calls to make	*/
int			pots_calls;	    /* POTS calls to make	*/
_alist_l		*alist = NULL;	    /* Nodes to call list	*/
extern int		s_do_inet;	    /* Internet wanted		*/



/*
 * Load noderecord if the node is in our setup.
 */
int load_node(fidoaddr);
int load_node(fidoaddr n)
{
    char    *temp;
    FILE    *fp;
    int	    i, j = 0;

    temp = calloc(PATH_MAX, sizeof(char));
    sprintf(temp, "%s/etc/nodes.data", getenv("MBSE_ROOT"));
    if ((fp = fopen(temp, "r")) == NULL) {
	free(temp);
	memset(&nodes, 0, sizeof(nodes));
	return FALSE;
    }
 
    fread(&nodeshdr, sizeof(nodeshdr), 1, fp);
    while (fread(&nodes, nodeshdr.recsize, 1, fp) == 1) {
	fseek(fp, nodeshdr.filegrp + nodeshdr.mailgrp, SEEK_CUR);
	j++;
	for (i = 0; i < 20; i++) {
	    if ((n.zone == nodes.Aka[i].zone) && (n.net == nodes.Aka[i].net) &&
		(n.node == nodes.Aka[i].node) && (n.point == nodes.Aka[i].point)) {
		fclose(fp);
		free(temp);
//		tasklog('-' , "Node record %d, aka nr %d", j, i+1);
		return TRUE;
	    }
	}
    }

    fclose(fp);
    memset(&nodes, 0, sizeof(nodes));
    free(temp);
    return FALSE;    
}



char *size_str(long);
char *size_str(long size)
{
    static char	fmt[25];
    
    if (size > 1048575) {
	sprintf(fmt, "%ldK", size / 1024);
    } else {
	sprintf(fmt, "%ld ", size);
    }
    return fmt;
}



void set_next(int, int);
void set_next(int hour, int min)
{
    time_t	now;
    struct tm	*etm;
    int		uhour, umin;

    now = time(NULL);
    etm = gmtime(&now);
    uhour = etm->tm_hour; /* For some reason, these intermediate integers are needed */
    umin  = etm->tm_min;

    if ((hour > uhour) || ((hour == uhour) && (min > umin))) {
	if (hour < nxt_hour) {
	    nxt_hour = hour;
	    nxt_min  = min;
	} else if ((hour == nxt_hour) && (min < nxt_min)) {
	    nxt_hour = hour;
	    nxt_min  = min;
	}
    }
}



char *callstatus(int status)
{
    switch (status) {
	case 0:	    return (char *)"Ok     ";
	case 1:	    return (char *)"tty err";
	case 2:	    return (char *)"No conn";
	case 3:	    return (char *)"Mdm err";
	case 4:	    return (char *)"Locked ";
	case 5:	    return (char *)"unknown";
	case 6:	    return (char *)"Unlist ";
	case 7:	    return (char *)"error 7";
	case 8:	    return (char *)"error 8";
	case 9:	    return (char *)"No tty ";
	case 10:    return (char *)"No ZMH ";
	case 30:    return (char *)"Badsess";
	default:    return (char *)"ERROR  ";
    }
}



char *callmode(int mode)
{
    switch (mode) {
	case CM_NONE:	return (char *)"None   ";
	case CM_INET:	return (char *)"Inet   ";
	case CM_ISDN:	return (char *)"ISDN   ";
	case CM_POTS:	return (char *)"POTS   ";
	case MBFIDO:	return (char *)"mbfido ";
	case MBINDEX:	return (char *)"mbindex";
	case MBFILE:	return (char *)"mbfile ";
	case MBINIT:	return (char *)"mbinit ";
	default:	return (char *)"None   ";
    }
}



/*
 * Scan outbound, the call status is set in three counters: internet,
 * ISDN and POTS (analogue modems).
 * For all systems the CM and Txx flags are checked and for official
 * FidoNet nodes the Zone Mail Hour wich belongs to the destination
 * zone.
 * All nodes are qualified, if there is a way to call them or not on
 * this moment.
 * The method how to call a node is decided as well.
 */
int outstat()
{
    int		    rc, first = TRUE, T_window, iszmh = FALSE;
    struct _alist   *tmp, *old;
    char	    flstr[13];
    char	    temp[81];
    char	    as[6], be[6], utc[6];
    time_t	    now;
    struct tm	    *tm;
    int		    uhour, umin, thour, tmin;

    now = time(NULL);
    tm = gmtime(&now); /* UTC time */
    uhour = tm->tm_hour;
    umin  = tm->tm_min;
    sprintf(utc, "%02d:%02d", uhour, umin);
    tasklog('+', "Scanning outbound at %s UTC.", utc);
    nxt_hour = 24;
    nxt_min  = 0;
    inet_calls = isdn_calls = pots_calls = 0;

    /*
     *  Clear current table
     */
    for (tmp = alist; tmp; tmp = old) {
	old = tmp->next;
	free(tmp);
    }
    alist = NULL;

    if ((rc = scanout(each))) {
	tasklog('?', "Error scanning outbound, aborting");
	return rc;
    }

    /*
     * During processing the outbound list, determine when the next event will occur,
     * ie. the time when the callout status of a node changes because of starting a
     * ZMH, or changeing the time window for Txx flags.
     */
    for (tmp = alist; tmp; tmp = tmp->next) {
	if (first) {
	    tasklog('+', "Flavor Out       Size    Online    Modem     ISDN   TCP/IP Calls Status  Mode    Address");
	    first = FALSE;
	}

	rc = load_node(tmp->addr);
//	tasklog('o', "Load node %s rc=%s, NoCall=%s, NoTCP=%s", ascfnode(tmp->addr, 0x0f), rc?"true":"false",
//		    nodes.NoCall?"True":"False", nodes.NoTCP?"True":"False");

	/*
	 * Zone Mail Hours, only use Fidonet Hours.
	 * Other nets use your default ZMH.
	 */
	T_window = iszmh = FALSE;
	switch (tmp->addr.zone) {
	    case 1:	if (uhour == 9)
			    iszmh = TRUE;
			set_next(9, 0);
			set_next(10, 0);
			break;
	    case 2:	if (((uhour == 2) && (umin >= 30)) || ((uhour == 3) && (umin < 30)))
			    iszmh = TRUE;
			set_next(2, 30);
			set_next(3, 30);
			break;
	    case 3:	if (uhour == 18)
			    iszmh = TRUE;
			set_next(18, 0);
			set_next(19, 0);
			break;
	    case 4:	if (uhour == 8)
			    iszmh = TRUE;
			set_next(8, 0);
			set_next(9, 0);
			break;
	    case 5:	if (uhour == 1)
			    iszmh = TRUE;
			set_next(1, 0);
			set_next(2, 0);
			break;
	    case 6:	if (uhour == 20)
			    iszmh = TRUE;
			set_next(20, 0);
			set_next(21, 0);
			break;
	    default:	if (get_zmh())
			    iszmh = TRUE;
			break;
	}

	if (tmp->t1 && tmp->t2) {
	    /*
	     * Txx flags, check callwindow
	     */
	    thour = toupper(tmp->t1) - 'A';
	    if (isupper(tmp->t1))
		tmin = 0;
	    else
		tmin = 30;
	    sprintf(as, "%02d:%02d", thour, tmin);
	    set_next(thour, tmin);
	    thour = toupper(tmp->t2) - 'A';
	    if (isupper(tmp->t2))
		tmin = 0;
	    else
		tmin = 30;
	    sprintf(be, "%02d:%02d", thour, tmin);
	    set_next(thour, tmin);
	    if (strcmp(as, be) > 0) {
		/*
		 * Time window is passing midnight
		 */
		if ((strcmp(utc, as) >= 0) || (strcmp(utc, be) < 0))
		    T_window = TRUE;
	    } else {
		/*
		 * Time window is not passing midnight
		 */
		if ((strcmp(utc, as) >= 0) && (strcmp(utc, be) < 0))
		    T_window = TRUE;
	    }
	}
//	tasklog('o', "T_window=%s, iszmh=%s", T_window?"true":"false", iszmh?"true":"false");
	strcpy(flstr,"...... ... ..");

	/*
	 * If the node has internet and we have internet configured, 
	 * check if we can send immediatly.
	 */
	if (TCFG.max_tcp && (tmp->olflags & OL_CM) &&
		(((tmp->flavors) & F_IMM) || ((tmp->flavors) & F_CRASH) || ((tmp->flavors) & F_NORMAL)) &&
		((tmp->ipflags & IP_IBN) || (tmp->ipflags & IP_IFC) || (tmp->ipflags & IP_ITN))) {
	    tmp->flavors |= F_CALL;
	}

	if ((tmp->flavors) & F_IMM) {
	    flstr[0]='I';
	    /*
	     * Immediate mail, send if node is CM or is in a Txx window or is in ZMH.
	     */
	    if ((tmp->olflags & OL_CM) || T_window || iszmh) {
		tmp->flavors |= F_CALL;
	    }
	}

	if ((tmp->flavors) & F_CRASH ) {
	    flstr[1]='C';
	    /*
	     * Crash mail, send if node is CM or is in a Txx window or is in ZMH.
	     */
	    if ((tmp->olflags & OL_CM) || T_window || iszmh) {
		tmp->flavors |= F_CALL;
	    }
	}

	if ((tmp->flavors) & F_NORMAL)
	    flstr[2]='N';
	if ((tmp->flavors) & F_HOLD  ) 
	    flstr[3]='H';
	if ((tmp->flavors) & F_FREQ  ) 
	    flstr[4]='R';
	if ((tmp->flavors) & F_POLL  ) {
	    flstr[5]='P';
	    tmp->flavors |= F_CALL;
	}
	if ((tmp->flavors) & F_ISPKT ) { 
	    flstr[7]='M';
	    /*
	     * Normal mail, send during ZMH or if node has a Txx window.
	     */
	    if (iszmh || T_window) {
		tmp->flavors |= F_CALL;
	    }
	}
	if ((tmp->flavors) & F_ISFLO ) 
	    flstr[8]='F';

	if (tmp->cst.tryno >= 30) {
	    /*
	     * Node is undialable, clear callflag
	     */
	    tmp->flavors &= ~F_CALL;
	}
	if ((tmp->flavors) & F_CALL) 
	    flstr[9]='C';
	if (tmp->t1) 
	    flstr[11] = tmp->t1;
	if (tmp->t2) 
	    flstr[12] = tmp->t2;

	/*
	 * If forbidden to call from setup, clear callflag.
	 */
	if (nodes.NoCall)
	    tmp->flavors &= ~F_CALL;

	/*
	 * If we must call this node, figure out how to call this node.
	 */
	if ((tmp->flavors) & F_CALL) {
	    tmp->callmode = CM_NONE;

	    if (TCFG.max_tcp && !nodes.NoTCP &&
		    ((tmp->ipflags & IP_IBN) || (tmp->ipflags & IP_IFC) || (tmp->ipflags & IP_ITN))) {
		inet_calls++;
		tmp->callmode = CM_INET;
	    }

	    if ((tmp->callmode == CM_NONE) && TCFG.max_isdn) {
		/*
		 * ISDN node
		 */
		isdn_calls++;
		tmp->callmode = CM_ISDN;
		break;
	    }

	    if ((tmp->callmode == CM_NONE) && TCFG.max_pots) {
		/*
		 * POTS node
		 */
		pots_calls++;
		tmp->callmode = CM_POTS;
		break;
	    }
	}
	
	/*
	 * Show callresult for this node.
	 */
	sprintf(temp, "%s %8s %08x %08x %08x %08x %5d %s %s %s", flstr, size_str(tmp->size),
		(unsigned int)tmp->olflags, (unsigned int)tmp->moflags,
		(unsigned int)tmp->diflags, (unsigned int)tmp->ipflags,
		tmp->cst.tryno, callstatus(tmp->cst.trystat), callmode(tmp->callmode), ascfnode(tmp->addr, 0x0f));
	tasklog('+', "%s", temp);

    } /* All nodes scanned. */
	
    if (nxt_hour == 24) {
	/*
	 * 24:00 hours doesn't exist
	 */
	nxt_hour = 0;
	nxt_min  = 0;
    }

    /*
     * Always set/reset semafore do_inet if internet is needed.
     */
    if (!IsSema((char *)"do_inet") && inet_calls) {
	CreateSema((char *)"do_inet");
	s_do_inet = TRUE;
	tasklog('c', "Created semafore do_inet");
    } else if (IsSema((char *)"do_inet") && !inet_calls) {
	RemoveSema((char *)"do_inet");
	s_do_inet = FALSE;
	tasklog('c', "Removed semafore do_inet");
    }

    /*
     * Log results
     */
    tasklog('+', "Inet=%d, ISDN=%d, POTS=%d, Next event at %02d:%02d UTC", 
	    inet_calls, isdn_calls, pots_calls, nxt_hour, nxt_min);
    return 0;
}



int each(faddr *addr, char flavor, int isflo, char *fname)
{
	struct		_alist **tmp;
	struct		stat st;
	FILE		*fp;
	char		buf[256], *p;
	node		*nlent;
	callstat	*cst;

	if ((isflo != OUT_PKT) && (isflo != OUT_FLO) && (isflo != OUT_REQ) && (isflo != OUT_POL))
		return 0;

	for (tmp = &alist; *tmp; tmp = &((*tmp)->next))
		if (((*tmp)->addr.zone  == addr->zone) && ((*tmp)->addr.net   == addr->net) &&
		    ((*tmp)->addr.node  == addr->node) && ((*tmp)->addr.point == addr->point) &&
		    (((*tmp)->addr.domain == NULL) || (addr->domain == NULL) ||
		     (strcasecmp((*tmp)->addr.domain,addr->domain) == 0)))
			break;
	if (*tmp == NULL) {
		nlent = getnlent(addr);
		*tmp = (struct _alist *)malloc(sizeof(struct _alist));
		(*tmp)->next = NULL;
		(*tmp)->addr.zone   = addr->zone;
		(*tmp)->addr.net    = addr->net;
		(*tmp)->addr.node   = addr->node;
		(*tmp)->addr.point  = addr->point;
		sprintf((*tmp)->addr.domain, "%s", addr->domain);
		if (nlent->addr.domain)
			free(nlent->addr.domain);
		(*tmp)->flavors = 0;
		if (nlent->pflag != NL_DUMMY) {
			(*tmp)->olflags = nlent->oflags;
			(*tmp)->moflags = nlent->mflags;
			(*tmp)->diflags = nlent->dflags;
			(*tmp)->ipflags = nlent->iflags;
			(*tmp)->t1 = nlent->t1;
			(*tmp)->t2 = nlent->t2;
		} else {
			(*tmp)->olflags = 0L;
			(*tmp)->moflags = 0L;
			(*tmp)->diflags = 0L;
			(*tmp)->ipflags = 0L;
			(*tmp)->t1 = '\0';
			(*tmp)->t2 = '\0';
		}
		(*tmp)->time = time(NULL);
		(*tmp)->size = 0L;
	}

	cst = getstatus(addr);
	(*tmp)->cst.trytime = cst->trytime;
	(*tmp)->cst.tryno   = cst->tryno;
	(*tmp)->cst.trystat = cst->trystat;

	if ((isflo == OUT_FLO) || (isflo == OUT_PKT)) 
		switch (flavor) {
			case '?':	break;
			case 'i':	(*tmp)->flavors |= F_IMM; break;
			case 'o':	(*tmp)->flavors |= F_NORMAL; break;
			case 'c':	(*tmp)->flavors |= F_CRASH; break;
			case 'h':	(*tmp)->flavors |= F_HOLD; break;
			default:	tasklog('?', "Unknown flavor: '%c'\n",flavor); break;
		}

	if (stat(fname,&st) != 0) {
		tasklog('?', "$Can't stat %s", fname);
		st.st_size  = 0L;
		st.st_mtime = time(NULL);
	}

	/*
	 * Find the oldest time
	 */
	if (st.st_mtime < (*tmp)->time) 
		(*tmp)->time = st.st_mtime;

	if (isflo == OUT_FLO) {
		(*tmp)->flavors |= F_ISFLO;
		if ((fp = fopen(fname,"r"))) {
			while (fgets(buf, sizeof(buf) - 1, fp)) {
				if (*(p = buf + strlen(buf) - 1) == '\n') 
					*p-- = '\0';
				while (isspace(*p)) 
					*p-- = '\0';
				for (p = buf; *p; p++) 
					if (*p == '\\') 
						*p='/';
				for (p = buf; *p && isspace(*p); p++);
				if (*p == '~') continue;
				if ((*p == '#') || (*p == '-') || (*p == '^') || (*p == '@')) 
					p++;
				if (stat(p, &st) != 0) {
					if (strlen(CFG.dospath)) {
						if (stat(Dos2Unix(p), &st) != 0) {
							/*
							 * Fileattach dissapeared, maybe
							 * the node doesn't poll enough and
							 * is losing mail or files.
							 */
							st.st_size  = 0L;
							st.st_mtime = time(NULL);
						}
					} else {
						if (stat(p, &st) != 0) {
							st.st_size  = 0L;
							st.st_mtime = time(NULL);
						}
					}
				}

				if ((p = strrchr(fname,'/'))) 
					p++;
				else 
					p = fname;
				if ((strlen(p) == 12) && (strspn(p,"0123456789abcdefABCDEF") == 8) && (p[8] == '.')) {
					if (st.st_mtime < (*tmp)->time) 
						(*tmp)->time = st.st_mtime;
				}
				(*tmp)->size += st.st_size;
			}
			fclose(fp);
		} else 
			tasklog('?', "Can't open %s", fname);

	} else if (isflo == OUT_PKT) {
		(*tmp)->size += st.st_size;
		(*tmp)->flavors |= F_ISPKT;
	} else if (isflo == OUT_REQ) {
		(*tmp)->flavors |= F_FREQ;
	} else if (isflo == OUT_POL) {
		(*tmp)->flavors |= F_POLL;
	}

	return 0;
}


