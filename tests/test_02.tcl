#!/usr/bin/tclsh
# non-destructive tests to be run with 2 MPI tasks.
source harness.tcl

set comm   ::tclmpi::comm_world
set self   ::tclmpi::comm_self
set null   ::tclmpi::comm_null
set split0 ::tclmpi::comm0
set split1 ::tclmpi::comm1

# parallel init
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
    [list {::tclmpi::comm_size: MPI_ERR_COMM: invalid communicator} \
         {::tclmpi::comm_size: MPI_ERR_COMM: invalid communicator} ]
par_error  [list [list ::tclmpi::comm_rank $null] \
                [list ::tclmpi::comm_rank $null] ] \
    [list {::tclmpi::comm_rank: MPI_ERR_COMM: invalid communicator} \
         {::tclmpi::comm_rank: MPI_ERR_COMM: invalid communicator} ]

set split0 ::tclmpi::comm0
set split1 ::tclmpi::comm1
set split2 ::tclmpi::comm2

# new "world" communicator with ranking reversed
par_return [list [list ::tclmpi::comm_split $comm 5 1] \
                [list ::tclmpi::comm_split $comm 5 -1] ] \
    [list {::tclmpi::comm0} {::tclmpi::comm0}]
par_return [list [list ::tclmpi::comm_size $split0] \
                [list ::tclmpi::comm_size $split0]] [list 2 2]
par_return [list [list ::tclmpi::comm_rank $split0] \
                [list ::tclmpi::comm_rank $split0]] [list 1 0]

# two "self" communicators with rank 0
par_return [list [list ::tclmpi::comm_split $comm 3 1] \
                [list ::tclmpi::comm_split $comm 1 -1] ] \
    [list {::tclmpi::comm1} {::tclmpi::comm1}]
par_return [list [list ::tclmpi::comm_size $split1] \
                [list ::tclmpi::comm_size $split1]] [list 1 1]
par_return [list [list ::tclmpi::comm_rank $split1] \
                [list ::tclmpi::comm_rank $split1]] [list 0 0]

# new world communicator with rank retained
par_return [list [list ::tclmpi::comm_split $comm 0 0] \
                [list ::tclmpi::comm_split $comm 0 0] ] \
    [list {::tclmpi::comm2} {::tclmpi::comm2}]
par_return [list [list ::tclmpi::comm_rank $split2] \
                [list ::tclmpi::comm_rank $split2]] [list 0 1]

# one real and one "null" communicator
par_return [list [list ::tclmpi::comm_split $comm ::tclmpi::undefined 1] \
                [list ::tclmpi::comm_split $comm 1 -1] ] \
    [list {::tclmpi::comm_null} {::tclmpi::comm3}]

# barrier
par_return [list [list ::tclmpi::barrier $comm] \
                [list ::tclmpi::barrier $comm] ]  [list {} {}]
par_return [list [list ::tclmpi::barrier $self] \
                [list ::tclmpi::barrier $self] ]  [list {} {}]
par_return [list [list ::tclmpi::barrier $split0] \
                [list ::tclmpi::barrier $split0] ]  [list {} {}]
par_return [list [list ::tclmpi::barrier $split1] \
                [list ::tclmpi::barrier $split1] ]  [list {} {}]
par_return [list [list ::tclmpi::barrier $split2] \
                [list ::tclmpi::barrier $split2] ]  [list {} {}]

# bcast
set idata [list {xx 11} {1 2 3} {}]
par_return [list [list ::tclmpi::bcast $idata $auto $master $comm] \
                [list ::tclmpi::bcast {} $auto $master $comm]] [list $idata $idata]
par_return [list [list ::tclmpi::bcast {} $auto $master $comm] \
                [list ::tclmpi::bcast $idata $auto $master $comm]] [list {} {}]

set idata {016 {1 2 3} 2.0 7 0xff yy}
set odata {14 0 0 7 255 0}
par_return [list [list ::tclmpi::bcast {} $int 1 $comm] \
                [list ::tclmpi::bcast $idata $int 1 $comm]] [list $odata $odata]
set odata {14.0 0.0 2.0 7.0 255.0 0.0}
par_return [list [list ::tclmpi::bcast $idata $double $master $comm] \
                [list ::tclmpi::bcast {} $double $master $comm]] [list $odata $odata]

