/* ----------------------------------------------------------------------------
   bat - binary 'cat'
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
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif /* HAVE_FCNTL_H */
#include <ctype.h>
#include <locale.h>
#include <stdlib.h>

#include <cbase/cbase.h>

/* --- Local Headers --- */

/* --- Macros --- */

#define BYTES 16                        /* bytes per line */
#define FILLCHAR '.'                    /* default nonprint character */

#define HEADER "bat v" VERSION " - Mark Lindner"
#define USAGE "[ -c <char> ] [ -hx8] [-b <base-addr>] [ <file> ... ]"

#define canprint(A)                             \
  ((A) >= ' ' && (A) <= '~')

/* --- File Scope Variables --- */

static char nonprint = FILLCHAR;
static int width = BYTES;
static c_bool_t use7bit = TRUE;
static c_bool_t mapHighASCII = FALSE;

/* --- Functions --- */

static void dump(int, uint_t);

int main(int argc, char **argv)
{
  int fct = 0, ch, x;
  uint_t base_addr = 0;
  c_bool_t errflag = FALSE;
  extern char *optarg;
  extern int optind;
  char **p;

  setlocale(LC_CTYPE, "POSIX");

  C_error_init(*argv);

  while((ch = getopt(argc, argv, "hx8c:b:")) != EOF)
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

      case 'b':
      {
        int base = 10;
        
        if(C_string_endswith(optarg, "H") || C_string_endswith(optarg, "h")) {
          base = 16;
        }
        base_addr = strtoul(optarg, NULL, base);
        break;
      }

      case 'x':
        mapHighASCII = TRUE;
        use7bit = TRUE;
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
    dump(STDIN_FILENO, base_addr);

  else for(x = fct, p = &(argv[optind]); x--; p++)
  {
    int fd;

    if((fd = open(*p, O_RDONLY)) < 0)
    {
      C_error_printf("cannot open %s\n", *p);
      continue;
    }

    printf("---- %s\n", *p);
    dump(fd, base_addr);
    close(fd);
  }

  return(EXIT_SUCCESS);
}

/*
 */

static void dump(int fd, uint_t base_addr)
{
  int bytes = 0, i, col, bytes_left;
  c_bool_t first_line = TRUE;
  uint_t count = 0, addr = base_addr, hi, lo;
  char *b, buffer[64];
  int start_offset = base_addr % BYTES;
  int rcount = C_min(width, BYTES - start_offset);

  while((bytes = read(fd, buffer, rcount)) > 0)
  {
    lo = addr & 0xFFFF;
    hi = (addr >> 16) & 0xFFFF;

    printf("%04X-%04X: ", hi, lo);

    col = 0;
    b = buffer;
    bytes_left = bytes;

    for(col = 0; col < width; ++col)
    {
      if((first_line && start_offset && (col < start_offset))
         || (bytes_left == 0))
      {
        printf("   ");
      }
      else
      {
        printf("%02X ", (unsigned char)*b);
        ++b;
        --bytes_left;
      }

      if((col + 1) == (width / 2))
        printf("- ");
    }

    printf("| ");

    if(first_line && start_offset)
      printf("%*s", start_offset, " ");
    
    for(i = bytes, b = buffer; i--; b++)
    {
      char c = *b;
      
      if(use7bit)
      {
        if(mapHighASCII && (c & 0x80))
          c &= 0x7F;
          
        putchar(canprint(c) ? c : nonprint);
      }
      else
        putchar(isprint(c) ? c : nonprint);
    }

    putchar('\n');
    count += bytes;
    addr += bytes;
    rcount = width;
    first_line = FALSE;
  }

  printf("---- %i bytes ----\n", count);
}

/* end of source file */
