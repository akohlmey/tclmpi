/*! \file */

/***************************************************************************

Tcl Interface to MPI

Copyright (c) 2012 Axel Kohlmeyer <akohlmey@gmail.com>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
 - Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
 - Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
 - Neither the name of the author of this software nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER>
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
THE POSSIBILITY OF SUCH DAMAGE.

 ***************************************************************************/

#include <mpi.h>
#include <tcl.h>

#include <string.h>
#include <stdio.h>

/*! \page userguide TclMPI User's Guide 
 *
 * This page describes Tcl bindings for MPI. This package provides a
 * shared object that can be loaded into a Tcl interpreter to provide
 * additional commands that act as an interface to an underlying MPI
 * implementation. This allows to run Tcl scripts in parallel via
 * mpirun or mpiexec similar to C, C++ or Fortran programs and
 * communicate via wrappers to MPI function call.
 *
 * The original motivation for writing this package was to complement a
 * Tcl wrapper for the LAMMPS molecular dynamics simulation software,
 * but also allow using the VMD molecular visualization and analysis
 * package in parallel without having to recompile VMD and using a
 * convenient API to people that already know how to program parallel
 * programs with MPI in C, C++ or Fortran.
 *
 * \section compile Compilation and Installation
 *
 * The package currently consist of a single C source file which usually
 * will be compiled for dynamic linkage, but can also be compiled into a
 * new Tcl interpreter with TclMPI included (required on platforms like
 * Cray XT, XE, or XK machines that need static linkage) and a Tcl
 * script file. The corresponding commands for Linux and MacOSX systems
 * are included in the provided makefile. All that is required to
 * compile the package is an installed Tcl development system and a
 * working MPI-2 compliant installation. When compiled for a dynamically
 * loaded shared object (DSO), MPI has to be compiled and linked as
 * shared libraries (this is the default OpenMPI on Linux, but your
 * mileage may vary). As of May 25 2012 the code has been tested on
 * 32-bit and 64-bit x86 Linux platforms with OpenMPI, MVAPICH, Cray MPT
 * and SGI-MPT. OpenMPI and MVAPICH complete all tests, however MVAPICH
 * fails some due to differences in the error messages (needs to be
 * fixed in the test suite). SGI-MPT has problems with MPI_COMM_NULL
 * (segfault).
 *
 * To compile the package adjust the settings in the Makefile according
 * to your platform, MPI and Tcl installation. For most Linux
 * distributions, this requires installing not only an MPI and Tcl
 * package, but also the corresponding development packages, e.g. on
 * Fedora you need openmpi, openmpi-devel, tcl, and tcl-devel and their
 * dependencies. Then type make to compile the _tclmpi.so file or make
 * tclmpish to compile the extended Tcl shell. With make check or make
 * check-static you can run the integrated unittest package to see, if
 * everything is working as expected.
 *
 * To install you can create a directory,
 * e.g. /usr/local/libexec/tclmpi, and copy the files tclmpi.tcl,
 * _tclmpi.so and pkgIndex.tcl into it. If you then use the command set
 * auto_path [concat /usr/local/libexec/tclmpi $auto_path] in your
 * .tclshrc or .vmdrc, you can load the TclMPI wrappers on demand simply
 * by using the command package require tclmpi. For the extended shell,
 * the _tclmpi.so file can be omitted and instead tclmpish needs to be
 * copied somewhere in the search path for executable,
 * e.g. /usr/local/bin. In this case the startup script is called
 * .tclmpishrc instead of .tclshrc.
 *
 * \section devel Software Development and Bug Reports
 *
 * The TclMPI code is maintained using git for source code management,
 * and the project is hosted on github at
 * https://github.com/akohlmey/tclmpi From there you can download
 * snapshots of the development and releases, clode the repository to
 * follow development, or work on your own branch through forking
 * it. Bug reports and feature requests should also be filed on github
 * at through the issue tracker at:
 * https://github.com/akohlmey/tclmpi/issues.
 *
 * \section examples Example Programs
 * The following section provides some simple examples using TclMPI
 * to recreate some common MPI example programs in Tcl.
 *
 * \subsection hello Hello World
 * This is the TclMPI version of "hello world".
 * \code {.tcl}
 #!/bin/sh \
 exec tclsh "$0" "$@"
 package require tclmpi 0.6

 # initialize MPI
 ::tclmpi::init

 # get size of communicator and rank of process
 set comm tclmpi::comm_world
 set size [::tclmpi::comm_size $comm]
 set rank [::tclmpi::comm_rank $comm]

 puts "hello world, this is rank $rank of $size"

 # shut down MPI
 ::tclmpi::finalize
 exit 0
 * \endcode
 *
 * \subsection mypi Computation of Pi
 * This script uses TclMPI to compute the value of Pi from
 * numerical quadrature of the integral: 
   \f[ 
      \pi = \int^1_0 {\frac{4}{1 + x^2}} dx
   \f]
 * \code {.tcl}
 #!/bin/sh \
 exec tclsh "$0" "$@"
 package require tclmpi 0.6

 # initialize MPI
 ::tclmpi::init

 set comm tclmpi::comm_world
 set size [::tclmpi::comm_size $comm]
 set rank [::tclmpi::comm_rank $comm]
 set master 0

 set num [lindex $argv 0]
 # make sure all processes have the same interval parameter
 set num [::tclmpi::bcast $num ::tclmpi::int $master $comm]

 # run parallel calculation
 set h [expr {1.0/$num}]
 set sum 0.0
 for {set i $rank} {$i < $num} {incr i $size} {
     set sum [expr {$sum + 4.0/(1.0 + ($h*($i+0.5))**2)}]
 }
 set mypi [expr {$h * $sum}]

 # combine and print results
 set mypi [::tclmpi::allreduce $mypi tclmpi::double \
             tclmpi::sum $comm]
 if {$rank == $master} {
     set rel [expr {abs(($mypi - 3.14159265358979)/3.14159265358979)}]
     puts "result: $mypi. relative error: $rel"
 }

 # shut down MPI
 ::tclmpi::finalize
 exit 0
 * \endcode
 */

/*! \page devguide TclMPI Developer's Guide
 * 
 * This document explains the implementation of the Tcl bindings
 * for MPI implemented in TclMPI. The following sections will
 * document how and which MPI is mapped to Tcl and what design
 * choices were made.
 *
 * \section design Overall Design and Differences to the MPI C-bindings
 *
 * To be consistent with typical Tcl conventions all commands and constants
 * in lower case and prefixed with tclmpi, so that clashes with existing
 * programs are reduced.
 * This is not yet set up to be a proper namespace, but that may happen at
 * a later point, if the need arises. The overall philosophy of the bindings
 * is to make the API similar to the MPI one (e.g. maintain the order of
 * arguments), but don't stick to it slavishly and do things the Tcl way
 * wherever justified. Convenience and simplicity take precedence over
 * performance. If performance matters that much, one would write the entire
 * code C/C++ or Fortran and not Tcl. The biggest visible change is that
 * for sending data around, receive buffers will be automatically set up
 * to handle the entire message. Thus the typical "count" arguments of the
 * C/C++ or Fortran bindings for MPI is not required, and the received data
 * will be the return value of the corresponding command. This is consistent
 * with the automatic memory management in Tcl, but this convenience and
 * consistency will affect performance and the semantics. For example calls
 * to tclmpi::bcast will be converted into *two* calls to MPI_Bcast();
 * the first will broadcast the size of the data set being sent (so that
 * a sufficiently sized buffers can be allocated) and then the second call
 * will finally send the data for real. Similarly, tclmpi::recv will be
 * converted into calling MPI_Probe() and then MPI_Recv() for the purpose
 * of determining the amount of temporary storage required. The second call
 * will also use the MPI_SOURCE and MPI_TAG flags from the MPI_Status object
 * created for MPI_Probe() to make certain, the correct data is received.
 * 
 * Things get even more complicated with with non-blocking receives. Since
 * we need to know the size of the message to receive, a non-blocking receive
 * can only be posted, if the corresponding send is already pending. This is
 * being determined by calling MPI_Iprobe() and when this shows no (matching)
 * pending message, the parameters for the receive will be cached and the 
 * then MPI_Probe() followed by MPI_Recv() will be called as part of
 * tclmpi::wait. The blocking/non-blocking behavior of the Tcl script
 * should be very close to the corresponding C bindings, but probably not
 * as efficient.
 *
 *
 * \section Internal TclMPI Support Functions
 * Several MPI entities like communicators, requests, status objects
 * cannot be represented directly in Tcl. For TclMPI they need to be
 * mapped to something else, for example a string that will uniquely
 * identify this entity and then it will be translated into the real
 * object it represents with the help of the following support functions.
 *
 * \subsection tclcomm Mapping Communicators
 * MPI communicators are represented in TclMPI by strings of the form
 * "tclmpi::comm%d", with "%d" being replaced by a unique integer.
 * In addition, a few string constants are mapped to the default
 * communicators that are defined in MPI. These are tclmpi::comm_world,
 * tclmpi::comm_self, and tclmpi::comm_null, which represent
 * MPI_COMM_WORLD, MPI_COMM_SELF, and MPI_COMM_NULL, respectively.
 *
 * Internally the map is maintained in a simple linked list which
 * is initialized with the three default communicators when the plugin
 * is loaded and where new communicators are added at the end as needed.
 * The functions mpi2tcl_comm and tcl2mpi_comm are then used to translate
 * from one representation to the other while tclmpi_add_comm will 
 * append a new communicator to the list.
 */

/* fallback */
#if !defined(MPI_VERSION)
#define MPI_VERSION 1
#endif

/*! Linked list entry type for managing MPI communicators */
typedef struct tclmpi_comm tclmpi_comm_t;

/*! Linked list entry to map MPI communicators to strings. */
struct  tclmpi_comm {
    const char *label;    /*!< String representing the communicator in Tcl */
    MPI_Comm comm;        /*!< MPI communicator corresponding of this entry */
    int valid;            /*!< Non-zero if communicator is valid */
    tclmpi_comm_t *next;  /*!< Pointer to next element in linked list */
};

/*! First element of the communicator map list */
static tclmpi_comm_t *first_comm = NULL;
/*! Last element of the communicator map list */
static tclmpi_comm_t *last_comm = NULL;
/*! Communicator counter. Incremented to get unique strings */
static int tclmpi_comm_cntr = 0;

/*! Additional global communicator to detect unlisted communicators */
static MPI_Comm MPI_COMM_INVALID;

/*! Translate an MPI communicator to its Tcl label.
 * \param comm an MPI communicator
 * \return the corresponding string label or NULL.
 *
 * This function will search through the linked list of known communicators
 * until it finds the (first) match and then returns the string label to
 * the calling function. If a NULL is returned, the communicator does not
 * yet exist in the linked list.
 */
static const char *mpi2tcl_comm(MPI_Comm comm)
{
    tclmpi_comm_t *next;

    next = first_comm;
    while (next) {
        if (comm == next->comm) {
            if (next->valid != 0)
                return next->label;
            else
                return NULL;
        } else next = next->next;
    }
    return NULL;
}

/*! Translate a Tcl communicator label into the MPI communicator it represents.
 * \param label the Tcl name for the communicator
 * \return the matching MPI communicator or MPI_COMM_INVALID
 *
 * This function will search through the linked list of known communicators
 * until it finds the (first) match and then returns the string label to
 * the calling function. If a NULL is returned, the communicator does not
 * yet exist in the linked list.
 */
static MPI_Comm tcl2mpi_comm(const char *label)
{
    tclmpi_comm_t *next;

    next = first_comm;
    while (next) {
        if (strcmp(next->label,label) == 0) {
            if (next->valid != 0)
                return next->comm;
            else
                return MPI_COMM_INVALID;
        } else next = next->next;
    }
    return MPI_COMM_INVALID;
}

/*! Add an MPI communicator to the linked list of communicators, if needed.
 * \param comm an MPI communicator
 * \return the corresponding string label or NULL.
 *
 * This function will first call mpi2tcl_comm in order to see, if the
 * communicator handed it, is already listed and return that communicators
 * Tcl label string. If it is not yet lists, a new entry is added to the 
 * linked list and a new label of the format "tclmpi::comm%d" assigned.
 * The (global/static) variable tclmpi_comm_cntr is incremented every time
 * to make the communicator label unique.
 */
static const char *tclmpi_add_comm(MPI_Comm comm)
{
    tclmpi_comm_t *next;
    char *label;
    const char *oldlabel;

    oldlabel = mpi2tcl_comm(comm);
    if (oldlabel != NULL) return oldlabel;

    next = (tclmpi_comm_t *)Tcl_Alloc(sizeof(tclmpi_comm_t));
    next->next = NULL;
    next->comm = comm;
    next->valid = 1;
    label = (char *)Tcl_Alloc(32);
    sprintf(label,"tclmpi::comm%d",tclmpi_comm_cntr);
    next->label = label;
    ++tclmpi_comm_cntr;
    last_comm->next = next;
    last_comm = next;
    return next->label;
}

/* some symbolic constants */

#define TCLMPI_INVALID   -1  /*!< not ready to handle data */
#define TCLMPI_NONE       0  /*!< no data type assigned */
#define TCLMPI_AUTO       1  /*!< the tcl native data type (string) */
#define TCLMPI_INT        2  /*!< data type for integers */
#define TCLMPI_INT_INT    3  /*!< data type for pairs of integers */
#define TCLMPI_DOUBLE     4  /*!< floating point data type */
#define TCLMPI_DOUBLE_INT 5  /*!< data type for double/integer pair */

/* translate MPI requests to Tcl strings and back "tclmpi::req%d" */

/*! Linked list entry type for managing MPI requests */
typedef struct tclmpi_req tclmpi_req_t;

