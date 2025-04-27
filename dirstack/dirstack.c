/* ----------------------------------------------------------------------------
   dirstack - directory browser/selector
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
#if defined HAVE_NCURSES_NCURSES_H
#include <ncurses/ncurses.h>
#elif defined HAVE_NCURSES_H
#include <ncurses.h>
#endif
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <termios.h>
#include <sys/ioctl.h>

#include <cbase/cbase.h>

/* --- Local Headers --- */

/* --- Macros --- */

#define HEADER "dirstack v" VERSION " - Mark Lindner"
#define USAGE "[ -hnpq ]"

#define center(S)                               \
  ((COLS - strlen(S)) / 2)

#define DATA_FILE ".dirstack"
#define DATA_SELFILE ".dirstacksel"

#define MIN_COLS 30
#define MIN_LINES 10

/* --- File Scope Variables --- */

static const char *title = "dirstack " VERSION;
static const char *more[2] = { "        ", "--more--" };
static WINDOW *mainwin, *scrollwin, *infowin;
static char *data_file, *data_selfile, *homedir, **dirlist;
static const char *newdir;
static int scrollsz, top = 0, selpos = 0;
static size_t listlen = 0;
static c_bool_t quiet = FALSE, pushcurrent = FALSE;

/* --- Functions --- */

static void redraw(void);

static void buildscreen(void)
{
  cbreak();
  noecho();

  if((LINES < MIN_LINES) || (COLS < MIN_COLS))
  {
    endwin();
    C_error_printf("Screen is too small. Must be at least %dx%d.\n",
                   MIN_COLS, MIN_LINES);
    exit(EXIT_FAILURE);
  }

  scrollsz = LINES - 6;

#ifdef NCURSES_MOUSE_VERSION
  mousemask((BUTTON1_CLICKED | BUTTON1_DOUBLE_CLICKED), NULL);
#endif

  /* draw screen */

  mainwin = newwin(LINES, COLS, 0, 0);

  scrollwin = derwin(mainwin, scrollsz, COLS, 2, 0);
  scrollok(scrollwin, TRUE);

  infowin = derwin(mainwin, 2, COLS, LINES - 2, 0);
  keypad(infowin, TRUE);
  noqiflush();
}

/*
 */

static void flow_text(WINDOW *win, const char *text, int cols, int rows)
{
  int n = 0, y = 0;
  const char *word, *ctx;
  size_t len;

  wmove(win, 0, 0);

  for(word = C_string_tokenize(text, " \n", &ctx, &len);
      word;
      word = C_string_tokenize(NULL, " \n", &ctx, &len))
  {
    if((n + len + 1) > cols)
    {
      if(++y == rows)
        return; // full

      wmove(win, y, 0);
    }

    waddnstr(win, word, len);
    waddch(win, ' ');

    n += (len + 1);
  }
}

/*
 */

static void drawscreen(void)
{
  int i;
  size_t nl = strlen(newdir);
  size_t ml = COLS - 3;
  c_bool_t truncated = FALSE;

  wmove(mainwin, 0, 0);
  wattron(mainwin, A_REVERSE);
  for(i = 0; i < COLS; i++)
  {
    mvwaddch(mainwin, 0, i, ' ');
    mvwaddch(mainwin, LINES - 3, i, ' ');
  }
  mvwaddstr(mainwin, 0, center(title), title);

  if(nl > ml)
  {
    truncated = TRUE;
    ml -= 3;
  }

  mvwaddnstr(mainwin, LINES - 3, 1, newdir, ml);
  if(truncated)
    waddstr(mainwin, "...");

  wattroff(mainwin, A_REVERSE);

  flow_text(infowin, "Use arrows or mouse click to select,"
            " <Return> or double-click to accept, <Backspace> to"
            " delete, <q> to quit.", COLS, LINES);
}

/*
 */

static void adjust(int sig)
{
  struct winsize size;

  if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &size) == 0)
  {
    LINES = size.ws_row;
    COLS = size.ws_col;
  }

  endwin();
  doupdate();

  buildscreen();
  drawscreen();
  redraw();

  wrefresh(mainwin);
  wrefresh(infowin);

  /* reposition cursor */
  wmove(infowin, 1, COLS - 1);

  /* Flush input. Why the hell is this necessary? if we don't, the
   * next character typed is "lost". I don't get it.
   */

  nodelay(infowin, TRUE);
  while(wgetch(infowin) != ERR) { };
  nodelay(infowin, FALSE);

}

