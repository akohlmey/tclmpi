#!/usr/bin/tclsh
# tests that can be run with just one process
source harness.tcl

ser_init

# import all API from namespace
namespace import tclmpi::*
# use aliases instead of predefined variables
unset comm self null
namespace upvar tclmpi comm_world comm
namespace upvar tclmpi comm_self self
namespace upvar tclmpi comm_null null
namespace upvar tclmpi sum    mpi_sum
namespace upvar tclmpi prod   mpi_prod
namespace upvar tclmpi max    mpi_max
namespace upvar tclmpi min    mpi_min
namespace upvar tclmpi land   mpi_land
namespace upvar tclmpi auto   mpi_auto
namespace upvar tclmpi double mpi_double
namespace upvar tclmpi int    mpi_int
namespace upvar tclmpi minloc minloc
namespace upvar tclmpi maxloc maxloc
namespace upvar tclmpi undefined undefined

# init
run_error  [list init 0] \
    "wrong # args: should be \"init\""
run_return [list init] {}
run_error  [list init] \
    {Calling init multiple times is erroneous.}

# comm_size
set numargs "wrong # args: should be \"comm_size <comm>\""
run_error  [list comm_size]  $numargs
run_error  [list comm_size $comm $self] $numargs
run_error  [list comm_size comm0] \
    {comm_size: unknown communicator: comm0}
run_return [list comm_size $comm] 1
run_return [list comm_size $self] 1
run_error  [list comm_size $null] \
    {comm_size: MPI_ERR_COMM: invalid communicator}

# comm_rank
set numargs "wrong # args: should be \"comm_rank <comm>\""
run_error  [list comm_rank] $numargs
run_error  [list comm_rank $comm $self] $numargs
run_error  [list comm_rank comm0] \
    {comm_rank: unknown communicator: comm0}
run_return [list comm_rank $comm] 0
run_return [list comm_rank $self] 0
run_error  [list comm_rank $null] \
    {comm_rank: MPI_ERR_COMM: invalid communicator}

# comm_split
set split0 tclmpi::comm0
set split1 tclmpi::comm1
set split2 tclmpi::comm2
set numargs \
    "wrong # args: should be \"comm_split <comm> <color> <key>\""
run_error  [list comm_split] $numargs
run_error  [list comm_split $comm 1] $numargs
run_error  [list comm_split $comm 1 1 1] $numargs
run_error  [list comm_split comm0 0 0]  \
    {comm_split: unknown communicator: comm0}
run_return [list comm_split $comm 5 -1] [list $split0]
run_return [list comm_split $comm 0 0]  [list $split1]
run_return [list comm_split $self 4 -1] [list $split2]
run_return [list comm_split $self $undefined -1] [list $null]
run_error  [list comm_split $comm -1 0] \
    {comm_split: MPI_ERR_ARG: invalid argument of some other kind}
run_error  [list comm_split $null 5 0]  \
    {comm_split: MPI_ERR_COMM: invalid communicator}
run_error  [list comm_split $comm x 0]  \
    {expected integer but got "x"}
run_error  [list comm_split $comm 0 x]  \
    {expected integer but got "x"}

# check size and rank on generated communicators
run_return [list comm_size $split0] 1
run_return [list comm_size $split1] 1
run_return [list comm_rank $split0] 0
run_return [list comm_rank $split1] 0

# barrier
set numargs "wrong # args: should be \"barrier <comm>\""
run_error  [list barrier] $numargs
run_error  [list barrier $comm 1] $numargs
run_error  [list barrier comm0]   \
    {barrier: unknown communicator: comm0}
run_error  [list barrier $null]   \
    {barrier: MPI_ERR_COMM: invalid communicator}
run_return [list barrier $comm] {}
run_return [list barrier $self] {}

# bcast
set numargs \
    "wrong # args: should be \"bcast <data> <type> <root> <comm>\""
run_error  [list bcast] $numargs
run_error  [list bcast {}] $numargs
run_error  [list bcast {} $auto] $numargs
run_error  [list bcast {} $auto $master] $numargs
run_error  [list bcast {} $auto $master $comm xxx] $numargs
run_error  [list bcast {} $auto $master comm0] \
    {bcast: unknown communicator: comm0}
run_error  [list bcast {} $auto $master $null] \
    {bcast: MPI_ERR_COMM: invalid communicator}
run_error  [list bcast {{xx 11} {1 2 3} {}} $auto 1 $comm] \
    {bcast: MPI_ERR_ROOT: invalid root}

# check data type conversions
run_return [list bcast {{xx 11} {1 2 3} {}}            \
                $auto $master $comm] {{xx 11} {1 2 3} {}}
