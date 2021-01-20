/*------------------------------------------------------------------------------
 *
 * tcl_fftw.c++
 *
 * Glue the FFTW library to the TCL interpreter
 *
 * $Id: tcl_fftw.c++,v 1.6 2011/02/16 16:54:39 abelix Exp $
 *
 * (c)2010 Peter Schurek
 *-----------------------------------------------------------------------------*/

#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <algorithm>

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <float.h>

#include "tcl.h"
#include "tclDecls.h"
#include "fftw3.h"

#include "interpolator.h"


#ifdef EBUG
#define ENTER(fmt, ...) fprintf(stderr, "ENTER %s:" fmt "\n", __func__, ##__VA_ARGS__)
#define LEAVE(fmt, ...) fprintf(stderr, "LEAVE %s:" fmt "\n", __func__, ##__VA_ARGS__)
#define INFO(fmt, ...)  fprintf(stderr, "INFO: " fmt "\n", ##__VA_ARGS__)
#else
#define ENTER(fmt, ...)
#define LEAVE(fmt, ...)
#define INFO(fmt, ...)
#endif

//#define NS        "fftw"                             /* our Tcl namespace */
#define NS_PREFIX "fftw::"                       /* Tcl namespace prefix for command definitions */


using namespace std;            // so we don't have to add the std:: prefix everywhere


typedef struct _plan {
    int n;                      /* FFT size */
    fftw_plan plan_fwd;         /* forward transformation */
    fftw_plan plan_bwd;         /* return transformation */
    fftw_complex *in;           /* input data */
    fftw_complex *out;          /* output data */
} plan;


/**
 * implementation of the TCL command {@code fftw::plan}.
 *
 * <code>set <i>plan</i> [fftw::plan <i>size</i> <i>inplace</i>]</code>
 *
 * @param clientData pointer to client data defined with Tcl_CreateObjCommand
 * @param interp handle to TCL interpreter
 * @param objc number of elements in objv
 * @param objv arguments passed to fftw_command
 *
 * @return TCL_OK or TCL_ERROR
 */
extern "C" int
fft_plan(ClientData     clientData,
         Tcl_Interp     *interp,
         int            objc,
         Tcl_Obj *CONST objv[])
{
    int  n       = 0;
    int  inPlace = 0;
    plan *p      = NULL;

    ENTER();

    if (objc <= 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "size inplace");
        LEAVE("TCL_ERROR");
        return TCL_ERROR;
    }

    /*
     * get FFT size n
     */
    Tcl_GetIntFromObj(interp, objv[1], &n);

    if (n <= 0) {
        const char s[] = "FFT size must be > 0";
        Tcl_Obj *poResult = Tcl_NewStringObj(s, sizeof(s) - 1);
        Tcl_SetObjResult(interp, poResult);
        LEAVE("TCL_ERROR");
        return TCL_ERROR;
    }

    INFO("FFT size is %d", n);

    /*
     * get in-place flag
     */
    Tcl_GetIntFromObj(interp, objv[2], &inPlace);
    INFO("FFT is %s in-place", inPlace ? "" : "not");

    /*
     * allocate plan
     */
    if ((p = (plan *)ckalloc(sizeof(plan))) == NULL) {
        const char s[] = "cannot allocate plan";
        Tcl_Obj *poResult = Tcl_NewStringObj(s, sizeof(s) - 1);
        Tcl_SetObjResult(interp, poResult);
        LEAVE("TCL_ERROR");
        return TCL_ERROR;
    }

    p->n = n;

    INFO("allocating input%s array%s for %d points...", inPlace ? "" : "/output", inPlace ? "" : "s", n);

    /*
     * allocate in/out arrays
     */
    if ((p->in = (double (*)[2])fftw_malloc(sizeof(fftw_complex) * n)) == NULL) {
        const char s[] = "cannot allocate input array";
        Tcl_Obj *poResult = Tcl_NewStringObj(s, sizeof(s) - 1);
        Tcl_SetObjResult(interp, poResult);
        free(p);
        LEAVE("TCL_ERROR");
        return TCL_ERROR;
    }

    if (inPlace)
        p->out = p->in;
    else {
        if ((p->out = (double (*)[2])fftw_malloc(sizeof(fftw_complex) * n)) == NULL) {
            const char s[] = "cannot allocate output array";
            Tcl_Obj *poResult = Tcl_NewStringObj(s, sizeof(s) - 1);
            Tcl_SetObjResult(interp, poResult);
            fftw_free(p->in);
            free(p);
            LEAVE("TCL_ERROR");
            return TCL_ERROR;
        }
    }

    /*
     * create plans for forward/backward transformations
     */
    INFO("creating plans...");
    p->plan_fwd = fftw_plan_dft_1d(n, p->in, p->out, FFTW_FORWARD, FFTW_ESTIMATE);
    p->plan_bwd = fftw_plan_dft_1d(n, p->out, p->in, FFTW_BACKWARD, FFTW_ESTIMATE);

    INFO("returning plan %p as byte array", p);
    Tcl_Obj *poResult = Tcl_NewByteArrayObj(reinterpret_cast<const unsigned char *>(&p), sizeof(plan *));
    Tcl_SetObjResult(interp, poResult);

    LEAVE("TCL_OK");
    return TCL_OK;
}

/**
 * implementation of the TCL command {@code fftw::destroy}.
 *
 * <code>fftw::destroy <i>plan</i></code>
 *
 * @param clientData pointer to client data defined with Tcl_CreateObjCommand
 * @param interp handle to TCL interpreter
 * @param objc number of elements in objv
 * @param objv arguments passed to fftw_command
 *
 * @return TCL_OK or TCL_ERROR
 */
extern "C" int
fft_destroy(ClientData     clientData,
            Tcl_Interp     *interp,
            int            objc,
            Tcl_Obj *CONST objv[])
{
    plan **pp   = NULL;
    plan *p     = NULL;
    int  length = 0;

    ENTER();

    if (objc <= 1) {
        Tcl_WrongNumArgs(interp, 1, objv, "plan");
        LEAVE("TCL_ERROR");
        return TCL_ERROR;
    }

    /*
     * get pointer to FFT plan
     */
    pp = reinterpret_cast<plan **>(Tcl_GetByteArrayFromObj(objv[1], &length));

    if (length != sizeof(*pp)) {
        const char s[] = "need proper plan";
        Tcl_Obj *poResult = Tcl_NewStringObj(s, sizeof(s) - 1);
        Tcl_SetObjResult(interp, poResult);
        LEAVE("TCL_ERROR");
        return TCL_ERROR;
    }

    p = *pp;
    INFO("plan is %p", p);

    /* free in/out arrays */
    if (p->in == p->out)
        fftw_free(p->in);
    else {
        fftw_free(p->in);
        fftw_free(p->out);
    }

    /* free plan */
    ckfree(reinterpret_cast<char *>(p));

    LEAVE("TCL_OK");
    return TCL_OK;
}

/**
 * implementation of the TCL command {@code fftw::cleanup}.
 *
 * <code>fftw::cleanup</code>
 *
 * @param clientData pointer to client data defined with Tcl_CreateObjCommand
 * @param interp handle to TCL interpreter
 * @param objc number of elements in objv
 * @param objv arguments passed to fftw_command
 *
 * @return TCL_OK or TCL_ERROR
 */
extern "C" int
fft_cleanup(ClientData     clientData,
            Tcl_Interp     *interp,
            int            objc,
            Tcl_Obj *CONST objv[])
{
    ENTER();

    /* cleanup */
    fftw_cleanup();

    LEAVE("TCL_OK");
    return TCL_OK;
}

/**
 * implementation of the TCL command {@code fftw::execute}.
 *
 * <code>fftw::execute <i>plan</i> <i>dir</i></code>
 *
 * <i>Dir</i> selects forward(0)/backwards(1) transformation
 *
 * @param clientData pointer to client data defined with Tcl_CreateObjCommand
 * @param interp handle to TCL interpreter
 * @param objc number of elements in objv
 * @param objv arguments passed to fftw_command
 *
 * @return TCL_OK or TCL_ERROR
 */
extern "C" int
fft_execute(ClientData     clientData,
            Tcl_Interp     *interp,
            int            objc,
            Tcl_Obj *CONST objv[])
{
    plan **pp   = NULL;
    plan *p     = NULL;
    int  length = 0;
    int  dir    = 0;

    ENTER();

    if (objc <= 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "plan dir");
        LEAVE("TCL_ERROR");
        return TCL_ERROR;
    }

    /*
     * get pointer to FFT plan
     */
    pp = reinterpret_cast<plan **>(Tcl_GetByteArrayFromObj(objv[1], &length));

    if (length != sizeof(*pp)) {
        const char s[] = "need proper plan";
        Tcl_Obj *poResult = Tcl_NewStringObj(s, sizeof(s) - 1);
        Tcl_SetObjResult(interp, poResult);
        LEAVE("TCL_ERROR");
        return TCL_ERROR;
    }

    p = *pp;
    INFO("plan is %p", p);

    /*
     * get direction
     */
    Tcl_GetIntFromObj(interp, objv[2], &dir);

    /*
     * execute FFT
     */
    INFO("executing %sFFT for plan %p...", (dir ? "I" : ""), p);

    if (dir) {
        fftw_execute(p->plan_bwd);
    } else {
        fftw_execute(p->plan_fwd);
    }

    LEAVE("TCL_OK");
    return TCL_OK;
}

