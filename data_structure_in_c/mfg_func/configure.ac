#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT(mfg_func, 0.1, XXXX@YYY.ZZZ)
AC_CONFIG_SRCDIR([include/mfg_queue.h])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC
AC_PROG_RANLIB
AM_INIT_AUTOMAKE

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([memset])
AC_CONFIG_FILES([Makefile common/Makefile mfg_funcs/Makefile mfg_funcs/AA/Makefile])
AC_OUTPUT
