#!/usr/bin/tclsh
# ring example

set auto_path [concat [file normalize ..] $auto_path]
package require tclmpi

# error helper
proc abend {test rank msg} {
  global master
  if {$test} {
    if {$rank == $master} {puts $msg}
    tclmpi::finalize
    exit 1
  }
}

set tv microseconds
set master 0
set tstart [clock $tv]

# initialize MPI environment
tclmpi::init
set comm tclmpi::comm_world
set size [tclmpi::comm_size $comm]
set rank [tclmpi::comm_rank $comm]

# parse command line
abend [expr {$size < 2}] $rank \
    {need at least two processes.}
abend [expr {[llength $argv] < 1}] $rank \
    {usage: ring.tcl <num passes>}

set num [lindex $argv 0]

abend [expr {![string is integer $num]}] $rank \
    {usage: ring.tcl <*num* intervals>}
abend [expr {int($num) < 1}] $rank \
    "need to go at least 1 round"

# find neighboring processes
set next [expr {($rank + 1) % $size}]
set from [expr {($rank - 1 + $size) % $size}]
puts "rank $rank sends to $next and receives from $from"
# arbitrary message tag
set tag  201

# make sure all processes have the same interval parameter
set num [tclmpi::bcast $num tclmpi::int $master $comm]
if {$rank == $master} {
    puts "startup time: [expr {([clock $tv]-$tstart)/1e6}] seconds"
}
set tstart [clock $tv]

# start the parallel program

if {$rank == $master} {
    tclmpi::send $num tclmpi::int $next $tag $comm
}

# Pass the message around the ring.  The exit mechanism works as
# follows: the message (a positive integer) is passed around the ring.
# Each time is passes rank 0, it is decremented.  When each processes
# receives the 0 message, it passes it on to the next process and then
# quits.  By passing the 0 first, every process gets the 0 message and
# can quit normally.

while {$num > 0} {
    set num [tclmpi::recv tclmpi::int $from $tag $comm]
    puts "rank $rank received $num"

    if {$rank == $master} {
        incr num -1
        puts "rank $master decremented message to $num"
    }

    tclmpi::send $num tclmpi::int $next $tag $comm
}

if {$rank == $master} {
    set num [tclmpi::recv tclmpi::int $from $tag $comm]
    puts "rank $rank received $num"
    puts "loop time:    [expr {([clock $tv]-$tstart)/1e6}] seconds"
}

# close out TclMPI
tclmpi::finalize
exit 0