/**
 * implementation of the TCL command {@code fftw::clear}.
 *
 * <code>fftw::clear <i>plan</i></code>
 *
 * @param clientData pointer to client data defined with Tcl_CreateObjCommand
 * @param interp handle to TCL interpreter
 * @param objc number of elements in objv
 * @param objv arguments passed to fftw_command
 *
 * @return TCL_OK or TCL_ERROR
 */
extern "C" int
fft_clear(ClientData     clientData,
          Tcl_Interp     *interp,
          int            objc,
          Tcl_Obj *CONST objv[])
{
    plan **pp   = NULL;
    plan *p     = NULL;
    int  length = 0;

    ENTER();

    if (objc <= 1) {
        Tcl_WrongNumArgs(interp, 1, objv, "plan");
        LEAVE("TCL_ERROR");
        return TCL_ERROR;
    }

    /*
     * get pointer to FFT plan
     */
    pp = reinterpret_cast<plan **>(Tcl_GetByteArrayFromObj(objv[1], &length));

    if (length != sizeof(*pp)) {
        const char s[] = "need proper plan";
        Tcl_Obj *poResult = Tcl_NewStringObj(s, sizeof(s) - 1);
        Tcl_SetObjResult(interp, poResult);
        LEAVE("TCL_ERROR");
        return TCL_ERROR;
    }

    p = *pp;
    INFO("plan is %p", p);

    /*
     * clear the input/output arrays
     */
    INFO("clearing in/out arrays...");
    memset(p->in, 0, sizeof(fftw_complex) * p->n);

    if (p->in != p->out)
        memset(p->out, 0, sizeof(fftw_complex) * p->n);

    LEAVE("TCL_OK");
    return TCL_OK;
}

/**
 * implementation of the TCL command {@code fftw::scale}.
 *
 * <code>fftw::scale <i>plan</i> <i>dir</i> <i>maxScale</i></code>
 *
 * @param clientData pointer to client data defined with Tcl_CreateObjCommand
 * @param interp handle to TCL interpreter
 * @param objc number of elements in objv
 * @param objv arguments passed to fftw_command
 *
 * @return TCL_OK or TCL_ERROR
 */
extern "C" int
fft_scale(ClientData     clientData,
          Tcl_Interp     *interp,
          int            objc,
          Tcl_Obj *CONST objv[])
{
    plan   **pp     = NULL;
    plan   *p       = NULL;
    int    length   = 0;
    int    dir      = 0;
    double scaleMax = 0.;

    ENTER();

    if (objc <= 3) {
        Tcl_WrongNumArgs(interp, 1, objv, "plan dir max");
        LEAVE("TCL_ERROR");
        return TCL_ERROR;
    }

    /*
     * get pointer to FFT plan
     */
    pp = reinterpret_cast<plan **>(Tcl_GetByteArrayFromObj(objv[1], &length));

    if (length != sizeof(*pp)) {
        const char s[] = "need proper plan";
        Tcl_Obj *poResult = Tcl_NewStringObj(s, sizeof(s) - 1);
        Tcl_SetObjResult(interp, poResult);
        LEAVE("TCL_ERROR");
        return TCL_ERROR;
    }

    p = *pp;
    INFO("plan is %p", p);

    /*
     * get direction
     */
    Tcl_GetIntFromObj(interp, objv[2], &dir);
    INFO("direction is %d", dir);

    fftw_complex *pFftOut = dir ? p->in : p->out;

    /*
     * get max scale
     */
    Tcl_GetDoubleFromObj(interp, objv[3], &scaleMax);
    INFO("scaleMax is %lf", scaleMax);

    /*
     * search absolute max, and compute scale factor
     */
    fftw_complex *pCx        = pFftOut;
    double       absMax      = 0.;
    double       scaleFactor = 1.;

    for (; pCx - pFftOut < p->n; pCx++) {
        if (fabs((*pCx)[0]) > absMax)
            absMax = fabs((*pCx)[0]);

        if (fabs((*pCx)[1]) > absMax)
            absMax = fabs((*pCx)[1]);
    }

    if (absMax > DBL_MIN)
        scaleFactor = scaleMax / absMax;

    INFO("scale factor is %lf", scaleFactor);

    /*
     * rescale output with current scale factor
     */
    for (pCx = pFftOut; pCx - pFftOut < p->n; pCx++) {
        (*pCx)[0] *= scaleFactor;
        (*pCx)[1] *= scaleFactor;
    }

    INFO("returning scale factor %lf", scaleFactor);
    Tcl_Obj *poResult = Tcl_NewDoubleObj(scaleFactor);
    Tcl_SetObjResult(interp, poResult);

    LEAVE("TCL_OK");
    return TCL_OK;
}

/**
 * implementation of the TCL command {@code fftw::set}.
 *
 * <code>fftw::set <i>plan</i> <i>dir</i> <i>ix</i> <i>{ {re im}+ }</i></code>
 *
 * @param clientData pointer to client data defined with Tcl_CreateObjCommand
 * @param interp handle to TCL interpreter
 * @param objc number of elements in objv
 * @param objv arguments passed to fftw_command
 *
 * @return TCL_OK or TCL_ERROR
 */
extern "C" int
fft_set(ClientData     clientData,
        Tcl_Interp     *interp,
        int            objc,
        Tcl_Obj *CONST objv[])
{
    plan **pp   = NULL;
    plan *p     = NULL;
    int  length = 0;
    int  dir    = 0;
    int  ix     = 0;

    ENTER();

    if (objc <= 4) {
        Tcl_WrongNumArgs(interp, 1, objv, "plan dir ix { {re im}+ }");
        LEAVE("TCL_ERROR");
        return TCL_ERROR;
    }

    /*
     * get pointer to FFT plan
     */
    pp = reinterpret_cast<plan **>(Tcl_GetByteArrayFromObj(objv[1], &length));

    if (length != sizeof(*pp)) {
        const char s[] = "need proper plan";
        Tcl_Obj *poResult = Tcl_NewStringObj(s, sizeof(s) - 1);
        Tcl_SetObjResult(interp, poResult);
        LEAVE("TCL_ERROR");
        return TCL_ERROR;
    }

    p = *pp;
    INFO("plan is %p", p);

    /*
     * get direction
     */
    Tcl_GetIntFromObj(interp, objv[2], &dir);
    INFO("direction is %d", dir);

    /*
     * get start index
     */
    Tcl_GetIntFromObj(interp, objv[3], &ix);
    INFO("start index is %d", ix);

    if ((ix < 0) || (ix >= p->n)) {
        const char s[] = "invalid start index";
        Tcl_Obj *poResult = Tcl_NewStringObj(s, sizeof(s) - 1);
        Tcl_SetObjResult(interp, poResult);
        LEAVE("TCL_ERROR");
        return TCL_ERROR;
    }

    /*
     * get re/im list as array
     */
    int     lenReIm;
    Tcl_Obj **pReIm;

    if (Tcl_ListObjGetElements(interp, objv[4], &lenReIm, &pReIm) != TCL_OK) {
        LEAVE("TCL_ERROR");
        return TCL_ERROR;
    }

    INFO("setting %d points in plan", lenReIm);

    fftw_complex *pFftInput = dir ? p->out + ix : p->in + ix;

    while ((ix++ < p->n) && (lenReIm-- > 0)) {
        Tcl_Obj *pRe = NULL;
        Tcl_Obj *pIm = NULL;

        /*
         * Read re/im as list
         */
        if (Tcl_ListObjIndex(interp, pReIm[0], 0, &pRe) != TCL_OK) {
            LEAVE("TCL_ERROR");
            return TCL_ERROR;
        }

        if (Tcl_ListObjIndex(interp, pReIm[0], 1, &pIm) != TCL_OK) {
            LEAVE("TCL_ERROR");
            return TCL_ERROR;
        }

        if ((pRe == NULL) || (pIm == NULL)) {
            const char s[] = "invalid FFT point list";
            Tcl_Obj *poResult = Tcl_NewStringObj(s, sizeof(s) - 1);
            Tcl_SetObjResult(interp, poResult);
            LEAVE("TCL_ERROR");
            return TCL_ERROR;
        }

        /*
         * write into input array
         */
        if (Tcl_GetDoubleFromObj(interp, pRe, &((*pFftInput)[0])) != TCL_OK) {
            const char s[] = "invalid real part";
            Tcl_Obj *poResult = Tcl_NewStringObj(s, sizeof(s) - 1);
            Tcl_SetObjResult(interp, poResult);
            LEAVE("TCL_ERROR");
            return TCL_ERROR;
        }

        if (Tcl_GetDoubleFromObj(interp, pIm, &((*pFftInput)[1])) != TCL_OK) {
            const char s[] = "invalid imaginary part";
            Tcl_Obj *poResult = Tcl_NewStringObj(s, sizeof(s) - 1);
            Tcl_SetObjResult(interp, poResult);
            LEAVE("TCL_ERROR");
            return TCL_ERROR;
        }

        INFO("wrote %.2lf + j%.2lf", (*pFftInput)[0], (*pFftInput)[1]);

        /*
         * advance pointers
         */
        ++pFftInput;
        ++pReIm;
    }

    LEAVE("TCL_OK");
    return TCL_OK;
}

/**
 * implementation of the TCL command {@code fftw::setAt}.
 *
 * <code>fftw::setAt <i>plan</i> <i>dir</i> <i>ix</i> <i>{re im}</i></code>
 *
 * @param clientData pointer to client data defined with Tcl_CreateObjCommand
 * @param interp handle to TCL interpreter
 * @param objc number of elements in objv
 * @param objv arguments passed to fftw_command
 *
 * @return TCL_OK or TCL_ERROR
 */
