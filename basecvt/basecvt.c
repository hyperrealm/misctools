/* ----------------------------------------------------------------------------
   basecvt - interactive base converter
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
#include <ctype.h>
#include <locale.h>

#include <cbase/cbase.h>

/* --- Local Headers --- */

/* --- Macros --- */

#define DFL_INPUT_BASE 16
#define DFL_OUTPUT_BASE 10

#define HEADER "basecvt v" VERSION " - Mark Lindner"
#define USAGE "[ -i <base> ] [ -o <base> ] [ -h ]"

/* --- File Scope Variables --- */

static const char *digits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

static int base_in = DFL_INPUT_BASE, base_out = DFL_OUTPUT_BASE;

/* --- Functions --- */

static void print_bases(void)
{
  printf("Input Base: %d, Output Base: %d\n\n", base_in, base_out);
}

int main(int argc, char **argv)
{
  int ch, x;
  c_bool_t errflag = FALSE;
  extern char *optarg;
  extern int optind;
  char buf[256];

  setlocale(LC_CTYPE, "POSIX");

  C_error_init(*argv);

  while((ch = getopt(argc, argv, "hi:o:")) != EOF)
    switch((char)ch)
    {
      case 'h':
        C_error_printf("%s\n", HEADER);
        C_error_usage(USAGE);
        exit(EXIT_SUCCESS);

      case 'i':
        x = atoi(optarg);
        if((x < 2) || (x > 36))
        {
          C_error_printf("Invalid input base (must be in range [2,36])\n");
          errflag = TRUE;
        }
        else
          base_in = x;
        break;

      case 'o':
        x = atoi(optarg);
        if((x < 2) || (x > 36))
        {
          C_error_printf("Invalid output base (must be in range [2,36])\n");
          errflag = TRUE;
        }
        else
          base_out = x;
        break;

      default:
        errflag = TRUE;
    }

  if(errflag)
  {
    C_error_usage(USAGE);
    exit(EXIT_FAILURE);
  }

  print_bases();

  for(;;)
  {
    char *p, *e, val[34];
    int x;

    val[33] = 0;

    if(C_io_gets(stdin, buf, sizeof(buf), '\n') == EOF)
      break;

    if(buf[0] == '<')
    {
      x = atoi(buf + 1);
      if((x < 2) || (x > 36))
        C_error_printf("Invalid input base (must be in range [2,36])\n");
      else
        base_in = x;

      print_bases();
    }
    else if(buf[0] == '>')
    {
      x = atoi(buf + 1);
      if((x < 2) || (x > 36))
        C_error_printf("Invalid output base (must be in range [2,36])\n");
      else
        base_out = x;

      print_bases();
    }
    else if(buf[0] == '?')
    {
      print_bases();
    }
    else
    {
      for(p = strtok(buf, " ");
          p;
          p = strtok(NULL, " "))
      {
        c_bool_t neg = FALSE;

        long x = strtol(p, &e, base_in);
        int i = 33;

        if(x < 0)
        {
          neg = TRUE;
          x = -x;

          if(x < 0) /* overflow */
          {
            val[--i] = digits[-(x + base_out) % base_out];
            x = -(x / base_out);
          }
        }

        do
        {
          val[--i] = digits[x % base_out];
          x /= base_out;
        }
        while(x > 0);

        if(neg)
          val[--i] = '-';

        printf("%s ", val + i);
      }
      putchar('\n');
    }
  }

  return(EXIT_SUCCESS);
}

/* end of source file */
