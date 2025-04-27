/* ----------------------------------------------------------------------------
   textlock - text console lock utility
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
#include <sys/types.h>
#include <pwd.h>
#include <sys/param.h>
#include <time.h>
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif /* HAVE_SYS_TIME_H */
#include <signal.h>
#if defined HAVE_NCURSES_NCURSES_H
#include <ncurses/ncurses.h>
#elif defined HAVE_NCURSES_H
#include <ncurses.h>
#endif
#include <termios.h>

#include <cbase/cbase.h>

/* --- Local Headers --- */

#include "textlock.h"

/* --- Macros --- */

/* --- File Scope Variables --- */

static char *pass, inbuf[TL_MAXPASS+1], text1[40], text2[40],
  *id = TL_SHORTID, *inst = TL_INSTR;
static struct termios tio_new, tio_old;
static int cattr = 0;
static WINDOW *mainwin;

/* --- Functions --- */

static void tl_init(void);
static void tl_quit(void);
static c_bool_t tl_input(void);
static void tl_sleep(void);
static c_bool_t tl_verify(char *);
static void tl_crash(int);

int main(int argc, char **argv)
{
  C_error_init(*argv);

  if(argc == 2)
    if(!strcmp(argv[1], "-h"))
    {
      C_error_printf("%s\n", TL_ID);
      C_error_usage(TL_USAGE);
      exit(EXIT_SUCCESS);
    }  

  tl_init();

  for(;;)
  {
    halfdelay(TL_TIMEOUT);
    tl_sleep();
    if((tl_input()))
    {
      if(tl_verify(inbuf))
        break;
      beep(), beep();
    }
  }

  tl_quit();
  return(EXIT_SUCCESS);
}

/*
 *  tl_verify: verify entered password
 *
 *  Encrypt the password _s_ and compare it to the encrypted password
 *  from the passwd file. Return TRUE if it's a match.
 */

static c_bool_t tl_verify(char *s)
{
  c_bool_t ok = C_system_passwd_validate(s, pass);

  C_zeroa(s, TL_MAXPASS, char);

  return(ok);
}

/*
 *  tl_sleep: wait for the input of a password
 *
 *  Draw a floating clock on the text screen while waiting for input.
 *  Return when a key is pressed.
 */

static void tl_sleep(void)
{
  time_t t1;
  struct tm *t2;
  chtype c;
  char clock[TL_CLOCKSZ+1];
  int x, y, i;

  flushinp(); /* flush typeahead */

  for(;;)
  {
    /* draw the clock */

    time(&t1);
    t2 = localtime(&t1);
    strftime(clock, TL_CLOCKSZ, "[ %l:%M %p ]", t2);

    /* find a random spot to put it & draw it there */

    y = C_random(LINES);
    x = C_random(COLS - TL_CLOCKSZ);

    cattr = 1 - cattr;
    if(cattr)
      wattron(mainwin, A_REVERSE);
    mvwaddstr(mainwin, y, x, clock);
    wattroff(mainwin, A_REVERSE);
    wrefresh(mainwin);

    /* get a keypress */

    c = mvwgetch(mainwin, 0, 0);

    /* erase the clock */

    wmove(mainwin, y, x);
    for(i = 0; i < TL_CLOCKSZ; waddch(mainwin, ' '), i++);
    wrefresh(mainwin);

    if(c != ERR)
      break;
  }

  /* got a keypress, so jump out */

  return;
}

static int tl_centerstr(char *s)
{
  return((COLS - strlen(s)) / 2);
}

/*
 *  tl_input: input a password
 *
 *  Input the password in a dialog box. Return TRUE if we got a
 *  string, or FALSE if there was a timeout.
 */

static c_bool_t tl_input(void)
{
  int xpos, i, xb = (COLS - (TL_MAXPASS + 4)) / 2, yb = (LINES - 3) / 2;
  chtype c;
  char *p;
  c_bool_t flag = FALSE, literal = FALSE;

  flushinp(); /* flush typeahead */

  /* draw the centered titles */

  mvwaddstr(mainwin, 1, tl_centerstr(text1), text1);
  mvwaddstr(mainwin, 2, tl_centerstr(text2), text2);
  mvwaddstr(mainwin, LINES-2, tl_centerstr(id), id);
  mvwaddstr(mainwin, LINES-4, tl_centerstr(inst), inst);

  /* draw box for input field */

  for(i = 1; i < (TL_MAXPASS + 4); i++)
  {
    mvwaddch(mainwin, yb, xb + i, '-');
    mvwaddch(mainwin, yb + 2, xb + i, '-');
  }

  mvwaddch(mainwin, yb + 1, xb, '|');
  mvwaddch(mainwin, yb + 1, xb + TL_MAXPASS + 4, '|');

LOOP:
  p = inbuf;
  *p = 0;
  xpos = 0;
  literal = FALSE;

  wrefresh(mainwin);

  /* input loop with delete capabilities */

  halfdelay(TL_INPUT_TIMEOUT);
  for(;;)
  {
    c = mvwgetch(mainwin, yb + 1, xb + xpos + 2);

    if(c == ERR)
      break;

    *p = c;

    if((xpos < TL_MAXPASS) && (literal || (*p > ' ' && *p <= '~')))
      literal = FALSE, wechochar(mainwin, '*'), xpos++, p++;

    else if(*p == ' ')
      break;

    else if(((*p == KEY_BS) || (c == KEY_DEL) || (c == KEY_LEFT)
             || (c == KEY_DC)) && xpos)
      xpos--, p--, wechochar(mainwin, '\b'), wechochar(mainwin, ' '),
        literal = FALSE;

    else if((*p == '\f') || (c == KEY_REFRESH))
      wrefresh(curscr), literal = FALSE;

    else if((*p == KEY_CR) || (c == KEY_ENTER))
    {
      *p = 0;
      flag = TRUE;
      break;
    }

    else if((*p == KEY_CTRLU) || (c == KEY_EOL))
    {
      for(i = 0; i < TL_MAXPASS; i++)
        mvwaddch(mainwin, yb+1, xb+2+i, ' ');
      goto LOOP;
    }

    else if((*p == KEY_CTRLT))
      literal = TRUE;

    else beep();
  }

  wclear(mainwin);

  return(flag);
}

