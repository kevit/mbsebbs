/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: AreaMgr and FileMgr utilities.
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
#include "../lib/structs.h"
#include "../lib/users.h"
#include "../lib/records.h"
#include "../lib/common.h"
#include "../lib/clcomm.h"
#include "../lib/dbnode.h"
#include "../lib/diesel.h"
#include "sendmail.h"
#include "rollover.h"
#include "addpkt.h"
#include "createm.h"
#include "createf.h"
#include "mgrutil.h"


extern int	net_out;
extern int	do_quiet;
extern int	do_flush;


void tidy_arealist(AreaList **);
void fill_arealist(AreaList **, char *, int);



void MacroRead(FILE *fi, FILE *fp)
{
    char    *line, *temp;
    int	    res;

    line = calloc(256, sizeof(char));
    temp = calloc(256, sizeof(char));

    while ((fgets(line, 254, fi) != NULL) && ((line[0]!='@') || (line[1]!='|'))) {
	/*
	 * Skip comment lines
	 */
	if (line[0] != '#') {
	    Striplf(line);
	    if (strlen(line) == 0) {
		/*
		 * Empty lines are just written
		 */
		fprintf(fp, "\r");
	    } else {
		strncpy(temp, ParseMacro(line,&res), 254);
		if (res)
		    Syslog('!', "Macro error line: \"%s\"", line);
		/*
		 * Only output if something was evaluated
		 */
		if (strlen(temp))
		    fprintf(fp, "%s\r", temp);
	    }
	}
    }
    free(line);
    free(temp);
}



/*
 * Write Echomail groups list to tempfile
 */
void WriteMailGroups(FILE *fp, faddr *f)
{
    int	    Count = 0, First = TRUE;
    char    *Group, *temp;
    FILE    *gp,*fi;
    faddr   *g, *Temp;
    fpos_t  fileptr;

    if ((fi = OpenMacro("areamgr.group", nodes.Language, FALSE)) == NULL)
	return;

    MacroRead(fi, fp);
    fgetpos(fi,&fileptr);

    temp = calloc(PATH_MAX, sizeof(char));
    sprintf(temp, "%s/etc/mgroups.data", getenv("MBSE_ROOT"));

    if ((gp = fopen(temp, "r")) == NULL) {
	WriteError("$Can't open %s", temp);
	free(temp);
	fclose(fi);
	return;
    }
    fread(&mgrouphdr, sizeof(mgrouphdr), 1, gp);

    while (TRUE) {
	Group = GetNodeMailGrp(First);
	if (Group == NULL)
	    break;
	First = FALSE;

	fseek(gp, mgrouphdr.hdrsize, SEEK_SET);
	while (fread(&mgroup, mgrouphdr.recsize, 1, gp) == 1) {
	    Temp = fido2faddr(mgroup.UseAka);
	    g = bestaka_s(Temp);
	    if ((!strcmp(mgroup.Name, Group)) && 
		    (g->zone  == f->zone) && (g->net   == f->net) && (g->node  == f->node) && (g->point == f->point)) {
		MacroVars("gh", "ss", mgroup.Name, mgroup.Comment);
		fsetpos(fi, &fileptr);
		MacroRead(fi, fp);
		Count++;
		break;
	    }
	    tidy_faddr(Temp);
	}
    }

    MacroVars("b", "d", Count);
    MacroRead(fi, fp);
    fclose(fi);
    fclose(gp);
    free(temp);
}



/*
 * Write ticarea groups to tempfile
 */
