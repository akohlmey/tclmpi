#!/usr/bin/tclsh
# tests that can be run with just one process
source harness.tcl

ser_init

# init
run_error  [list ::tclmpi::init 0] \
    [list "wrong # args: should be \"::tclmpi::init\""]
run_return [list ::tclmpi::init] {}
run_error  [list ::tclmpi::init] \
    {{calling ::tclmpi::init multiple times is erroneous.}}

# comm_size
set numargs "wrong # args: should be \"::tclmpi::comm_size <comm>\""
run_error  [list ::tclmpi::comm_size]  [list $numargs]
run_error  [list ::tclmpi::comm_size $comm $self] [list $numargs]
run_error  [list ::tclmpi::comm_size comm0] \
    {::tclmpi::comm_size: unknown communicator comm0}
run_return [list ::tclmpi::comm_size $comm] 1
run_return [list ::tclmpi::comm_size $self] 1
run_error  [list ::tclmpi::comm_size $null] \
    {::tclmpi::comm_size: mpi invalid communicator}

# comm_rank
set numargs "wrong # args: should be \"::tclmpi::comm_rank <comm>\""
run_error  [list ::tclmpi::comm_rank] [list $numargs]
run_error  [list ::tclmpi::comm_rank $comm $self] [list $numargs]
run_error  [list ::tclmpi::comm_rank comm0] \
    {::tclmpi::comm_rank: unknown communicator: comm0}
run_return [list ::tclmpi::comm_rank $comm] 0
run_return [list ::tclmpi::comm_rank $self] 0
run_error  [list ::tclmpi::comm_rank $null] \
    {::tclmpi::comm_rank: mpi invalid communicator}

# comm_split
set split0 tclmpi::comm0
set split1 tclmpi::comm1
set split2 tclmpi::comm2
set numargs \
    "wrong # args: should be \"::tclmpi::comm_split <comm> <color> <key>\""
run_error  [list ::tclmpi::comm_split] [list $numargs]
run_error  [list ::tclmpi::comm_split $comm 1] [list $numargs]
run_error  [list ::tclmpi::comm_split $comm 1 1 1] [list $numargs]
run_error  [list ::tclmpi::comm_split comm0 0 0]  \
    {{::tclmpi::comm_split: unknown communicator: comm0}}
run_return [list ::tclmpi::comm_split $comm 5 -1] {tclmpi::comm0}
run_return [list ::tclmpi::comm_split $comm 0 0]  {tclmpi::comm1}
run_return [list ::tclmpi::comm_split $self 4 -1] {tclmpi::comm2}
run_return [list ::tclmpi::comm_split $self tclmpi::undefined -1] \
    {tclmpi::comm_null}
run_error  [list ::tclmpi::comm_split $comm -5 0] \
    {{::tclmpi::comm_split: invalid color argument}}
run_error  [list ::tclmpi::comm_split $null 5 0]  \
    {::tclmpi::comm_split: mpi invalid communicator}
run_error  [list ::tclmpi::comm_split $comm x 0]  \
    {{expected integer but got "x"}}
run_error  [list ::tclmpi::comm_split $comm 0 x]  \
    {{expected integer but got "x"}}

# check size and rank on generated communicators
run_return [list ::tclmpi::comm_size $split0] 1
run_return [list ::tclmpi::comm_size $split1] 1
run_return [list ::tclmpi::comm_rank $split0] 0
run_return [list ::tclmpi::comm_rank $split1] 0

# comm_free
set numargs \
    "wrong # args: should be \"::tclmpi::comm_free <comm>\""
run_error  [list ::tclmpi::comm_free] [list $numargs]
run_error  [list ::tclmpi::comm_free $comm 1] [list $numargs]
run_error  [list ::tclmpi::comm_free comm0]  \
    {{::tclmpi::comm_free: unknown communicator: comm0}}
run_return [list ::tclmpi::comm_free $split2] {}

