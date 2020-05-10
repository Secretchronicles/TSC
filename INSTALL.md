Installation instructions for TSC
=================================

Time-stamp: <2016-08-03 10:13:02 quintus>

TSC uses [CMake][1] as the build system, so the first thing you have to
ensure is that you have CMake installed.

TSC supports the Linux and Windows platforms officially.
On Windows, testing is done on Windows 7.
**Windows XP is unsupported**.

TSC can be installed either from Git, meaning that you clone the
repository, or from a release tarball, where for the purpose of this
document a beta release is considered a release. Finally, you have the
possibility to cross-compile to Windows from Linux either from Git or
from a release tarball. Each of these possibilities will be covered
after we have had a look on the dependencies. Note that if you want
to crosscompile, you should probably read this entire file and not
just the section on crosscompilation to get a better understanding.

Installation instructions tailored specifically towards compiling TSC
from Git on Lubuntu 16.10 can be found in the separate file
tsc/docs/pages/compile_on_lubuntu_16_10.md.


Contents
--------

I. Dependencies
  1. Common dependencies
  2. Optional Windows dependencies

II. Configuration options

III. Installing from a released tarball

IV. Installing from Git

V. Upgrade notices

VI. Compiling on Windows with msys2
  1. Installing and updating msys2
  2. Installing the dependencies
  3. Optional dependencies
    3.1 CMake GUI Qt requirement workaround
  4. Building TSC

I. Dependencies
----------------

In any case, you will have to install a number of dependencies before
you can try installing TSC itself. The following sections list the
dependencies for each supported system.




### 1. Common dependencies ###

The following dependencies are required regardless of the system you
install to.

* A Ruby installation.
  * This is not required if you have a precompiled mruby available
    that you want to use instead of TSC's included static mruby.
    Ruby is only used to build mruby.
* The `pkg-config` program.
* The `bison` program.
* OpenGL.
* GLEW OpenGL wrangler extension library.
* GNU Gettext.
* The LibPNG library.
* The DevIL library.
* The libPCRE regular expression library.
* The libxml++ library < 3.0.0. Versions >= 3.0.0 will not be
  supported until the libxml++ developers provide a porting guide
  from version 2.8 to version 3.0.
* The Freetype library.
* CEGUI >= 0.8.0
  * If you have glm 0.9.6 or newer, you need CEGUI >= 0.8.5
    due to CEGUI bug #1063 (https://bitbucket.org/cegui/cegui/issues/1063).
* Boost >= 1.50.0 (to be exact: boost_system, boost_filesystem, boost_thread)
* SFML >= 2.3.0
* X11 development headers, namely for libx11 and libxt
* gperf

#### Example for Fedora ####
(Tested on Fedora 28)

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
sudo dnf install ruby rubygem-rake gperf pkgconf bison libGLEW \
freeglut-devel gettext libpng-devel pcre-devel libxml++-devel \
freetype-devel DevIL-devel boost SFML-devel gcc-c++ \
cegui-devel cmake @development-tools git libXt-devel
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#### Example for Debian/Ubuntu ####

(specific instructions for Lubuntu 16.10 can be found in
tsc/docs/pages/compile_on_lubuntu_16_10.md).

Install core dependencies:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
sudo apt install ruby-full rake gperf pkg-config bison libglew-dev \
  freeglut3-dev gettext libpng-dev libpcre3-dev libxml++2.6-dev \
  libfreetype6-dev libdevil-dev libboost1.58-all-dev libsfml-dev \
  libcegui-mk2-dev libxt-dev cmake build-essential git git-core
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Note that Debian 10 does not have CEGUI in the repositories anymore,
thus "libcegui-mk2-dev" needs to be left off from the the above list
and instead it is required to compile CEGUI manually before compiling
TSC. Further note that CEGUI 0.8.7 does not compile against Debian
10's libxml2. TSC offers the special compilation option
CEGUI_USE_EXPAT to use expat instead of libxml2 to overcome the
problem. Pass -DCEGUI_USE_EXPAT=ON to cmake when configuring TSC
to use it.

Alternatively, you can use a precompiled CEGUI. Example instructions
for using a precompiled CEGUI are provided in the file
tsc/docs/pages/compile_on_debian_10.md.

### 2. Optional Windows dependencies ###
* The FreeImage library.
* For generating a setup installer:
  * The NSIS package.













