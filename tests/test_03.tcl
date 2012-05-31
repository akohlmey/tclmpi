#!/usr/bin/tclsh
###########################################################
# Unit tests for TclMPI - Part 3:
# non-destructive tests to be run with 2 MPI tasks
# using the fully qualified name of the commands.
#
# Copyright (c) 2012 Axel Kohlmeyer <akohlmey@gmail.com>
# All Rights Reserved.
# 
# See the file LICENSE in the top level directory for
# licensing conditions.
###########################################################

# import and initialize test harness
source harness.tcl
namespace import tclmpi_test::*
par_init

# size/rank checks
par_return [list [list ::tclmpi::comm_size $comm] \
                [list ::tclmpi::comm_size $comm]] [list 2 2]
par_return [list [list ::tclmpi::comm_rank $comm] \
                [list ::tclmpi::comm_rank $comm]] [list 0 1]
par_return [list [list ::tclmpi::comm_size $self] \
                [list ::tclmpi::comm_size $self]] [list 1 1]
par_return [list [list ::tclmpi::comm_rank $self] \
                [list ::tclmpi::comm_rank $self]] [list 0 0]
par_error  [list [list ::tclmpi::comm_size $null] \
                [list ::tclmpi::comm_size $null] ] \
    [list {::tclmpi::comm_size: invalid communicator} \
         {::tclmpi::comm_size: invalid communicator} ]
par_error  [list [list ::tclmpi::comm_rank $null] \
                [list ::tclmpi::comm_rank $null] ] \
    [list {::tclmpi::comm_rank: invalid communicator} \
         {::tclmpi::comm_rank: invalid communicator} ]

set split0 tclmpi::comm0
set split1 tclmpi::comm1
set split2 tclmpi::comm2

# new "world" communicator with ranking reversed
par_return [list [list ::tclmpi::comm_split $comm 5 1] \
                [list ::tclmpi::comm_split $comm 5 -1] ] \
    [list {tclmpi::comm0} {tclmpi::comm0}]
par_return [list [list ::tclmpi::comm_size $split0] \
                [list ::tclmpi::comm_size $split0]] [list 2 2]
par_return [list [list ::tclmpi::comm_rank $split0] \
                [list ::tclmpi::comm_rank $split0]] [list 1 0]

# two "self" communicators with rank 0
par_return [list [list ::tclmpi::comm_split $comm 3 1] \
                [list ::tclmpi::comm_split $comm 1 -1] ] \
    [list {tclmpi::comm1} {tclmpi::comm1}]
par_return [list [list ::tclmpi::comm_size $split1] \
                [list ::tclmpi::comm_size $split1]] [list 1 1]
par_return [list [list ::tclmpi::comm_rank $split1] \
                [list ::tclmpi::comm_rank $split1]] [list 0 0]

# new world communicator with rank retained
par_return [list [list ::tclmpi::comm_split $comm 0 0] \
                [list ::tclmpi::comm_split $comm 0 0] ] \
    [list {tclmpi::comm2} {tclmpi::comm2}]
par_return [list [list ::tclmpi::comm_rank $split2] \
                [list ::tclmpi::comm_rank $split2]] [list 0 1]

# one real and one "null" communicator
par_return [list [list ::tclmpi::comm_split $comm 1 -1] \
                [list ::tclmpi::comm_split $comm tclmpi::undefined 1] ] \
    [list {tclmpi::comm3} {tclmpi::comm_null}]

# barrier
par_return [list [list ::tclmpi::barrier $comm]    \
                [list ::tclmpi::barrier $comm] ]   \
    [list {} {}]
par_return [list [list ::tclmpi::barrier $self]    \
                [list ::tclmpi::barrier $self] ]   \
    [list {} {}]
par_return [list [list ::tclmpi::barrier $split0]  \
                [list ::tclmpi::barrier $split0] ] \
    [list {} {}]
par_return [list [list ::tclmpi::barrier $split1]  \
                [list ::tclmpi::barrier $split1] ] \
    [list {} {}]
par_return [list [list ::tclmpi::barrier $split2]  \
                [list ::tclmpi::barrier $split2] ] \
    [list {} {}]

# comm_free
par_return [list [list ::tclmpi::comm_free $split2]  \
                [list ::tclmpi::comm_free $split2] ] \
    [list {} {}]
