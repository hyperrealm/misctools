/* ----------------------------------------------------------------------------
   bin2c - generate C structure from file contents
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
#include <string.h>
#include <time.h>

#include <cbase/cbase.h>

/* --- Local Headers --- */

/* --- Macros --- */

#define USAGE "[-hls] [-i infile] [-o outfile] [-n name]"
#define HEADER "bin2c v" VERSION " - Mark Lindner"

/* --- Functions --- */

/*
 */

int main(int argc, char **argv)
{
  int c;
  uint_t i = 0, base = 0;
  c_bool_t errflag = FALSE;
  char *input_file = NULL, *output_file = NULL, *name = "data";
  FILE *inf = stdin, *outf = stdout;
  time_t now;
  c_byte_t buf[16834], *p;
  size_t count;
  c_bool_t output_length = FALSE, static_vars = FALSE;

  C_error_init(*argv);

  while((c = getopt(argc, argv, "hi:ln:o:s")) != EOF)
  {
    switch(c)
    {
      case 'h':
        C_error_printf("%s\n", HEADER);
        C_error_usage(USAGE);
        exit(EXIT_SUCCESS);
        break;

      case 'i':
        input_file = strdup(optarg);
        break;

      case 'o':
        output_file = strdup(optarg);
        break;

      case 'n':
        name = strdup(optarg);
        break;

      case 'l':
        output_length = TRUE;
        break;

      case 's':
        static_vars = TRUE;
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

  if(input_file)
  {
    if(!(inf = fopen(input_file, "r")))
    {
      C_error_printf("Unable to open input file \"%s\"\n", input_file);
      exit(EXIT_FAILURE);
    }
  }

  if(output_file)
  {
    if(!(outf = fopen(output_file, "w")))
    {
      C_error_printf("Unable to open output file \"%s\"\n", output_file);
      fclose(inf);
      exit(EXIT_FAILURE);
    }
  }

  now = time(NULL);
  fprintf(outf, "/* Generated from %s\n * by bin2c on %.24s\n */\n\n",
          input_file ? input_file : "standard input", ctime(&now));

  fprintf(outf, "#include <stddef.h>\n\n");
  
  if(static_vars)
    fputs("static ", outf);

  fprintf(outf, "const unsigned char %s[] = {\n  ", name);

  while((count = fread(buf, 1, sizeof(buf), inf)) > 0)
  {
    for(p = buf; count > 0; ++p, --count)
    {
      if ((i > 0) && (i % 8 == 0))
      {
        fprintf(outf, "// 0x%04X\n  ", base);
        base = i;
      }
      fprintf(outf, "0x%02X, ", *p);
      ++i;
    }
  }

  int pad = i % 8;
  if(pad != 0)
  {
    pad = 8 - pad;
    fprintf(outf, "%*s", (pad * 6), " ");
  }

  fprintf(outf, "// 0x%04X\n", base);


  fputs("};\n", outf);

  if(output_length)
  {
    fputc('\n', outf);

    if(static_vars)
      fputs("static ", outf);

    fprintf(outf, "const unsigned int %s_length = %uU;\n\n", name, i);
  }

  fclose(inf);
  fclose(outf);

  exit(EXIT_SUCCESS);
}
