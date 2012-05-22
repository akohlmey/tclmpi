## \file
# This is the ::tclmpi:: namespace and the documentation of the 
# Tcl API of TclMPI.

## TclMPI wrapper
namespace eval tclmpi {

    variable auto   tclmpi::auto   ;#< constant for automatic data type
    variable int    tclmpi::int    ;#< constant for integer data type
    variable intint tclmpi::intint ;#< constant for integer pair data type
    variable double tclmpi::double ;#< constant for double data type
    variable dblint tclmpi::dblint ;#< constant for double/int pair data type

    variable comm_world tclmpi::comm_world ;#< constant for world communicator
    variable comm_self  tclmpi::comm_self  ;#< constant for self communicator
    variable comm_null  tclmpi::comm_null  ;#< constant empty communicator

    variable any_source tclmpi::any_source ;#< constant to accept messages from any source rank
    variable any_tag    tclmpi::any_tag    ;#< constant to accept messages with any tag

    variable undefined  tclmpi::undefined  ;#< constant to indicate an undefined number

    variable version "0.6"  ;#< version number of this package

    # NOTE: the following section is to trick doxygen into documenting
    # the Tcl API of TclMPI. The since the actual functions are provided
    # as compiled C code in _tclmpi.c, we immediately delete them again.

    ## Initialize the MPI environment from Tcl
    #
    # This command initializes the MPI environment. Needs to be called
    # before any other TclMPI commands. MPI can be initialized at most
    # once, so calling ::tclmpi::init multiple times is an error.
    # Like in the C bindings for MPI, ::tclmpi::init will scan the argument
    # vector, the global variable $argv, for any MPI implementation specific
    # flags and will remove them. The global variable $argc will be adjusted
    # accordingly. This command takes no arguments and has no return value.
    #
    # For implementation details see TclMPI_Init().
    proc init {} {}
    rename init ""

    ## Shut down the MPI environment from Tcl
    #
    # This command closes the MPI environment and cleans up all MPI
    # states. All processes much call this routine before exiting.
    # Calling this function before calling ::tclmpi::init is an error.
    # After calling this function, no more TclMPI commands including
    # ::tclmpi::finalize and ::tclmpi::init may be used. This command
    # takes no arguments and has no return value.
    #
    # For implementation details see TclMPI_Finalize().
    proc finalize {} {}
    rename finalize ""

    ## Terminates the MPI environment from Tcl
    # \param comm Tcl representation of an MPI communicator
    # \param errorcode an integer that will be returned as exit code to the OS
    #
    # This command makes a best attempt to abort all tasks sharing the
    # communicator and exit with the provided error code. Only one task
    # needs to call ::tclmpi::abort. This command terminates the
    # program, so there can be no return value.
    #
    # For implementation details see TclMPI_Abort().
    proc abort {comm errorcode} {}
    rename abort ""

    ## Returns the number of processes involved in an MPI communicator
    # \param comm Tcl representation of an MPI communicator
    # \return number of MPI processes on communicator
    #
    # This function indicates the number of processes involved in a
    # communicator. For tclmpi::comm_world, it indicates the total
    # number of processes available. This call is often used in
    # combination with ::tclmpi::comm_rank to determine the amount of
    # concurrency available for a specific library or
    # program. ::tclmpi::comm_rank indicates the rank of the process that
    # calls it in the range from 0...size-1, where size is the return
    # value of tclmpi::comm_size.
    #
    # For implementation details see TclMPI_Comm_size().
    proc comm_size {comm} {}
    rename comm_size ""

    ## Returns the rank of the current process in an MPI communicator
    # \param comm Tcl representation of an MPI communicator
    # \return rank on the communicator (integer between 0 and size-1)
    #
    # This function gives the rank of the process in the particular
    # communicator. Many programs will be written with a manager-worker
    # model, where one process (such as the rank-zero process) will play
    # a supervisory role, and the other processes will serve as compute
    # nodes. In this framework, ::tclmpi::comm_size and ::tclmpi::comm_rank
    # are useful for determining the roles of the various processes of
    # a communicator.
    #
    # For implementation details see TclMPI_Comm_rank().
    proc comm_rank {comm} {}
    rename comm_rank ""

