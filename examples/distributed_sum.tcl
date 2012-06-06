#!/usr/bin/tclsh
# distributed_sum.tcl --
#     Use MPI to sum a long list of numbers
#     with explicit send/receive and data serialization
#

set auto_path [concat [file normalize ..] $auto_path]
package require tclmpi 0.9

# simplify code through namespace
namespace import tclmpi::*

set master     0
set comm       $tclmpi::comm_world
set mpi_sum    $tclmpi::sum
set mpi_double $tclmpi::double
set mpi_int    $tclmpi::int
set mpi_auto   $tclmpi::auto

# backward compatibility
set tv microseconds
if {$tcl_version < 8.5} { set tv clicks }

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

#
# resynchronize processes for correct timings
#
barrier $comm
set tstart [clock $tv]

if { [comm_rank $comm] == $master } {

    #
    # Distribute the data over the workers - the master just waits
    # for the results
    #
    set size [comm_size $comm]

    set incrSize [expr {($dataSize+$size-2)/($size-1)}]
    set receiver 0
    for { set start 0 } { $start < $dataSize } { incr start $incrSize } {
        incr receiver
        set end [expr {$start+$incrSize-1}]
        if { $end > $dataSize } {
            set end $dataSize
        }

        isend [lrange $data $start $end] $mpi_auto $receiver 42 $comm
    }
}

#
# Let the workers do the work
#
if { [comm_rank $comm] != $master } {
    set data [recv $mpi_auto $master 42 $comm]
} else {
    set data 0   ;# The master should not do anything
}

set sum [allreduce [sum $data] $mpi_double $mpi_sum $comm]

if { [comm_rank $comm] == $master } {
    puts "Distributed sum: $sum"
    puts "Communication and computation time: [expr {([clock $tv]-$tstart)/1e6}] seconds"
}

#
# Finalize MPI
#
finalize
exit
