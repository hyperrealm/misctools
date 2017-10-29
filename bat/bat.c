/* ----------------------------------------------------------------------------
   bat - binary 'cat'
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
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif /* HAVE_FCNTL_H */
#include <ctype.h>
#include <locale.h>

#include <cbase/cbase.h>

/* --- Local Headers --- */

/* --- Macros --- */

#define BYTES 16                        /* bytes per line */
#define FILLCHAR '.'                    /* default nonprint character */

#define HEADER "bat v" VERSION " - Mark Lindner"
#define USAGE "[ -c <char> ] [ -h ] [-8] [ <file> ... ]"

#define canprint(A)                             \
  ((A) >= ' ' && (A) <= '~')

/* --- File Scope Variables --- */

static char nonprint = FILLCHAR;
static int width = BYTES;
static c_bool_t use7bit = TRUE;

/* --- Functions --- */

static void dump(int);

int main(int argc, char **argv)
{
  int fct = 0, ch, x;
  c_bool_t errflag = FALSE;
  extern char *optarg;
  extern int optind;
  char **p;

  setlocale(LC_CTYPE, "POSIX");

  C_error_init(*argv);

  while((ch = getopt(argc, argv, "h8c:")) != EOF)
    switch((char)ch)
    {
      case 'h':
        C_error_printf("%s\n", HEADER);
        C_error_usage(USAGE);
        exit(EXIT_SUCCESS);

      case 'c':
        if(isprint((int)*optarg))
          nonprint = *optarg;
        else
        {
          C_error_printf("Bad fill character\n");
          C_error_printf("Defaulting to \"%c\"\n", *argv, FILLCHAR);
        }
        break;

      case '8':
        use7bit = FALSE;
        break;

      default:
        errflag = TRUE;
    }

  if(errflag)
  {
    C_error_usage(USAGE);
    exit(EXIT_FAILURE);
  }

  fct = argc - optind;

  if(fct == 0)
    dump(STDIN_FILENO);

  else for(x = fct, p = &(argv[optind]); x--; p++)
  {
    int fd;

    if((fd = open(*p, O_RDONLY)) < 0)
    {
      C_error_printf("cannot open %s\n", *p);
      continue;
    }

    printf("---- %s\n", *p);
    dump(fd);
    close(fd);
  }

  return(EXIT_SUCCESS);
}

/*
 */

static void dump(int fd)
{
  int bytes = 0, i, x;
  uint_t count = 0, hi, lo;
  char *b, buffer[64];

  while((bytes = read(fd, buffer, width)) > 0)
  {
    lo = count & 0xFFFF;
    hi = (count >> 16) & 0xFFFF;

    printf("%04X-%04X: ", hi, lo);

    for(i = bytes, x = 1, b = buffer; i--; b++, x++)
    {
      printf("%2.2X ", (unsigned char)*b);
      if(x == 8)
        printf("- ");
    }

    if(x < 8)
      printf("  ");

    if(bytes < width)
      for(i = (width - bytes); i--; printf("   "));

    putchar(' ');
    for(i = bytes, b = buffer; i--; b++)
    {
      if(use7bit)
        putchar(canprint(*b) ? *b : nonprint);
      else
        putchar(isprint(*b) ? *b : nonprint);
    }

    putchar('\n');
    count += bytes;
  }

  printf("---- %i bytes ----\n", count);
}

/* end of source file */