    ## Creates new communicators based on "color" and "key" flags
    # \param comm Tcl representation of an MPI communicator
    # \param color subset assignment (non-negative integer or tclmpi::undefined)
    # \param key relative rank assignment (integer)
    # \return Tcl representation of the newly created MPI communicator
    #
    # This function partitions the group associated with comm into
    # disjoint subgroups, one for each value of color. Each subgroup
    # contains all processes of the same color. Within each subgroup,
    # the processes are ranked in the order defined by the value of the
    # argument key, with ties broken according to their rank in the
    # old group. A new communicator is created for each subgroup and
    # returned in newcomm. A process may supply the color value
    # tclmpi::undefined, in which case the function returns tclmpi::comm_null.
    # This is a collective call, but each process is permitted to provide
    # different values for color and key.
    #
    # The following example shows how to construct a communicator where
    # the ranks are reversed in comparison to the world communicator.
    # \code
    # set comm tclmpi::comm_world
    # set size [::tclmpi::comm_size $comm]
    # set key -[::tclmpi::comm_rank $comm]
    # set revcomm [::tclmpi::comm_split $comm 1 $key]
    # \endcode
    #
    # For implementation details see TclMPI_Comm_split().
    proc comm_split {comm color key} {}
    rename comm_split ""

    ## Synchronize MPI processes
    # \param comm Tcl representation of an MPI communicator
    # 
    # Blocks the caller until all processes sharing the communicator
    # have called it; the call returns at any process only after \b all
    # processes have entered the call and thus effectively synchronizes
    # the processes. This function has no return value.
    #
    # For implementation details see TclMPI_Barrier().
    proc barrier {comm} {}
    rename barrier ""

    ## Broadcasts data from one process to all other processes on the communicator
    # \param data data to be broadcast (Tcl data object)
    # \param type data type to be used (string constant)
    # \param root rank of process that is providing the data (integer)
    # \param comm Tcl representation of an MPI communicator
    # \return data that was broadcast
    #
    # This command broadcasts the provided data object (list or single
    # number or string) from the process with rank root on the
    # communicator comm to all processes sharing the communicator. The
    # data argument has to be present on all processes but will be
    # ignored on all but the root process. The data resulting from the
    # broadcast will be stored in the return value of the command on \b
    # all processes. This is important when the data type is not
    # tclmpi::auto, since using other data types may incur an
    # irreversible conversion of the data elements.
    # This function call is an implicit synchronization.
    #
    # For implementation details see TclMPI_Bcast().
    proc bcast {data type root comm} {}
    rename bcast ""

    ## Combines data from all processes and distributes the result back to them
    # \param data data to be reduced (Tcl data object)
    # \param type data type to be used (string constant)
    # \param op reduction operation (string constant)
    # \param comm Tcl representation of an MPI communicator
    # \return data resulting from the reduction operation
    #
    # This command performs a global reduction operation op on the
    # provided data object across all processes participating in the
    # communicator comm. If data is a list, then the reduction will be
    # done across each respective entry of the same list index. The
    # result is distributed to all processes and used as return value of
    # the command. This command only supports the data types
    # tclmpi::int and tclmpi::double and tclmpi::intint for operations
    # tclmpi::maxloc and tclmpi::minloc. The following reduction
    # operations are supported: tclmpi::max (maximum), tclmpi::min
    # (minimum), tclmpi::sum (sum), tclmpi::prod (product),
    # tclmpi::land (logical and), tclmpi::band (bitwise and),
    # tclmpi::lor (logical or), tclmpi::bor (bitwise or),
    # tclmpi::lxor (logical exclusive or), tclmpi::bxor (bitwise
    # exclusive or), tclmpi::maxloc (max value and location),
    # tclmpi::minloc (min value and location).
    # This function call is an implicity synchronization.
    #
    # For implementation details see TclMPI_Allreduce().
    proc allreduce {data type op comm} {}
    rename allreduce ""