# barrier
set numargs "wrong # args: should be \"::tclmpi::barrier <comm>\""
run_error  [list ::tclmpi::barrier] [list $numargs]
run_error  [list ::tclmpi::barrier $comm 1] [list $numargs]
run_error  [list ::tclmpi::barrier comm0]   \
    {{::tclmpi::barrier: unknown communicator: comm0}}
run_error  [list ::tclmpi::barrier $null]   \
    {::tclmpi::barrier: mpi invalid communicator}
run_return [list ::tclmpi::barrier $comm] {}
run_return [list ::tclmpi::barrier $self] {}

# bcast
set numargs \
    "wrong # args: should be \"::tclmpi::bcast <data> <type> <root> <comm>\""
run_error  [list ::tclmpi::bcast] [list $numargs]
run_error  [list ::tclmpi::bcast {}] [list $numargs]
run_error  [list ::tclmpi::bcast {} $auto] [list $numargs]
run_error  [list ::tclmpi::bcast {} $auto $master] [list $numargs]
run_error  [list ::tclmpi::bcast {} $auto $master $comm xxx] [list $numargs]
run_error  [list ::tclmpi::bcast {} $auto $master comm0] \
    {{::tclmpi::bcast: unknown communicator: comm0}}
run_error  [list ::tclmpi::bcast {} $auto $master $null] \
    {::tclmpi::bcast: mpi invalid communicator}
run_error  [list ::tclmpi::bcast {{xx 11} {1 2 3} {}} $auto 1 $comm] \
    {::tclmpi::bcast: mpi invalid root}
run_error  [list ::tclmpi::bcast {} tclmpi::real $master $comm]    \
    {{::tclmpi::bcast: invalid data type: tclmpi::real}}

# check data type conversions
run_return [list ::tclmpi::bcast {{xx 11} {1 2 3} {}}            \
                $auto $master $comm] {{{xx 11} {1 2 3} {}}}
run_return [list ::tclmpi::bcast {{xx 11} {1 2 3} {}}            \
                $auto $master $self] {{{xx 11} {1 2 3} {}}}
run_return [list ::tclmpi::bcast {{xx 11} {1 2 3} 2.0 7 0xff yy} \
                $int $master $self] {{0 0 0 7 255 0}}
run_return [list ::tclmpi::bcast {{xx 11} {1 2 3} 2.5 yy 1}      \
                $double $master $self] {{0.0 0.0 2.5 0.0 1.0}}
run_return [list ::tclmpi::bcast {-1 2 +3 2.0 7 016}             \
                $int $master $comm] {{-1 2 3 0 7 14}}
run_return [list ::tclmpi::bcast {-1e5 1.1 1.2d0 0.2e-1 0.06E+28 0x22} \
                $double $master $self] {{-100000.0 1.1 0.0 0.02 6e+26 34.0}}

# scatter
set numargs \
    "wrong # args: should be \"::tclmpi::scatter <data> <type> <root> <comm>\""
run_error  [list ::tclmpi::scatter] [list $numargs]
run_error  [list ::tclmpi::scatter {}] [list $numargs]
run_error  [list ::tclmpi::scatter {} $auto] [list $numargs]
run_error  [list ::tclmpi::scatter {} $auto $master] [list $numargs]
run_error  [list ::tclmpi::scatter {} $auto $master $comm xxx] [list $numargs]
run_error  [list ::tclmpi::scatter {} $auto $master comm0] \
    {{::tclmpi::scatter: unknown communicator: comm0}}
run_error  [list ::tclmpi::scatter {} $auto $master $null] \
    {{::tclmpi::scatter: does not support data type tclmpi::auto}}
run_error  [list ::tclmpi::scatter {{xx 11} {1 2 3} {}} $int 1 $comm] \
    {::tclmpi::scatter: mpi invalid root}
