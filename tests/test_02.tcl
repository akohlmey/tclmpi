#!/usr/bin/tclsh
# tests that can be run with just one process
source harness.tcl

ser_init

# import all API from namespace
namespace import tclmpi::*
# use aliases instead of predefined variables
unset comm self null

# version 8.4 compatibility
if {$tcl_version < 8.5} {
    set comm $tclmpi::comm_world
    set self $tclmpi::comm_self
    set null $tclmpi::comm_null
    set mpi_sum $tclmpi::sum
    set mpi_prod $tclmpi::prod
    set mpi_max $tclmpi::max
    set mpi_min $tclmpi::min
    set mpi_land $tclmpi::land
    set mpi_auto $tclmpi::auto
    set mpi_double $tclmpi::double
    set mpi_int $tclmpi::int
    set minloc $tclmpi::minloc
    set maxloc $tclmpi::maxloc
    set undefined $tclmpi::undefined
} else {
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
}

# init
run_error  [list init 0] \
    [list "wrong # args: should be \"init\""]
run_return [list init] {}
run_error  [list init] \
    {{calling init multiple times is erroneous.}}

# comm_size
set numargs "wrong # args: should be \"comm_size <comm>\""
run_error  [list comm_size] [list $numargs]
run_error  [list comm_size $comm $self] [list $numargs]
run_error  [list comm_size comm0] \
    {{comm_size: unknown communicator: comm0}}
run_return [list comm_size $comm] 1
run_return [list comm_size $self] 1
run_error  [list comm_size $null] \
    {comm_size: invalid communicator}

# comm_rank
set numargs "wrong # args: should be \"comm_rank <comm>\""
run_error  [list comm_rank] [list $numargs]
run_error  [list comm_rank $comm $self] [list $numargs]
run_error  [list comm_rank comm0] \
    {comm_rank: unknown communicator: comm0}
run_return [list comm_rank $comm] 0
run_return [list comm_rank $self] 0
run_error  [list comm_rank $null] \
    {comm_rank: invalid communicator}

# comm_split
set split0 tclmpi::comm0
set split1 tclmpi::comm1
set split2 tclmpi::comm2
set numargs \
    "wrong # args: should be \"comm_split <comm> <color> <key>\""
run_error  [list comm_split] [list $numargs]
run_error  [list comm_split $comm 1] [list $numargs]
run_error  [list comm_split $comm 1 1 1] [list $numargs]
run_error  [list comm_split comm0 0 0]  \
    {{comm_split: unknown communicator: comm0}}
run_return [list comm_split $comm 5 -1] [list $split0]
run_return [list comm_split $comm 0 0]  [list $split1]
run_return [list comm_split $self 4 -1] [list $split2]
run_return [list comm_split $self $undefined -1] [list $null]
run_error  [list comm_split $comm -5 0] \
    {{comm_split: invalid color argument}}
run_error  [list comm_split $null 5 0]  \
    {comm_split: invalid communicator}
run_error  [list comm_split $comm x 0]  \
    {{expected integer but got "x"}}
run_error  [list comm_split $comm 0 x]  \
    {{expected integer but got "x"}}

# check size and rank on generated communicators
run_return [list comm_size $split0] 1
run_return [list comm_size $split1] 1
run_return [list comm_rank $split0] 0
run_return [list comm_rank $split1] 0

# comm_free
set numargs \
    "wrong # args: should be \"comm_free <comm>\""
run_error  [list comm_free] [list $numargs]
run_error  [list comm_free $comm 1] [list $numargs]
run_error  [list comm_free comm0]  \
    {{comm_free: unknown communicator: comm0}}
run_return [list comm_free $split2] {}

# barrier
set numargs "wrong # args: should be \"barrier <comm>\""
run_error  [list barrier] [list $numargs]
run_error  [list barrier $comm 1] [list $numargs]
run_error  [list barrier comm0]   \
    {{barrier: unknown communicator: comm0}}
run_error  [list barrier $null]   \
    {barrier: invalid communicator}
run_return [list barrier $comm] {}
run_return [list barrier $self] {}

# bcast
set numargs \
    "wrong # args: should be \"bcast <data> <type> <root> <comm>\""
run_error  [list bcast] [list $numargs]
run_error  [list bcast {}] [list $numargs]
run_error  [list bcast {} $auto] [list $numargs]
run_error  [list bcast {} $auto $master] [list $numargs]
run_error  [list bcast {} $auto $master $comm xxx] [list $numargs]
run_error  [list bcast {} $auto $master comm0] \
    {{bcast: unknown communicator: comm0}}
