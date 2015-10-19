dnl Welcome to the Sphinx automated build system.
dnl try not to hurt yourself ;)

AC_INIT([sphinxbase], 5prealpha)
AC_CONFIG_HEADERS([include/config.h include/sphinx_config.h])
AM_INIT_AUTOMAKE([no-define foreign])
AC_CONFIG_MACRO_DIR([m4])

CFLAGS=${CFLAGS:--g -O2 -Wall}

AC_CANONICAL_HOST
AC_ISC_POSIX
AC_PROG_CC
AC_PROG_CC_STDC

LT_INIT

# We don't bother ourselves with flex checks until the rest of the world has an up-to-date Flex
# We need specific bison extensions, so check for bison
AC_CHECK_PROG(YACC, bison, [bison -y], no)
if test "x$YACC" = "xno";
then
    AC_MSG_ERROR([You need to install bison])
fi
AC_ARG_VAR(YACC,[Bison command])

AC_HEADER_STDC
AC_C_BIGENDIAN
AC_TYPE_SIGNAL
AC_CHECK_TYPES(long long)
AC_CHECK_SIZEOF(long long)
AC_CHECK_SIZEOF(long)
AC_CHECK_FUNCS([popen perror snprintf])
AC_CHECK_HEADER(errno.h)

dnl
dnl Check for Lapack stuff unless disabled
dnl
use_lapack=true
use_internal_lapack=false
AC_ARG_WITH(lapack,
	AS_HELP_STRING([--without-lapack],
		       [Disable matrix algebra support (depends on LAPACK)]),[
if test x$withval = xno; then
   use_lapack=false
fi
])
if test x$use_lapack = xtrue; then
   AC_CHECK_LIB(lapack, sgesv_,,use_internal_lapack=true)
   if test x$use_internal_lapack != xtrue; then
      AC_CHECK_LIB(blas, sgemm_,,use_internal_lapack=true)
   fi
   AC_DEFINE(WITH_LAPACK, [], [Enable matrix algebra with LAPACK])
fi
AM_CONDITIONAL(LAPACK_LITE, test x$use_internal_lapack = xtrue)
AM_CONDITIONAL(USE_LAPACK, test x$use_lapack = xtrue)

dnl
dnl Just make sure we have this...
dnl
AC_CHECK_LIB(m, log)

dnl
dnl Enable thread-safe versions of some internal functions
dnl
threadsafe=auto
AC_ARG_ENABLE(threads,
	AS_HELP_STRING([--disable-threads],
		       [Disable thread-safe versions of memory allocation functions (default: check)]),[
if test x$enableval != x; then
   threadsafe=$enableval
fi
])
if test x$threadsafe != xno; then
   threads=false
   dnl
   dnl Can't seem to check for Windows API functions, but that's okay
   dnl because we know they will be there...
   dnl
   case $host in
     *cygwin*|*mingw*|*wince*)
	threads=true
     ;;
     *)
        AC_CHECK_HEADERS([pthread.h], [threads=true])
	AC_CHECK_LIB(pthread, pthread_create)
     ;;
   esac
   if test x$threads = xtrue; then
     AC_DEFINE(ENABLE_THREADS, [], [Enable thread safety])
     threadsafe=yes
   elif test x$threadsafe = xyes; then
     AC_MSG_ERROR(--enable-threads was given but test for thread functions failed!)
   else
     threadsafe=no
   fi
fi
AM_CONDITIONAL(USE_THREADS, test x$threadsafe = xyes)

dnl
dnl Allow compilation for fixed or floating-point MFCC and GMM computation
dnl
fixed_point=false
AC_ARG_ENABLE(fixed,
	AS_HELP_STRING([--enable-fixed=RADIX],
		       [Use 32-bit fixed-point for MFCC and GMM computation,
		        optionally specifying a radix point]),[
if test x$enableval = xyes; then
   AC_DEFINE(FIXED_POINT)
   fixed_point=true
else
   AC_DEFINE(FIXED_POINT, [], [Use fixed-point computation])
   AC_DEFINE_UNQUOTED(DEFAULT_RADIX,$enableval,[Default radix point for fixed-point])
fi])
AM_CONDITIONAL(FIXED_POINT, test x$fixed_point = xtrue)

dnl
dnl Enable debugging
dnl
AC_ARG_ENABLE(debug,
	AS_HELP_STRING([--enable-debug],
			[Enable debugging output]),
	AC_DEFINE(SPHINX_DEBUG, [],
	          [Enable debugging output]))

