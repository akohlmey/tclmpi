#!/usr/bin/tclsh
# tests that can be run with just one process
source harness.tcl

ser_init

# init
run_error  [list ::tclmpi::init 0] \
    "wrong # args: should be \"::tclmpi::init\""
run_return [list ::tclmpi::init] {}
run_error  [list ::tclmpi::init] \
    {Calling ::tclmpi::init multiple times is erroneous.}

# comm_size
set numargs "wrong # args: should be \"::tclmpi::comm_size <comm>\""
run_error  [list ::tclmpi::comm_size]  $numargs
run_error  [list ::tclmpi::comm_size $comm $self] $numargs
run_error  [list ::tclmpi::comm_size comm0] \
    {::tclmpi::comm_size: unknown communicator: comm0}
run_return [list ::tclmpi::comm_size $comm] 1
run_return [list ::tclmpi::comm_size $self] 1
run_error  [list ::tclmpi::comm_size $null] \
    {::tclmpi::comm_size: MPI_ERR_COMM: invalid communicator}

# comm_rank
set numargs "wrong # args: should be \"::tclmpi::comm_rank <comm>\""
run_error  [list ::tclmpi::comm_rank] $numargs
run_error  [list ::tclmpi::comm_rank $comm $self] $numargs
run_error  [list ::tclmpi::comm_rank comm0] \
    {::tclmpi::comm_rank: unknown communicator: comm0}
run_return [list ::tclmpi::comm_rank $comm] 0
run_return [list ::tclmpi::comm_rank $self] 0
run_error  [list ::tclmpi::comm_rank $null] \
    {::tclmpi::comm_rank: MPI_ERR_COMM: invalid communicator}

# comm_split
set split0 tclmpi::comm0
set split1 tclmpi::comm1
set numargs \
    "wrong # args: should be \"::tclmpi::comm_split <comm> <color> <key>\""
run_error  [list ::tclmpi::comm_split] $numargs
run_error  [list ::tclmpi::comm_split $comm 1] $numargs
run_error  [list ::tclmpi::comm_split $comm 1 1 1] $numargs
run_error  [list ::tclmpi::comm_split comm0 0 0]  \
    {::tclmpi::comm_split: unknown communicator: comm0}
run_return [list ::tclmpi::comm_split $comm 5 -1] {tclmpi::comm0}
run_return [list ::tclmpi::comm_split $comm 0 0]  {tclmpi::comm1}
run_return [list ::tclmpi::comm_split $self 4 -1] {tclmpi::comm2}
run_return [list ::tclmpi::comm_split $self tclmpi::undefined -1] \
    {tclmpi::comm_null}
run_error  [list ::tclmpi::comm_split $comm -1 0] \
    {::tclmpi::comm_split: MPI_ERR_ARG: invalid argument of some other kind}
run_error  [list ::tclmpi::comm_split $null 5 0]  \
    {::tclmpi::comm_split: MPI_ERR_COMM: invalid communicator}
run_error  [list ::tclmpi::comm_split $comm x 0]  \
    {expected integer but got "x"}
run_error  [list ::tclmpi::comm_split $comm 0 x]  \
    {expected integer but got "x"}

# check size and rank on generated communicators
run_return [list ::tclmpi::comm_size $split0] 1
run_return [list ::tclmpi::comm_size $split1] 1
run_return [list ::tclmpi::comm_rank $split0] 0
run_return [list ::tclmpi::comm_rank $split1] 0

# barrier
set numargs "wrong # args: should be \"::tclmpi::barrier <comm>\""
run_error  [list ::tclmpi::barrier] $numargs
run_error  [list ::tclmpi::barrier $comm 1] $numargs
run_error  [list ::tclmpi::barrier comm0]   \
    {::tclmpi::barrier: unknown communicator: comm0}
run_error  [list ::tclmpi::barrier $null]   \
    {::tclmpi::barrier: MPI_ERR_COMM: invalid communicator}
run_return [list ::tclmpi::barrier $comm] {}
run_return [list ::tclmpi::barrier $self] {}

# bcast
set numargs \
    "wrong # args: should be \"::tclmpi::bcast <data> <type> <root> <comm>\""
run_error  [list ::tclmpi::bcast] $numargs
run_error  [list ::tclmpi::bcast {}] $numargs
run_error  [list ::tclmpi::bcast {} $auto] $numargs
run_error  [list ::tclmpi::bcast {} $auto $master] $numargs
run_error  [list ::tclmpi::bcast {} $auto $master $comm xxx] $numargs
run_error  [list ::tclmpi::bcast {} $auto $master comm0] \
    {::tclmpi::bcast: unknown communicator: comm0}
run_error  [list ::tclmpi::bcast {} $auto $master $null] \
    {::tclmpi::bcast: MPI_ERR_COMM: invalid communicator}
run_error  [list ::tclmpi::bcast {{xx 11} {1 2 3} {}} $auto 1 $comm] \
    {::tclmpi::bcast: MPI_ERR_ROOT: invalid root}
run_error  [list ::tclmpi::bcast {} ::tclmpi::real $master $comm]    \
    {::tclmpi::bcast: invalid data type: ::tclmpi::real}

# check data type conversions
run_return [list ::tclmpi::bcast {{xx 11} {1 2 3} {}}            \
                $auto $master $comm] {{xx 11} {1 2 3} {}}
run_return [list ::tclmpi::bcast {{xx 11} {1 2 3} {}}            \
                $auto $master $self] {{xx 11} {1 2 3} {}}