extern "C" int
fft_setAt(ClientData     clientData,
          Tcl_Interp     *interp,
          int            objc,
          Tcl_Obj *CONST objv[])
{
    plan **pp   = NULL;
    plan *p     = NULL;
    int  length = 0;
    int  dir    = 0;
    int  ix     = 0;

    ENTER();

    if (objc <= 4) {
        Tcl_WrongNumArgs(interp, 1, objv, "plan dir ix {re im}");
        LEAVE("TCL_ERROR");
        return TCL_ERROR;
    }

    /*
     * get pointer to FFT plan
     */
    pp = reinterpret_cast<plan **>(Tcl_GetByteArrayFromObj(objv[1], &length));

    if (length != sizeof(*pp)) {
        const char s[] = "need proper plan";
        Tcl_Obj *poResult = Tcl_NewStringObj(s, sizeof(s) - 1);
        Tcl_SetObjResult(interp, poResult);
        LEAVE("TCL_ERROR");
        return TCL_ERROR;
    }

    p = *pp;
    INFO("plan is %p", p);

    /*
     * get direction
     */
    Tcl_GetIntFromObj(interp, objv[2], &dir);
    INFO("direction is %d", dir);

    /*
     * get point index
     */
    Tcl_GetIntFromObj(interp, objv[3], &ix);
    INFO("index is %d", ix);

    if ((ix < 0) || (ix >= p->n)) {
        const char s[] = "invalid start index";
        Tcl_Obj *poResult = Tcl_NewStringObj(s, sizeof(s) - 1);
        Tcl_SetObjResult(interp, poResult);
        LEAVE("TCL_ERROR");
        return TCL_ERROR;
    }

    /*
     * get re/im pair as array
     */
    int     lenReIm;
    Tcl_Obj **pReIm;

    if (Tcl_ListObjGetElements(interp, objv[4], &lenReIm, &pReIm) != TCL_OK) {
        LEAVE("TCL_ERROR");
        return TCL_ERROR;
    }

    if (lenReIm != 2) {
        const char s[] = "expected {re im}";
        Tcl_Obj *poResult = Tcl_NewStringObj(s, sizeof(s) - 1);
        Tcl_SetObjResult(interp, poResult);
        LEAVE("TCL_ERROR");
        return TCL_ERROR;
    }

    /*
     * goto selected point in in-memory array
     */
    fftw_complex *pFftInput = dir ? p->out + ix : p->in + ix;

    /*
     * write into input array
     */
    if (Tcl_GetDoubleFromObj(interp, pReIm[0], &((*pFftInput)[0])) != TCL_OK) {
        const char s[] = "invalid real part";
        Tcl_Obj *poResult = Tcl_NewStringObj(s, sizeof(s) - 1);
        Tcl_SetObjResult(interp, poResult);
        LEAVE("TCL_ERROR");
        return TCL_ERROR;
    }

    if (Tcl_GetDoubleFromObj(interp, pReIm[1], &((*pFftInput)[1])) != TCL_OK) {
        const char s[] = "invalid imaginary part";
        Tcl_Obj *poResult = Tcl_NewStringObj(s, sizeof(s) - 1);
        Tcl_SetObjResult(interp, poResult);
        LEAVE("TCL_ERROR");
        return TCL_ERROR;
    }

    INFO("wrote %.2lf + j%.2lf", (*pFftInput)[0], (*pFftInput)[1]);

    LEAVE("TCL_OK");
    return TCL_OK;
}

/**
 * implementation of the TCL command {@code fftw::get}.
 *
 * <code>fftw::get <i>plan</i> <i>dir</i> <i>ix</i></code>
 * <code>fftw::get <i>plan</i> <i>dir</i></code>
 *
 * @param clientData pointer to client data defined with Tcl_CreateObjCommand
 * @param interp handle to TCL interpreter
 * @param objc number of elements in objv
 * @param objv arguments passed to fftw_command
 *
 * @return TCL_OK or TCL_ERROR
 */
extern "C" int
fft_get(ClientData     clientData,
        Tcl_Interp     *interp,
        int            objc,
        Tcl_Obj *CONST objv[])
{
    plan **pp   = NULL;
    plan *p     = NULL;
    int  length = 0;
    int  dir    = 0;
    int  ix     = 0;

    ENTER();

    if (objc <= 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "plan dir [ix]");
        LEAVE("TCL_ERROR");
        return TCL_ERROR;
    }

    /*
     * get pointer to FFT plan
     */
    pp = reinterpret_cast<plan **>(Tcl_GetByteArrayFromObj(objv[1], &length));

    if (length != sizeof(*pp)) {
        const char s[] = "need proper plan";
        Tcl_Obj *poResult = Tcl_NewStringObj(s, sizeof(s) - 1);
        Tcl_SetObjResult(interp, poResult);
        LEAVE("TCL_ERROR");
        return TCL_ERROR;
    }

    p = *pp;
    INFO("plan is %p", p);

    /*
     * get direction
     */
    Tcl_GetIntFromObj(interp, objv[2], &dir);
    INFO("direction is %d", dir);

    /*
     * get optional start index
     */
    if (objc >=4)
        Tcl_GetIntFromObj(interp, objv[3], &ix);

    if ((ix < 0) || (ix >= p->n)) {
        const char s[] = "invalid start index";
        Tcl_Obj *poResult = Tcl_NewStringObj(s, sizeof(s) - 1);
        Tcl_SetObjResult(interp, poResult);
        LEAVE("TCL_ERROR");
        return TCL_ERROR;
    }

    INFO("start index is %d", ix);

    /*
     * go to selected start in in-memory array and open a new Tcl list
     * for the result
     */
    fftw_complex *pFftInput   = dir ? p->in + ix : p->out + ix;
    Tcl_Obj      *pResultList = Tcl_NewListObj(0, NULL);

    if (pResultList == NULL) {
        const char s[] = "cannot allocate result list";
        Tcl_Obj *poResult = Tcl_NewStringObj(s, sizeof(s) - 1);
        Tcl_SetObjResult(interp, poResult);
        LEAVE("TCL_ERROR");
        return TCL_ERROR;
    }

    INFO("appending %d points", p->n - ix);

    while (ix++ < p->n) {
        Tcl_Obj *pRe    = Tcl_NewDoubleObj((*pFftInput)[0]);
        Tcl_Obj *pIm    = Tcl_NewDoubleObj((*pFftInput)[1]);
        Tcl_Obj *pCmplx = Tcl_NewListObj(0, NULL);

        if ((pRe == NULL) || (pIm == NULL) || (pCmplx == NULL)) {
            const char s[] = "cannot allocate FFT point";
            Tcl_Obj *poResult = Tcl_NewStringObj(s, sizeof(s) - 1);
            Tcl_SetObjResult(interp, poResult);
            LEAVE("TCL_ERROR");
            return TCL_ERROR;
        }

        if ((Tcl_ListObjAppendElement(interp, pCmplx, pRe) != TCL_OK) ||
            (Tcl_ListObjAppendElement(interp, pCmplx, pIm) != TCL_OK)) {
            LEAVE("TCL_ERROR");
            return TCL_ERROR;
        }

        if (Tcl_ListObjAppendElement(interp, pResultList, pCmplx) != TCL_OK) {
            LEAVE("TCL_ERROR");
            return TCL_ERROR;
        }

        INFO("appended {%.2lf + j%.2lf}", (*pFftInput)[0], (*pFftInput)[1]);

        /*
         * advance pointer
         */
        ++pFftInput;
    }

    INFO("returning complex list");
    Tcl_SetObjResult(interp, pResultList);

    LEAVE("TCL_OK");
    return TCL_OK;
}

/**
 * implementation of the TCL command {@code fftw::getFlat}.
 *
 * <code>fftw::getFlat <i>plan</i> <i>dir</i> <i>ix</i></code>
 * <code>fftw::getFlat <i>plan</i> <i>dir</i></code>
 *
 * @param clientData pointer to client data defined with Tcl_CreateObjCommand
 * @param interp handle to TCL interpreter
 * @param objc number of elements in objv
 * @param objv arguments passed to fftw_command
 *
 * @return TCL_OK or TCL_ERROR
 */