par_error [list [list set i 0]  \
                [list ::tclmpi::comm_free tclmpi::comm3] ] \
    [list {0} {{::tclmpi::comm_free: unknown communicator: tclmpi::comm3}}]
par_return [list [list ::tclmpi::comm_free tclmpi::comm3] \
                [list set i 0] ] \
    [list {} {0}]

# bcast
set idata [list {xx 11} {1 2 3} {}]
par_return [list [list ::tclmpi::bcast $idata $auto 0 $comm] \
                [list ::tclmpi::bcast {} $auto 0 $comm]]     \
    [list [list $idata] [list $idata]]
par_return [list [list ::tclmpi::bcast {} $auto 0 $comm]     \
                [list ::tclmpi::bcast $idata $auto 0 $comm]] \
    [list {} {}]

set idata {016 {1 2 3} 2.0 7 0xff yy}
set odata {14 0 0 7 255 0}
par_return [list [list ::tclmpi::bcast {} $int 1 $comm]     \
                [list ::tclmpi::bcast $idata $int 1 $comm]] \
    [list [list $odata] [list $odata]]
if {$tcl_version < 8.5} {
    set odata {16.0 0.0 2.0 7.0 255.0 0.0}
} else {
    set odata {14.0 0.0 2.0 7.0 255.0 0.0}
}
par_return [list [list ::tclmpi::bcast $idata $double 0 $comm] \
                [list ::tclmpi::bcast {} $double 0 $comm]] \
    [list [list $odata] [list $odata]]

# when mixing $auto with other data types, we have mismatch or low-level
# MPI calls which is indicated in the truncated error message.
par_error  [list [list ::tclmpi::bcast $idata $double 0 $comm] \
                [list ::tclmpi::bcast {} $auto 0 $comm]] \
    [list [list $odata] {::tclmpi::bcast: message truncated}]

# scatter
set idata {016 {1 2 3} 2.0 7 0xff yy}
par_return [list [list ::tclmpi::scatter {} $int 1 $comm] \
                [list ::tclmpi::scatter $idata $int 1 $comm]] \
    [list {{14 0 0}} {{7 255 0}}]

if {$tcl_version < 8.5} {
    par_return [list [list ::tclmpi::scatter $idata $double 0 $comm] \
                    [list ::tclmpi::scatter {} $double 0 $comm]] \
        [list {{16.0 0.0 2.0}} {{7.0 255.0 0.0}}]
} else {
    par_return [list [list ::tclmpi::scatter $idata $double 0 $comm] \
                    [list ::tclmpi::scatter {} $double 0 $comm]] \
        [list {{14.0 0.0 2.0}} {{7.0 255.0 0.0}}]
}

set idata {016 {1 2 3} 2.0 7 0xff}
set odata {::tclmpi::scatter: number of data items must be divisible by the number of processes}
par_error [list [list ::tclmpi::scatter {} $int 1 $comm] \
                [list ::tclmpi::scatter $idata $int 1 $comm]] \
    [list [list $odata] [list $odata]]
par_error [list [list ::tclmpi::scatter $idata $double 0 $comm] \
                [list ::tclmpi::scatter {} $double 0 $comm]] \
    [list [list $odata] [list $odata]]

# allgather
set odata {14 0 0 7 255 0}
par_return [list [list ::tclmpi::allgather {016 {1 2 3} 2.0} $int $comm] \
                [list ::tclmpi::allgather {7 0xff yy} $int $comm]] \
    [list [list $odata] [list $odata]]

if {$tcl_version < 8.5} {
    set odata {16.0 0.0 2.0 7.0 255.0 0.0}
} else {
    set odata {14.0 0.0 2.0 7.0 255.0 0.0}
}
par_return [list [list ::tclmpi::allgather {016 {1 2 3} 2.0} $double $comm] \
                [list ::tclmpi::allgather {7 0xff yy} $double $comm]] \
    [list [list $odata] [list $odata]]

set odata {::tclmpi::allgather: number of data items must be the same on all processes}
par_error [list [list ::tclmpi::allgather {016 {1 2 3}} $int $comm] \
                [list ::tclmpi::allgather {7 0xff yy} $int $comm]] \
    [list [list $odata] [list $odata]]
par_error [list [list ::tclmpi::allgather {016 {1 2 3} 2.0} $double $comm] \
                [list ::tclmpi::allgather {2.0 7 0xff yy} $double $comm]] \
    [list [list $odata] [list $odata]]

