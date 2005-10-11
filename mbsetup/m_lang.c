/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Setup Languages.
 *
 *****************************************************************************
 * Copyright (C) 1997-2005
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
 * MB BBS is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with MB BBS; see the file COPYING.  If not, write to the Free
 * Software Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 *****************************************************************************/

#include "../config.h"
#include "../lib/mbselib.h"
#include "screen.h"
#include "mutil.h"
#include "ledit.h"
#include "stlist.h"
#include "m_global.h"
#include "m_lang.h"



int	LangUpdated = 0;


void AddLang(char *, char *, char *, FILE *);
void AddLang(char *Name, char *Key, char *Path, FILE *fil)
{
    memset(&lang, 0, sizeof(lang));
    snprintf(lang.Name,      30, "%s", Name);
    snprintf(lang.LangKey,    2, "%s", Key);
    snprintf(lang.MenuPath,  81, "%s/%s/menus", getenv("MBSE_ROOT"), Path);
    snprintf(lang.TextPath,  81, "%s/%s/txtfiles", getenv("MBSE_ROOT"), Path);
    snprintf(lang.MacroPath, 81, "%s/%s/macro", getenv("MBSE_ROOT"), Path);
    snprintf(lang.Filename,  81, "%s.lang", Path);
    lang.Available = TRUE;
    fwrite(&lang, sizeof(lang), 1, fil);
}



/*
 * Count nr of lang records in the database.
 * Creates the database if it doesn't exist.
 */
int CountLanguage(void)
{
    FILE    *fil;
    char    ffile[PATH_MAX];
    int	    count = 0;

    snprintf(ffile, PATH_MAX, "%s/etc/language.data", getenv("MBSE_ROOT"));
    if ((fil = fopen(ffile, "r")) == NULL) {
	if ((fil = fopen(ffile, "a+")) != NULL) {
	    Syslog('+', "Created new %s", ffile);
	    langhdr.hdrsize = sizeof(langhdr);
	    langhdr.recsize = sizeof(lang);
	    fwrite(&langhdr, sizeof(langhdr), 1, fil);

	    /*
	     *  Setup default records
	     */
	    AddLang((char *)"English",    (char *)"E", (char *)"english", fil);	count++;
	    AddLang((char *)"Nederlands", (char *)"N", (char *)"dutch",   fil);	count++;
	    AddLang((char *)"Spanish",    (char *)"S", (char *)"spanish", fil);	count++;
	    AddLang((char *)"Galego",     (char *)"G", (char *)"galego",  fil); count++;
	    AddLang((char *)"Deutsch",    (char *)"D", (char *)"german",  fil); count++;
	    AddLang((char *)"French",     (char *)"F", (char *)"french",  fil); count++;
	    AddLang((char *)"Chinese",    (char *)"C", (char *)"chinese", fil); count++;
	    
	    fclose(fil);
	    chmod(ffile, 0640);
	    return count;
	} else
	    return -1;
    }

    fread(&langhdr, sizeof(langhdr), 1, fil);
    fseek(fil, 0, SEEK_END);
    count = (ftell(fil) - langhdr.hdrsize) / langhdr.recsize;
    fclose(fil);

    return count;
}



/*
 * Open database for editing. The datafile is copied, if the format
 * is changed it will be converted on the fly. All editing must be 
 * done on the copied file.
 */