extern "C" int
fft_getFlat(ClientData     clientData,
            Tcl_Interp     *interp,
            int            objc,
            Tcl_Obj *CONST objv[])
{
    plan **pp   = NULL;
    plan *p     = NULL;
    int  length = 0;
    int  dir    = 0;
    int  ix     = 0;

    ENTER();

    if (objc <= 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "plan dir [ix]");
        LEAVE("TCL_ERROR");
        return TCL_ERROR;
    }

    /*
     * get pointer to FFT plan
     */
    pp = reinterpret_cast<plan **>(Tcl_GetByteArrayFromObj(objv[1], &length));

    if (length != sizeof(*pp)) {
        const char s[] = "need proper plan";
        Tcl_Obj *poResult = Tcl_NewStringObj(s, sizeof(s) - 1);
        Tcl_SetObjResult(interp, poResult);
        LEAVE("TCL_ERROR");
        return TCL_ERROR;
    }

    p = *pp;
    INFO("plan is %p", p);

    /*
     * get direction
     */
    Tcl_GetIntFromObj(interp, objv[2], &dir);
    INFO("direction is %d", dir);

    /*
     * get optional start index
     */
    if (objc >=4)
        Tcl_GetIntFromObj(interp, objv[3], &ix);

    if ((ix < 0) || (ix >= p->n)) {
        const char s[] = "invalid start index";
        Tcl_Obj *poResult = Tcl_NewStringObj(s, sizeof(s) - 1);
        Tcl_SetObjResult(interp, poResult);
        LEAVE("TCL_ERROR");
        return TCL_ERROR;
    }

    INFO("start index is %d", ix);

    /*
     * go to selected start in in-memory array and open a new Tcl list
     * for the result
     */
    fftw_complex *pFftInput   = dir ? p->in + ix : p->out + ix;
    Tcl_Obj      *pResultList = Tcl_NewListObj(0, NULL);

    if (pResultList == NULL) {
        const char s[] = "cannot allocate result list";
        Tcl_Obj *poResult = Tcl_NewStringObj(s, sizeof(s) - 1);
        Tcl_SetObjResult(interp, poResult);
        LEAVE("TCL_ERROR");
        return TCL_ERROR;
    }

    INFO("appending %d points", p->n - ix);

    while (ix++ < p->n) {
        Tcl_Obj *pRe    = Tcl_NewDoubleObj((*pFftInput)[0]);
        Tcl_Obj *pIm    = Tcl_NewDoubleObj((*pFftInput)[1]);

        if ((pRe == NULL) || (pIm == NULL)) {
            const char s[] = "cannot allocate FFT point";
            Tcl_Obj *poResult = Tcl_NewStringObj(s, sizeof(s) - 1);
            Tcl_SetObjResult(interp, poResult);
            LEAVE("TCL_ERROR");
            return TCL_ERROR;
        }

        if ((Tcl_ListObjAppendElement(interp, pResultList, pRe) != TCL_OK) ||
            (Tcl_ListObjAppendElement(interp, pResultList, pIm) != TCL_OK)) {
            LEAVE("TCL_ERROR");
            return TCL_ERROR;
        }

        INFO("appended {%.2lf + j%.2lf}", (*pFftInput)[0], (*pFftInput)[1]);

        /*
         * advance pointer
         */
        ++pFftInput;
    }

    INFO("returning flat list");
    Tcl_SetObjResult(interp, pResultList);

    LEAVE("TCL_OK");
    return TCL_OK;
}

/**
 * implementation of the TCL command {@code fftw::getFlatInt}.
 *
 * <code>fftw::getFlatInt <i>plan</i> <i>dir</i> <i>ix</i></code>
 * <code>fftw::getFlatInt <i>plan</i> <i>dir</i></code>
 *
 * @param clientData pointer to client data defined with Tcl_CreateObjCommand
 * @param interp handle to TCL interpreter
 * @param objc number of elements in objv
 * @param objv arguments passed to fftw_command
 *
 * @return TCL_OK or TCL_ERROR
 */
extern "C" int
fft_getFlatInt(ClientData     clientData,
               Tcl_Interp     *interp,
               int            objc,
               Tcl_Obj *CONST objv[])
{
    plan **pp   = NULL;
    plan *p     = NULL;
    int  length = 0;
    int  dir    = 0;
    int  ix     = 0;

    ENTER();

    if (objc <= 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "plan dir [ix]");
        LEAVE("TCL_ERROR");
        return TCL_ERROR;
    }

    /*
     * get pointer to FFT plan
     */
    pp = reinterpret_cast<plan **>(Tcl_GetByteArrayFromObj(objv[1], &length));

    if (length != sizeof(*pp)) {
        const char s[] = "need proper plan";
        Tcl_Obj *poResult = Tcl_NewStringObj(s, sizeof(s) - 1);
        Tcl_SetObjResult(interp, poResult);
        LEAVE("TCL_ERROR");
        return TCL_ERROR;
    }

    p = *pp;
    INFO("plan is %p", p);

    /*
     * get direction
     */
    Tcl_GetIntFromObj(interp, objv[2], &dir);
    INFO("direction is %d", dir);

    /*
     * get optional start index
     */
    if (objc >=4)
        Tcl_GetIntFromObj(interp, objv[3], &ix);

    if ((ix < 0) || (ix >= p->n)) {
        const char s[] = "invalid start index";
        Tcl_Obj *poResult = Tcl_NewStringObj(s, sizeof(s) - 1);
        Tcl_SetObjResult(interp, poResult);
        LEAVE("TCL_ERROR");
        return TCL_ERROR;
    }

    INFO("start index is %d", ix);

    /*
     * go to selected start in in-memory array and open a new Tcl list
     * for the result
     */
    fftw_complex *pFftInput   = dir ? p->in + ix : p->out + ix;
    Tcl_Obj      *pResultList = Tcl_NewListObj(0, NULL);

    if (pResultList == NULL) {
        const char s[] = "cannot allocate result list";
        Tcl_Obj *poResult = Tcl_NewStringObj(s, sizeof(s) - 1);
        Tcl_SetObjResult(interp, poResult);
        LEAVE("TCL_ERROR");
        return TCL_ERROR;
    }

    INFO("appending %d points", p->n - ix);

    while (ix++ < p->n) {
        Tcl_Obj *pRe    = Tcl_NewIntObj((int)round((*pFftInput)[0]));
        Tcl_Obj *pIm    = Tcl_NewIntObj((int)round((*pFftInput)[1]));

        if ((pRe == NULL) || (pIm == NULL)) {
            const char s[] = "cannot allocate FFT point";
            Tcl_Obj *poResult = Tcl_NewStringObj(s, sizeof(s) - 1);
            Tcl_SetObjResult(interp, poResult);
            LEAVE("TCL_ERROR");
            return TCL_ERROR;
        }

        if ((Tcl_ListObjAppendElement(interp, pResultList, pRe) != TCL_OK) ||
            (Tcl_ListObjAppendElement(interp, pResultList, pIm) != TCL_OK)) {
            const char s[] = "cannot append FFT point";
            Tcl_Obj *poResult = Tcl_NewStringObj(s, sizeof(s) - 1);
            Tcl_SetObjResult(interp, poResult);
            LEAVE("TCL_ERROR");
            return TCL_ERROR;
        }

        INFO("appended {%d + j%d}", (int)round((*pFftInput)[0]), (int)round((*pFftInput)[1]));

        /*
         * advance pointer
         */
        ++pFftInput;
    }

    INFO("returning flat list");
    Tcl_SetObjResult(interp, pResultList);

    LEAVE("TCL_OK");
    return TCL_OK;
}

/**
 * implementation of the TCL command {@code fftw::getAt}.
 *
 * <code>fftw::getAt <i>plan</i> <i>dir</i> <i>ix</i></code>
 *
 * @param clientData pointer to client data defined with Tcl_CreateObjCommand
 * @param interp handle to TCL interpreter
 * @param objc number of elements in objv
 * @param objv arguments passed to fftw_command
 *
 * @return TCL_OK or TCL_ERROR
 */
extern "C" int
fft_getAt(ClientData     clientData,
          Tcl_Interp     *interp,
          int            objc,
          Tcl_Obj *CONST objv[])
{
    plan **pp   = NULL;
    plan *p     = NULL;
    int  length = 0;
    int  dir    = 0;
    int  ix     = 0;

    ENTER();

    if (objc <= 3) {
        Tcl_WrongNumArgs(interp, 1, objv, "plan dir ix");
        LEAVE("TCL_ERROR");
        return TCL_ERROR;
    }

    /*
     * get pointer to FFT plan
     */
    pp = reinterpret_cast<plan **>(Tcl_GetByteArrayFromObj(objv[1], &length));

    if (length != sizeof(*pp)) {
        const char s[] = "need proper plan";
        Tcl_Obj *poResult = Tcl_NewStringObj(s, sizeof(s) - 1);
        Tcl_SetObjResult(interp, poResult);
        LEAVE("TCL_ERROR");
        return TCL_ERROR;
    }

    p = *pp;
    INFO("plan is %p", p);

    /*
     * get direction
     */
    Tcl_GetIntFromObj(interp, objv[2], &dir);
    INFO("direction is %d", dir);

    /*
     * get point index
     */
    Tcl_GetIntFromObj(interp, objv[3], &ix);

    if ((ix < 0) || (ix >= p->n)) {
        const char s[] = "invalid point index";
        Tcl_Obj *poResult = Tcl_NewStringObj(s, sizeof(s) - 1);
        Tcl_SetObjResult(interp, poResult);
        LEAVE("TCL_ERROR");
        return TCL_ERROR;
    }

    INFO("point index is %d", ix);

    /*
     * go to selected start in in-memory array and open a new Tcl list
     * for the result
     */
    fftw_complex *pFftInput   = dir ? p->in + ix : p->out + ix;

    Tcl_Obj *pRe    = Tcl_NewDoubleObj((*pFftInput)[0]);
    Tcl_Obj *pIm    = Tcl_NewDoubleObj((*pFftInput)[1]);
    Tcl_Obj *pCmplx = Tcl_NewListObj(0, NULL);

    if ((pRe == NULL) || (pIm == NULL) || (pCmplx == NULL)) {
        const char s[] = "cannot allocate FFT point";
        Tcl_Obj *poResult = Tcl_NewStringObj(s, sizeof(s) - 1);
        Tcl_SetObjResult(interp, poResult);
        LEAVE("TCL_ERROR");
        return TCL_ERROR;
    }

    if ((Tcl_ListObjAppendElement(interp, pCmplx, pRe) != TCL_OK) ||
        (Tcl_ListObjAppendElement(interp, pCmplx, pIm) != TCL_OK)) {
        LEAVE("TCL_ERROR");
        return TCL_ERROR;
    }

    INFO("found {%.2lf + j%.2lf}", (*pFftInput)[0], (*pFftInput)[1]);

    INFO("returning complex list");
    Tcl_SetObjResult(interp, pCmplx);

    LEAVE("TCL_OK");
    return TCL_OK;
}


