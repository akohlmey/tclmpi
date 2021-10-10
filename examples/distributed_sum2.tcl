#!/usr/bin/tclsh
# distributed_sum2.tcl --
#     Use MPI to sum a long list of numbers
#     using scatter and avoiding serialization
#

package require tclmpi

# simplify code through namespace
namespace import tclmpi::*

set master     0
set comm       $tclmpi::comm_world
set mpi_sum    $tclmpi::sum
set mpi_double $tclmpi::double
set mpi_int    $tclmpi::int
set tv microseconds

# sum --
#     Sum the data
#
proc sum {data} {
    set sum 0
    foreach d $data {
        set sum [expr {$sum + $d}]
    }
    puts "Worker: [comm_rank $::comm] -- $sum"
    return $sum
}

# main --
#     Generate a long list of data
#

#
# Initialize MPI - essential
#
init

#
# Now the master creates the list of data
# and distributes them
#
set dataSize 1000000
set data {}

if { [comm_rank $comm] == $master } {

    set mysum 0
    for { set i 0 } { $i < $dataSize } { incr i } {
        lappend data $i
    }
    set tstart [clock $tv]
    puts "Direct computation: [sum $data]"
    puts "Computation time: [expr {([clock $tv]-$tstart)/1e6}] seconds"
}

# Distribute the data

set size [comm_size $comm]

# add padding, so the number of data elements is
# cleanly divisible by the number of processors.

set needpad [expr {$dataSize % $size}]
set numpad [expr {$needpad ? ($size - $needpad) : 0}]
if { [comm_rank $comm] == $master } {
    for {set i 0} {$i < $numpad} {incr i} {
        lappend data 0
    }
}
set blocksz [expr {($dataSize + $numpad)/ $size}]

# resynchronize processes for correct timings

barrier $comm
set tstart [clock $tv]

# Distribute data and do the work
set mydata [scatter $data $mpi_int $master $comm]
set sum [allreduce [sum $mydata] $mpi_double $mpi_sum $comm]

if { [comm_rank $comm] == $master } {
    puts "Distributed sum: $sum"
    puts "Communication and computation time: [expr {([clock $tv]-$tstart)/1e6}] seconds"
}

#
# Finalize MPI
#
finalize
exit
