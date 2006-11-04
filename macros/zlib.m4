dnl  
dnl    Copyright (C) 2005, 2006 Free Software Foundation, Inc.
dnl  
dnl  This program is free software; you can redistribute it and/or modify
dnl  it under the terms of the GNU General Public License as published by
dnl  the Free Software Foundation; either version 2 of the License, or
dnl  (at your option) any later version.
dnl  
dnl  This program is distributed in the hope that it will be useful,
dnl  but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl  GNU General Public License for more details.
dnl  You should have received a copy of the GNU General Public License
dnl  along with this program; if not, write to the Free Software
dnl  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

dnl $Id: zlib.m4,v 1.13 2006/11/04 09:04:35 nihilus Exp $

AC_DEFUN([GNASH_PATH_ZLIB],
[
  dnl Lool for the header
  AC_ARG_WITH(zlib_incl, AC_HELP_STRING([--with-zlib-incl], [directory where zlib header is]), with_zlib_incl=${withval})
  AC_CACHE_VAL(ac_cv_path_zlib_incl,[
  if test x"${with_zlib_incl}" != x ; then
    if test -f ${with_zlib_incl}/zlib.h ; then
      ac_cv_path_zlib_incl=`(cd ${with_zlib_incl}; pwd)`
    elif test -f ${with_zlib_incl}/zlib.h ; then
      ac_cv_path_zlib_incl=`(cd ${with_zlib_incl}; pwd)`
    else
      AC_MSG_ERROR([${with_zlib_incl} directory doesn't contain zlib.h])
    fi
  fi
  ])

  if test x"${ac_cv_path_zlib_incl}" = x; then
    AC_CHECK_HEADERS(zlib.h, [ac_cv_path_zlib_incl=""],[
    if test x"${ac_cv_path_zlib_incl}" = x; then
      AC_MSG_CHECKING([additional locations for zlib header])
      incllist="${prefix}/include /sw/include /usr/local/include /home/latest/include /opt/include /opt/local/include /usr/include /usr/pkg/include .. ../.."
      for i in $incllist; do
        if test -f $i/png.h; then
	  if test x"$i" != x"/usr/include"; then
	    ac_cv_path_zlib_incl="$i"
	    break
          else
	    ac_cv_path_zlib_incl=""
	    break
	  fi
        fi
      done
      AC_MSG_RESULT(${ac_cv_path_zlib_incl})
    fi])
  else
    if test x"${ac_cv_path_zlib_incl}" != x"/usr/include"; then
      ac_cv_path_zlib_incl="${ac_cv_path_zlib_incl}"
    else
      ac_cv_path_zlib_incl=""
    fi
  fi

  if test x"${ac_cv_path_zlib_incl}" != x ; then
    ZLIB_CFLAGS="-I${ac_cv_path_zlib_incl}"
  else
    ZLIB_CFLAGS=""
  fi

  dnl Look for the library
  AC_ARG_WITH(zlib_lib, AC_HELP_STRING([--with-zlib-lib], [directory where zlib library is]), with_zlib_lib=${withval})
  AC_MSG_CHECKING([for zlib library])
  AC_CACHE_VAL(ac_cv_path_zlib_lib,[
  if test x"${with_zlib_lib}" != x ; then
    if test -f ${with_zlib_lib}/libz.a ; then
      ac_cv_path_zlib_lib=`(cd ${with_zlib_lib}; pwd)`
    elif test -f ${with_zlib_lib}/libz.a -o -f ${with_zlib_lib}/libz.so -o -f ${with_zlib_lib}/libz.dylib; then
      ac_cv_path_zlib_lib=`(cd ${with_zlib_incl}; pwd)`
    else
      AC_MSG_ERROR([${with_zlib_lib} directory doesn't contain libz.a])
    fi
  fi
  AC_MSG_RESULT(yes)
  ])

  dnl If the header doesn't exist, there is no point looking for the library.
  if test x"${ac_cv_path_zlib_lib}" = x; then
    AC_CHECK_LIB(z, zlibVersion, [ac_cv_path_zlib_lib="-lz"],[
    AC_MSG_CHECKING([for zlib library])
      libslist="${prefix}/lib64 ${prefix}/lib /usr/lib64 /usr/lib /sw/lib /usr/local/lib /home/latest/lib /opt/lib /opt/local/lib /usr/pkg/lib /usr/X11R6/lib .. ../.."
      for i in $libslist; do
        if test -f $i/libz.a -o -f $i/libz.so; then
          if test x"$i" != x"/usr/lib"; then
	    ac_cv_path_zlib_lib="-L$i -lz"
            AC_MSG_RESULT(${ac_cv_path_zlib_lib})
	    break
          else
	    ac_cv_path_zlib_lib="-lz"
            AC_MSG_RESULT(yes)
	    break
	  fi
	fi
      done])
  fi

  if test x"${ac_cv_path_zlib_lib}" != x ; then
    ZLIB_LIBS="${ac_cv_path_zlib_lib}"
  else
    ZLIB_LIBS=""
  fi

  AC_SUBST(ZLIB_CFLAGS)
  AC_SUBST(ZLIB_LIBS)
])