/*! Linked list entry to map MPI requests to "tclmpi::req%d" strings. */
struct  tclmpi_req {
    const char *label;  /*!< identifier of this request */
    void *data;         /*!< pointer to send or receive data buffer */
    int len;            /*!< size of data block */
    int type;           /*!< data type of send data */
    int source;         /*!< source rank of non-blocking receive */
    int tag;            /*!< tag selector of non-blocking receive */
    MPI_Request *req;   /*!< pointer MPI request handle generated by MPI */
    MPI_Comm comm;      /*!< communicator for non-blocking receive */
    tclmpi_req_t *next; /*!< pointer to next struct */
};

/*! First element of the list of generated requests */
static tclmpi_req_t *first_req = NULL;
/*! Request counter. Incremented to get unique strings */
static int tclmpi_req_cntr = 0;

/*! Allocate and add an entry to the request map linked list
 * \return the corresponding string label or NULL.
 *
 * This function will allocate and initialize a new linked list entry
 * for the translation between MPI requests and their string
 * representation passed to Tcl scripts. The assigned label of the 
 * for "tclmpi::req%d" will be returned. The (global/static) variable
 * tclmpi_req_cntr is incremented every time to make the communicator
 * label unique.
 */
static const char *tclmpi_add_req()
{
    tclmpi_req_t *next;
    char *label;

    next = (tclmpi_req_t *)Tcl_Alloc(sizeof(tclmpi_req_t));
    if (next == NULL) return NULL;
    memset(next,0,sizeof(tclmpi_req_t));

    next->req = (MPI_Request *)Tcl_Alloc(sizeof(MPI_Request));
    if (next->req == NULL) {
        Tcl_Free((char *)next);
        return NULL;
    }
    
    label = (char *)Tcl_Alloc(32);
    if (label == NULL) {
        Tcl_Free((char *)next->req);
        Tcl_Free((char *)next);
        return NULL;
    }

    sprintf(label,"tclmpi::req%d",tclmpi_req_cntr);
    next->label = label;
    next->type = TCLMPI_NONE;
    next->len = TCLMPI_INVALID;
    ++tclmpi_req_cntr;

    if (first_req == NULL) {
        first_req = next;
    } else {
        tclmpi_req_t *req;
        req = first_req;
        while (req->next) req = req->next;
        req->next = next;
    }
    
    return next->label;
}

/*! translate Tcl representation of an MPI request to request itself.
 * \param label the Tcl name for the communicator
 * \return a pointer to the matching tclmpi_req_t structure
 *
 * This function will search through the linked list of known MPI requests
 * until it finds the (first) match and then returns a pointer to this data.
 * If NULL is returned, the communicator does not exist in the linked list.
 */
static tclmpi_req_t *tclmpi_find_req(const char *label)
{
    tclmpi_req_t *req;

    req = first_req;
    while (req) {
        if (strcmp(req->label,label) == 0)
            return req;
        else req = req->next;
    }
    return NULL;
}

/*! remove tclmpi_req_t entry from the request linked list
 * \param req a pointer to the request in question
 * \return TCL_OK on succes, TCL_ERROR on failure
 *
 * This function will search through the linked list of known MPI requests
 * until it finds the (first) match and then will remove it from the linked
 * and free the allocated storage.
 * If TCL_ERROR is returned, the request did not exist in the linked list.
 */
static int tclmpi_del_req(tclmpi_req_t *req)
{
    if (req == NULL) return TCL_ERROR;

    if (req == first_req) {
        first_req = req->next;
        return TCL_OK;
    } else {
        tclmpi_req_t *prev = first_req;
        while (prev->next) {
            if (prev->next == req) {
                prev->next = prev->next->next;
                Tcl_Free((char *)req->label);
                Tcl_Free((char *)req->req);
                Tcl_Free((char *)req);
                return TCL_OK;
            }
            prev = prev->next;
        }
    }
    return TCL_ERROR;
}


/*! convert a string describing a data type to a numeric representation */
static int tclmpi_datatype(const char *type)
{
    if (strcmp(type,"tclmpi::int") == 0)
        return TCLMPI_INT;
    else if (strcmp(type,"tclmpi::double") == 0)
        return TCLMPI_DOUBLE;
    else if (strcmp(type,"tclmpi::dblint") == 0)
        return TCLMPI_DOUBLE_INT;
    else if (strcmp(type,"tclmpi::intint") == 0)
        return TCLMPI_INT_INT;
    else if (strcmp(type,"tclmpi::auto") == 0)
        return TCLMPI_AUTO;
    else return TCLMPI_NONE;
}


/*! buffer for error messages. */
static char tclmpi_errmsg[MPI_MAX_ERROR_STRING];

/*! convert MPI error code to Tcl error error message and append to result
 * \param interp current Tcl interpreter
 * \param ierr MPI error number. return value of an MPI call.
 * \param obj Tcl object representing the current command name
 * \return TCL_OK if the "error" is MPI_SUCCESS or TCL_ERROR
 *
 * This is a convenience wrapper that will use MPI_Error_string() to
 * convert any error code returned from MPI function calls to the
 * respective error class and that into a string. This string is
 * appended to the Tcl result vector of the current command. Should be
 * called after each MPI call. Since we change error handlers on all
 * communicators to not result in fatal errors, we have to generate Tcl
 * errors instead (which can be caught).
 */
static int tclmpi_errcheck(Tcl_Interp *interp, int ierr, Tcl_Obj *obj)
{
    if (ierr != MPI_SUCCESS) {
        int len,eclass;
        MPI_Error_class(ierr,&eclass);
        MPI_Error_string(eclass,tclmpi_errmsg,&len);
        Tcl_AppendResult(interp,Tcl_GetString(obj),": ",
                         tclmpi_errmsg,NULL);
        return TCL_ERROR;
    } else return TCL_OK;
}


/*! convenience function to report an unknown communicator as Tcl error
 * \param interp current Tcl interpreter
 * \param comm MPI communicator
 * \param obj0 Tcl object representing the current command name
 * \param obj1 Tcl object representing the communicator as Tcl name
 * \return TCL_ERROR if the communicator is MPI_COMM_INVALID or TCL_OK
 */
static int tclmpi_commcheck(Tcl_Interp *interp, MPI_Comm comm,
                            Tcl_Obj *obj0, Tcl_Obj *obj1)
{
    if (comm == MPI_COMM_INVALID) {
        Tcl_AppendResult(interp,Tcl_GetString(obj0),
                         ": unknown communicator: ",
                         Tcl_GetString(obj1),NULL);
        return TCL_ERROR;
    } else return TCL_OK;
}


/*! convenience function to report an unknown data type as Tcl error
 * \param interp current Tcl interpreter
 * \param type TclMPI data type
 * \param obj0 Tcl object representing the current command name
 * \param obj1 Tcl object representing the data type as Tcl name
 * \return TCL_ERROR if the communicator is TCLMPI_NONE or TCL_OK
 */
static int tclmpi_typecheck(Tcl_Interp *interp, int type,
                            Tcl_Obj *obj0, Tcl_Obj *obj1)
{
    if (type == TCLMPI_NONE) {
        Tcl_AppendResult(interp,Tcl_GetString(obj0),
                         ": invalid data type: ",
                         Tcl_GetString(obj1),NULL);
        return TCL_ERROR;
    } else return TCL_OK;
}


/*! is 1 after MPI_Init() and -1 after MPI_Finalize() */
static int tclmpi_init_done = 0;

/*! wrapper for MPI_Init() 
 * \param nodata ignored
 * \param interp current Tcl interpreter
 * \param objc number of argument objects
 * \param objv list of argument object
 * \return TCL_OK or TCL_ERROR
 *
 * This function does a little more work than just calling MPI_Init().
 * First of it tries to detect whether MPI_Init() has been called before
 * (from Tcl) and then creates a (catchable) Tcl error instead of an 
 * (uncatchable) MPI error. It will also try to pass the argument vector
 * to the script from the Tcl generated 'argv' array to the underlying
 * MPI_Init() call and reset argv as needed.
 */
int TclMPI_Init(ClientData nodata, Tcl_Interp *interp,
                int objc, Tcl_Obj *const objv[])
{
    Tcl_Obj *result,*argobj,**args;
    int argc,narg,i,ierr,tlevel;
    char **argv;

    if (objc != 1) {
        Tcl_WrongNumArgs(interp,1,objv,NULL);
        return TCL_ERROR;
    }

    /* convert "command line arguments" back to standard C stuff. */
    argobj = Tcl_GetVar2Ex(interp,"argv",NULL,TCL_GLOBAL_ONLY);
    Tcl_ListObjGetElements(interp,argobj,&narg,&args);

    argv = (char **)Tcl_Alloc((narg+1)*sizeof(char *));
    for (argc=1; argc <= narg; ++argc) {
        Tcl_IncrRefCount(args[argc-1]);
        argv[argc] = Tcl_GetString(args[argc-1]);
    }

    argobj = Tcl_GetVar2Ex(interp,"argv0",NULL,TCL_GLOBAL_ONLY);
    Tcl_IncrRefCount(argobj);
    argv[0] = Tcl_GetString(argobj);

    if (tclmpi_init_done != 0) {
        Tcl_AppendResult(interp,"Calling ",Tcl_GetString(objv[0]),
                         " multiple times is erroneous.",NULL);
        return TCL_ERROR;
    }

#if MPI_VERSION < 2
    ierr = MPI_Init(&argc,&argv);
#else
    /* XXX: this should be made dependent on whether Tcl
       was compiled with thread support or not and how 
       Tcl would handle threading in this case. */
    ierr = MPI_Init_thread(&argc,&argv,MPI_THREAD_SINGLE,&tlevel);
#endif
    if (tclmpi_errcheck(interp,ierr,objv[0]) != TCL_OK)
        return TCL_ERROR;
    tclmpi_init_done=1;

    /* change default error handler, so we can convert
       MPI errors into 'catch'able Tcl errors */
#if MPI_VERSION < 2
    MPI_Errhandler_set(MPI_COMM_WORLD,MPI_ERRORS_RETURN);
#else
    MPI_Comm_set_errhandler(MPI_COMM_WORLD,MPI_ERRORS_RETURN);
#endif

    /* build new argv list */
    result = Tcl_NewListObj(0,NULL);
    for (i=1; i < argc; ++i) {
        Tcl_ListObjAppendElement(interp,result,Tcl_NewStringObj(argv[i],-1));
    }
    for (i=0; i < narg; ++i)
        Tcl_DecrRefCount(args[i]);
    Tcl_DecrRefCount(argobj);

    Tcl_SetVar2Ex(interp,"argv",NULL,result,TCL_GLOBAL_ONLY);
    Tcl_SetVar2Ex(interp,"argc",NULL,Tcl_NewIntObj(argc-1),TCL_GLOBAL_ONLY);

    Tcl_Free((char *)argv);
    Tcl_ResetResult(interp);
    return TCL_OK;
}

/*! wrapper for MPI_Finalize() 
 * \param nodata ignored
 * \param interp current Tcl interpreter
 * \param objc number of argument objects
 * \param objv list of argument object
 * \return TCL_OK or TCL_ERROR
 *
 * This function does a little more than just calling MPI_Finalize(). It
 * also tries to detect whether MPI_Init() or MPI_Finialize() have been
 * called before (from Tcl) and then creates a (catchable) Tcl error
 * instead of an (uncatchable) MPI error.
 */
int TclMPI_Finalize(ClientData nodata, Tcl_Interp *interp,
                    int objc, Tcl_Obj *const objv[])
{
    if (objc != 1) {
        Tcl_WrongNumArgs(interp,1,objv,NULL);
        return TCL_ERROR;
    }

    if (tclmpi_init_done < 0) {
        Tcl_AppendResult(interp,"Calling ",Tcl_GetString(objv[0]),
                         " twice is erroneous.",NULL);
        return TCL_ERROR;
    }

    if (tclmpi_init_done == 0) {
        Tcl_AppendResult(interp,"Calling ",Tcl_GetString(objv[0]),
                         " before tclmpi::init is erroneous.",NULL);
        return TCL_ERROR;
    }

    MPI_Finalize();
    tclmpi_init_done=-1;

    return TCL_OK;
}


/*! wrapper for MPI_Abort() 
 * \param nodata ignored
 * \param interp current Tcl interpreter
 * \param objc number of argument objects
 * \param objv list of argument object
 * \return TCL_OK or TCL_ERROR
 *
 * This function translates the Tcl string representing a communicator
 * into the corresponding MPI communicator and then calls MPI_Abort().
 */
int TclMPI_Abort(ClientData nodata, Tcl_Interp *interp,
                 int objc, Tcl_Obj *const objv[])
{
    MPI_Comm comm;
    int ierr;

    if (objc != 3) {
        Tcl_WrongNumArgs(interp,1,objv,"<comm> <errorcode>");
        return TCL_ERROR;
    }

    comm = tcl2mpi_comm(Tcl_GetString(objv[1]));
    if (tclmpi_commcheck(interp,comm,objv[0],objv[1]) != TCL_OK)
        return TCL_ERROR;

    if (Tcl_GetIntFromObj(interp,objv[2],&ierr) != TCL_OK) {
        return TCL_ERROR;
    }

    MPI_Abort(comm,ierr);
    return TCL_OK;
}


/*! wrapper for MPI_Comm_size() 
 * \param nodata ignored
 * \param interp current Tcl interpreter
 * \param objc number of argument objects
 * \param objv list of argument object
 * \return TCL_OK or TCL_ERROR
 *
 * This function translates the Tcl string representing a communicator
 * into the corresponding MPI communicator and then calls MPI_Comm_size()
 * on it. The resulting number is passed to Tcl as result or the MPI error
 * message is passed up similarly.
 */
int TclMPI_Comm_size(ClientData nodata, Tcl_Interp *interp,
                     int objc, Tcl_Obj *const objv[])
{
    Tcl_Obj *result;
    MPI_Comm comm;
    int commsize,ierr;

    if (objc != 2) {
        Tcl_WrongNumArgs(interp,1,objv,"<comm>");
        return TCL_ERROR;
    }

    comm = tcl2mpi_comm(Tcl_GetString(objv[1]));
    if (tclmpi_commcheck(interp,comm,objv[0],objv[1]) != TCL_OK)
        return TCL_ERROR;

    ierr = MPI_Comm_size(comm,&commsize);
    if (tclmpi_errcheck(interp,ierr,objv[0]) != TCL_OK)
        return TCL_ERROR;

    result = Tcl_NewIntObj(commsize);
    Tcl_SetObjResult(interp,result);
    return TCL_OK;
}


