#!/usr/bin/tclsh
set auto_path [concat $env(PWD) $auto_path]
package require tclmpi

::tclmpi::init $argv

set comm ::tclmpi::comm_world

set size [::tclmpi::comm_size $comm]
set rank [::tclmpi::comm_rank $comm]
puts "this is rank $rank of $size"
::tclmpi::barrier $comm
puts "after barrier on rank $rank"

if {$rank == 0} {
  set idata [list 0 1 2 {3 4} 4 5 6]
} else {
  set idata {}
}

puts "before bcast/auto on rank $rank: $idata"
set res [::tclmpi::bcast $idata ::tclmpi::auto 0 $comm]
puts "after bcast/auto on rank $rank: $res"

puts "before bcast/int on rank $rank: $idata"
set res [::tclmpi::bcast $idata ::tclmpi::int 0 $comm]
puts "after bcast/int on rank $rank: $res"

puts "before bcast/double on rank $rank: $idata"
set res [::tclmpi::bcast $idata ::tclmpi::double 0 $comm]
puts "after bcast/double on rank $rank: $res"

if {$rank == 1} { set idata 3.14 }
puts "before bcast one on rank $rank: $idata"
set res [::tclmpi::bcast $idata ::tclmpi::double 1 $comm]
puts "after bcast/double on rank $rank: $res"


if {$rank == 0} {
  set idata 0.0

  ::tclmpi::probe ::tclmpi::any_source ::tclmpi::any_tag $comm status
  puts "pending message from rank $status(MPI_SOURCE) containing $status(COUNT_DOUBLE) doubles"
  ::tclmpi::recv ::tclmpi::double ::tclmpi::any_source ::tclmpi::any_tag $comm status
  puts "received data $res tag = $status(MPI_TAG) source = $status(MPI_SOURCE) error= $status(MPI_ERROR)"
  puts "[array get status]"
}
if {$rank == 1} {
  ::tclmpi::send $idata ::tclmpi::double 0 0 $comm
}

if {$size > 4} {

  if {$rank  < 2} { set newcomm [::tclmpi::comm_split $comm 1 $rank] }
  if {$rank == 2} { set newcomm [::tclmpi::comm_split $comm ::tclmpi::undefined 0] }
  if {$rank  > 2} { set newcomm [::tclmpi::comm_split $comm 0 -$rank] }

  puts "comm: $comm  new communicator: $newcomm"

  if {"$newcomm" != "::tclmpi::comm_null"} {
    set newsize [::tclmpi::comm_size $newcomm]
    set newrank [::tclmpi::comm_rank $newcomm]
  } else {
    set newsize 0
    set newrank -1
  }
  puts "this is rank $rank of $size with $newrank of $newsize on $newcomm"
}


::tclmpi::finalize

exit 0
