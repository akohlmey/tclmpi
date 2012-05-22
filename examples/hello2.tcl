#!/usr/bin/tclsh
# hello world a la TclMPI

# point Tcl to the directory with pkgIndex.tcl
# and load the TclMPI package.
set auto_path [concat [file normalize ..] $auto_path]
package require tclmpi 0.6

# import the namespace and a shortcut to the world communicator
namespace import tclmpi::*
namespace upvar tclmpi comm_world comm

# initialize TclMPI
init

set size [comm_size $comm]
set rank [comm_rank $comm]

puts "hello world, this is rank $rank of $size"

# close out TclMPI
finalize
exit 0