/*! wrapper for MPI_Comm_rank() 
 * \param nodata ignored
 * \param interp current Tcl interpreter
 * \param objc number of argument objects
 * \param objv list of argument object
 * \return TCL_OK or TCL_ERROR
 *
 * This function translates the Tcl string representing a communicator
 * into the corresponding MPI communicator and then calls MPI_Comm_rank()
 * on it. The resulting number is passed to Tcl as result or the MPI error
 * message is passed up similarly.
 */
int TclMPI_Comm_rank(ClientData nodata, Tcl_Interp *interp,
                     int objc, Tcl_Obj *const objv[])
{
    Tcl_Obj *result;
    MPI_Comm comm;
    int commrank,ierr;

    if (objc != 2) {
        Tcl_WrongNumArgs(interp,1,objv,"<comm>");
        return TCL_ERROR;
    }

    comm = tcl2mpi_comm(Tcl_GetString(objv[1]));
    if (tclmpi_commcheck(interp,comm,objv[0],objv[1]) != TCL_OK)
        return TCL_ERROR;

    ierr = MPI_Comm_rank(comm,&commrank);
    if (tclmpi_errcheck(interp,ierr,objv[0]) != TCL_OK)
        return TCL_ERROR;

    result = Tcl_NewIntObj(commrank);
    Tcl_SetObjResult(interp,result);
    return TCL_OK;
}


/*! wrapper for MPI_Comm_split() 
 * \param nodata ignored
 * \param interp current Tcl interpreter
 * \param objc number of argument objects
 * \param objv list of argument object
 * \return TCL_OK or TCL_ERROR
 *
 * This function translates the Tcl string representing a communicator
 * into the corresponding MPI communicator also checks and converts the
 * values for 'color' and 'key' and then calls MPI_Comm_split().
 * The resulting communicator is added to the internal communicator map
 * linked list and its string representation is passed to Tcl as result.
 * If the MPI call failed the MPI error message is passed up similarly.
 */
int TclMPI_Comm_split(ClientData nodata, Tcl_Interp *interp,
                      int objc, Tcl_Obj *const objv[])
{
    Tcl_Obj *result;
    MPI_Comm comm,newcomm;
    int color,key,ierr;

    if (objc != 4) {
        Tcl_WrongNumArgs(interp,1,objv,"<comm> <color> <key>");
        return TCL_ERROR;
    }

    comm = tcl2mpi_comm(Tcl_GetString(objv[1]));
    if (tclmpi_commcheck(interp,comm,objv[0],objv[1]) != TCL_OK)
        return TCL_ERROR;

    if (strcmp(Tcl_GetString(objv[2]),"tclmpi::undefined") == 0)
        color = MPI_UNDEFINED;
    else {
        if (Tcl_GetIntFromObj(interp,objv[2],&color) != TCL_OK)
            return TCL_ERROR;
    }
    if (Tcl_GetIntFromObj(interp,objv[3],&key) != TCL_OK)
        return TCL_ERROR;

    /* several MPI applications do not catch this. so we got to do it */
    if (color < 0 && color != MPI_UNDEFINED) {
        Tcl_AppendResult(interp,Tcl_GetString(objv[0]),
                         ": invalid color argument", NULL);
        return TCL_ERROR;
    }

    ierr = MPI_Comm_split(comm,color,key,&newcomm);
    if (tclmpi_errcheck(interp,ierr,objv[0]) != TCL_OK)
        return TCL_ERROR;

    /* change default error handler on new communicator, so that
       we can convert MPI errors into 'catch'able Tcl errors */
#if MPI_VERSION < 2
    MPI_Errhandler_set(newcomm,MPI_ERRORS_RETURN);
#else
    MPI_Comm_set_errhandler(newcomm,MPI_ERRORS_RETURN);
#endif

    result = Tcl_NewStringObj(tclmpi_add_comm(newcomm),-1);
    Tcl_SetObjResult(interp,result);
    return TCL_OK;
}


/*! wrapper for MPI_Barrier() 
 * \param nodata ignored
 * \param interp current Tcl interpreter
 * \param objc number of argument objects
 * \param objv list of argument object
 * \return TCL_OK or TCL_ERROR
 *
 * This function translates the Tcl string representing a communicator
 * into the corresponding MPI communicator and then calls MPI_Barrier().
 * If the MPI call failed an MPI error message is passed up as result.
 */
int TclMPI_Barrier(ClientData nodata, Tcl_Interp *interp,
                     int objc, Tcl_Obj *const objv[])
{
    MPI_Comm comm;
    int ierr;

    if (objc != 2) {
        Tcl_WrongNumArgs(interp,1,objv,"<comm>");
        return TCL_ERROR;
    }

    comm = tcl2mpi_comm(Tcl_GetString(objv[1]));
    if (tclmpi_commcheck(interp,comm,objv[0],objv[1]) != TCL_OK)
        return TCL_ERROR;

    ierr = MPI_Barrier(comm);
    if (tclmpi_errcheck(interp,ierr,objv[0]) != TCL_OK)
        return TCL_ERROR;

    return TCL_OK;
}


/*! wrapper for MPI_Bcast() 
 * \param nodata ignored
 * \param interp current Tcl interpreter
 * \param objc number of argument objects
 * \param objv list of argument object
 * \return TCL_OK or TCL_ERROR
 *
 * This function implements a broadcast function for TclMPI. Unlike in the 
 * C bindings, the length of the data is inferred from the data object
 * passed to this function and thus a 'count' argument is not needed.
 * Only a limited number of data types are currently supported, since 
 * Tcl has a limited number of "native" data types. The tclmpi::auto
 * data type transfers the internal string representation of an object,
 * while the other data types convert data to native data types as needed,
 * with all non-representable data translated into either 0 or 0.0.
 * In all cases, two broadcasts are needed. The first to transmit the
 * amount of data being sent so that a suitable receive buffer can be set up.
 * 
 * The result of the broadcast is converted back into Tcl objects and
 * passed up as result value to the calling Tcl code. If the MPI call
 * failed an MPI error message is passed up as result instead.
 */
int TclMPI_Bcast(ClientData nodata, Tcl_Interp *interp,
                 int objc, Tcl_Obj *const objv[])
{
    Tcl_Obj *result = NULL;
    MPI_Comm comm;
    int i,rank,root,type,len,ierr;

    if (objc != 5) {
        Tcl_WrongNumArgs(interp,1,objv,"<data> <type> <root> <comm>");
        return TCL_ERROR;
    }

    type = tclmpi_datatype(Tcl_GetString(objv[2]));
    if (tclmpi_typecheck(interp,type,objv[0],objv[2]) != TCL_OK)
        return TCL_ERROR;

    if (Tcl_GetIntFromObj(interp,objv[3],&root) != TCL_OK)
        return TCL_ERROR;

    comm = tcl2mpi_comm(Tcl_GetString(objv[4]));
    if (tclmpi_commcheck(interp,comm,objv[0],objv[4]) != TCL_OK)
        return TCL_ERROR;

    ierr = MPI_SUCCESS;
    MPI_Comm_rank(comm,&rank);

    if (type == TCLMPI_AUTO) {
        char *idata;
        if (rank == root) {
            idata = Tcl_GetStringFromObj(objv[1],&len);
            MPI_Bcast(&len,1,MPI_INT,root,comm);
            ierr = MPI_Bcast(idata,len,MPI_CHAR,root,comm);
            result = Tcl_DuplicateObj(objv[1]);
        } else {
            MPI_Bcast(&len,1,MPI_INT,root,comm);
            idata = Tcl_Alloc(len);
            ierr = MPI_Bcast(idata,len,MPI_CHAR,root,comm);
            result = Tcl_NewStringObj(idata,len);
            Tcl_Free(idata);
        }
    } else if (type == TCLMPI_INT) {
        Tcl_Obj **ilist;
        int *idata;
        if (rank == root) {
            if (Tcl_ListObjGetElements(interp,objv[1],&len,&ilist) != TCL_OK)
                return TCL_ERROR;
            idata = (int *)Tcl_Alloc(len*sizeof(int));
            for (i=0; i < len; ++i)
                if (Tcl_GetIntFromObj(interp,ilist[i],idata+i) != TCL_OK) {
                    Tcl_ResetResult(interp);
                    idata[i]=0; /* this is non-fatal by choice */
                }
            MPI_Bcast(&len,1,MPI_INT,root,comm);
            ierr = MPI_Bcast(idata,len,MPI_INT,root,comm);
        } else {
            MPI_Bcast(&len,1,MPI_INT,root,comm);
            idata = (int *)Tcl_Alloc(len*sizeof(int));
            ierr = MPI_Bcast(idata,len,MPI_INT,root,comm);
        }
        result = Tcl_NewListObj(0,NULL);
        for (i=0; i < len; ++i)
            Tcl_ListObjAppendElement(interp,result,Tcl_NewIntObj(idata[i]));
        Tcl_Free((char *)idata);

    } else if (type == TCLMPI_DOUBLE) {
        Tcl_Obj **ilist;
        double *idata;
        if (rank == root) {
            if (Tcl_ListObjGetElements(interp,objv[1],&len,&ilist) != TCL_OK)
                return TCL_ERROR;
            idata = (double *)Tcl_Alloc(len*sizeof(double));
            for (i=0; i < len; ++i)
                if (Tcl_GetDoubleFromObj(interp,ilist[i],idata+i) != TCL_OK) {
                    Tcl_ResetResult(interp);
                    idata[i]=0.0; /* this is non-fatal by choice */
                }
            MPI_Bcast(&len,1,MPI_INT,root,comm);
            ierr = MPI_Bcast(idata,len,MPI_DOUBLE,root,comm);
        } else {
            MPI_Bcast(&len,1,MPI_INT,root,comm);
            idata = (double *)Tcl_Alloc(len*sizeof(double));
            ierr = MPI_Bcast(idata,len,MPI_DOUBLE,root,comm);
        }
        result = Tcl_NewListObj(0,NULL);
        for (i=0; i < len; ++i)
            Tcl_ListObjAppendElement(interp,result,
                                     Tcl_NewDoubleObj(idata[i]));
        Tcl_Free((char *)idata);
    } else {
        Tcl_AppendResult(interp,Tcl_GetString(objv[0]),
                         ": support for data type ", Tcl_GetString(objv[2]),
                         " is not yet implemented.",NULL);
        return TCL_ERROR;
    }

    if (tclmpi_errcheck(interp,ierr,objv[0]) != TCL_OK)
        return TCL_ERROR;

    if (result) Tcl_SetObjResult(interp,result);
    return TCL_OK;
}

/*! wrapper for MPI_Scatter() 
 * \param nodata ignored
 * \param interp current Tcl interpreter
 * \param objc number of argument objects
 * \param objv list of argument object
 * \return TCL_OK or TCL_ERROR
 *
 * This function implements a scatter operation that distributes 
 * data for TclMPI.
 * This operation does not accept the tclmpi::auto data type, also support
 * for types outside of tclmpi::int and tclmpi::double is incomplete.
 * The length of the data is inferred from the data object passed to this 
 * function and thus a 'count' argument is not needed. The number of data
 * items has to be divisible by the number of processes on the communicator.
 * 
 * The result is converted back into Tcl objects and passed up as result
 * value to the calling Tcl code. If the MPI call failed an MPI error
 * message is passed up as result instead.
 */