/*
 *  tl_init: initialization function
 *
 *  Trap all signals; get user's login name, finger name, password and
 *  salt, host name; initialize screen. Exit if any of the operations fail.
 */

static void tl_init(void)
{
  char *host, *name, *user;
  struct passwd *pw;

  /* trap all keyboard-generated signals */

  signal(SIGINT, SIG_IGN);
  signal(SIGQUIT, SIG_IGN);
  signal(SIGTERM, SIG_IGN);
  signal(SIGTSTP, SIG_IGN);

  /* trap all signals that could cause a core dump */

  signal(SIGSEGV, tl_crash);
  signal(SIGBUS, tl_crash);
  signal(SIGFPE, tl_crash);
  signal(SIGILL, tl_crash);
  signal(SIGTRAP, tl_crash);
  signal(SIGIOT, tl_crash);
#ifdef SIGEMT
  signal(SIGEMT, tl_crash);
#endif
  signal(SIGSYS, tl_crash);

  if(!(pw = getpwuid(getuid())))
  {
    C_error_printf("Can't find your password entry.\n");
    exit(EXIT_FAILURE);
  }

  if(strlen(pw->pw_passwd) < 3)
  {
    FILE *fp;
    char text[80];

    if(! C_system_cdhome())
    {
      C_error_printf("Unable to access home directory.\n");
      exit(EXIT_FAILURE);
    }

    if(!(fp = fopen(".textlock", "r")))
    {
      C_error_printf("Can't open ~/.textlock file.\n");
      exit(EXIT_FAILURE);
    }

    C_io_gets(fp, text, sizeof(text), '\n');
    fclose(fp);

    pass = C_string_dup(text);
  }
  else
    pass = C_string_dup(pw->pw_passwd);

  user = C_system_get_login();
  name = C_system_get_fullname();
  host = C_system_get_hostname();

  /* turn off XON/XOFF processing (that is, don't allow freeze of
     screen output with control-S */

  tcgetattr(STDIN_FILENO, &tio_old);
  tio_new = tio_old;

  tio_new.c_iflag &= ~(IXON | IXOFF);
  tio_new.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);

  tio_new.c_cc[VINTR] =
    tio_new.c_cc[VSTART] =
    tio_new.c_cc[VSTOP] =
    tio_new.c_cc[VQUIT] =
    tio_new.c_cc[VERASE] =
    tio_new.c_cc[VKILL] =
    tio_new.c_cc[VEOF] =
    tio_new.c_cc[VEOL] =
#ifdef SOLARIS
    tio_new.c_cc[VSWTCH] =
#endif
    -1;

  tcsetattr(STDIN_FILENO, TCSANOW, &tio_new);

  mainwin = initscr();

  if((COLS < 40) || (LINES < 15))
  {
    endwin();
    C_error_printf("Can't run in a window less than 40x15.\n");
    exit(EXIT_FAILURE);
  }

  noecho();
  halfdelay(TL_TIMEOUT);
  keypad(mainwin, TRUE);

  C_random_seed();

  sprintf(text1, "Locked by %s", name);
  sprintf(text2, "(%s@%s)", user, host);
}

/*
 */

static void tl_crash(int sig)
{
  /* zap the input buffer in case of a core dump */

  memset(inbuf, 0, TL_MAXPASS);
  C_error_printf("Aborted due to signal #%i\n", sig);
  exit(EXIT_FAILURE);
}

/*
 */

static void tl_quit(void)
{
  wclear(mainwin);
  wrefresh(mainwin);
  endwin();

  /* reset old terminal attributes */

  tcsetattr(STDIN_FILENO, TCSANOW, &tio_old);
  exit(EXIT_SUCCESS);
}

/* end of source file */
