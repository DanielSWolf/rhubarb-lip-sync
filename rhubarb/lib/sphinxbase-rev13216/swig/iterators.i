// Macro to construct iterable objects.
%define sb_iterator(TYPE, PREFIX, VALUE_TYPE)

#if !SWIGRUBY

#if SWIGJAVA
%typemap(javainterfaces) TYPE##Iterator "java.util.Iterator<"#VALUE_TYPE">"
%typemap(javacode) TYPE##Iterator %{
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
    TYPE##Iterator *iter = (TYPE##Iterator *)ckd_malloc(sizeof *iter);
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
#elif SWIGJAVASCRIPT
  %newobject next;
  VALUE_TYPE * next() {
    if ($self->ptr) {
      VALUE_TYPE *value = ##VALUE_TYPE##_fromIter($self->ptr);
      $self->ptr = ##PREFIX##_next($self->ptr);
      return value;
    }

    return NULL;
  }
#elif SWIGPYTHON
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

#endif // SWIGRUBY

%enddef


%define sb_iterable(TYPE, ITER_TYPE, PREFIX, INIT_PREFIX, VALUE_TYPE)

// Methods to retrieve the iterator from the container

#if SWIGJAVA
%typemap(javainterfaces) TYPE "Iterable<"#VALUE_TYPE">"
#endif

%extend TYPE {
  
#if SWIGRUBY  
  void each() {
    ##PREFIX##_t *iter = INIT_PREFIX##($self);
    while (iter) {
      VALUE_TYPE *value = ##VALUE_TYPE##_fromIter(iter);
      rb_yield(SWIG_NewPointerObj(SWIG_as_voidptr(value), SWIGTYPE_p_##VALUE_TYPE##, 0 |  0 ));
      iter = PREFIX##_next(iter);
    }
    return;
  }

  void each(int count) {
    ##PREFIX##_t *iter = INIT_PREFIX##($self);
    int cnt = 0;
    while (iter && cnt < count) {
      VALUE_TYPE *value = ##VALUE_TYPE##_fromIter(iter);
      rb_yield(SWIG_NewPointerObj(SWIG_as_voidptr(value), SWIGTYPE_p_##VALUE_TYPE##, 0 |  0 ));
      iter = PREFIX##_next(iter);
      cnt++;
    }
    if (iter)
	PREFIX##_free(iter);
    return;
  }

#elif SWIGJAVA
  %newobject iterator;
  ITER_TYPE##Iterator * iterator() {
    return new_##ITER_TYPE##Iterator(INIT_PREFIX##($self));
  }

#else  /* PYTHON, JS */
  %newobject __iter__;
  ITER_TYPE##Iterator * __iter__() {
    return new_##ITER_TYPE##Iterator(INIT_PREFIX##($self));
  }  
#endif 

}

%enddef