void WriteFileGroups(FILE *fp, faddr *f)
{
    int	    Count = 0, First = TRUE;
    char    *Group, *temp;
    FILE    *gp, *fi;
    faddr   *g, *Temp;
    fpos_t  fileptr;

    if ((fi = OpenMacro("filemgr.group", nodes.Language, FALSE)) == NULL)
	return;

    MacroRead(fi, fp);
    fgetpos(fi,&fileptr);

    temp = calloc(128, sizeof(char));
    sprintf(temp, "%s/etc/fgroups.data", getenv("MBSE_ROOT"));
	
    if ((gp = fopen(temp, "r")) == NULL) {
	WriteError("$Can't open %s", temp);
	free(temp);
	fclose(fi);
	return;
    }
    fread(&fgrouphdr, sizeof(fgrouphdr), 1, gp);

    while (TRUE) {
	Group = GetNodeFileGrp(First);
	if (Group == NULL)
	    break;
	First = FALSE;

	fseek(gp, fgrouphdr.hdrsize, SEEK_SET);
	while (fread(&fgroup, fgrouphdr.recsize, 1, gp) == 1) {
	    Temp = fido2faddr(fgroup.UseAka);
	    g = bestaka_s(Temp);
	    if ((!strcmp(fgroup.Name, Group)) && 
		(g->zone  == f->zone) && (g->net   == f->net) && (g->node  == f->node) && (g->point == f->point)) {
 	        MacroVars("gh", "ss", fgroup.Name, fgroup.Comment );
		fsetpos(fi,&fileptr);
		MacroRead(fi, fp);
		Count++;
		break;
	    }
	    tidy_faddr(Temp);
	}
    }

    MacroVars("b", "d", Count );
    MacroRead(fi, fp);
    fclose(fi);
    fclose(gp);
    free(temp);
}



/*
 * Shift all characters in Buf Cnt places to left
 */
void ShiftBuf(char *Buf, int Cnt)
{
	int	i;

	for (i = Cnt; i < strlen(Buf); i++)
		Buf[i - Cnt] = Buf[i];
	Buf[i - Cnt] = '\0';
}



/*
 * Remove spaces and = characters from begin of line
 */
void CleanBuf(char *Buf)
{
	while (strlen(Buf) && ((Buf[0] == ' ') || (Buf[0] == '=')))
		ShiftBuf(Buf, 1);
}



/*
 * Change AreaMgr and FileMgr password for a node
 */
void MgrPasswd(faddr *t, char *Buf, FILE *tmp, int Len, int mgr)
{
	ShiftBuf(Buf, Len);
	CleanBuf(Buf);
	MacroVars("SsP", "sss", CFG.sysop_name, nodes.Sysop, mgr?(char *)"Filemgr":(char *)"Areamgr");
	if ((strlen(Buf) < 3) || (strlen(Buf) > 15)) {
 	    MacroVars("RABCDE", "ssssss",(char *)"ERR_PASS_LEN",(char *)"",(char *)"",(char *)"",(char *)"",(char *)"");
	    MsgResult(mgr?"filemgr.responses":"areamgr.responses",tmp);
	    Mgrlog("%s: Password length %d, not changed", mgr?(char *)"Filemgr":(char *)"Areamgr", strlen(Buf));
	    return;
	}

	memset(&nodes.Apasswd, 0, sizeof(nodes.Apasswd));
	strncpy(nodes.Apasswd, tu(Buf), 15);
	MacroVars("RABCDE", "ssssss",(char *)"OK_PASS",nodes.Apasswd,(char *)"",(char *)"",(char *)"",(char *)"");
	MsgResult(mgr?"filemgr.responses":"areamgr.responses",tmp);
	Mgrlog("%s: Password \"%s\" for node %s", mgr?(char *)"Filemgr":(char *)"Areamgr", nodes.Apasswd, ascfnode(t, 0x1f));
        MacroClear();
	UpdateNode();
	SearchNodeFaddr(t);
}



/*
 * Change AreaMgr/FileMgr nodify flag for node
 */
void MgrNotify(faddr *t, char *Buf, FILE *tmp, int mgr)
{
	/*
	 *  First strip leading garbage
	 */
	ShiftBuf(Buf, 7);
	CleanBuf(Buf);

	if (!strncasecmp(Buf, "on", 2))
		nodes.Notify = TRUE;
	else if (!strncasecmp(Buf, "off", 3))
		nodes.Notify = FALSE;
	else
		return;

	UpdateNode();
	SearchNodeFaddr(t);
	Mgrlog("%s: Notify %s", mgr?(char *)"Filemgr":(char *)"Areamgr", nodes.Notify?"Yes":"No");
	MacroVars("SsP", "sss", CFG.sysop_name, nodes.Sysop,mgr?(char *)"Filemgr":(char *)"Areamgr");
	MacroVars("RABCDE", "sdssss",(char *)"NOTIFY",nodes.Notify,(char *)"",(char *)"",(char *)"",(char *)"");
	MsgResult(mgr?"filemgr.responses":"areamgr.responses",tmp);
	MacroClear();
}



