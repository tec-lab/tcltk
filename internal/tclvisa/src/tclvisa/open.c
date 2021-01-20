/*
 * open.c --
 *
 * This file is part of tclvisa library.
 *
 * Copyright (c) 2011 Andrey V. Nakin <andrey.nakin@gmail.com>
 * All rights reserved.
 *
 * See the file "COPYING" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <tcl.h>
#include "visa_channel.h"
#include "visa_utils.h"
#include "tcl_utils.h"
#include "tclvisa_utils.h"

int tclvisa_open(const ClientData clientData, Tcl_Interp* const interp, const int objc, Tcl_Obj* const objv[]) {
	VisaChannelData *rmSession, *channel;
	ViStatus status;
	ViSession vi;
	ViAccessMode accessMode = VI_NULL;
	ViUInt32 timeOut = VI_NULL;

	UNREFERENCED_PARAMETER(clientData);	/* avoid "unused parameter" warning */

	/* Check number of arguments */
	if (objc < 3 || objc > 5) {
		Tcl_WrongNumArgs(interp, 1, objv, "RMsession rsrcName ?accessMode? ?timeout?");
		return TCL_ERROR;
	}

	/* Convert first argument to valid Tcl channel reference */
    rmSession = getVisaChannelFromObj(interp, objv[1]);
	if (rmSession == NULL) {
		return TCL_ERROR;
	}

	if (objc > 3) {
		/* Access mode specified */
		long l;
		if (Tcl_GetLongFromObj(interp, objv[3], &l)) {
			return TCL_ERROR;
		}
		accessMode = (ViAccessMode) l;
	}

	if (objc > 4) {
		/* Timeout specified */
		long l;
		if (Tcl_GetLongFromObj(interp, objv[4], &l)) {
			return TCL_ERROR;
		}
		timeOut = (ViUInt32) l;
	}

	/* Attempt to open instrument session */
	status = viOpen(rmSession->session, TclGetString(objv[2]), accessMode, timeOut, &vi);
	storeLastError(rmSession, status, interp);

	/* Check status returned */
	if (status < 0) {
		return TCL_ERROR;
	}

	/* Create Tcl channel backed by instrument session */
	channel = createVisaChannel(interp, vi);
	if (NULL == channel) {
		return TCL_ERROR;
	}

	/* Return channel string representation as a procedure result */
	Tcl_AppendResult(interp, Tcl_GetChannelName(channel->channel), NULL);
	return TCL_OK;
}