int TclMPI_Scatter(ClientData nodata, Tcl_Interp *interp,
                   int objc, Tcl_Obj *const objv[])
{
    Tcl_Obj *result = NULL;
    MPI_Comm comm;
    int i,type,root,size,rank,ilen,olen,ierr;

    if (objc != 5) {
        Tcl_WrongNumArgs(interp,1,objv,"<data> <type> <root> <comm>");
        return TCL_ERROR;
    }

    type = tclmpi_datatype(Tcl_GetString(objv[2]));
    if (tclmpi_typecheck(interp,type,objv[0],objv[2]) != TCL_OK)
        return TCL_ERROR;

    if (Tcl_GetIntFromObj(interp,objv[3],&root) != TCL_OK)
        return TCL_ERROR;

    comm = tcl2mpi_comm(Tcl_GetString(objv[4]));
    if (tclmpi_commcheck(interp,comm,objv[0],objv[4]) != TCL_OK)
        return TCL_ERROR;

    /* special case check for reduction */
    if (type == TCLMPI_AUTO) {
        Tcl_AppendResult(interp,Tcl_GetString(objv[0]),
                         ": does not support data type ",
                         Tcl_GetString(objv[2]),NULL);
        return TCL_ERROR;
    }
    MPI_Comm_size(comm,&size);
    MPI_Comm_rank(comm,&rank);
    ierr = MPI_SUCCESS;
    Tcl_IncrRefCount(objv[1]);

    if (type == TCLMPI_INT) {
        Tcl_Obj **ilist;
        int *idata, *odata;
        if (Tcl_ListObjGetElements(interp,objv[1],&ilen,&ilist) != TCL_OK)
            return TCL_ERROR;
        MPI_Bcast(&ilen,1,MPI_INT,root,comm);
        olen = ilen / size;
        if (olen*size != ilen) {
            Tcl_AppendResult(interp,Tcl_GetString(objv[0]),
                             ": number of data items must be divisible"
                             " by the number of processes",NULL);
            Tcl_DecrRefCount(objv[1]);
            return TCL_ERROR;
        }
        
        odata = (int *)Tcl_Alloc(olen*sizeof(int));
        result = Tcl_NewListObj(0,NULL);
        if (rank == root) {
            idata = (int *)Tcl_Alloc(ilen*sizeof(int));
            for (i=0; i < ilen; ++i)
                if (Tcl_GetIntFromObj(interp,ilist[i],idata+i) != TCL_OK) {
                    Tcl_ResetResult(interp);
                    idata[i]=0; /* this is non-fatal by choice */
                }
            ierr=MPI_Scatter(idata,olen,MPI_INT,odata,olen,MPI_INT,root,comm);
            Tcl_Free((char *)idata);
        } else {
            idata=NULL;
            ierr=MPI_Scatter(idata,olen,MPI_INT,odata,olen,MPI_INT,root,comm);
        }
        for (i=0; i < olen; ++i)
            Tcl_ListObjAppendElement(interp,result,
                                     Tcl_NewIntObj(odata[i]));
        Tcl_Free((char *)odata);

    } else if (type == TCLMPI_DOUBLE) {
        Tcl_Obj **ilist;
        double *idata, *odata;
        if (Tcl_ListObjGetElements(interp,objv[1],&ilen,&ilist) != TCL_OK)
            return TCL_ERROR;
        MPI_Bcast(&ilen,1,MPI_INT,root,comm);
        olen = ilen / size;
        if (olen*size != ilen) {
            Tcl_AppendResult(interp,Tcl_GetString(objv[0]),
                             ": number of data items must be divisible"
                             " by the number of processes",NULL);
            Tcl_DecrRefCount(objv[1]);
            return TCL_ERROR;
        }
        
        odata = (double *)Tcl_Alloc(olen*sizeof(double));
        result = Tcl_NewListObj(0,NULL);
        if (rank == root) {
            idata = (double *)Tcl_Alloc(ilen*sizeof(double));
            for (i=0; i < ilen; ++i)
                if (Tcl_GetDoubleFromObj(interp,ilist[i],idata+i) != TCL_OK) {
                    Tcl_ResetResult(interp);
                    idata[i]=0.0; /* this is non-fatal by choice */
                }
            ierr=MPI_Scatter(idata,olen,MPI_DOUBLE,
                             odata,olen,MPI_DOUBLE,root,comm);
            Tcl_Free((char *)idata);
        } else {
            idata=NULL;
            ierr=MPI_Scatter(idata,olen,MPI_DOUBLE,
                             odata,olen,MPI_DOUBLE,root,comm);
        }
        for (i=0; i < olen; ++i)
            Tcl_ListObjAppendElement(interp,result,
                                     Tcl_NewDoubleObj(odata[i]));
        Tcl_Free((char *)odata);

    } else {
        Tcl_DecrRefCount(objv[1]);
        Tcl_AppendResult(interp,Tcl_GetString(objv[0]),
                         ": support for data type ", Tcl_GetString(objv[2]),
                         " is not yet implemented.",NULL);
        return TCL_ERROR;
    }
    Tcl_DecrRefCount(objv[1]);

    if (tclmpi_errcheck(interp,ierr,objv[0]) != TCL_OK)
        return TCL_ERROR;

    if (result) Tcl_SetObjResult(interp,result);
    return TCL_OK;
}

/*! wrapper for MPI_Gather() 
 * \param nodata ignored
 * \param interp current Tcl interpreter
 * \param objc number of argument objects
 * \param objv list of argument object
 * \return TCL_OK or TCL_ERROR
 *
 * This function implements a gather operation that collects data for TclMPI.
 * This operation does not accept the tclmpi::auto data type, also support
 * for types outside of tclmpi::int and tclmpi::double is incomplete.
 * The length of the data is inferred from the data object passed to this 
 * function and thus a 'count' argument is not needed. The number of data
 * items has to be the same on all processes on the communicator.
 * 
 * The result is converted back into Tcl objects and passed up as result
 * value to the calling Tcl code. If the MPI call failed an MPI error
 * message is passed up as result instead.
 */
int TclMPI_Gather(ClientData nodata, Tcl_Interp *interp,
                  int objc, Tcl_Obj *const objv[])
{
    Tcl_Obj *result = NULL;
    MPI_Comm comm;
    int i,type,root,size,rank,ilen,olen,mlen,ierr;

    if (objc != 5) {
        Tcl_WrongNumArgs(interp,1,objv,"<data> <type> <root> <comm>");
        return TCL_ERROR;
    }

    type = tclmpi_datatype(Tcl_GetString(objv[2]));
    if (tclmpi_typecheck(interp,type,objv[0],objv[2]) != TCL_OK)
        return TCL_ERROR;

    if (Tcl_GetIntFromObj(interp,objv[3],&root) != TCL_OK)
        return TCL_ERROR;

    comm = tcl2mpi_comm(Tcl_GetString(objv[4]));
    if (tclmpi_commcheck(interp,comm,objv[0],objv[4]) != TCL_OK)
        return TCL_ERROR;

    /* special case check for reduction */
    if (type == TCLMPI_AUTO) {
        Tcl_AppendResult(interp,Tcl_GetString(objv[0]),
                         ": does not support data type ",
                         Tcl_GetString(objv[2]),NULL);
        return TCL_ERROR;
    }

    MPI_Comm_size(comm,&size);
    MPI_Comm_rank(comm,&rank);
    ierr = MPI_SUCCESS;
    Tcl_IncrRefCount(objv[1]);

    if (type == TCLMPI_INT) {
        Tcl_Obj **ilist;
        int *idata, *odata;
        if (Tcl_ListObjGetElements(interp,objv[1],&ilen,&ilist) != TCL_OK)
            return TCL_ERROR;
        MPI_Allreduce(&ilen,&olen,1,MPI_INT,MPI_MAX,comm);
        MPI_Allreduce(&ilen,&mlen,1,MPI_INT,MPI_MIN,comm);
        if (olen != mlen) {
            Tcl_AppendResult(interp,Tcl_GetString(objv[0]),
                             ": number of data items must be the same"
                             " on all processes",NULL);
            Tcl_DecrRefCount(objv[1]);
            return TCL_ERROR;
        }
        
        mlen = olen*size;
        idata = (int *)Tcl_Alloc(ilen*sizeof(int));
        for (i=0; i < ilen; ++i)
            if (Tcl_GetIntFromObj(interp,ilist[i],idata+i) != TCL_OK) {
                Tcl_ResetResult(interp);
                idata[i]=0; /* this is non-fatal by choice */
            }
        result = Tcl_NewListObj(0,NULL);
        if (rank == root) {
            odata = (int *)Tcl_Alloc(olen*size*sizeof(int));
            ierr=MPI_Gather(idata,ilen,MPI_INT,odata,olen,MPI_INT,root,comm);
            for (i=0; i < mlen; ++i)
                Tcl_ListObjAppendElement(interp,result,
                                         Tcl_NewIntObj(odata[i]));
            Tcl_Free((char *)odata);
        } else {
            odata=NULL;
            ierr=MPI_Gather(idata,ilen,MPI_INT,odata,olen,MPI_INT,root,comm);
        }
        Tcl_Free((char *)idata);

    } else if (type == TCLMPI_DOUBLE) {
        Tcl_Obj **ilist;
        double *idata, *odata;
        if (Tcl_ListObjGetElements(interp,objv[1],&ilen,&ilist) != TCL_OK)
            return TCL_ERROR;
        MPI_Allreduce(&ilen,&olen,1,MPI_INT,MPI_MAX,comm);
        MPI_Allreduce(&ilen,&mlen,1,MPI_INT,MPI_MIN,comm);
        if (olen != mlen) {
            Tcl_AppendResult(interp,Tcl_GetString(objv[0]),
                             ": number of data items must be the same"
                             " on all processes",NULL);
            Tcl_DecrRefCount(objv[1]);
            return TCL_ERROR;
        }
        
        mlen = olen*size;
        idata = (double *)Tcl_Alloc(ilen*sizeof(double));
        for (i=0; i < ilen; ++i)
            if (Tcl_GetDoubleFromObj(interp,ilist[i],idata+i) != TCL_OK) {
                Tcl_ResetResult(interp);
                idata[i]=0.0; /* this is non-fatal by choice */
            }
        result = Tcl_NewListObj(0,NULL);
        if (rank == root) {
            odata = (double *)Tcl_Alloc(mlen*sizeof(double));
            ierr=MPI_Gather(idata,ilen,MPI_DOUBLE,
                            odata,olen,MPI_DOUBLE,root,comm);
            for (i=0; i < mlen; ++i)
                Tcl_ListObjAppendElement(interp,result,
                                         Tcl_NewDoubleObj(odata[i]));
            Tcl_Free((char *)odata);
        } else {
            odata=NULL;
            ierr=MPI_Gather(idata,ilen,MPI_DOUBLE,
                            odata,olen,MPI_DOUBLE,root,comm);
        }
        Tcl_Free((char *)idata);

    } else {
        Tcl_DecrRefCount(objv[1]);
        Tcl_AppendResult(interp,Tcl_GetString(objv[0]),
                         ": support for data type ", Tcl_GetString(objv[2]),
                         " is not yet implemented.",NULL);
        return TCL_ERROR;
    }
    Tcl_DecrRefCount(objv[1]);

    if (tclmpi_errcheck(interp,ierr,objv[0]) != TCL_OK)
        return TCL_ERROR;

    if (result) Tcl_SetObjResult(interp,result);
    return TCL_OK;
}


/*! wrapper for MPI_Allreduce() 
 * \param nodata ignored
 * \param interp current Tcl interpreter
 * \param objc number of argument objects
 * \param objv list of argument object
 * \return TCL_OK or TCL_ERROR
 *
 * This function implements a reduction plus broadcast function for TclMPI.
 * This operation does not accept the tclmpi::auto data type, also support
 * for types outside of tclmpi::int and tclmpi::double is incomplete.
 * The length of the data is inferred from the data object passed to this 
 * function and thus a 'count' argument is not needed.
 * 
 * The result is converted back into Tcl objects and passed up as result
 * value to the calling Tcl code. If the MPI call failed an MPI error
 * message is passed up as result instead.
 */
int TclMPI_Allreduce(ClientData nodata, Tcl_Interp *interp,
                     int objc, Tcl_Obj *const objv[])
{
    Tcl_Obj *result = NULL;
    const char *opstr;
    MPI_Comm comm;
    MPI_Op op;
    int i,type,len,ierr;

    if (objc != 5) {
        Tcl_WrongNumArgs(interp,1,objv,"<data> <type> <op> <comm>");
        return TCL_ERROR;
    }

    type = tclmpi_datatype(Tcl_GetString(objv[2]));
    if (tclmpi_typecheck(interp,type,objv[0],objv[2]) != TCL_OK)
        return TCL_ERROR;

    opstr = Tcl_GetString(objv[3]);
    comm = tcl2mpi_comm(Tcl_GetString(objv[4]));
    if (tclmpi_commcheck(interp,comm,objv[0],objv[4]) != TCL_OK)
        return TCL_ERROR;

    /* special case check for reduction */
    if (type == TCLMPI_AUTO) {
        Tcl_AppendResult(interp,Tcl_GetString(objv[0]),
                         ": does not support data type ",
                         Tcl_GetString(objv[2]),NULL);
        return TCL_ERROR;
    }

    if (strcmp(opstr,"tclmpi::max") == 0)
        op = MPI_MAX;
    else if (strcmp(opstr,"tclmpi::min") == 0)
        op = MPI_MIN;
    else if (strcmp(opstr,"tclmpi::sum") == 0)
        op = MPI_SUM;
    else if (strcmp(opstr,"tclmpi::prod") == 0)
        op = MPI_PROD;
    else if (strcmp(opstr,"tclmpi::land") == 0)
        op = MPI_LAND;
    else if (strcmp(opstr,"tclmpi::band") == 0)
        op = MPI_BAND;
    else if (strcmp(opstr,"tclmpi::lor") == 0)
        op = MPI_LOR;
    else if (strcmp(opstr,"tclmpi::bor") == 0)
        op = MPI_BOR;
    else if (strcmp(opstr,"tclmpi::lxor") == 0)
        op = MPI_LXOR;
    else if (strcmp(opstr,"tclmpi::bxor") == 0)
        op = MPI_BXOR;
    else if (strcmp(opstr,"tclmpi::maxloc") == 0)
        op = MPI_MAXLOC;
    else if (strcmp(opstr,"tclmpi::minloc") == 0)
        op = MPI_MINLOC;
    else {
        Tcl_AppendResult(interp,Tcl_GetString(objv[0]),
                         ": unknown reduction operator: ",opstr,NULL);
        return TCL_ERROR;
    }

    ierr = MPI_SUCCESS;
    Tcl_IncrRefCount(objv[1]);

    if (type == TCLMPI_INT) {
        Tcl_Obj **ilist;
        int *idata, *odata;
        if (Tcl_ListObjGetElements(interp,objv[1],&len,&ilist) != TCL_OK)
            return TCL_ERROR;
        idata = (int *)Tcl_Alloc(len*sizeof(int));
        odata = (int *)Tcl_Alloc(len*sizeof(int));
        for (i=0; i < len; ++i)
            if (Tcl_GetIntFromObj(interp,ilist[i],idata+i) != TCL_OK) {
                Tcl_ResetResult(interp);
                idata[i]=0; /* this is non-fatal by choice */
            }
        ierr = MPI_Allreduce(idata,odata,len,MPI_INT,op,comm);
        result = Tcl_NewListObj(0,NULL);
        for (i=0; i < len; ++i)
            Tcl_ListObjAppendElement(interp,result,
                                     Tcl_NewIntObj(odata[i]));
        Tcl_Free((char *)idata);
        Tcl_Free((char *)odata);

    } else if (type == TCLMPI_DOUBLE) {
        Tcl_Obj **ilist;
        double *idata, *odata;
        if (Tcl_ListObjGetElements(interp,objv[1],&len,&ilist) != TCL_OK)
            return TCL_ERROR;
        idata = (double *)Tcl_Alloc(len*sizeof(double));
        odata = (double *)Tcl_Alloc(len*sizeof(double));
        for (i=0; i < len; ++i)
            if (Tcl_GetDoubleFromObj(interp,ilist[i],idata+i) != TCL_OK) {
                Tcl_ResetResult(interp);
                idata[i]=0.0; /* this is non-fatal by choice */
            }
        ierr = MPI_Allreduce(idata,odata,len,MPI_DOUBLE,op,comm);
        result = Tcl_NewListObj(0,NULL);
        for (i=0; i < len; ++i)
            Tcl_ListObjAppendElement(interp,result,
                                     Tcl_NewDoubleObj(odata[i]));
        Tcl_Free((char *)idata);
        Tcl_Free((char *)odata);
    } else if (type == TCLMPI_INT_INT) {
        Tcl_Obj **ilist;
        int *idata, *odata;
        if (Tcl_ListObjGetElements(interp,objv[1],&len,&ilist) != TCL_OK)
            return TCL_ERROR;
        idata = (int *)Tcl_Alloc(len*sizeof(int));
        odata = (int *)Tcl_Alloc(len*sizeof(int));
        for (i=0; i < len; ++i)
            if (Tcl_GetIntFromObj(interp,ilist[i],idata+i) != TCL_OK) {
                Tcl_ResetResult(interp);
                idata[i]=0; /* this is non-fatal by choice */
            }
        ierr = MPI_Allreduce(idata,odata,len/2,MPI_2INT,op,comm);
        result = Tcl_NewListObj(0,NULL);
        for (i=0; i < len; ++i)
            Tcl_ListObjAppendElement(interp,result,
                                     Tcl_NewIntObj(odata[i]));
        Tcl_Free((char *)idata);
        Tcl_Free((char *)odata);    
    } else {
        Tcl_DecrRefCount(objv[1]);
        Tcl_AppendResult(interp,Tcl_GetString(objv[0]),
                         ": support for data type ", Tcl_GetString(objv[2]),
                         " is not yet implemented.",NULL);
        return TCL_ERROR;
    }
    Tcl_DecrRefCount(objv[1]);

    if (tclmpi_errcheck(interp,ierr,objv[0]) != TCL_OK)
        return TCL_ERROR;

    if (result) Tcl_SetObjResult(interp,result);
    return TCL_OK;
}