/** type of a hash table holding Tcl command-line options */
typedef map<string, Tcl_Obj *> OptionMap;

/* define option keys as static strings */
static const string opt_fileName("-fileName");
static const string opt_center("-center");
static const string opt_phaseSeed("-phaseSeed");
static const string opt_spacing("-spacing");
static const string opt_sampleRate("-sampleRate");
static const string opt_scale("-scale");
static const string opt_startFreqList("-startFreqList");
static const string opt_stopFreqList("-stopFreqList");
static const string opt_powerList("-powerList");
static const string opt_flatFreqsList("-flatFreqsList");
static const string opt_flatAmpsList("-flatAmpsList");
static const string opt_ssbFreqsList("-ssbFreqsList");
static const string opt_ssbAmpsList("-ssbAmpsList");
static const string opt_ssbPhaseList("-ssbPhaseList");
static const string yes("y Y Yes YES 1 true TRUE on ON");

/* define Tcl defaults for options */
static Tcl_Obj *def_center            = Tcl_NewDoubleObj(14250.);
static Tcl_Obj *def_phaseSeed         = Tcl_NewIntObj(2);
static Tcl_Obj *def_spacing           = Tcl_NewDoubleObj(1.);
static Tcl_Obj *def_sampleRate        = Tcl_NewDoubleObj(600.);
static Tcl_Obj *def_scale             = Tcl_NewStringObj("y", sizeof("y"));
static Tcl_Obj *def_startFreqList     = Tcl_NewListObj(0, NULL);
static Tcl_Obj *def_stopFreqList      = Tcl_NewListObj(0, NULL);
static Tcl_Obj *def_powerList[]       = { Tcl_NewDoubleObj(0.) };
static Tcl_Obj *def_powerListHead     = Tcl_NewListObj(1, def_powerList);
static Tcl_Obj *def_flatFreqsList[]   = { Tcl_NewDoubleObj(0.), Tcl_NewDoubleObj(100000.) };
static Tcl_Obj *def_flatFreqsListHead = Tcl_NewListObj(2, def_flatFreqsList);
static Tcl_Obj *def_flatAmpsList[]    = { Tcl_NewDoubleObj(0.), Tcl_NewDoubleObj(0.) };
static Tcl_Obj *def_flatAmpsListHead  = Tcl_NewListObj(2, def_flatAmpsList);
static Tcl_Obj *def_ssbFreqsList[]    = { Tcl_NewDoubleObj(0.), Tcl_NewDoubleObj(100000.) };
static Tcl_Obj *def_ssbFreqsListHead  = Tcl_NewListObj(2, def_ssbFreqsList);
static Tcl_Obj *def_ssbAmpsList[]     = { Tcl_NewDoubleObj(0.), Tcl_NewDoubleObj(0.) };
static Tcl_Obj *def_ssbAmpsListHead   = Tcl_NewListObj(2, def_ssbAmpsList);
static Tcl_Obj *def_ssbPhaseList[]    = { Tcl_NewDoubleObj(0.), Tcl_NewDoubleObj(0.) };
static Tcl_Obj *def_ssbPhaseListHead  = Tcl_NewListObj(2, def_ssbPhaseList);


/**
 * cleanup all FFTW related data: in/out arrays, plans, and wisdom
 *
 * @param p reference to FFTW plan
 */
static void releaseMcWaveformData(plan &p, OptionMap &options) {
    /* release old FFT array if present */
    if (p.in != NULL)
        fftw_free(p.in);

    if ((p.in != p.out) && (p.out != NULL))
        fftw_free(p.out);

    /* release FFTW plans */
    if (p.plan_fwd != NULL)
        fftw_destroy_plan(p.plan_fwd);

    if (p.plan_bwd != NULL)
        fftw_destroy_plan(p.plan_bwd);

    fftw_cleanup();
}


/**
 * implementation of the TCL command {@code calcMcWaveform}.
 *
 * #%ProcedureRange
 * set optionsRange(-center)     {-minmax 0:+}
 * set optionsRange(-spacing)    {-minmax {0.0001:600}}
 * set optionsRange(-sampleRate) {-minmax {0.001:600}}
 * set optionsRange(-scale)      {-values {y n yes no t f true false 1 0} -case no}
 *
 * #%ProcedureArgs
 * set options(-fileName)       {}
 * set options(-center)         {14250}
 * set options(-phaseSeed)      {2}
 * set options(-spacing)        {1.}
 * set options(-sampleRate)     {600}
 * set options(-scale)          {y}
 * set options(-startFreqList)  {}
 * set options(-stopFreqList)   {}
 * set options(-powerList)      {0.}
 * set options(-flatFreqsList)  {0. 100000.}
 * set options(-flatAmpsList)   {0. 0.}
 * set options(-ssbFreqsList)   {0. 100000.}
 * set options(-ssbAmpsList)    {0. 0.}
 * set options(-ssbPhaseList)   {0. 0.}
 *
 * @param clientData pointer to client data defined with Tcl_CreateObjCommand
 * @param interp handle to TCL interpreter
 * @param objc number of elements in objv
 * @param objv arguments passed to fftw_command
 *
 * @return TCL_OK or TCL_ERROR
 */
