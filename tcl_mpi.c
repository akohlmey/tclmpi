
#include <mpi.h>
#include <tcl.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* translate communicators to Tcl string references "comm%d" */

typedef struct tclmpi_comm tclmpi_comm_t;

struct  tclmpi_comm {
    const char *label;
    MPI_Comm comm;
    int valid;
    tclmpi_comm_t *next;
};

static tclmpi_comm_t *first_comm = NULL;
static tclmpi_comm_t *last_comm = NULL;
static int tclmpi_comm_cntr = 0;

/* for error messages */
static char tclmpi_errmsg[MPI_MAX_ERROR_STRING];

/* translate MPI communicator to Tcl label */

static const char *mpi2tcl_comm(MPI_Comm comm)
{
    tclmpi_comm_t *next;

    next = first_comm;
    while (next) {
        if (comm == next->comm) {
            if (next->valid)
                return next->label;
            else
                return NULL;
        } else next = next->next;
    }
    return NULL;
}

/* translate Tcl label to MPI communicator */
static MPI_Comm tcl2mpi_comm(const char *label)
{
    tclmpi_comm_t *next;

    next = first_comm;
    while (next) {
        if (strcmp(next->label,label) == 0) {
            if (next->valid)
                return next->comm;
            else
                return MPI_COMM_NULL;
        } else next = next->next;
    }
    return MPI_COMM_NULL;
}

/* add communicator to translation table */
static const char *tclmpi_add_comm(MPI_Comm comm)
{
    tclmpi_comm_t *next;
    char *label;

    next = (tclmpi_comm_t *)malloc(sizeof(tclmpi_comm_t));
    next->next = NULL;
    next->comm = comm;
    next->valid = 1;
    label = (char *)malloc(32);
    sprintf(label,"::tclmpi::comm%d",tclmpi_comm_cntr);
    next->label = label;
    ++tclmpi_comm_cntr;
    last_comm->next = next;
    last_comm = next;
    return next->label;
}

/* wrapper for MPI_Init() */

int TclMPI_Init(ClientData nodata, Tcl_Interp *interp,
                int objc, Tcl_Obj *const objv[]) 
{
    Tcl_Obj *result, **args;
    int argc, narg, i, j;
    char **argv;

    if (objc != 2) {
        Tcl_WrongNumArgs(interp,1,objv,"<argv>");
        return TCL_ERROR;
    }

    /* convert "command line arguments" back to standard C stuff. */
    Tcl_IncrRefCount(objv[1]);
    if (Tcl_ListObjGetElements(interp,objv[1],&narg,&args) != TCL_OK) {
        return TCL_ERROR;
    }

    for (argc=0; argc < narg; ++argc) {
        Tcl_IncrRefCount(args[argc]);
        argv[argc] = Tcl_GetString(args[argc]); 
    }

    MPI_Init(&argc,&argv);

    result = Tcl_NewListObj(0,NULL);

    /* check if results are changed */
    if (argc == narg) {
        for (i=0; i < narg; ++i) {
            Tcl_ListObjAppendElement(interp,result,args[i]);
        }
    } else {
        for (i=0, j=0; (i < argc) && (i+j < narg); ++i) {
            if (argv[i] == Tcl_GetString(args[i+j])) {
                Tcl_ListObjAppendElement(interp,result,args[i+j]);
            } else {
                Tcl_DecrRefCount(args[i+j]);
                ++j;
            }
        }
    }

    Tcl_SetObjResult(interp,result);
    return TCL_OK;
}

/* wrapper for MPI_Finalize() */

int TclMPI_Finalize(ClientData nodata, Tcl_Interp *interp,
                int objc, Tcl_Obj *const objv[]) 
{
    Tcl_Obj *result;

    if (objc != 1) {
        Tcl_WrongNumArgs(interp,0,objv,NULL);
        return TCL_ERROR;
    }

    MPI_Finalize();
    result = Tcl_NewListObj(0,NULL);
    Tcl_SetObjResult(interp,result);
    return TCL_OK;
}


/* wrapper for MPI_Comm_size() */