II. Configuration options
-------------------------

This section describes possible configuration you may apply before
building. If you just want to build the game, you can skip it. If you
want custom configuration or want to package it for e.g. a Linux
distribution, read on. Each of the flags described in this section
needs to be passed when invoking cmake by use of a `-D` option,
e.g. `-DENABLE_SCRIPT_DOCS=OFF`. Default values are indicated in brackets.

The following options are available:

ENABLE_SCRIPT_DOCS [ON]
: Build the scripting API documentation.

ENABLE_NLS [ON]
: Enables or disables use of translations. If disabled, TSC will use
  English only.

USE_SYSTEM_MRUBY [OFF]
: TSC embeds mruby for level scripting. If this option is turned
  off (default), then the mruby included in TSC's source tree is
  used for that. Turning this option on advises the build system
  to link against a system-provided mruby instead. This is useful
  for packaging and on odd systems on which TSC's included mruby does
  not build. However, keep in mind that TSC is developed only against
  the included mruby library. There's one major caveat you should
  make users aware of if enabling this option: if your mruby's
  version does not match the one included in TSC's sources, level
  scripts can fail due to version incompatibilities, especially
  if the version of mruby you provide is older than the one included
  in TSC's source tree. Check the version in the TSC source tree
  by executing $ git status in the mruby/mruby directory after
  updating the Git submodules.

USE_SYSTEM_TINYCLIPBOARD [OFF]
: For clipboard access, TSC uses the `tinyclipboard` library written
  by Marvin Gülker (Quintus). The library is not part of many Linux
  distributions yet, so it is build as part of building TSC itself
  as a static library. If you *are* on a Linux distribution where
  this library is packaged, set this value to ON and the build
  system will dynamically link to the tinyclipboard library of
  the system and not build its own variant.

USE_SYSTEM_PODPARSER [OFF]
: This option only has an effect if ENABLE_SCRIPT_DOCS is set to ON.
  If ON, it configures cmake to link the scripting API generator (scrdg)
  against the system-provided libpod-cpp. As scrdg is not installed
  in a normal setup (the programme is really only used for generating
  the scripting API HTML documents during the build process) there
  should rarely ever be a need to enable this option. If OFF,
  libpod-cpp is compiled from the Git submodule and linked into
  scrdg statically.

USE_LIBXMLPP3 [OFF]
: Enabling this upgrades TSC's dependency from libxml++2.6 to
  libxml++3.0. This is EXPERIMENTAL. If it breaks something,
  file a bug. For now, TSC officially only supports libxml++2.6.

The following path options are available:

CMAKE_INSTALL_PREFIX [/usr/local]
: Prefix value for all other CMAKE_INSTALL variables.

CMAKE_INSTALL_BINDIR [(prefix)/bin]
: Binary directory, i.e. where the `tsc` executable will be installed.
  Do not change this option when compiling for Windows (see below
  for further information).

CMAKE_INSTALL_DATADIR [(prefix)/share]
: Directory where the main data (levels, graphics, etc.) will be
  installed. Do not change this option when compiling for
  Windows (see below for further information).

CMAKE_INSTALL_DATAROOTDIR [(prefix)/share]
: Installation target of non-program-specific data, namely the
  `.desktop` starter file, icons, the scripting API docs,
  and the manpage by default. Only change this if you have
  good reasons.

CMAKE_INSTALL_MANDIR [(datarootdir)/man]
: Where TSC will install its manpage under. Note that the
  installer will create a subdirectory `man6`, i.e. the
  manpage is installed into (mandir)/man6/tsc.6.

**Do not change CMAKE_INSTALL_BINDIR or CMAKE_INSTALL_DATADIR when
compiling for Windows**! The `tsc` executable when run on Windows
searches for the data directory relative to its own path on the
filesystem, and it assumes the default layout. If you change this
option, the `tsc` executable will be unable to locate the data
directory and crash. On all other systems, the `tsc` executable will
take value of `CMAKE_INSTALL_DATADIR` as the data directory, hence you
can change it to your likening (handy for Linux distribution
packagers).

Especially if you are packaging, you will most likely also find it
useful to execute the install step like this:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
make DESTDIR=/some/dir install
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This will shift the file system root for installation to `/some/dir`
so that all pathes you gave to `cmake` will be below that path.













