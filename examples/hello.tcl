#!/usr/bin/tclsh
# hello world a la TclMPI

package require tclmpi

# initialize TclMPI
::tclmpi::init

set comm tclmpi::comm_world
set size [::tclmpi::comm_size $comm]
set rank [::tclmpi::comm_rank $comm]

puts "hello world, this is rank $rank of $size"

# close out TclMPI
::tclmpi::finalize
exit 0
