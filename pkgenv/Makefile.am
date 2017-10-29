pkgenv_SOURCES = pkgenv.c
bin_PROGRAMS = pkgenv

man_MANS = pkgenv.1
EXTRA_DIST = $(man_MANS) automake_patch.txt

pkgenv_CPPFLAGS = $(CBASE_CFLAGS) -DDEBUG -Wall
pkgenv_LDADD = $(CBASE_LIBS)

