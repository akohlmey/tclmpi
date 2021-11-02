The TclMPI package  contains software that wraps  an MPI library for Tcl
and allows MPI  calls to  be  used from  Tcl scripts.  This code can  be
compiled as   a  shared object   to   be loaded   into  an existing  Tcl
interpreter  or as a  standalone TclMPI interpreter. In combination with
some additional  bundled   Tcl script  code,  additional  commands   are
provided that   allow to run   Tcl scripts in  parallel via  "mpirun" or
"mpiexec" similar to C, C++ or Fortran programs.

# Homepage

The main author of this package is Axel Kohlmeyer  and you can reach him
at <akohlmey@gmail.com>. The online documentation for this project is at
[https://akohlmey.github.io/tclmpi/](https://akohlmey.github.io/tclmpi/),
a [PDF version of the documentation](https://akohlmey.github.io/tclmpi/tclmpi_docs.pdf)
is also available, and development is [hosted on GitHub](https://github.com/akohlmey/tclmpi/).

For basic compilation and installation instructions, please see the file
INSTALL. More detailed documentation is available online from the
[User's Guide](https://akohlmey.github.io/tclmpi/userguide.html).

Information about the implementation and design of the package are in the
[Developer's Guide](https://akohlmey.github.io/tclmpi/devguide.html).

# Precompiled Binaries

Precompiled binary packages of TclMPI are available for the following
operating systems and distributions.

## Microsoft Windows

A precompiled installer package for 64-bit Windows 10 is available from the
[TclMPI GitHub Releases Page](https://github.com/akohlmey/tclmpi/releases).

To use this package, the MS-MPI package version 10.x and ActiveTcl version 8.6
from ActiveState must be downloaded and installed first.  The installer will
check for them and refuse to install TclMPI without.

MS-MPI is [available here](https://github.com/microsoft/Microsoft-MPI/releases).
You only need the "msmpisetup.exe" file and ActiveTcl is
[available here](https://www.activestate.com/products/tcl/)

## Fedora Linux

Repositories with TclMPI for Fedora Linux are hosted at https://copr.fedorainfracloud.org/coprs/akohlmey/TclMPI/

[![Copr build status](https://copr.fedorainfracloud.org/coprs/akohlmey/TclMPI/package/tclmpi/status_image/last_build.png)](https://copr.fedorainfracloud.org/coprs/akohlmey/TclMPI/package/tclmpi/)

# Test Status

[![Linux](https://github.com/akohlmey/tclmpi/actions/workflows/unittest-linux.yml/badge.svg?branch=master)](https://github.com/akohlmey/tclmpi/actions/workflows/unittest-linux.yml)
[![macOS](https://github.com/akohlmey/tclmpi/actions/workflows/unittest-macos.yml/badge.svg?branch=master)](https://github.com/akohlmey/tclmpi/actions/workflows/unittest-macos.yml)
[![CodeQL](https://github.com/akohlmey/tclmpi/actions/workflows/codeql-analysis.yml/badge.svg?branch=master)](https://github.com/akohlmey/tclmpi/actions/workflows/codeql-analysis.yml)

# Citing

You can cite TclMPI as:

Axel Kohlmeyer. (2021). TclMPI: Release 1.2 [Data set]. Zenodo. [![DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.598343.svg)](https://doi.org/10.5281/zenodo.598343)

# Acknowledgements

Thanks to Arjen Markus and Chris MacDermaid  for encouragement and (lots
of) constructive criticism, that has  helped enourmously  to develop the
package  from a crazy idea to its current  level.  Thanks to  Alex Baker
for motivating me to convert to using CMake as build system which makes
building TclMPI natively on Windows much easier.

A special thanks also
goes to Karolina Sarnowska-Upton and  Andrew Grimshaw that allowed me to
use TclMPI as an example in their MPI portability study, which helped to
find quite a few bugs and  resolve several portability issues before the
code was hitting the real world.

