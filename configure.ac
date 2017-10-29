
AC_INIT(misctools, 2.6.1, markl@gnu.org)
AC_CONFIG_AUX_DIR([aux-build])
AM_INIT_AUTOMAKE
AM_CONFIG_HEADER([config.h])

AC_PROG_MAKE_SET

dnl Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_INSTALL

dnl Checks for libraries.
AC_CHECK_LIB(ncurses, initscr)
AC_CHECK_LIB(crypt, crypt)

AC_MSG_CHECKING([whether markl gets enough sleep])
sleep 2
AC_MSG_RESULT(no)

AC_DEFUN([PKG_CHECK_MODULES_STATIC],
  [AC_REQUIRE([PKG_PROG_PKG_CONFIG])dnl
  _save_PKG_CONFIG=$PKG_CONFIG
  PKG_CONFIG="$PKG_CONFIG --static"
  PKG_CHECK_MODULES([$1], [$2], [$3], [$4], [$5])
  PKG_CONFIG=$_save_PKG_CONFIG[]dnl
])

PKG_CHECK_MODULES_STATIC([CBASE], [libcbase >= 1.3.4])

dnl Checks for header files.
AC_HEADER_STDC
AC_HEADER_DIRENT
AC_HEADER_MAJOR
AC_CHECK_HEADERS([sys/time.h sys/param.h utime.h fcntl.h unistd.h ncurses.h ncurses/ncurses.h])
AC_CHECK_HEADERS([syslog.h])

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_OFF_T
AC_CHECK_SIZEOF(off_t)
AC_CHECK_SIZEOF(ino_t)
AC_TYPE_PID_T
AC_TYPE_SIGNAL
AC_STRUCT_ST_RDEV
AC_STRUCT_TM

dnl Checks for library functions.
AC_FUNC_CHOWN
AC_FUNC_STRFTIME
AC_FUNC_STAT
AC_FUNC_UTIME_NULL
AC_CHECK_FUNCS([utime mkfifo uname])

AC_SUBST(RELEASE_DATE, '21 Jun 2011')

AC_OUTPUT(Makefile
	basecvt/Makefile basecvt/basecvt.1
	bat/Makefile bat/bat.1
	bin2c/Makefile bin2c/bin2c.1
	cpmod/Makefile cpmod/cpmod.1
	dirstack/Makefile dirstack/dirstack.1
	dirtree/Makefile dirtree/dirtree.1
	djinn/Makefile djinn/djinn.1
	ftrunc/Makefile ftrunc/ftrunc.1
	pascii/Makefile pascii/pascii.1
	pkgenv/Makefile pkgenv/pkgenv.1
	ranline/Makefile ranline/ranline.1
	stat/Makefile stat/stat.1
	textlock/Makefile textlock/textlock.1
	wrap/Makefile wrap/wrap.1
	scripts/Makefile scripts/man2ps.1 scripts/ps2pdf.1 scripts/psprint.1
	scripts/man2pdf.1 scripts/texi2pdf.1 scripts/texi2ps.1)