/*
 */

static void drawitem(int i)
{
  char *s = dirlist[i];
  int l = strlen(homedir);
  int ml = COLS - 3;
  size_t sl = strlen(s);
  c_bool_t truncated = FALSE;

  if(!strncmp(s, homedir, l))
  {
    waddstr(scrollwin, "~");
    s += l;
    sl -= l;
    sl++;
  }

  if(sl > ml)
  {
    truncated = TRUE;
    ml -= 3;
  }

  waddnstr(scrollwin, s, ml);
  if(truncated)
    waddstr(scrollwin, "...");
}

/*
 */

static void dehighlight(void)
{
  mvwaddstr(scrollwin, selpos, 0, " ");
  drawitem(top + selpos);
  waddstr(scrollwin, " ");
}

/*
 */

static void highlight(void)
{
  wattron(scrollwin, A_REVERSE);
  dehighlight();
  wattroff(scrollwin, A_REVERSE);
}

/*
 */

static void updatemore(void)
{
  wattron(mainwin, A_BOLD);
  mvwaddstr(mainwin, 1, 0, more[top > 0]);
  mvwaddstr(mainwin, scrollsz + 2, 0, more[(top + scrollsz) < listlen]);
  wattroff(mainwin, A_BOLD);
  wrefresh(mainwin);
}

/*
 */

static void redraw(void)
{
  char **p;
  int i, z;

  werase(scrollwin);
  for(i = 0, z = top, p = dirlist + top; (i < scrollsz) && *p; i++, p++, z++)
  {
    wmove(scrollwin, i, 1);
    drawitem(z);
  }

  highlight();
  updatemore();

  wrefresh(scrollwin);

}

/*
 */

static void moveup(void)
{
  if((top + selpos) > 0)
  {
    dehighlight();

    if(selpos == 0)
    {
      wmove(scrollwin, 0, 0);
      winsertln(scrollwin);
      top--;
    }
    else
      selpos--;

    highlight();
    updatemore();

    wrefresh(scrollwin);
  }
  else
    if(! quiet)
      beep();
}

/*
 */

static void pageup(void)
{
  if(top >= scrollsz)
  {
    selpos = 0;
    top -= scrollsz;
    redraw();
  }
  else if(top > 0)
  {
    selpos = top = 0;
    redraw();
  }
  else
    if(! quiet)
      beep();
}

/*
 */

static void movedown(void)
{
  if((top + selpos) < (listlen - 1))
  {
    dehighlight();

    if(selpos == (scrollsz - 1))
    {
      wmove(scrollwin, 0, 0);
      wdeleteln(scrollwin);
      top++;
    }
    else
      selpos++;

    highlight();
    updatemore();

    wrefresh(scrollwin);
  }
  else
    if(! quiet)
      beep();
}

/*
 */

static void pagedown(void)
{
  if((top + scrollsz) < listlen)
  {
    selpos = 0;
    top += scrollsz;
    redraw();
  }

  else
    if(! quiet)
      beep();
}

/*
 */

static void delete(void)
{
  char **q, **p;

  if(listlen > 1)
  {
    for(p = (dirlist + top + selpos), q = (p + 1); *p; p++, q++)
      *p = *q;

    if((top + selpos) == (listlen - 1))
      selpos--;

    listlen--;

    redraw();
  }
  else
    if(! quiet)
      beep();
}

/*
 */

static c_bool_t dirstack_write(void)
{
  FILE *fp;
  char **p, **sel;

  if(!(fp = fopen(data_selfile, "w+")))
    return(FALSE);

  fputs(dirlist[top + selpos], fp);

  fputc('\n', fp);
  fclose(fp);

  /* write the selected one first, then the rest */

  if(!(fp = fopen(data_file, "w+")))
    return(FALSE);

  fputs(dirlist[top + selpos], fp);
  fputc('\n', fp);

  sel = (dirlist + top + selpos);
  for(p = dirlist; *p; p++)
  {
    if(p != sel)
    {
      fputs(*p, fp);
      fputc('\n', fp);
    }
  }
  fclose(fp);

  return(TRUE);
}

/*
 */