run_error  [list bcast {} $auto $master $null] \
    {bcast: invalid communicator}
run_error  [list bcast {{xx 11} {1 2 3} {}} $auto 1 $comm] \
    {bcast: invalid root}

# check data type conversions
run_return [list bcast {{xx 11} {1 2 3} {}}            \
                $auto $master $comm] {{{xx 11} {1 2 3} {}}}
run_return [list bcast {{xx 11} {1 2 3} {}}            \
                $auto $master $self] {{{xx 11} {1 2 3} {}}}
run_return [list bcast {{xx 11} {1 2 3} 2.0 7 0xff yy} \
                $int $master $self] {{0 0 0 7 255 0}}
run_return [list bcast {{xx 11} {1 2 3} 2.5 yy 1}      \
                $double $master $self] {{0.0 0.0 2.5 0.0 1.0}}
run_return [list bcast {-1 2 +3 2.0 7 016}             \
                $int $master $comm] {{-1 2 3 0 7 14}}
run_return [list bcast {-1e5 1.1 1.2d0 0.2e-1 0.06E+28 0x22} \
                $double $master $self] {{-100000.0 1.1 0.0 0.02 6e+26 34.0}}


# scatter
set numargs \
    "wrong # args: should be \"scatter <data> <type> <root> <comm>\""
run_error  [list scatter] [list $numargs]
run_error  [list scatter {}] [list $numargs]
run_error  [list scatter {} $auto] [list $numargs]
run_error  [list scatter {} $auto $master] [list $numargs]
run_error  [list scatter {} $auto $master $comm xxx] [list $numargs]
run_error  [list scatter {} $auto $master comm0] \
    {{scatter: unknown communicator: comm0}}
run_error  [list scatter {} $auto $master $null] \
    {{scatter: does not support data type tclmpi::auto}}
run_error  [list scatter {{xx 11} {1 2 3} {}} $int 1 $comm] \
    {scatter: invalid root}
run_error  [list scatter {} tclmpi::real $master $comm]    \
    {{scatter: invalid data type: tclmpi::real}}

# check data type conversions
run_return [list scatter {{xx 11} {1 2 3} 2.0 7 0xff yy} \
                $int $master $self] {{0 0 0 7 255 0}}
run_return [list scatter {{xx 11} {1 2 3} 2.5 yy 1}      \
                $double $master $comm] {{0.0 0.0 2.5 0.0 1.0}}
run_return [list scatter {-1 2 +3 2.0 7 016}             \
                $int $master $comm] {{-1 2 3 0 7 14}}
run_return [list scatter {-1e5 1.1 1.2d0 0.2e-1 0.06E+28 0x22} \
                $double $master $self] {{-100000.0 1.1 0.0 0.02 6e+26 34.0}}

# allgather
set numargs \
    "wrong # args: should be \"allgather <data> <type> <comm>\""
run_error  [list allgather] [list $numargs]
run_error  [list allgather {}] [list $numargs]
run_error  [list allgather {} $auto] [list $numargs]
run_error  [list allgather {} $auto $comm xxx] [list $numargs]
run_error  [list allgather {} $auto comm0] \
    {{allgather: unknown communicator: comm0}}
run_error  [list allgather {} $auto $null] \
    {{allgather: does not support data type tclmpi::auto}}
run_error  [list allgather {} tclmpi::real $comm]    \
    {{allgather: invalid data type: tclmpi::real}}

# check data type conversions
run_return [list allgather {{xx 11} {1 2 3} 2.0 7 0xff yy} \
                $int $self] {{0 0 0 7 255 0}}
run_return [list allgather {{xx 11} {1 2 3} 2.5 yy 1}      \
                $double $comm] {{0.0 0.0 2.5 0.0 1.0}}
run_return [list allgather {-1 2 +3 2.0 7 016}             \
                $int $comm] {{-1 2 3 0 7 14}}
run_return [list allgather {-1e5 1.1 1.2d0 0.2e-1 0.06E+28 0x22} \
                $double $self] {{-100000.0 1.1 0.0 0.02 6e+26 34.0}}

# gather
set numargs \
    "wrong # args: should be \"gather <data> <type> <root> <comm>\""
run_error  [list gather] [list $numargs]
run_error  [list gather {}] [list $numargs]
run_error  [list gather {} $auto] [list $numargs]
run_error  [list gather {} $auto $master] [list $numargs]
run_error  [list gather {} $auto $master $comm xxx] [list $numargs]
run_error  [list gather {} $auto $master comm0] \
    {{gather: unknown communicator: comm0}}
