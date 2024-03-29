#!/usr/bin/tclsh
###########################################################
# Unit tests for TclMPI - Part 4:
# non-destructive tests to be run with 2 MPI tasks
# using commands from the imported tclmpi namespace
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

# import all API from namespace
namespace import tclmpi::*
# use aliases instead of predefined variables
unset comm self null

namespace upvar tclmpi comm_world comm
namespace upvar tclmpi comm_self self
namespace upvar tclmpi comm_null null

namespace upvar tclmpi any_source any_source
namespace upvar tclmpi any_tag    any_tag
namespace upvar tclmpi undefined  undefined

namespace upvar tclmpi sum    mpi_sum
namespace upvar tclmpi prod   mpi_prod
namespace upvar tclmpi max    mpi_max
namespace upvar tclmpi min    mpi_min
namespace upvar tclmpi land   mpi_land
namespace upvar tclmpi lor    mpi_lor
namespace upvar tclmpi lxor   mpi_lxor
namespace upvar tclmpi band   mpi_band
namespace upvar tclmpi bor    mpi_bor
namespace upvar tclmpi bxor   mpi_bxor
namespace upvar tclmpi auto   mpi_auto
namespace upvar tclmpi double mpi_double
namespace upvar tclmpi int    mpi_int
namespace upvar tclmpi intint mpi_intint
namespace upvar tclmpi dblint mpi_dblint
namespace upvar tclmpi minloc mpi_minloc
namespace upvar tclmpi maxloc mpi_maxloc

# size/rank checks
par_return [list [list comm_size $comm] \
                [list comm_size $comm]] [list 2 2]
par_return [list [list comm_rank $comm] \
                [list comm_rank $comm]] [list 0 1]
par_return [list [list comm_size $self] \
                [list comm_size $self]] [list 1 1]
par_return [list [list comm_rank $self] \
                [list comm_rank $self]] [list 0 0]
par_error  [list [list comm_size $null] \
                [list comm_size $null] ] \
    [list {comm_size: invalid communicator} \
         {comm_size: invalid communicator} ]
par_error  [list [list comm_rank $null] \
                [list comm_rank $null] ] \
    [list {comm_rank: invalid communicator} \
         {comm_rank: invalid communicator} ]

set split0 tclmpi::comm0
set split1 tclmpi::comm1
set split2 tclmpi::comm2

# new "world" communicator with ranking reversed
par_return [list [list comm_split $comm 5 1] \
                [list comm_split $comm 5 -1] ] \
    [list [list $split0] [list $split0]]
par_return [list [list comm_size $split0] \
                [list comm_size $split0]] [list 2 2]
par_return [list [list comm_rank $split0] \
                [list comm_rank $split0]] [list 1 0]

# two "self" communicators with rank 0
par_return [list [list comm_split $comm 3 1] \
                [list comm_split $comm 1 -1] ] \
    [list [list $split1] [list $split1]]
par_return [list [list comm_size $split1] \
                [list comm_size $split1]] [list 1 1]
par_return [list [list comm_rank $split1] \
                [list comm_rank $split1]] [list 0 0]

# new world communicator with rank retained
par_return [list [list comm_split $comm 0 0] \
                [list comm_split $comm 0 0] ] \
    [list [list $split2] [list $split2]]
par_return [list [list comm_rank $split2] \
                [list comm_rank $split2]] [list 0 1]

# one real and one "null" communicator
par_return [list [list comm_split $comm 1 -1] \
                [list comm_split $comm $undefined 1] ] \
    [list {tclmpi::comm3} [list $null]]

# barrier
par_return [list [list barrier $comm]    \
                [list barrier $comm] ]   \
    [list {} {}]
par_return [list [list barrier $self]    \
                [list barrier $self] ]   \
    [list {} {}]
par_return [list [list barrier $split0]  \
                [list barrier $split0] ] \
    [list {} {}]
par_return [list [list barrier $split1]  \
                [list barrier $split1] ] \
    [list {} {}]
par_return [list [list barrier $split2]  \
                [list barrier $split2] ] \
    [list {} {}]

# comm_free
par_return [list [list comm_free $split2]  \
                [list comm_free $split2] ] \
    [list {} {}]
par_error [list [list set i 0]  \
                [list comm_free tclmpi::comm3] ] \
    [list {0} {{comm_free: unknown communicator: tclmpi::comm3}}]
par_return [list [list comm_free tclmpi::comm3] \
                [list set i 0] ] \
    [list {} {0}]

