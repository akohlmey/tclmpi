#!/usr/bin/tclsh
# compute pi

set auto_path [concat [file normalize ..] $auto_path]
package require tclmpi

# error helper
proc abend {test rank msg} {
  global master
  if {$test} {
    if {$rank == $master} {puts $msg}
    ::tclmpi::finalize
    exit 1
  }
}

set tv microseconds
set master 0
set tstart [clock $tv]

set piref 3.14159265358979

# initialize MPI
::tclmpi::init

set comm tclmpi::comm_world
set size [::tclmpi::comm_size $comm]
set rank [::tclmpi::comm_rank $comm]

# parse command line
abend [expr {$size < 2}] $rank \
    {need at least two processes.}
abend [expr {[llength $argv] < 1}] $rank \
    {usage: pi.tcl <num intervals>}

set num [lindex $argv 0]

abend [expr {![string is integer $num]}] $rank \
    {usage: pi.tcl <*num* intervals>}
abend [expr {int($num) < $size}] $rank \
    "need at least $size intervals"

# make sure all processes have the same interval parameter
set num [::tclmpi::bcast $num tclmpi::int $master $comm]
if {$rank == $master} {
    puts "startup time: [expr {([clock $tv]-$tstart)/1e6}] seconds"
}
set tstart [clock $tv]

# run parallel calculation
set h [expr {1.0/$num}]
set sum 0.0
if {$tcl_version < 8.5} {
    # Tcl 8.4 and older has no '**' operator
    for {set i $rank} {$i < $num} {incr i $size} {
        set sum [expr {$sum + 4.0/(1.0 + ($h*$h*($i+0.5)*($i+0.5)))}]
    }
} else {
    # this is slightly faster
    for {set i $rank} {$i < $num} {incr i $size} {
        set sum [expr {$sum + 4.0/(1.0 + ($h*($i+0.5))**2)}]
    }
}
set mypi [expr {$h * $sum}]

if {$rank == $master} {
    puts "loop time:    [expr {([clock $tv]-$tstart)/1e6}] seconds"
}
set tstart [clock $tv]

# combine results
set mypi [::tclmpi::allreduce $mypi tclmpi::double tclmpi::sum $comm]
if {$rank == $master} {
    puts "result: $mypi relative error: [expr {abs(($mypi - $piref)/$piref)}]"}
if {$rank == $master} {
    puts "result time:  [expr {([clock microseconds]-$tstart)/1e6}] seconds"
}

# close out TclMPI
::tclmpi::finalize
exit 0