# when mixing $auto with other data types, we have mismatch or low-level MPI calls
# which is indicated in the truncated error message.
par_error  [list [list ::tclmpi::bcast $idata $double $master $comm] \
                [list ::tclmpi::bcast {} $auto $master $comm]] \
    [list $odata {::tclmpi::bcast: MPI_ERR_TRUNCATE: message truncated}]

# allreduce
set idata {0 1 3 0 1 10}
set odata {1 -1 0 0 1 18}

set rdata {0 1 0 0 1 1}
par_return [list [list ::tclmpi::allreduce $idata $int ::tclmpi::land $comm] \
                [list ::tclmpi::allreduce $odata $int ::tclmpi::land $comm]] \
    [list $rdata $rdata]
set rdata {1 1 1 0 1 1}
par_return [list [list ::tclmpi::allreduce $idata $int ::tclmpi::lor $comm] \
                [list ::tclmpi::allreduce $odata $int ::tclmpi::lor $comm]] \
    [list $rdata $rdata]
set rdata {1 0 1 0 0 0}
par_return [list [list ::tclmpi::allreduce $idata $int ::tclmpi::lxor $comm] \
                [list ::tclmpi::allreduce $odata $int ::tclmpi::lxor $comm]] \
    [list $rdata $rdata]
set rdata {0 1 0 0 1 2}
par_return [list [list ::tclmpi::allreduce $idata $int ::tclmpi::band $comm] \
                [list ::tclmpi::allreduce $odata $int ::tclmpi::band $comm]] \
    [list $rdata $rdata]
set rdata {1 -1 3 0 1 26}
par_return [list [list ::tclmpi::allreduce $idata $int ::tclmpi::bor $comm] \
                [list ::tclmpi::allreduce $odata $int ::tclmpi::bor $comm]] \
    [list $rdata $rdata]
set rdata {1 -2 3 0 0 24}
par_return [list [list ::tclmpi::allreduce $idata $int ::tclmpi::bxor $comm] \
                [list ::tclmpi::allreduce $odata $int ::tclmpi::bxor $comm]] \
    [list $rdata $rdata]

set rdata {1 1 3 0 1 18}
par_return [list [list ::tclmpi::allreduce $idata $int ::tclmpi::max $comm] \
                [list ::tclmpi::allreduce $odata $int ::tclmpi::max $comm]] \
    [list $rdata $rdata]
set rdata {0 -1 0 0 1 10}
par_return [list [list ::tclmpi::allreduce $idata $int ::tclmpi::min $comm] \
                [list ::tclmpi::allreduce $odata $int ::tclmpi::min $comm]] \
    [list $rdata $rdata]

set rdata {1 0 3 0 2 28}
par_return [list [list ::tclmpi::allreduce $idata $int ::tclmpi::sum $comm] \
                [list ::tclmpi::allreduce $odata $int ::tclmpi::sum $comm]] \
    [list $rdata $rdata]
set rdata {0 -1 0 0 1 180}
par_return [list [list ::tclmpi::allreduce $idata $int ::tclmpi::prod $comm] \
                [list ::tclmpi::allreduce $odata $int ::tclmpi::prod $comm]] \
    [list $rdata $rdata]

set idata {-1e5 1.1 1.2d0 0.2e-1 0.06E+28 0x22}
set rdata {-99999.0 0.10000000000000009 0.0 0.02 6e+26 52.0}
par_return [list [list ::tclmpi::allreduce $idata $double ::tclmpi::sum $comm] \
                [list ::tclmpi::allreduce $odata $double ::tclmpi::sum $comm]] \
    [list $rdata $rdata]
set rdata {-100000.0 -1.1 0.0 0.0 6e+26 612.0}
par_return [list [list ::tclmpi::allreduce $idata $double ::tclmpi::prod $comm] \
                [list ::tclmpi::allreduce $odata $double ::tclmpi::prod $comm]] \
    [list $rdata $rdata]

if {0} {
# check data type conversions
    run_return [list ::tclmpi::bcast {-1 2 +3 2.0 7 016} $int $master $comm] {-1 2 3 0 7 14}
    run_return [list ::tclmpi::bcast  $double $master $self] {-100000.0 1.1 0.0 0.02 6e+26 34.0}
}

# print results and exit
::tclmpi::finalize
test_summary 02