# bcast
set idata [list {xx 11} {1 2 3} {}]
par_return [list [list bcast $idata $auto 0 $comm] \
                [list bcast {} $auto 0 $comm]]     \
    [list [list $idata] [list $idata]]
par_return [list [list bcast {} $auto 0 $comm]     \
                [list bcast $idata $auto 0 $comm]] \
    [list {} {}]

set idata {016 {1 2 3} 2.0 7 0xff yy}
set odata {14 0 0 7 255 0}
par_return [list [list bcast {} $int 1 $comm]     \
                [list bcast $idata $int 1 $comm]] \
    [list [list $odata] [list $odata]]
if {$tcl_version < 8.5} {
    set odata {16.0 0.0 2.0 7.0 255.0 0.0}
} else {
    set odata {14.0 0.0 2.0 7.0 255.0 0.0}
}
par_return [list [list bcast $idata $double 0 $comm] \
                [list bcast {} $double 0 $comm]] \
    [list [list $odata] [list $odata]]

# when mixing $auto with other data types, we have mismatch or low-level
# MPI calls which is indicated in the truncated error message.
par_error  [list [list bcast $idata $double 0 $comm] \
                [list bcast {} $auto 0 $comm]] \
    [list [list $odata] {bcast: message truncated}]

# scatter
set idata {016 {1 2 3} 2.0 7 0xff yy}
par_return [list [list scatter {} $int 1 $comm] \
                [list scatter $idata $int 1 $comm]] \
    [list {{14 0 0}} {{7 255 0}}]

if {$tcl_version < 8.5} {
    par_return [list [list scatter $idata $double 0 $comm] \
                    [list scatter {} $double 0 $comm]] \
        [list {{16.0 0.0 2.0}} {{7.0 255.0 0.0}}]
} else {
    par_return [list [list scatter $idata $double 0 $comm] \
                    [list scatter {} $double 0 $comm]] \
        [list {{14.0 0.0 2.0}} {{7.0 255.0 0.0}}]
}

set idata {016 {1 2 3} 2.0 7 0xff}
set odata {scatter: number of data items must be divisible by the number of processes}
par_error [list [list scatter {} $int 1 $comm] \
                [list scatter $idata $int 1 $comm]] \
    [list [list $odata] [list $odata]]
par_error [list [list scatter $idata $double 0 $comm] \
                [list scatter {} $double 0 $comm]] \
    [list [list $odata] [list $odata]]

# allgather
set odata {14 0 0 7 255 0}
par_return [list [list allgather {016 {1 2 3} 2.0} $int $comm] \
                [list allgather {7 0xff yy} $int $comm]] \
    [list [list $odata] [list $odata]]

if {$tcl_version < 8.5} {
    set odata {16.0 0.0 2.0 7.0 255.0 0.0}
} else {
    set odata {14.0 0.0 2.0 7.0 255.0 0.0}
}
par_return [list [list allgather {016 {1 2 3} 2.0} $double $comm] \
                [list allgather {7 0xff yy} $double $comm]] \
    [list [list $odata] [list $odata]]

set odata {allgather: number of data items must be the same on all processes}
par_error [list [list allgather {016 {1 2 3}} $int $comm] \
                [list allgather {7 0xff yy} $int $comm]] \
    [list [list $odata] [list $odata]]
par_error [list [list allgather {016 {1 2 3} 2.0} $double $comm] \
                [list allgather {2.0 7 0xff yy} $double $comm]] \
    [list [list $odata] [list $odata]]

# gather
set odata {14 0 0 7 255 0}
par_return [list [list gather {016 {1 2 3} 2.0} $int 1 $comm] \
                [list gather {7 0xff yy} $int 1 $comm]] \
    [list {} [list $odata]]

if {$tcl_version < 8.5} {
    set odata {16.0 0.0 2.0 7.0 255.0 0.0}
} else {
    set odata {14.0 0.0 2.0 7.0 255.0 0.0}
}
par_return [list [list gather {016 {1 2 3} 2.0} $double 0 $comm] \
                [list gather {7 0xff yy} $double 0 $comm]] \
    [list [list $odata] {}]

set odata {gather: number of data items must be the same on all processes}
par_error [list [list gather {016 {1 2 3}} $int 1 $comm] \
                [list gather {7 0xff yy} $int 1 $comm]] \
    [list [list $odata] [list $odata]]
par_error [list [list gather {016 {1 2 3} 2.0} $double 0 $comm] \
                [list gather {2.0 7 0xff yy} $double 0 $comm]] \
    [list [list $odata] [list $odata]]

