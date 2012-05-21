## \file
# This is the ::tclmpi:: namespace and the documentation of the 
# Tcl API of TclMPI.

## TclMPI wrapper
namespace eval tclmpi {

    variable auto   tclmpi::auto   ;#< constant for automatic data type
    variable int    tclmpi::int    ;#< constant for integer data type
    variable intint tclmpi::intint ;#< constant for integer pair data type
    variable double tclmpi::double ;#< constant for double data type
    variable dblint tclmpi::dblint ;#< constant for double/int pair data type

    variable comm_world tclmpi::comm_world ;#< constant for world communicator
    variable comm_self  tclmpi::comm_self  ;#< constant for self communicator
    variable comm_null  tclmpi::comm_null  ;#< constant empty communicator

    variable any_source tclmpi::any_source ;#< constant to accept messages from any source rank
    variable any_tag    tclmpi::any_tag    ;#< constant to accept messages with any tag

    variable version "0.6"  ;#< version number of this package

    # NOTE: the following section is to trick doxygen into documenting
    # the Tcl API of TclMPI. The since the actual functions are provided
    # as compiled C code in _tclmpi.c, we immediately delete them again.

    ## Initialize the MPI environment from Tcl
    # \return empty
    proc init {} {}
    rename init ""

    ## Shut down the MPI environment from Tcl
    # \return empty
    proc finalize {} {}
    rename finalize ""
}

package provide tclmpi $tclmpi::version

# load the ancilliary methods from the DSO
package require _tclmpi $::tclmpi::version

