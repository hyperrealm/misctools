/* ----------------------------------------------------------------------------
   pkgenv - simple package environment manager
   Copyright (C) 2007-2025  Mark A Lindner

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
#include <sys/types.h>
#include <dirent.h>

#include <cbase/cbase.h>

/* --- Local Headers --- */

/* --- Macros --- */

#define HEADER "pkgenv v" VERSION " - Mark Lindner"
#define USAGE "[ -hvV ] [ -s <shell> ] { add | del | list | avail }\n\t" \
    "[ <module> ... ]"

#define BIN_DIR        "bin"
#define MAN_DIR        "share/man"
#define OLD_MAN_DIR    "man"
#define LIB_DIR        "lib"
#define PKGCONFIG_DIR  "lib/pkgconfig"
#define ACLOCAL_DIR    "share/aclocal"

#define BIN_ENV        "PATH"
#define MAN_ENV        "MANPATH"

#ifdef __APPLE__
#define LIB_ENV        "DYLD_LIBRARY_PATH"
#else
#define LIB_ENV        "LD_LIBRARY_PATH"
#endif

#define PKGCONFIG_ENV  "PKG_CONFIG_PATH"
#define ACLOCAL_ENV    "ACLOCAL_PATH"

#define DEFAULT_PKGDIR "/pkg"
#define DEFAULT_SHELL  "bash"
#define PKGENV_DIR     "PKGENV"
#define PKGENV_EXT     ".pkgenv"
#define PKGENV_OUTFILE ".pkgenv"

#define OP_SET 0
#define OP_APPEND 1
#define OP_PREPEND 2

/* --- Structures --- */

typedef struct path_t
{
  const char *dir;
  const char *old_dir;
  const char *env;
  c_linklist_t *list;
  const char *formatted;
  c_bool_t modified;
} path_t;

/* --- File Scope Variables --- */

static const char *shell = NULL;

static path_t paths[] =
{ { BIN_DIR,       NULL,        BIN_ENV,          NULL, NULL, FALSE },
  { MAN_DIR,       OLD_MAN_DIR, MAN_ENV,          NULL, NULL, FALSE },
  { LIB_DIR,       NULL,        LIB_ENV,          NULL, NULL, FALSE },
  { PKGCONFIG_DIR, NULL,        PKGCONFIG_ENV,    NULL, NULL, FALSE },
  { ACLOCAL_DIR,   NULL,        ACLOCAL_ENV,      NULL, NULL, FALSE } };

static path_t current_list =
  { NULL,          NULL,        "PKGENV_CURRENT", NULL, NULL, FALSE };

static c_hashtable_t *varmap;

static uint_t rows = 0, columns = 0;

static c_bool_t verbose = FALSE;

/* --- Functions --- */

/*
static void print_line(void)
{
  int i;

  for(i = 0; i < (columns - 1); ++i)
    putchar('-');
  putchar('\n');
}
*/

/*
 * see if 'foo-x.y.z' matches 'foo'; return the version if so, NULL otherwise
 */

static const char * pkg_name_match(const char *spec, const char *name)
{
  const char *p;
  size_t len;

  p = strrchr(spec, '-');
  if(! p)
    return(NULL);

  len = (p - spec);

  if((strlen(name) == len) && ! strncmp(spec, name, len))
    return(p + 1);
  else
    return(NULL);
}

/*
 */

static int pkg_version_compare(const char *ver1, const char *ver2)
{
  const char *p = ver1, *q = ver2;
  char *pp, *qq;
  int a, b;

  while(*p && *q)
  {
    if(*p == '.')
    {
      if(*q != '.')
        return(-1);
      else
      {
        ++p;
        ++q;
        continue;
      }
    }
    else if(isdigit(*p))
    {
      if(! isdigit(*q))
      {
        // weird case
        return(-1);
      }
      else
      {
        a = strtol(p, &pp, 10);
        b = strtol(q, &qq, 10);

        if(a < b)
          return(-1);
        else if(a > b)
          return(1);
        else
        {
          p = pp;
          q = qq;
          continue;
        }
      }
    }
    else
    {
      if(*p < *q)
        return(-1);
      else if(*p > *q)
        return(1);
      else
      {
        ++p;
        ++q;
        continue;
      }
    }
  }

  if(*p)
    return(1);
  else if(*q)
    return(-1);
  else
    return(0);
}