run_return [list ::tclmpi::bcast {{xx 11} {1 2 3} 2.0 7 0xff yy} \
                $int $master $self] {0 0 0 7 255 0}
run_return [list ::tclmpi::bcast {{xx 11} {1 2 3} 2.5 yy 1}      \
                $double $master $self] {0.0 0.0 2.5 0.0 1.0}
run_return [list ::tclmpi::bcast {-1 2 +3 2.0 7 016}             \
                $int $master $comm] {-1 2 3 0 7 14}
run_return [list ::tclmpi::bcast {-1e5 1.1 1.2d0 0.2e-1 0.06E+28 0x22} \
                $double $master $self] {-100000.0 1.1 0.0 0.02 6e+26 34.0}

# allreduce
set numargs \
    "wrong # args: should be \"::tclmpi::allreduce <data> <type> <op> <comm>\""
run_error  [list ::tclmpi::allreduce] $numargs
run_error  [list ::tclmpi::allreduce {}] $numargs
run_error  [list ::tclmpi::allreduce {} $auto] $numargs
run_error  [list ::tclmpi::allreduce {} $auto tclmpi::sum] $numargs
run_error  [list ::tclmpi::allreduce {} $auto tclmpi::prod $comm xxx] $numargs
run_error  [list ::tclmpi::allreduce {} $auto tclmpi::max $comm]      \
    {::tclmpi::allreduce: does not support data type tclmpi::auto}
run_error  [list ::tclmpi::allreduce {} $int tclmpi::max comm0]       \
    {::tclmpi::allreduce: unknown communicator: comm0}
run_error  [list ::tclmpi::allreduce {} $int tclmpi::maxloc $comm]    \
    {::tclmpi::allreduce: MPI_ERR_OP: invalid reduce operation}
run_error  [list ::tclmpi::allreduce {} $double tclmpi::minloc $comm] \
    {::tclmpi::allreduce: MPI_ERR_OP: invalid reduce operation}
run_error [list ::tclmpi::allreduce {1 0 2 1 4 3} $intint \
               tclmpi::max $comm] \
    {::tclmpi::allreduce: MPI_ERR_OP: invalid reduce operation}
run_error  [list ::tclmpi::allreduce {} tclmpi::real tclmpi::min $comm] \
    {::tclmpi::allreduce: invalid data type: tclmpi::real}
run_error  [list ::tclmpi::allreduce {} $int tclmpi::land $null]          \
    {::tclmpi::allreduce: MPI_ERR_COMM: invalid communicator}
run_error  [list ::tclmpi::allreduce {{}} $int tclmpi::gamma $comm]       \
    {::tclmpi::allreduce: unknown reduction operator: tclmpi::gamma}
run_return [list ::tclmpi::allreduce {2 0 1 1} $intint \
                tclmpi::maxloc $comm] {2 0 1 1}
#run_return [list ::tclmpi::allreduce {1.0 0 2.0 1} $dblint \
::tclmpi::minloc $comm] {1.0 0 2.0 1}

# check some data type conversions
run_return [list ::tclmpi::allreduce {{xx 11} {1 2 3} 2.0 7 0xff yy} \
                $int tclmpi::max $self] {0 0 0 7 255 0}
run_return [list ::tclmpi::allreduce {{xx 11} {1 2 3} 2.5 yy 1}      \
                $double tclmpi::sum $comm] {0.0 0.0 2.5 0.0 1.0}
run_return [list ::tclmpi::allreduce {-1 2 +3 2.0 7 016}             \
                $int tclmpi::prod $comm] {-1 2 3 0 7 14}

# probe
set numargs \
    "wrong # args: should be \"::tclmpi::probe <source> <tag> <comm> ?status?\""
run_error  [list ::tclmpi::probe] $numargs
run_error  [list ::tclmpi::probe 0] $numargs
run_error  [list ::tclmpi::probe 0 0] $numargs
run_error  [list ::tclmpi::probe 0 0 $comm status xxx] $numargs
run_error  [list ::tclmpi::probe 0 0 comm0]                    \
    {::tclmpi::probe: unknown communicator: comm0}
run_error  [list ::tclmpi::probe tclmpi::any_tag 0 $comm]    \
    {expected integer but got "tclmpi::any_tag"}
run_error  [list ::tclmpi::probe 0 tclmpi::any_source $comm] \
    {expected integer but got "tclmpi::any_source"}
run_error  [list ::tclmpi::probe tclmpi::any_source \
                tclmpi::any_tag $null] \
    {::tclmpi::probe: invalid communicator: tclmpi::comm_null}

# abort (non-destructive tests only)
set numargs "wrong # args: should be \"::tclmpi::abort <comm> <errorcode>\""
run_error  [list ::tclmpi::abort] $numargs
run_error  [list ::tclmpi::abort $comm] $numargs
run_error  [list ::tclmpi::abort $comm 1 2] $numargs
run_error  [list ::tclmpi::abort comm0 1] \
    {::tclmpi::abort: unknown communicator: comm0}
run_error  [list ::tclmpi::abort $comm comm0] \
    {expected integer but got "comm0"}

# finalize
run_error  [list ::tclmpi::finalize 0] \
    "wrong # args: should be \"::tclmpi::finalize\""
run_return [list ::tclmpi::finalize] {}
run_error  [list ::tclmpi::finalize] \
    {Calling ::tclmpi::finalize twice is erroneous.}

# print results and exit
test_summary 01

exit 0