run_error  [list gather {} $auto $master $null] \
    {{gather: does not support data type tclmpi::auto}}
run_error  [list gather {{xx 11} {1 2 3} {}} $int 1 $comm] \
    {gather: invalid root}
run_error  [list gather {} tclmpi::real $master $comm]    \
    {{gather: invalid data type: tclmpi::real}}

# check data type conversions
run_return [list gather {{xx 11} {1 2 3} 2.0 7 0xff yy} \
                $int $master $self] {{0 0 0 7 255 0}}
run_return [list gather {{xx 11} {1 2 3} 2.5 yy 1}      \
                $double $master $comm] {{0.0 0.0 2.5 0.0 1.0}}
run_return [list gather {-1 2 +3 2.0 7 016}             \
                $int $master $comm] {{-1 2 3 0 7 14}}
run_return [list gather {-1e5 1.1 1.2d0 0.2e-1 0.06E+28 0x22} \
                $double $master $self] {{-100000.0 1.1 0.0 0.02 6e+26 34.0}}

# allreduce
set numargs \
    "wrong # args: should be \"allreduce <data> <type> <op> <comm>\""
run_error  [list allreduce] [list $numargs]
run_error  [list allreduce {}] [list $numargs]
run_error  [list allreduce {} $auto] [list $numargs]
run_error  [list allreduce {} $auto $mpi_sum] [list $numargs]
run_error  [list allreduce {} $auto $mpi_prod $comm xxx] [list $numargs]
run_error  [list allreduce {} $auto $mpi_max $comm]      \
    {{allreduce: does not support data type tclmpi::auto}}
run_error  [list allreduce {} $int $mpi_max comm0]       \
    {{allreduce: unknown communicator: comm0}}
run_error  [list allreduce {} $int $maxloc $comm]    \
    {allreduce: invalid mpi op}
run_error  [list allreduce {} $double $minloc $comm] \
    {allreduce: invalid mpi op}
run_error [list allreduce {{1 0} {2 1} {4 3}} $intint \
               tclmpi::max $comm] \
    {allreduce: invalid mpi op}
run_error [list allreduce {{1.0 0} {2.0 -1} {4.5 3}} $dblint \
               tclmpi::min $comm] \
    {allreduce: invalid mpi op}
run_error  [list allreduce {} tclmpi::real $mpi_min $comm] \
    {{allreduce: invalid data type: tclmpi::real}}
run_error  [list allreduce {} $int $mpi_land $null]          \
    {allreduce: invalid communicator}
run_error  [list allreduce {{}} $int tclmpi::gamma $comm]       \
    {{allreduce: unknown reduction operator: tclmpi::gamma}}
run_error [list allreduce {2 0 1 1} $intint $maxloc $comm] \
    {{allreduce: bad list format for loc reduction: tclmpi::maxloc}}
run_return [list allreduce {{2 0} {1 -1}} $intint $maxloc $comm] \
    {{{2 0} {1 -1}}}
run_return [list allreduce {{2 1 0} {1 1 0 0}} $intint $maxloc $comm] \
    {{{2 1} {1 1}}}
run_error [list allreduce {2.0 0 1.0 1} $dblint \
                tclmpi::maxloc $comm] \
    {{allreduce: bad list format for loc reduction: tclmpi::maxloc}}
run_return [list allreduce {{2.1 0} {-1 -1}} $dblint \
                tclmpi::maxloc $comm] {{{2.1 0} {-1.0 -1}}}
run_error  [list allreduce {{2 1.1} {-1 -1}} $dblint \
                tclmpi::maxloc $comm] \
    {{allreduce: bad location data for reduction: tclmpi::maxloc}}
run_return [list allreduce {{2 1 0} {1.0 1 0 0}} $dblint \
                tclmpi::minloc $comm] {{{2.0 1} {1.0 1}}}

# check some data type conversions
run_return [list allreduce {{xx 11} {1 2 3} 2.0 7 0xff yy} \
                $int tclmpi::max $self] {{0 0 0 7 255 0}}
run_return [list allreduce {{xx 11} {1 2 3} 2.5 yy 1}      \
                $double tclmpi::sum $comm] {{0.0 0.0 2.5 0.0 1.0}}
run_return [list allreduce {-1 2 +3 2.0 7 016}             \
                $int tclmpi::prod $comm] {{-1 2 3 0 7 14}}