/*
 */

static char *var_subst(char *text, c_hashtable_t *vars)
{
  c_dstring_t *ds = C_dstring_create(256);
  char *p, *var = NULL;
  c_bool_t invar = FALSE, sawvar = FALSE;

  for(p = text; *p; ++p)
  {
    if(*p == '$')
    {
      if(sawvar)
        C_dstring_putc(ds, *p);
      else
        sawvar = TRUE;
    }
    else if((*p == '{') && sawvar)
    {
      invar = TRUE;
    }
    else if((*p == '}') && invar)
    {
      *p = NUL;

      if(var)
      {
        char *val = (char *)C_hashtable_restore(vars, var);
        if(val)
          C_dstring_puts(ds, val);
        else
        {
          val = getenv(var);
          if(val)
            C_dstring_puts(ds, val);
        }
      }

      var = NULL;
      invar = sawvar = FALSE;
    }
    else if(invar)
    {
      if(! var)
        var = p;
    }
    else
    {
      invar = sawvar = FALSE;
      C_dstring_putc(ds, *p);
    }
  }

  return(C_dstring_destroy(ds));
}

/*
 */

static void vec_print(char **vec, size_t len, size_t longest)
{
  size_t ncols = 0, rowlen = 0;
  int x, y;

  longest += 3;

  ncols = (columns - 1) / longest;
  if(ncols == 0)
    ncols = 1;

  rowlen = (len + ncols - 1) / ncols;

  for(y = 0; y < rowlen; y++)
  {
    int n = y;

    for(x = 0; x < ncols; ++x, n += rowlen)
    {
      char *q = "";

      if(n < len)
        q = vec[n];

      printf("%-*s", (int)longest, q);
    }

    putchar('\n');
  }
}

/*
 */

static path_t *path_find(const char *env)
{
  int i;

  for(i = 0; i < C_lengthof(paths); i++)
  {
    if(! strcmp(paths[i].env, env))
      return(&(paths[i]));
  }

  return(NULL);
}

/*
 */

static void path_parse(path_t *path)
{
  char *p, *s;

  if(path->list)
    C_linklist_destroy(path->list);

  path->list = C_linklist_create();
  C_linklist_set_destructor(path->list, free);

  s = getenv(path->env);

  if(s)
  {
    for(p = strtok(s, ":"); p; p = strtok(NULL, ":"))
      C_linklist_append(path->list, C_string_dup(p));
  }
}

/*
 */

static c_bool_t path_contains(path_t *path, const char *dir)
{
  if(path->list)
  {
    const char *p;

    for(C_linklist_move_head(path->list);
        (p = (const char *)C_linklist_restore(path->list)) != NULL;
        C_linklist_move_next(path->list))
    {
      if(! strcmp(p, dir))
        return(TRUE);
    }
  }

  return(FALSE);
}

/*
 */

static const char *path_formatted(path_t *path)
{
  if(! path->list)
    return("");

  if(! path->formatted)
  {
    c_dstring_t *ds = C_dstring_create(256);
    c_bool_t first = TRUE;
    const char *p;

    for(C_linklist_move_head(path->list);
        (p = (const char *)C_linklist_restore(path->list)) != NULL;
        C_linklist_move_next(path->list))
    {
      if(! first)
        C_dstring_putc(ds, ':');

      C_dstring_puts(ds, p);

      first = FALSE;
    }

    path->formatted = C_dstring_destroy(ds);
  }

  return(path->formatted);
}

/*
 */

static void print_setenv(FILE *fp, const char *var, const char *value)
{
  if((! strcmp(shell, "sh"))
     || (! strcmp(shell, "bash"))
     || (! strcmp(shell, "zsh"))
     || (! strcmp(shell, "ksh")))
  {
    if(strlen(value) == 0)
      fprintf(fp, "unset %s;\n", var);
    else
      fprintf(fp, "export %s=\"%s\";\n", var, value);
  }
  else if((! strcmp(shell, "csh"))
          || (! strcmp(shell, "tcsh")))
  {
    if(strlen(value) == 0)
      fprintf(fp, "unsetenv %s;\n", var);
    else
      fprintf(fp, "setenv %s \"%s\";\n", var, value);
  }
}

