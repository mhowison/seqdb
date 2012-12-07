AC_PREREQ(2.69)
AC_INIT([seqdb], [0.2.0], [mhowison@brown.edu])
AC_CONFIG_HEADER(config.h)
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE
AM_SILENT_RULES([yes])

AC_CANONICAL_HOST

# Set default optimization to O3
if test -z $CFLAGS; then
    CFLAGS="-O3 -g3"
fi
if test -z $CXXFLAGS; then
    CXXFLAGS="-O3 -g3"
fi

# Checks for programs.
AC_PROG_MAKE_SET
AC_PROG_INSTALL
AC_PROG_LIBTOOL
AC_PROG_CC
AC_LANG([C++])
AC_PROG_CXX

# Checks for libraries.
AX_LIB_HDF5
if test $with_hdf5 = "no"; then
	AC_MSG_ERROR([could not locate HDF5 library])
fi

AC_OPENMP

CPPFLAGS="$HDF5_CPPFLAGS $CPPFLAGS"
CXXFLAGS="$OPENMP_CXXFLAGS $CXXFLAGS"
LDFLAGS="$HDF5_LDFLAGS $OPENMP_LDFLAGS $LDFLAGS"

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
AC_CHECK_FUNCS([pthread_setaffinity_np])

# Version information.
PACKAGE_BUILD_HOST=`uname -n`
PACKAGE_BUILD_HOST_OS=`uname -srm`
PACKAGE_BUILD_DATE=`date`
PACKAGE_COMPILER=`$CXX --version | head -1`
PACKAGE_COMPILER="$CXX (version: $PACKAGE_COMPILER)"
PACKAGE_COMPILE_OPTIONS="$CXXFLAGS"

AC_SUBST(PACKAGE_BUILD_HOST)
AC_SUBST(PACKAGE_BUILD_HOST_OS)
AC_SUBST(PACKAGE_BUILD_DATE)
AC_SUBST(PACKAGE_COMPILER)
AC_SUBST(PACKAGE_COMPILE_OPTIONS)

AC_CONFIG_FILES([
	Makefile
	src/Makefile
	src/seqdb-version.h
	src/seqdb.pc
	scripts/Makefile
	scripts/seqdb])
AC_OUTPUT

# Print summary.
AC_MSG_RESULT([ ])
AC_MSG_RESULT([Summary:])
AC_MSG_RESULT([ ])
AC_MSG_RESULT([Build OS:     $PACKAGE_BUILD_HOST_OS])
AC_MSG_RESULT([Build host:   $PACKAGE_BUILD_HOST])
AC_MSG_RESULT([Install dir:  $prefix])
AC_MSG_RESULT([CC       = $CC])
AC_MSG_RESULT([CXX      = $CXX])
AC_MSG_RESULT([CFLAGS   = $CFLAGS])
AC_MSG_RESULT([CXXFLAGS = $CXXFLAGS])
AC_MSG_RESULT([CPPFLAGS = $HDF5_CPPFLAGS])
AC_MSG_RESULT([LDFLAGS  = $LDFLAGS])
AC_MSG_RESULT([LIBS     = $LIBS])
AC_MSG_RESULT([ ])

