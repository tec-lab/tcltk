#ifndef TCL_CONFIG_H
#define TCL_CONFIG_H

/*
 * Additional types exported to clients.
 */
#ifndef _TK
typedef char *Tk_Uid;
#endif

EXTERN Tk_Uid		Tk_GetUid _ANSI_ARGS_((const char *string));

/*
 * Structure used to specify how to handle argv options.
 */


/*
 * Legal values for the type field of a Tcl_ArgvInfo: see the user
 * documentation for details.
 */

#define TCL_ARGV_CONSTANT		15
#define TCL_ARGV_INT			16
#define TCL_ARGV_STRING			17
#define TCL_ARGV_UID			18
//#define TCL_ARGV_REST			19
//#define TCL_ARGV_FLOAT			20
//#define TCL_ARGV_FUNC			21
//#define TCL_ARGV_GENFUNC			22
//#define TCL_ARGV_HELP			23
#define TCL_ARGV_CONST_OPTION		24
#define TCL_ARGV_OPTION_VALUE		25
#define TCL_ARGV_OPTION_NAME_VALUE	26
//#define TCL_ARGV_END			27

/*
 * Flag bits for passing to Tcl_ParseArgv:
 */

#define TCL_ARGV_NO_DEFAULTS		0x1
#define TCL_ARGV_NO_LEFTOVERS		0x2
#define TCL_ARGV_NO_ABBREV		0x4
#define TCL_ARGV_DONT_SKIP_FIRST_ARG	0x8

/*
 * Structure used to describe application-specific configuration
 * options:  indicates procedures to call to parse an option and
 * to return a text string describing an option.
 */

typedef int (Tcl_OptionParseProc) _ANSI_ARGS_((ClientData clientData,
	Tcl_Interp *interp, const char *value, char *widgRec,
	int offset));
typedef char *(Tcl_OptionPrintProc) _ANSI_ARGS_((ClientData clientData,
	char *widgRec, int offset,
	Tcl_FreeProc **freeProcPtr));

typedef struct Tcl_CustomOption {
    Tcl_OptionParseProc *parseProc;	/* Procedure to call to parse an
					 * option and store it in converted
					 * form. */
    Tcl_OptionPrintProc *printProc;	/* Procedure to return a printable
					 * string describing an existing
					 * option. */
    ClientData clientData;		/* Arbitrary one-word value used by
					 * option parser:  passed to
					 * parseProc and printProc. */
} Tcl_CustomOption;

/*
 * Structure used to specify information for Tcl_ConfigureStruct.  Each
 * structure gives complete information for one option, including
 * how the option is specified on the command line, where it appears
 * in the option database, etc.
 */

typedef struct Tcl_ConfigSpec {
    int type;			/* Type of option, such as TCL_CONFIG_INT;
				 * see definitions below.  Last option in
				 * table must have type TCL_CONFIG_END. */
    char *argvName;		/* Switch used to specify option in argv.
				 * NULL means this spec is part of a group. */
    char *dbName;		/* Name for option in option database. */
    char *dbClass;		/* Class for option in database. */
    char *defValue;		/* Default value for option if not
				 * specified in command line or database. */
    int offset;			/* Where in Struct record to store value;
				 * use Tcl_Offset macro to generate values
				 * for this. */
    int specFlags;		/* Any combination of the values defined
				 * below;  other bits are used internally
				 * by tclConfig.c. */
    Tcl_CustomOption *customPtr;/* If type is TCL_CONFIG_CUSTOM then this is
				 * a pointer to info about how to parse and
				 * print the option.  Otherwise it is
				 * irrelevant. */
} Tcl_ConfigSpec;

/*
 * Type values for Tcl_ConfigSpec structures.  See the user
 * documentation for details.
 */

#define TCL_CONFIG_BOOLEAN	1
#define TCL_CONFIG_INT		2
#define TCL_CONFIG_DOUBLE	3
#define TCL_CONFIG_STRING	4
#define TCL_CONFIG_UID		5
#define TCL_CONFIG_SYNONYM	15
#define TCL_CONFIG_CUSTOM	21
#define TCL_CONFIG_END		22

/*
 * Macro to use to fill in "offset" fields of Tcl_ConfigInfos.
 * Computes number of bytes from beginning of structure to a
 * given field.
 */

#ifdef offsetof
#define Tcl_Offset(type, field) ((long int) offsetof(type, field))
#else
#define Tcl_Offset(type, field) ((long int) ((char *) &((type *) 0)->field))
#endif

/*
 * Possible values for flags argument to Tcl_ConfigureStruct:
 */

#define TCL_CONFIG_ARGV_ONLY	1

/*
 * Possible flag values for Tcl_ConfigInfo structures.  Any bits at
 * or above TCL_CONFIG_USER_BIT may be used by clients for selecting
 * certain entries.  Before changing any values here, coordinate with
 * tclConfig.c (internal-use-only flags are defined there).
 */

#define TCL_CONFIG_NULL_OK		4
#define TCL_CONFIG_DONT_SET_DEFAULT	8
#define TCL_CONFIG_OPTION_SPECIFIED	0x10
#define TCL_CONFIG_USER_BIT		0x100


int Tcl_ConfigureStruct(
     Tcl_Interp *interp		/* Interpreter for error reporting. */
    ,Tcl_ConfigSpec *specs	/* Describes legal options. */
    ,int argc			/* Number of elements in argv. */
    ,const char *argv[]	        /* Command-line options. */
    ,char *strucPtr		/* Record whose fields are to be
				 * modified.  Values must be properly
				 * initialized. */
    ,int flags			/* Used to specify additional flags
				 * that must be present in config specs
				 * for them to be considered.         */
);
void
Tcl_FreeOptions(
    Tcl_ConfigSpec *specs,	/* Describes legal options. */
    char *strucPtr,		/* Record whose fields contain current
				 * values for options. */
    int needFlags		/* Used to specify additional flags
				 * that must be present in config specs
				 * for them to be considered. */
);
#endif