# allreduce
set idata {0 1 3 0 1 10}
set odata {1 -1 0 0 1 18}

# logical operators
set rdata {0 1 0 0 1 1}
par_return [list [list allreduce $idata $int $mpi_land $comm] \
                [list allreduce $odata $int $mpi_land $comm]] \
    [list [list $rdata] [list $rdata]]
set rdata {1 1 1 0 1 1}
par_return [list [list allreduce $idata $int $mpi_lor $comm] \
                [list allreduce $odata $int $mpi_lor $comm]] \
    [list [list $rdata] [list $rdata]]
set rdata {1 0 1 0 0 0}
par_return [list [list allreduce $idata $int $mpi_lxor $comm] \
                [list allreduce $odata $int $mpi_lxor $comm]] \
    [list [list $rdata] [list $rdata]]
set rdata {0 1 0 0 1 2}
par_return [list [list allreduce $idata $int $mpi_band $comm] \
                [list allreduce $odata $int $mpi_band $comm]] \
    [list [list $rdata] [list $rdata]]
set rdata {1 -1 3 0 1 26}
par_return [list [list allreduce $idata $int $mpi_bor $comm] \
                [list allreduce $odata $int $mpi_bor $comm]] \
    [list [list $rdata] [list $rdata]]
set rdata {1 -2 3 0 0 24}
par_return [list [list allreduce $idata $int $mpi_bxor $comm] \
                [list allreduce $odata $int $mpi_bxor $comm]] \
    [list [list $rdata] [list $rdata]]

# integer
set rdata {1 1 3 0 1 18}
par_return [list [list allreduce $idata $int $mpi_max $comm] \
                [list allreduce $odata $int $mpi_max $comm]] \
    [list [list $rdata] [list $rdata]]
set rdata {0 -1 0 0 1 10}
par_return [list [list allreduce $idata $int $mpi_min $comm] \
                [list allreduce $odata $int $mpi_min $comm]] \
    [list [list $rdata] [list $rdata]]
set rdata {1 0 3 0 2 28}
par_return [list [list allreduce $idata $int $mpi_sum $comm] \
                [list allreduce $odata $int $mpi_sum $comm]] \
    [list [list $rdata] [list $rdata]]
set rdata {0 -1 0 0 1 180}
par_return [list [list allreduce $idata $int $mpi_prod $comm] \
                [list allreduce $odata $int $mpi_prod $comm]] \
    [list [list $rdata] [list $rdata]]

# floating point
set idata {-1e5 2.4 1.5d0 0.2e-1 0.06E+28 0x22}
set rdata {1.0 2.4 0.0 0.02 6e+26 34.0}
par_return [list [list allreduce $idata $double $mpi_max $comm] \
                [list allreduce $odata $double $mpi_max $comm]] \
    [list [list $rdata] [list $rdata]]
set rdata {-100000.0 -1.0 0.0 0.0 1.0 18.0}
par_return [list [list allreduce $idata $double $mpi_min $comm] \
                [list allreduce $odata $double $mpi_min $comm]] \
    [list [list $rdata] [list $rdata]]
set rdata {-99999.0 1.4 0.0 0.02 6e+26 52.0}
par_return [list [list allreduce $idata $double $mpi_sum $comm] \
                [list allreduce $odata $double $mpi_sum $comm]] \
    [list [list $rdata] [list $rdata]]
set rdata {-100000.0 -2.4 0.0 0.0 6e+26 612.0}
par_return [list [list allreduce $idata $double $mpi_prod $comm] \
                [list allreduce $odata $double $mpi_prod $comm]] \
    [list [list $rdata] [list $rdata]]

# pairs
set idata {{-016 0} {2 0} {1.5 0} {2 -1} {two 0} {0x22 0}}
set odata {{1 1} {-1 1} {-10 1} {0 1} {1 1} {18 1}}
set rdata {{1 1} {2 0} {0 0} {2 -1} {1 1} {34 0}}
par_return [list [list allreduce $idata $mpi_intint $mpi_maxloc $comm] \
                [list allreduce $odata $mpi_intint $mpi_maxloc $comm]] \
    [list [list $rdata] [list $rdata]]
set rdata {{-14 0} {-1 1} {-10 1} {0 1} {0 0} {18 1}}
par_return [list [list allreduce $idata $mpi_intint $mpi_minloc $comm] \
                [list allreduce $odata $mpi_intint $mpi_minloc $comm]] \
    [list [list $rdata] [list $rdata]]
