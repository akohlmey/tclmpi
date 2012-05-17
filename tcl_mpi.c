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

/* translate communicators to Tcl string references and back "::tclmpi::comm%d" */

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
    const char *oldlabel;

    oldlabel = mpi2tcl_comm(comm);
    if (oldlabel != NULL) return oldlabel;

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


/* some symbolic constants */

#define TCLMPI_INVALID   -1
#define TCLMPI_NONE       0
#define TCLMPI_AUTO       1
#define TCLMPI_INT        2
#define TCLMPI_INT_INT    3
#define TCLMPI_DOUBLE     4
#define TCLMPI_DOUBLE_INT 5

/* translate MPI requests to Tcl strings and back "::tclmpi::req%d" */

typedef struct tclmpi_req tclmpi_req_t;

struct  tclmpi_req {
    const char *label;  /* identifier of this request */
    void *data;         /* pointer to send data */
    int len;            /* size of data block */
    int type;           /* type of send data */
    int source;         /* source selector of receive */
    int tag;            /* tag selector of receive */
    MPI_Request *req;   /* MPI request handle */
    MPI_Comm comm;      /* communicator for receive */
    tclmpi_req_t *next; /* pointer to next struct */
};

static tclmpi_req_t *first_req = NULL;
static int tclmpi_req_cntr = 0;

/* allocate and add request struct to translation table */
static const char *tclmpi_add_req()
{
    tclmpi_req_t *req, *next;
    char *label;

    next = (tclmpi_req_t *)Tcl_Alloc(sizeof(tclmpi_req_t));
    if (next == NULL) return NULL;
    memset(req,0,sizeof(tclmpi_req_t));

    next->req = (MPI_Request *)Tcl_Alloc(sizeof(MPI_Request));
    if (next->req == NULL) {
        Tcl_Free(next);
        return NULL;
    }
    
    label = (char *)Tcl_Alloc(32);
    if (label == NULL) {
        Tcl_Free(next->req);
        Tcl_Free(next);
        return NULL;
    }

    sprintf(label,"::tclmpi::req%d",tclmpi_req_cntr);
    next->label = label;
    next->type = TCLMPI_NONE;
    next->len = TCLMPI_INVALID;
    ++tclmpi_req_cntr;

    if (first_req == NULL) {
        first_req = next;
    } else {
        req = first_req;
        while (req->next) req = req->next;
        req->next = next;
    }
    
    return next->label;
}

/* translate Tcl request label to request struct */
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

/* remove request struct from translation table and deallocate memory */
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
                Tcl_Free(req->label);
                Tcl_Free((char *) req->req);
                Tcl_Free((char *) req);
                return TCL_OK;
            }
            prev = prev->next;
        }
    }
    return TCL_ERROR;
}


/* convert string to numeric type */
static int tclmpi_datatype(const char *type)
{
    if (strcmp(type,"::tclmpi::int") == 0)
        return TCLMPI_INT;
    else if (strcmp(type,"::tclmpi::double") == 0)
        return TCLMPI_DOUBLE;
    else if (strcmp(type,"::tclmpi::dblint") == 0)
        return TCLMPI_DOUBLE_INT;
    else if (strcmp(type,"::tclmpi::intint") == 0)
        return TCLMPI_INT_INT;
    else if (strcmp(type,"::tclmpi::auto") == 0)
        return TCLMPI_AUTO;
    else return TCLMPI_NONE;
}


/* for error messages */
static char tclmpi_errmsg[MPI_MAX_ERROR_STRING];

/* convert MPI error code to Tcl error */
static int tclmpi_errcheck(Tcl_Interp *interp, int ierr, Tcl_Obj *obj)
{
    if (ierr != MPI_SUCCESS) {
        int len;
        MPI_Error_string(ierr,tclmpi_errmsg,&len);
        Tcl_AppendResult(interp,Tcl_GetString(obj),": ",
                         tclmpi_errmsg,NULL);
        return TCL_ERROR;
    } else return TCL_OK;
}


/* check for valid communicator */
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


/* check for valid data type */
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


/* is 1 after MPI_Init() and -1 after MPI_Finalize() */
static int tclmpi_init_done = 0;

/* wrapper for MPI_Init() */