int OpenLanguage(void);
int OpenLanguage(void)
{
	FILE	*fin, *fout;
	char	fnin[PATH_MAX], fnout[PATH_MAX];
	int	oldsize;

	snprintf(fnin,  PATH_MAX, "%s/etc/language.data", getenv("MBSE_ROOT"));
	snprintf(fnout, PATH_MAX, "%s/etc/language.temp", getenv("MBSE_ROOT"));
	if ((fin = fopen(fnin, "r")) != NULL) {
		if ((fout = fopen(fnout, "w")) != NULL) {
			fread(&langhdr, sizeof(langhdr), 1, fin);
			/*
			 * In case we are automaic upgrading the data format
			 * we save the old format. If it is changed, the
			 * database must always be updated.
			 */
			oldsize = langhdr.recsize;
			if (oldsize != sizeof(lang)) {
				LangUpdated = 1;
				Syslog('+', "Updated %s, format changed", fnin);
			} else
				LangUpdated = 0;
			langhdr.hdrsize = sizeof(langhdr);
			langhdr.recsize = sizeof(lang);
			fwrite(&langhdr, sizeof(langhdr), 1, fout);

			/*
			 * The datarecord is filled with zero's before each
			 * read, so if the format changed, the new fields
			 * will be empty.
			 */
			memset(&lang, 0, sizeof(lang));
			while (fread(&lang, oldsize, 1, fin) == 1) {
				fwrite(&lang, sizeof(lang), 1, fout);
				memset(&lang, 0, sizeof(lang));
			}

			fclose(fin);
			fclose(fout);
			return 0;
		} else
			return -1;
	}
	return -1;
}



void CloseLanguage(int);
void CloseLanguage(int force)
{
	char	fin[PATH_MAX], fout[PATH_MAX];
	FILE	*fi, *fo;
	st_list	*lan = NULL, *tmp;

	snprintf(fin,  PATH_MAX, "%s/etc/language.data", getenv("MBSE_ROOT"));
	snprintf(fout, PATH_MAX, "%s/etc/language.temp", getenv("MBSE_ROOT"));

	if (LangUpdated == 1) {
		if (force || (yes_no((char *)"Database is changed, save changes") == 1)) {
			working(1, 0, 0);
			fi = fopen(fout, "r");
			fo = fopen(fin,  "w");
			fread(&langhdr, langhdr.hdrsize, 1, fi);
			fwrite(&langhdr, langhdr.hdrsize, 1, fo);

			while (fread(&lang, langhdr.recsize, 1, fi) == 1)
				if (!lang.Deleted)
					fill_stlist(&lan, lang.LangKey , ftell(fi) - langhdr.recsize);
			sort_stlist(&lan);

			for (tmp = lan; tmp; tmp = tmp->next) {
				fseek(fi, tmp->pos, SEEK_SET);
				fread(&lang, langhdr.recsize, 1, fi);
				fwrite(&lang, langhdr.recsize, 1, fo);
			}

			fclose(fi);
			fclose(fo);
			unlink(fout);
			tidy_stlist(&lan);
			chmod(fin, 0640);
			Syslog('+', "Updated \"language.data\"");
			disk_reset();
			if (!force)
			    working(6, 0, 0);
			return;
		}
	}
	chmod(fin, 0640);
	working(1, 0, 0);
	unlink(fout); 
}



int AppendLanguage(void)
{
	FILE	*fil;
	char	ffile[PATH_MAX];

	snprintf(ffile, PATH_MAX, "%s/etc/language.temp", getenv("MBSE_ROOT"));
	if ((fil = fopen(ffile, "a")) != NULL) {
		memset(&lang, 0, sizeof(lang));
		fwrite(&lang, sizeof(lang), 1, fil);
		fclose(fil);
		LangUpdated = 1;
		return 0;
	} else
		return -1;
}



void s_lang(void)
{
	clr_index();
	set_color(WHITE, BLACK);
	mbse_mvprintw( 5, 2, "8.2 EDIT LANGUAGE");
	set_color(CYAN, BLACK);
	mbse_mvprintw( 7, 2, "1.  Select");
	mbse_mvprintw( 8, 2, "2.  Name");
	mbse_mvprintw( 9, 2, "3.  Menupath");
	mbse_mvprintw(10, 2, "4.  Textpath");
	mbse_mvprintw(11, 2, "5.  Macropath");
	mbse_mvprintw(12, 2, "6.  Available");
	mbse_mvprintw(13, 2, "7.  Datafile");
	mbse_mvprintw(14, 2, "8.  Security");
	mbse_mvprintw(15, 2, "9.  Deleted");
}




