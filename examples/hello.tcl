#!/usr/bin/tclsh
# hello world a la TclMPI

# point Tcl to the directory with pkgIndex.tcl
# and load the TclMPI package.
set auto_path [concat [file normalize ..] $auto_path]
package require tclmpi 0.6

# initialize TclMPI
::tclmpi::init

set comm tclmpi::comm_world
set size [::tclmpi::comm_size $comm]
set rank [::tclmpi::comm_rank $comm]

puts "hello world, this is rank $rank of $size"

# close out TclMPI
::tclmpi::finalize
exit 0