    ## Perform a blocking send
    # \param data data to be sent (Tcl data object)
    # \param type data type to be used (string constant)
    # \param dest rank of destination process (non-negative integer)
    # \param tag message identification tag (integer)
    # \param comm Tcl representation of an MPI communicator
    #
    # This function performs a regular \b blocking send to process rank
    # dest on communicator comm. The choice of data type determines how
    # data is being sent and thus unlike in the C-bindings the
    # corresponding receive has to use the same data data type.
    # As a blocking call, the function will only return when all data is sent.
    # This function has no return value.
    #
    # For implementation details see TclMPI_Send().
    proc send {data type dest tag comm} {}
    rename send ""

    ## Perform a non-blocking send
    # \param data data to be sent (Tcl data object)
    # \param type data type to be used (string constant)
    # \param dest rank of destination process (non-negative integer)
    # \param tag message identification tag (integer)
    # \param comm Tcl representation of an MPI communicator
    # \return Tcl representation of generated MPI request
    #
    # This function performs a regular \b non-blocking send to process rank
    # dest on communicator comm. The choice of data type determines how
    # data is being sent and thus unlike in the C-bindings the
    # corresponding receive has to use the same data data type.
    # As a non-blocking call, the function will return immediately.
    # The return value is a string representing the generated MPI request
    # and it can be passed to a call to ::tclmpi::wait in order to wait
    # for its completion and release all reserved storage associated with
    # the request.
    #
    # For implementation details see TclMPI_Isend().
    proc isend {data type dest tag comm} {}
    rename isend ""

    ## Perform a blocking receive
    # \param type data type to be used (string constant)
    # \param source rank of sending process or tclmpi::any_source
    # \param tag message identification tag or tclmpi::any_tag
    # \param comm Tcl representation of an MPI communicator
    # \param status variable name for status array (string)
    # \return the received data
    #
    # This procedure provides a blocking receive operation, i.e. it only
    # returns \b after the message is received in full. The received data
    # will be passed as return value. The type argument has to match
    # that of the corresponding send command. Instead of using a specific
    # source rank, the constant tclmpi::any_source can be used and
    # similarly tclmpi::any_tag as tag. This way the receive operation
    # will not select a message based on source rank or tag, respectively.
    # The (optional) status argument would be the name of a variable in
    # which status information about the receive will be stored in the
    # form of an array. The associative array has the entries MPI_SOURCE
    # (rank of sender), MPI_TAG (tag of message), COUNT_CHAR (size of 
    # message in bytes), COUNT_INT (size of message in tclmpi::int units),
    # COUNT_DOUBLE (size of message in tclmpi::double units).
    #
    # For implementation details see TclMPI_Recv().
    proc recv {type source tag comm {status {}}} {}
    rename recv ""

    ## Initiate a non-blocking receive
    # \param type data type to be used (string constant)
    # \param source rank of sending process or tclmpi::any_source
    # \param tag message identification tag or tclmpi::any_tag
    # \param comm Tcl representation of an MPI communicator
    # \return Tcl representation of generated MPI request
    #
    # This procedure provides a non-blocking receive operation, i.e. it 
    # returns \b immediately. The call does not return any data but a
    # request handle of the form tclmpi::req#, with # being a unique
    # integer number. This request handle is best stored in a variable
    # and needs to be passed to a ::tclmpi::wait call to wait for completion
    # of the receive and pass the data to the calling code as return value
    # of the wait call. The type argument has to match that of the
    # corresponding send command. Instead of a specific source rank, the
    # constant tclmpi::any_source can be used and similarly
    # tclmpi::any_tag as tag, to not select on source rank or tag,
    # respectively.
    #
    # For implementation details see TclMPI_Irecv().
    proc irecv {type source tag comm} {}
    rename irecv ""

