The TclMPI package  contains software that wraps  an MPI library for Tcl
and allows MPI  calls to  be  used from  Tcl scripts.  This code can  be
compiled as   a  shared object   to   be loaded   into  an existing  Tcl
interpreter  or as a  standalone TclMPI interpreter. In combination with
some additional  bundled   Tcl script  code,  additional  commands   are
provided that   allow to run   Tcl scripts in  parallel via  "mpirun" or
"mpiexec" similar to C, C++ or Fortran programs.

Homepage:

The main author of this package is Axel Kohlmeyer  and you can reach him
at <akohlmey@gmail.com>.  The  official  homepage  for this  project  is
http://sites.google.com/site/akohlmey/software/tclmpi

For compilation   and  installation instructions,  please see   the file
INSTALL.   Detailed documentation is   available online from the project
home page and  as a bundled PDF  file,  tclmpi_docs.pdf, as part  of the
source package.

Citing:
If needed, you can cite TclMPI as:
Axel Kohlmeyer. (2017). TclMPI: Release 1.0 [Data set]. Zenodo. 
[![DOI](https://www.zenodo.org/badge/4368856.svg)](https://www.zenodo.org/badge/latestdoi/4368856)

Acknowledgements:

Thanks to Arjen Markus and Chris MacDermaid  for encouragement and (lots
of) constructive criticism, that has  helped enourmously  to develop the
package  from a crazy idea to its current  level.  A special thanks also
goes to Karolina Sarnowska-Upton and  Andrew Grimshaw that allowed me to
use TclMPI as an example in their MPI portability study, which helped to
find quite a few bugs and  resolve several portability issues before the
code was hitting the real world.
