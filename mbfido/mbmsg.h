#ifndef	_MBMSG_H
#define	_MBMSG_H

/* $Id$ */

void	ProgName(void);
void	Help(void);
void	die(int);
void	DoMsgBase(void);
void	PackArea(char *, int);
void	LinkArea(char *, int);
void	KillArea(char *, char *, int, int, int);

#endif

