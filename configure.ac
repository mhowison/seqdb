AC_PREREQ(2.61)
AC_INIT([seqdb], [0.1.1], [mhowison@brown.edu])
AC_CONFIG_HEADER(config.h)

AM_INIT_AUTOMAKE

AC_CANONICAL_HOST

# Checks for programs.
AC_PROG_INSTALL
AC_PROG_RANLIB
AC_PROG_CC
AC_LANG([C++])
AC_PROG_CXX

LIBS="-lhdf5 -lz $LIBS"

AC_ARG_WITH(
	[hdf5],
	[AS_HELP_STRING(
		[--with-hdf5=DIR],
		[Specify a non-standard HDF5 install path.])],
	[HDF5PATH=1],
	[HDF5PATH=0]
)
if test $HDF5PATH = 1 ; then
#	AM_CXXFLAGS="$AM_CXXFLAGS -I$with_hdf5/include"
	CPPFLAGS="$CPPFLAGS -I$with_hdf5/include"
#	AM_LDFLAGS="$AM_LDFLAGS -L$with_hdf5/lib"
	LDFLAGS="$LDFLAGS -L$with_hdf5/lib"
fi

# Checks for libraries.
AC_OPENMP
CPPFLAGS="$OPENMP_CXXFLAGS $CPPFLAGS"
LDFLAGS="$OPENMP_CXXFLAGS $LDFLAGS"

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h inttypes.h limits.h stdint.h stdlib.h string.h sys/time.h unistd.h])
AC_CHECK_HEADERS([hdf5.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_MMAP
AC_FUNC_SETVBUF_REVERSED
AC_FUNC_STAT
AC_FUNC_STRNLEN
AC_CHECK_FUNCS([gettimeofday memchr memset munmap strdup strerror])

AC_CONFIG_FILES([
	Makefile
	src/Makefile
	scripts/Makefile
	scripts/seqdb])
AC_OUTPUT

# Print summary.
AC_MSG_RESULT([ ])
AC_MSG_RESULT([Summary:])
AC_MSG_RESULT([ ])
AC_MSG_RESULT([Host OS:             $host_os])
AC_MSG_RESULT([Host CPU:            $host_cpu])
AC_MSG_RESULT([Host vendor:         $host_vendor])
AC_MSG_RESULT([Install dir:         $prefix])
AC_MSG_RESULT([CC =                 $CC])
AC_MSG_RESULT([CXX =                $CXX])
AC_MSG_RESULT([CPPFLAGS =           $CPPFLAGS])
AC_MSG_RESULT([LDFLAGS =            $LDFLAGS])
AC_MSG_RESULT([LIBS =               $LIBS])
AC_MSG_RESULT([ ])

