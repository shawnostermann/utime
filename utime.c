/* utime -- Change the atime and mtime stamps on files */
/* Copyright Shawn Ostermann */
/* Version 2.0 -- Tue May 21, 1996 */
/* Version 2.1 - Fri Jul 11 15:16:13 2008 */
/*     added option to change format and specify time */

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>
#include <utime.h>
#include <sys/stat.h>


/* external routines */
char *strptime(const char *buf, const char *format, struct tm *tm);

/* command line flags */
static int debug = 0;
static int mtime_only = 0;
static int atime_only = 0;
static char *same_time_file = NULL;
static struct stat *psame_time_stat = NULL;

/* user specified time information */
char *time_format_string = "%b %d %Y %H:%M:%S";
int time_format_string_changed = 0;
char *time_value_string = NULL;


static void
usage(
    char *progname)
{
	fprintf(stderr,"Usage: %s [-d] [-am] [-f file] [+-N[SMHDWmY]] file [files]*\n",
		progname);
	fprintf(stderr,"\n\
This command changes the ACCESS and MODIFY timestamps on a list of\n\
files.  If no date arguments are given, you'll be prompted for both\n\
times. All files given as arguments are changed to the same dates.\n\
    -d      whistle while you work\n\
    -a      alter ONLY the atime timestamps\n\
    -m      alter ONLY the mtime timestamps\n\
    -f FILE change timestamp to match FILE\n\
    -F fmt  use time format string fmt (see man strptime)\n\
    -t tstr change time to tstr (see -F)\n\
    +-N<units>:\n\
        change the timestamps by +- N units, where units is\n\
        [S]econds, [M]inutes, [H]ours, [D]ays, [W]eeks, [m]onths, or [Y]ears (365 days)\n");
	exit(-1);
}


static time_t
parsetime(
    char *prompt,
    char *opt_new_time)
{
    time_t time;
    struct tm tm;
    char *timebuf;
    char input[100];

    if (opt_new_time) {
	/* user specified new time */
	strncpy(input,opt_new_time,sizeof(input));
    } else {

	puts(prompt);
	if (!time_format_string_changed)
	    printf("[eg Jan 15 1995 17:34:56 (24 hour clock)] ");
	else
	    printf("[to match user-specified format '%s'] ", time_format_string);
	fflush(stdout);

	if (fgets(input,sizeof(input),stdin) == NULL) {
	    exit(-1);
	}
    }

    /* break it down into a struct TM */
    if (strptime(input,time_format_string,&tm) == NULL) {
	fprintf(stderr,"Bad input format '%s'\n", input);
	exit(-2);
    }

    /* convert the broken-out time into Unix seconds */
    time = mktime(&tm);

    timebuf = ctime(&time);
    timebuf[strlen(timebuf)-1] = '\00';

    printf("You asked for '%s'\n", timebuf);

    return(time);
}


static void
dofile(
    char *file,
    int offset)
{
    struct utimbuf times;
    struct stat statbuf;
    int ret;

    /* get the current time stamps */
    if (stat(file,&statbuf) != 0) {
	perror(file);
	return;
    }

    /* default is same as now */
    times.actime  = statbuf.st_atime;
    times.modtime = statbuf.st_mtime;
    

    if (same_time_file) {
	if (!psame_time_stat) {
	    psame_time_stat = malloc(sizeof(struct stat));
	    if (stat(same_time_file,psame_time_stat) != 0) {
		perror(same_time_file);
		exit(-1);
	    }
	}
	if (!mtime_only)
	    times.actime  = psame_time_stat->st_atime;
	if (!atime_only)
	    times.modtime = psame_time_stat->st_mtime;
    } else if (offset != 0) {
	if (!atime_only)
	    times.actime  += offset;
	if (!mtime_only)
	    times.modtime += offset;
    } else {  /* read it */
	time_t newtime;
	newtime = parsetime("Enter new timestamp: ", time_value_string);
	if (!mtime_only)
	    times.actime = newtime;
	if (!atime_only)
	    times.modtime = newtime;
    }

    if (debug) {
	char *str;
	char *str2;
	if (atime_only) {
	    str = ctime(&times.actime); str[24] = '\00';
	    printf("Changing the access time for '%s' to '%s'\n",
		   file, str);
	} else if (mtime_only) {
	    str = ctime(&times.modtime); str[24] = '\00';
	    printf("Changing the modify time for '%s' to '%s'\n",
		   file, str);
	} else {
	    str = ctime(&times.actime); str[24] = '\00';
	    str2 = ctime(&times.modtime); str2[24] = '\00';
	    printf("Changing the times for '%s' to\n\taccess: %s\n\tmodify: %s\n",
		   file, str, str2);
	}
    }

    /* OK, do it */
    if ((ret = utime(file,&times)) != 0)
	perror(file); /* no exit, there might be more files to try */
}


int
main(
    int argc,
    char *argv[])
{
    int i;
    int count;
    char units;
    char *unit_name = NULL;
    int offset = 0;

    if (argc == 1)
	usage(argv[0]);

    /* parse the args */
    for (i=1; i < argc; ++i) {
	if (strcmp(argv[i],"-f") == 0) {
	    if (i+1 >= argc)
		usage(argv[0]);
	    same_time_file = argv[++i];
	} else if (strcmp(argv[i],"-d") == 0) {
	    ++debug;
	} else if (strcmp(argv[i],"-a") == 0) {
	    atime_only = 1;
	} else if (strcmp(argv[i],"-m") == 0) {
	    mtime_only = 1;
	} else if (strcmp(argv[i],"-F") == 0) {
	    time_format_string = argv[++i];
	    time_format_string_changed = 1;
	} else if (strcmp(argv[i],"-t") == 0) {
	    time_value_string = argv[++i];
	} else if ((*argv[i] == '-') || (*argv[i] == '+')) {
	    if (offset != 0)
		usage(argv[0]);
	    if (sscanf(argv[i]+1,"%d%c", &count, &units) != 2) {
		usage(argv[0]);
	    }
	    if (*argv[i] == '-')
		offset = -1;
	    else
		offset = 1;
	    switch(units) {
	      case 'S': offset *= count * 1;
		unit_name = "seconds";
		break; 
	      case 'M': offset *= count * 60;
		unit_name = "minutes";
		break; 
	      case 'H': offset *= count * 60*60;
		unit_name = "hours";
		break; 
	      case 'D': offset *= count * 60*60*24;
		unit_name = "days";
		break; 
	      case 'W': offset *= count * 60*60*24*7;
		unit_name = "weeks";
		break; 
	      case 'm': offset *= count * 60*60*24*31; /* close enough */
		unit_name = "months";
		break; 
	      case 'Y': offset *= count * 60*60*24*365;
		unit_name = "years";
		break;
	      default:
		usage(argv[0]);
	    }
	    if (debug)
		printf("changing by %c%d %s (%d seconds)\n",
		       *argv[i], count, unit_name, offset);
	} else {
	    /* else, must be a file name */
	    dofile(argv[i], offset);
	}
    }

    exit(0);
}

