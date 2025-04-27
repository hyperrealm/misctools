/* ----------------------------------------------------------------------------
   wrap - word wrap text
   Copyright (C) 1994-2025  Mark A Lindner

   This file is part of misctools.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
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
#include <ctype.h>

/* --- Local Headers --- */

#include <cbase/cbase.h>

/* --- Macros --- */

#define HEADER "wrap v" VERSION " - Mark Lindner"
#define USAGE "[ -h ] [ -w width ]"

#define DFL_WIDTH 78
#define MAX_WIDTH 200
#define MIN_WIDTH 10

/* --- Functions --- */

int main(int argc, char **argv)
{
  char *wbuf = C_newstr(DFL_WIDTH + 1), *wp = wbuf, prevc = 0;
  c_bool_t errf = FALSE;
  int c, wl = 0, ll = 0, width = DFL_WIDTH;
  extern char *optarg;

  C_error_init(*argv);

  while((c = getopt(argc, argv, "w:h")) != EOF)
  {
    switch(c)
    {
      case 'w':
        width = atoi(optarg);
        if(width < MIN_WIDTH || width > MAX_WIDTH)
        {
          C_error_printf("Width parameter out of range (defaulting to %i)\n",
                         DFL_WIDTH);
          width = DFL_WIDTH;
        }
        break;

      case 'h':
        C_error_printf("%s\n", HEADER);
        C_error_usage(USAGE);
        C_free(wbuf);
        exit(EXIT_SUCCESS);

      default:
        errf = TRUE;
    }
  }

  if(errf)
  {
    C_error_usage(USAGE);
    C_free(wbuf);
    exit(EXIT_FAILURE);
  }

  /* now do the actual formatting */

  while((c = getchar()) != EOF)
  {
    if(!isspace(c))
    {
      if(wl < width)
      {
        *wp = (char)c;
        wl++, wp++;
      }
      else /* word is full so we gotta dump it */
      {
        *wp = 0;
        putchar('\n');
        fputs(wbuf, stdout);
        putchar(c);
        putchar('\n');
        wp = wbuf, ll = wl = 0;
      }
    }
    else /* isspace(c) */
    {
      if(prevc == '\n')
      {
        if(prevc == '\n' && c == '\n')
          ll = 0, putchar('\n'), putchar('\n');
        else if(ll > 0)
          ll++;
      }
      else if(wl && ((ll + wl + (ll > 0)) <= width))
      {
        *wp = 0;
        if(ll)
          putchar(' '), ll++;
        fputs(wbuf, stdout);
        ll += wl;
        wl = 0, wp = wbuf;
      }
      else if(wl)
      {
        *wp = 0;
        putchar('\n');
        fputs(wbuf, stdout);
        ll = wl;
        wl = 0, wp = wbuf;
      }
    }
    prevc = c;
  }
  /* if stuff is left in word buffer, need to deal with it */

  if(wl)
  {
    *wp = 0;
    if((ll + wl + (ll > 0)) <= width)
    {
      if(ll)
        putchar(' ');
    }
    else putchar('\n');
    fputs(wbuf, stdout);
  }
  putchar('\n');
  C_free(wbuf);
  exit(EXIT_SUCCESS);
}

/* end of source file */
