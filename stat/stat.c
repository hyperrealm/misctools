/* ----------------------------------------------------------------------------
   stat - get file information
   Copyright (C) 2000-2011  Mark A Lindner

   This file is part of misctools.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, see
   <http://www.gnu.org/licenses/>.
   ----------------------------------------------------------------------------
*/

/*
 * This program was originally written by someone at Purdue in the days of the
 * PDP-11, and was subsequently modified by various people. This version was
 * cleaned up by Mark Lindner; all the old junk was removed, the source was
 * reformatted, the program's output was made more readable, and various other
 * tweaks were made.
 */

/* --- Feature Test Switches --- */

#include "config.h"

/* --- System Headers --- */

#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <ctype.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#if defined MAJOR_IN_MKDEV
#include <sys/mkdev.h>
#elif (defined MAJOR_IN_SYSMACROS)
#include <sys/sysmacros.h>
#endif
#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>

#include <cbase/cbase.h>

/* --- Local Headers --- */

/* --- Macros --- */

#define HEADER "stat v" VERSION " - Mark Lindner"
#define USAGE "[ -h ] <file> [ <file> ... ]"

#define LINKBUFSZ 256 /* symlink buffer size */

#define SINCE

/* --- Functions --- */

#ifdef SINCE

static void tsince(long time_sec)
{
  long time_buf;
  long d_since; /* days elapsed since time */
  long h_since; /* hours elapsed since time */
  long m_since; /* minutes elapsed since time */
  long s_since; /* seconds elapsed since time */

  time(&time_buf);

  if(time_sec > time_buf)
  {
    fprintf(stderr, "Time going backwards\n");
    exit(EXIT_FAILURE);
  }

  s_since = time_buf - time_sec;
  d_since = s_since / 86400l ;
  s_since -= d_since * 86400l ;
  h_since = s_since / 3600l ;
  s_since -= h_since * 3600l ;
  m_since = s_since / 60l ;
  s_since -= m_since * 60l ;

  printf(" (%05ld.%02ld:%02ld:%02ld)\n", d_since, h_since, m_since, s_since);
}

#endif /* SINCE */

/*
 */

static void dostat(const char *filename);
static void tsince(long time_sec);

/*
 */

int main(int argc, char **argv)
{
  int fct, ch, x;
  c_bool_t errflag = FALSE, **p;

  C_error_init(*argv);
    
  while((ch = getopt(argc, argv, "h")) != EOF)
    switch((char)ch)
    {
      case 'h':
        C_error_printf("%s\n", HEADER);
        C_error_usage(USAGE);
        exit(EXIT_SUCCESS);

      default:
        errflag = TRUE;
    }

  fct = argc - optind;
  if(fct == 0)
    errflag = TRUE;

  if(errflag)
  {
    C_error_usage(USAGE);
    exit(EXIT_FAILURE);
  }

  for(x = fct, p = &(argv[optind]); x--; p++)
  {
    dostat(*p);
    putchar('\n');
  }
  
  return(EXIT_SUCCESS);
}

/*
 */