int TclMPI_Init(ClientData nodata, Tcl_Interp *interp,
                int objc, Tcl_Obj *const objv[])
{
    Tcl_Obj *result,**args;
    int argc,narg,i,j,ierr;
    char **argv;

    if (objc != 2) {
        Tcl_WrongNumArgs(interp,1,objv,"<argv>");
        return TCL_ERROR;
    }

    /* convert "command line arguments" back to standard C stuff. */
    Tcl_IncrRefCount(objv[1]);
    Tcl_ListObjGetElements(interp,objv[1],&narg,&args);

    argv = (char **)Tcl_Alloc(narg*sizeof(char *));
    for (argc=0; argc < narg; ++argc) {
        Tcl_IncrRefCount(args[argc]);
        argv[argc] = Tcl_GetString(args[argc]);
    }

    if (tclmpi_init_done != 0) {
        Tcl_AppendResult(interp,"Calling ",Tcl_GetString(objv[0]),
                         " twice is erroneous.",NULL);
        return TCL_ERROR;
    }
    ierr = MPI_Init(&argc,&argv);
    if (tclmpi_errcheck(interp,ierr,objv[0]) != TCL_OK)
        return TCL_ERROR;
    tclmpi_init_done=1;

    /* change default error handler, so we can convert
       MPI errors into 'catch'able Tcl errors */
    MPI_Comm_set_errhandler(MPI_COMM_WORLD,MPI_ERRORS_RETURN);

    result = Tcl_NewListObj(0,NULL);

    /* check if results are changed */
    if (argc == narg) {
        for (i=0; i < narg; ++i) {
            Tcl_ListObjAppendElement(interp,result,args[i]);
            Tcl_DecrRefCount(args[i]);
        }
    } else {
        for (i=0, j=0; (i < argc) && (i+j < narg); ++i) {
            if (argv[i] == Tcl_GetString(args[i+j])) {
                Tcl_ListObjAppendElement(interp,result,args[i+j]);
                Tcl_DecrRefCount(args[i+j]);
            } else {
                Tcl_DecrRefCount(args[i+j]);
                ++j;
            }
        }
    }
    Tcl_DecrRefCount(objv[1]);
    Tcl_Free((char *)argv);
    Tcl_SetObjResult(interp,result);
    return TCL_OK;
}

/* wrapper for MPI_Finalize() */

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
    MPI_Finalize();
    tclmpi_init_done=-1;

    return TCL_OK;
}


/* wrapper for MPI_Abort() */

int TclMPI_Abort(ClientData nodata, Tcl_Interp *interp,
                 int objc, Tcl_Obj *const objv[])
{
    MPI_Comm comm;
    int ierr;

    if (objc != 3) {
        Tcl_WrongNumArgs(interp,0,objv,NULL);
        return TCL_ERROR;
    }

    if (Tcl_GetIntFromObj(interp,objv[1],&ierr) != TCL_OK)
        ierr=0;
    comm = tcl2mpi_comm(Tcl_GetString(objv[1]));
    if (tclmpi_commcheck(interp,comm,objv[0],objv[1]) != TCL_OK)
        return TCL_ERROR;

    MPI_Abort(comm,ierr);
    return TCL_OK;
}


/* wrapper for MPI_Comm_size() */

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


/* wrapper for MPI_Comm_rank() */

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


/* wrapper for MPI_Comm_split() */

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

    if (strcmp(Tcl_GetString(objv[2]),"::tclmpi::undefined") == 0)
        color = MPI_UNDEFINED;
    else Tcl_GetIntFromObj(interp,objv[2],&color);
    Tcl_GetIntFromObj(interp,objv[3],&key);

    ierr = MPI_Comm_split(comm,color,key,&newcomm);
    if (tclmpi_errcheck(interp,ierr,objv[0]) != TCL_OK)
        return TCL_ERROR;

    /* change default error handler on new communicator, so that
       we can convert MPI errors into 'catch'able Tcl errors */
    MPI_Comm_set_errhandler(newcomm,MPI_ERRORS_RETURN);

    result = Tcl_NewStringObj(tclmpi_add_comm(newcomm),-1);
    Tcl_SetObjResult(interp,result);
    return TCL_OK;
}


/* wrapper for MPI_Barrier() */

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


