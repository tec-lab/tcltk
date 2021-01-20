/* 
 * stolen from tkConfig.c -- 
 *
 *	This file contains the Tcl_ConfigureStruct procedure.
 *
 * Copyright (c) 1990-1994 The Regents of the University of California.
 * Copyright (c) 1994-1995 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

static char sccsid[] = "@(#) tclConfig.c 1.51 95/05/26 14:59:53";

#include <stdlib.h>
#include <string.h>
#include "tcl.h"
#include "tclConfig.h"

/*
 * Values for "flags" field of Tcl_ConfigSpec structures.  Be sure
 * to coordinate these values with those defined in tcl.h
 * (TK_CONFIG_COLOR_ONLY, etc.).  There must not be overlap!
 *
 * INIT -		Non-zero means (char *) things have been
 *			converted to Tcl_Uid's.
 */

#define INIT		0x20

/*
 * Forward declarations for procedures defined later in this file:
 */

static int		DoConfig _ANSI_ARGS_((Tcl_Interp *interp,
			    Tcl_ConfigSpec *specPtr,
			    const char *value, int valueIsUid, char *strucPtr));
static Tcl_ConfigSpec *	FindConfigSpec _ANSI_ARGS_((Tcl_Interp *interp,
			    Tcl_ConfigSpec *specs, const char *argvName,
			    int needFlags, int hateFlags));
static char *		FormatConfigInfo _ANSI_ARGS_((Tcl_Interp *interp,
			    Tcl_ConfigSpec *specPtr,
			    char *strucPtr));
static char *		FormatConfigValue _ANSI_ARGS_((Tcl_Interp *interp,
			    Tcl_ConfigSpec *specPtr,
			    char *strucPtr, char *buffer,
			    Tcl_FreeProc **freeProcPtr));

/*
 *--------------------------------------------------------------
 *
 * Tcl_ConfigureStruct --
 *
 *	Process command-line options and database options to
 *	fill in fields of a widget record with resources and
 *	other parameters.
 *
 * Results:
 *	A standard Tcl return value.  In case of an error,
 *	interp->result will hold an error message.
 *
 * Side effects:
 *	The fields of strucPtr get filled in with information
 *	from argc/argv and the option database.  Old information
 *	in strucPtr's fields gets recycled.
 *
 *--------------------------------------------------------------
 */