run_return [list bcast {{xx 11} {1 2 3} {}}            \
                $auto $master $self] {{xx 11} {1 2 3} {}}
run_return [list bcast {{xx 11} {1 2 3} 2.0 7 0xff yy} \
                $int $master $self] {0 0 0 7 255 0}
run_return [list bcast {{xx 11} {1 2 3} 2.5 yy 1}      \
                $double $master $self] {0.0 0.0 2.5 0.0 1.0}
run_return [list bcast {-1 2 +3 2.0 7 016}             \
                $int $master $comm] {-1 2 3 0 7 14}
run_return [list bcast {-1e5 1.1 1.2d0 0.2e-1 0.06E+28 0x22} \
                $double $master $self] {-100000.0 1.1 0.0 0.02 6e+26 34.0}


# scatter
set numargs \
    "wrong # args: should be \"scatter <data> <type> <root> <comm>\""
run_error  [list scatter] $numargs
run_error  [list scatter {}] $numargs
run_error  [list scatter {} $auto] $numargs
run_error  [list scatter {} $auto $master] $numargs
run_error  [list scatter {} $auto $master $comm xxx] $numargs
run_error  [list scatter {} $auto $master comm0] \
    {scatter: unknown communicator: comm0}
run_error  [list scatter {} $auto $master $null] \
    {scatter: does not support data type tclmpi::auto}
run_error  [list scatter {{xx 11} {1 2 3} {}} $int 1 $comm] \
    {scatter: MPI_ERR_ROOT: invalid root}
run_error  [list scatter {} tclmpi::real $master $comm]    \
    {scatter: invalid data type: tclmpi::real}

# check data type conversions
run_return [list scatter {{xx 11} {1 2 3} 2.0 7 0xff yy} \
                $int $master $self] {0 0 0 7 255 0}
run_return [list scatter {{xx 11} {1 2 3} 2.5 yy 1}      \
                $double $master $comm] {0.0 0.0 2.5 0.0 1.0}
run_return [list scatter {-1 2 +3 2.0 7 016}             \
                $int $master $comm] {-1 2 3 0 7 14}
run_return [list scatter {-1e5 1.1 1.2d0 0.2e-1 0.06E+28 0x22} \
                $double $master $self] {-100000.0 1.1 0.0 0.02 6e+26 34.0}

# gather
set numargs \
    "wrong # args: should be \"gather <data> <type> <root> <comm>\""
run_error  [list gather] $numargs
run_error  [list gather {}] $numargs
run_error  [list gather {} $auto] $numargs
run_error  [list gather {} $auto $master] $numargs
run_error  [list gather {} $auto $master $comm xxx] $numargs
run_error  [list gather {} $auto $master comm0] \
    {gather: unknown communicator: comm0}
run_error  [list gather {} $auto $master $null] \
    {gather: does not support data type tclmpi::auto}
run_error  [list gather {{xx 11} {1 2 3} {}} $int 1 $comm] \
    {gather: MPI_ERR_ROOT: invalid root}
run_error  [list gather {} tclmpi::real $master $comm]    \
    {gather: invalid data type: tclmpi::real}

# check data type conversions
run_return [list gather {{xx 11} {1 2 3} 2.0 7 0xff yy} \
                $int $master $self] {0 0 0 7 255 0}
run_return [list gather {{xx 11} {1 2 3} 2.5 yy 1}      \
                $double $master $comm] {0.0 0.0 2.5 0.0 1.0}
run_return [list gather {-1 2 +3 2.0 7 016}             \
                $int $master $comm] {-1 2 3 0 7 14}
run_return [list gather {-1e5 1.1 1.2d0 0.2e-1 0.06E+28 0x22} \
                $double $master $self] {-100000.0 1.1 0.0 0.02 6e+26 34.0}

# reduce
set numargs \
    "wrong # args: should be \"reduce <data> <type> <op> <root> <comm>\""
run_error  [list reduce] $numargs
run_error  [list reduce {}] $numargs
run_error  [list reduce {} $auto] $numargs
run_error  [list reduce {} $auto $mpi_sum] $numargs
run_error  [list reduce {} $auto $mpi_prod 0] $numargs
run_error  [list reduce {} $auto $mpi_prod 0 $comm xxx] $numargs
run_error  [list reduce {} $auto $mpi_max 0 $comm]      \
    {reduce: does not support data type tclmpi::auto}
run_error  [list reduce {} $int $mpi_max 0 comm0]       \
    {reduce: unknown communicator: comm0}
run_error  [list reduce {} $int $maxloc 0 $comm]    \
    {reduce: MPI_ERR_OP: invalid reduce operation}
