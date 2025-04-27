/* ----------------------------------------------------------------------------
   ftrunc - truncate a file
   Copyright (C) 1994-2025  Mark A Lindner

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
#include <sys/stat.h>

#include <cbase/cbase.h>

/* --- Local Headers --- */

/* --- Macros --- */

#define HEADER "ftrunc v" VERSION " - Mark Lindner"
#define USAGE "[ -l <length> ] [ -h ] <file> [ <file> ... ]"

/* --- File Scope Variables --- */

static off_t length = 0;

/* --- Functions --- */

int main(int argc, char **argv)
{
  int fct = 0, ch, x;
  c_bool_t errflag = FALSE;
  extern char *optarg;
  extern int optind;
  char **p;
  struct stat stbuf;

  C_error_init(*argv);

  while((ch = getopt(argc, argv, "hl:")) != EOF)
    switch((char)ch)
    {
      case 'h':
        C_error_printf("%s\n", HEADER);
        C_error_usage(USAGE);
        exit(EXIT_SUCCESS);

      case 'l':
        length = (off_t)atol(optarg);

        if(length < 0)
        {
          C_error_printf("Length must be >= 0\n");
          exit(EXIT_FAILURE);
        }
        break;

      default:
        errflag = TRUE;
    }

  fct = argc - optind;

  if(errflag || (fct == 0))
  {
    C_error_usage(USAGE);
    exit(EXIT_FAILURE);
  }

  for(x = fct, p = &(argv[optind]); x--; p++)
  {
    if(stat(*p, &stbuf) < 0)
    {
      C_error_printf("Can't stat %s; skipped\n", *p);
      continue;
    }

    if(! S_ISREG(stbuf.st_mode))
    {
      C_error_printf("%s is not a regular file; skipped\n", *p);
      continue;
    }

    if(truncate(*p, length) < 0)
    {
      C_error_printf("Unable to truncate %s\n", *p);
      C_error_syserr();
    }
  }

  return(EXIT_SUCCESS);
}

/* end of source file */
