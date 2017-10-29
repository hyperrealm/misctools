textlock_SOURCES = textlock.c
tlpasswd_SOURCES = tlpasswd.c
bin_PROGRAMS = textlock tlpasswd
AM_CFLAGS = -Wall
man_MANS = textlock.1
EXTRA_DIST = $(man_MANS)
noinst_HEADERS = textlock.h

textlock_CPPFLAGS = $(CBASE_CFLAGS)
tlpasswd_CPPFLAGS = $(CBASE_CFLAGS)
textlock_LDADD = $(CBASE_LIBS)
tlpasswd_LDADD = $(CBASE_LIBS)