/*! wrapper for MPI_Reduce() 
 * \param nodata ignored
 * \param interp current Tcl interpreter
 * \param objc number of argument objects
 * \param objv list of argument object
 * \return TCL_OK or TCL_ERROR
 *
 * This function implements a reduction function for TclMPI.
 * This operation does not accept the tclmpi::auto data type, also support
 * for types outside of tclmpi::int and tclmpi::double is incomplete.
 * The length of the data is inferred from the data object passed to this 
 * function and thus a 'count' argument is not needed.
 * 
 * The result is collected on the process with rank root and converted
 *  back into Tcl objects and passed up as result value to the calling
 * Tcl code. If the MPI call failed an MPI error message is passed up
 * as result instead.
 */
int TclMPI_Reduce(ClientData nodata, Tcl_Interp *interp,
                     int objc, Tcl_Obj *const objv[])
{
    Tcl_Obj *result = NULL;
    const char *opstr;
    MPI_Comm comm;
    MPI_Op op;
    int i,type,root,rank,len,ierr;

    if (objc != 6) {
        Tcl_WrongNumArgs(interp,1,objv,"<data> <type> <op> <root> <comm>");
        return TCL_ERROR;
    }

    type = tclmpi_datatype(Tcl_GetString(objv[2]));
    if (tclmpi_typecheck(interp,type,objv[0],objv[2]) != TCL_OK)
        return TCL_ERROR;

    opstr = Tcl_GetString(objv[3]);

    if (Tcl_GetIntFromObj(interp,objv[4],&root) != TCL_OK)
        return TCL_ERROR;

    comm = tcl2mpi_comm(Tcl_GetString(objv[5]));
    if (tclmpi_commcheck(interp,comm,objv[0],objv[5]) != TCL_OK)
        return TCL_ERROR;

    /* special case check for reduction */
    if (type == TCLMPI_AUTO) {
        Tcl_AppendResult(interp,Tcl_GetString(objv[0]),
                         ": does not support data type ",
                         Tcl_GetString(objv[2]),NULL);
        return TCL_ERROR;
    }

    if (strcmp(opstr,"tclmpi::max") == 0)
        op = MPI_MAX;
    else if (strcmp(opstr,"tclmpi::min") == 0)
        op = MPI_MIN;
    else if (strcmp(opstr,"tclmpi::sum") == 0)
        op = MPI_SUM;
    else if (strcmp(opstr,"tclmpi::prod") == 0)
        op = MPI_PROD;
    else if (strcmp(opstr,"tclmpi::land") == 0)
        op = MPI_LAND;
    else if (strcmp(opstr,"tclmpi::band") == 0)
        op = MPI_BAND;
    else if (strcmp(opstr,"tclmpi::lor") == 0)
        op = MPI_LOR;
    else if (strcmp(opstr,"tclmpi::bor") == 0)
        op = MPI_BOR;
    else if (strcmp(opstr,"tclmpi::lxor") == 0)
        op = MPI_LXOR;
    else if (strcmp(opstr,"tclmpi::bxor") == 0)
        op = MPI_BXOR;
    else if (strcmp(opstr,"tclmpi::maxloc") == 0)
        op = MPI_MAXLOC;
    else if (strcmp(opstr,"tclmpi::minloc") == 0)
        op = MPI_MINLOC;
    else {
        Tcl_AppendResult(interp,Tcl_GetString(objv[0]),
                         ": unknown reduction operator: ",opstr,NULL);
        return TCL_ERROR;
    }

    MPI_Comm_rank(comm,&rank);
    ierr = MPI_SUCCESS;
    Tcl_IncrRefCount(objv[1]);

    if (type == TCLMPI_INT) {
        Tcl_Obj **ilist;
        int *idata, *odata;
        if (Tcl_ListObjGetElements(interp,objv[1],&len,&ilist) != TCL_OK)
            return TCL_ERROR;
        idata = (int *)Tcl_Alloc(len*sizeof(int));
        if (rank == root)
            odata = (int *)Tcl_Alloc(len*sizeof(int));
        else odata = NULL;
        for (i=0; i < len; ++i)
            if (Tcl_GetIntFromObj(interp,ilist[i],idata+i) != TCL_OK) {
                Tcl_ResetResult(interp);
                idata[i]=0; /* this is non-fatal by choice */
            }
        ierr = MPI_Reduce(idata,odata,len,MPI_INT,op,root,comm);
        result = Tcl_NewListObj(0,NULL);
        if (rank == root)
            for (i=0; i < len; ++i)
                Tcl_ListObjAppendElement(interp,result,
                                         Tcl_NewIntObj(odata[i]));
        Tcl_Free((char *)idata);
        if (rank == root)
            Tcl_Free((char *)odata);

    } else if (type == TCLMPI_DOUBLE) {
        Tcl_Obj **ilist;
        double *idata, *odata;
        if (Tcl_ListObjGetElements(interp,objv[1],&len,&ilist) != TCL_OK)
            return TCL_ERROR;
        idata = (double *)Tcl_Alloc(len*sizeof(double));
        if (rank == root)
            odata = (double *)Tcl_Alloc(len*sizeof(double));
        else odata = NULL;
        for (i=0; i < len; ++i)
            if (Tcl_GetDoubleFromObj(interp,ilist[i],idata+i) != TCL_OK) {
                Tcl_ResetResult(interp);
                idata[i]=0.0; /* this is non-fatal by choice */
            }
        ierr = MPI_Reduce(idata,odata,len,MPI_DOUBLE,op,root,comm);
        result = Tcl_NewListObj(0,NULL);
        if (rank == root)
            for (i=0; i < len; ++i)
                Tcl_ListObjAppendElement(interp,result,
                                         Tcl_NewDoubleObj(odata[i]));
        Tcl_Free((char *)idata);
        if (rank == root)
            Tcl_Free((char *)odata);
    } else if (type == TCLMPI_INT_INT) {
        Tcl_Obj **ilist;
        int *idata, *odata;
        if (Tcl_ListObjGetElements(interp,objv[1],&len,&ilist) != TCL_OK)
            return TCL_ERROR;
        idata = (int *)Tcl_Alloc(len*sizeof(int));
        if (rank == root)
            odata = (int *)Tcl_Alloc(len*sizeof(int));
        else odata = NULL;
        for (i=0; i < len; ++i)
            if (Tcl_GetIntFromObj(interp,ilist[i],idata+i) != TCL_OK) {
                Tcl_ResetResult(interp);
                idata[i]=0; /* this is non-fatal by choice */
            }
        ierr = MPI_Reduce(idata,odata,len/2,MPI_2INT,op,root,comm);
        result = Tcl_NewListObj(0,NULL);
        if (rank == root)
            for (i=0; i < len; ++i)
                Tcl_ListObjAppendElement(interp,result,
                                         Tcl_NewIntObj(odata[i]));
        Tcl_Free((char *)idata);
        if (rank == root)
            Tcl_Free((char *)odata);    
    } else {
        Tcl_DecrRefCount(objv[1]);
        Tcl_AppendResult(interp,Tcl_GetString(objv[0]),
                         ": support for data type ", Tcl_GetString(objv[2]),
                         " is not yet implemented.",NULL);
        return TCL_ERROR;
    }
    Tcl_DecrRefCount(objv[1]);

    if (tclmpi_errcheck(interp,ierr,objv[0]) != TCL_OK)
        return TCL_ERROR;

    if (result) Tcl_SetObjResult(interp,result);
    return TCL_OK;
}

/*! wrapper for MPI_Send() 
 * \param nodata ignored
 * \param interp current Tcl interpreter
 * \param objc number of argument objects
 * \param objv list of argument object
 * \return TCL_OK or TCL_ERROR
 *
 * This function implements a blocking send operation for TclMPI.
 * The length of the data is inferred from the data object passed to this 
 * function and thus a 'count' argument is not needed.
 * In the case of tclmpi::auto, the string representation of the send data
 * is directly passed to MPI_Send() otherwise a copy is made and data converted.
 * 
 * If the MPI call failed, an MPI error message is passed up as result
 * instead and a Tcl error is indicated, otherwise nothing is returned.
 */
int TclMPI_Send(ClientData nodata, Tcl_Interp *interp,
                int objc, Tcl_Obj *const objv[])
{
    MPI_Comm comm;
    int i,dest,tag,type,len,ierr;

    if (objc != 6) {
        Tcl_WrongNumArgs(interp,1,objv,"<data> <type> <dest> <tag> <comm>");
        return TCL_ERROR;
    }

    type = tclmpi_datatype(Tcl_GetString(objv[2]));
    if (tclmpi_typecheck(interp,type,objv[0],objv[2]) != TCL_OK)
        return TCL_ERROR;

    comm = tcl2mpi_comm(Tcl_GetString(objv[5]));
    if (tclmpi_commcheck(interp,comm,objv[0],objv[5]) != TCL_OK)
        return TCL_ERROR;

    if (Tcl_GetIntFromObj(interp,objv[3],&dest) != TCL_OK)
        return TCL_ERROR;
    if (Tcl_GetIntFromObj(interp,objv[4],&tag) != TCL_OK)
        return TCL_ERROR;

    ierr = MPI_SUCCESS;

    Tcl_IncrRefCount(objv[1]);
    if (type == TCLMPI_AUTO) {
        char *idata;
        idata = Tcl_GetStringFromObj(objv[1],&len);
        ierr = MPI_Send(idata,len,MPI_CHAR,dest,tag,comm);
    } else if (type == TCLMPI_INT) {
        Tcl_Obj **ilist;
        int *idata;
        if (Tcl_ListObjGetElements(interp,objv[1],&len,&ilist) != TCL_OK)
            return TCL_ERROR;
        idata = (int *)Tcl_Alloc(len*sizeof(int));
        for (i=0; i < len; ++i)
            if (Tcl_GetIntFromObj(interp,ilist[i],idata+i) != TCL_OK) {
                Tcl_ResetResult(interp); /* this is non-fatal by choice */
                idata[i]=0; /* this is non-fatal by choice */
            }
        ierr = MPI_Send(idata,len,MPI_INT,dest,tag,comm);
        Tcl_Free((char *)idata);
    } else if (type == TCLMPI_DOUBLE) {
        Tcl_Obj **ilist;
        double *idata;
        if (Tcl_ListObjGetElements(interp,objv[1],&len,&ilist) != TCL_OK)
            return TCL_ERROR;
        idata = (double *)Tcl_Alloc(len*sizeof(double));
        for (i=0; i < len; ++i)
            if (Tcl_GetDoubleFromObj(interp,ilist[i],idata+i) != TCL_OK) {
                Tcl_ResetResult(interp); /* this is non-fatal by choice */
                idata[i]=0.0; /* this is non-fatal by choice */
            }
        ierr = MPI_Send(idata,len,MPI_DOUBLE,dest,tag,comm);
        Tcl_Free((char *)idata);
    } else {
        Tcl_DecrRefCount(objv[1]);
        Tcl_AppendResult(interp,Tcl_GetString(objv[0]),
                         ": support for data type ", Tcl_GetString(objv[2]),
                         " is not yet implemented.",NULL);
        return TCL_ERROR;
    }
    Tcl_DecrRefCount(objv[1]);

    if (tclmpi_errcheck(interp,ierr,objv[0]) != TCL_OK)
        return TCL_ERROR;

    return TCL_OK;
}



/*! wrapper for MPI_Isend() 
 * \param nodata ignored
 * \param interp current Tcl interpreter
 * \param objc number of argument objects
 * \param objv list of argument object
 * \return TCL_OK or TCL_ERROR
 *
 * This function implements a non-blocking send operation for TclMPI.
 * The length of the data is inferred from the data object passed to
 * this function and thus a 'count' argument is not needed.  Unlike for
 * the blocking TclMPI_Send, in the case of tclmpi::auto as data a
 * copy has to be made since the string representation of the send data
 * might be invalidated during the send. The command generates a new
 * tclmpi_req_t communication request via tclmpi_add_req and the
 * pointers to the data buffer and the MPI_Request info generated by
 * MPI_Isend is stored in this request list entry for later perusal, see
 * TclMPI_Wait. The generated string label representing this request
 * will be passed on to the calling program as Tcl result. If the MPI
 * call failed, an MPI error message is passed up as result instead and
 * a Tcl error is indicated.
 */
