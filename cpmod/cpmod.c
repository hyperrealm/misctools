/* ----------------------------------------------------------------------------
   cpmod - copy file permissions
   Copyright (C) 2003-2025  Mark A Lindner

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

/* --- Feature Test Switches --- */

#include "config.h"

/* --- System Headers --- */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <utime.h>

#include <cbase/cbase.h>

/* --- Local Headers --- */

/* --- Macros --- */

#define HEADER "cpmod v" VERSION " - Mark Lindner"
#define USAGE "[ -hmota ] <source-file> <file> [ <file> ... ]"

/* --- Functions --- */

int main(int argc, char **argv)
  {
  int ch, fct, x;
  c_bool_t errflag = FALSE, modef = FALSE, ownerf = FALSE, timef = FALSE;
  struct stat stat_src;
  struct utimbuf time_buf;
  char **p;

  C_error_init(*argv);

  while((ch = getopt(argc, argv, "hmota")) != EOF)
    switch((char)ch)
      {
      case 'h':
	C_error_printf("%s\n", HEADER);
        C_error_usage(USAGE);
        exit(EXIT_SUCCESS);

      case 'm':
        modef = TRUE;
        break;

      case 'o':
        ownerf = TRUE;
        break;

      case 't':
        timef = TRUE;
        break;

      case 'a':
        modef = ownerf = timef = TRUE;
        break;

      default:
        errflag = TRUE;
      }

  if(!(modef || ownerf || timef))
    modef = TRUE; /* default to just mode copy */

  fct = argc - optind;
  if(fct < 2)
    errflag = TRUE;

  if(errflag)
    {
    C_error_usage(USAGE);
    exit(EXIT_FAILURE);
    }

  p = &(argv[optind]);
  if(stat(*p, &stat_src) != 0)
    {
    C_error_printf("Can't stat %s\n", *p);
    exit(EXIT_FAILURE);
    }

  for(x = --fct, p++; x--; p++)
    {
    c_bool_t ok = TRUE;

    if(access(*p, F_OK) != 0)
      {
      C_error_printf("%s: No such file or directory\n", *p);
      continue;
      }

    if(modef)
      {
      if(chmod(*p, stat_src.st_mode) != 0)
        C_error_printf("Can't change mode for %s\n", *p);
      }

    if(ownerf)
      {
      if(chown(*p, stat_src.st_uid, stat_src.st_gid) != 0)
        C_error_printf("Can't change owner/group for %s\n", *p);
      }

    if(timef)
      {
      time_buf.actime = stat_src.st_atime;
      time_buf.modtime = stat_src.st_mtime;

      if(utime(*p, &time_buf))
        C_error_printf("can't change access/modification time for %s\n", *p);
      }
    }

  return(EXIT_SUCCESS);
  }

/* end of source file */