/*
 */

static void path_print_setenv(FILE *fp, path_t *path)
{
  print_setenv(fp, path->env, path_formatted(path));
}

/*
 */

static void list_print(c_linklist_t *list)
{
  char *p, **v;
  size_t longest = 0, len;
  c_vector_t *vec;

  if(list)
  {
    vec = C_vector_start(C_linklist_length(list));
    for(C_linklist_move_head(list);
        (p = (char *)C_linklist_restore(list)) != NULL;
        C_linklist_move_next(list))
    {
      len = strlen(p);
      if(len > longest)
        longest = len;

      C_vector_store(vec, p);
    }

    v = C_vector_end(vec, &len);
    C_string_sortvec(v, len);

    vec_print(v, len, longest);

    C_free_vec(v);
  }
}

/*
 */

static c_bool_t path_remove(path_t *path, const char *dir)
{
  if(! path->list)
  {
    path->list = C_linklist_create();
    C_linklist_set_destructor(path->list, free);
  }
  else
  {
    for(C_linklist_move_head(path->list);
        ! C_linklist_isend(path->list);
        C_linklist_move_next(path->list))
    {
      char *p = (char *)C_linklist_restore(path->list);

      if(! strcmp(p, dir))
      {
        C_linklist_delete(path->list);

        path->modified = TRUE;
        return(TRUE);
      }
    }
  }

  return(FALSE);
}

/*
 */

static c_bool_t path_prepend(path_t *path, const char *dir)
{
  if(! path->list)
  {
    path->list = C_linklist_create();
    C_linklist_set_destructor(path->list, free);
  }
  else
    C_linklist_prepend(path->list, C_string_dup(dir));

  path->modified = TRUE;

  return(TRUE);
}

/*
 */

static c_bool_t path_append(path_t *path, const char *dir)
{
  if(! path->list)
  {
    path->list = C_linklist_create();
    C_linklist_set_destructor(path->list, free);
  }
  else
  {
    C_linklist_append(path->list, C_string_dup(dir));
  }

  path->modified = TRUE;

  return(TRUE);
}

/*
 */

