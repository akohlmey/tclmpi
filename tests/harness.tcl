#!/usr/bin/tclsh
###########################################################
# Test harness for TclMPI
#
# Copyright (c) 2012 Axel Kohlmeyer <akohlmey@gmail.com>
# All Rights Reserved.
# 
# See the file LICENSE in the top level directory for
# licensing conditions.
###########################################################
# Current version.
set version 0.6

# load extension
load ../tclmpi.so

# some convenience variables
set comm   ::tclmpi::comm_world
set self   ::tclmpi::comm_self
set null   ::tclmpi::comm_null
set master 0
set rank 0
set size 0
set auto   ::tclmpi::auto
set int    ::tclmpi::int
set double ::tclmpi::double
set intint ::tclmpi::intint
set dblint ::tclmpi::dblint

# counters for successful and failed tests
set pass 0
set fail 0

# format output.
# if needed, the $cmd text is truncated
# to not overflow the lines.
proc test_format {kind cmd result} {
    global pass fail
    set num [expr {$pass + $fail}]
    set string [format "%03d %-10s | %-47s | " $num $kind $cmd]
    if {[string length $string] > 67} {
        set string [string range $string 0 60]
        append string {... | }
    }
    append string $result
    return $string
}


# serial init
proc ser_init {args} {
    global version rank master
    if {$rank == $master} {
        puts {------------------------------------------------------------------------------}
        run_return "package require tclmpi $version" $version
    }
}

# parallel init
proc par_init {args} {
    global argv comm rank size

    package require tclmpi
    ::tclmpi::init $argv
    set rank [::tclmpi::comm_rank $comm]
    set size [::tclmpi::comm_size $comm]

    ser_init
}

# run serial command and expect Tcl error
proc run_error {cmd errormsg} {
    global pass fail
    if {[catch $cmd result]} {
        if {[string equal $result $errormsg]} {
            incr pass
            puts [test_format {check/fail} $cmd PASS];
        } else {
            incr fail
            puts [test_format {check/fail} $cmd {FAIL/errmsg}]
            puts "$fail/Expected $errormsg"
            puts "$fail/Received $result"
        }
    } else {
        incr fail
        puts [test_format {check/fail} $cmd {FAIL/noerr}]
        puts "$fail/Result $result"
    }
    return {}
}

# run serial command and check return value
proc run_return {cmd retval} {
    global pass fail

    if {[catch $cmd result]} {
        incr fail
        puts [test_format {check/run} $cmd {FAIL/abort}]
        puts "$fail/Error message $result"
    } else {
        if {[string equal $result $retval]} {
            incr pass
            puts [test_format {check/run} $cmd PASS]
        } else {
            incr fail
            puts [test_format {check/run} $cmd {FAIL/retval}]
            puts "$fail/Expected $retval"
            puts "$fail/Received $result"
        }
    }
    return {}
}

# run parallel commands and check return values
proc par_return {cmd retval} {
    global pass fail comm master int

    flush stdout
    set size [::tclmpi::comm_size $comm]
    set rank [::tclmpi::comm_rank $comm]

    set num [llength $cmd]
    if {$num < $size} {
        incr fail
        if {$rank == $master} {
            puts [test_format {par/run} \
                      "number of commands ($num) < $size" {FAIL/input}]
        }
        return {}
    }
    set num [llength $retval]
    if {$num < $size} {
        incr fail
        if {$rank == $master} {
            puts [test_format {par/run} \
                      "number of return values ($num) < $size" {FAIL/input}]
        }
        return {}
    }

    ::tclmpi::barrier $comm
    set cmd [lindex $cmd $rank]
    set retval [lindex $retval $rank]
    set res [::tclmpi::allreduce [list [catch $cmd result] $rank] \
                 ::tclmpi::intint ::tclmpi::maxloc $comm]

    # all parallel commands came through
    if {[lindex $res 0] == 0} {
        set res [::tclmpi::allreduce \
                     [list [string equal $retval $result] $rank] \
                     ::tclmpi::intint ::tclmpi::minloc $comm]
        if {[lindex $res 0] == 0} {
            incr fail
            if {$rank == [lindex $res 1]} {
                puts [test_format {par/run} $cmd {FAIL/retval}]
                puts "$fail/Expected $retval"
                puts "$fail/Received $result on rank $rank"
            }
        } else {
            incr pass
            if {$rank == $master} {
                puts [test_format {par/run} $cmd PASS]
            }
        }

    # at least one parallel command failed
    } else {
        incr fail
        if {$rank == [lindex $res 1]} {
            puts [test_format {par/run} $cmd {FAIL/abort}]
            puts "$fail/Error message $result on rank $rank"
        }
    }
    ::tclmpi::barrier $comm
    flush stdout
    return {}
}

# run parallel commands and expect Tcl error
proc par_error {cmd retval} {
    global pass fail comm master int
    flush stdout

    set size [::tclmpi::comm_size $comm]
    set rank [::tclmpi::comm_rank $comm]

    set num [llength $cmd]
    if {$num < $size} {
        incr fail
        if {$rank == $master} {
            puts [test_format {par/fail} \
                      "number of commands ($num) < $size" {FAIL/input}]
        }
        return {}
    }
    set num [llength $retval]
    if {$num < $size} {
        incr fail
        if {$rank == $master} {
            puts [test_format {par/fail} \
                      "number of return values ($num) < $size" {FAIL/input}]
        }
        return {}
    }
    ::tclmpi::barrier $comm

    set cmd [lindex $cmd $rank]
    set retval [lindex $retval $rank]
    set res [::tclmpi::allreduce [list [catch $cmd result] $rank] \
                 ::tclmpi::intint ::tclmpi::maxloc $comm]

    # at least one parallel command failed
    if {[lindex $res 0] == 1} {
        set res [::tclmpi::allreduce \
                     [list [string equal $retval $result] $rank] \
                     ::tclmpi::intint ::tclmpi::minloc $comm]
        if {[lindex $res 0] > 0} {
            incr pass
            if {$rank == $master} {
                puts [test_format {par/fail} $cmd PASS]
            }
        } else {
            incr fail
            if {$rank == [lindex $res 1]} {
                puts [test_format {par/fail} $cmd {FAIL/retval}]
                puts "$fail/Expected $retval"
                puts "$fail/Received $result on rank $rank"
            }
        }

    # all completed
    } else {
        incr fail
        if {$rank != [lindex $res 1]} {
            puts [test_format {par/fail} $cmd {FAIL/noerr}]
            puts "$fail/Error message $result on rank $rank"
        }
    }
    ::tclmpi::barrier $comm
    flush stdout
    return {}
}

# set variable to different values on different ranks
# (not yet used)
proc par_set {name value} {
    global rank
    upvar $name var
    set var [lindex $value $rank]
}

# print result summary
proc test_summary {section} {
    global pass fail rank master
    if {$rank == $master} {
        puts {------------------------------------------------------------------------------}
        puts [format {test section %02d | total pass: %03d | total fail: %03d} $section $pass $fail]
    }
    if {$fail > 0} {
        exit $fail
    }
    return {}
}