set idata {{-1e5 0} {2.4 0} {1.5d0 0} {0.2e-1 0} {0.06E+28 0} {0x22 0}}
set rdata {{1.0 1} {2.4 0} {0.0 0} {0.02 0} {6e+26 0} {34.0 0}}
par_return [list [list allreduce $idata $mpi_dblint $mpi_maxloc $comm] \
                [list allreduce $odata $mpi_dblint $mpi_maxloc $comm]] \
    [list [list $rdata] [list $rdata]]
set rdata {{-100000.0 0} {-1.0 1} {-10.0 1} {0.0 1} {1.0 1} {18.0 1}}
par_return [list [list allreduce $idata $mpi_dblint $mpi_minloc $comm] \
                [list allreduce $odata $mpi_dblint $mpi_minloc $comm]] \
    [list [list $rdata] [list $rdata]]

# reduce
set idata {0 1 3 0 1 10}
set odata {1 -1 0 0 1 18}

# logical operators
set rdata {0 1 0 0 1 1}
par_return [list [list reduce $idata $int $mpi_land 0 $comm] \
                [list reduce $odata $int $mpi_land 0 $comm]] \
    [list [list $rdata] {}]
set rdata {1 1 1 0 1 1}
par_return [list [list reduce $idata $int $mpi_lor 1 $comm] \
                [list reduce $odata $int $mpi_lor 1 $comm]] \
    [list {} [list $rdata]]
set rdata {1 0 1 0 0 0}
par_return [list [list reduce $idata $int $mpi_lxor 1 $comm] \
                [list reduce $odata $int $mpi_lxor 1 $comm]] \
    [list {} [list $rdata]]
set rdata {0 1 0 0 1 2}
par_return [list [list reduce $idata $int $mpi_band 0 $comm] \
                [list reduce $odata $int $mpi_band 0 $comm]] \
    [list [list $rdata] {}]
set rdata {1 -1 3 0 1 26}
par_return [list [list reduce $idata $int $mpi_bor 0 $comm] \
                [list reduce $odata $int $mpi_bor 0 $comm]] \
    [list [list $rdata] {}]
set rdata {1 -2 3 0 0 24}
par_return [list [list reduce $idata $int $mpi_bxor 1 $comm] \
                [list reduce $odata $int $mpi_bxor 1 $comm]] \
    [list {} [list $rdata]]

# integer
set rdata {1 1 3 0 1 18}
par_return [list [list reduce $idata $int $mpi_max 1 $comm] \
                [list reduce $odata $int $mpi_max 1 $comm]] \
    [list {} [list $rdata]]
set rdata {0 -1 0 0 1 10}
par_return [list [list reduce $idata $int $mpi_min 1 $comm] \
                [list reduce $odata $int $mpi_min 1 $comm]] \
    [list {} [list $rdata]]
set rdata {1 0 3 0 2 28}
par_return [list [list reduce $idata $int $mpi_sum 0 $comm] \
                [list reduce $odata $int $mpi_sum 0 $comm]] \
    [list [list $rdata] {}]
set rdata {0 -1 0 0 1 180}
par_return [list [list reduce $idata $int $mpi_prod 1 $comm] \
                [list reduce $odata $int $mpi_prod 1 $comm]] \
    [list {} [list $rdata]]

# floating point
set idata {-1e5 2.4 1.5d0 0.2e-1 0.06E+28 0x22}
set rdata {1.0 2.4 0.0 0.02 6e+26 34.0}
par_return [list [list reduce $idata $double $mpi_max 0 $comm] \
                [list reduce $odata $double $mpi_max 0 $comm]] \
    [list [list $rdata] {}]
set rdata {-100000.0 -1.0 0.0 0.0 1.0 18.0}
par_return [list [list reduce $idata $double $mpi_min 0 $comm] \
                [list reduce $odata $double $mpi_min 0 $comm]] \
    [list [list $rdata] {}]
set rdata {-99999.0 1.4 0.0 0.02 6e+26 52.0}
par_return [list [list reduce $idata $double $mpi_sum 0 $comm] \
                [list reduce $odata $double $mpi_sum 0 $comm]] \
    [list [list $rdata] {}]
set rdata {-100000.0 -2.4 0.0 0.0 6e+26 612.0}
par_return [list [list reduce $idata $double $mpi_prod 1 $comm] \
                [list reduce $odata $double $mpi_prod 1 $comm]] \
    [list {} [list $rdata]]