int TclMPI_Isend(ClientData nodata, Tcl_Interp *interp,
                 int objc, Tcl_Obj *const objv[])
{
    tclmpi_req_t *req;
    const char *reqlabel;
    void *data;
    MPI_Comm comm;
    int i,dest,tag,type,len,ierr;

    if (objc != 6) {
        Tcl_WrongNumArgs(interp,1,objv,"<data> <type> <dest> <tag> <comm>");
        return TCL_ERROR;
    }

    type = tclmpi_datatype(Tcl_GetString(objv[2]));
    if (tclmpi_typecheck(interp,type,objv[0],objv[2]) != TCL_OK)
        return TCL_ERROR;

    comm = tcl2mpi_comm(Tcl_GetString(objv[5]));
    if (tclmpi_commcheck(interp,comm,objv[0],objv[5]) != TCL_OK)
        return TCL_ERROR;

    if (Tcl_GetIntFromObj(interp,objv[3],&dest) != TCL_OK)
        return TCL_ERROR;
    if (Tcl_GetIntFromObj(interp,objv[4],&tag) != TCL_OK)
        return TCL_ERROR;

    reqlabel = tclmpi_add_req();
    if (reqlabel == NULL) {
        Tcl_AppendResult(interp,Tcl_GetString(objv[0]),
                         ": cannot create TclMPI request handle.",NULL);
        return TCL_ERROR;
    }
    req = tclmpi_find_req(reqlabel);
    req->type = type;
    ierr = MPI_SUCCESS;
    data = NULL;

    Tcl_IncrRefCount(objv[1]);
    if (type == TCLMPI_AUTO) {
        char *idata;
        data = Tcl_GetStringFromObj(objv[1],&len);
        idata = Tcl_Alloc(len);
        memcpy(idata,data,len);
        req->data = idata;
        ierr = MPI_Isend(idata,len,MPI_CHAR,dest,tag,comm,req->req);
        data = idata;
    } else if (type == TCLMPI_INT) {
        Tcl_Obj **ilist;
        int *idata;
        if (Tcl_ListObjGetElements(interp,objv[1],&len,&ilist) != TCL_OK)
            return TCL_ERROR;
        idata = (int *)Tcl_Alloc(len*sizeof(int));
        for (i=0; i < len; ++i)
            if (Tcl_GetIntFromObj(interp,ilist[i],idata+i) != TCL_OK) {
                Tcl_ResetResult(interp);
                idata[i]=0; /* this is non-fatal by choice */
            }
        req->data = idata;
        ierr = MPI_Isend(idata,len,MPI_INT,dest,tag,comm,req->req);
        data = idata;
    } else if (type == TCLMPI_DOUBLE) {
        Tcl_Obj **ilist;
        double *idata;
        if (Tcl_ListObjGetElements(interp,objv[1],&len,&ilist) != TCL_OK)
            return TCL_ERROR;
        idata = (double *)Tcl_Alloc(len*sizeof(double));
        for (i=0; i < len; ++i)
            if (Tcl_GetDoubleFromObj(interp,ilist[i],idata+i) != TCL_OK) {
                Tcl_ResetResult(interp);
                idata[i]=0.0; /* this is non-fatal by choice */
            }
        req->data = idata;
        ierr = MPI_Isend(idata,len,MPI_DOUBLE,dest,tag,comm,req->req);
        data = idata;
    } else {
        Tcl_DecrRefCount(objv[1]);
        Tcl_AppendResult(interp,Tcl_GetString(objv[0]),
                         ": support for data type ", Tcl_GetString(objv[2]),
                         " is not yet implemented.",NULL);
        return TCL_ERROR;
    }
    Tcl_DecrRefCount(objv[1]);

    if (tclmpi_errcheck(interp,ierr,objv[0]) != TCL_OK) {
        if (data) Tcl_Free((char *)data);
        tclmpi_del_req(req);
        return TCL_ERROR;
    }

    /* return request handle */
    Tcl_SetObjResult(interp,Tcl_NewStringObj(reqlabel,-1));
    return TCL_OK;
}


/*! wrapper for MPI_Recv()
 * \param nodata ignored
 * \param interp current Tcl interpreter
 * \param objc number of argument objects
 * \param objv list of argument object
 * \return TCL_OK or TCL_ERROR
 *
 * This function implements a blocking receive operation for TclMPI.
 * Since the length of the data object is supposed to be automatically
 * adjusted to the amount of data being sent, this function will first
 * call MPI_Probe to identify the amount of storage needed from the
 * MPI_Status object that is populated by MPI_Probe. Then a temporary
 * receive buffer is allocated and then converted back to Tcl objects
 * according to the data type passed to the receive command. Due to this
 * deviation from the MPI C bindings a 'count' argument is not needed.
 * This command returns the received data to the calling procedure. If
 * the MPI call failed, an MPI error message is passed up as result
 * instead and a Tcl error is indicated.
 */
int TclMPI_Recv(ClientData nodata, Tcl_Interp *interp,
                int objc, Tcl_Obj *const objv[])
{
    Tcl_Obj *result;
    const char *statvar;
    MPI_Comm comm;
    MPI_Status status;
    int i,source,tag,type,len,ierr;

    if ((objc < 5) || (objc > 6)) {
        Tcl_WrongNumArgs(interp,1,objv,
                         "<type> <source> <tag> <comm> ?status?");
        return TCL_ERROR;
    }

    type = tclmpi_datatype(Tcl_GetString(objv[1]));
    if (tclmpi_typecheck(interp,type,objv[0],objv[1]) != TCL_OK)
        return TCL_ERROR;

    comm = tcl2mpi_comm(Tcl_GetString(objv[4]));
    if (tclmpi_commcheck(interp,comm,objv[0],objv[4]) != TCL_OK)
        return TCL_ERROR;

    if (strcmp(Tcl_GetString(objv[2]),"tclmpi::any_source") == 0)
        source = MPI_ANY_SOURCE;
    else if (Tcl_GetIntFromObj(interp,objv[2],&source) != TCL_OK)
        return TCL_ERROR;

    if (strcmp(Tcl_GetString(objv[3]),"tclmpi::any_tag") == 0)
        tag = MPI_ANY_TAG;
    else if (Tcl_GetIntFromObj(interp,objv[3],&tag) != TCL_OK)
        return TCL_ERROR;

    if (objc > 5) statvar = Tcl_GetString(objv[5]);
    else statvar = NULL;

    ierr = MPI_SUCCESS;
    len = 0;

    if (type == TCLMPI_AUTO) {
        char *idata;
        MPI_Probe(source,tag,comm,&status);
        MPI_Get_count(&status,MPI_CHAR,&len);
        idata = Tcl_Alloc(len);
        tag = status.MPI_TAG; source = status.MPI_SOURCE;

        if (statvar != NULL) 
            ierr=MPI_Recv(idata,len,MPI_CHAR,source,tag,comm,&status);
        else 
            ierr=MPI_Recv(idata,len,MPI_CHAR,source,tag,comm,MPI_STATUS_IGNORE);

        result = Tcl_NewStringObj(idata,len);
        Tcl_Free(idata);

    } else if (type == TCLMPI_INT) {
        int *idata;
        MPI_Probe(source,tag,comm,&status);
        MPI_Get_count(&status,MPI_INT,&len);
        idata = (int *)Tcl_Alloc(len*sizeof(int));
        tag = status.MPI_TAG; source = status.MPI_SOURCE;

        if (statvar != NULL)
            ierr = MPI_Recv(idata,len,MPI_INT,source,tag,comm,&status);
        else
            ierr = MPI_Recv(idata,len,MPI_INT,source,tag,comm,MPI_STATUS_IGNORE);

        result = Tcl_NewListObj(0,NULL);
        for (i=0; i < len; ++i)
            Tcl_ListObjAppendElement(interp,result,
                                     Tcl_NewIntObj(idata[i]));
        Tcl_Free((char *)idata);

    } else if (type == TCLMPI_DOUBLE) {
        double *idata;
        MPI_Probe(source,tag,comm,&status);
        MPI_Get_count(&status,MPI_DOUBLE,&len);
        idata = (double *)Tcl_Alloc(len*sizeof(double));
        tag = status.MPI_TAG; source = status.MPI_SOURCE;

        if (statvar != NULL)
            ierr = MPI_Recv(idata,len,MPI_DOUBLE,source,tag,comm,&status);
        else
            ierr = MPI_Recv(idata,len,MPI_DOUBLE,source,tag,comm,MPI_STATUS_IGNORE);

        result = Tcl_NewListObj(0,NULL);
        for (i=0; i < len; ++i)
            Tcl_ListObjAppendElement(interp,result,
                                     Tcl_NewDoubleObj(idata[i]));
        Tcl_Free((char *)idata);
    } else {
        result = Tcl_NewListObj(0,NULL);
    }

    if (tclmpi_errcheck(interp,ierr,objv[0]) != TCL_OK)
        return TCL_ERROR;

    if (statvar != NULL) {
        Tcl_Obj *var;
        int len_char,len_int,len_double;
        MPI_Get_count(&status,MPI_CHAR,&len_char);
        MPI_Get_count(&status,MPI_INT,&len_int);
        MPI_Get_count(&status,MPI_DOUBLE,&len_double);
        Tcl_UnsetVar(interp,statvar,0);
        var = Tcl_NewStringObj(statvar,-1);
        Tcl_ObjSetVar2(interp,var,Tcl_NewStringObj("MPI_SOURCE",-1),
                       Tcl_NewIntObj(status.MPI_SOURCE),0);
        Tcl_ObjSetVar2(interp,var,Tcl_NewStringObj("MPI_TAG",-1),
                       Tcl_NewIntObj(status.MPI_TAG),0);
        Tcl_ObjSetVar2(interp,var,Tcl_NewStringObj("MPI_ERROR",-1),
                       Tcl_NewIntObj(status.MPI_ERROR),0);
        Tcl_ObjSetVar2(interp,var,Tcl_NewStringObj("COUNT_CHAR",-1),
                      Tcl_NewIntObj(len_char),0);
        Tcl_ObjSetVar2(interp,var,Tcl_NewStringObj("COUNT_INT",-1),
                      Tcl_NewIntObj(len_char),0);
        Tcl_ObjSetVar2(interp,var,Tcl_NewStringObj("COUNT_DOUBLE",-1),
                      Tcl_NewIntObj(len_char),0);
    }

    Tcl_SetObjResult(interp,result);
    return TCL_OK;
}


/*! wrapper for MPI_Iecv()
 * \param nodata ignored
 * \param interp current Tcl interpreter
 * \param objc number of argument objects
 * \param objv list of argument object
 * \return TCL_OK or TCL_ERROR
 *
 * This function implements a non-blocking receive operation for TclMPI.
 * Since the length of the data object is supposed to be automatically
 * adjusted to the amount of data being sent, this function needs to be
 * more complex than just a simple wrapper around the corresponding MPI
 * C bindings. It will first call tclmpi_add_req to generate a new entry
 * to the list of registered MPI requests. It will then call MPI_Iprobe
 * to see if a matching send is already in progress and thus the
 * necessary amount of storage required can be inferred from the
 * MPI_Status object that is populated by MPI_Iprobe. If yes, a
 * temporary receive buffer is allocated and the non-blocking receive is
 * posted and all information is transferred to the tclmpi_req_t
 * object. If not, only the arguments of the receive call are registered
 * in the request object for later use.  The command will pass the Tcl
 * string that represents the generated MPI request to the Tcl
 * interpreter as return value. If the MPI call failed, an MPI error
 * message is passed up as result instead and a Tcl error is indicated.
 */
int TclMPI_Irecv(ClientData nodata, Tcl_Interp *interp,
                 int objc, Tcl_Obj *const objv[])
{
    tclmpi_req_t *req;
    const char *reqlabel;
    void *data;
    MPI_Comm comm;
    MPI_Status status;
    int source,tag,type,pending,len,ierr;

    if ((objc < 4) || (objc > 5)) {
        Tcl_WrongNumArgs(interp,1,objv,"<type> <source> <tag> <comm>");
        return TCL_ERROR;
    }

    type = tclmpi_datatype(Tcl_GetString(objv[1]));
    if (tclmpi_typecheck(interp,type,objv[0],objv[1]) != TCL_OK)
        return TCL_ERROR;

    comm = tcl2mpi_comm(Tcl_GetString(objv[4]));
    if (tclmpi_commcheck(interp,comm,objv[0],objv[4]) != TCL_OK)
        return TCL_ERROR;

    if (strcmp(Tcl_GetString(objv[2]),"tclmpi::any_source") == 0)
        source = MPI_ANY_SOURCE;
    else if (Tcl_GetIntFromObj(interp,objv[2],&source) != TCL_OK)
        return TCL_ERROR;

    if (strcmp(Tcl_GetString(objv[3]),"tclmpi::any_tag") == 0)
        tag = MPI_ANY_TAG;
    else if (Tcl_GetIntFromObj(interp,objv[3],&tag) != TCL_OK)
        return TCL_ERROR;

    reqlabel = tclmpi_add_req();
    if (reqlabel == NULL) {
        Tcl_AppendResult(interp,Tcl_GetString(objv[0]),
                         ": cannot create TclMPI request handle.",NULL);
        return TCL_ERROR;
    }
    req = tclmpi_find_req(reqlabel);
    req->type = type;
    req->source = source;
    req->tag = tag;
    req->comm = comm;
    req->data = NULL;
    /* indicate receive */
    req->len = TCLMPI_NONE;

    ierr = MPI_SUCCESS;
    pending = len = 0;

    /* check if a matching send is already posted and ready to be received */
    ierr = MPI_Iprobe(source,tag,comm,&pending,&status);
    if (tclmpi_errcheck(interp,ierr,objv[0]) != TCL_OK) {
        tclmpi_del_req(req);
        return TCL_ERROR;
    }

    if (pending != 0) {
        if (type == TCLMPI_AUTO) {
            char *idata;
            MPI_Get_count(&status,MPI_CHAR,&len);
            idata = Tcl_Alloc(len);
            req->data = idata;
            req->len = len;
            tag = status.MPI_TAG; source = status.MPI_SOURCE;
            ierr=MPI_Irecv(idata,len,MPI_CHAR,source,tag,comm,req->req);

        } else if (type == TCLMPI_INT) {
            int *idata;
            MPI_Get_count(&status,MPI_INT,&len);
            idata = (int *)Tcl_Alloc(len*sizeof(int));
            req->data = idata;
            req->len = len;
            tag = status.MPI_TAG; source = status.MPI_SOURCE;
            ierr = MPI_Irecv(idata,len,MPI_INT,source,tag,comm,req->req);

        } else if (type == TCLMPI_DOUBLE) {
            double *idata;
            MPI_Get_count(&status,MPI_DOUBLE,&len);
            idata = (double *)Tcl_Alloc(len*sizeof(double));
            req->data = idata;
            req->len = len;
            tag = status.MPI_TAG; source = status.MPI_SOURCE;
            ierr = MPI_Irecv(idata,len,MPI_DOUBLE,source,tag,comm,req->req);
        }

        /* posting the receive failed */
        if (tclmpi_errcheck(interp,ierr,objv[0]) != TCL_OK) {
            Tcl_Free((char *)data);
            tclmpi_del_req(req);
            return TCL_ERROR;
        }
    }
    
    /* return request handle */
    Tcl_SetObjResult(interp,Tcl_NewStringObj(reqlabel,-1));
    return TCL_OK;
}