/*
 * Edit one record, return -1 if there are errors, 0 if ok.
 */
int EditLangRec(int Area)
{
	FILE		*fil;
	char		mfile[PATH_MAX];
	int		offset;
	int		j;
	unsigned int	crc, crc1;

	clr_index();
	working(1, 0, 0);
	IsDoing("Edit Language");

	snprintf(mfile, PATH_MAX, "%s/etc/language.temp", getenv("MBSE_ROOT"));
	if ((fil = fopen(mfile, "r")) == NULL) {
		working(2, 0, 0);
		return -1;
	}

	offset = sizeof(langhdr) + ((Area -1) * sizeof(lang));
	if (fseek(fil, offset, 0) != 0) {
		working(2, 0, 0);
		return -1;
	}

	fread(&lang, sizeof(lang), 1, fil);
	fclose(fil);
	crc = 0xffffffff;
	crc = upd_crc32((char *)&lang, crc, sizeof(lang));

	s_lang();
	for (;;) {
		set_color(WHITE, BLACK);
		show_str(  7,16, 1, lang.LangKey);
		show_str(  8,16,30, lang.Name);
		show_str(  9,16,64, lang.MenuPath);
		show_str( 10,16,64, lang.TextPath);
		show_str( 11,16,64, lang.MacroPath);
		show_bool(12,16,    lang.Available);
		show_str( 13,16,24, lang.Filename);
		show_sec( 14,16,    lang.Security);
		show_bool(15,16,    lang.Deleted);

		j = select_menu(9);
		switch(j) {
		case 0:	crc1 = 0xffffffff;
			crc1 = upd_crc32((char *)&lang, crc1, sizeof(lang));
			if (crc != crc1) {
				if (yes_no((char *)"Record is changed, save") == 1) {
					working(1, 0, 0);
					if ((fil = fopen(mfile, "r+")) == NULL) {
						working(2, 0, 0);
						return -1;
					}
					fseek(fil, offset, 0);
					fwrite(&lang, sizeof(lang), 1, fil);
					fclose(fil);
					LangUpdated = 1;
					working(6, 0, 0);
				}
			}
			IsDoing("Browsing Menu");
			return 0;
		case 1:	E_UPS(  7,16,1, lang.LangKey,  "The ^Key^ to select this language")
		case 2:	E_STR(  8,16,30,lang.Name,     "The ^name^ of this language")
		case 3:	E_PTH(  9,16,64,lang.MenuPath, "The ^Menus Path^ of this language", 0755)
		case 4:	E_PTH( 10,16,64,lang.TextPath, "The ^Textfile path^ of this language", 0755)
		case 5:	E_PTH( 11,16,64,lang.MacroPath,"The ^Macro template path^ if this language", 0755)
		case 6:	E_BOOL(12,16,   lang.Available,"Is this language ^available^")
		case 7:	E_STR( 13,16,24,lang.Filename, "The ^Filename^ (without path) of the language datafile")
		case 8:	E_SEC( 14,16,   lang.Security, "8.2. LANGUAGE SECURITY", s_lang)
		case 9: E_BOOL(15,16,   lang.Deleted,  "Is this language record ^Deleted^")
		}
	}

	return 0;
}