# gather
set odata {14 0 0 7 255 0}
par_return [list [list ::tclmpi::gather {016 {1 2 3} 2.0} $int 1 $comm] \
                [list ::tclmpi::gather {7 0xff yy} $int 1 $comm]] \
    [list {} [list $odata]]

if {$tcl_version < 8.5} {
    set odata {16.0 0.0 2.0 7.0 255.0 0.0}
} else {
    set odata {14.0 0.0 2.0 7.0 255.0 0.0}
}
par_return [list [list ::tclmpi::gather {016 {1 2 3} 2.0} $double 0 $comm] \
                [list ::tclmpi::gather {7 0xff yy} $double 0 $comm]] \
    [list [list $odata] {}]

set odata {::tclmpi::gather: number of data items must be the same on all processes}
par_error [list [list ::tclmpi::gather {016 {1 2 3}} $int 1 $comm] \
                [list ::tclmpi::gather {7 0xff yy} $int 1 $comm]] \
    [list [list $odata] [list $odata]]
par_error [list [list ::tclmpi::gather {016 {1 2 3} 2.0} $double 0 $comm] \
                [list ::tclmpi::gather {2.0 7 0xff yy} $double 0 $comm]] \
    [list [list $odata] [list $odata]]

# allreduce
set idata {0 1 3 0 1 10}
set odata {1 -1 0 0 1 18}

# logical operators
set rdata {0 1 0 0 1 1}
par_return [list [list ::tclmpi::allreduce $idata $int tclmpi::land $comm] \
                [list ::tclmpi::allreduce $odata $int tclmpi::land $comm]] \
    [list [list $rdata] [list $rdata]]
set rdata {1 1 1 0 1 1}
par_return [list [list ::tclmpi::allreduce $idata $int tclmpi::lor $comm] \
                [list ::tclmpi::allreduce $odata $int tclmpi::lor $comm]] \
    [list [list $rdata] [list $rdata]]
set rdata {1 0 1 0 0 0}
par_return [list [list ::tclmpi::allreduce $idata $int tclmpi::lxor $comm] \
                [list ::tclmpi::allreduce $odata $int tclmpi::lxor $comm]] \
    [list [list $rdata] [list $rdata]]
set rdata {0 1 0 0 1 2}
par_return [list [list ::tclmpi::allreduce $idata $int tclmpi::band $comm] \
                [list ::tclmpi::allreduce $odata $int tclmpi::band $comm]] \
    [list [list $rdata] [list $rdata]]
set rdata {1 -1 3 0 1 26}
par_return [list [list ::tclmpi::allreduce $idata $int tclmpi::bor $comm] \
                [list ::tclmpi::allreduce $odata $int tclmpi::bor $comm]] \
    [list [list $rdata] [list $rdata]]
set rdata {1 -2 3 0 0 24}
par_return [list [list ::tclmpi::allreduce $idata $int tclmpi::bxor $comm] \
                [list ::tclmpi::allreduce $odata $int tclmpi::bxor $comm]] \
    [list [list $rdata] [list $rdata]]

# integer
set rdata {1 1 3 0 1 18}
par_return [list [list ::tclmpi::allreduce $idata $int tclmpi::max $comm] \
                [list ::tclmpi::allreduce $odata $int tclmpi::max $comm]] \
    [list [list $rdata] [list $rdata]]
set rdata {0 -1 0 0 1 10}
par_return [list [list ::tclmpi::allreduce $idata $int tclmpi::min $comm] \
                [list ::tclmpi::allreduce $odata $int tclmpi::min $comm]] \
    [list [list $rdata] [list $rdata]]
set rdata {1 0 3 0 2 28}
par_return [list [list ::tclmpi::allreduce $idata $int tclmpi::sum $comm] \
                [list ::tclmpi::allreduce $odata $int tclmpi::sum $comm]] \
    [list [list $rdata] [list $rdata]]
set rdata {0 -1 0 0 1 180}
par_return [list [list ::tclmpi::allreduce $idata $int tclmpi::prod $comm] \
                [list ::tclmpi::allreduce $odata $int tclmpi::prod $comm]] \
    [list [list $rdata] [list $rdata]]

