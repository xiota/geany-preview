AC_INIT([preview], [0.01.1], [xiota])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([src/plugin.cc])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([subdir-objects -Wall -Werror foreign])
AM_SILENT_RULES([yes])
AM_PROG_AR
LT_INIT([disable-static pic-only])
#AC_PROG_CC_C99
AC_PROG_CXX
AC_PROG_CXXCPP
AC_PROG_CXX_C_O
PKG_CHECK_MODULES([GEANY], [geany >= 1.23])
PKG_CHECK_MODULES([WEBKIT], [webkit2gtk-4.0])
PKG_CHECK_MODULES([CMARK], [libcmark-gfm])
AC_CONFIG_FILES([Makefile compile_commands.json])
AC_OUTPUT