/*
 * Create uplink areamgr request. One netmail per request.
 * More is possible, cmd is then: "+area1\r+area2\r-area3"
 * Return values:
 *  0   - Ok
 *  1   - Node not in setup
 *  2   - No Uplink mgr program in setup
 *  3   - No uplink password in setup
 *  4   - Can't add mail to outbound
 */
int UplinkRequest(faddr *t, faddr *From, int FileMgr, char *cmd)
{
    FILE	*qp;
    time_t	Now;
    struct tm	*tm;
    fidoaddr	Orig, Dest;
    unsigned	flags = M_PVT;
    char	ext[4], *mgrname, *subj;
    int		i;

    memset(&Orig, 0, sizeof(Orig));
    Orig.zone  = From->zone;
    Orig.net   = From->net;
    Orig.node  = From->node;
    Orig.point = From->point;
    sprintf(Orig.domain, "%s", From->domain);

    memset(&Dest, 0, sizeof(Dest));
    Dest.zone  = t->zone;
    Dest.net   = t->net;
    Dest.node  = t->node;
    Dest.point = t->point;
    sprintf(Dest.domain, "%s", t->domain);

    if (!SearchNode(Dest)) {
	Syslog('+', "Can't find node %s in setup", aka2str(Dest));
	return 1;
    }

    if (FileMgr) {
	if (strlen(nodes.UplFmgrPgm) == 0) {
	    Syslog('!', "FileMgr program not defined in setup of node %s", aka2str(Dest));
	    return 2;
	}
	mgrname = xstrcpy(nodes.UplFmgrPgm);
    } else {
	if (strlen(nodes.UplAmgrPgm) == 0) {
	    Syslog('!', "AreaMgr program not defined in setup of node %s", aka2str(Dest));
	    return 2;
	}
	mgrname = xstrcpy(nodes.UplAmgrPgm);
    }

    if (strlen(nodes.Apasswd) == 0) {
	Syslog('!', "No %s password set for node %s", mgrname, aka2str(Dest));
	return 3;
    }
    subj = xstrcpy(nodes.Apasswd);

    Syslog('-', "  Netmail from %s to %s", aka2str(Orig), ascfnode(t, 0x1f));

    Now = time(NULL) - (gmt_offset((time_t)0) * 60);
    flags |= (nodes.Crash)  ? M_CRASH    : 0;
    flags |= (nodes.Hold)   ? M_HOLD     : 0;

    /*
     *  Increase counters, update record and reload.
     */
    StatAdd(&nodes.MailSent, 1L);
    UpdateNode();
    SearchNode(Dest);

    memset(&ext, 0, sizeof(ext));
    if (nodes.PackNetmail)
	sprintf(ext, (char *)"qqq");
    else if (nodes.Crash)
	sprintf(ext, (char *)"ccc");
    else if (nodes.Hold)
	sprintf(ext, (char *)"hhh");
    else
	sprintf(ext, (char *)"nnn");

    if ((qp = OpenPkt(Orig, Dest, (char *)ext)) == NULL)
	return 4;

    if (AddMsgHdr(qp, From, t, flags, 0, Now, mgrname, CFG.sysop_name, subj)) {
	fclose(qp);
	return 4;
    }

    if (Dest.point)
	fprintf(qp, "\001TOPT %d\r", Dest.point);
    if (Orig.point)
	fprintf(qp, "\001FMPT %d\r", Orig.point);

    fprintf(qp, "\001INTL %d:%d/%d %d:%d/%d\r", Dest.zone, Dest.net, Dest.node, Orig.zone, Orig.net, Orig.node);

    /*
     * Add MSGID, REPLY and PID
     */
    fprintf(qp, "\001MSGID: %s %08lx\r", aka2str(Orig), sequencer());
    fprintf(qp, "\001PID: MBSE-FIDO %s\r", VERSION);
    fprintf(qp, "\001TZUTC: %s\r", gmtoffset(Now));

    for (i = 0; i < strlen(cmd); i++)
	putc(cmd[i], qp);
    putc('\r', qp);
    fprintf(qp, TearLine());
    /*
     * Add a warning after the tearline.
     */
    fprintf(qp, "Please note, this is an automatic created message\r");

    tm = gmtime(&Now);
    fprintf(qp, "\001Via %s @%d%02d%02d.%02d%02d%02d.02.UTC %s\r",
		ascfnode(bestaka_s(t), 0x1f), tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday, 
		tm->tm_hour, tm->tm_min, tm->tm_sec, VERSION);

    putc(0, qp);
    fclose(qp);

    free(mgrname);
    free(subj);
    net_out++;
    return 0;
}



