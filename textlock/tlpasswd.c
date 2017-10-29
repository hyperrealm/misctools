/* ----------------------------------------------------------------------------
   tlpasswd - text console lock password generator
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

#include <cbase/cbase.h>

/* --- Local Headers --- */

#include "textlock.h"

/* --- Macros --- */

/* --- File Scope Variables --- */

/* --- Functions --- */

int main(int argc, char **argv)
{
  char text[80], text2[80], *user;
  FILE *fp;

  C_error_init(*argv);

  puts(TL_ID);

  user = C_system_get_login();

  printf("Setting new textlock password for %s...\n", user);

  if(! C_system_cdhome())
  {
    C_error_printf("Unable to access home directory.\n");
    exit(EXIT_FAILURE);
  }

  if(!(fp = fopen(".textlock", "w+")))
  {
    C_error_printf("Unable to open ~/.textlock file for writing.\n");
    exit(EXIT_FAILURE);
  }

  C_io_getpasswd("Enter password: ", text, sizeof(text) - 1);
  C_io_getpasswd("Enter it again: ", text2, sizeof(text2) - 1);

  if(strcmp(text, text2))
  {
    C_error_printf("Passwords don't match. Try again.\n");
    exit(EXIT_FAILURE);
  }

  C_system_passwd_generate(text, text2, sizeof(text2) - 1);

  fputs(text2, fp);
  fputc('\n', fp);
  fclose(fp);

  printf("New textlock password saved for %s.\n", user);
  exit(EXIT_SUCCESS);
}

/* end of source file */
