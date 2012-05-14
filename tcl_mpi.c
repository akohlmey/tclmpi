/***************************************************************************
 *
 * Minimalistic MPI Wrapper for Tcl
 *
 * Copyright (c) 2012 Axel Kohlmeyer <akohlmey@gmail.com>
 *
 ***************************************************************************/

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
/* we need this to detect unlisted communicators */
static MPI_Comm MPI_COMM_INVALID;

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
                return MPI_COMM_INVALID;
        } else next = next->next;
    }
    return MPI_COMM_INVALID;
}

/* add communicator to translation table, if not already present */
static const char *tclmpi_add_comm(MPI_Comm comm)
{
    tclmpi_comm_t *next;
    char *label;

    label = mpi2tcl_comm(comm);
    if (label != NULL) return label;

    next = (tclmpi_comm_t *)Tcl_Alloc(sizeof(tclmpi_comm_t));
    next->next = NULL;
    next->comm = comm;
    next->valid = 1;
    label = (char *)Tcl_Alloc(32);
    sprintf(label,"::tclmpi::comm%d",tclmpi_comm_cntr);
    next->label = label;
    ++tclmpi_comm_cntr;
    last_comm->next = next;
    last_comm = next;
    return next->label;
}

#define TCLMPI_NONE   0
#define TCLMPI_INT    1
#define TCLMPI_DOUBLE 2
#define TCLMPI_AUTO   3

/* convert string to numeric type */
static int tclmpi_datatype(const char *type)
{
    if (strcmp(type,"::tclmpi::int") == 0)
        return TCLMPI_INT;
    else if (strcmp(type,"::tclmpi::double") == 0)
        return TCLMPI_DOUBLE;
    else if (strcmp(type,"::tclmpi::auto") == 0)
        return TCLMPI_AUTO;
    else return TCLMPI_NONE;
}

/* wrapper for MPI_Init() */