void GetRpSubject(const char *report, char* subject)
{
    FILE    *fi;
    char    *temp;
    int	    res;

    temp = calloc(256,sizeof(char)); 
    if ((fi=OpenMacro(report, nodes.Language, FALSE))!=NULL){
       while ( fgets(temp, 254, fi) != NULL )
	   if (temp[0] != '#')
          	ParseMacro(temp,&res);
       fclose(fi);
    }
    
    res=diesel((char *)"@(getvar,subject)",temp);
    
    if(res==0)
    	sprintf(subject,"%s",temp);
    free(temp);
}



int MsgResult(const char * report, FILE *fo)
{
    FILE    *fi;
    char    *temp, *resp;
    int	    res;

    temp = calloc(256,sizeof(char)); 
    resp = calloc(256,sizeof(char));

    if ((fi = OpenMacro(report, nodes.Language, FALSE)) != NULL){
        while ( fgets(temp, 254, fi) != NULL ){
	    if (temp[0] != '#') {
		strncpy(resp, ParseMacro(temp, &res), 80);
		if ((res == 0) && strlen(resp))
		    fprintf(fo,"%s\r",ParseMacro(temp,&res));
	    }
	}
        fclose(fi);
        res=1;
    } else {
        res = 0;
    }

    free(resp);
    free(temp);
    return res;
}



void tidy_arealist(AreaList **fdp)
{
    AreaList	*tmp, *old;

    for (tmp = *fdp; tmp; tmp = old) {
	old = tmp->next;
	free(tmp);
    }
    *fdp = NULL;
}

    

void fill_arealist(AreaList **fdp, char *tag, int DoDelete)
{
    AreaList	**tmp;

    for (tmp = fdp; *tmp; tmp = &((*tmp)->next));
    *tmp = (AreaList *)malloc(sizeof(AreaList));
    (*tmp)->next = NULL;
    strcpy((*tmp)->Name, tag);
    (*tmp)->IsPresent = FALSE;
    (*tmp)->DoDelete = DoDelete;
}



