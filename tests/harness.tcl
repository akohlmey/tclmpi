#!/usr/bin/tclsh
# tcl test script helpers

# load extension
load ../tclmpi.so

# count successful tests
set pass 0
set fail 0

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

# print result summary
proc test_summary {section} {
    global pass fail
    puts {------------------------------------------------------------------------------}
    puts "test section $section | total pass: $pass | total fail: $fail"
    exit $fail
}
