#ifndef _MBTASK_H
#define	_MBTASK_H



/*
 *  Running tasks information
 */
typedef struct _onetask {
	char		name[16];		/* Name of the task	*/
	char		cmd[PATH_MAX];		/* Command to binary	*/
	char		opts[128];		/* Commandline opts	*/
	int		tasktype;		/* Type of task		*/
	pid_t		pid;			/* Pid of task		*/
	int		running;		/* Running or not	*/
	int		status;			/* Waitpid status	*/
	int		rc;			/* Exit code		*/
} onetask;



/*
 *  Callist
 */
typedef struct _tocall {
    fidoaddr	    addr;			/* Address to call	*/
    int		    callmode;			/* Method to use	*/
    callstat	    cst;			/* Last call status	*/
    int		    calling;			/* Is calling		*/
    pid_t	    taskpid;			/* Task pid number	*/
    unsigned long   moflags;			/* Modem flags		*/
    unsigned long   diflags;			/* ISDN flags		*/
    unsigned long   ipflags;			/* TCP/IP flags		*/
} tocall;



/*
 * Logging flagbits, ' ' ? ! + -
 */
#define DLOG_ALLWAYS    0x00000001
#define DLOG_ERROR      0x00000002
#define DLOG_ATTENT     0x00000004
#define DLOG_NORMAL     0x00000008
#define DLOG_VERBOSE    0x00000010



time_t		file_time(char *);
void		load_maincfg(void);
void		load_taskcfg(void);
pid_t		launch(char *, char *, char *, int);
int		runtasktype(int);
int		checktasks(int);
void		die(int);
static int	icmp4_errcmp(char *, int, struct in_addr *, char *, int, int);
unsigned short	get_rand16(void);
int		ping_send(struct in_addr);
int		ping_receive(struct in_addr);
void		scheduler(void);
int		locktask(char *);
void		ulocktask(void);


#endif

