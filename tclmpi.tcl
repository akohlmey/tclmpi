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

    ## Terminates the MPI environment from Tcl
    # \return empty
    proc abort {comm errorcode} {}
    rename abort ""

    ## Returns the number of processes involved in an MPI communicator
    # \param[in] comm Tcl representation of the communicator
    # \return number of MPI tasks
    proc comm_size {comm} {}
    rename comm_size ""

    ## Returns the rank of the current process for the given MPI communicator
    # \param comm Tcl representation of the communicator
    # \return rank on the communicator (integer between 0 and size-1)
    proc comm_rank {comm} {}
    rename comm_rank ""

    ## Creates new communicators based on "color" and "key" flags
    # \return Tcl representation of the newly created MPI communicator
    proc comm_split {comm color key} {}
    rename comm_split ""

    ## 
    # \return empty
    proc barrier {} {}
    rename barrier ""

    ## 
    # \return Data that was broadcast
    proc bcast {data type root comm} {}
    rename bcast ""

    ## 
    # \return Data for the reduction operation
    proc allreduce {data type op comm} {}
    rename allreduce ""

    ## Blocking send
    # \return empty
    proc send {data type dest tag comm} {}
    rename send ""

    ## Non-blocking send
    # \return Tcl representation of generated MPI request
    proc isend {data type dest tag comm} {}
    rename isend ""

    ## Blocking receive
    # \return received data
    proc recv {type source tag comm {status {}}} {}
    rename recv ""

    ## Non-blocking receive
    # \return Tcl representation of generated MPI request
    proc irecv {type source tag comm} {}
    rename irecv ""

    ## 
    # \return empty
    proc X {} {}
    rename X ""

    ## 
    # \return empty
    proc X {} {}
    rename X ""

    ## 
    # \return empty
    proc X {} {}
    rename X ""

    ## 
    # \return empty
    proc X {} {}
    rename X ""

    ## 
    # \return empty
    proc X {} {}
    rename X ""

    ## 
    # \return empty
    proc X {} {}
    rename X ""

    ## 
    # \return empty
    proc X {} {}
    rename X ""

    ## 
    # \return empty
    proc X {} {}
    rename X ""

    ## 
    # \return empty
    proc X {} {}
    rename X ""

    ## 
    # \return empty
    proc X {} {}
    rename X ""

}

package provide tclmpi $tclmpi::version

# load the ancilliary methods from the DSO
package require _tclmpi $::tclmpi::version