# reduce
set numargs \
    "wrong # args: should be \"reduce <data> <type> <op> <root> <comm>\""
run_error  [list reduce] [list $numargs]
run_error  [list reduce {}] [list $numargs]
run_error  [list reduce {} $auto] [list $numargs]
run_error  [list reduce {} $auto $mpi_sum] [list $numargs]
run_error  [list reduce {} $auto $mpi_prod 0] [list $numargs]
run_error  [list reduce {} $auto $mpi_prod 0 $comm xxx] [list $numargs]
run_error  [list reduce {} $auto $mpi_max 0 $comm]      \
    {{reduce: does not support data type tclmpi::auto}}
run_error  [list reduce {} $int $mpi_max 0 comm0]       \
    {{reduce: unknown communicator: comm0}}
run_error  [list reduce {} $int $maxloc 0 $comm]    \
    {reduce: invalid mpi op}
run_error  [list reduce {} $double $minloc 0 $comm] \
    {reduce: invalid mpi op}
run_error [list reduce {{1 0} {2 1} {4 3}} $intint \
               tclmpi::max 0 $comm] \
    {reduce: invalid mpi op}
run_error [list reduce {{1.0 0} {2.0 -1} {4.5 3}} $dblint \
               $mpi_min 0 $comm] {reduce: invalid mpi op}
run_error  [list reduce {} tclmpi::real $mpi_min 0 $comm] \
    {{reduce: invalid data type: tclmpi::real}}
run_error  [list reduce {} $int $mpi_land 0 $null]          \
    {reduce: invalid communicator}
run_error  [list reduce {{}} $int tclmpi::gamma 0 $comm]       \
    {{reduce: unknown reduction operator: tclmpi::gamma}}
run_error [list reduce {2 0 1 1} $intint \
                $maxloc 0 $comm] \
    {{reduce: bad list format for loc reduction: tclmpi::maxloc}}
run_return [list reduce {{2 0} {1 -1}} $intint \
                $maxloc 0 $comm] {{{2 0} {1 -1}}}
run_return [list reduce {{2 1 0} {1 1 0 0}} $intint \
                $minloc 0 $comm] {{{2 1} {1 1}}}
run_error [list reduce {2.0 0 1.0 1} $dblint \
                $maxloc 0 $comm] \
    {{reduce: bad list format for loc reduction: tclmpi::maxloc}}
run_return [list reduce {{2.1 0} {-1 -1}} $dblint \
                $maxloc 0 $comm] {{{2.1 0} {-1.0 -1}}}
run_error  [list reduce {{2 1.1} {-1 -1}} $dblint \
                $maxloc 0 $comm] \
    {{reduce: bad location data for reduction: tclmpi::maxloc}}
run_return [list reduce {{2 1 0} {1.0 1 0 0}} $dblint \
                $minloc 0 $comm] {{{2.0 1} {1.0 1}}}

# probe
set numargs \
    "wrong # args: should be \"probe <source> <tag> <comm> ?status?\""
run_error  [list probe] [list $numargs]
run_error  [list probe 0] [list $numargs]
run_error  [list probe 0 0] [list $numargs]
run_error  [list probe 0 0 $comm status xxx] [list $numargs]
run_error  [list probe 0 0 comm0]                    \
    {{probe: unknown communicator: comm0}}
run_error  [list probe tclmpi::any_tag 0 $comm]    \
    {{expected integer but got "tclmpi::any_tag"}}
run_error  [list probe 0 tclmpi::any_source $comm] \
    {{expected integer but got "tclmpi::any_source"}}
run_error  [list probe tclmpi::any_source \
                tclmpi::any_tag $null] \
    {{probe: invalid communicator: tclmpi::comm_null}}

# abort (non-destructive tests only)
set numargs "wrong # args: should be \"abort <comm> <errorcode>\""
run_error  [list abort] [list $numargs]
run_error  [list abort $comm] [list $numargs]
run_error  [list abort $comm 1 2] [list $numargs]
run_error  [list abort comm0 1] \
    {{abort: unknown communicator: comm0}}
run_error  [list abort $comm comm0] \
    {{expected integer but got "comm0"}}

# special case. this has to be at the end
run_error  [list comm_free $null]  \
    {comm_free: mpi invalid communicator}

# finalize
run_error  [list finalize 0] \
    [list "wrong # args: should be \"finalize\""]
run_return [list finalize] {}
run_error  [list finalize] \
    {{calling finalize twice is erroneous.}}

# print results and exit
test_summary 02

exit 0
