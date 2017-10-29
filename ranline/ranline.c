/* ----------------------------------------------------------------------------
   ranline - select a line at random from a text file
   Copyright (C) 1994-2011  Mark A Lindner

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
#include <time.h>

#include <cbase/cbase.h>

/* --- Local Headers --- */

/* --- Macros --- */

#define MAX_LINELEN 1024

#define HEADER "ranline v" VERSION " - Mark Lindner"
#define USAGE "[ -h ] [file]"

/* --- File Scope Variables --- */

/* --- Functions --- */

static const char *ranline(FILE *, c_bool_t seek);

int main(int argc, char **argv)
{
  FILE *fp;
  extern char *optarg;
  extern int optind;
  int c;
  c_bool_t errflag = FALSE;
  const char *line;
  c_bool_t filter = FALSE;

  C_error_init(*argv);

  /* parse the command line */

  while((c = getopt(argc, argv, "h")) != EOF)
  {
    switch(c)
    {
      case 'h':
        C_error_printf("%s\n", HEADER);
        C_error_usage(USAGE);
        exit(EXIT_SUCCESS);
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

  if(argc == optind)
  {
    fp = stdin;
    filter = TRUE;
  }
  else if(!(fp = fopen(argv[optind], "r")))
  {
    C_error_syserr();
    exit(EXIT_FAILURE);
  }

  /* select a line and print it out */

  C_random_seed();

  if((line = ranline(fp, !filter)) != NULL)
    puts(line);

  if(filter)
    fclose(fp);

  exit(EXIT_SUCCESS);
}

/*
 */

static const char *ranline(FILE *fp, c_bool_t seek)
{
  static char buf[MAX_LINELEN] = "";
  static char buf2[MAX_LINELEN] = "";

  if(seek)
  {
    /* the stream is seekable, so use a more efficient algorithm that selects
     * a random spot in the file, seeks to that position,  and finds the
     * first complete line after that spot, wrapping to the beginning of the
     * file if EOF is reached
     */

    long length, rpos;
    char c;

    /* get the length of the file */

    fseek(fp, 0, SEEK_END);
    length = ftell(fp);

    if(length == 0)
      return(NULL);

    rpos = random() % length;
    fseek(fp, rpos, 0);

    while(((c = fgetc(fp)) != '\n') && (c != EOF));

    if(C_io_gets(fp, buf, MAX_LINELEN, '\n') <= 0)
    {
      /* wrap around to beginning of file */

      fseek(fp, 0, SEEK_SET);
      C_io_gets(fp, buf, MAX_LINELEN, '\n');
    }
  }
  else
  {
    /* stream isn't seekable (we're reading from stdin), so we have to read
     * all of the input, selecting a line at random along the way (with
     * uniform probability)
     */

    unsigned long line = 0;

    while(C_io_gets(fp, buf2, MAX_LINELEN, '\n') >= 0)
    {
      if((random() % ++line) == 0)
        strcpy(buf, buf2);
    }
  }

  return(buf);
}

/* end of source file */
