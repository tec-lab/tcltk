#ifndef HPIB_CORE_H
#define HPIB_CORE_H

/*%%**************************************************************************
%% Comment
   includes required.
%%***************************************************************************/

#include <sys/types.h>
#ifdef _WINDOWS
#define MAXNAMLEN 1024
#else
//#include <dir.h>
#endif
#include <string.h>

#include "tcl.h"
#include "sicl.h"
#include "tclConfig.h"

/*%%**************************************************************************
%% Comment
   defines for managing allocation of public data/procedures
%%***************************************************************************/
#ifndef HPIB_EXPORT
#define HPIB_EXPORT EXTERN
#endif
#ifndef SRQ_EXPORT
#define SRQ_EXPORT EXTERN
#endif

/*%%**************************************************************************
%% Comment
 *  Additional HPIB constants not defined in sicl.h
%%***************************************************************************/
#define HPIB_RESULT_BUFFER_SIZE 1024*1024

#define min(a,b) ((a) <= (b) ? (a) : (b))    

#define HPIB_DEFAULT_TIMEOUT    5000    /* ms */

#define N_HPIB_INTERFACES       4       /* Number of interfaces */
#define N_HPIB_DEVICES          30      /* Number of devices per interface */

#define SRQ_STATUS_MASK_OLD         0x70    /* Request Service +
					       Standard Event +
					       Message Available */
#define SRQ_STATUS_MASK         0x40    /* Request Service */

/*%%**************************************************************************
%% Comment
 *  HPIB management info for Tcl HPIB commands
%%***************************************************************************/

typedef struct Hpib_Info {

    Tcl_Interp *main;          /* interpreter that manages this info */
    Tcl_Command hpibCmd;       /* Token for object's tcl command. */

    INST  dev_inst;            /* SICL device session instance */
    char *interface;           /* Name of associated interface, including devnum */
    int   interface_active;    /* indicates wheter the interface is active */
    int   termchar;            /* command termination char /  -1 = EOI */
    long  timeout;             /* I/O timeout in ms (HPIB) */
    long  lan_timeout;         /* I/O timeout in ms (LAN Gateway)*/

    int   srq_poll_interval;   /* <= 0 means async SRQ via signal, otherwise
				  SRQ poll intervall in ms */
    int   srq_created;         /* flag */
    int   srq_enabled;         /* flag */
    int   srq_pending;         /* flag */
    int   poll_status;         /* result of HPIB poll */
    int   srq_priority;        /* MIN_PRIORITY <= srq_priority <= MAX_PRIORITY 
                                  lower values have higher priorities 
                                  (see hpib_lib.h)                           */
    char *srq_command;           /* tcl command to invoke for SRQ */

    char  stringbuf[MAXNAMLEN]; /*temporary string data*/

} Hpib_Info;

typedef struct SRQ_Interface_Entry {

    INST    interface_inst;  
    char    interface_name[MAXNAMLEN];
    int     device_use; /** number of devices using this interface for SRQ */

} SRQ_Interface_Entry;

typedef struct SRQ_Device_Entry {

    INST    dev_inst;    
    int     interface;  
    int     priority;
    void  (*service_routine) (Hpib_Info *);
    Hpib_Info   *client_data;

} SRQ_Device_Entry;

/*%%**************************************************************************
%% Comment
   Tcl procedure for initialising HPIB support
%%***************************************************************************/
HPIB_EXPORT int Hpib_Init(Tcl_Interp *interp);

HPIB_EXPORT int Hpib_retry_iopen(Tcl_Interp *interp, Hpib_Info *info);
/*%%**************************************************************************
%% Comment
   Tcl command  procedure for configuring  SRQs
%%***************************************************************************/
SRQ_EXPORT int Hpib_SrqConfig(ClientData clientdata, Tcl_Interp *interp, 
			   int argc, const char* argv[]);

/*%%**************************************************************************
%% Comment
   Tcl Async Handler procedure for asynchronous SRQ requests and
   associated handler token
%%***************************************************************************/
SRQ_EXPORT int Hpib_TclSrqProc _ANSI_ARGS_((ClientData clientData,
					 Tcl_Interp *interp,
					 int code));
SRQ_EXPORT Tcl_AsyncHandler  Hpib_Handle;
/* SRQ_EXPORT Tcl_AsyncHandler  Hpib_PollSRQHandle; */

/*%%**************************************************************************
%% Comment
   error handler used during SRQ processing 
   (normally NULL or Tk_BackgroundError)
%%***************************************************************************/

SRQ_EXPORT void (*tclSrqBackgroundError) _ANSI_ARGS_((Tcl_Interp *interp));
#endif