int
Tcl_ConfigureStruct(interp, specs, argc, argv, strucPtr, flags)
    Tcl_Interp *interp;		/* Interpreter for error reporting. */
    Tcl_ConfigSpec *specs;	/* Describes legal options. */
    int argc;			/* Number of elements in argv. */
    const char *argv[];		/* Command-line options. */
    char *strucPtr;		/* Record whose fields are to be
				 * modified.  Values must be properly
				 * initialized. */
    int flags;			/* Used to specify additional flags
				 * that must be present in config specs
				 * for them to be considered.         */
{
    register Tcl_ConfigSpec *specPtr;
    //Tk_Uid value;		/* Value of option from database. */
    int needFlags;		/* Specs must contain this set of flags
				 * or else they are not considered. */
    int hateFlags=0;		/* If a spec contains any bits here, it's
				 * not considered. */

    needFlags = flags & ~(TCL_CONFIG_USER_BIT - 1);

    /*
     * Pass one:  scan through all the option specs, replacing strings
     * with Tk_Uids (if this hasn't been done already) and clearing
     * the TCL_CONFIG_OPTION_SPECIFIED flags.
     */

    for (specPtr = specs; specPtr->type != TCL_CONFIG_END; specPtr++) {
      if (!(specPtr->specFlags & INIT) && (specPtr->argvName != NULL)) {
        if (specPtr->dbName != NULL) {
		specPtr->dbName = Tk_GetUid(specPtr->dbName);
	    }
	    if (specPtr->dbClass != NULL) {
		specPtr->dbClass = Tk_GetUid(specPtr->dbClass);
	    }
	    if (specPtr->defValue != NULL) {
		specPtr->defValue = Tk_GetUid(specPtr->defValue);
	    }
	}
	specPtr->specFlags = (specPtr->specFlags & ~TCL_CONFIG_OPTION_SPECIFIED)
		| INIT;
    }

    /*
     * Pass two:  scan through all of the arguments, processing those
     * that match entries in the specs.
     */

    for ( ; argc > 0; argc -= 2, argv += 2) {
	specPtr = FindConfigSpec(interp, specs, *argv, needFlags, hateFlags);
	if (specPtr == NULL) {
	    return TCL_ERROR;
	}

	/*
	 * Process the entry.
	 */

	if (argc < 2) {
	    Tcl_AppendResult(interp, "value for \"", *argv,
		    "\" missing", (char *) NULL);
	    return TCL_ERROR;
	}
	if (DoConfig(interp, specPtr, argv[1], 0, strucPtr) != TCL_OK) {
	    char msg[100];

	    sprintf(msg, "\n    (processing \"%.40s\" option)",
		    specPtr->argvName);
	    Tcl_AddErrorInfo(interp, msg);
	    return TCL_ERROR;
	}
	specPtr->specFlags |= TCL_CONFIG_OPTION_SPECIFIED;
    }


    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * FindConfigSpec --
 *
 *	Search through a table of configuration specs, looking for
 *	one that matches a given argvName.
 *
 * Results:
 *	The return value is a pointer to the matching entry, or NULL
 *	if nothing matched.  In that case an error message is left
 *	in interp->result.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

static Tcl_ConfigSpec *
FindConfigSpec(interp, specs, argvName, needFlags, hateFlags)
    Tcl_Interp *interp;		/* Used for reporting errors. */
    Tcl_ConfigSpec *specs;	/* Pointer to table of configuration
				 * specifications for a Struct. */
    const char *argvName;	/* Name (suitable for use in a "config"
				 * command) identifying particular option. */
    int needFlags;		/* Flags that must be present in matching
				 * entry. */
    int hateFlags;		/* Flags that must NOT be present in
				 * matching entry. */
{
    register Tcl_ConfigSpec *specPtr;
    register char c;		/* First character of current argument. */
    Tcl_ConfigSpec *matchPtr;	/* Matching spec, or NULL. */
    size_t length;

    c = argvName[1];
    length = strlen(argvName);
    matchPtr = NULL;
    for (specPtr = specs; specPtr->type != TCL_CONFIG_END; specPtr++) {
	if (specPtr->argvName == NULL) {
	    continue;
	}
	if ((specPtr->argvName[1] != c)
		|| (strncmp(specPtr->argvName, argvName, length) != 0)) {
	    continue;
	}
	if (((specPtr->specFlags & needFlags) != needFlags)
		|| (specPtr->specFlags & hateFlags)) {
	    continue;
	}
	if (specPtr->argvName[length] == 0) {
	    matchPtr = specPtr;
	    goto gotMatch;
	}
	if (matchPtr != NULL) {
	    Tcl_AppendResult(interp, "ambiguous option \"", argvName,
		    "\"", (char *) NULL);
	    return (Tcl_ConfigSpec *) NULL;
	}
	matchPtr = specPtr;
    }

    if (matchPtr == NULL) {
	Tcl_AppendResult(interp, "unknown option \"", argvName,
		"\"", (char *) NULL);
	return (Tcl_ConfigSpec *) NULL;
    }

    /*
     * Found a matching entry.  If it's a synonym, then find the
     * entry that it's a synonym for.
     */

    gotMatch:
    specPtr = matchPtr;
    if (specPtr->type == TCL_CONFIG_SYNONYM) {
	for (specPtr = specs; ; specPtr++) {
	    if (specPtr->type == TCL_CONFIG_END) {
		Tcl_AppendResult(interp,
			"couldn't find synonym for option \"",
			argvName, "\"", (char *) NULL);
		return (Tcl_ConfigSpec *) NULL;
	    }
	    if ((specPtr->dbName == matchPtr->dbName) 
		    && (specPtr->type != TCL_CONFIG_SYNONYM)
		    && ((specPtr->specFlags & needFlags) == needFlags)
		    && !(specPtr->specFlags & hateFlags)) {
		break;
	    }
	}
    }
    return specPtr;
}

/*
 *--------------------------------------------------------------
 *
 * DoConfig --
 *
 *	This procedure applies a single configuration option
 *	to a Struct record.
 *
 * Results:
 *	A standard Tcl return value.
 *
 * Side effects:
 *	strucPtr is modified as indicated by specPtr and value.
 *	The old value is recycled, if that is appropriate for
 *	the value type.
 *
 *--------------------------------------------------------------
 */

static int
DoConfig(interp,  specPtr, value, valueIsUid, strucPtr)
    Tcl_Interp *interp;		/* Interpreter for error reporting. */
    Tcl_ConfigSpec *specPtr;	/* Specifier to apply. */
    const char *value;		/* Value to use to fill in strucPtr. */
    int valueIsUid;		/* Non-zero means value is a Tk_Uid;
				 * zero means it's an ordinary string. */
    char *strucPtr;		/* Record whose fields are to be
				 * modified.  Values must be properly
				 * initialized. */
{
    char *ptr;
    Tk_Uid uid;
    int nullValue;

    nullValue = 0;
    if ((*value == 0) && (specPtr->specFlags & TCL_CONFIG_NULL_OK)) {
	nullValue = 1;
    }

    do {
	ptr = strucPtr + specPtr->offset;
	switch (specPtr->type) {
	    case TCL_CONFIG_BOOLEAN:
		if (Tcl_GetBoolean(interp, value, (int *) ptr) != TCL_OK) {
		    return TCL_ERROR;
		}
		break;
	    case TCL_CONFIG_INT:
		if (Tcl_GetInt(interp, value, (int *) ptr) != TCL_OK) {
		    return TCL_ERROR;
		}
		break;
	    case TCL_CONFIG_DOUBLE:
		if (Tcl_GetDouble(interp, value, (double *) ptr) != TCL_OK) {
		    return TCL_ERROR;
		}
		break;
	    case TCL_CONFIG_STRING: {
		char *old, *new;

		if (nullValue) {
		    new = NULL;
		} else {
		    new = (char *) ckalloc((unsigned) (strlen(value) + 1));
		    strcpy(new, value);
		}
		old = *((char **) ptr);
		if (old != NULL) {
		    ckfree(old);
		}
		*((char **) ptr) = new;
		break;
	    }
	    case TCL_CONFIG_UID: 
		if (nullValue) {
		    *((Tk_Uid *) ptr) = NULL;
		} else {
		    uid = valueIsUid ? (Tk_Uid) value : Tk_GetUid(value);
		    *((Tk_Uid *) ptr) = uid;
		}
		break;
	    case TCL_CONFIG_CUSTOM:
		if ((*specPtr->customPtr->parseProc)(
			specPtr->customPtr->clientData, interp,
			value, strucPtr, specPtr->offset) != TCL_OK) {
		    return TCL_ERROR;
		}
		break;
	    default: {
              char temp[255];
              sprintf(temp, "bad config table: unknown type %d", 
                      specPtr->type);
              Tcl_ResetResult(interp);
              Tcl_SetResult(interp, temp, TCL_VOLATILE);
              return TCL_ERROR;
	    }
	}
	specPtr++;
    } while ((specPtr->argvName == NULL) && (specPtr->type != TCL_CONFIG_END));
    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * Tcl_ConfigureInfo --
 *
 *	Return information about the configuration options
 *	for a window, and their current values.
 *
 * Results:
 *	Always returns TCL_OK.  Interp->result will be modified
 *	hold a description of either a single configuration option
 *	available for "strucPtr" via "specs", or all the configuration
 *	options available.  In the "all" case, the result will
 *	available for "strucPtr" via "specs".  The result will
 *	be a list, each of whose entries describes one option.
 *	Each entry will itself be a list containing the option's
 *	name for use on command lines, database name, database
 *	class, default value, and current value (empty string
 *	if none).  For options that are synonyms, the list will
 *	contain only two values:  name and synonym name.  If the
 *	"name" argument is non-NULL, then the only information
 *	returned is that for the named argument (i.e. the corresponding
 *	entry in the overall list is returned).
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

int
Tcl_ConfigureInfo(interp,  specs, strucPtr, argvName, flags)
    Tcl_Interp *interp;		/* Interpreter for error reporting. */
    Tcl_ConfigSpec *specs;	/* Describes legal options. */
    char *strucPtr;		/* Record whose fields contain current
				 * values for options. */
    char *argvName;		/* If non-NULL, indicates a single option
				 * whose info is to be returned.  Otherwise
				 * info is returned for all options. */
    int flags;			/* Used to specify additional flags
				 * that must be present in config specs
				 * for them to be considered. */
{
    register Tcl_ConfigSpec *specPtr;
    int needFlags, hateFlags=0;
    char *list;
    char *leader = "{";

    needFlags = flags & ~(TCL_CONFIG_USER_BIT - 1);
 
    /*
     * If information is only wanted for a single configuration
     * spec, then handle that one spec specially.
     */

    Tcl_SetResult(interp, (char *) NULL, TCL_STATIC);
    if (argvName != NULL) {
	specPtr = FindConfigSpec(interp, specs, argvName, needFlags,
		hateFlags);
	if (specPtr == NULL) {
	    return TCL_ERROR;
	}
        Tcl_ResetResult(interp);
        Tcl_SetResult(interp, FormatConfigInfo(interp,  specPtr, strucPtr), TCL_DYNAMIC);

	//interp->result = FormatConfigInfo(interp,  specPtr, strucPtr);
	//interp->freeProc = TCL_DYNAMIC;
	return TCL_OK;
    }

    /*
     * Loop through all the specs, creating a big list with all
     * their information.
     */

    for (specPtr = specs; specPtr->type != TCL_CONFIG_END; specPtr++) {
	if ((argvName != NULL) && (specPtr->argvName != argvName)) {
	    continue;
	}
	if (((specPtr->specFlags & needFlags) != needFlags)
		|| (specPtr->specFlags & hateFlags)) {
	    continue;
	}
	if (specPtr->argvName == NULL) {
	    continue;
	}
	list = FormatConfigInfo(interp, specPtr, strucPtr);
	Tcl_AppendResult(interp, leader, list, "}", (char *) NULL);
	ckfree(list);
	leader = " {";
    }
    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * FormatConfigInfo --
 *
 *	Create a valid Tcl list holding the configuration information
 *	for a single configuration option.
 *
 * Results:
 *	A Tcl list, dynamically allocated.  The caller is expected to
 *	arrange for this list to be freed eventually.
 *
 * Side effects:
 *	Memory is allocated.
 *
 *--------------------------------------------------------------
 */

static char *
FormatConfigInfo(interp, specPtr, strucPtr)
    Tcl_Interp *interp;			/* Interpreter to use for things
					 * like floating-point precision. */
    register Tcl_ConfigSpec *specPtr;	/* Pointer to information describing
					 * option. */
    char *strucPtr;			/* Pointer to record holding current
					 * values of info for Struct. */
{
    char *argv[6], *result;
    char buffer[200];
    Tcl_FreeProc *freeProc = (Tcl_FreeProc *) NULL;

    argv[0] = specPtr->argvName;
    argv[1] = specPtr->dbName;
    argv[2] = specPtr->dbClass;
    argv[3] = specPtr->defValue;
    if (specPtr->type == TCL_CONFIG_SYNONYM) {
      return Tcl_Merge(2, (const char *const*)argv);
    }
    argv[4] = FormatConfigValue(interp, specPtr, strucPtr, buffer,
	    &freeProc);
    if (argv[1] == NULL) {
	argv[1] = "";
    }
    if (argv[2] == NULL) {
	argv[2] = "";
    }
    if (argv[3] == NULL) {
	argv[3] = "";
    }
    if (argv[4] == NULL) {
	argv[4] = "";
    }
    result = Tcl_Merge(5, (const char *const*)argv);
    if (freeProc != NULL) {
	if ((freeProc == TCL_DYNAMIC) || (freeProc == (Tcl_FreeProc *) free)) {
	    ckfree(argv[4]);
	} else {
	    (*freeProc)(argv[4]);
	}
    }
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * FormatConfigValue --
 *
 *	This procedure formats the current value of a configuration
 *	option.
 *
 * Results:
 *	The return value is the formatted value of the option given
 *	by specPtr and strucPtr.  If the value is static, so that it
 *	need not be freed, *freeProcPtr will be set to NULL;  otherwise
 *	*freeProcPtr will be set to the address of a procedure to
 *	free the result, and the caller must invoke this procedure
 *	when it is finished with the result.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static char *
FormatConfigValue(interp, specPtr, strucPtr, buffer, freeProcPtr)
    Tcl_Interp *interp;		/* Interpreter for use in real conversions. */
    Tcl_ConfigSpec *specPtr;	/* Pointer to information describing option.
				 * Must not point to a synonym option. */
    char *strucPtr;		/* Pointer to record holding current
				 * values of info for Struct. */
    char *buffer;		/* Static buffer to use for small values.
				 * Must have at least 200 bytes of storage. */
    Tcl_FreeProc **freeProcPtr;	/* Pointer to word to fill in with address
				 * of procedure to free the result, or NULL
				 * if result is static. */
{
    char *ptr, *result;

    *freeProcPtr = NULL;
    ptr = strucPtr + specPtr->offset;
    result = "";
    switch (specPtr->type) {
	case TCL_CONFIG_BOOLEAN:
	    if (*((int *) ptr) == 0) {
		result = "0";
	    } else {
		result = "1";
	    }
	    break;
	case TCL_CONFIG_INT:
	    sprintf(buffer, "%d", *((int *) ptr));
	    result = buffer;
	    break;
	case TCL_CONFIG_DOUBLE:
	    Tcl_PrintDouble(interp, *((double *) ptr), buffer);
	    result = buffer;
	    break;
	case TCL_CONFIG_STRING:
	    result = (*(char **) ptr);
	    if (result == NULL) {
		result = "";
	    }
	    break;
	case TCL_CONFIG_UID: {
	    Tk_Uid uid = *((Tk_Uid *) ptr);
	    if (uid != NULL) {
		result = uid;
	    }
	    break;
	}
	case TCL_CONFIG_CUSTOM:
	    result = (*specPtr->customPtr->printProc)(
		    specPtr->customPtr->clientData, strucPtr,
		    specPtr->offset, freeProcPtr);
	    break;
	default: 
	    result = "?? unknown type ??";
    }
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_ConfigureValue --
 *
 *	This procedure returns the current value of a configuration
 *	option for a Struct.
 *
 * Results:
 *	The return value is a standard Tcl completion code (TCL_OK or
 *	TCL_ERROR).  Interp->result will be set to hold either the value
 *	of the option given by argvName (if TCL_OK is returned) or
 *	an error message (if TCL_ERROR is returned).
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_ConfigureValue(interp, specs, strucPtr, argvName, flags)
    Tcl_Interp *interp;		/* Interpreter for error reporting. */
    Tcl_ConfigSpec *specs;	/* Describes legal options. */
    char *strucPtr;		/* Record whose fields contain current
				 * values for options. */
    const char *argvName;	/* Gives the command-line name for the
				 * option whose value is to be returned. */
    int flags;			/* Used to specify additional flags
				 * that must be present in config specs
				 * for them to be considered. */
{
    Tcl_ConfigSpec *specPtr;
    int needFlags, hateFlags=0;

    needFlags = flags & ~(TCL_CONFIG_USER_BIT - 1);
    specPtr = FindConfigSpec(interp, specs, argvName, needFlags, hateFlags);
    if (specPtr == NULL) {
	return TCL_ERROR;
    }
    Tcl_ResetResult(interp);
    char temp[255];
    Tcl_FreeProc *freeProc = TCL_VOLATILE;
    Tcl_SetResult(interp, FormatConfigValue(interp, specPtr, strucPtr,
          &temp[0], &freeProc), freeProc);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_FreeOptions --
 *
 *	Free up all resources associated with configuration options.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Any resource in strucPtr that is controlled by a configuration
 *	option (e.g. a Tk_3DBorder or XColor) is freed in the appropriate
 *	fashion.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
void
Tcl_FreeOptions(specs, strucPtr, needFlags)
    Tcl_ConfigSpec *specs;	/* Describes legal options. */
    char *strucPtr;		/* Record whose fields contain current
				 * values for options. */
    int needFlags;		/* Used to specify additional flags
				 * that must be present in config specs
				 * for them to be considered. */
{
    register Tcl_ConfigSpec *specPtr;
    char *ptr;

    for (specPtr = specs; specPtr->type != TCL_CONFIG_END; specPtr++) {
	if ((specPtr->specFlags & needFlags) != needFlags) {
	    continue;
	}
        printf( ".");fflush(0);
	ptr = strucPtr + specPtr->offset;
	switch (specPtr->type) {
	    case TCL_CONFIG_STRING:
		if (*((char **) ptr) != NULL) {
                  printf("%li %li ",(long int)(specPtr-specs+1),(long int)ptr);fflush(0);
		    ckfree(*((char **) ptr));
		     printf("- ");fflush(0);
		    *((char **) ptr) = NULL;
		    printf( " - \n");
	}
		break;
	}
    }
    printf( "\n");
}
