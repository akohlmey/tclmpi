#!/usr/bin/tclsh
# hello world a la TclMPI

package require tclmpi

# import the namespace and a shortcut to the world communicator
namespace import tclmpi::*
set comm $tclmpi::comm_world

# initialize TclMPI
init

set size [comm_size $comm]
set rank [comm_rank $comm]

puts "hello world, this is rank $rank of $size"

# close out TclMPI
finalize
exit 0
