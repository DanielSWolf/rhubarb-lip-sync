# Where Am I?

A drop-in two files library to locate the current executable and the current
module on the file system.

Supported platforms:

- Windows
- Linux
- Mac
- iOS
- Android
- QNX Neutrino
- FreeBSD

Just drop `whereami.h` and `whereami.c` into your build and get started. (see
also [customizing compilation])

[customizing compilation]: #customizing-compilation

--------------------------------------------------------------------------------

## Usage

- `wai_getExecutablePath()` returns the path of the enclosing executable
- `wai_getModulePath()` returns the path of the enclosing module

Example usage:

- first call `int length = wai_getExecutablePath(NULL, 0, NULL);` to retrieve
 the length of the path
- allocate the destination buffer with `path = (char*)malloc(length + 1);`
- call `wai_getExecutablePath(path, length, &dirname_length)` again to retrieve
 the path
- add a terminal `NUL` character with `path[length] = '\0';`

Here is the output of the example:

    $ make -C _gnu-make
    $ cp ./bin/mac-x86_64/library.dylib /tmp/
    $ ./bin/mac-x86_64/executable --load-library=/tmp/library.dylib

    executable path: /Users/gregory/Projects/whereami/bin/mac-x86_64/executable
      dirname: /Users/gregory/Projects/whereami/bin/mac-x86_64
      basename: executable
    module path: /Users/gregory/Projects/whereami/bin/mac-x86_64/executable
      dirname: /Users/gregory/Projects/whereami/bin/mac-x86_64
      basename: executable

    library loaded
    executable path: /Users/gregory/Projects/whereami/bin/mac-x86_64/executable
      dirname: /Users/gregory/Projects/whereami/bin/mac-x86_64
      basename: executable
    module path: /private/tmp/library.dylib
      dirname: /private/tmp
      basename: library.dylib
    library unloaded

--------------------------------------------------------------------------------

## Customizing compilation

You can customize the library's behavior by defining the following macros:

- `WAI_FUNCSPEC`
- `WAI_PREFIX`
- `WAI_MALLOC`
- `WAI_REALLOC`
- `WAI_FREE`

## Compiling for Windows

There is a Visual Studio 2015 solution in the `_win-vs14/` folder.

## Compiling for Linux or Mac

There is a GNU Make 3.81 `MakeFile` in the `_gnu-make/` folder:

    $ make -C _gnu-make/

## Compiling for Mac

See above if you want to compile from command line. Otherwise there is an Xcode
project located in the `_mac-xcode/` folder.

## Compiling for iOS

There is an Xcode project located in the `_ios-xcode/` folder.

If you prefer compiling from command line and deploying to a jailbroken device
through SSH, use:

    $ make -C _gnu-make/ binsubdir=ios CC="$(xcrun --sdk iphoneos --find clang) -isysroot $(xcrun --sdk iphoneos --show-sdk-path) -arch armv7 -arch armv7s -arch arm64" postbuild="codesign -s 'iPhone Developer'"

## Compiling for Android

You will have to install the Android NDK, and point the `$NDK_ROOT` environment
variable to the NDK path: e.g. `export NDK_ROOT=/opt/android-ndk` (without a
trailing `/` character).

Next, the easy way is to make a standalone Android toolchain with the following
command:

    $ $NDK_ROOT/build/tools/make-standalone-toolchain.sh --system=$(uname -s | tr [A-Z] [a-z])-$(uname -m) --platform=android-3 --arch=arm --install-dir=/tmp/android

Now you can compile the self test and self benchmark programs by running:

    $ make -C _gnu-make/ libsuffix=.so binsubdir=android CC=/tmp/android/bin/arm-linux-androideabi-gcc CFLAGS='-march=armv7-a -mfloat-abi=softfp -O2'

--------------------------------------------------------------------------------

If you find this library useful and decide to use it in your own projects please
drop me a line [@gpakosz].

If you use it in a commercial project, consider using [Gittip].

[@gpakosz]: https://twitter.com/gpakosz
[Gittip]: https://www.gittip.com/gpakosz/