int Areas(void)
{
    FILE	*gp, *ap, *fp;
    char	*temp, *buf, *tag, *desc, *p, *cmd = NULL;
    AreaList	*alist = NULL, *tmp;
    int		i, count = 0, Found;
    sysconnect	System;
    faddr	*From, *To;

    Mgrlog("Process areas taglists");

    if (!do_quiet) {
	colour(3, 0);
	printf("Processing areas taglists...\n");
    }

    temp = calloc(PATH_MAX, sizeof(char));
    buf  = calloc(4097, sizeof(char));

    sprintf(temp, "%s/etc/mgroups.data", getenv("MBSE_ROOT"));
    if ((gp = fopen(temp, "r")) == NULL) {
	WriteError("Can't open %s", temp);
    } else {
	fread(&mgrouphdr, sizeof(mgrouphdr), 1, gp);
	fseek(gp, mgrouphdr.hdrsize, SEEK_SET);

	while ((fread(&mgroup, mgrouphdr.recsize, 1, gp)) == 1) {
	    if (mgroup.Active && mgroup.AutoChange && strlen(mgroup.AreaFile)) {
		if (!do_quiet) {
		    colour(CYAN, BLACK);
		    printf("\rEcho group %-12s ", mgroup.Name);
		    fflush(stdout);
		}
		Syslog('+', "Checking mail group %s, file %s", mgroup.Name, mgroup.AreaFile);
		sprintf(temp, "%s/%s", CFG.alists_path, mgroup.AreaFile);
		if ((ap = fopen(temp, "r")) == NULL) {
		    WriteError("Can't open %s", temp);
		} else {
		    while (fgets(buf, 4096, ap)) {
			if (strlen(buf) && isalnum(buf[0])) {
			    tag = strtok(buf, "\t \r\n\0");
			    fill_arealist(&alist, tag, FALSE);
			}
		    }
		    fclose(ap);

		    /*
		     * Mark areas already present in the taglist.
		     */
		    if (!do_quiet) {
			colour(LIGHTRED, BLACK);
			printf("(check missing areas)\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
			fflush(stdout);
		    }
		    sprintf(temp, "%s/etc/mareas.data", getenv("MBSE_ROOT"));
		    if ((fp = fopen(temp, "r")) == NULL) {
			WriteError("Can't open %s", temp);
			tidy_arealist(&alist);
			free(buf);
			free(temp);
			return FALSE;
		    }
		    fread(&msgshdr, sizeof(msgshdr), 1, fp);
		    for (tmp = alist; tmp; tmp = tmp->next) {
			fseek(fp, msgshdr.hdrsize, SEEK_SET);
			if (CFG.slow_util && do_quiet)
			    usleep(1);
			while (fread(&msgs, msgshdr.recsize, 1, fp) == 1) {
			    if (msgs.Active && !strcmp(msgs.Group, mgroup.Name) && !strcmp(msgs.Tag, tmp->Name))
				tmp->IsPresent = TRUE;
			    fseek(fp, msgshdr.syssize, SEEK_CUR);
			}
		    }

		    /*
		     * Add areas to AreaList not in the taglist, they must be deleted.
		     */
		    if (!do_quiet) {
			printf("(check deleted areas)\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
			fflush(stdout);
		    }
		    fseek(fp, msgshdr.hdrsize, SEEK_SET);
		    while (fread(&msgs, msgshdr.recsize, 1, fp) == 1) {
			if (msgs.Active && !strcmp(msgs.Group, mgroup.Name)) {
			    Found = FALSE;
			    for (tmp = alist; tmp; tmp = tmp->next) {
				if (!strcmp(msgs.Tag, tmp->Name))
				    Found = TRUE;
			    }
			    if (!Found)
				fill_arealist(&alist, msgs.Tag, TRUE);
			}
			fseek(fp, msgshdr.syssize, SEEK_CUR);
		    }
		    fclose(fp);
		    if (!do_quiet) {
			printf("(update database)    \b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
			fflush(stdout);
		    }
		    
		    /*
		     * Make modification, first add missing areas
		     */
		    for (tmp = alist; tmp; tmp = tmp->next) {
			if (!tmp->IsPresent && !tmp->DoDelete) {
			    /*
			     * Autocraete group, don't sent uplink request yet.
			     */
			    CheckEchoGroup(tmp->Name, FALSE, NULL);
			    if (cmd == NULL) {
				cmd = xstrcpy((char *)"+");
			    } else {
				cmd = xstrcat(cmd, (char *)"\r+");
			    }
			    cmd = xstrcat(cmd, tmp->Name);
			    if (CFG.slow_util && do_quiet)
				usleep(1);
			}
		    }

		    /*
		     * Now remove deleted areas. If there are messages in the area,
		     * the area is set to read-only and all links are disconnected.
		     * If the area is empty, it is removed from the setup.
		     */
		    sprintf(temp, "%s/etc/mareas.data", getenv("MBSE_ROOT"));
		    if ((fp = fopen(temp, "r+")) == NULL) {
			WriteError("Can't open %s for r/w");
		    } else {
			fread(&msgshdr, sizeof(msgshdr), 1, fp);
			for (tmp = alist; tmp; tmp = tmp->next) {
			    if (!tmp->IsPresent && tmp->DoDelete) {
				fseek(fp, msgshdr.hdrsize, SEEK_SET);
				Syslog('m', "Delete %s", tmp->Name);
				if (CFG.slow_util && do_quiet)
				    usleep(1);
				while (fread(&msgs, msgshdr.recsize, 1, fp) == 1) {
				    if (msgs.Active && !strcmp(msgs.Group, mgroup.Name) && !strcmp(msgs.Tag, tmp->Name)) {
					fseek(fp, - msgshdr.recsize, SEEK_CUR);
					sprintf(temp, "%s.jhr", msgs.Base);
					if (strlen(msgs.Base) && (file_size(temp) != 1024)) {
					    Mgrlog("Marking message area %d, %s read-only",
						    ((ftell(fp) - msgshdr.hdrsize) / (msgshdr.recsize + msgshdr.syssize)) + 1,
						    msgs.Tag);
					    msgs.MsgKinds = RONLY;          // Area read-only
					    sprintf(msgs.Group, "DELETED"); // Make groupname invalid
					} else {
					    Mgrlog("Removing empty message area %d, %s", 
						((ftell(fp) - msgshdr.hdrsize) / (msgshdr.recsize + msgshdr.syssize)) + 1, 
						msgs.Tag);
					    memset(&msgs, 0, sizeof(msgs));
					    msgs.DaysOld = CFG.defdays;
					    msgs.MaxMsgs = CFG.defmsgs;
					    msgs.Type = ECHOMAIL;
					    msgs.MsgKinds = PUBLIC;
					    msgs.UsrDelete = TRUE;
					    msgs.Rfccode = CHRS_DEFAULT_RFC;
					    msgs.Ftncode = CHRS_DEFAULT_FTN;
					    msgs.MaxArticles = CFG.maxarticles;
					    strcpy(msgs.Origin, CFG.origin);
					}
					fwrite(&msgs, msgshdr.recsize, 1, fp);
					/*
					 * Always clear all connections, the area doesn't exist anymore.
					 */
					memset(&System, 0, sizeof(System));
					for (i = 0; i < (msgshdr.syssize / sizeof(sysconnect)); i++)
					    fwrite(&System, sizeof(system), 1, fp);
					/*
					 * Prepare uplink command
					 */
					if (cmd == NULL)
					    cmd = xstrcpy((char *)"-");
					else
					    cmd = xstrcat(cmd, (char *)"\r-");
					cmd = xstrcat(cmd, tmp->Name);
					break;
				    } else {
					fseek(fp, msgshdr.syssize, SEEK_CUR);
				    }
				}
			    }
			}
			fclose(fp);
		    }
		    tidy_arealist(&alist);
		    if (cmd != NULL) {
			/*
			 * Sent one uplink command with additions and deletions
			 */
			if (mgroup.UpLink.zone) {
			    From = fido2faddr(mgroup.UseAka);
			    To   = fido2faddr(mgroup.UpLink);
			    if (UplinkRequest(To, From, FALSE, cmd)) {
				WriteError("Uplink request failed");
			    } else {
				Mgrlog("AreaMgr request sent to %s", aka2str(mgroup.UpLink));
			    }
			    tidy_faddr(From);
			    tidy_faddr(To);
			} else {
			    Syslog('+', "No uplink defined, not sending a AreaMgr request");
			}
			free(cmd);
			cmd = NULL;
		    }
		    if (!do_quiet) {
			printf("                     \b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
			fflush(stdout);
		    }
		}
	    }
	}
	fclose(gp);
    }
    
    sprintf(temp, "%s/etc/fgroups.data", getenv("MBSE_ROOT"));
    if ((gp = fopen(temp, "r")) == NULL) {
	WriteError("Can't open %s", temp);
    } else {
	fread(&fgrouphdr, sizeof(fgrouphdr), 1, gp);
	fseek(gp, fgrouphdr.hdrsize, SEEK_SET);

	while ((fread(&fgroup, fgrouphdr.recsize, 1, gp)) == 1) {
	    if (fgroup.Active && fgroup.AutoChange && strlen(fgroup.AreaFile)) {
		if (!do_quiet) {
		    colour(CYAN, BLACK);
		    printf("\r TIC group %-12s ", fgroup.Name);
		    fflush(stdout);
		}
		Syslog('+', "Checking tic group %s, file %s", fgroup.Name, fgroup.AreaFile);
		sprintf(temp, "%s/%s", CFG.alists_path, fgroup.AreaFile);
		if ((ap = fopen(temp, "r")) == NULL) {
		    WriteError("Can't open %s", temp);
		} else {
		    if (fgroup.FileGate) {
			/*
			 * filegate.zxx format
			 */
			Found = FALSE;
			while (fgets(buf, 4096, ap)) {
			    /*
			     * Each group starts with % FDN:    FileGroup Descrition
			     */
			    if (strlen(buf) && !strncmp(buf, "% FDN:", 6)) {
				tag = strtok(buf, "\t \r\n\0");
				p = strtok(NULL, "\t \r\n\0");
				p = strtok(NULL, "\r\n\0");
				desc = p;
				while ((*desc == ' ') || (*desc == '\t'))
				    desc++;
				if (!strcmp(desc, fgroup.Comment)) {
				    Syslog('f', "Start of group \"%s\" found", desc);
				    while (fgets(buf, 4096, ap)) {
					if (!strncasecmp(buf, "Area ", 5)) {
					    Syslog('f', "Area: %s", buf);
					    tag = strtok(buf, "\t \r\n\0");
					    tag = strtok(NULL, "\t \r\n\0");
					    Found = TRUE;
					    fill_arealist(&alist, tag, FALSE);
					}
					if (strlen(buf) && !strncmp(buf, "% FDN:", 6)) {
					    /*
					     * All entries in group are seen, the area wasn't there.
					     */
					    Syslogp('f', buf);
					    break;
					}
				    }
				    if (Found)
					break;
				}
			    }
			}
		    } else {
			/*
			 * Normal taglist format
			 */
			while (fgets(buf, 4096, ap)) {
			    if (strlen(buf) && isalnum(buf[0])) {
				tag = strtok(buf, "\t \r\n\0");
				fill_arealist(&alist, tag, FALSE);
			    }
			}
		    }
		    fclose(ap);

                    /*
		     * Mark areas already present in the taglist.
		     */
		    if (!do_quiet) {
			colour(LIGHTRED, BLACK);
			printf("(check missing areas)\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
			fflush(stdout);
		    }
		    sprintf(temp, "%s/etc/tic.data", getenv("MBSE_ROOT"));
		    if ((fp = fopen(temp, "r")) == NULL) {
			WriteError("Can't open %s", temp);
			tidy_arealist(&alist);
			free(buf);
			free(temp);
			return FALSE;
		    }
		    fread(&tichdr, sizeof(tichdr), 1, fp);
		    for (tmp = alist; tmp; tmp = tmp->next) {
			fseek(fp, tichdr.hdrsize, SEEK_SET);
			if (CFG.slow_util && do_quiet)
			    usleep(1);
			while (fread(&tic, tichdr.recsize, 1, fp) == 1) {
			    if (tic.Active && !strcmp(tic.Group, fgroup.Name) && !strcmp(tic.Name, tmp->Name))
				tmp->IsPresent = TRUE;
			    fseek(fp, tichdr.syssize, SEEK_CUR);
			}
		    }

                    /*
		     * Add areas to AreaList not in the taglist, they must be deleted.
		     */
		    if (!do_quiet) {
			printf("(check deleted areas)\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
			fflush(stdout);
		    }
		    fseek(fp, tichdr.hdrsize, SEEK_SET);
		    while (fread(&tic, tichdr.recsize, 1, fp) == 1) {
			if (tic.Active && !strcmp(tic.Group, fgroup.Name)) {
			    Found = FALSE;
			    for (tmp = alist; tmp; tmp = tmp->next) {
				if (!strcmp(tic.Name, tmp->Name))
				    Found = TRUE;
			    }
			    if (!Found)
				fill_arealist(&alist, tic.Name, TRUE);
			}
			fseek(fp, tichdr.syssize, SEEK_CUR);
		    }
		    fclose(fp);
		    if (!do_quiet) {
			printf("(update database)    \b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
			fflush(stdout);
		    }

		    /*
		     * Now we have a list of actions to perform
		     */
		    Syslog('f', "Area tag             Oke Del");
		    for (tmp = alist; tmp; tmp = tmp->next) {
			Syslog('f', "%-20s %s %s", tmp->Name, tmp->IsPresent?"Yes":"No ", tmp->DoDelete?"Yes":"No ");
		    }
		    
		    /*
		     * Make modification, first add missing areas
		     */
		    for (tmp = alist; tmp; tmp = tmp->next) {
			if (!tmp->IsPresent && !tmp->DoDelete) {
                            /*
			     * Autocraete group, don't sent uplink request yet.
			     */
			    CheckTicGroup(tmp->Name, FALSE, NULL);
			    if (cmd == NULL) {
				cmd = xstrcpy((char *)"+");
			    } else {
				cmd = xstrcat(cmd, (char *)"\r+");
			    }
			    cmd = xstrcat(cmd, tmp->Name);
			    if (CFG.slow_util && do_quiet)
				usleep(1);
			}
		    }

		    /*
		     * Mark TIC areas for deletion. The original file areas
		     * are not deleted. They probably contain files and we
		     * may want to keep these. If the area was empty we are
		     * still warned about that by the "mbfile check" command.
		     */
		    Found = FALSE;
                    sprintf(temp, "%s/etc/tic.data", getenv("MBSE_ROOT"));
		    if ((fp = fopen(temp, "r+")) == NULL) { 
			WriteError("Can't open %s for r/w");
		    } else {
			fread(&tichdr, sizeof(tichdr), 1, fp);
			for (tmp = alist; tmp; tmp = tmp->next) {
			    if (!tmp->IsPresent && tmp->DoDelete) {
				fseek(fp, tichdr.hdrsize, SEEK_SET);
				Syslog('f', "Delete %s", tmp->Name);
				if (CFG.slow_util && do_quiet)
				    usleep(1);
				while (fread(&tic, tichdr.recsize, 1, fp) == 1) {
				    if (tic.Active && !strcmp(tic.Group, fgroup.Name) && !strcmp(tic.Name, tmp->Name)) {
					fseek(fp, - tichdr.recsize, SEEK_CUR);
					Syslog('+', "Marked TIC area %s for deletion", tmp->Name);
					tic.Deleted = TRUE;
					tic.Active  = FALSE;
					fwrite(&tic, tichdr.recsize, 1, fp);
					Found = TRUE;
                                        /*
					 * Prepare uplink command
					 */
					 if (cmd == NULL)
					    cmd = xstrcpy((char *)"-");
					else
					    cmd = xstrcat(cmd, (char *)"\r-");
					cmd = xstrcat(cmd, tmp->Name);
				    }
				    fseek(fp, tichdr.syssize, SEEK_CUR);
				}
			    }
			}
			fclose(fp);
		    }
		    if (Found) {
			/*
			 * Purge marked records
			 */
			sprintf(buf, "%s/etc/tic.temp", getenv("MBSE_ROOT"));
			if ((fp = fopen(temp, "r")) == NULL) {
			    WriteError("Can't open %s", temp);
			} else if ((ap = fopen(buf, "w")) == NULL) {
			    WriteError("Can't create %s", buf);
			    fclose(fp);
			} else {
			    fread(&tichdr, tichdr.hdrsize, 1, fp);
			    fwrite(&tichdr, tichdr.hdrsize, 1, ap);
			    while (fread(&tic, tichdr.recsize, 1, fp) == 1) {
				if (tic.Deleted && !tic.Active) {
				    fseek(fp, tichdr.syssize, SEEK_CUR);
				    count++;
				} else {
				    fwrite(&tic, tichdr.recsize, 1, ap);
				    for (i = 0; i < (tichdr.syssize / sizeof(System)); i++) {
					fread(&System, sizeof(System), 1, fp);
					fwrite(&System, sizeof(System), 1, ap);
				    }
				}
			    }
			    fclose(fp);
			    fclose(ap);
			    unlink(temp);
			    rename(buf, temp);
			    Mgrlog("Purged %d TIC records", count);
			}
		    }

		    tidy_arealist(&alist);
                    if (cmd != NULL) {
			/*
			 * Sent one uplink command with additions and deletions
			 */
			if (fgroup.UpLink.zone) {
			    From = fido2faddr(fgroup.UseAka);
			    To   = fido2faddr(fgroup.UpLink);
			    if (UplinkRequest(To, From, TRUE, cmd)) {
				WriteError("Uplink request failed");
			    } else {
				Mgrlog("AreaMgr request sent to %s", aka2str(fgroup.UpLink));
			    }
			    tidy_faddr(From);
			    tidy_faddr(To);
			} else {
			    Syslog('+', "No uplink defined, not sending a FileMgr request");
			}
			free(cmd);
			cmd = NULL;
		    }
		    if (!do_quiet) {
			printf("                     \b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
			fflush(stdout);
		    }
		}
	    }
	}
	fclose(gp);
    }

    if (!do_quiet)
	printf("\r                                                    \r");

    free(buf);
    free(temp);
    if (net_out)
	do_flush = TRUE;
    return TRUE;
}



