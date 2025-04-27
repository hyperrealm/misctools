/* ----------------------------------------------------------------------------
   djinn - UNIX daemon wrapper for Java programs
   Copyright (C) 1998-2025  Mark Lindner

   This file is part of misctools.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License, or (at your option) any later version.

   This software is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this software; if not, see
   <http://www.gnu.org/licenses/>.
   ----------------------------------------------------------------------------
*/

/* --- System Headers --- */

#include "config.h"

#include <getopt.h>
#include <stdio.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif /* HAVE_STDLIB_H */
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#include <stddef.h>
#include <string.h>
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif /* HAVE_SYS_TYPES_H */
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif /* HAVE_SYS_STAT_H */
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif /* HAVE_FCNTL_H */
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cbase/cbase.h>

/* --- Macros --- */

#define ENV_JAVA "JAVA"
#define ENV_JAVA_ARGS "JAVA_ARGS"
#define DEVNULL "/dev/null"
#define VM_NAME "java"

#define USAGE "[-p classpath] [-j jvm] [[-J vmarg] ...]\n\t" \
    "[-o file] [--version --help --verbose] class [arg ...]\n"
#define HEADER "djinn v" VERSION " - Mark Lindner\n"

#define HELP_OPTION 0
#define VERSION_OPTION 1
#define VERBOSE_OPTION 2

static const struct option options[] = { { "help", 0, NULL, 0  },
                                         { "version", 0, NULL, 0 },
                                         { "verbose", 0, NULL, 0 },
                                         { NULL, 0, NULL, 0 } };

static int verbose = 0;

/* --- Functions --- */

int main(int argc, char **argv)
{
  char *javavm = NULL, *classpath = NULL, *class = NULL, *logfile = NULL,
    **q, **xargs;
  c_vector_t *jargs, *vmargs, *args;
  int c, i, option_index, fd = -1, errflag = 0, vmargflag = 0;
  pid_t pid;

  C_error_init(*argv);

  vmargs = C_vector_start(20);

  /* parse the command line */

  while((c = getopt_long(argc, argv, "+J:j:o:p:", options, &option_index))
        != EOF)
  {
    switch(c)
    {
      case 'J':
        C_vector_store(vmargs, optarg);
        vmargflag = 1;
        break;

      case 'p':
        classpath = optarg;
        break;

      case 'j':
        javavm = optarg;
        break;

      case 'o':
        logfile = optarg;
        break;

      case 0:
      {
        switch(option_index)
        {
          case HELP_OPTION:
            C_error_printf("%s\n", HEADER);
            C_error_usage(USAGE);
            exit(EXIT_SUCCESS);
            break;

          case VERSION_OPTION:
            C_error_printf("%s\n", HEADER);
            exit(EXIT_SUCCESS);
            break;

          case VERBOSE_OPTION:
            verbose = 1;
            break;

          default:
            errflag = 1;
            break;
        }
        break;
      }

      default:
        errflag = 1;
        break;
    }
  }

  /* catch illegal option errors */

  if(errflag || (argc <= optind))
  {
    C_error_printf("%s\n", HEADER);
    C_error_usage(USAGE);
    exit(EXIT_FAILURE);
  }

  /* parse out class file */

  class = argv[optind++];

  jargs = C_vector_start(20);

  for(i = optind; i < argc; i++)
    C_vector_store(jargs, argv[i]);

  /* check for optional java args; first check command line, and if no
     -J switches were specified, resort to the environment variable
     JAVA_ARGS
  */

  if(! vmargflag)
  {
    char *p;
    char *args = getenv(ENV_JAVA_ARGS);

    if(args)
    {
      for(p = strtok(args, "\t "); p; p = strtok(NULL, "\t "))
        C_vector_store(vmargs, p);
    }
  }

  /* find the Java VM; first check the command line, and if the -j switch
     wasn't specified, resort to the environment variable JAVA
  */

  if(!javavm)
    javavm = getenv(ENV_JAVA);

  /* see if VM is valid */

  if(javavm)
    if(access(javavm, X_OK))
    {
      C_error_printf("Cannot execute JVM: %s\n", javavm);
      exit(EXIT_FAILURE);
    }

  /* construct the complete argument list */

  args = C_vector_start(20);

  // java [-classpath CLASSPATH] [VM ARGS] CLASS [ARGS]

  C_vector_store(args, VM_NAME);

  if(classpath != NULL)
  {
    // add classpath switch if classpath specified

    C_vector_store(args, "-classpath");
    C_vector_store(args, classpath);
  }

  // add any other args to the list

  if((q = C_vector_end(vmargs, NULL)))
    for(; *q; q++)
      C_vector_store(args, *q);

  C_vector_store(args, class);

  if((q = C_vector_end(jargs, NULL)))
    for(; *q; q++)
      C_vector_store(args, *q);

  xargs = C_vector_end(args, NULL);

  if(verbose)
  {
    C_error_printf("Invoking:");
    for(q = xargs; *q; q++)
      fprintf(stderr, " %s", *q);
    fputc('\n', stderr);
  }

  /* open logfile, if one specified */

  if(!logfile)
    logfile = C_string_dup(DEVNULL);

  if((fd = open(logfile, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR)) < 0)
  {
    C_error_printf("Unable to open logfile: %s\n", logfile);
    exit(EXIT_FAILURE);
  }
  else
  {
    if(verbose)
      C_error_printf("Using logfile: %s\n", logfile);
  }

  /* other miscellany */

  signal(SIGHUP, SIG_IGN);

  /* fork subprocess and run VM */

  pid = fork();
  switch(pid)
  {
    case 0:
    {
      /* child */

      (void)setsid();
      dup2(fd, STDOUT_FILENO);
      dup2(fd, STDERR_FILENO);

      if(! javavm)
        execvp(VM_NAME, xargs);
      else
        execvp(javavm, xargs);

      exit(EXIT_FAILURE);
    }

    case -1:
    {
      /* fork error */

      C_error_printf("Unable to fork subprocess.\n");
      exit(EXIT_FAILURE);
      break;
    }

    default:
    {
      int r;

      /* parent */

      if(verbose)
        C_error_printf("Daemon process started (pid: %i)\n", (int)pid);

      /* Wait 1 second and then try to collect the exit status from the
       * subprocess. If the JVM exited abnormally, we'll detect it and
       * notify the user.
       */

      sleep(1);
      if((waitpid(pid, &r, WNOHANG) == pid) && (r != 0))
      {
        C_error_printf("JVM exited abnormally with exit code %i\n",
                       WEXITSTATUS(r));
        exit(EXIT_FAILURE);
      }

      break;
    }
  }

  exit(EXIT_SUCCESS);
}

/* end of source file */