run_error  [list ::tclmpi::scatter {} tclmpi::real $master $comm]    \
    {{::tclmpi::scatter: invalid data type: tclmpi::real}}

# check data type conversions
run_return [list ::tclmpi::scatter {{xx 11} {1 2 3} 2.0 7 0xff yy} \
                $int $master $self] {{0 0 0 7 255 0}}
run_return [list ::tclmpi::scatter {{xx 11} {1 2 3} 2.5 yy 1}      \
                $double $master $comm] {{0.0 0.0 2.5 0.0 1.0}}
run_return [list ::tclmpi::scatter {-1 2 +3 2.0 7 016}             \
                $int $master $comm] {{-1 2 3 0 7 14}}
run_return [list ::tclmpi::scatter {-1e5 1.1 1.2d0 0.2e-1 0.06E+28 0x22} \
                $double $master $self] {{-100000.0 1.1 0.0 0.02 6e+26 34.0}}

# allgather
set numargs \
    "wrong # args: should be \"::tclmpi::allgather <data> <type> <comm>\""
run_error  [list ::tclmpi::allgather] [list $numargs]
run_error  [list ::tclmpi::allgather {}] [list $numargs]
run_error  [list ::tclmpi::allgather {} $auto] [list $numargs]
run_error  [list ::tclmpi::allgather {} $auto $comm xxx] [list $numargs]
run_error  [list ::tclmpi::allgather {} $auto comm0] \
    {{::tclmpi::allgather: unknown communicator: comm0}}
run_error  [list ::tclmpi::allgather {} $auto $null] \
    {{::tclmpi::allgather: does not support data type tclmpi::auto}}
run_error  [list ::tclmpi::allgather {} tclmpi::real $comm]  \
    {{::tclmpi::allgather: invalid data type: tclmpi::real}}

# check data type conversions
run_return [list ::tclmpi::allgather {{xx 11} {1 2 3} 2.0 7 0xff yy} \
                $int $self] {{0 0 0 7 255 0}}
run_return [list ::tclmpi::allgather {{xx 11} {1 2 3} 2.5 yy 1}      \
                $double $comm] {{0.0 0.0 2.5 0.0 1.0}}
run_return [list ::tclmpi::allgather {-1 2 +3 2.0 7 016}             \
                $int $comm] {{-1 2 3 0 7 14}}
run_return [list ::tclmpi::allgather {-1e5 1.1 1.2d0 0.2e-1 0.06E+28 0x22} \
                $double $self] {{-100000.0 1.1 0.0 0.02 6e+26 34.0}}

# gather
set numargs \
    "wrong # args: should be \"::tclmpi::gather <data> <type> <root> <comm>\""
run_error  [list ::tclmpi::gather] [list $numargs]
run_error  [list ::tclmpi::gather {}] [list $numargs]
run_error  [list ::tclmpi::gather {} $auto] [list $numargs]
run_error  [list ::tclmpi::gather {} $auto $master] [list $numargs]
run_error  [list ::tclmpi::gather {} $auto $master $comm xxx] [list $numargs]
run_error  [list ::tclmpi::gather {} $auto $master comm0] \
    {{::tclmpi::gather: unknown communicator: comm0}}
run_error  [list ::tclmpi::gather {} $auto $master $null] \
    {{::tclmpi::gather: does not support data type tclmpi::auto}}
run_error  [list ::tclmpi::gather {{xx 11} {1 2 3} {}} $int 1 $comm] \
    {::tclmpi::gather: mpi invalid root}
run_error  [list ::tclmpi::gather {} tclmpi::real $master $comm]    \
    {{::tclmpi::gather: invalid data type: tclmpi::real}}

# check data type conversions
run_return [list ::tclmpi::gather {{xx 11} {1 2 3} 2.0 7 0xff yy} \
                $int $master $self] {{0 0 0 7 255 0}}
run_return [list ::tclmpi::gather {{xx 11} {1 2 3} 2.5 yy 1}      \
                $double $master $comm] {{0.0 0.0 2.5 0.0 1.0}}
