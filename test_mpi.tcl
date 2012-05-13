#!/usr/bin/tclsh
set auto_path [concat $env(PWD) $auto_path]
package require tclmpi

::tclmpi::init $argv

set comm ::tclmpi::world

set size [::tclmpi::comm_size $comm]
set rank [::tclmpi::comm_rank $comm]
puts "this is rank $rank of $size"
::tclmpi::barrier $comm
puts "after barrier on rank $rank"


::tclmpi::finalize