# floating point
set idata {-1e5 2.4 1.5d0 0.2e-1 0.06E+28 0x22}
set rdata {1.0 2.4 0.0 0.02 6e+26 34.0}
par_return [list [list ::tclmpi::allreduce $idata $double tclmpi::max $comm] \
                [list ::tclmpi::allreduce $odata $double tclmpi::max $comm]] \
    [list [list $rdata] [list $rdata]]
set rdata {-100000.0 -1.0 0.0 0.0 1.0 18.0}
par_return [list [list ::tclmpi::allreduce $idata $double tclmpi::min $comm] \
                [list ::tclmpi::allreduce $odata $double tclmpi::min $comm]] \
    [list [list $rdata] [list $rdata]]
set rdata {-99999.0 1.4 0.0 0.02 6e+26 52.0}
par_return [list [list ::tclmpi::allreduce $idata $double tclmpi::sum $comm] \
                [list ::tclmpi::allreduce $odata $double tclmpi::sum $comm]] \
    [list [list $rdata] [list $rdata]]
set rdata {-100000.0 -2.4 0.0 0.0 6e+26 612.0}
par_return [list [list ::tclmpi::allreduce $idata $double tclmpi::prod $comm] \
                [list ::tclmpi::allreduce $odata $double tclmpi::prod $comm]] \
    [list [list $rdata] [list $rdata]]

# pairs
set idata {{-016 0} {2 0} {1.5 0} {2 -1} {two 0} {0x22 0}}
set odata {{1 1} {-1 1} {-10 1} {0 1} {1 1} {18 1}}
set rdata {{1 1} {2 0} {0 0} {2 -1} {1 1} {34 0}}
par_return [list [list ::tclmpi::allreduce $idata tclmpi::intint \
                      tclmpi::maxloc $comm] \
                [list ::tclmpi::allreduce $odata tclmpi::intint \
                     tclmpi::maxloc $comm]] [list [list $rdata] [list $rdata]]
set rdata {{-14 0} {-1 1} {-10 1} {0 1} {0 0} {18 1}}
par_return [list [list ::tclmpi::allreduce $idata tclmpi::intint \
                      tclmpi::minloc $comm] \
                [list ::tclmpi::allreduce $odata tclmpi::intint \
                     tclmpi::minloc $comm]] [list [list $rdata] [list $rdata]]
set idata {{-1e5 0} {2.4 0} {1.5d0 0} {0.2e-1 0} {0.06E+28 0} {0x22 0}}
set rdata {{1.0 1} {2.4 0} {0.0 0} {0.02 0} {6e+26 0} {34.0 0}}
par_return [list [list ::tclmpi::allreduce $idata tclmpi::dblint \
                      tclmpi::maxloc $comm] \
                [list ::tclmpi::allreduce $odata tclmpi::dblint \
                     tclmpi::maxloc $comm]] [list [list $rdata] [list $rdata]]
set rdata {{-100000.0 0} {-1.0 1} {-10.0 1} {0.0 1} {1.0 1} {18.0 1}}
par_return [list [list ::tclmpi::allreduce $idata tclmpi::dblint \
                      tclmpi::minloc $comm] \
                [list ::tclmpi::allreduce $odata tclmpi::dblint \
                     tclmpi::minloc $comm]] [list [list $rdata] [list $rdata]]

# reduce
set idata {0 1 3 0 1 10}
set odata {1 -1 0 0 1 18}

# logical operators
set rdata {0 1 0 0 1 1}
par_return [list [list ::tclmpi::reduce $idata $int tclmpi::land 0 $comm] \
                [list ::tclmpi::reduce $odata $int tclmpi::land 0 $comm]] \
    [list [list $rdata] {}]
set rdata {1 1 1 0 1 1}
par_return [list [list ::tclmpi::reduce $idata $int tclmpi::lor 1 $comm] \
                [list ::tclmpi::reduce $odata $int tclmpi::lor 1 $comm]] \
    [list {} [list $rdata]]
set rdata {1 0 1 0 0 0}
par_return [list [list ::tclmpi::reduce $idata $int tclmpi::lxor 1 $comm] \
                [list ::tclmpi::reduce $odata $int tclmpi::lxor 1 $comm]] \
    [list {} [list $rdata]]