static void dostat(const char *filename)
{
  int count;
  struct passwd *pwent;
  struct group *grent;
  struct stat stbuf;
  char mode[11] = "----------";
  char linkbuf[LINKBUFSZ];
  char *user, *group;

  /* name */
  
  if(lstat(filename, &stbuf) < 0)
  {
    C_error_printf("Can't lstat %s\n", filename);
    return;
  }

  if((stbuf.st_mode & S_IFMT) == S_IFLNK)
  {
    if((count = readlink(filename, linkbuf, LINKBUFSZ)) < 0)
    {
      C_error_printf("Can't read link %s\n", filename);
      return;
    }
    if(count < LINKBUFSZ)
      linkbuf[count] = '\0';

    printf("  File: \"%s\" -> \"%s\"\n", filename, linkbuf);
  }
  else
  {
    printf("  File: \"%s\"\n", filename);
  }

  /* size */

#if (SIZEOF_OFF_T == 8)
  printf("  Size: %-10lld", (long long)stbuf.st_size);
#else
  printf("  Size: %-10ld", stbuf.st_size);
#endif
  
  /* type */
  
  printf("               Type: ");

  switch(stbuf.st_mode & S_IFMT)
  {
    case S_IFDIR:       
      printf("Directory\n");
      break;
    case S_IFCHR:       
      printf("Character Device\n");
      break;
    case S_IFBLK:       
      printf("Block Device\n");
      break;
    case S_IFREG:       
      printf("Regular File\n");
      break;
    case S_IFLNK:       
      printf("Symbolic Link\n");
      break;
    case S_IFSOCK:      
      printf("Socket\n");
      break;
#ifdef S_IFDOOR
    case S_IFDOOR:
      printf("Door\n");
      break;
#endif
    case S_IFIFO:       
      printf("Fifo (Named Pipe)\n");
      break;
    default             :       
      printf("Unknown\n");
  }

  /* mode */

  if(stbuf.st_mode & (S_IEXEC >> 6)) /* Other execute */
    mode[9] = 'x';
  if(stbuf.st_mode & (S_IWRITE >> 6)) /* Other write */
    mode[8] = 'w';
  if(stbuf.st_mode & (S_IREAD >> 6)) /* Other read */
    mode[7] = 'r';
  if(stbuf.st_mode & (S_IEXEC >> 3)) /* Group execute */
    mode[6] = 'x';
  if(stbuf.st_mode & (S_IWRITE >> 3)) /* Group write */
    mode[5] = 'w';
  if(stbuf.st_mode & (S_IREAD >> 3)) /* Group read */
    mode[4] = 'r';
  if(stbuf.st_mode & S_IEXEC) /* User execute */
    mode[3] = 'x';
  if(stbuf.st_mode & S_IWRITE) /* User write */
    mode[2] = 'w';
  if(stbuf.st_mode & S_IREAD) /* User read */
    mode[1] = 'r';
  if(stbuf.st_mode & S_ISVTX) /* Sticky bit */
    mode[9] = 't';
  if(stbuf.st_mode & S_ISGID) /* Set group id */
    mode[6] = 's';
  if(stbuf.st_mode & S_ISUID) /* Set user id */
    mode[3] = 's';

  switch(stbuf.st_mode & S_IFMT)
  {
    case S_IFDIR:       
      mode[0] = 'd';
      break;
      
    case S_IFCHR:       
      mode[0] = 'c';
      break;
      
    case S_IFBLK:       
      mode[0] = 'b';
      break;
      
    case S_IFREG:       
      mode[0] = '-';
      break;

    case S_IFLNK:       
      mode[0] = 'l';
      break;

    case S_IFSOCK:      
      mode[0] = 's';
      break;

    case S_IFIFO:       
      mode[0] = 'p';
      break;

#ifdef S_IFDOOR
    case S_IFDOOR:
      mode[0] = 'D';
      break;
#endif

    default:    
      mode[0] = '?';
  }
  
  printf("  Mode: %04lo/%s", (long)(stbuf.st_mode & 07777), mode);

  /* links */

  printf("         Links: %-5ld\n", (long)stbuf.st_nlink);
  
  /* UID */
  
  setpwent();
  pwent = getpwuid(stbuf.st_uid);
  endpwent();

  user = (pwent ? pwent->pw_name : "?");
  
  printf("   UID: %5d/%-8s", (int)stbuf.st_uid, user);

  /* inode */

#if (SIZEOF_INO_T == 8)
  printf("         I-Node: %-10lld\n", (long long)stbuf.st_ino);
#else
  printf("         I-Node: %-10ld\n", (long)stbuf.st_ino);
#endif

  /* GID */
  
  setgrent();
  grent = getgrgid(stbuf.st_gid);
  endgrent();

  group = (grent ? grent->gr_name : "?");

  printf("   GID: %5ld/%-8s", (long)stbuf.st_gid, group);

  /* device */
  
  printf("         Device: %ld,%ld", (long)major(stbuf.st_dev),
         (long)minor(stbuf.st_dev));

#ifdef HAVE_ST_RDEV
  /* only meaningful if file is device */

  if(((stbuf.st_mode & S_IFMT) == S_IFCHR)
     || ((stbuf.st_mode & S_IFMT) == S_IFBLK))
    printf(" (Type: %ld,%ld)\n", (long)major(stbuf.st_rdev),
           (long)minor(stbuf.st_rdev));
  else
    putchar('\n');
#endif

#ifdef SINCE
  /* The %.24s strips the newline from the ctime() string */

  printf("Access: %.24s", ctime(&(stbuf.st_atime)));
  tsince(stbuf.st_atime);

  printf("Modify: %.24s", ctime(&(stbuf.st_mtime)));
  tsince(stbuf.st_mtime);

  printf("Change: %.24s", ctime(&(stbuf.st_ctime)));
  tsince(stbuf.st_ctime);

#else /* SINCE */

  printf("Access: %s", ctime(&(stbuf.st_atime)));
  printf("Modify: %s", ctime(&(stbuf.st_mtime)));
  printf("Change: %s", ctime(&(stbuf.st_ctime)));

#endif /* SINCE */
}

/* end of source file */