run_return [list ::tclmpi::gather {-1 2 +3 2.0 7 016}             \
                $int $master $comm] {{-1 2 3 0 7 14}}
run_return [list ::tclmpi::gather {-1e5 1.1 1.2d0 0.2e-1 0.06E+28 0x22} \
                $double $master $self] {{-100000.0 1.1 0.0 0.02 6e+26 34.0}}

# allreduce
set numargs \
    "wrong # args: should be \"::tclmpi::allreduce <data> <type> <op> <comm>\""
run_error  [list ::tclmpi::allreduce] [list $numargs]
run_error  [list ::tclmpi::allreduce {}] [list $numargs]
run_error  [list ::tclmpi::allreduce {} $auto] [list $numargs]
run_error  [list ::tclmpi::allreduce {} $auto tclmpi::sum] [list $numargs]
run_error  [list ::tclmpi::allreduce {} $auto tclmpi::prod $comm xxx] \
    [list $numargs]
run_error  [list ::tclmpi::allreduce {} $auto tclmpi::max $comm]      \
    {{::tclmpi::allreduce: does not support data type tclmpi::auto}}
run_error  [list ::tclmpi::allreduce {} $int tclmpi::max comm0]       \
    {{::tclmpi::allreduce: unknown communicator: comm0}}
run_error  [list ::tclmpi::allreduce {} $int tclmpi::maxloc $comm]    \
    {::tclmpi::allreduce: invalid mpi op}
run_error  [list ::tclmpi::allreduce {} $double tclmpi::minloc $comm] \
    {::tclmpi::allreduce: invalid mpi op}
run_error [list ::tclmpi::allreduce {1 0 2 1 4 3} $intint \
               tclmpi::max $comm] \
    {::tclmpi::allreduce: invalid mpi op}
run_error  [list ::tclmpi::allreduce {} tclmpi::real tclmpi::min $comm] \
    {{::tclmpi::allreduce: invalid data type: tclmpi::real}}
run_error  [list ::tclmpi::allreduce {} $int tclmpi::land $null]          \
    {::tclmpi::allreduce: invalid communicator}
run_error  [list ::tclmpi::allreduce {{}} $int tclmpi::gamma $comm]       \
    {{::tclmpi::allreduce: unknown reduction operator: tclmpi::gamma}}
run_return [list ::tclmpi::allreduce {2 0 1 1} $intint \
                tclmpi::maxloc $comm] {{2 0 1 1}}
#run_return [list ::tclmpi::allreduce {1.0 0 2.0 1} $dblint \
    ::tclmpi::minloc $comm] {{1.0 0 2.0 1}}

# check some data type conversions
run_return [list ::tclmpi::allreduce {{xx 11} {1 2 3} 2.0 7 0xff yy} \
                $int tclmpi::max $self] {{0 0 0 7 255 0}}
run_return [list ::tclmpi::allreduce {{xx 11} {1 2 3} 2.5 yy 1}      \
                $double tclmpi::sum $comm] {{0.0 0.0 2.5 0.0 1.0}}
run_return [list ::tclmpi::allreduce {-1 2 +3 2.0 7 016}             \
                $int tclmpi::prod $comm] {{-1 2 3 0 7 14}}

# reduce
set numargs \
    "wrong # args: should be \"::tclmpi::reduce <data> <type> <op> <root> <comm>\""
run_error  [list ::tclmpi::reduce] [list $numargs]
run_error  [list ::tclmpi::reduce {}] [list $numargs]
run_error  [list ::tclmpi::reduce {} $auto] [list $numargs]
run_error  [list ::tclmpi::reduce {} $auto tclmpi::sum] [list $numargs]
run_error  [list ::tclmpi::reduce {} $auto tclmpi::prod 0] [list $numargs]
run_error  [list ::tclmpi::reduce {} $auto tclmpi::prod 0 $comm xxx] \
    [list $numargs]
