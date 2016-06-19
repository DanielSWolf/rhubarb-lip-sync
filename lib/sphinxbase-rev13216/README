CMU Sphinx common libraries
===============================================================================

This package contains the basic libraries shared by the CMU Sphinx trainer and
all the Sphinx decoders (Sphinx-II, Sphinx-III, and PocketSphinx), as well as
some common utilities for manipulating acoustic feature and audio files.

**Please see the LICENSE file for terms of use.**

Linux/Unix installation
-------------------------------------------------------------------------------

sphinxbase is used by other modules.  The convention requires the
physical layout of the code looks like this:

    .
    ├── package/
    └── sphinxbase/

So if you get the file from a distribution, you might want to rename
sphinxbase-X.X to sphinxbase by typing:

    $ mv sphinxbase-<X.X> sphinxbase (<X.X> being the version of sphinxbase)

If you downloaded directly from the Subversion repository, you need to create
the "configure" file by typing:

    $ ./autogen.sh

If you downloaded a release version or if you have already run
"autogen.sh", you can build simply by running:

    $ ./configure
    $ make

If you are compiling for a platform without floating-point arithmetic, you
should instead use:

    $ ./configure --enable-fixed --without-lapack
    $ make

You can also check the validity of the package by typing:

    $ make check

... and then install it with (might require permissions):

    $ make install

This defaults to installing SphinxBase under /usr/local. You may customize it
by running ./configure with an argument, as in:

    $ ./configure --prefix=/my/own/installation/directory

XCode Installation (for iPhone)
-------------------------------------------------------------------------------

Sphinxbase uses the standard unix autogen system, you can build sphinxbase with
automake. You just need to pass correct configure arguments, set compiler path,
set sysroot and other options. After you build the code you need to import
dylib file into your project and you also need to configure includes for your
project to find sphinxbase headers.

You also will have to create a recorder to capture audio with CoreAudio and
feed it into the recognizer.

For details see http://github.com/cmusphinx/pocketsphinx-ios-demo

If you want to quickly start with Pocketsphinx, try OpenEars toolkit which
includes Pocketsphinx http://www.politepix.com/openears/


Android installation
-------------------------------------------------------------------------------

See http://github.com/cmusphinx/pocketsphinx-android-demo.


MS Windows installation:
-------------------------------------------------------------------------------

To compile sphinxbase in Visual Studio 2010 Express (or newer):

  1. Unzip the file.
  2. Rename the directory to sphinxbase
  3. Go into the sphinxbase folder and click sphinxbase.sln
  4. In the menu, choose Build -> Rebuild All -> Batch Build -> Build

In Step 4, make sure all projects are selected, preferably the "Release"
version of each.

If you are using cygwin, the installation procedure is very similar to the Unix
installation. However, there is no audio driver support in cygwin currently so
one can only use the batch mode recognzier.

If you want to install Python packages on Windows, take a look at
http://github.com/cmusphinx/pocketsphinx-python.