static c_bool_t pkg_modify_explicit(const char *pkgdir, const char *pkgenvdir,
                                    const char *pkg, const char *fver,
                                    c_bool_t add)
{
  FILE *fp;
  char *p, *file, buf[1024], *dir, *pkgtag;
  c_hashtable_t *vars;
  int line;

  file = C_string_va_make(pkgenvdir, "/", pkg, "-", fver, PKGENV_EXT, NULL);

  if((fp = fopen(file, "r")) == NULL)
  {
    C_free(file);
    return(FALSE);
  }

  pkgtag = C_string_va_make(pkg, "-", fver, NULL);

  vars = C_hashtable_create(26);
  C_hashtable_set_destructor(vars, free);

  dir = C_string_va_make(pkgdir, "/", pkg, "-", fver, NULL);

  C_hashtable_store(vars, "root", dir);
  C_hashtable_store(vars, "prefix", C_string_dup(dir));
  C_hashtable_store(vars, "version", C_string_dup(fver));
  C_hashtable_store(vars, "name", C_string_dup(pkg));

  for(line = 1; ((C_io_gets(fp, buf, sizeof(buf), '\n')) != EOF); ++line)
  {
    C_string_trim(buf);

    if(*buf == NUL)
      continue; /* blank line */

    else if(*buf == '#')
      continue; /* comment */

    p = strtok(buf, " \t");
    if(! strcmp(p, "adjust"))
    {
      while((p = strtok(NULL, " \t")) != NULL)
      {
        path_t *path = path_find(p);
        if(path)
        {
          const char *dirs[] = { path->dir, path->old_dir, NULL };
          const char **dirp;

          for(dirp = dirs; *dirp; ++dirp)
          {
            dir = C_string_va_make(pkgdir, "/", pkg, "-", fver, "/", *dirp,
                                   NULL);

            if(add)
            {
              if(! C_file_isdir(dir))
              {
                if(verbose)
                  C_error_printf("%s: directory \"%s\" not found\n",
                                 path->env, dir);
              }
              else if(! path_contains(path, dir))
              {
                if(verbose)
                  C_error_printf("%s: adding directory \"%s\"\n", path->env,
                                 dir);

                path_prepend(path, dir);
              }
            }
            else
              path_remove(path, dir);

            C_free(dir);
          }
        }
      }
    }
    else if(! strcmp(p, "depends"))
    {
      /* I'll implement this in a future verison... */
    }
    else
    {
      char *var = p, *value = NULL;
      int op = OP_SET;
      path_t *path;

      if((p = strtok(NULL, " \t")) == NULL)
      {
        C_error_printf("%s:%d - Parse error\n", file, line);
        continue;
      }

      if(! strcmp(p, "="))
        op = OP_SET;
      else if(! strcmp(p, ".="))
        op = OP_APPEND;
      else if(! strcmp(p, "+="))
        op = OP_PREPEND;
      else
      {
        C_error_printf("%s:%d - Parse error\n", file, line);
        continue;
      }

      if((value = strtok(NULL, " \t")) == NULL)
      {
        C_error_printf("%s:%d: Parse error\n", file, line);
        continue;
      }

      path = path_find(var);
      if(path)
      {
        if(op == OP_SET)
        {
          C_error_printf("%s:%d - Operator `=' not allowed for %s\n", file,
                         line, var);
          continue;
        }
        else if(op == OP_PREPEND)
        {
          char *svalue = var_subst(value, vars);

          if(add)
          {
            if(! path_contains(path, svalue))
            {
              if(verbose)
                C_error_printf("%s: prepending directory \"%s\"\n", path->env,
                               svalue);

              path_prepend(path, svalue);
            }
          }
          else
          {
            if(verbose)
              C_error_printf("%s: removing directory \"%s\"\n", path->env,
                             svalue);

            path_remove(path, svalue);
          }

          C_free(svalue);
        }
        else
        {
          char *svalue = var_subst(value, vars);

          if(add)
          {
            if(! path_contains(path, svalue))
            {
              if(verbose)
                C_error_printf("%s: appending directory \"%s\"\n", path->env,
                               svalue);

              path_append(path, svalue);
            }
          }
          else
          {
            if(verbose)
              C_error_printf("%s: removing directory \"%s\"\n", path->env,
                             svalue);

            path_remove(path, svalue);
          }

          C_free(svalue);
        }
      }
      else
      {
        if(op != OP_SET)
        {
          C_error_printf("%s%d - Operators `+=' and `.=' allowed for paths"
                         " only.\n", file, line);
          continue;
        }
        else
        {
          if(add)
            C_hashtable_store(varmap, var, var_subst(value, vars));
          else
            C_hashtable_store(varmap, var, C_string_dup(""));
        }
      }
    }
  }

  fclose(fp);

  C_free(file);

  C_hashtable_destroy(vars);

  if(add)
  {
    if(! path_contains(&current_list, pkgtag))
      path_append(&current_list, pkgtag);
  }
  else
    path_remove(&current_list, pkgtag);

  C_free(pkgtag);

  return(TRUE);
}

/*
 */

static c_bool_t pkg_modify(const char *pkgdir, const char *pkgenvdir,
                           const char *pkg, const char *ver, c_bool_t add)
{
  DIR *dp;
  struct dirent *de;
  char *q, *fver = NULL;
  c_bool_t ok = FALSE;

  if(ver)
    return(pkg_modify_explicit(pkgdir, pkgenvdir, pkg, ver, add));

  if(add)
  {
    /* for add, find the latest version */

    char *cver = NULL;

    if(! (dp = opendir(pkgenvdir)))
      return(FALSE);

    while((de = readdir(dp)) != NULL)
    {
      char *cpkg = de->d_name;

      if(! C_string_endswith(cpkg, PKGENV_EXT))
        continue;

      C_string_rchop(cpkg, ".");

      q = strrchr(cpkg, '-');
      if(! q)
        continue;

      *q = NUL;
      cver = q + 1;

      if(strcmp(pkg, cpkg))
        continue;

      if(ver)
        if(strcmp(ver, cver))
          continue;

      if(fver)
      {
        /* is this version newer than the one we previously found? */
        if(pkg_version_compare(cver, fver) > 0)
        {
          /* yes...save it */
          C_free(fver);
          fver = C_string_dup(cver);
        }
      }
      else
      {
        /* first version found */
        fver = C_string_dup(cver);
      }

      *q = '-';
    }

    closedir(dp);

    if(! fver)
      return(FALSE); /* no matching version found */

    ok = pkg_modify_explicit(pkgdir, pkgenvdir, pkg, fver, add);
    C_free(fver);
    return(ok);
  }
  else
  {
    /* for del, remove every version that's currently active */

    const char *pname, *pver;
    c_linklist_t *matches;

    /* build a list of all version #s of the package that are currently
       active */

    matches = C_linklist_create();
    C_linklist_set_destructor(matches, free);

    for(C_linklist_move_head(current_list.list);
        (pname = (const char *)C_linklist_restore(current_list.list)) != NULL;
        C_linklist_move_next(current_list.list))
    {
      pver = pkg_name_match(pname, pkg);
      if(pver)
        C_linklist_append(matches, C_string_dup(pver));
    }

    /* remove all matching versions from the environment */

    for(C_linklist_move_head(matches);
        (pver = C_linklist_restore(matches)) != NULL;
        C_linklist_move_next(matches))
    {
      /* ignore failures from this call */
      pkg_modify_explicit(pkgdir, pkgenvdir, pkg, pver, add);
    }

    C_linklist_destroy(matches);

    return(TRUE);
  }
}