int TclMPI_Init(ClientData nodata, Tcl_Interp *interp,
                int objc, Tcl_Obj *const objv[])
{
    Tcl_Obj *result,**args;
    int argc,narg,i,j;
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
    MPI_Comm comm;
    int commsize,ierr,len;

    if (objc != 2) {
        Tcl_WrongNumArgs(interp,1,objv,"<comm>");
        return TCL_ERROR;
    }

    Tcl_IncrRefCount(objv[1]);
    comm = tcl2mpi_comm(Tcl_GetString(objv[1]));

    ierr = MPI_Comm_size(comm,&commsize);

    if (ierr != MPI_SUCCESS) {
        MPI_Error_string(ierr,tclmpi_errmsg,&len);
        Tcl_AppendResult(interp,Tcl_GetString(objv[0]),": ",
                         tclmpi_errmsg,NULL);
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
    MPI_Comm comm;
    int commrank,ierr,len;

    if (objc != 2) {
        Tcl_WrongNumArgs(interp,1,objv,"<comm>");
        return TCL_ERROR;
    }

    Tcl_IncrRefCount(objv[1]);
    comm = tcl2mpi_comm(Tcl_GetString(objv[1]));

    ierr = MPI_Comm_rank(comm,&commrank);

    if (ierr != MPI_SUCCESS) {
        MPI_Error_string(ierr,tclmpi_errmsg,&len);
        Tcl_AppendResult(interp,Tcl_GetString(objv[0]),": ",
                         tclmpi_errmsg,NULL);
        Tcl_DecrRefCount(objv[1]);
        return TCL_ERROR;
    }

    result = Tcl_NewIntObj(commrank);
    Tcl_DecrRefCount(objv[1]);
    Tcl_SetObjResult(interp,result);
    return TCL_OK;
}


/* wrapper for MPI_Comm_split() */

int TclMPI_Comm_split(ClientData nodata, Tcl_Interp *interp,
                      int objc, Tcl_Obj *const objv[])
{
    Tcl_Obj *result;
    MPI_Comm comm,newcomm;
    int color,key,len,ierr;

    if (objc != 4) {
        Tcl_WrongNumArgs(interp,1,objv,"<comm> <color> <key>");
        return TCL_ERROR;
    }

    comm = tcl2mpi_comm(Tcl_GetString(objv[1]));
    if (strcmp(Tcl_GetString(objv[2]),"::tclmpi::undefined") == 0)
        color = MPI_UNDEFINED;
    else Tcl_GetIntFromObj(interp,objv[2],&color);
    Tcl_GetIntFromObj(interp,objv[3],&key);

    ierr = MPI_Comm_split(comm,color,key,&newcomm);

    if (ierr != MPI_SUCCESS) {
        MPI_Error_string(ierr,tclmpi_errmsg,&len);
        Tcl_AppendResult(interp,Tcl_GetString(objv[0]),": ",
                         tclmpi_errmsg,NULL);
        return TCL_ERROR;
    }

    result = Tcl_NewStringObj(tclmpi_add_comm(newcomm),-1);
    Tcl_SetObjResult(interp,result);
    return TCL_OK;
}


/* wrapper for MPI_Barrier() */

int TclMPI_Barrier(ClientData nodata, Tcl_Interp *interp,
                     int objc, Tcl_Obj *const objv[])
{
    MPI_Comm comm;
    int ierr,len;

    if (objc != 2) {
        Tcl_WrongNumArgs(interp,1,objv,"<comm>");
        return TCL_ERROR;
    }

    Tcl_IncrRefCount(objv[1]);
    comm = tcl2mpi_comm(Tcl_GetString(objv[1]));

    ierr = MPI_Barrier(comm);

    if (ierr != MPI_SUCCESS) {
        MPI_Error_string(ierr,tclmpi_errmsg,&len);
        Tcl_AppendResult(interp,Tcl_GetString(objv[0]),": ",
                         tclmpi_errmsg,NULL);
        Tcl_DecrRefCount(objv[1]);
        return TCL_ERROR;
    }

    Tcl_DecrRefCount(objv[1]);
    return TCL_OK;
}


/* wrapper for MPI_Bcast() */

int TclMPI_Bcast(ClientData nodata, Tcl_Interp *interp,
                 int objc, Tcl_Obj *const objv[])
{
    Tcl_Obj *result = NULL;
    void *data;
    MPI_Comm comm;
    int i,rank,root,type,len,ierr;

    if (objc != 5) {
        Tcl_WrongNumArgs(interp,1,objv,"<data> <type> <root> <comm>");
        return TCL_ERROR;
    }

    type = tclmpi_datatype(Tcl_GetString(objv[2]));
    Tcl_GetIntFromObj(interp,objv[3],&root);
    comm = tcl2mpi_comm(Tcl_GetString(objv[4]));
    ierr = MPI_SUCCESS;

    if (type == TCLMPI_NONE) {
        Tcl_AppendResult(interp,Tcl_GetString(objv[0]),
                         ": invalid data type: ",
                         Tcl_GetString(objv[2]),NULL);
        return TCL_ERROR;
    } else if (comm == MPI_COMM_NULL) {
        Tcl_AppendResult(interp,Tcl_GetString(objv[0]),
                         ": invalid communicator: ",
                         Tcl_GetString(objv[4]),NULL);
        return TCL_ERROR;
    }

    Tcl_IncrRefCount(objv[1]);
    MPI_Comm_rank(comm,&rank);

    if (type == TCLMPI_AUTO) {
        if (rank == root) {
            data = Tcl_GetStringFromObj(objv[1],&len);
            MPI_Bcast(&len,1,MPI_INT,root,comm);
            ierr = MPI_Bcast(data,len,MPI_CHAR,root,comm);
            result = Tcl_DuplicateObj(objv[1]);
        } else {
            MPI_Bcast(&len,1,MPI_INT,root,comm);
            data = Tcl_Alloc(len);
            ierr = MPI_Bcast(data,len,MPI_CHAR,root,comm);
            result = Tcl_NewStringObj((char *)data,len);
            Tcl_Free(data);
        }
    } else if (type == TCLMPI_INT) {
        Tcl_Obj **ilist;
        int *idata;
        if (rank == root) {
            if (Tcl_ListObjGetElements(interp,objv[1],&len,&ilist) != TCL_OK)
                return TCL_ERROR;
            idata = (int *)Tcl_Alloc(len*sizeof(int));
            for (i=0; i < len; ++i)
                Tcl_GetIntFromObj(interp,ilist[i],idata+i);
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
                Tcl_GetDoubleFromObj(interp,ilist[i],idata+i);
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
    }
    Tcl_DecrRefCount(objv[1]);

    if (ierr != MPI_SUCCESS) {
        MPI_Error_string(ierr,tclmpi_errmsg,&len);
        Tcl_AppendResult(interp,Tcl_GetString(objv[0]),": ",
                         tclmpi_errmsg,NULL);
        return TCL_ERROR;
    }

    if (result) Tcl_SetObjResult(interp,result);
    return TCL_OK;
}

/* wrapper for MPI_Send() */

int TclMPI_Send(ClientData nodata, Tcl_Interp *interp,
                int objc, Tcl_Obj *const objv[])
{
    const char *tclcomm;
    void *data;
    MPI_Comm comm;
    int i,dest,tag,type,len,ierr;

    if (objc != 6) {
        Tcl_WrongNumArgs(interp,1,objv,"<data> <type> <dest> <tag> <comm>");
        return TCL_ERROR;
    }

    type = tclmpi_datatype(Tcl_GetString(objv[2]));
    Tcl_GetIntFromObj(interp,objv[3],&dest);
    Tcl_GetIntFromObj(interp,objv[4],&tag);
    comm = tcl2mpi_comm(Tcl_GetString(objv[5]));
    ierr = MPI_SUCCESS;

    if (type == TCLMPI_NONE) {
        Tcl_AppendResult(interp,Tcl_GetString(objv[0]),
                         ": invalid data type: ",
                         Tcl_GetString(objv[2]),NULL);
        return TCL_ERROR;
    } else if (comm == MPI_COMM_NULL) {
        Tcl_AppendResult(interp,Tcl_GetString(objv[0]),
                         ": invalid communicator: ",
                         Tcl_GetString(objv[5]),NULL);
        return TCL_ERROR;
    }

    Tcl_IncrRefCount(objv[1]);
    if (type == TCLMPI_AUTO) {
        data = Tcl_GetStringFromObj(objv[1],&len);
        ierr = MPI_Send(data,len,MPI_CHAR,dest,tag,comm);
    } else if (type == TCLMPI_INT) {
        Tcl_Obj **ilist;
        int *idata;
        if (Tcl_ListObjGetElements(interp,objv[1],&len,&ilist) != TCL_OK)
            return TCL_ERROR;
        idata = (int *)Tcl_Alloc(len*sizeof(int));
        for (i=0; i < len; ++i)
            Tcl_GetIntFromObj(interp,ilist[i],idata+i);
        ierr = MPI_Send(idata,len,MPI_INT,dest,tag,comm);
        Tcl_Free((char *)idata);
    } else if (type == TCLMPI_DOUBLE) {
        Tcl_Obj **ilist;
        double *idata;
        if (Tcl_ListObjGetElements(interp,objv[1],&len,&ilist) != TCL_OK)
            return TCL_ERROR;
        idata = (double *)Tcl_Alloc(len*sizeof(double));
        for (i=0; i < len; ++i)
            Tcl_GetDoubleFromObj(interp,ilist[i],idata+i);
        ierr = MPI_Send(idata,len,MPI_DOUBLE,dest,tag,comm);
        Tcl_Free((char *)idata);
    }
    Tcl_DecrRefCount(objv[1]);

    if (ierr != MPI_SUCCESS) {
        MPI_Error_string(ierr,tclmpi_errmsg,&len);
        Tcl_AppendResult(interp,Tcl_GetString(objv[0]),": ",
                         tclmpi_errmsg,NULL);
        return TCL_ERROR;
    }

    return TCL_OK;
}


/* wrapper for MPI_Recv() */

int TclMPI_Recv(ClientData nodata, Tcl_Interp *interp,
                int objc, Tcl_Obj *const objv[])
{
    Tcl_Obj *list,*result,*mpistat;
    const char *tclcomm;
    char mpimsg[256];
    void *data;
    MPI_Comm comm;
    MPI_Status status;
    int i,source,tag,type,len,ierr;

    if (objc != 5) {
        Tcl_WrongNumArgs(interp,1,objv,"<type> <source> <tag> <comm>");
        return TCL_ERROR;
    }

    type = tclmpi_datatype(Tcl_GetString(objv[1]));
    if (strcmp(Tcl_GetString(objv[2]),"::tclmpi::any_source") == 0)
        source = MPI_ANY_SOURCE;
    else Tcl_GetIntFromObj(interp,objv[2],&source);
    if (strcmp(Tcl_GetString(objv[3]),"::tclmpi::any_tag") == 0)
        tag = MPI_ANY_TAG;
    else Tcl_GetIntFromObj(interp,objv[3],&tag);
    comm = tcl2mpi_comm(Tcl_GetString(objv[4]));
    ierr = MPI_SUCCESS;

    if (type == TCLMPI_NONE) {
        Tcl_AppendResult(interp,Tcl_GetString(objv[0]),
                         ": invalid data type: ",
                         Tcl_GetString(objv[1]),NULL);
        return TCL_ERROR;
    } else if (comm == MPI_COMM_NULL) {
        Tcl_AppendResult(interp,Tcl_GetString(objv[0]),
                         ": invalid communicator: ",
                         Tcl_GetString(objv[4]),NULL);
        return TCL_ERROR;
    }

    if (type == TCLMPI_AUTO) {
        MPI_Probe(source,tag,comm,&status);
        MPI_Get_count(&status,MPI_CHAR,&len);
        data = Tcl_Alloc(len);
        tag = status.MPI_TAG; source = status.MPI_SOURCE;
        MPI_Recv(data,len,MPI_CHAR,source,tag,comm,&status);
        result = Tcl_NewStringObj((char *)data,len);
        Tcl_Free(data);
    } else if (type == TCLMPI_INT) {
        int *idata;
        MPI_Probe(source,tag,comm,&status);
        MPI_Get_count(&status,MPI_INT,&len);
        idata = (int *)Tcl_Alloc(len*sizeof(int));
        tag = status.MPI_TAG; source = status.MPI_SOURCE;
        ierr = MPI_Recv(idata,len,MPI_INT,source,tag,comm,&status);
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
        ierr = MPI_Recv(idata,len,MPI_DOUBLE,source,tag,comm,&status);
        result = Tcl_NewListObj(0,NULL);
        for (i=0; i < len; ++i)
            Tcl_ListObjAppendElement(interp,result,
                                     Tcl_NewDoubleObj(idata[i]));
        Tcl_Free((char *)idata);
    } else {
        result = Tcl_NewListObj(0,NULL);
    }
    sprintf(mpimsg,"MPI_SOURCE %d MPI_TAG %d MPI_ERROR %d",
            status.MPI_SOURCE,status.MPI_TAG,status.MPI_ERROR);
    mpistat = Tcl_NewStringObj(mpimsg,-1);

    if (ierr != MPI_SUCCESS) {
        MPI_Error_string(ierr,tclmpi_errmsg,&len);
        Tcl_AppendResult(interp,Tcl_GetString(objv[0]),": ",
                         tclmpi_errmsg,NULL);
        return TCL_ERROR;
    }

    list = Tcl_NewListObj(0,NULL);
    Tcl_ListObjAppendElement(interp,list,result);
    Tcl_ListObjAppendElement(interp,list,mpistat);
    Tcl_SetObjResult(interp,list);
    return TCL_OK;
}


/* wrapper for MPI_Probe() */

int TclMPI_Probe(ClientData nodata, Tcl_Interp *interp,
                 int objc, Tcl_Obj *const objv[])
{
    Tcl_Obj *result;
    char mpimsg[384];
    MPI_Comm comm;
    MPI_Status status;
    int source,tag,len,len_char,len_int,len_double,ierr;

    if (objc != 4) {
        Tcl_WrongNumArgs(interp,1,objv,"<source> <tag> <comm>");
        return TCL_ERROR;
    }

    if (strcmp(Tcl_GetString(objv[1]),"::tclmpi::any_source") == 0)
        source = MPI_ANY_SOURCE;
    else Tcl_GetIntFromObj(interp,objv[1],&source);
    if (strcmp(Tcl_GetString(objv[2]),"::tclmpi::any_tag") == 0)
        tag = MPI_ANY_TAG;
    else Tcl_GetIntFromObj(interp,objv[2],&tag);
    comm = tcl2mpi_comm(Tcl_GetString(objv[3]));
    ierr = MPI_SUCCESS;

    if (comm == MPI_COMM_NULL) {
        Tcl_AppendResult(interp,Tcl_GetString(objv[0]),
                         ": invalid communicator: ",
                         Tcl_GetString(objv[3]),NULL);
        return TCL_ERROR;
    }

    ierr = MPI_Probe(source,tag,comm,&status);
    if (ierr != MPI_SUCCESS) {
        MPI_Error_string(ierr,tclmpi_errmsg,&len);
        Tcl_AppendResult(interp,Tcl_GetString(objv[0]),": ",
                         tclmpi_errmsg,NULL);
        return TCL_ERROR;
    }
    MPI_Get_count(&status,MPI_CHAR,&len_char);
    MPI_Get_count(&status,MPI_INT,&len_int);
    MPI_Get_count(&status,MPI_DOUBLE,&len_double);
    tag = status.MPI_TAG; source = status.MPI_SOURCE;
    sprintf(mpimsg,"MPI_SOURCE %d MPI_TAG %d MPI_ERROR %d "
            "COUNT_CHAR %d COUNT_INT %d COUNT_DOUBLE %d",
            source,tag,status.MPI_ERROR,len_char,len_int,len_double);
    result = Tcl_NewStringObj(mpimsg,-1);
    Tcl_SetObjResult(interp,result);
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
    tclmpi_comm_t *comm;

    if (Tcl_PkgProvide(interp,"tclmpi","0.2") != TCL_OK) {
        return TCL_ERROR;
    }

    /* add world, self, and null communicator to translation table */
    comm = (tclmpi_comm_t *)Tcl_Alloc(sizeof(tclmpi_comm_t));
    comm->next = NULL;
    comm->valid = 1;
    comm->comm = MPI_COMM_WORLD;
    label = (char *)Tcl_Alloc(32);
    strcpy(label,"::tclmpi::comm_world");
    comm->label = label;
    first_comm = comm;

    comm = (tclmpi_comm_t *)Tcl_Alloc(sizeof(tclmpi_comm_t));
    comm->next = NULL;
    comm->valid = 1;
    comm->comm = MPI_COMM_SELF;
    label = (char *)Tcl_Alloc(32);
    strcpy(label,"::tclmpi::comm_self");
    comm->label = label;
    first_comm->next = comm;

    comm = (tclmpi_comm_t *)Tcl_Alloc(sizeof(tclmpi_comm_t));
    comm->next = NULL;
    comm->valid = 1;
    comm->comm = MPI_COMM_NULL;
    label = (char *)Tcl_Alloc(32);
    strcpy(label,"::tclmpi::comm_null");
    comm->label = label;
    first_comm->next->next = comm;
    last_comm = comm;
    memset(&MPI_COMM_INVALID,255,sizeof(MPI_Comm));

    Tcl_CreateObjCommand(interp,"::tclmpi::init",TclMPI_Init,
                         (ClientData)NULL,(Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand(interp,"::tclmpi::finalize",TclMPI_Finalize,
                         (ClientData)NULL,(Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand(interp,"::tclmpi::comm_size",TclMPI_Comm_size,
                         (ClientData)NULL,(Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand(interp,"::tclmpi::comm_rank",TclMPI_Comm_rank,
                         (ClientData)NULL,(Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand(interp,"::tclmpi::comm_split",TclMPI_Comm_split,
                         (ClientData)NULL,(Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand(interp,"::tclmpi::barrier",TclMPI_Barrier,
                         (ClientData)NULL,(Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand(interp,"::tclmpi::bcast",TclMPI_Bcast,
                         (ClientData)NULL,(Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand(interp,"::tclmpi::send",TclMPI_Send,
                         (ClientData)NULL,(Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand(interp,"::tclmpi::recv",TclMPI_Recv,
                         (ClientData)NULL,(Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand(interp,"::tclmpi::probe",TclMPI_Probe,
                         (ClientData)NULL,(Tcl_CmdDeleteProc*)NULL);
    return TCL_OK;
}