extern "C" int
calcMcWaveform(ClientData     clientData,
               Tcl_Interp     *interp,
               int            objc,
               Tcl_Obj *CONST objv[])
{
    ENTER();

    /* zeroth argument is the command name */
    if (((objc - 1) % 2) != 0) {
        const char s[] = "argument list must be of even length";
        Tcl_Obj *poResult = Tcl_NewStringObj(s, sizeof(s) - 1);
        Tcl_SetObjResult(interp, poResult);
        LEAVE("TCL_ERROR");
        return TCL_ERROR;
    }

    /* allocate & clear FFTW plan */
    plan p;
    memset(static_cast<void *>(&p), 0, sizeof(p));

    /* we expect a list of "-opt val" pairs and store them into a hash table */
    OptionMap options;

    try {
        /* preset options with default values */
        if ((options[opt_center] = def_center) == NULL)
            throw "cannot set -center";

        if ((options[opt_phaseSeed] = def_phaseSeed) == NULL)
            throw "cannot set -phaseSeed";

        if ((options[opt_spacing] = def_spacing) == NULL)
            throw "cannot set -spacing";

        if ((options[opt_sampleRate] = def_sampleRate) == NULL)
            throw "cannot set -sampleRate";

        if ((options[opt_scale] = def_scale) == NULL)
            throw "cannot set -scale";

        if ((options[opt_startFreqList] = def_startFreqList) == NULL)
            throw "cannot set -startFreqList";

        if ((options[opt_stopFreqList] = def_stopFreqList) == NULL)
            throw "cannot set -stopFreqList";

        Tcl_Obj *powerList[] = { NULL };

        if ((options[opt_powerList] = def_powerListHead) == NULL)
            throw "cannot set -powerList";

        if ((options[opt_flatFreqsList] = def_flatFreqsListHead) == NULL)
            throw "cannot set -flatFreqsList";

        if ((options[opt_flatAmpsList] = def_flatAmpsListHead) == NULL)
            throw "cannot set -flatAmpsList";

        Tcl_Obj *ssbFreqsList[] = { NULL, NULL };

        if ((options[opt_ssbFreqsList] = def_ssbFreqsListHead) == NULL)
            throw "cannot set -ssbFreqsList";

        Tcl_Obj *ssbAmpsList[] = { NULL, NULL };

        if ((options[opt_ssbAmpsList] = def_ssbAmpsListHead) == NULL)
            throw "cannot set -ssbAmpsList";

        Tcl_Obj *ssbPhaseList[] = { NULL, NULL };

        if ((options[opt_ssbPhaseList] = def_ssbPhaseListHead) == NULL)
            throw "cannot set -ssbPhaseList";

        /* get actual option value from arguments */
        for (int i = 1; i < objc; i += 2) {
            const string        k = string(Tcl_GetString(objv[i]));
            options[k] = objv[i + 1];
        }

#ifdef EBUG
        {
            for (OptionMap::iterator i = options.begin(); i != options.end(); i++) {
                INFO("options %s is %s", i->first.c_str(), Tcl_GetString(i->second));
            }
        }
#endif

        /* get list lengths of -powerList, -startFreqList, -stopFreqList, -phaseSeed, -ssbAmpsList, and -ssbPhaseList */
        int powerListLen     = 0;
        int startFreqListLen = 0;
        int stopFreqListLen  = 0;
        int phaseSeedListLen = 0;
        int ssbFreqsListLen  = 0;
        int ssbAmpsListLen   = 0;
        int ssbPhaseListLen  = 0;

        if (Tcl_ListObjLength(interp, options[opt_powerList], &powerListLen) != TCL_OK)
            throw "-powerList not a list";

        if (Tcl_ListObjLength(interp, options[opt_startFreqList], &startFreqListLen) != TCL_OK)
            throw "-startFreqList not a list";

        if (Tcl_ListObjLength(interp, options[opt_stopFreqList], &stopFreqListLen) != TCL_OK)
            throw "-stopFreqList not a list";

        if (Tcl_ListObjLength(interp, options[opt_phaseSeed], &phaseSeedListLen) != TCL_OK)
            throw "-phaseSeedList not a list";

        if (Tcl_ListObjLength(interp, options[opt_ssbFreqsList], &ssbFreqsListLen) != TCL_OK)
            throw "-ssbFreqsList not a list";

        if (Tcl_ListObjLength(interp, options[opt_ssbAmpsList], &ssbAmpsListLen) != TCL_OK)
            throw "-ssbAmpsList not a list";

        if (Tcl_ListObjLength(interp, options[opt_ssbPhaseList], &ssbPhaseListLen) != TCL_OK)
            throw "-ssbPhaseList not a list";

        /* check some necessary conditions for -powerList, -startFreqList, and -stopFreqList */
        if (startFreqListLen < 1)
            throw "-startFreqList must not be empty";

        if (powerListLen < 1) {
            options[opt_powerList] = def_powerListHead;
            powerListLen           = 1;
        }

        if ((powerListLen > 1) && (powerListLen != startFreqListLen))
            throw "-powerList must have just one element or be of same length as -startFreqList";

        if ((stopFreqListLen > 0) && (stopFreqListLen != startFreqListLen))
            throw "-stopFreqList must be either empty or of same length as -startFreqList";

        if (phaseSeedListLen < 1)
            throw "-phaseSeed must not be empty";

        if ((phaseSeedListLen > 1) && (phaseSeedListLen != startFreqListLen))
            throw "-phaseSeed must have just one element or be of same length as -startFreqList";

        if (ssbFreqsListLen != ssbAmpsListLen)
            throw "-ssbFreqsList must have same length as -ssbAmpsList";

        if (ssbFreqsListLen != ssbPhaseListLen)
            throw "-ssbFreqsList must have same length as -ssbPhaseList";

        INFO("card(-powerList)     = %d", powerListLen);
        INFO("card(-startFreqList) = %d", startFreqListLen);
        INFO("card(-stopFreqList)  = %d", stopFreqListLen);
        INFO("card(-phaseSeed)     = %d", phaseSeedListLen);

        /* get & check  -sampleRate from options */
        double sampleRate;

        if (Tcl_GetDoubleFromObj(interp, options[opt_sampleRate], &sampleRate) != TCL_OK)
            throw "invalid -sampleRate";

        if ((sampleRate < 0.001) || (sampleRate > 600.))
            throw "-sampleRate out of range [.001, 600.]";

        /* get & check -spacing */
        double spacing;

        if (Tcl_GetDoubleFromObj(interp, options[opt_spacing], &spacing) != TCL_OK)
            throw "invalid -spacing";

        if ((spacing < 0.0001) || (spacing > 600.))
            throw "-spacing out of range [.0001, 600.]";

        /* calculate number of FFT points */
        const long fftPoints  = lround(sampleRate / spacing);

        /* allocate & clear FFT point arrays */
        INFO("allocating input/output array for %ld points...", fftPoints);

        if ((p.in = static_cast<double (*)[2]>(fftw_malloc(sizeof(fftw_complex) * fftPoints))) == NULL)
            throw "cannot allocate input/output array";

        /* this function uses in-place transforms */
        p.out = p.in;
        p.n   = fftPoints;

        memset(static_cast<void *>(p.in), 0, sizeof(fftw_complex) * fftPoints);

        /* create execution plans */
        INFO("creating FFTW plans...");

        p.plan_fwd = fftw_plan_dft_1d(fftPoints, p.in, p.out, FFTW_FORWARD,  FFTW_ESTIMATE);
        p.plan_bwd = fftw_plan_dft_1d(fftPoints, p.out, p.in, FFTW_BACKWARD, FFTW_ESTIMATE);

        INFO("FFTW plans allocated");

        double rmsPower = 0.;   /* linear total power of the generated waveform [mW] */

        /*
         * in order to force the destructors of the interpolators, we put them
         * within a block
         */
        {
            /* setup the interpolators */
            INFO("converting -ssbAmpsList from dB to linear");
            ssbAmpsList[0]  = options[opt_ssbAmpsList];
            ssbFreqsList[0] = options[opt_ssbFreqsList];

            /* we copy -ssbFreqsList and the converted -ssbAmpsList directly into vectors */
            vector<double> ssbFreqs;
            vector<double> ssbAmps;
            ssbFreqs.reserve(ssbFreqsListLen);
            ssbAmps.reserve(ssbAmpsListLen);

            for (int i = 0; i < ssbAmpsListLen; i++) {
                Tcl_Obj *ampObj;
                Tcl_Obj *frqObj;
                double  amp;
                double  f;

                /* get -ssbFreqsList[i] */
                if (Tcl_ListObjIndex(interp, ssbFreqsList[0], i, &frqObj) != TCL_OK)
                    throw "cannot access element of -ssbFreqsList";

                if (Tcl_GetDoubleFromObj(interp, frqObj, &f) != TCL_OK)
                    throw "element of -ssbFreqsList not a proper double";

                /* get -ssbAmpsList[i] */
                if (Tcl_ListObjIndex(interp, ssbAmpsList[0], i, &ampObj) != TCL_OK)
                    throw "cannot access element of -ssbAmpsList";

                if (Tcl_GetDoubleFromObj(interp, ampObj, &amp) != TCL_OK)
                    throw "element of -ssbAmpsList not a proper double";

                /* convert dB (power) to amplitude correction factor */
                amp = pow(10., amp / 20.);

                /* append to vectors */
                ssbFreqs.push_back(f);
                ssbAmps.push_back(amp);
            }

            INFO("converting -ssbPhaseList from degree to radiants");
            ssbPhaseList[0] = options[opt_ssbPhaseList];

            /* we copy -ssbFreqsList and the converted -ssbPhaseList directly into vectors */
            vector<double> ssbPhase;
            ssbPhase.reserve(ssbPhaseListLen);

            for (int i = 0; i < ssbPhaseListLen; i++) {
                Tcl_Obj *phaseObj;
                double  phase;

                /* get -ssbPhaseList[i] */
                if (Tcl_ListObjIndex(interp, ssbPhaseList[0], i, &phaseObj) != TCL_OK)
                    throw "cannot access element of -ssbPhaseList";

                if (Tcl_GetDoubleFromObj(interp, phaseObj, &phase) != TCL_OK)
                    throw "element of -ssbPhaseList not a proper double";

                /* convert degree to radiants */
                phase = M_PI * phase / 180.;

                /* append to vectors */
                ssbPhase.push_back(phase);
            }

            INFO("setup Interpolators for flatness and sideband suppression...");
            Interpolator ipFlatAmp  = Interpolator(interp, options[opt_flatFreqsList], options[opt_flatAmpsList]);
            Interpolator ipSsbAmp   = Interpolator(ssbFreqs, ssbAmps);
            Interpolator ipSsbPhase = Interpolator(ssbFreqs, ssbPhase);

            INFO("Interpolators for flatness and sideband suppression allocated and set up");

            /* frequencies were passed in units of MHz, but we calculate center and spacing in units of 100 Hz */
            double center = 0;

            if (Tcl_GetDoubleFromObj(interp, options[opt_center], &center) != TCL_OK)
                throw "-center not a valid double";

            const long center_cHz  = lround(center  * 1.e6 / 1.e2);
            const long spacing_cHz = lround(spacing * 1.e6 / 1.e2);

            /* get -startFreqList, -stopFreqList, -powerList, and -phaseSeed as Tcl objects */
            Tcl_Obj *startFreqList = options[opt_startFreqList];
            Tcl_Obj *stopFreqList  = options[opt_stopFreqList];
            Tcl_Obj *phaseSeedList = options[opt_phaseSeed];
            powerList[0]           = options[opt_powerList];

            /* optimization: get the first power level */
            Tcl_Obj *powerObj;
            double power0;

            if (Tcl_ListObjIndex(interp, powerList[0], 0, &powerObj) != TCL_OK)
                throw "cannot access first element of -powerList";

            if (Tcl_GetDoubleFromObj(interp, powerObj, &power0) != TCL_OK)
                throw "first element of -powerList is not a proper double";

            /* optimization: get the first phase seed */
            Tcl_Obj *phaseSeedObj;
            double phaseSeed0;

            if (Tcl_ListObjIndex(interp, phaseSeedList, 0, &phaseSeedObj) != TCL_OK)
                throw "cannot access first element of -phaseSeed";

            if (Tcl_GetDoubleFromObj(interp, phaseSeedObj, &phaseSeed0) != TCL_OK)
                throw "first element of -phaseSeed is not a proper double";

            /* setup random number generator, if -phaseSeed >= 11 */
            if ((phaseSeedListLen == 1) && (phaseSeed0 >= 11)) {
                INFO("set random phases");
                srand(static_cast<int>(phaseSeed0));
            }

            /*
             * We start filling the real part of the FFT array with the
             * magnitude of the desired frequency response and the imaginary part
             * with the phases (Newman phases are a special case handeled separately)
             */
            long freqCount = 0;

            for (int i = 0; i < startFreqListLen; i++) {
                /* get and check fStart */
                Tcl_Obj *fStartObj;
                double  fStart;

                if (Tcl_ListObjIndex(interp, startFreqList, i, &fStartObj) != TCL_OK)
                    throw "cannot access element of -startFreqList";

                if (Tcl_GetDoubleFromObj(interp, fStartObj, &fStart) != TCL_OK)
                    throw "-startFreqList not a proper list of doubles";

                /*
                 * get fstop.
                 * if -stopFreqList is a non-emptry list, a frequency range has been
                 * specified, if -stopFreqList is {}, a frequency point is requested
                 */
                double fStop;

                if (stopFreqListLen == 0)
                    fStop = fStart;
                else {
                    Tcl_Obj *fStopObj;

                    if (Tcl_ListObjIndex(interp, stopFreqList, i, &fStopObj) != TCL_OK)
                        throw "cannot access element of -stopFreqList";

                    if (Tcl_GetDoubleFromObj(interp, fStopObj, &fStop) != TCL_OK)
                        throw "-stopFreqList not a proper list of doubles";
                }

                /* sanity */
                if (fStart > fStop)
                    throw "frequencies in -startFreqList must be lower than frequencies in -stopFreqList";

                /* rescale start and stop frequency from Mhz to cHz */
                const long fStart_cHz = lround(fStart * 1.e6 / 1.e2);
                const long fStop_cHz  = lround(fStop * 1.e6 / 1.e2);

                /*
                 * get power.
                 * if -powerList has just one element, this specifies the power for all
                 * frequency ranges, otherwise, a per-range power has been given
                 */
                double power;

                if (powerListLen <= 1)
                    power = power0;
                else {
                    Tcl_Obj *powerObj;

                    if (Tcl_ListObjIndex(interp, powerList[0], i, &powerObj) != TCL_OK)
                        throw "cannot access element of -powerList";

                    if (Tcl_GetDoubleFromObj(interp, powerObj, &power) != TCL_OK)
                        throw "-powerList not a proper list of doubles";
                }

                INFO("set power %lf dBm for range [%ld, %ld] cHz in %ld cHz steps", power, fStart_cHz, fStop_cHz, spacing_cHz);

                /*
                 * get phaseSeed.
                 * if -phaseSeed has just one element, this specifies the seed for
                 * random phases. Otherwise it sets the phase for each block of
                 * frequency ranges
                 */
                double phaseSeed;

                if (phaseSeedListLen <= 1)
                    phaseSeed = phaseSeed0;
                else {
                    Tcl_Obj *phaseSeedObj;

                    if (Tcl_ListObjIndex(interp, phaseSeedList, i, &phaseSeedObj) != TCL_OK)
                        throw "cannot access element of -phaseSeed";

                    if (Tcl_GetDoubleFromObj(interp, phaseSeedObj, &phaseSeed) != TCL_OK)
                        throw "-phaseSeed not a proper list of doubles";

                    INFO("set phase %lf for range [%ld, %ld] cHz", phaseSeed, fStart_cHz, fStop_cHz);
                }

                /* convert power into magnitude/amplitude */
                double apc;

                if (fStart_cHz == fStop_cHz)
                    /* single tone */
                    apc = pow(10, power / 20.);
                else
                    /* power is the total power of the whole frequency block [fStart, fStop]. */
                    apc = sqrt(pow(10, power / 10.) / (1. + (fStop_cHz - fStart_cHz) / spacing_cHz));

                INFO("%lf dBm = %lf Vrms", power, apc);

                /*
                 * fill the FFT array with the selected frequencies
                 * index 0 corresponds center_cHz, frequencies left to center_cHz enter the FFT array at the right edge
                 */
                for (long f = fStart_cHz; f <= fStop_cHz; f += spacing_cHz) {
                    long ix = lround((f - center_cHz) / spacing_cHz);

                    if (ix < 0)
                        ix = fftPoints + ix;

                    if ((ix >= 0) && (ix < fftPoints))
                        p.in[ix][0] = apc;
                    else
                        throw "span of -startFreqList and -stopFreqList too wide for -center, -spacing and -sampleRate";

                    ++freqCount;

                    if ((phaseSeedListLen == 1) && (phaseSeed0 >= 11))
                        /* set random phase for frequency point */
                        p.in[ix][1] = rand() * M_PI * 2.0 / RAND_MAX;
                    else if ((phaseSeedListLen > 1) && (f == fStart_cHz))
                        /* set specified phase for left edge of frequency block */
                        p.in[ix][1] = phaseSeed;
                }
            }

            if ((phaseSeedListLen == 1) && (phaseSeed0 < 11)) {
                /*
                 * if a single -phaseSeed below 11 has been specified, we fill the
                 * imaginary part with Newman phases: p[k] = pi * (k - 1)^2 / N
                 */
                INFO("set Newman phases");

                long j = 0;

                for (long i = fftPoints / 2; i < (fftPoints + fftPoints / 2); i++) {
                    long k = (i >= fftPoints) ? i - fftPoints : i;

                    if (p.in[k][0] == 0.)
                        /* empty point, continue */
                        continue;
                    else {
                        p.in[k][1] = M_PI * pow(j, phaseSeed0) / freqCount;
                        ++j;
                    }
                }
            }

            /*
             * convert polar to cartesian and apply flatness calibration
             */
            for (long i = 0; i < fftPoints; i++) {
                /* sum power */
                rmsPower += (p.in[i][0] * p.in[i][0]);

                /* convert index to frequency */
                double f = (i < (fftPoints >> 1)) ? center + i * spacing : center + (i - fftPoints) * spacing;

                /* get flatness correction */
                double flatCorrDb = ipFlatAmp(f);
                double flatCorr   = pow(10, flatCorrDb / 20.);

                /* correct by factor and convert to cartesian */
                double amplCal = p.in[i][0] * flatCorr;
                double phase   = p.in[i][1];
                p.in[i][0]     = amplCal * cos(phase);
                p.in[i][1]     = amplCal * sin(phase);
            }

            /*
             * for sideband suppression, separate the spectrum into even (I) and
             * odd (Q), then apply SSB to the odd/Q part.
             *
             * The I part of the spectrum is the real part of the time signal, i.e., its
             * spectrum has an even real part and an odd imaginary part:
             *
             * 2 * I[k] = (Re{FFT[k]} + Re{FFT[-k]}) + j * (Im{FFT[k]} - Im{FFT[-k]})
             *
             * Q is the imaginary part of the time signal, i.e., its spectrum has an
             * odd real part and an even imaginary part
             *
             * 2 * Q[k] = (Re{FFT[k]} - Re{FFT[-k]}) + j * (Im{FFT[k]} + Im{FFT[-k]})
             *
             * FFT[k] = I[k] + j * Q[k] * ssbCal[k]
             *
             * center frequency (FFT index 0) is a special case:
             *
             * I[0] = Re{FFT[0]}
             * Q[0] = Im{FFT[0]} * ssbCal(0)
             * FFT[0] = I[0] + j * Q[0]
             */

            /* calibrated center frequency */
            p.in[0][1] *= ipSsbAmp(0);

            for (int k = 1; k <= (fftPoints / 2); k++) {
                /* compute mirrored index, frequency and mirrored frequency from running index */
                const int km = fftPoints - k;

                /* FFT spectrum point FFT[k] and its mirrored point FFT[-k] */
                const fftw_complex FFT  = {p.in[k][0],  p.in[k][1]};
                const fftw_complex FFTm = {p.in[km][0], p.in[km][1]};

                if ((FFT[0] == 0.) && (FFT[1] == 0.) && (FFTm[0] == 0) && (FFTm[1] == 0.))
                    continue;

                /* I[k] and I[-k] */
                const fftw_complex I  = {(FFT[0] + FFTm[0]) / 2., (FFT[1] - FFTm[1]) / 2.};
                const fftw_complex Im = {I[0],                    -I[1]};

                /* Q[k] and Q[-k] */
                fftw_complex Q  = {(FFT[1] + FFTm[1]) / 2., (FFTm[0] - FFT[0]) / 2.};
                fftw_complex Qm = {Q[0],                    -Q[1]};

                /* apply SSB calibration to Q[k], Q[-k] */
                const double f           = k * spacing;
                const double fm          = -k * spacing;
                const double ampCorr     = ipSsbAmp(f);
                const double ampCorr_m   = ipSsbAmp(fm);
                const double phaseCorr   = ipSsbPhase(f);
                const double phaseCorr_m = ipSsbPhase(fm);
                const double c           = cos(phaseCorr);
                const double s           = sin(phaseCorr);
                const double c_m         = cos(phaseCorr_m);
                const double s_m         = sin(phaseCorr_m);

                const fftw_complex Qcorr = {
                    ampCorr * (Q[0] * c + Q[1] * s),
                    ampCorr * (Q[1] * c - Q[0] * s)
                };
                const fftw_complex Qcorr_m = {
                    ampCorr_m * (Qm[0] * c_m + Qm[1] * s_m),
                    ampCorr_m * (Qm[1] * c_m - Qm[0] * s_m)
                };

                /* store corrected FFT spectrum points */
                p.in[k][0]  = I[0] - Qcorr[1];
                p.in[k][1]  = I[1] + Qcorr[0];
                p.in[km][0] = Im[0] - Qcorr_m[1];
                p.in[km][1] = Im[1] + Qcorr_m[0];
            }
        }

        double powerSum = 10. * log10(rmsPower); /* total power in dBm */

        if (!isfinite(powerSum))
            throw "under- or overflow for total power [dBm]";

        INFO("rms: %lf mW (%lf dBm)", rmsPower, powerSum);

        INFO("executing IFFT...");
        fftw_execute(p.plan_bwd);

        /*
         * search absolute max, and compute scale factor
         */
        double    absMax      = 0.;
        double    scaleFactor = 1.;
        const int scaleMax    = 32767;
        INFO("rescaling to %d...", scaleMax);

        for (int i = 0; i < fftPoints; i++) {
            if (fabs(p.in[i][0]) > absMax)
                absMax = fabs(p.in[i][0]);

            if (fabs(p.in[i][1]) > absMax)
                absMax = fabs(p.in[i][1]);
        }

        if (absMax > DBL_MIN)
            scaleFactor = scaleMax / absMax;

        INFO("scale factor is %lf", scaleFactor);

        /* scale rms power with scale factor */
        rmsPower = 10. * log10(rmsPower / (scaleMax * scaleMax / (scaleFactor * scaleFactor)));
        INFO("scaled rms power is %lf", rmsPower);

        INFO("building result lists...");

        if (options[opt_fileName] == NULL) {
            /*
             * build the -pattern list
             */
            Tcl_Obj *patternList = Tcl_NewListObj(fftPoints * 2, NULL);     /* prepare internal representation for fftPoints complex numbers*/
            if (patternList == NULL)
                throw "cannot allocate -pattern";

            INFO("appending %ld points to -pattern", fftPoints);

            /* rescale pattern, if -scale is true */
            if (yes.find(Tcl_GetString(options[opt_scale])) != string::npos) {
                INFO("rescaling time signal...");

                for (int i = 0; i < fftPoints; i++) {
                    Tcl_Obj *pRe = Tcl_NewIntObj(static_cast<int>(round(p.in[i][0] * scaleFactor)));
                    Tcl_Obj *pIm = Tcl_NewIntObj(static_cast<int>(round(p.in[i][1] * scaleFactor)));

                    if ((pRe == NULL) || (pIm == NULL))
                        throw "cannot allocate -pattern point";

                    if ((Tcl_ListObjAppendElement(interp, patternList, pRe) != TCL_OK) ||
                        (Tcl_ListObjAppendElement(interp, patternList, pIm) != TCL_OK))
                        throw "cannot append FFT point to -pattern";
                }
            } else {
                INFO("descaling time signal...");

                for (int i = 0; i < fftPoints; i++) {
                    Tcl_Obj *pRe = Tcl_NewDoubleObj(static_cast<int>(round(p.in[i][0] * scaleFactor)) / scaleFactor);
                    Tcl_Obj *pIm = Tcl_NewDoubleObj(static_cast<int>(round(p.in[i][1] * scaleFactor)) / scaleFactor);

                    if ((pRe == NULL) || (pIm == NULL))
                        throw "cannot allocate -pattern point";

                    if ((Tcl_ListObjAppendElement(interp, patternList, pRe) != TCL_OK) ||
                        (Tcl_ListObjAppendElement(interp, patternList, pIm) != TCL_OK))
                        throw "cannot append FFT point to -pattern";
                }
            }

            INFO("building result list...");

            /* combine -rms, -powerSum and -pattern into the final result */
            Tcl_Obj *resultList[] = {
                Tcl_NewStringObj("-rms", sizeof("-rms") - 1),           Tcl_NewDoubleObj(rmsPower),
                Tcl_NewStringObj("-powerSum", sizeof("-powerSum") - 1), Tcl_NewDoubleObj(powerSum),
                Tcl_NewStringObj("-pattern", sizeof("-pattern") - 1),   patternList
            };

            INFO("set result with -pattern");
            Tcl_SetObjResult(interp, Tcl_NewListObj(6, resultList));
        } else {
            /*
             * write the binary data to a file
             */
            const char *fileName = Tcl_GetString(options[opt_fileName]);

            if (fileName == NULL)
                throw "cannot access -fileName";

            INFO("writing %ld points to %s", fftPoints, fileName);

            fstream patternFile;
            patternFile.open(fileName, fstream::out | fstream::binary);

            if (!patternFile.is_open())
                throw "cannot open -fileName";

            /* rescale pattern, if -scale is true */
            if (yes.find(Tcl_GetString(options[opt_scale])) != string::npos) {
                INFO("rescaling time signal...");

                for (int i = 0; i < fftPoints; i++) {
#if BYTE_ORDER == LITTLE_ENDIAN
                    const int16_t x[2] = {
                        static_cast<int16_t>(round(p.in[i][0] * scaleFactor)),
                        static_cast<int16_t>(round(p.in[i][1] * scaleFactor))
                    };
#else
                    const int16_t y[2] = {
                        static_cast<int16_t>(round(p.in[i][1] * scaleFactor)),
                        static_cast<int16_t>(round(p.in[i][0] * scaleFactor))
                    };

                    const int32_t x = __builtin_bswap32(reinterpret_cast<int32_t>(y));
#endif

                    patternFile.write(reinterpret_cast<const char *>(&x), sizeof(x));

                    if (!patternFile)
                        throw "cannot write complex point to -fileName";
                }
            } else {
                INFO("descaling time signal...");
                for (int i = 0; i < fftPoints; i++) {
#if BYTE_ORDER == LITTLE_ENDIAN
                    const int16_t x[2] = {
                        static_cast<int16_t>(round(p.in[i][0] * scaleFactor) / scaleFactor),
                        static_cast<int16_t>(round(p.in[i][1] * scaleFactor) / scaleFactor)
                    };
#else
                    const int16_t y[2] = {
                        static_cast<int16_t>(round(p.in[i][1] * scaleFactor) / scaleFactor),
                        static_cast<int16_t>(round(p.in[i][0] * scaleFactor) / scaleFactor)
                    };

                    const int32_t x = __builtin_bswap32(reinterpret_cast<int32_t>(y));
#endif

                    patternFile.write(reinterpret_cast<const char *>(&x), sizeof(x));

                    if (!patternFile)
                        throw "cannot write complex point to -fileName";
                }
            }

            patternFile.close();

            INFO("building result list...");

            /* combine -rms, -powerSum and -pattern into the final result */
            Tcl_Obj *resultList[] = {
                Tcl_NewStringObj("-rms", sizeof("-rms") - 1),             Tcl_NewDoubleObj(rmsPower),
                Tcl_NewStringObj("-powerSum", sizeof("-powerSum") - 1),   Tcl_NewDoubleObj(powerSum),
                Tcl_NewStringObj("-fftPoints", sizeof("-fftPoints") - 1), Tcl_NewIntObj(fftPoints)
            };

            INFO("set result with -fftPoints");
            Tcl_SetObjResult(interp, Tcl_NewListObj(6, resultList));
        }
    } catch (Interpolator::Exception &ex) {
        INFO("interpolator exception: %s", ex.error.c_str());
        const char *s = ex.error.c_str();
        Tcl_Obj *poResult = Tcl_NewStringObj(s, strlen(s));
        Tcl_SetObjResult(interp, poResult);
        releaseMcWaveformData(p, options);
        LEAVE("TCL_ERROR");
        return TCL_ERROR;
    } catch (const char *s) {
        INFO("calcMcWaveform exception: %s", s);
        Tcl_Obj *poResult = Tcl_NewStringObj(s, strlen(s));
        Tcl_SetObjResult(interp, poResult);
        releaseMcWaveformData(p, options);
        LEAVE("TCL_ERROR");
        return TCL_ERROR;
    } catch (exception exStd) {
        INFO("C++ exception: %s", exStd.what());
        Tcl_Obj *poResult = Tcl_NewStringObj(exStd.what(), strlen(exStd.what()));
        Tcl_SetObjResult(interp, poResult);
        releaseMcWaveformData(p, options);
        LEAVE("TCL_ERROR");
        return TCL_ERROR;
    }

    releaseMcWaveformData(p, options);
    LEAVE("TCL_OK");
    return TCL_OK;
}