int TclMPI_Comm_size(ClientData nodata, Tcl_Interp *interp,
                     int objc, Tcl_Obj *const objv[]) 
{
    Tcl_Obj *result;
    const char *tclcomm;
    MPI_Comm comm;
    int commsize, ierr, len;

    if (objc != 2) {
        Tcl_WrongNumArgs(interp,1,objv,"<comm>");
        return TCL_ERROR;
    }

    Tcl_IncrRefCount(objv[1]);
    tclcomm = Tcl_GetString(objv[1]); 
    comm = tcl2mpi_comm(tclcomm);

    ierr = MPI_Comm_size(comm,&commsize);

    if (ierr != MPI_SUCCESS) {
        MPI_Error_string(ierr,tclmpi_errmsg,&len);
        Tcl_AppendResult(interp,objv[0],": ",tclmpi_errmsg,NULL);
        Tcl_DecrRefCount(objv[1]);
        return TCL_ERROR;    
    }

    result = Tcl_NewIntObj(commsize);
    Tcl_DecrRefCount(objv[1]);
    Tcl_SetObjResult(interp,result);
    return TCL_OK;
}


/* wrapper for MPI_Comm_rank() */

int TclMPI_Comm_rank(ClientData nodata, Tcl_Interp *interp,
                     int objc, Tcl_Obj *const objv[]) 
{
    Tcl_Obj *result;
    const char *tclcomm;
    MPI_Comm comm;
    int commrank, ierr, len;

    if (objc != 2) {
        Tcl_WrongNumArgs(interp,1,objv,"<comm>");
        return TCL_ERROR;
    }

    Tcl_IncrRefCount(objv[1]);
    tclcomm = Tcl_GetString(objv[1]); 
    comm = tcl2mpi_comm(tclcomm);

    ierr = MPI_Comm_rank(comm,&commrank);

    if (ierr != MPI_SUCCESS) {
        MPI_Error_string(ierr,tclmpi_errmsg,&len);
        Tcl_AppendResult(interp,objv[0],": ",tclmpi_errmsg,NULL);
        Tcl_DecrRefCount(objv[1]);
        return TCL_ERROR;    
    }

    result = Tcl_NewIntObj(commrank);
    Tcl_DecrRefCount(objv[1]);
    Tcl_SetObjResult(interp,result);
    return TCL_OK;
}


/* wrapper for MPI_Barrier() */

int TclMPI_Barrier(ClientData nodata, Tcl_Interp *interp,
                     int objc, Tcl_Obj *const objv[]) 
{
    const char *tclcomm;
    MPI_Comm comm;
    int ierr, len;

    if (objc != 2) {
        Tcl_WrongNumArgs(interp,1,objv,"<comm>");
        return TCL_ERROR;
    }

    Tcl_IncrRefCount(objv[1]);
    tclcomm = Tcl_GetString(objv[1]); 
    comm = tcl2mpi_comm(tclcomm);

    ierr = MPI_Barrier(comm);

    if (ierr != MPI_SUCCESS) {
        MPI_Error_string(ierr,tclmpi_errmsg,&len);
        Tcl_AppendResult(interp,objv[0],": ",tclmpi_errmsg,NULL);
        Tcl_DecrRefCount(objv[1]);
        return TCL_ERROR;    
    }

    Tcl_DecrRefCount(objv[1]);
    return TCL_OK;
}



/* register the plugin with the tcl interpreter */
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

EXTERN int Tclmpi_Init(Tcl_Interp *interp)

#else

int Tclmpi_Init(Tcl_Interp *interp)   

#endif
{
    char *label;

    if (Tcl_PkgProvide(interp,"tclmpi","0.1") != TCL_OK) {
        return TCL_ERROR;
    }

    /* add world communicator to translation table */
    first_comm = (tclmpi_comm_t *)malloc(sizeof(tclmpi_comm_t));
    first_comm->next = NULL;
    first_comm->valid = 1;
    first_comm->comm = MPI_COMM_WORLD;
    label = (char *)malloc(32);
    strcpy(label,"::tclmpi::world");
    first_comm->label=label;
    ++tclmpi_comm_cntr;
    last_comm = first_comm;

    Tcl_CreateObjCommand(interp,"::tclmpi::init",TclMPI_Init,
                         (ClientData)NULL,(Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand(interp,"::tclmpi::finalize",TclMPI_Finalize,
                         (ClientData)NULL,(Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand(interp,"::tclmpi::comm_size",TclMPI_Comm_size,
                         (ClientData)NULL,(Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand(interp,"::tclmpi::comm_rank",TclMPI_Comm_rank,
                         (ClientData)NULL,(Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand(interp,"::tclmpi::barrier",TclMPI_Barrier,
                         (ClientData)NULL,(Tcl_CmdDeleteProc*)NULL);
    return TCL_OK;
}