static char ** dirstack_read(size_t *listlen)
{
  FILE *fp;
  char buf[1024];
  c_vector_t *vec;
  size_t c = 0;

  vec = C_vector_start(20);

  if(pushcurrent)
  {
    ++c;
    C_vector_store(vec, C_string_dup(newdir));
  }

  if((fp = fopen(data_file, "r")))
  {
    while(C_io_gets(fp, buf, sizeof(buf), '\n') != EOF)
    {
      int l;

      if(! *buf)
        continue;

      l = strlen(buf) - 1;
      if((l > 0) && (*(buf + l) == '/'))
        *(buf + l) = NUL;

      if(isspace((int)*buf))
        continue;

      if(!strcmp(buf, newdir) && pushcurrent)
        continue; /* it was already pushed */

      C_vector_store(vec, C_string_dup(buf));
      ++c;
    }

    fclose(fp);
  }

  if(c == 0)
    C_vector_store(vec, C_string_dup(newdir));

  return(C_vector_end(vec, listlen));
}

/*
 */

int main(int argc, char **argv)
{
  c_bool_t cancel = FALSE, errflag = FALSE, nodisplay = FALSE;
  int c;

  C_error_init(*argv);

  /* parse the command line */

  while((c = getopt(argc, argv, "hnpq")) != EOF)
  {
    switch(c)
    {
      case 'h':
        C_error_printf("%s\n", HEADER);
        C_error_usage(USAGE);
        exit(EXIT_SUCCESS);
        break;

      case 'p':
        pushcurrent = TRUE;
        break;

      case 'n':
        nodisplay = TRUE;
        break;

      case 'q':
        quiet = TRUE;
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

  signal(SIGWINCH, adjust);

  if(!(newdir = C_file_getcwd()))
  {
    C_error_printf("Unable to determine working directory.\n");
    exit(EXIT_FAILURE);
  }

  /* initialize paths */

  if(!(homedir = C_system_get_homedir()))
  {
    C_error_printf("Unable to determine home directory.\n");
    exit(EXIT_FAILURE);
  }

  data_file = C_newstr(strlen(homedir) + strlen(DATA_FILE) + 2);
  strcpy(data_file, homedir);
  strcat(data_file, "/");
  strcat(data_file, DATA_FILE);

  data_selfile = C_newstr(strlen(homedir) + strlen(DATA_SELFILE) + 2);
  strcpy(data_selfile, homedir);
  strcat(data_selfile, "/");
  strcat(data_selfile, DATA_SELFILE);

  /* read in directory list */

  if(!(dirlist = dirstack_read(&listlen)))
  {
    C_error_printf("Error reading directory stack file.\n");
    exit(EXIT_FAILURE);
  }

  if(nodisplay)
  {
    if(!dirstack_write())
    {
      C_error_printf("Error writing directory stack file(s).\n");
      exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
  }

  /* init display */

  initscr();
  buildscreen();
  drawscreen();
  wrefresh(mainwin);
  redraw();

  /* command processing loop */

  for(;;)
  {
    int c;

    c = mvwgetch(infowin, 1, COLS - 1);
    switch(c)
    {
      case 'k':
      case KEY_UP:
        moveup();
        break;

      case 'j':
      case KEY_DOWN:
        movedown();
        break;

      case 'd':
      case KEY_BACKSPACE:
        delete();
        break;

      case 'n':
      case KEY_NPAGE:
      case KEY_RIGHT:
        pagedown();
        break;

      case 'p':
      case KEY_PPAGE:
      case KEY_LEFT:
        pageup();
        break;

      case KEY_ENTER:
      case '\n':
      case ' ':
        goto END;
        break;

      case 'q':
        cancel = TRUE;
        goto END;

      case 12:
        wrefresh(curscr);
        break;

#ifdef NCURSES_MOUSE_VERSION
      case KEY_MOUSE:
      {
        MEVENT mevt;
        int y;

        getmouse(&mevt);
        y = mevt.y;

        if(y == 1)
          pageup();

        else if(y == scrollsz + 2)
          pagedown();

        y -= 2;
        if(y >= 0 && ((y + top) < listlen))
        {
          dehighlight();
          selpos = y;
          highlight();
          wrefresh(scrollwin);

          if(mevt.bstate & BUTTON1_DOUBLE_CLICKED)
          {
            usleep(100000);
            goto END;
          }
        }

        break;
      }
#endif

      default:
        if(! quiet)
          beep();
    }
  }

  /* clean up & exit */

END:
  endwin();

  if(!cancel)
    if(!dirstack_write())
    {
      C_error_printf("Error writing directory stack file(s).\n");
      exit(EXIT_FAILURE);
    }

  exit(cancel ? EXIT_FAILURE : EXIT_SUCCESS);
}

/* end of source file */
