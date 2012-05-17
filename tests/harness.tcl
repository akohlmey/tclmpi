#!/usr/bin/tclsh
# tcl test script helpers

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

# count successful tests
set pass 0
set fail 0
set version 0.5

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

# run command and expect Tcl error
proc run_error {cmd errormsg} {
    global pass fail
    puts -nonewline [format "check/fail %-60s |" $cmd]
    if {[catch $cmd result]} {
        if {[string equal $result $errormsg]} {
            puts " PASS"; incr pass
        } else {
            puts " FAIL/errmsg"; incr fail
            puts "$fail/Expected $errormsg"
            puts "$fail/Received $result"
        }
    } else {
        puts " FAIL/noerr"; incr fail
        puts "$fail/Result $result"
    }
    return {}
}

# run command and check return value
proc run_return {cmd retval} {
    global pass fail
    puts -nonewline [format "check/run  %-60s |" $cmd]
    if {[catch $cmd result]} {
        puts " FAIL/abort"; incr fail
        puts "$fail/Error message $result"
    } else {
        if {[string equal $result $retval]} {
            puts " PASS"; incr pass
        } else {
            puts " FAIL/retval"; incr fail
            puts "$fail/Expected $retval"
            puts "$fail/Received $result"
        }
    }
    return {}
}

# run parallel commands and check return values
proc par_return {cmd retval} {
    global pass fail comm master int

    set size [::tclmpi::comm_size $comm]
    set rank [::tclmpi::comm_rank $comm]

    if {[llength $cmd] != $size} {
        if {$rank == $master} {
            puts " FAIL/input number of commands != $size"
        }
        incr fail
        return {}
    }
    if {[llength $retval] != $size} {
        if {$rank == $master} {
            puts " FAIL/input number of return values != $size"
        }
        incr fail
        return {}
    }

    set red 0
    set res [::tclmpi::allreduce \
                 [list [catch [lindex $cmd $rank] result] $rank] \
                 ::tclmpi::intint ::tclmpi::maxloc $comm]

    # all parallel commands came through
    if {$res == 0} {

        set res [::tclmpi::allreduce \
                     [list [string equal [lindex $retval $rank] $result] $rank] \
                     ::tclmpi::intint ::tclmpi::maxloc $comm]

        if {$rank == $master} {
            puts "$rank: $res $result"
        } else {
            puts "$rank: $res $result"
        }
    }
    return {}
}

# print result summary
proc test_summary {section} {
    global pass fail rank master
    if {$rank == $master} {
        puts {------------------------------------------------------------------------------}
        puts "test section $section | total pass: $pass | total fail: $fail"
    }
    if {$fail > 0} {
        ::tclmpi::abort $fail
    }
    return {}
}