/*
 */

static c_bool_t pkg_avail(const char *pkgenvdir, const char *pkg,
                          const char *ver, c_vector_t *vec, size_t *longest)
{
  DIR *dp;
  struct dirent *de;
  char *q;
  size_t len;
  c_bool_t found = FALSE;

  if(! (dp = opendir(pkgenvdir)))
    return(FALSE);

  while((de = readdir(dp)) != NULL)
  {
    char *cpkg = de->d_name;

    if(C_string_endswith(cpkg, PKGENV_EXT))
    {
      char *cver = NULL;

      C_string_rchop(cpkg, ".");

      q = strrchr(cpkg, '-');
      if(! q)
        continue;

      *q = NUL;
      cver = q + 1;

      if(pkg != NULL)
      {
        if(strcmp(pkg, cpkg))
          continue;

        if(ver != NULL)
        {
          if(strcmp(ver, cver))
            continue;
        }
      }

      *q = '-';

      found = TRUE;

      len = strlen(cpkg);
      if(len > *longest)
        *longest = len;

      if(! C_vector_contains(vec, cpkg))
        C_vector_store(vec, C_string_dup(cpkg));
    }
  }

  closedir(dp);

  return(found);
}

/*
 */

int main(int argc, char **argv)
{
  int ch, i, nargs;
  c_bool_t errflag = FALSE;
  char *cmd, *pkgdir = NULL, *pkgenvdir, *s, **v, **vp, *p;
  FILE *fp;

  C_error_init(*argv);

  if(! C_tty_getsize(&columns, &rows))
    columns = 80;

  while((ch = getopt(argc, argv, "hvVs:p:")) != EOF)
    switch((char)ch)
    {
      case 'h':
        C_error_printf("%s\n", HEADER);
        C_error_usage(USAGE);
        exit(EXIT_SUCCESS);
        break;

      case 'v':
        C_error_printf("%s\n", HEADER);
        exit(EXIT_SUCCESS);
        break;

      case 'V':
        verbose = TRUE;
        break;

      case 'p':
        if(! pkgdir)
          pkgdir = C_string_dup(optarg);
        else
          C_error_printf("Extra -p switch ignored");
        break;

      case 's':
        if(! shell)
          shell = C_string_dup(optarg);
        else
          C_error_printf("Extra -s switch ignored");
        break;

      default:
        errflag = TRUE;
    }

  if(errflag)
  {
    C_error_usage(USAGE);
    exit(EXIT_FAILURE);
  }

  if(! pkgdir)
  {
    s = getenv("PKGENV_ROOT");
    pkgdir = s ? C_string_dup(s) : DEFAULT_PKGDIR;
  }

  pkgenvdir = C_string_va_make(pkgdir, "/", PKGENV_DIR, NULL);

  if(! shell)
  {
    s = getenv("SHELL");
    shell = s ? s : DEFAULT_SHELL;
  }

  p = strrchr(shell, '/'); 
  if(p)
    shell = ++p;

  if(! (!strcmp(shell, "sh") || !strcmp(shell, "bash") || !strcmp(shell, "zsh")
        || !strcmp(shell, "ksh") || !strcmp(shell, "csh")
        || !strcmp(shell, "tcsh")))
  {
    C_error_printf("Unsupported shell: \"%s\"\n", shell);
    C_error_printf("Valid shells are: sh, bash, zsh, ksh, csh, tcsh\n");
    exit(EXIT_FAILURE);
  }

  /* parse the env settings */

  for(i = 0; i < C_lengthof(paths); ++i)
    path_parse(&paths[i]);

  path_parse(&current_list);

  /* do command processing */

  nargs = argc - optind;
  if(nargs < 1)
    return(EXIT_SUCCESS);

  varmap = C_hashtable_create(26);
  C_hashtable_set_destructor(varmap, free);

  cmd = argv[optind++];

  if(! strcmp(cmd, "add") && (nargs >= 2))
  {
    for(--nargs; nargs > 0; --nargs, ++optind)
    {
      char *ver = NULL;
      char *pkg = argv[optind];
      char *q = strrchr(pkg, '-');
      if(q)
      {
        if(isdigit(*(q + 1)))
        {
          *q = NUL;
          ver = q + 1;
        }
        else
          q = NULL;
      }

      if(! pkg_modify(pkgdir, pkgenvdir, pkg, ver, TRUE))
      {
        if(q)
          *q = '-';

        C_error_printf("can't find package: %s\n", pkg);
      }
    }
  }
  else if(! strcmp(cmd, "del") && (nargs >= 2))
  {
    for(--nargs; nargs > 0; --nargs, ++optind)
    {
      char *ver = NULL;
      char *pkg = argv[optind];
      char *q = strrchr(pkg, '-');
      if(q)
      {
        if(isdigit(*(q + 1)))
        {
          *q = NUL;
          ver = q + 1;
        }
        else
          q = NULL;
      }

      if(! pkg_modify(pkgdir, pkgenvdir, pkg, ver, FALSE))
      {
        if(q)
          *q = '-';
        C_error_printf("can't find package: %s\n", pkg);
      }
    }
  }
  else if(! strcmp(cmd, "avail") && (nargs >= 1))
  {
    size_t len, longest = 0;
    char **v;
    c_vector_t *vec = C_vector_start(40);
    --nargs;

    if(nargs == 0)
      pkg_avail(pkgenvdir, NULL, NULL, vec, &longest);
    else
    {
      for(; nargs > 0; --nargs, ++optind)
      {
        char *ver = NULL;
        char *pkg = argv[optind];
        char *q = strrchr(pkg, '-');
        if(q)
        {
          if(isdigit(*(q + 1)))
          {
            *q = NUL;
            ver = q + 1;
          }
          else
            q = NULL;
        }

        if(! pkg_avail(pkgenvdir, pkg, ver, vec, &longest))
        {
          if(q)
            *q = '-';
          C_error_printf("can't find package: %s\n", argv[optind]);
        }
      }
    }

    v = C_vector_end(vec, &len);
    C_string_sortvec(v, len);

    vec_print(v, len, longest);

    C_free_vec(v);
  }
  else if(! strcmp(cmd, "list") && (nargs == 1))
  {
    list_print(current_list.list);
  }
  else
  {
    C_error_usage(USAGE);
    return(EXIT_FAILURE);
  }

  /* write the new env settings back out */

  if(! C_system_cdhome())
  {
    C_error_printf("Unable to switch to home directory.\n");
    exit(EXIT_FAILURE);
  }

  if(! (fp = fopen(PKGENV_OUTFILE, "w")))
  {
    C_error_printf("Unable to write to ~/%s.\n", PKGENV_OUTFILE);
    exit(EXIT_FAILURE);
  }

  if(current_list.modified)
    path_print_setenv(fp, &current_list);

  for(i = 0; i < C_lengthof(paths); ++i)
  {
    if(paths[i].modified)
      path_print_setenv(fp, &paths[i]);
  }

  v = C_hashtable_keys(varmap, NULL);
  for(vp = v; *vp; ++vp)
  {
    const char *value = (char *)C_hashtable_restore(varmap, *vp);
    print_setenv(fp, *vp, value);
  }

  fclose(fp);

  C_hashtable_destroy(varmap);

  return(EXIT_SUCCESS);
}

/* end of source file */