/**
 * the interpreter calls this function after loading the
 * shared library
 *
 * @param interp handle to TCL interpreter
 * @return always TCL_OK
 */
extern "C" int
Tclfftw_Init(Tcl_Interp *interp)
{
    ENTER();

    /* create namespace */
    if (Tcl_CreateNamespace(interp, NS_PREFIX, NULL, NULL) == NULL)
        return TCL_ERROR;

    /* define commands */
    Tcl_CreateObjCommand(interp, NS_PREFIX "plan",       fft_plan,       NULL, NULL);
    Tcl_CreateObjCommand(interp, NS_PREFIX "destroy",    fft_destroy,    NULL, NULL);
    Tcl_CreateObjCommand(interp, NS_PREFIX "cleanup",    fft_cleanup,    NULL, NULL);
    Tcl_CreateObjCommand(interp, NS_PREFIX "execute",    fft_execute,    NULL, NULL);
    Tcl_CreateObjCommand(interp, NS_PREFIX "scale",      fft_scale,      NULL, NULL);
    Tcl_CreateObjCommand(interp, NS_PREFIX "clear",      fft_clear,      NULL, NULL);
    Tcl_CreateObjCommand(interp, NS_PREFIX "set",        fft_set,        NULL, NULL);
    Tcl_CreateObjCommand(interp, NS_PREFIX "setAt",      fft_setAt,      NULL, NULL);
    Tcl_CreateObjCommand(interp, NS_PREFIX "get",        fft_get,        NULL, NULL);
    Tcl_CreateObjCommand(interp, NS_PREFIX "getFlat",    fft_getFlat,    NULL, NULL);
    Tcl_CreateObjCommand(interp, NS_PREFIX "getFlatInt", fft_getFlatInt, NULL, NULL);
    Tcl_CreateObjCommand(interp, NS_PREFIX "getAt",      fft_getAt,      NULL, NULL);
    Tcl_CreateObjCommand(interp, NS_PREFIX "calcMcWaveform", calcMcWaveform, NULL, NULL);

    /* provide package */
    if (Tcl_PkgProvide(interp, "tclFFTW", "3.3.1") == TCL_ERROR)
        return TCL_ERROR;

    LEAVE("TCL_OK");
    return TCL_OK;
}