    ## Blocking test for a message 
    # \param source rank of sending process or tclmpi::any_source
    # \param tag message identification tag or tclmpi::any_tag
    # \param comm Tcl representation of an MPI communicator
    # \param status variable name for status array (string)
    # \return empty
    #
    # This function allows to check for an incoming message on the
    # communicator comm without actually receiving it. Nevertheless,
    # this call is blocking, i.e. it will not return unless there is
    # actually a message pending that matches the requirements of source
    # rank and message tag. Instead of a specific source rank, the
    # constant tclmpi::any_source can be used and similarly tclmpi::any_tag
    # as tag, to accept send requests from any rank or tag, respectively.
    # The (optional) status argument would be the name of a variable in
    # which status information about the message will be stored in the
    # form of an array. This associative array has the entries MPI_SOURCE
    # (rank of sender), MPI_TAG (tag of message), COUNT_CHAR (size of 
    # message in bytes), COUNT_INT (size of message in tclmpi::int units),
    # COUNT_DOUBLE (size of message in tclmpi::double units).
    #
    # For implementation details see TclMPI_Probe().
    proc probe {source tag comm {status {}}} {}
    rename probe ""

    ## Non-blocking test for a message 
    # \param source rank of sending process or tclmpi::any_source
    # \param tag message identification tag or tclmpi::any_tag
    # \param comm Tcl representation of an MPI communicator
    # \param status variable name for status array (string)
    # \return 1 or 0 depending on whether a pending request was detected
    #
    # This function allows to check for an incoming message on the
    # communicator comm without actually receiving it. Unlike
    # ::tclmpi::probe, this call is non-blocking, i.e. it will return
    # immediately and report whether there is a message pending or not
    # in its return value (1 or 0, respectively). Instead of a specific
    # source rank, the constant tclmpi::any_source can be used and
    # similarly tclmpi::any_tag as tag, to test for send requests from
    # any rank or tag, respectively.  The (optional) status argument
    # would be the name of a variable in which status information about
    # the message will be stored in the form of an array. This
    # associative array has the entries MPI_SOURCE (rank of sender),
    # MPI_TAG (tag of message), COUNT_CHAR (size of message in bytes),
    # COUNT_INT (size of message in tclmpi::int units), COUNT_DOUBLE
    # (size of message in tclmpi::double units).
    #
    # For implementation details see TclMPI_Iprobe().
    proc iprobe {source tag comm {status {}}} {}
    rename iprobe ""

    ## Wait for MPI request completion
    # \param request Tcl representation of an MPI request
    # \param status variable name for status array (string)
    # \return empty or received data that was associated with the request
    #
    # This function takes a communication request created by a
    # non-blocking send or receive call (tclmpi::isend or tclmpi::irecv)
    # and waits for its completion. In case of a send, it will merely
    # wait until the matching communication is completed and any
    # resources associated with the request will be releaseed. If the
    # request was generated by a non-blocking receive call, tclmpi::wait
    # will hand the received data to the calling routine in its return
    # value.  The (optional) status argument would be the name of a
    # variable in which the resulting status information will be stored
    # in the form of an associative array. The associative array will
    # have the entries MPI_SOURCE (rank of sender), MPI_TAG (tag of
    # message), COUNT_CHAR (size of message in bytes), COUNT_INT (size
    # of message in tclmpi::int units), COUNT_DOUBLE (size of message in
    # tclmpi::double units).
    #
    # For implementation details see TclMPI_Wait().
    proc wait {request {status {}}} {}
    rename wait ""
}

package provide tclmpi $tclmpi::version

# load the ancilliary methods from the DSO
package require _tclmpi $::tclmpi::version