void EditLanguage(void)
{
	int	records, i, x;
	char	pick[12];
	FILE	*fil;
	char	temp[PATH_MAX];
	int	offset;

	clr_index();
	working(1, 0, 0);
	IsDoing("Browsing Menu");
	if (config_read() == -1) {
		working(2, 0, 0);
		return;
	}

	records = CountLanguage();
	if (records == -1) {
		working(2, 0, 0);
		return;
	}

	if (OpenLanguage() == -1) {
		working(2, 0, 0);
		return;
	}

	for (;;) {
		clr_index();
		set_color(WHITE, BLACK);
		mbse_mvprintw( 5, 6, "8.2 LANGUAGE SETUP");
		set_color(CYAN, BLACK);
		if (records != 0) {
			snprintf(temp, PATH_MAX, "%s/etc/language.temp", getenv("MBSE_ROOT"));
			if ((fil = fopen(temp, "r")) != NULL) {
				fread(&langhdr, sizeof(langhdr), 1, fil);
				x = 4;
				set_color(CYAN, BLACK);
				for (i = 1; i <= records; i++) {
					offset = sizeof(langhdr) + ((i - 1) * langhdr.recsize);
					fseek(fil, offset, 0);
					fread(&lang, langhdr.recsize, 1, fil);
					if (i == 11)
						x = 44;
					if (lang.Available)
						set_color(CYAN, BLACK);
					else
						set_color(LIGHTBLUE, BLACK);
					snprintf(temp, 81, "%3d.  %s %-30s", i, lang.LangKey, lang.Name);
					mbse_mvprintw(i + 6, x, temp);
				}
				fclose(fil);
			}
			/* Show records here */
		}
		strcpy(pick, select_record(records, 20));
		
		if (strncmp(pick, "-", 1) == 0) {
			CloseLanguage(FALSE);
			return;
		}

		if (strncmp(pick, "A", 1) == 0) {
			working(1, 0, 0);
			if (AppendLanguage() == 0) {
				records++;
				working(1, 0, 0);
			} else
				working(2, 0, 0);
		}

		if ((atoi(pick) >= 1) && (atoi(pick) <= records))
			EditLangRec(atoi(pick));
	}
}



void InitLanguage(void)
{
    CountLanguage();
    OpenLanguage();
    CloseLanguage(TRUE);
}



int PickLanguage(char *nr)
{
	int	Lang = '\0';
	int	records, i, x;
	char	pick[12];
	FILE	*fil;
	char	temp[PATH_MAX];
	int	offset;


	clr_index();
	working(1, 0, 0);
	if (config_read() == -1) {
		working(2, 0, 0);
		return Lang;
	}

	records = CountLanguage();
	if (records == -1) {
		working(2, 0, 0);
		return Lang;
	}


	clr_index();
	set_color(WHITE, BLACK);
	snprintf(temp, 81, "%s.  LANGUAGE SELECT", nr);
	mbse_mvprintw( 5, 4, temp);
	set_color(CYAN, BLACK);
	if (records != 0) {
		snprintf(temp, PATH_MAX, "%s/etc/language.data", getenv("MBSE_ROOT"));
		if ((fil = fopen(temp, "r")) != NULL) {
			fread(&langhdr, sizeof(langhdr), 1, fil);
			x = 2;
			set_color(CYAN, BLACK);
			for (i = 1; i <= records; i++) {
				offset = sizeof(langhdr) + ((i - 1) * langhdr.recsize);
				fseek(fil, offset, 0);
				fread(&lang, langhdr.recsize, 1, fil);
				if (i == 11)
					x = 42;
				if (lang.Available)
					set_color(CYAN, BLACK);
				else
					set_color(LIGHTBLUE, BLACK);
				snprintf(temp, 81, "%3d.  %s %-28s", i, lang.LangKey, lang.Name);
				mbse_mvprintw(i + 6, x, temp);
			}
			strcpy(pick, select_pick(records, 20));

			if ((atoi(pick) >= 1) && (atoi(pick) <= records)) {
				offset = sizeof(langhdr) + ((atoi(pick) - 1) * langhdr.recsize);
				fseek(fil, offset, 0);
				fread(&lang, langhdr.recsize, 1, fil);
				Lang = lang.LangKey[0];
			}
			fclose(fil);
		}
	}
	return Lang;
}