/*! wrapper for MPI_Probe()
 * \param nodata ignored
 * \param interp current Tcl interpreter
 * \param objc number of argument objects
 * \param objv list of argument object
 * \return TCL_OK or TCL_ERROR
 *
 * This function implements a blocking probe operation for TclMPI.
 * Argument flags for source, tag, and communicator are translated
 * into their native MPI equivalents and then MPI_Probe called.
 *
 * Similar to MPI_Probe, generating a status object to inspect the
 * pending receive is optional. If desired, the argument is taken
 * as a variable name which will then be generated as associative
 * array with several entries similar to what MPI_Status contains.
 * Those are source, tag, error status and count, however this is
 * directly provided as multiple entries translated to char, int
 * and double data types (COUNT_CHAR, COUNT_INT, COUNT_DOUBLE).
 */
int TclMPI_Probe(ClientData nodata, Tcl_Interp *interp,
                 int objc, Tcl_Obj *const objv[])
{
    const char *statvar;
    MPI_Comm comm;
    MPI_Status status;
    int source,tag,ierr;

    if ((objc < 4) || (objc > 5)) {
        Tcl_WrongNumArgs(interp,1,objv,"<source> <tag> <comm> ?status?");
        return TCL_ERROR;
    }

    if (strcmp(Tcl_GetString(objv[1]),"tclmpi::any_source") == 0)
        source = MPI_ANY_SOURCE;
    else if (Tcl_GetIntFromObj(interp,objv[1],&source) != TCL_OK)
        return TCL_ERROR;

    if (strcmp(Tcl_GetString(objv[2]),"tclmpi::any_tag") == 0)
        tag = MPI_ANY_TAG;
    else if (Tcl_GetIntFromObj(interp,objv[2],&tag) != TCL_OK)
        return TCL_ERROR;

    comm = tcl2mpi_comm(Tcl_GetString(objv[3]));
    if (tclmpi_commcheck(interp,comm,objv[0],objv[3]) != TCL_OK)
        return TCL_ERROR;
    if (comm == MPI_COMM_NULL) {
        Tcl_AppendResult(interp,Tcl_GetString(objv[0]),
                         ": invalid communicator: ",
                         Tcl_GetString(objv[3]),NULL);
        return TCL_ERROR;
    }

    if (objc > 4) statvar = Tcl_GetString(objv[4]);
    else statvar = NULL;

    if (statvar != NULL) {
        Tcl_Obj *var;
        int len_char,len_int,len_double;

        ierr = MPI_Probe(source,tag,comm,&status);

        MPI_Get_count(&status,MPI_CHAR,&len_char);
        MPI_Get_count(&status,MPI_INT,&len_int);
        MPI_Get_count(&status,MPI_DOUBLE,&len_double);
        Tcl_UnsetVar(interp,statvar,0);
        var = Tcl_NewStringObj(statvar,-1);
        Tcl_ObjSetVar2(interp,var,Tcl_NewStringObj("MPI_SOURCE",-1),
                       Tcl_NewIntObj(status.MPI_SOURCE),0);
        Tcl_ObjSetVar2(interp,var,Tcl_NewStringObj("MPI_TAG",-1),
                       Tcl_NewIntObj(status.MPI_TAG),0);
        Tcl_ObjSetVar2(interp,var,Tcl_NewStringObj("MPI_ERROR",-1),
                       Tcl_NewIntObj(status.MPI_ERROR),0);
        Tcl_ObjSetVar2(interp,var,Tcl_NewStringObj("COUNT_CHAR",-1),
                      Tcl_NewIntObj(len_char),0);
        Tcl_ObjSetVar2(interp,var,Tcl_NewStringObj("COUNT_INT",-1),
                      Tcl_NewIntObj(len_char),0);
        Tcl_ObjSetVar2(interp,var,Tcl_NewStringObj("COUNT_DOUBLE",-1),
                      Tcl_NewIntObj(len_char),0);
    } else ierr = MPI_Probe(source,tag,comm,MPI_STATUS_IGNORE);

    if (tclmpi_errcheck(interp,ierr,objv[0]) != TCL_OK)
        return TCL_ERROR;

    return TCL_OK;
}

/*! wrapper for MPI_Iprobe()
 * \param nodata ignored
 * \param interp current Tcl interpreter
 * \param objc number of argument objects
 * \param objv list of argument object
 * \return TCL_OK or TCL_ERROR
 *
 * This function implements a non-blocking probe operation for TclMPI.
 * Argument flags for source, tag, and communicator are translated
 * into their native MPI equivalents and then MPI_Iprobe called.
 *
 * Similar to MPI_Probe, generating a status object to inspect the
 * pending receive is optional. If desired, the argument is taken
 * as a variable name which will then be generated as associative
 * array with several entries similar to what MPI_Status contains.
 * Those are source, tag, error status and count, however this is
 * directly provided as multiple entries translated to char, int
 * and double data types (COUNT_CHAR, COUNT_INT, COUNT_DOUBLE).
 *
 * The status flag in MPI_Iprobe that returns true if a request is
 * pending will be passed to the calling routine as Tcl result.
 */
int TclMPI_Iprobe(ClientData nodata, Tcl_Interp *interp,
                 int objc, Tcl_Obj *const objv[])
{
    const char *statvar;
    MPI_Comm comm;
    MPI_Status status;
    int source,tag,ierr,pending;

    if ((objc < 4) || (objc > 5)) {
        Tcl_WrongNumArgs(interp,1,objv,"<source> <tag> <comm> ?status?");
        return TCL_ERROR;
    }

    if (strcmp(Tcl_GetString(objv[1]),"tclmpi::any_source") == 0)
        source = MPI_ANY_SOURCE;
    else if (Tcl_GetIntFromObj(interp,objv[1],&source) != TCL_OK)
        return TCL_ERROR;

    if (strcmp(Tcl_GetString(objv[2]),"tclmpi::any_tag") == 0)
        tag = MPI_ANY_TAG;
    else if (Tcl_GetIntFromObj(interp,objv[2],&tag) != TCL_OK)
        return TCL_ERROR;

    comm = tcl2mpi_comm(Tcl_GetString(objv[3]));
    if (tclmpi_commcheck(interp,comm,objv[0],objv[3]) != TCL_OK)
        return TCL_ERROR;
    if (comm == MPI_COMM_NULL) {
        Tcl_AppendResult(interp,Tcl_GetString(objv[0]),
                         ": invalid communicator: ",
                         Tcl_GetString(objv[3]),NULL);
        return TCL_ERROR;
    }

    if (objc > 4) statvar = Tcl_GetString(objv[4]);
    else statvar = NULL;
    pending = 0;

    if (statvar != NULL) {
        Tcl_Obj *var;
        int len_char,len_int,len_double;

        ierr = MPI_Iprobe(source,tag,comm,&pending,&status);

        MPI_Get_count(&status,MPI_CHAR,&len_char);
        MPI_Get_count(&status,MPI_INT,&len_int);
        MPI_Get_count(&status,MPI_DOUBLE,&len_double);
        Tcl_UnsetVar(interp,statvar,0);
        var = Tcl_NewStringObj(statvar,-1);
        Tcl_ObjSetVar2(interp,var,Tcl_NewStringObj("MPI_SOURCE",-1),
                       Tcl_NewIntObj(status.MPI_SOURCE),0);
        Tcl_ObjSetVar2(interp,var,Tcl_NewStringObj("MPI_TAG",-1),
                       Tcl_NewIntObj(status.MPI_TAG),0);
        Tcl_ObjSetVar2(interp,var,Tcl_NewStringObj("MPI_ERROR",-1),
                       Tcl_NewIntObj(status.MPI_ERROR),0);
        Tcl_ObjSetVar2(interp,var,Tcl_NewStringObj("COUNT_CHAR",-1),
                      Tcl_NewIntObj(len_char),0);
        Tcl_ObjSetVar2(interp,var,Tcl_NewStringObj("COUNT_INT",-1),
                      Tcl_NewIntObj(len_char),0);
        Tcl_ObjSetVar2(interp,var,Tcl_NewStringObj("COUNT_DOUBLE",-1),
                      Tcl_NewIntObj(len_char),0);
    } else ierr = MPI_Iprobe(source,tag,comm,&pending,MPI_STATUS_IGNORE);

    if (tclmpi_errcheck(interp,ierr,objv[0]) != TCL_OK)
        return TCL_ERROR;

    Tcl_SetObjResult(interp,Tcl_NewIntObj(pending));
    return TCL_OK;
}

/*! wrapper for MPI_Wait()
 * \param nodata ignored
 * \param interp current Tcl interpreter
 * \param objc number of argument objects
 * \param objv list of argument object
 * \return TCL_OK or TCL_ERROR
 *
 * This function implements a wrapper around MPI_Wait for TclMPI. Due to
 * the design decisions in TclMPI, it works a bit different than
 * MPI_Wait, particularly for non-blocking receive requests. As
 * explained in the TclMPI_Irecv documentation, the corresponding
 * MPI_Irecv may not yet have been posted, so we have to first inspect
 * the tclmpi_req_t object, if the receive still needs to be posted. If
 * yes, then we need to do about the same procedure as for a blocking
 * receive, i.e. call MPI_Probe to determine the size of the receive
 * buffer, allocated that buffer and the post a blocking receive. If
 * no, we call MPI_Wait to wait until the non-blocking receive is
 * completed. In both cases, the result needed to be converted to Tcl
 * objects and passed to the calling procedure as Tcl return
 * values. Then the receive buffers can be deleted and the tclmpi_req_t
 * entry removed from it translation table.
 *
 * For non-blocking send requests, MPI_Wait is called and after completion
 * the send buffer freed and the tclmpi_req_t data released.
 * The MPI spec allows to call MPI_Wait on non-existing MPI_Requests
 * and just return immediately. This is handled directly without calling
 * MPI_Wait, since we cache all generated MPI requests.
 */
