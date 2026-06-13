/* Minimal config.h for building opus as a bundled static library.
   Platform-specific intrinsics (ARM NEON, x86 SSE) are intentionally
   left disabled so the same file works on all three CI platforms. */

#define OPUS_BUILD 1
#define PACKAGE_VERSION "1.5.2"

/* Use the floating-point API (required by opusfile) */
#undef FIXED_POINT
#undef FLOAT_APPROX

/* Standard-library availability (safe to assume on all targets) */
#define HAVE_LRINT  1
#define HAVE_LRINTF 1
#define HAVE_STDINT_H 1
#define HAVE_STDLIB_H 1
#define HAVE_STRING_H 1

/* Stack allocation */
#define USE_ALLOCA 1
