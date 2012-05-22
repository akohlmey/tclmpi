#!/usr/bin/tclsh
# compute pi
set auto_path [concat [file normalize ..] $auto_path]
package require tclmpi 0.6

# simplify code through namespace
namespace import tclmpi::*
namespace upvar tclmpi comm_world comm
namespace upvar tclmpi sum    mpi_sum
namespace upvar tclmpi double mpi_double
namespace upvar tclmpi int    mpi_int

set master 0
set tstart [clock microseconds]

# error helper
proc abend {test rank msg} {
  global master
  if {$test} {
    if {$rank == $master} {puts $msg}
    finalize
    exit 1
  }
}

# initialize MPI
init

set size [comm_size $comm]
set rank [comm_rank $comm]

# parse command line
abend [expr {$size < 2}] $rank {need at least two processes.}
abend [expr {[llength $argv] < 1}] $rank {usage: pi.tcl <num intervals>}
set num [lindex $argv 0]
abend [expr {![string is integer $num]}] $rank {usage: pi.tcl <*num* intervals>}
abend [expr {int($num) < $size}] $rank "need at least $size intervals"

# make sure all processes have the same interval parameter
set num [bcast $num $mpi_int $master $comm]
if {$rank == $master} {puts "startup time: [expr {([clock microseconds]-$tstart)/1000000.0}] seconds"}
set tstart [clock microseconds]

# run parallel calculation
set h [expr {1.0/$num}]
set sum 0.0
for {set i $rank} {$i < $num} {incr i $size} {
  set sum [expr {$sum + 4.0/(1.0 + ($h*($i+0.5))**2)}]
}
set mypi [expr {$h * $sum}]
if {$rank == $master} {puts "loop time:    [expr {([clock microseconds]-$tstart)/1000000.0}] seconds"}
set tstart [clock microseconds]

# combine results
set mypi [allreduce $mypi $mpi_double $mpi_sum $comm]
if {$rank == $master} {puts "result: $mypi. relative error: [expr {abs(($mypi - 3.14159265358979)/3.14159265358979)}]"}
if {$rank == $master} {puts "result time:  [expr {([clock microseconds]-$tstart)/1000000.0}] seconds"}

# close out TclMPI
finalize
exit 0
