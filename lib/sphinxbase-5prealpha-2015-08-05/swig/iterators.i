// Macro to construct iterable objects.
%define sb_iterator(TYPE, PREFIX, VALUE_TYPE)

#if SWIGJAVA
%typemap(javainterfaces) TYPE##Iterator "java.util.Iterator<"#VALUE_TYPE">"
%typemap(javabody) TYPE##Iterator %{

  private long swigCPtr;
  protected boolean swigCMemOwn;

  public $javaclassname(long cPtr, boolean cMemoryOwn) {
    swigCMemOwn = cMemoryOwn;
    swigCPtr = cPtr;
  }

  public static long getCPtr($javaclassname obj) {
    return (obj == null) ? 0 : obj.swigCPtr;
  }

  @Override
  public void remove() {
    throw new UnsupportedOperationException();
  }
%}
#endif

// Basic types

%inline %{
typedef struct {
  PREFIX##_t *ptr;
} TYPE##Iterator;
%}

// Exception to end iteration

#if SWIGJAVA
%exception TYPE##Iterator##::next() {
  if (!arg1->ptr) {
    jclass cls = (*jenv)->FindClass(jenv, "java/util/NoSuchElementException");
    (*jenv)->ThrowNew(jenv, cls, NULL);
    return $null;
  }
  $action;
}
#elif SWIGPYTHON
%exception TYPE##Iterator##::next() {
  if (!arg1->ptr) {
    SWIG_SetErrorObj(PyExc_StopIteration, SWIG_Py_Void());
    SWIG_fail;
  }
  $action;
}
%exception TYPE##Iterator##::__next__() {
  if (!arg1->ptr) {
    SWIG_SetErrorObj(PyExc_StopIteration, SWIG_Py_Void());
    SWIG_fail;
  }
  $action;
}
#endif

// Implementation of the iterator itself

%extend TYPE##Iterator {
  TYPE##Iterator(PREFIX##_t *ptr) {
    TYPE##Iterator *iter = ckd_malloc(sizeof *iter);
    iter->ptr = ptr;
    return iter;
  }

  ~TYPE##Iterator() {
    if ($self->ptr)
	PREFIX##_free($self->ptr);
    ckd_free($self);
  }

#if SWIGJAVA
  %newobject next;
  VALUE_TYPE * next() {
    if ($self->ptr) {
      VALUE_TYPE *value = ##VALUE_TYPE##_fromIter($self->ptr);
      $self->ptr = ##PREFIX##_next($self->ptr);
      return value;
    }

    return NULL;
  }
  bool hasNext() {
    return $self->ptr != NULL;
  }
#endif
#if SWIGPYTHON

  // Python2
  %newobject next;
  VALUE_TYPE * next() {
    if ($self->ptr) {
      VALUE_TYPE *value = ##VALUE_TYPE##_fromIter($self->ptr);
      $self->ptr = ##PREFIX##_next($self->ptr);
      return value;
    }
    return NULL;
  }

  // Python3
  %newobject __next__;
  VALUE_TYPE * __next__() {
    if ($self->ptr) {
      VALUE_TYPE *value = ##VALUE_TYPE##_fromIter($self->ptr);
      $self->ptr = ##PREFIX##_next($self->ptr);
      return value;
    }
    return NULL;
  }
#endif

}

%enddef


%define sb_iterable(TYPE, PREFIX, VALUE_TYPE)

// Methods to retrieve the iterator from the container

%extend TYPE {
  // Also used in Java, but underscores are automatically removed
  TYPE##Iterator * __iter__() {
    return new_##TYPE##Iterator(PREFIX##($self));
  }
}

sb_iterable_java(TYPE, VALUE_TYPE)

%enddef

%define sb_iterable_java(TYPE, VALUE_TYPE)

// Same but without __iter__ which can vary from class to class

#if SWIGJAVA
%typemap(javainterfaces) TYPE "Iterable<"#VALUE_TYPE">"
%typemap(javabody) TYPE %{

  private long swigCPtr;
  protected boolean swigCMemOwn;

  public $javaclassname(long cPtr, boolean cMemoryOwn) {
    swigCMemOwn = cMemoryOwn;
    swigCPtr = cPtr;
  }

  public static long getCPtr($javaclassname obj) {
    return (obj == null) ? 0 : obj.swigCPtr;
  }

  @Override
  public java.util.Iterator<VALUE_TYPE> iterator () {
    return iter();
  }
%}
%javamethodmodifiers TYPE::__iter__ "private";
#endif

%enddef
