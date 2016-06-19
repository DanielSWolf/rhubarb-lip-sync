%include <exception.i>

%apply int {int32};
%apply double {float64};

#if SWIGJAVA

%include <arrays_java.i>

// Raw data return support
%typemap(in, numinputs=0, noblock=1) int32 *RAWDATA_SIZE {
   int32 temp_len;
   $1 = &temp_len;
}
%typemap(jstype) int16 *get_rawdata "short[]"
%typemap(jtype) int16 *get_rawdata "short[]"
%typemap(jni) int16 *get_rawdata "jshortArray"
%typemap(javaout) int16 *get_rawdata {
  return $jnicall;
}
%typemap(out) int16 *get_rawdata {
  $result = JCALL1(NewShortArray, jenv, temp_len);
  JCALL4(SetShortArrayRegion, jenv, $result, 0, temp_len, $1);
}

// Special typemap for arrays of audio.
%apply short[] {const int16 *SDATA};

#endif

// Define typemaps to wrap error codes returned by some functions,
// into runtime exceptions.
%typemap(in, numinputs=0, noblock=1) int *errcode {
  int errcode;
  $1 = &errcode;
}

%typemap(argout) int *errcode {
  if (*$1 < 0) {
    char buf[64];
    sprintf(buf, "$symname returned %d", *$1);
    SWIG_exception(SWIG_RuntimeError, buf);
  }
}

