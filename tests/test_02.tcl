#!/usr/bin/tclsh
# tests that can be run with just one process
source harness.tcl

set comm   ::tclmpi::comm_world
set self   ::tclmpi::comm_self
set null   ::tclmpi::comm_null
set split0 ::tclmpi::comm0
set split1 ::tclmpi::comm1

# parallel init
par_init

par_return [list [list ::tclmpi::comm_size $comm] \
                 [list ::tclmpi::comm_size $comm]] [list 2 2]
par_return [list [list ::tclmpi::comm_rank $comm] \
                 [list ::tclmpi::comm_rank $comm]] [list 0 1]
par_return [list [list ::tclmpi::comm_size $self] \
                 [list ::tclmpi::comm_size $self]] [list 1 1]
par_return [list [list ::tclmpi::comm_rank $self] \
                 [list ::tclmpi::comm_rank $self]] [list 0 0]

if {0} {
run_return [list ::tclmpi::comm_rank $comm] 0
run_return [list ::tclmpi::comm_rank $self] 0
run_error  [list ::tclmpi::comm_rank $null] {::tclmpi::comm_rank: MPI_ERR_COMM: invalid communicator}

# comm_split
run_error  [list ::tclmpi::comm_split] "wrong # args: should be \"::tclmpi::comm_split <comm> <color> <key>\""
run_error  [list ::tclmpi::comm_split $comm 1] "wrong # args: should be \"::tclmpi::comm_split <comm> <color> <key>\""
run_error  [list ::tclmpi::comm_split comm0 0 0]  {::tclmpi::comm_split: unknown communicator: comm0}
run_return [list ::tclmpi::comm_split $comm 5 -1] {::tclmpi::comm0}
run_return [list ::tclmpi::comm_split $comm 0 0]  {::tclmpi::comm1}
run_return [list ::tclmpi::comm_split $self 4 -1] {::tclmpi::comm2}
run_return [list ::tclmpi::comm_split $self ::tclmpi::undefined -1] {::tclmpi::comm_null}
run_error  [list ::tclmpi::comm_split $comm -1 0] {::tclmpi::comm_split: MPI_ERR_ARG: invalid argument of some other kind}
run_error  [list ::tclmpi::comm_split $null 5 0]  {::tclmpi::comm_split: MPI_ERR_COMM: invalid communicator}

# check size and rank on generated communicators
run_return [list ::tclmpi::comm_size $split0] 2
run_return [list ::tclmpi::comm_size $split1] 2
run_return [list ::tclmpi::comm_rank $split0] 0
run_return [list ::tclmpi::comm_rank $split1] 0

# barrier
run_error  [list ::tclmpi::barrier]         "wrong # args: should be \"::tclmpi::barrier <comm>\""
run_error  [list ::tclmpi::barrier $comm 1] "wrong # args: should be \"::tclmpi::barrier <comm>\""
run_error  [list ::tclmpi::barrier comm0]   {::tclmpi::barrier: unknown communicator: comm0}
run_return [list ::tclmpi::barrier $comm] {}
run_return [list ::tclmpi::barrier $self] {}
run_error  [list ::tclmpi::barrier $null]  {::tclmpi::barrier: MPI_ERR_COMM: invalid communicator}

# finalize
run_error  [list ::tclmpi::finalize 0] "wrong # args: should be \"::tclmpi::finalize\""
run_error  [list ::tclmpi::finalize] {Calling ::tclmpi::finalize twice is erroneous.}
}

# print results and exit
test_summary 02
::tclmpi::finalize