/* wrapper for MPI_Bcast() */

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

    Tcl_GetIntFromObj(interp,objv[3],&root);
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
                if (Tcl_GetIntFromObj(interp,ilist[i],idata+i) != TCL_OK)
                    idata[i]=0;
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
                if (Tcl_GetDoubleFromObj(interp,ilist[i],idata+i) != TCL_OK)
                    idata[i]=0.0;
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

    if (tclmpi_errcheck(interp,ierr,objv[0]) != TCL_OK)
        return TCL_ERROR;

    if (result) Tcl_SetObjResult(interp,result);
    return TCL_OK;
}

/* wrapper for MPI_Allreduce() */

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

    if (strcmp(opstr,"::tclmpi::max") == 0)
        op = MPI_MAX;
    else if (strcmp(opstr,"::tclmpi::min") == 0)
        op = MPI_MIN;
    else if (strcmp(opstr,"::tclmpi::sum") == 0)
        op = MPI_SUM;
    else if (strcmp(opstr,"::tclmpi::prod") == 0)
        op = MPI_PROD;
    else if (strcmp(opstr,"::tclmpi::land") == 0)
        op = MPI_LAND;
    else if (strcmp(opstr,"::tclmpi::band") == 0)
        op = MPI_BAND;
    else if (strcmp(opstr,"::tclmpi::lor") == 0)
        op = MPI_LOR;
    else if (strcmp(opstr,"::tclmpi::bor") == 0)
        op = MPI_BOR;
    else if (strcmp(opstr,"::tclmpi::lxor") == 0)
        op = MPI_LXOR;
    else if (strcmp(opstr,"::tclmpi::bxor") == 0)
        op = MPI_BXOR;
    else if (strcmp(opstr,"::tclmpi::maxloc") == 0)
        op = MPI_MAXLOC;
    else if (strcmp(opstr,"::tclmpi::minloc") == 0)
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
            if (Tcl_GetIntFromObj(interp,ilist[i],idata+i) != TCL_OK)
                idata[i] = 0;
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
            if (Tcl_GetDoubleFromObj(interp,ilist[i],idata+i) != TCL_OK)
                idata[i]=0.0;
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
            if (Tcl_GetIntFromObj(interp,ilist[i],idata+i) != TCL_OK)
                idata[i] = 0;
        ierr = MPI_Allreduce(idata,odata,len/2,MPI_2INT,op,comm);
        result = Tcl_NewListObj(0,NULL);
        for (i=0; i < len; ++i)
            Tcl_ListObjAppendElement(interp,result,
                                     Tcl_NewIntObj(odata[i]));
        Tcl_Free((char *)idata);
        Tcl_Free((char *)odata);    
    }
    
    Tcl_DecrRefCount(objv[1]);

    if (tclmpi_errcheck(interp,ierr,objv[0]) != TCL_OK)
        return TCL_ERROR;

    if (result) Tcl_SetObjResult(interp,result);
    return TCL_OK;
}

/* wrapper for MPI_Send() */

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

    Tcl_GetIntFromObj(interp,objv[3],&dest);
    Tcl_GetIntFromObj(interp,objv[4],&tag);

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
            if (Tcl_GetIntFromObj(interp,ilist[i],idata+i) != TCL_OK)
                idata[i] = 0;
        ierr = MPI_Send(idata,len,MPI_INT,dest,tag,comm);
        Tcl_Free((char *)idata);
    } else if (type == TCLMPI_DOUBLE) {
        Tcl_Obj **ilist;
        double *idata;
        if (Tcl_ListObjGetElements(interp,objv[1],&len,&ilist) != TCL_OK)
            return TCL_ERROR;
        idata = (double *)Tcl_Alloc(len*sizeof(double));
        for (i=0; i < len; ++i)
            if (Tcl_GetDoubleFromObj(interp,ilist[i],idata+i) != TCL_OK)
                idata[i]=0.0;
        ierr = MPI_Send(idata,len,MPI_DOUBLE,dest,tag,comm);
        Tcl_Free((char *)idata);
    }
    Tcl_DecrRefCount(objv[1]);

    if (tclmpi_errcheck(interp,ierr,objv[0]) != TCL_OK)
        return TCL_ERROR;

    return TCL_OK;
}