run_error  [list ::tclmpi::reduce {} $auto tclmpi::max 0 $comm]      \
    {{::tclmpi::reduce: does not support data type tclmpi::auto}}
run_error  [list ::tclmpi::reduce {} $int tclmpi::max 0 comm0]       \
    {{::tclmpi::reduce: unknown communicator: comm0}}
run_error  [list ::tclmpi::reduce {} $int tclmpi::maxloc 0 $comm]    \
    {::tclmpi::reduce: invalid mpi op}
run_error  [list ::tclmpi::reduce {} $double tclmpi::minloc 0 $comm] \
    {::tclmpi::reduce: invalid mpi op}
run_error [list ::tclmpi::reduce {1 0 2 1 4 3} $intint \
               tclmpi::max 0 $comm] \
    {::tclmpi::reduce: invalid mpi op}
run_error  [list ::tclmpi::reduce {} tclmpi::real tclmpi::min 0 $comm] \
    {{::tclmpi::reduce: invalid data type: tclmpi::real}}
run_error  [list ::tclmpi::reduce {} $int tclmpi::land 0 $null]          \
    {::tclmpi::reduce: invalid communicator}
run_error  [list ::tclmpi::reduce {{}} $int tclmpi::gamma 0 $comm]       \
    {{::tclmpi::reduce: unknown reduction operator: tclmpi::gamma}}
run_return [list ::tclmpi::reduce {2 0 1 1} $intint \
                tclmpi::maxloc 0 $comm] {{2 0 1 1}}
#run_return [list ::tclmpi::reduce {1.0 0 2.0 1} $dblint \
    tclmpi::minloc $comm] {{1.0 0 2.0 1}}

# probe
set numargs \
    "wrong # args: should be \"::tclmpi::probe <source> <tag> <comm> ?status?\""
run_error  [list ::tclmpi::probe] [list $numargs]
run_error  [list ::tclmpi::probe 0] [list $numargs]
run_error  [list ::tclmpi::probe 0 0] [list $numargs]
run_error  [list ::tclmpi::probe 0 0 $comm status xxx] [list $numargs]
run_error  [list ::tclmpi::probe 0 0 comm0]                    \
    {{::tclmpi::probe: unknown communicator: comm0}}
run_error  [list ::tclmpi::probe tclmpi::any_tag 0 $comm]    \
    {{expected integer but got "tclmpi::any_tag"}}
run_error  [list ::tclmpi::probe 0 tclmpi::any_source $comm] \
    {{expected integer but got "tclmpi::any_source"}}
run_error  [list ::tclmpi::probe tclmpi::any_source \
                tclmpi::any_tag $null] \
    {{::tclmpi::probe: invalid communicator: tclmpi::comm_null}}

# abort (non-destructive tests only)
set numargs "wrong # args: should be \"::tclmpi::abort <comm> <errorcode>\""
run_error  [list ::tclmpi::abort] [list $numargs]
run_error  [list ::tclmpi::abort $comm] [list $numargs]
run_error  [list ::tclmpi::abort $comm 1 2] [list $numargs]
run_error  [list ::tclmpi::abort comm0 1] \
    {{::tclmpi::abort: unknown communicator: comm0}}
run_error  [list ::tclmpi::abort $comm comm0] \
    {{expected integer but got "comm0"}}

# special case. this has to be at the end
run_error  [list ::tclmpi::comm_free $null]  \
    {::tclmpi::comm_free: mpi invalid communicator}

# finalize
run_error  [list ::tclmpi::finalize 0] \
    [list "wrong # args: should be \"::tclmpi::finalize\""]
run_return [list ::tclmpi::finalize] {}
run_error  [list ::tclmpi::finalize] \
    {{calling ::tclmpi::finalize twice is erroneous.}}

# print results and exit
test_summary 01

exit 0