int TclMPI_Wait(ClientData nodata, Tcl_Interp *interp,
                int objc, Tcl_Obj *const objv[])
{
    Tcl_Obj *result;
    const char *statvar;
    tclmpi_req_t *req;
    MPI_Status status;
    int ierr;

    if ((objc < 2) || (objc > 3)) {
        Tcl_WrongNumArgs(interp,1,objv,"<request> ?status?");
        return TCL_ERROR;
    }

    req = tclmpi_find_req(Tcl_GetString(objv[1]));
    /* waiting on an illegal request returns immediately */
    if (req == NULL) return TCL_OK;

    if (objc > 4) statvar = Tcl_GetString(objv[4]);
    else statvar = NULL;
    ierr = MPI_SUCCESS;

    /* handle non-blocking send requests */
    if (req->len == TCLMPI_INVALID) {
        if (statvar != NULL) {
            Tcl_Obj *var;
            int len_char,len_int,len_double;
            ierr = MPI_Wait(req->req,&status);

            MPI_Get_count(&status,MPI_CHAR,&len_char);
            MPI_Get_count(&status,MPI_INT,&len_int);
            MPI_Get_count(&status,MPI_DOUBLE,&len_double);
            Tcl_UnsetVar(interp,statvar,0);
            var = Tcl_NewStringObj(statvar,-1);
            Tcl_ObjSetVar2(interp,var,Tcl_NewStringObj("MPI_SOURCE",-1),
                           Tcl_NewIntObj(status.MPI_SOURCE),0);
            Tcl_ObjSetVar2(interp,var,Tcl_NewStringObj("MPI_TAG",-1),
                           Tcl_NewIntObj(status.MPI_TAG),0);
            Tcl_ObjSetVar2(interp,var,Tcl_NewStringObj("MPI_ERROR",-1),
                           Tcl_NewIntObj(status.MPI_ERROR),0);
            Tcl_ObjSetVar2(interp,var,Tcl_NewStringObj("COUNT_CHAR",-1),
                           Tcl_NewIntObj(len_char),0);
            Tcl_ObjSetVar2(interp,var,Tcl_NewStringObj("COUNT_INT",-1),
                           Tcl_NewIntObj(len_char),0);
            Tcl_ObjSetVar2(interp,var,Tcl_NewStringObj("COUNT_DOUBLE",-1),
                           Tcl_NewIntObj(len_char),0);
        } else ierr = MPI_Wait(req->req,MPI_STATUS_IGNORE);

        if (tclmpi_errcheck(interp,ierr,objv[0]) != TCL_OK)
            return TCL_ERROR;

        /* success. clean up. */
        Tcl_Free((char *)req->data);
        tclmpi_del_req(req);
        Tcl_SetResult(interp,NULL,NULL);
        return TCL_OK;

    } else {
        /* handle receive */
        int i,len,tag,source;

        /* already posted non-blocking receive */
        if (req->data != NULL) {
            ierr = MPI_Wait(req->req,&status);
            
            if (statvar != NULL) {
                Tcl_Obj *var;
                int len_char,len_int,len_double;

                MPI_Get_count(&status,MPI_CHAR,&len_char);
                MPI_Get_count(&status,MPI_INT,&len_int);
                MPI_Get_count(&status,MPI_DOUBLE,&len_double);
                Tcl_UnsetVar(interp,statvar,0);
                var = Tcl_NewStringObj(statvar,-1);
                Tcl_ObjSetVar2(interp,var,Tcl_NewStringObj("MPI_SOURCE",-1),
                               Tcl_NewIntObj(status.MPI_SOURCE),0);
                Tcl_ObjSetVar2(interp,var,Tcl_NewStringObj("MPI_TAG",-1),
                               Tcl_NewIntObj(status.MPI_TAG),0);
                Tcl_ObjSetVar2(interp,var,Tcl_NewStringObj("MPI_ERROR",-1),
                               Tcl_NewIntObj(status.MPI_ERROR),0);
                Tcl_ObjSetVar2(interp,var,Tcl_NewStringObj("COUNT_CHAR",-1),
                               Tcl_NewIntObj(len_char),0);
                Tcl_ObjSetVar2(interp,var,Tcl_NewStringObj("COUNT_INT",-1),
                               Tcl_NewIntObj(len_char),0);
                Tcl_ObjSetVar2(interp,var,Tcl_NewStringObj("COUNT_DOUBLE",-1),
                               Tcl_NewIntObj(len_char),0);
            } else ierr = MPI_Wait(req->req,MPI_STATUS_IGNORE);

            if (tclmpi_errcheck(interp,ierr,objv[0]) != TCL_OK)
                return TCL_ERROR;

            if (req->type == TCLMPI_AUTO) {
                result = Tcl_NewStringObj((const char*)req->data,req->len);

            } else if (req->type == TCLMPI_INT) {
                int *idata = (int *)req->data;
                result = Tcl_NewListObj(0,NULL);
                for (i=0; i < req->len; ++i)
                    Tcl_ListObjAppendElement(interp,result,
                                             Tcl_NewIntObj(idata[i]));
            } else if (req->type == TCLMPI_DOUBLE) {
                double *idata = (double *)req->data;
                result = Tcl_NewListObj(0,NULL);
                for (i=0; i < req->len; ++i)
                    Tcl_ListObjAppendElement(interp,result,
                                             Tcl_NewDoubleObj(idata[i]));
            } else {
                result = Tcl_NewListObj(0,NULL);
            }
        } else {

            /* receive not posted so far, we can do a blocking receive now */
            if (req->type == TCLMPI_AUTO) {
                char *idata;
                MPI_Probe(req->source,req->tag,req->comm,&status);
                MPI_Get_count(&status,MPI_CHAR,&len);
                idata = Tcl_Alloc(len);
                tag = status.MPI_TAG; source = status.MPI_SOURCE;

                if (statvar != NULL) 
                    ierr=MPI_Recv(idata,len,MPI_CHAR,source,tag,req->comm,&status);
                else 
                    ierr=MPI_Recv(idata,len,MPI_CHAR,source,tag,req->comm,MPI_STATUS_IGNORE);

                result = Tcl_NewStringObj(idata,len);
                req->data = idata;

            } else if (req->type == TCLMPI_INT) {
                int *idata;
                MPI_Probe(req->source,req->tag,req->comm,&status);
                MPI_Get_count(&status,MPI_INT,&len);
                idata = (int *)Tcl_Alloc(len*sizeof(int));
                tag = status.MPI_TAG; source = status.MPI_SOURCE;

                if (statvar != NULL)
                    ierr = MPI_Recv(idata,len,MPI_INT,source,tag,req->comm,&status);
                else
                    ierr = MPI_Recv(idata,len,MPI_INT,source,tag,req->comm,MPI_STATUS_IGNORE);

                result = Tcl_NewListObj(0,NULL);
                for (i=0; i < len; ++i)
                    Tcl_ListObjAppendElement(interp,result,
                                             Tcl_NewIntObj(idata[i]));
                req->data = idata;

            } else if (req->type == TCLMPI_DOUBLE) {
                double *idata;
                MPI_Probe(req->source,req->tag,req->comm,&status);
                MPI_Get_count(&status,MPI_DOUBLE,&len);
                idata = (double *)Tcl_Alloc(len*sizeof(double));
                tag = status.MPI_TAG; source = status.MPI_SOURCE;

                if (statvar != NULL)
                    ierr = MPI_Recv(idata,len,MPI_DOUBLE,source,tag,req->comm,&status);
                else
                    ierr = MPI_Recv(idata,len,MPI_DOUBLE,source,tag,req->comm,MPI_STATUS_IGNORE);

                result = Tcl_NewListObj(0,NULL);
                for (i=0; i < len; ++i)
                    Tcl_ListObjAppendElement(interp,result,
                                             Tcl_NewDoubleObj(idata[i]));
                req->data = idata;
            } else {
                result = Tcl_NewListObj(0,NULL);
            }

            if (tclmpi_errcheck(interp,ierr,objv[0]) != TCL_OK) {
                if (req->data) Tcl_Free((char *)req->data);
                tclmpi_del_req(req);
                return TCL_ERROR;
            }

            if (statvar != NULL) {
                Tcl_Obj *var;
                int len_char,len_int,len_double;
                MPI_Get_count(&status,MPI_CHAR,&len_char);
                MPI_Get_count(&status,MPI_INT,&len_int);
                MPI_Get_count(&status,MPI_DOUBLE,&len_double);
                Tcl_UnsetVar(interp,statvar,0);
                var = Tcl_NewStringObj(statvar,-1);
                Tcl_ObjSetVar2(interp,var,Tcl_NewStringObj("MPI_SOURCE",-1),
                               Tcl_NewIntObj(status.MPI_SOURCE),0);
                Tcl_ObjSetVar2(interp,var,Tcl_NewStringObj("MPI_TAG",-1),
                               Tcl_NewIntObj(status.MPI_TAG),0);
                Tcl_ObjSetVar2(interp,var,Tcl_NewStringObj("MPI_ERROR",-1),
                               Tcl_NewIntObj(status.MPI_ERROR),0);
                Tcl_ObjSetVar2(interp,var,Tcl_NewStringObj("COUNT_CHAR",-1),
                               Tcl_NewIntObj(len_char),0);
                Tcl_ObjSetVar2(interp,var,Tcl_NewStringObj("COUNT_INT",-1),
                               Tcl_NewIntObj(len_char),0);
                Tcl_ObjSetVar2(interp,var,Tcl_NewStringObj("COUNT_DOUBLE",-1),
                               Tcl_NewIntObj(len_char),0);
            }
            Tcl_SetObjResult(interp,result);
        }

        /* success. clean up. */
        Tcl_Free((char *)req->data);
        tclmpi_del_req(req);
        return TCL_OK;
    }
}

/*! initialize TclMPI extensions and do one time init
 * \param interp pointer to current Tcl interpreter
 *
 * This hooks up the commands provided by TclMPI into the provided
 * interpreter and also initializes the predefined communicators
 * tclmpi::comm_world, tclmpi::comm_self, and tclmpi::comm_null and its
 * corresponding MPI counterparts.
 */
static void tclmpi_init_api(Tcl_Interp *interp) 
{
    char *label;
    tclmpi_comm_t *comm;

    /* add world, self, and null communicator to translation table */
    comm = (tclmpi_comm_t *)Tcl_Alloc(sizeof(tclmpi_comm_t));
    comm->next = NULL;
    comm->valid = 1;
    comm->comm = MPI_COMM_WORLD;
    label = (char *)Tcl_Alloc(32);
    strcpy(label,"tclmpi::comm_world");
    comm->label = label;
    first_comm = comm;

    comm = (tclmpi_comm_t *)Tcl_Alloc(sizeof(tclmpi_comm_t));
    comm->next = NULL;
    comm->valid = 1;
    comm->comm = MPI_COMM_SELF;
    label = (char *)Tcl_Alloc(32);
    strcpy(label,"tclmpi::comm_self");
    comm->label = label;
    first_comm->next = comm;

    comm = (tclmpi_comm_t *)Tcl_Alloc(sizeof(tclmpi_comm_t));
    comm->next = NULL;
    comm->valid = 1;
    comm->comm = MPI_COMM_NULL;
    label = (char *)Tcl_Alloc(32);
    strcpy(label,"tclmpi::comm_null");
    comm->label = label;
    first_comm->next->next = comm;
    last_comm = comm;
    memset(&MPI_COMM_INVALID,255,sizeof(MPI_Comm));

    Tcl_CreateObjCommand(interp,"tclmpi::init",TclMPI_Init,
                         (ClientData)NULL,(Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand(interp,"tclmpi::finalize",TclMPI_Finalize,
                         (ClientData)NULL,(Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand(interp,"tclmpi::abort",TclMPI_Abort,
                         (ClientData)NULL,(Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand(interp,"tclmpi::comm_size",TclMPI_Comm_size,
                         (ClientData)NULL,(Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand(interp,"tclmpi::comm_rank",TclMPI_Comm_rank,
                         (ClientData)NULL,(Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand(interp,"tclmpi::comm_split",TclMPI_Comm_split,
                         (ClientData)NULL,(Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand(interp,"tclmpi::barrier",TclMPI_Barrier,
                         (ClientData)NULL,(Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand(interp,"tclmpi::bcast",TclMPI_Bcast,
                         (ClientData)NULL,(Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand(interp,"tclmpi::allreduce",TclMPI_Allreduce,
                         (ClientData)NULL,(Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand(interp,"tclmpi::reduce",TclMPI_Reduce,
                         (ClientData)NULL,(Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand(interp,"tclmpi::scatter",TclMPI_Scatter,
                         (ClientData)NULL,(Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand(interp,"tclmpi::gather",TclMPI_Gather,
                         (ClientData)NULL,(Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand(interp,"tclmpi::send",TclMPI_Send,
                         (ClientData)NULL,(Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand(interp,"tclmpi::isend",TclMPI_Isend,
                         (ClientData)NULL,(Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand(interp,"tclmpi::recv",TclMPI_Recv,
                         (ClientData)NULL,(Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand(interp,"tclmpi::irecv",TclMPI_Irecv,
                         (ClientData)NULL,(Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand(interp,"tclmpi::probe",TclMPI_Probe,
                         (ClientData)NULL,(Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand(interp,"tclmpi::iprobe",TclMPI_Iprobe,
                         (ClientData)NULL,(Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand(interp,"tclmpi::wait",TclMPI_Wait,
                         (ClientData)NULL,(Tcl_CmdDeleteProc*)NULL);
}

/*! register the package as a plugin with the Tcl interpreter
 * \param interp current Tcl interpreter
 * \return TCL_OK or TCL_ERROR
 *
 * This function sets up the plugin to register the various MPI wrappers
 * in this package with the Tcl interpreter.
 * 
 * Depending on the USE_TCL_STUBS define being active or not, this is
 * done using the native dynamic loader interface or the Tcl stubs 
 * interface, which would allow to load the plugin into static executables
 * and plugins from different Tcl versions.
 *
 * In addition the linked list for translating MPI communicators is 
 * initialized for the predefined communicators tclmpi::comm_world,
 * tclmpi::comm_self, and tclmpi::comm_null and its corresponding MPI
 * counterparts.
 */

#if defined(MPIWRAPSTCLDLL_EXPORTS) && defined(_WIN32)
#  undef TCL_STORAGE_CLASS
#  define TCL_STORAGE_CLASS DLLEXPORT

#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Window s headers
#include <windows.h>

BOOL APIENTRY DllMain( HANDLE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved )
{
    return TRUE;
}

EXTERN int _tclmpi_Init(Tcl_Interp *interp)

#else

int _tclmpi_Init(Tcl_Interp *interp)

#endif
{

#if defined(USE_TCL_STUBS)
    if (Tcl_InitStubs(interp, TCL_VERSION, 0) == NULL)
        return TCL_ERROR;
    if (Tcl_PkgRequire(interp, "Tcl", TCL_VERSION, 0) == NULL)
        return TCL_ERROR;
#endif
    if (Tcl_PkgProvide(interp, PACKAGE_NAME, PACKAGE_VERSION) != TCL_OK)
        return TCL_ERROR;

    tclmpi_init_api(interp);

    return TCL_OK;
}


#if defined(BUILD_TCLMPISH)
#if defined(USE_TCL_STUBS)
#error "building a static interpreter and USE_TCL_STUBS are not compatible"
#endif
/*! TclMPI shell application one time inititalization
 * \param interp pointer to current Tcl interpreter
 * \return error status
 *
 * This function does initialization calls required to get a standalone
 * Tcl interpreter application going.  It specifically includes a call
 * to Tcl_StaticPackage() declare the embedded code for the TclMPI
 * plugin as already loaded, but allow script code act as if it was not.
 */
static int tclmpi_app_init(Tcl_Interp *interp)
{
    if ((Tcl_Init)(interp) == TCL_ERROR)
        return TCL_ERROR;

    if (_tclmpi_Init(interp) == TCL_ERROR)
        return TCL_ERROR;

    Tcl_StaticPackage(interp,PACKAGE_NAME,_tclmpi_Init,NULL);

    /* use a specific profile filename */
#ifdef DJGPP
    (Tcl_SetVar)(interp, "tcl_rcFileName", "~/tclmpish.rc", TCL_GLOBAL_ONLY);
#else
    (Tcl_SetVar)(interp, "tcl_rcFileName", "~/.tclmpishrc", TCL_GLOBAL_ONLY);
#endif

    return TCL_OK;
}

/*! entry point for static tclmpish executable
 * \param argc number of elements of the argument vector
 * \param argv argument vector
 * \return executable exit status
 */
int main(int argc, char **argv)
{
    Tcl_Main(argc, argv, tclmpi_app_init);
    return 0;
}
#endif
