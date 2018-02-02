.PHONY: build clean

# directories
ifeq ($(realpath .),)
  $(error your version of Make doesn't support $$(realpath names...) - please use GNU Make 3.81 or later)
endif

ifeq ($(platform),)
  __uname_s := $(shell sh -c 'uname -s 2>/dev/null | tr [A-Z] [a-z] || echo unknown-platform')
  __uname_m := $(shell sh -c 'uname -m 2>/dev/null | tr [A-Z] [a-z] || echo unknown-architecture')

  ifeq ($(__uname_s),linux)
    override platform := linux
    override architecture := $(__uname_m)
  endif
  ifeq ($(__uname_s),darwin)
    override platform := mac
    override architecture := $(__uname_m)
  endif
  ifeq ($(findstring mingw,$(__uname_s)),mingw)
    override platform := windows
    override architecture := $(if $(findstring MINGW32,$(MSYSTEM)),i686,$(if $(findstring MINGW64,$(MSYSTEM)),x86_64,))
    ifeq ($(CC),cc)
      override CC := gcc
    endif
  endif
  ifeq ($(__uname_s),freebsd)
    override platform := freebsd
    override architecture := $(__uname_m)
  endif
endif
ifeq ($(architecture),)
  override architecture := unknown-architecture
endif

prefix := $(realpath ..)
srcdir := $(realpath ../src)
exampledir := $(realpath ../example)
testdir := $(realpath ../test)
buildir := $(realpath .)/build
binsubdir := $(platform)-$(architecture)
bindir := $(prefix)/bin/$(binsubdir)

CFLAGS := -O2 -g -Wall -pedantic -Werror -std=c99
CXXFLAGS := -O2 -g -Wall -pedantic -Werror

ifeq ($(platform),linux)
LDFLAGS += -ldl
CFLAGS += -D_XOPEN_SOURCE=500 -fpic
CXXFLAGS += -fpic
endif
ifeq ($(platform),mac)
CFLAGS += -D_DARWIN_C_SOURCE
endif
ifeq ($(platform),freebsd)
CFLAGS += -fpic
CXXFLAGS += -fpic
endif

ifeq ($(platform),mac)
libsuffix := .dylib
endif
ifeq ($(platform),linux)
libsuffix := .so
endif
ifeq ($(platform),windows)
libsuffix := .dll
endif
ifeq ($(platform),freebsd)
libsuffix := .so
endif

.PHONY: build-executable
build: build-executable
build-executable: $(bindir)/executable $(bindir)/executable-cpp

$(bindir)/executable: $(srcdir)/whereami.c $(srcdir)/whereami.h $(exampledir)/executable.c
	mkdir -p $(@D)
	$(CC) -o $@ -I $(srcdir) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) $(filter-out %.h,$^)
	$(if $(postbuild),$(postbuild) $@)

$(bindir)/executable-cpp: $(srcdir)/whereami.c $(srcdir)/whereami.h $(exampledir)/executable.c
	mkdir -p $(@D)
	$(CXX) -x c++ -o $@ -I $(srcdir) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) $(filter-out %.h,$^)
	$(if $(postbuild),$(postbuild) $@)

.PHONY: build-library
build: build-library
build-library: $(bindir)/library$(libsuffix) $(bindir)/library-cpp$(libsuffix)

$(bindir)/library$(libsuffix): $(srcdir)/whereami.c $(srcdir)/whereami.h $(exampledir)/library.c
	mkdir -p $(@D)
	$(CC) -shared -o $@ -I $(srcdir) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) $(filter-out %.h,$^)
	$(if $(postbuild),$(postbuild) $@)

$(bindir)/library-cpp$(libsuffix): $(srcdir)/whereami.c $(srcdir)/whereami.h $(exampledir)/library.c
	mkdir -p $(@D)
	$(CXX) -x c++ -shared -o $@ -I $(srcdir) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) $(filter-out %.h,$^)
	$(if $(postbuild),$(postbuild) $@)

clean:
	rm -rf $(buildir)
	rm -rf $(bindir)
