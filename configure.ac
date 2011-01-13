#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.67])
AC_INIT([cli], 1.0.0002, [schweizer@alumni.cmu.edu])
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([cli.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h fcntl.h netinet/in.h stdlib.h string.h sys/socket.h unistd.h])
AC_CHECK_HEADERS([stdio.h dirent.h pwd.h signal.h ctype.h unistd.h sys/errno.h sys/stat.h])
AC_CHECK_HEADERS([sys/types.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_HEADER_STDBOOL
AC_TYPE_UID_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_STAT
AC_CHECK_FUNCS([getcwd memset mkdir select socket strerror sigaction])
AC_CHECK_FUNCS([symlink getpid])

AC_CHECK_TYPES([pthread_t, pthread_mutex_t])
AC_CHECK_HEADERS([curses.h pthread.h])

AC_CHECK_LIB(pthread, pthread_create)
AC_CHECK_LIB(ncurses, getch)
AC_CHECK_LIB(archive, archive_read_new)

AC_OUTPUT