/* wrapper for MPI_Isend() */

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

    Tcl_GetIntFromObj(interp,objv[3],&dest);
    Tcl_GetIntFromObj(interp,objv[4],&tag);

    reqlabel = tclmpi_add_req();
    if (reqlabel == NULL) {
        Tcl_AppendResult(interp,Tcl_GetString(objv[0]),
                         ": cannot create TclMPI request handle.",NULL);
        return TCL_ERROR;
    }
    req = tclmpi_find_req(reqlabel);
    req->type = type;
    ierr = MPI_SUCCESS;

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
            if (Tcl_GetIntFromObj(interp,ilist[i],idata+i) != TCL_OK)
                idata[i] = 0;
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
            if (Tcl_GetDoubleFromObj(interp,ilist[i],idata+i) != TCL_OK)
                idata[i]=0.0;
        req->data = idata;
        ierr = MPI_Isend(idata,len,MPI_DOUBLE,dest,tag,comm,req->req);
        data = idata;
    }
    Tcl_DecrRefCount(objv[1]);

    if (tclmpi_errcheck(interp,ierr,objv[0]) != TCL_OK) {
        Tcl_Free((char *)data);
        tclmpi_del_req(req);
        return TCL_ERROR;
    }

    /* return request handle */
    Tcl_SetObjResult(interp,Tcl_NewStringObj(reqlabel,-1));
    return TCL_OK;
}


/* wrapper for MPI_Recv() */

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

    if (strcmp(Tcl_GetString(objv[2]),"::tclmpi::any_source") == 0)
        source = MPI_ANY_SOURCE;
    else if (Tcl_GetIntFromObj(interp,objv[2],&source) != TCL_OK)
        return TCL_ERROR;

    if (strcmp(Tcl_GetString(objv[3]),"::tclmpi::any_tag") == 0)
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


/* wrapper for MPI_Iecv() */

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

    if (strcmp(Tcl_GetString(objv[2]),"::tclmpi::any_source") == 0)
        source = MPI_ANY_SOURCE;
    else if (Tcl_GetIntFromObj(interp,objv[2],&source) != TCL_OK)
        return TCL_ERROR;

    if (strcmp(Tcl_GetString(objv[3]),"::tclmpi::any_tag") == 0)
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

    if (pending) {
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


/* wrapper for MPI_Probe() */

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

    if (strcmp(Tcl_GetString(objv[1]),"::tclmpi::any_source") == 0)
        source = MPI_ANY_SOURCE;
    else if (Tcl_GetIntFromObj(interp,objv[1],&source) != TCL_OK)
        return TCL_ERROR;

    if (strcmp(Tcl_GetString(objv[2]),"::tclmpi::any_tag") == 0)
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

/* wrapper for MPI_Iprobe() */

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

    if (strcmp(Tcl_GetString(objv[1]),"::tclmpi::any_source") == 0)
        source = MPI_ANY_SOURCE;
    else if (Tcl_GetIntFromObj(interp,objv[1],&source) != TCL_OK)
        return TCL_ERROR;

    if (strcmp(Tcl_GetString(objv[2]),"::tclmpi::any_tag") == 0)
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

/* wrapper for MPI_Wait() */

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
        Tcl_Free((char *) req->data);
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
                result = Tcl_NewStringObj(req->data,req->len);

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
        Tcl_Free((char *) req->data);
        tclmpi_del_req(req);
        return TCL_OK;
    }
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

    if (Tcl_PkgProvide(interp,"tclmpi","0.5") != TCL_OK) {
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
    Tcl_CreateObjCommand(interp,"::tclmpi::abort",TclMPI_Abort,
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
    Tcl_CreateObjCommand(interp,"::tclmpi::allreduce",TclMPI_Allreduce,
                         (ClientData)NULL,(Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand(interp,"::tclmpi::send",TclMPI_Send,
                         (ClientData)NULL,(Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand(interp,"::tclmpi::isend",TclMPI_Isend,
                         (ClientData)NULL,(Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand(interp,"::tclmpi::recv",TclMPI_Recv,
                         (ClientData)NULL,(Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand(interp,"::tclmpi::irecv",TclMPI_Irecv,
                         (ClientData)NULL,(Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand(interp,"::tclmpi::probe",TclMPI_Probe,
                         (ClientData)NULL,(Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand(interp,"::tclmpi::iprobe",TclMPI_Iprobe,
                         (ClientData)NULL,(Tcl_CmdDeleteProc*)NULL);
    Tcl_CreateObjCommand(interp,"::tclmpi::wait",TclMPI_Wait,
                         (ClientData)NULL,(Tcl_CmdDeleteProc*)NULL);
    return TCL_OK;
}