run_error  [list reduce {} $double $minloc 0 $comm] \
    {reduce: MPI_ERR_OP: invalid reduce operation}
run_error [list reduce {1 0 2 1 4 3} $intint \
               tclmpi::max 0 $comm] \
    {reduce: MPI_ERR_OP: invalid reduce operation}
run_error  [list reduce {} tclmpi::real $mpi_min 0 $comm] \
    {reduce: invalid data type: tclmpi::real}
run_error  [list reduce {} $int $mpi_land 0 $null]          \
    {reduce: MPI_ERR_COMM: invalid communicator}
run_error  [list reduce {{}} $int tclmpi::gamma 0 $comm]       \
    {reduce: unknown reduction operator: tclmpi::gamma}
run_return [list reduce {2 0 1 1} $intint \
                tclmpi::maxloc 0 $comm] {2 0 1 1}
#run_return [list reduce {1.0 0 2.0 1} $dblint \
    tclmpi::minloc $comm] {1.0 0 2.0 1}

# allreduce
set numargs \
    "wrong # args: should be \"allreduce <data> <type> <op> <comm>\""
run_error  [list allreduce] $numargs
run_error  [list allreduce {}] $numargs
run_error  [list allreduce {} $auto] $numargs
run_error  [list allreduce {} $auto $mpi_sum] $numargs
run_error  [list allreduce {} $auto $mpi_prod $comm xxx] $numargs
run_error  [list allreduce {} $auto $mpi_max $comm]      \
    {allreduce: does not support data type tclmpi::auto}
run_error  [list allreduce {} $int $mpi_max comm0]       \
    {allreduce: unknown communicator: comm0}
run_error  [list allreduce {} $int $maxloc $comm]    \
    {allreduce: MPI_ERR_OP: invalid reduce operation}
run_error  [list allreduce {} $double $minloc $comm] \
    {allreduce: MPI_ERR_OP: invalid reduce operation}
run_error [list allreduce {1 0 2 1 4 3} $intint \
               tclmpi::max $comm] \
    {allreduce: MPI_ERR_OP: invalid reduce operation}
run_error  [list allreduce {} tclmpi::real $mpi_min $comm] \
    {allreduce: invalid data type: tclmpi::real}
run_error  [list allreduce {} $int $mpi_land $null]          \
    {allreduce: MPI_ERR_COMM: invalid communicator}
run_error  [list allreduce {{}} $int tclmpi::gamma $comm]       \
    {allreduce: unknown reduction operator: tclmpi::gamma}
run_return [list allreduce {2 0 1 1} $intint $maxloc $comm] {2 0 1 1}
#run_return [list allreduce {1.0 0 2.0 1} $dblint \
    tclmpi::minloc $comm] {1.0 0 2.0 1}

# check some data type conversions
run_return [list allreduce {{xx 11} {1 2 3} 2.0 7 0xff yy} \
                $int tclmpi::max $self] {0 0 0 7 255 0}
run_return [list allreduce {{xx 11} {1 2 3} 2.5 yy 1}      \
                $double tclmpi::sum $comm] {0.0 0.0 2.5 0.0 1.0}
run_return [list allreduce {-1 2 +3 2.0 7 016}             \
                $int tclmpi::prod $comm] {-1 2 3 0 7 14}

# probe
set numargs \
    "wrong # args: should be \"probe <source> <tag> <comm> ?status?\""
run_error  [list probe] $numargs
run_error  [list probe 0] $numargs
run_error  [list probe 0 0] $numargs
run_error  [list probe 0 0 $comm status xxx] $numargs
run_error  [list probe 0 0 comm0]                    \
    {probe: unknown communicator: comm0}
run_error  [list probe tclmpi::any_tag 0 $comm]    \
    {expected integer but got "tclmpi::any_tag"}
run_error  [list probe 0 tclmpi::any_source $comm] \
    {expected integer but got "tclmpi::any_source"}
run_error  [list probe tclmpi::any_source \
                tclmpi::any_tag $null] \
    {probe: invalid communicator: tclmpi::comm_null}

# abort (non-destructive tests only)
set numargs "wrong # args: should be \"abort <comm> <errorcode>\""
run_error  [list abort] $numargs
run_error  [list abort $comm] $numargs
run_error  [list abort $comm 1 2] $numargs
run_error  [list abort comm0 1] \
    {abort: unknown communicator: comm0}
run_error  [list abort $comm comm0] \
    {expected integer but got "comm0"}

# finalize
run_error  [list finalize 0] \
    "wrong # args: should be \"finalize\""
run_return [list finalize] {}
run_error  [list finalize] \
    {Calling finalize twice is erroneous.}

# print results and exit
test_summary 02

exit 0