# send/recv both blocking
set idata [list 0 1 2 {3 4} 4 5 6]
par_return [list [list send $idata $auto 1 666 $comm] \
                [list recv $auto 0 666 $comm] ] [list {} $idata]
set rdata [list 0 1 2 0 4 5 6]
par_return [list [list send $idata $int 1 666 $comm]     \
                [list recv $int $any_source 666 $comm] ] \
    [list {} [list $rdata]]
set rdata [list 0.0 1.0 2.0 0.0 4.0 5.0 6.0]
par_return [list [list send $idata $double 1 666 $comm] \
                [list recv $double 0 $any_tag $comm] ]  \
    [list {} [list $rdata]]

# pairs
set idata {{-016 0} {2 0} {1.5 0} {2 -1} {two 0} {0x22 0}}
set odata {{1 1} {-1 1} {-10 1} {0 1} {1 1} {18 1}}
set rdata {{1 1} {2 0} {0 0} {2 -1} {1 1} {34 0}}
par_return [list [list reduce $idata $mpi_intint $mpi_maxloc 1 $comm] \
                [list reduce $odata $mpi_intint $mpi_maxloc 1 $comm]] \
    [list {} [list $rdata]]
set rdata {{-14 0} {-1 1} {-10 1} {0 1} {0 0} {18 1}}
par_return [list [list reduce $idata $mpi_intint $mpi_minloc 0 $comm] \
                [list reduce $odata $mpi_intint $mpi_minloc 0 $comm]] \
    [list [list $rdata] {}]
set idata {{-1e5 0} {2.4 0} {1.5d0 0} {0.2e-1 0} {0.06E+28 0} {0x22 0}}
set rdata {{1.0 1} {2.4 0} {0.0 0} {0.02 0} {6e+26 0} {34.0 0}}
par_return [list [list reduce $idata $mpi_dblint $mpi_maxloc 0 $comm] \
                [list reduce $odata $mpi_dblint $mpi_maxloc 0 $comm]] \
    [list [list $rdata] {}]
set rdata {{-100000.0 0} {-1.0 1} {-10.0 1} {0.0 1} {1.0 1} {18.0 1}}
par_return [list [list reduce $idata $mpi_dblint $mpi_minloc 1 $comm] \
                [list reduce $odata $mpi_dblint $mpi_minloc 1 $comm]] \
    [list {} [list $rdata]]

# non-blocking send / blocking recv
set req0 tclmpi::req0
set req1 tclmpi::req1
set req2 tclmpi::req2
set idata [list 0 1 2 {3 4} 4 5 6]
par_return [list [list isend $idata $auto 1 666 $comm] \
                [list recv $auto 0 666 $comm] ] [list $req0 $idata]
set rdata [list 0 1 2 0 4 5 6]
par_return [list [list isend $idata $int 1 666 $comm]           \
                [list recv $int $any_source 666 $comm status] ] \
    [list $req1 [list $rdata]]
set rdata [list 0.0 1.0 2.0 0.0 4.0 5.0 6.0]
par_return [list [list recv $double 1 $any_tag $comm status] \
                [list isend $idata $double 0 666 $comm] ]    \
    [list [list $rdata] $req0]

# clear up all pending requests
par_return [list [list wait $req0] \
                [list set i 0] ] [list {} 0]
par_return [list [list wait $req1 status] \
                [list set i 0] ] [list {} 0]

# it is no error to wait for a non-existing request
# or twice for the same request. wait returns immediately
par_return [list [list wait $req0] \
                [list set i 0] ] [list {} 0]
par_return [list [list wait $req2 status] \
                [list set i 0] ] [list {} 0]

# blocking send / non-blocking recv
set idata [list 0 1 2 {3 4} 4 5 6]
par_return [list [list send $idata $auto 1 666 $comm] \
                [list irecv $auto 0 666 $comm] ] \
    [list {} $req1]

set rdata [list 0 1 2 0 4 5 6]
par_return [list [list send $idata $int 1 66 $comm] \
                [list irecv $int $any_source 66 $comm]] \
    [list {} $req2]

set rdata [list 0.0 1.0 2.0 0.0 4.0 5.0 6.0]
par_return [list [list irecv $double 1 $any_tag $comm] \
                [list send $idata $double 0 6 $comm]] \
    [list $req2 {}]

# clear up all pending requests
par_return [list [list wait $req2] \
                [list wait $req1]] [list [list $rdata] [list $idata]]
par_return [list [list set i 0] [list wait $req2 status]] \
    [list {0} {{0 1 2 0 4 5 6}}]

# print results and exit
finalize
test_summary 04

exit 0