int bbs_lang_doc(FILE *fp, FILE *toc, int page)
{
    char	    temp[PATH_MAX];
    FILE	    *wp, *ip, *no;
    int		    j;
    DIR		    *dp;
    struct dirent   *de;

    snprintf(temp, PATH_MAX, "%s/etc/language.data", getenv("MBSE_ROOT"));
    if ((no = fopen(temp, "r")) == NULL)
	return page;

    page = newpage(fp, page);
    addtoc(fp, toc, 8, 2, page, (char *)"BBS Language setup");
    j = 0;
    fprintf(fp, "\n\n");
    fread(&langhdr, sizeof(langhdr), 1, no);

    ip = open_webdoc((char *)"language.html", (char *)"BBS Language Setup", NULL);
    fprintf(ip, "<A HREF=\"index.html\">Main</A>\n");
    fprintf(ip, "<UL>\n");
	    
    while ((fread(&lang, langhdr.recsize, 1, no)) == 1) {

	if (j == 5) {
	    page = newpage(fp, page);
	    fprintf(fp, "\n");
	    j = 0;
	}
	
	snprintf(temp, 81, "language_%s.html", lang.LangKey);
        fprintf(ip, " <LI><A HREF=\"%s\">%s</A> %s</LI>\n", temp, lang.LangKey, lang.Name);
	if ((wp = open_webdoc(temp, (char *)"Language", lang.Name))) {
	    fprintf(wp, "<A HREF=\"index.html\">Main</A>&nbsp;<A HREF=\"language.html\">Back</A>\n");
	    fprintf(wp, "<P>\n");
	    fprintf(wp, "<TABLE width='600' border='0' cellspacing='0' cellpadding='2'>\n");
	    fprintf(wp, "<COL width='30%%'><COL width='70%%'>\n");
	    fprintf(wp, "<TBODY>\n");
	    add_webtable(wp, (char *)"Language key", lang.LangKey);
	    add_webtable(wp, (char *)"Language name", lang.Name);
	    add_webtable(wp, (char *)"Available", getboolean(lang.Available));
	    add_webtable(wp, (char *)"Menu path", lang.MenuPath);
	    add_webtable(wp, (char *)"Textfiles path", lang.TextPath);
	    add_webtable(wp, (char *)"Macrofiles path", lang.MacroPath);
	    add_webtable(wp, (char *)"Language file", lang.Filename);
	    web_secflags(wp, (char *)"Security level", lang.Security);
	    fprintf(wp, "</TBODY>\n");
            fprintf(wp, "</TABLE>\n");
            fprintf(wp, "<HR>\n");
            fprintf(wp, "<H3>Menu files</H3>\n");
	    if ((dp = opendir(lang.MenuPath))) {
		while ((de = readdir(dp))) {
		    if (de->d_name[0] != '.') {
			fprintf(wp, "%s<BR>\n", de->d_name);
		    }
		}
		closedir(dp);
	    }
	    fprintf(wp, "<HR>\n");
	    fprintf(wp, "<H3>Text files</H3>\n");
	    if ((dp = opendir(lang.TextPath))) {
		while ((de = readdir(dp))) {
		    if (de->d_name[0] != '.') {
			fprintf(wp, "%s<BR>\n", de->d_name);
		    }
		}
		closedir(dp);
	    }
	    fprintf(wp, "<HR>\n");
	    fprintf(wp, "<H3>Macro template files</H3>\n");
	    if ((dp = opendir(lang.MacroPath))) {
		while ((de = readdir(dp))) {
		    if (de->d_name[0] != '.') {
			fprintf(wp, "%s<BR>\n", de->d_name);
		    }
		}
		closedir(dp);
	    }
	    close_webdoc(wp);
	}

	fprintf(fp, "     Language key     %s\n", lang.LangKey);
	fprintf(fp, "     Language name    %s\n", lang.Name);
	fprintf(fp, "     Available        %s\n", getboolean(lang.Available));
	fprintf(fp, "     Menu path        %s\n", lang.MenuPath);
	fprintf(fp, "     Textfiles path   %s\n", lang.TextPath);
	fprintf(fp, "     Macrofiles path  %s\n", lang.MacroPath);
	fprintf(fp, "     Language file    %s\n", lang.Filename);
	fprintf(fp, "     Security level   %s\n", get_secstr(lang.Security));
	fprintf(fp, "\n\n");
	j++;
    }

    fprintf(ip, "</UL>\n");
    close_webdoc(ip);

    fclose(no);
    return page;
}