dnl
dnl determine audio type or use none if none supported on this platform
dnl
case $host in
	*-*-linux*|*-*-uclinux*|*-*-freebsd*|*-*-netbsd*|*-*-openbsd*)
		# Check for PulseAudio, JACK, then fallback ALSA, then fall back to OSS
		AC_CHECK_HEADER(pulse/pulseaudio.h,[
			ad_files="ad_pulse.lo"
			AC_CHECK_LIB(pulse-simple, pa_simple_new)
			AC_CHECK_LIB(pulse, pa_strerror)
                        ],[
                AC_CHECK_HEADER(jack/jack.h,[
                        ad_files="ad_jack.lo"
                        AC_CHECK_LIB(jack, jack_connect)],[
		AC_CHECK_HEADER(alsa/asoundlib.h,[
			ad_files="ad_alsa.lo"
			ad_backend="AD_BACKEND_ALSA"
			AC_CHECK_LIB(asound, snd_pcm_open)],[
			ad_files="ad_oss.lo"
			])
		])
                ])
		;;
	*-*-*cygwin*|*-*-*mingw*)
		ad_files="ad_win32.lo"
		ad_libs="-lwinmm"
		;;
	*-apple-*darwin*)
		sdkparam=`xcodebuild -showsdks | awk '/^$/{p=0};p; /OS X SDKs:/{p=1}' | tail -1 | cut -f3`
		sdkpath=`xcodebuild -version $sdkparam Path`
		ad_cppflags="-I$sdkpath/System/Library/Frameworks/OpenAL.framework/Versions/A/Headers/"
		backup_CPPFLAGS="$CPPFLAGS"
		CPPFLAGS="$CPPFLAGS $ad_cppflags"
		AC_CHECK_HEADER("alc.h", [], AC_MSG_ERROR([OpenAL not found]))
		CPPFLAGS="$backup_CPPFLAGS"
		ad_files="ad_openal.lo"
		ad_libs="-framework OpenAL"
		;;
	*)
		ad_files="ad_base.lo"
		AC_MSG_RESULT([No audio interface for host type $host.])
		;;
esac

AC_SUBST(ad_cppflags)
AC_SUBST(ad_files)
AC_SUBST(ad_libs)

dnl
dnl Check for Doxygen, and build dox if present
dnl
AC_CHECK_PROG(HAVE_DOXYGEN, doxygen, yes, no)
AM_CONDITIONAL(BUILD_DOXYGEN, test "x$HAVE_DOXYGEN" = "xyes")


dnl Check for SWIG and Python
AC_ARG_WITH(python,
	AS_HELP_STRING([--with-python],
                 [Enable Python extension, built with swig, enabled by default]))

if test "x${with_python}" != "xno"; then
  AM_PATH_PYTHON([2.6])
  AX_PYTHON_DEVEL()
  AX_PKG_SWIG(2.0, [], [AC_MSG_ERROR([swig not found])])
fi

AC_SUBST([PYTHON_CPPFLAGS])
AM_CONDITIONAL(BUILD_SWIG, test "x$SWIG" != "x")
dnl swig python check


AC_CONFIG_FILES([
sphinxbase.pc
Makefile
include/Makefile
include/sphinxbase/Makefile
src/Makefile
src/libsphinxad/Makefile
src/libsphinxbase/Makefile
src/libsphinxbase/util/Makefile
src/libsphinxbase/feat/Makefile
src/libsphinxbase/fe/Makefile
src/libsphinxbase/lm/Makefile
src/sphinx_fe/Makefile
src/sphinx_cepview/Makefile
src/sphinx_jsgf2fsg/Makefile
src/sphinx_adtools/Makefile
src/sphinx_lmtools/Makefile
doc/Makefile
doc/doxyfile
test/Makefile
test/unit/testfuncs.sh
test/unit/Makefile
test/unit/test_alloc/Makefile
test/unit/test_bitvec/Makefile
test/unit/test_case/Makefile
test/unit/test_string/Makefile
test/unit/test_cmdln/Makefile
test/unit/test_hash/Makefile
test/unit/test_matrix/Makefile
test/unit/test_feat/Makefile
test/unit/test_fe/Makefile
test/unit/test_logmath/Makefile
test/unit/test_lineiter/Makefile
test/unit/test_ngram/Makefile
test/unit/test_fsg/Makefile
test/unit/test_thread/Makefile
test/unit/test_util/Makefile
test/regression/testfuncs.sh
test/regression/Makefile
swig/Makefile
swig/python/Makefile
])
AC_OUTPUT