set rdata {0 1 0 0 1 2}
par_return [list [list ::tclmpi::reduce $idata $int tclmpi::band 0 $comm] \
                [list ::tclmpi::reduce $odata $int tclmpi::band 0 $comm]] \
    [list [list $rdata] {}]
set rdata {1 -1 3 0 1 26}
par_return [list [list ::tclmpi::reduce $idata $int tclmpi::bor 0 $comm] \
                [list ::tclmpi::reduce $odata $int tclmpi::bor 0 $comm]] \
    [list [list $rdata] {}]
set rdata {1 -2 3 0 0 24}
par_return [list [list ::tclmpi::reduce $idata $int tclmpi::bxor 1 $comm] \
                [list ::tclmpi::reduce $odata $int tclmpi::bxor 1 $comm]] \
    [list {} [list $rdata]]

# integer
set rdata {1 1 3 0 1 18}
par_return [list [list ::tclmpi::reduce $idata $int tclmpi::max 1 $comm] \
                [list ::tclmpi::reduce $odata $int tclmpi::max 1 $comm]] \
    [list {} [list $rdata]]
set rdata {0 -1 0 0 1 10}
par_return [list [list ::tclmpi::reduce $idata $int tclmpi::min 1 $comm] \
                [list ::tclmpi::reduce $odata $int tclmpi::min 1 $comm]] \
    [list {} [list $rdata]]
set rdata {1 0 3 0 2 28}
par_return [list [list ::tclmpi::reduce $idata $int tclmpi::sum 0 $comm] \
                [list ::tclmpi::reduce $odata $int tclmpi::sum 0 $comm]] \
    [list [list $rdata] {}]
set rdata {0 -1 0 0 1 180}
par_return [list [list ::tclmpi::reduce $idata $int tclmpi::prod 1 $comm] \
                [list ::tclmpi::reduce $odata $int tclmpi::prod 1 $comm]] \
    [list {} [list $rdata]]

# floating point
set idata {-1e5 2.4 1.5d0 0.2e-1 0.06E+28 0x22}
set rdata {1.0 2.4 0.0 0.02 6e+26 34.0}
par_return [list [list ::tclmpi::reduce $idata $double tclmpi::max 0 $comm] \
                [list ::tclmpi::reduce $odata $double tclmpi::max 0 $comm]] \
    [list [list $rdata] {}]
set rdata {-100000.0 -1.0 0.0 0.0 1.0 18.0}
par_return [list [list ::tclmpi::reduce $idata $double tclmpi::min 0 $comm] \
                [list ::tclmpi::reduce $odata $double tclmpi::min 0 $comm]] \
    [list [list $rdata] {}]
set rdata {-99999.0 1.4 0.0 0.02 6e+26 52.0}
par_return [list [list ::tclmpi::reduce $idata $double tclmpi::sum 0 $comm] \
                [list ::tclmpi::reduce $odata $double tclmpi::sum 0 $comm]] \
    [list [list $rdata] {}]
set rdata {-100000.0 -2.4 0.0 0.0 6e+26 612.0}
par_return [list [list ::tclmpi::reduce $idata $double tclmpi::prod 1 $comm] \
                [list ::tclmpi::reduce $odata $double tclmpi::prod 1 $comm]] \
    [list {} [list $rdata]]

# pairs
set idata {{-016 0} {2 0} {1.5 0} {2 -1} {two 0} {0x22 0}}
set odata {{1 1} {-1 1} {-10 1} {0 1} {1 1} {18 1}}
set rdata {{1 1} {2 0} {0 0} {2 -1} {1 1} {34 0}}
par_return [list [list ::tclmpi::reduce $idata tclmpi::intint \
                      tclmpi::maxloc 0 $comm] \
                 [list ::tclmpi::reduce $odata tclmpi::intint \
                      tclmpi::maxloc 0 $comm]] [list [list $rdata] {}]
set rdata {{-14 0} {-1 1} {-10 1} {0 1} {0 0} {18 1}}
par_return [list [list ::tclmpi::reduce $idata tclmpi::intint \
                      tclmpi::minloc 1 $comm] \
                 [list ::tclmpi::reduce $odata tclmpi::intint \
                      tclmpi::minloc 1 $comm]] [list {} [list $rdata]]
