/* ----------------------------------------------------------------------------
   dirtree - print directory tree
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
#include <sys/param.h>

#include <cbase/cbase.h>

/* --- Local Headers --- */

/* --- Macros --- */

#define DFL_TABWIDTH 2

#define HEADER "dirtree v" VERSION " - Mark Lindner"
#define USAGE "[ -t <tabwidth> ] [ -h ] [ directory ]"

static int tab_width = DFL_TABWIDTH;

/* --- Functions --- */

static c_bool_t examine(const char *file, const struct stat *fst, uint_t depth,
                        void *hook)
{
  int i;

  for(i = depth * tab_width; i--; putchar(' '));
  fputs(file, stdout);
  if(S_ISDIR(fst->st_mode))
    putchar('/');
  putchar('\n');
  return(TRUE);
}

/*
 */

int main(int argc, char **argv)
{
  int c, tab;
  c_bool_t errflag = FALSE;
  char *dir = ".";

  C_error_init(*argv);

  /* parse the command line */

  while((c = getopt(argc, argv, "ht:")) != EOF)
  {
    switch(c)
    {
      case 'h':
        C_error_printf("%s\n", HEADER);
        C_error_usage(USAGE);
        exit(EXIT_SUCCESS);
        break;

      case 't':
        tab = atoi(optarg);
        if(tab < 1 || tab > 8)
        {
          C_error_printf("Tab width must be >= 1 and <= 8)\n");
          errflag = TRUE;
        }
        else
          tab_width = tab;
        break;

      default:
        errflag = TRUE;
        break;
    }
  }

  /* catch illegal option errors */

  if(errflag)
  {
    C_error_usage(USAGE);
    exit(EXIT_FAILURE);
  }

  /* parse out file */

  if(argc == (optind + 1))
    dir = argv[optind];

  /* now go to it! */

  if(access(dir, (R_OK | X_OK)))
  {
    C_error_printf("Directory doesn't exist or is not readable: %s\n", dir);
    exit(EXIT_FAILURE);
  }

  C_file_traverse(dir, examine, NULL);

  exit(EXIT_SUCCESS);
}

/* end of source file */
