/* ----------------------------------------------------------------------------
   pascii - get ASCII values for characters
   Copyright (C) 1994-2011  Mark A Lindner

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

#include <cbase/cbase.h>

/* --- Macros --- */

#define HEADER "pascii v" VERSION " - Mark Lindner"
#define USAGE "[ -h ]"

/* --- Functions --- */

static void convert(int);

int main(int argc, char **argv)
{
  int c, ch;
  c_bool_t errflag = FALSE;

  C_error_init(*argv);

  while((ch = getopt(argc, argv, "hc:")) != EOF)
    switch((char)ch)
    {
      case 'h':
        C_error_printf("%s\n", HEADER);
        C_error_usage(USAGE);
        exit(EXIT_SUCCESS);

      default:
        errflag = TRUE;
    }

  if(errflag)
  {
    C_error_usage(USAGE);
    exit(EXIT_FAILURE);
  }

  fprintf(stderr, "Press a key to receive its ASCII values; press"
          " Control-C to quit.\n\n");

  for(;;)
  {
    if((c = C_getchar()) == EOF)
      continue;

    convert(c);

    if(c == 3)
      break;
  }

  return(EXIT_SUCCESS);
}

/*
 */

static void convert(int c)
{
  char i;

  printf("char: ");
  if(c < ' ')
    printf("^%c ", (c + 64));
  else if(c < 127)
    printf("\"%c\"", c);
  else if(c == 127)
    printf("DEL");
  else
    printf("   ");

  printf("   dec: %3i   hex: %2.02X   oct: %3.3o   bin: ", c, c, c);

  for(i = 8; i--; c <<= 1)
    putchar('0' + ((c & 128) > 0));
  putchar('\n');
}

/* end of source file */