set idata {{-1e5 0} {2.4 0} {1.5d0 0} {0.2e-1 0} {0.06E+28 0} {0x22 0}}
set rdata {{1.0 1} {2.4 0} {0.0 0} {0.02 0} {6e+26 0} {34.0 0}}
par_return [list [list ::tclmpi::reduce $idata tclmpi::dblint \
                      tclmpi::maxloc 1 $comm] \
                 [list ::tclmpi::reduce $odata tclmpi::dblint \
                      tclmpi::maxloc 1 $comm]] [list {} [list $rdata]]
set rdata {{-100000.0 0} {-1.0 1} {-10.0 1} {0.0 1} {1.0 1} {18.0 1}}
par_return [list [list ::tclmpi::reduce $idata tclmpi::dblint \
                      tclmpi::minloc 0 $comm] \
                 [list ::tclmpi::reduce $odata tclmpi::dblint \
                      tclmpi::minloc 0 $comm]] [list [list $rdata] {}]

# send/recv both blocking
set idata [list 0 1 2 {3 4} 4 5 6]
par_return [list [list ::tclmpi::send $idata $auto 1 666 $comm] \
                [list ::tclmpi::recv $auto 0 666 $comm] ] [list {} $idata]
set rdata [list 0 1 2 0 4 5 6]
par_return [list [list ::tclmpi::send $idata $int 1 666 $comm]            \
                [list ::tclmpi::recv $int tclmpi::any_source 666 $comm] ] \
    [list {} [list $rdata]]
set rdata [list 0.0 1.0 2.0 0.0 4.0 5.0 6.0]
par_return [list [list ::tclmpi::send $idata $double 1 666 $comm]       \
                [list ::tclmpi::recv $double 0 tclmpi::any_tag $comm] ] \
    [list {} [list $rdata]]

# non-blocking send / blocking recv
set req0 tclmpi::req0
set req1 tclmpi::req1
set req2 tclmpi::req2
set idata [list 0 1 2 {3 4} 4 5 6]
par_return [list [list ::tclmpi::isend $idata $auto 1 666 $comm] \
                [list ::tclmpi::recv $auto 0 666 $comm] ] [list $req0 $idata]
set rdata [list 0 1 2 0 4 5 6]
par_return [list [list ::tclmpi::isend $idata $int 1 666 $comm]  \
                [list ::tclmpi::recv $int tclmpi::any_source 666 \
                     $comm status] ] [list $req1 [list $rdata]]
set rdata [list 0.0 1.0 2.0 0.0 4.0 5.0 6.0]
par_return [list [list ::tclmpi::recv $double 1 tclmpi::any_tag $comm status] \
                [list ::tclmpi::isend $idata $double 0 666 $comm] ] \
    [list [list $rdata] $req0]

# clear up all pending requests
par_return [list [list ::tclmpi::wait $req0] \
                [list set i 0] ] [list {} 0]
par_return [list [list ::tclmpi::wait $req1 status] \
                [list set i 0] ] [list {} 0]

# it is no error to wait for a non-existing request
# or twice for the same request. wait returns immediately
par_return [list [list ::tclmpi::wait $req0] \
                [list set i 0] ] [list {} 0]
par_return [list [list ::tclmpi::wait $req2 status] \
                [list set i 0] ] [list {} 0]

# blocking send / non-blocking recv
set idata [list 0 1 2 {3 4} 4 5 6]
par_return [list [list ::tclmpi::send $idata $auto 1 666 $comm] \
                [list ::tclmpi::irecv $auto 0 666 $comm] ] \
    [list {} $req1]

set rdata [list 0 1 2 0 4 5 6]
par_return [list [list ::tclmpi::send $idata $int 1 66 $comm] \
                [list ::tclmpi::irecv $int tclmpi::any_source 66 $comm]] \
    [list {} $req2]

set rdata [list 0.0 1.0 2.0 0.0 4.0 5.0 6.0]
par_return [list [list ::tclmpi::irecv $double 1 tclmpi::any_tag $comm] \
                [list ::tclmpi::send $idata $double 0 6 $comm]] \
    [list $req2 {}]

# clear up all pending requests
par_return [list [list ::tclmpi::wait $req2] \
                [list ::tclmpi::wait $req1]] \
    [list [list $rdata] [list $idata]]
par_return [list [list set i 0] [list ::tclmpi::wait $req2 status]] \
    [list {0} {{0 1 2 0 4 5 6}}]

# print results and exit
::tclmpi::finalize
test_summary 03

exit 0