III. Installing from a released tarball
----------------------------------------

Extract the tarball, create a directory for the build and switch into
it:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
$ tar -xvJf TSC-*.tar.xz
$ cd TSC-*/tsc
$ mkdir build
$ cd build
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Execute `cmake` to configure and `make` to build and install TSC. Be
sure to replace `/opt/tsc` with the directory you want TSC to install
into.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
$ cmake -DCMAKE_INSTALL_PREFIX=/opt/tsc ..
$ make -j$(nproc)
# make install
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

If you want or are asked to, add `-DCMAKE_BUILD_TYPE=Debug` as a
parameter to `cmake` in order to build a version with debugging
symbols. These are needed by the developers to track down bugs more
easily.

After the last command finishes, you will find a `bin/tsc` executable
file below your chosen install directory. Execute it in order to start
TSC.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
$ /opt/tsc/bin/tsc
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~












IV. Installing from Git
-----------------------

Installing from Git basically works the same way as the normal release
install, but with a few preparations needed. You have to clone the
repository, and initialize the Git submodules before you can continue
with the real build process. These preprations can be done as follows:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
$ git clone --recursive git://github.com/Secretchronicles/TSC.git
$ cd TSC/tsc
$ mkdir build
$ cd build
$ cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=/opt/tsc ..
$ make -j$(nproc)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~













V. Upgrade notices
------------------

Before upgrading TSC to a newer released version or new development
version from Git, you may want to make a backup of your locally
created levels and worlds. You can do this by copying the directory
`~/.locals/share/tsc` to a safe place.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
$ cp -r ~/.local/share/tsc ~/backup-tsc
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

To upgrade your Git copy of TSC:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
$ git pull
$ git submodule update
$ cmake ..
$ make
$ make install
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

If you switch branches (maybe because you want to test a specific new
feature not merged into `devel` yet), it is recommended to clean the
build directory as well.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
$ cd ..
$ git checkout feature-branch
$ rm -rf build
$ mkdir build
$ cd build
$ cmake [OPTIONS] ..
$ make
$ make install
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~



VI. Compiling on Windows with msys2
-----------------------------------------

TSC can be compiled on Windows using msys2. The msys2 suite provides a convenient Bash shell for Windows, along with an expanding
repository of prebuilt packages.


### 1. Installing and updating msys2 ###

To begin, download and install msys2 from the [official website][3].

After it's installed, using `MSYS2 MinGW 64-bit`, execute:

    $ pacman -Syuu

If the base system components are updated, you will be prompted to close the bash windows manually, go ahead and do so.


Now run update again:

    $ pacman -Syuu


### 2. Installing the dependencies ###

Once it's finished, using `MSYS2 MinGW 64-bit`, install the dependencies for 64bit and 32bit builds. This downloads about 1 GB of files.

    $ pacman -S --needed git base-devel bison ruby mingw-w64-x86_64-{toolchain,extra-cmake-modules,ruby,cegui,sfml,libxml++2.6,gperf,doxygen,graphviz,nsis,qt5} mingw-w64-i686-{toolchain,extra-cmake-modules,ruby,cegui,sfml,libxml++2.6,gperf,doxygen,graphviz,nsis,minizip-git}

Press Enter about 3 times to accept and start installing dependencies.


### 3. Building TSC ###

From the same shell, that you installed packages for, run:

    $ git clone --recursive git://github.com/Secretchronicles/TSC.git


Now, run either the `MSYS2 MinGW 64-bit` or the `MSYS2 MinGW 32-bit`, depending on which version you are going to build.

Debug build:

    $ rm -rf TSC/tsc/build && mkdir TSC/tsc/build && cd TSC/tsc/build && cmake -DCMAKE_BUILD_TYPE=Debug -G "MSYS Makefiles" .. && make -j$(nproc) && make package

Release build:

    $ rm -rf TSC/tsc/build && mkdir TSC/tsc/build && cd TSC/tsc/build && cmake -DCMAKE_BUILD_TYPE=Release -G "MSYS Makefiles" .. && make -j$(nproc) && make package

This will pack the game with the CPack generators that you chose in the CMake command line.

After building, installer .exe package is at TSC/tsc/build directory.

Optionally, at build directory you can install directly:

    $ make install

[1]: http://cmake.org
[2]: http://mxe.cc
[3]: https://www.msys2.org
