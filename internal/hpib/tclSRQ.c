#define MODULE 8002
/*%%******************* M O D U L E   H E A D E R ****************************
%% BeginOfModule
   tclSRQ.c
%% Identification
   SOU/MSG/RF_SCOE/010102-T-0002/SOE ????
%% Type
   C source file
%% Project
   MSG-RF-SCOE
%% Subsystem
   SCOE-APPLICATION-SW
%% File
   $Source: /cmdata/cvs/Tcl-Packages/tclHPIB/tclSRQ.c,v $
%% VersionAndStatus
   $Revision: 1.8 $ $State: Exp $
%% CompilerOS
   ANSI-C 8.0, HP-UX 8.0
%% AuthorAndDate
   O. Maenner, IVP1,  1996/01/10
%% Copyright
   (c) by SIEMENS Austria, all rights reserved, confidental.
%% HistoryOfChange
   $Log: tclSRQ.c,v $
   Revision 1.8  2010/08/04 09:03:27  maenner.o
   adapted Makefile for tcl8.5

   Revision 1.7  2003/06/26 12:38:41  cryo
   tclHPIB for windows and unix

   Revision 1.1  2003/06/26 11:53:03  cryo
   tclHPIB

   Revision 1.6  2002/10/04 14:20:50  cryo
   extended delete info

   Revision 1.5  2002/07/10 18:00:23  gras
   OM: Compilation  and installation of 2v2 under Windows now OK again

   Revision 1.4  2001/08/17 14:09:37  gras
   OM: basic functionality of vxi seems to work under Linux

   Revision 1.3  2001/08/17 12:22:12  gras
   tclHPIB v 2.0 / SRQ polling

   Revision 1.2  1999/12/13 13:27:44  gras
   {OM} V1.3 , compiles OK under HP-UX, timeout calc. with cast to long

   Revision 1.1.1.1  1999/11/22 10:34:15  tcl
   Import of dir. structure

   Revision 1.2  1999/10/19 14:55:15  mo-cm
   {OM} added stubs interface for remaining packages

   Revision 1.1.1.1  1998/10/14 11:05:06  mo-cm
   Imported sources

   Revision 1.5  1997/10/17 18:49:12  msg-cm
   [OM] Tcl SRQ only executed when Bit 6 in STB

   Revision 1.4  1997/07/10 06:18:37  msg-cm
   removing error message for srq delete; version based on OM files

   Revision 1.3  1997/06/02 08:52:17  msg-cm
   * converted to SICL

   Revision 1.2  1996/01/11 19:18:55  maenner
   * moved srq command code to tclSRQ.c
   * dmm_srq.tcl: measurement is restarted properly if timeout (occurs also
     with Robert's test routine, possible fault in 7150 multimeter -- needs
     a device clear)

 * Revision 1.1  1996/01/11  14:04:09  maenner
 * newly added
 *
%% Purpose
   routines allowing Tcl to handle HPIB SRQ requests 
%% Resources
*/
#define SRQ_EXPORT     /* allocate SRQ data/procedures here */

#include "tclHPIB.h"
#include "tclConfig.h"

#include <time.h>
 
/*
%% GlobalData --- see tclHPIB.h
*/
extern int Hpib_srq_debug;  /* SRQ debbuging flag*/
extern int Hpib_srq_poll_interval; /* polling interval in microseconds */

int Hpib_TclPollProc (ClientData  clientData, 
		      Tcl_Interp *interp, int cmdResultCode); /* forward declaration */
/*
%% Data 
*/


/*%%**************************************************************************
%% Comment
   configure specification for hpib srq command
%%***************************************************************************/

static Tcl_ConfigSpec srqConfig[] = {
    {TCL_CONFIG_INT, "-priority", "priority", "Priority", 
	0, Tcl_Offset(Hpib_Info, srq_priority), 0},
    {TCL_CONFIG_STRING, "-command", "command", "Command",
	"", Tcl_Offset(Hpib_Info, srq_command),
        TCL_CONFIG_NULL_OK},
    {TCL_CONFIG_END, (char *) NULL, (char *) NULL, (char *) NULL,
	(char *) NULL, 0, 0}
};


/*
 * Structure used to save error state of the interpreter.
 */
typedef struct {
    char  *result;
    char  *errorInfo;
    char  *errorCode;
} errState_t;

/*
 * Table containing a interpreters and their Async handler cookie.
 */
typedef struct {
    Tcl_Interp       *interp;
    Tcl_AsyncHandler  handler;
} interpHandler_t;

static interpHandler_t *interpTable = NULL;
static int              interpTableSize  = 0;
static int              numInterps  = 0;

static int SrqActive=0;  /* set when inside Hpib_SrqProc to avoid recursion */
static int PollActive=0; /* set when inside Hpib_PollProc to avoid recursion */

static int                  srqNextSchedule = 0;
static Tcl_TimerToken       Srq_pollTimerHandle = 0;

static SRQ_Interface_Entry  srq_interfaces[N_HPIB_INTERFACES];
static SRQ_Device_Entry     srq_queue[N_HPIB_INTERFACES * N_HPIB_DEVICES];

#define WRAP_IDX(x)         ((x) % (N_HPIB_INTERFACES * N_HPIB_DEVICES))

extern int Tcl_ConfigureInfo(Tcl_Interp *interp,
			     Tcl_ConfigSpec *specs, 
			     char *strucPtr, 
			     char *argvName, int flags);
extern int Tcl_ConfigureValue(Tcl_Interp *interp,
			      Tcl_ConfigSpec *specs, 
			      char *strucPtr, 
			      const char *argvName, int flags);
/*
%% InternalFunctions 
*/
static int          EvalSrqCode (Tcl_Interp *interp,  Hpib_Info* info);
static int          FormatSrqCode (Tcl_Interp  *interp, Hpib_Info* info, int pending,
                                   Tcl_DString *command);
static errState_t  *SaveErrorState (Tcl_Interp *interp);
static void         RestoreErrorState (Tcl_Interp *interp, errState_t *errStatePtr);
static void         PollTimer(ClientData clientData);

/*  BEGIN PROCEDURE ********************************************************
//
//  PROCEDURE NAME: Hpib_InitSRQQueue
//
//  PARAMETERS:     None
//
//  RETURN VALUE:   None
//
//  PURPOSE:        Initialize SRQ queue.
//
//  END PROCEDURE *********************************************************/

void Hpib_InitSRQQueue ()
{
    int     idx;

    if (Srq_pollTimerHandle!=0) {
	/* delete timer */
	Tcl_DeleteTimerHandler(Srq_pollTimerHandle);
    }

    /* Clear all interfaces in the SRQ queue */
    for (idx = 0; idx < (N_HPIB_INTERFACES); ++idx) {
	srq_interfaces[idx].interface_inst=0;
	srq_interfaces[idx].interface_name[0]='\0';
	srq_interfaces[idx].device_use=0;
    }

    /* Clear all entries in the SRQ queue */
    for (idx = 0; idx < (N_HPIB_INTERFACES * N_HPIB_DEVICES); ++idx)
    {
        srq_queue[idx].dev_inst  = 0;
	srq_queue[idx].interface = -1;
    }
}

/*  BEGIN PROCEDURE ********************************************************
//
//  PROCEDURE NAME: Hpib_AddSRQEntry
//
//  PARAMETERS:     device      SICL instance of the device
//                  priority    SRQ priority (non-negative int)
//                  callback    Pointer to callback procedure
//                  param       Pointer to device info structure
//
//  RETURN VALUE:   Completion code
//                      == 0    SRQ added successfully
//                      <  0    No more slots in SRQ queue
//
//  PURPOSE:        Inserts a new SRQ callback into the SRQ queue.
//
//      Searches from the beginning of the queue for an emty slot
//      (sicl instance == 0). If an emty slot is found, copy the
//      parameters, 'device' must be copied last to prevent the SRQ
//      scheduler to use the slot before all entries are valid.
//      Return error flag if no more empty slot found.
//
//  END PROCEDURE *********************************************************/

int Hpib_AddSRQEntry (INST device, const int priority,
                      void (*callback) (Hpib_Info *), Hpib_Info *param)
{
    int     idx,len_ifname,ifx,ret;
    unsigned int c;
    Tcl_Interp *interp;
    Hpib_Info *info=param;
    interp = info->main;

    /* Scan SRQ queue for empty slot */
    for (idx = 0; idx < (N_HPIB_INTERFACES * N_HPIB_DEVICES); ++idx)  {
        /* Is this an empty slot, then add entry */
        if (srq_queue[idx].dev_inst == 0)         {
            /* Copy data into SRQ queue */
            srq_queue[idx].priority        = priority;
            srq_queue[idx].service_routine = callback;
            srq_queue[idx].client_data     = param;
	    srq_queue[idx].interface = -1;


	    if ( info->srq_poll_interval>0 ) {
		/* doing SRQ via polling */
		/* try to find associated interfacee session */
		for(c=0; c<strlen(param->interface); c++) {   /* scan until ',' (hpib,12 -> hpib) */
		    if(param->interface[c] == ',')  break;
		}
		/* we are at the end ot the interface name */
		len_ifname = c;
		for (ifx = 0; ifx < N_HPIB_INTERFACES; ++ifx) {
		    if (strncmp(param->interface,srq_interfaces[ifx].interface_name,len_ifname)==0) {
			srq_queue[idx].interface = ifx;
			srq_interfaces[ifx].device_use++;
		    }
		}
		if (srq_queue[idx].interface<0) {
		    /* interface not yet used , search for first free slot */
		    for (ifx = 0; ifx < N_HPIB_INTERFACES; ++ifx) {
			if (srq_interfaces[ifx].device_use <=0) {
			    strncpy(srq_interfaces[ifx].interface_name,param->interface,len_ifname);
			    srq_interfaces[ifx].interface_name[len_ifname]='\0';
			    
			    /* open interface session */
			    if (Hpib_srq_debug) {
			      fprintf (stderr, "Opening interface session to %s\n",
				       srq_interfaces[ifx].interface_name); 
			      fflush(stderr);
			    } 
    
			    if ((srq_interfaces[ifx].interface_inst = iopen (srq_interfaces[ifx].interface_name)) == 0)   {
				ret = igeterrno ();
				Tcl_ResetResult (interp);
				Tcl_AppendResult (interp,
						  Tcl_GetCommandName (interp, info->hpibCmd),
						  " Hpib_AddSRQEntry:",
						  igeterrstr (ret),
						  NULL);
				if (ret==I_ERR_NOCONN) {
				    /* hpib gateway may have timed out, try to open anew */
				    if (Hpib_srq_debug || 1) {
					fprintf(stderr,"HPIB device %s (%s) error: %s\n",
						Tcl_GetCommandName (interp, info->hpibCmd),
						info->interface,
						igeterrstr (ret));
				    }
				    iclose(info->dev_inst); /* this may generate an error */
				}
				if ((info->dev_inst = iopen (srq_interfaces[ifx].interface_name)) != 0)  {
				    /* all went OK */
				    ret=0;
				} else {
				    ret = igeterrno ();
				    Tcl_AppendResult (interp,
						      "\n and could not open anew: ",
						      igeterrstr (ret),
						      NULL);
				}
				if (ret) {	    
				/* interface could not be opened again */
				    
				    Tcl_SetErrorCode (interp, "HPIB", "OPEN", NULL);
				    return (TCL_ERROR); 
				}
			    }
			    
			    
			    /* everything OK, mark as in use */ 
			    srq_queue[idx].interface = ifx;
			    srq_interfaces[ifx].device_use=1;
			    break;
			}
		    }
		}
		
		if (srq_queue[idx].interface<0) {
		    /* no free interface found or iopen error  */
		    Tcl_ResetResult (interp);
		    Tcl_AppendResult (interp,
				      Tcl_GetCommandName (interp, info->hpibCmd),
				      " Hpib_AddSRQEntry: no free slot in srq_interfaces[]",
				      NULL);
		    return (TCL_ERROR); 
		}

		/**** register an event timer if not done up to now ***/
		if (Srq_pollTimerHandle == 0) {
		    Srq_pollTimerHandle = Tcl_CreateTimerHandler(Hpib_srq_poll_interval,
								 PollTimer,
								 (int *)info);
		}
	    }


            /* Set entry valid */
            srq_queue[idx].dev_inst  = device;
            return (0);
        }
    }

    /* No free entry found - error */
    Tcl_ResetResult (interp);
    Tcl_AppendResult (interp,
		      Tcl_GetCommandName (interp, info->hpibCmd),
		      " Hpib_AddSRQEntry: no free slot in srq_queue[]",
		      NULL);
    return (TCL_ERROR); 

}

/*  BEGIN PROCEDURE ********************************************************
//
//  PROCEDURE NAME: Hpib_RemoveSRQEntry
//
//  PARAMETERS:     device      SICL instance of device to be removed
//
//  RETURN VALUE:   Completion code
//                      == 0    Entry removed
//                      <  0    No such entry in SRQ queue
//
//  PURPOSE:        Remove an existing entry from the SRQ queue.
//
//  END PROCEDURE *********************************************************/

int Hpib_RemoveSRQEntry (INST device)
{
    int     idx,ret,ifx;
    int err = 0;

    /* Scan SRQ queue for empty slot */
    for (idx = 0; idx < (N_HPIB_INTERFACES * N_HPIB_DEVICES); ++idx)
    {
        /* Is this the right slot, then remove entry */
        if (srq_queue[idx].dev_inst == device)
        {
            /* Set entry invalid */
            srq_queue[idx].dev_inst  = 0;
	    ifx = srq_queue[idx].interface;
	    if ( ifx >= 0  ) {
		/* doing SRQ via polling */
		/* decrease associated interface session use count */
		srq_interfaces[ifx].device_use--;
		if (srq_interfaces[ifx].device_use==0) {
		    /* close interface session */
		    if (srq_interfaces[ifx].interface_inst>0) {
		        if (Hpib_srq_debug) {
			  fprintf (stderr, "Closing interface session to %s\n",
				   srq_interfaces[ifx].interface_name); 
			  fflush(stderr);
			} 

			ret=iclose(srq_interfaces[ifx].interface_inst); /* this may generate an error */
			if (ret!=I_ERR_NOERROR) {
			    err = -1;
			}
		    }
		    srq_interfaces[ifx].interface_name[0]='\0';
		    srq_interfaces[ifx].interface_inst=0;
		}
	    }
            return err;
        }
    }

    /* No such entry found - error */
    return (-1);
}

#ifndef _WINDOWS

/*
 *----------------------------------------------------------------------
 *
 * TclpGetTime -- (Unix version from TCl 8.3.2)
 *
 *	Gets the current system time in seconds and microseconds
 *	since the beginning of the epoch: 00:00 UCT, January 1, 1970.
 *
 * Results:
 *	Returns the current time in timePtr.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
TclpGetTime(timePtr)
    Tcl_Time *timePtr;		/* Location to store time information. */
{
    struct timeval tv;
    struct timezone tz;
    
    (void) gettimeofday(&tv, &tz);
    timePtr->sec = tv.tv_sec;
    timePtr->usec = tv.tv_usec;
}

#endif /* _WINDOWS */


/*  BEGIN PROCEDURE ********************************************************
//
//  PROCEDURE NAME: Hpib_SRQServiceRoutine
//
//  PARAMETERS:     info            Pointer to device information structure
//
//  RETURN VALUE:   None
//
//  PURPOSE:        Set the Tcl async-mark to call the Tcl-SRQ procedure
//                  further.
//
//  END PROCEDURE *********************************************************/

void Hpib_SRQServiceRoutine (Hpib_Info* info)
{
	/* Warn if SRQ's are not fast enough scheduled */
       /*    if (info->srq_pending > 0) 
        fprintf (stderr, "Warning: SRQ with pending=%d\n", info->srq_pending);
        */
    if (info->srq_enabled)
    {
        /* Save parameters for further execution */
        info->srq_pending++;
	if (Hpib_srq_debug) {
	    fprintf (stderr, "SRQ (async) occured for %s\n",info->interface); 
	}
        /* Mark the Tcl SRQ procedure for scheduling */
        Tcl_AsyncMark (Hpib_Handle);
    }
}





/*  BEGIN PROCEDURE ********************************************************
//
//  PROCEDURE NAME: Hpib_SRQSICLCallback
//
//  PARAMETERS:     device      SICL instance of the device
//
//  RETURN VALUE:   None
//
//  PURPOSE:        Serve an device SRQ.
//
//      This routine is called every time the SICL library detects an
//      SRQ on the interface associated with the device (SRQ must be enabled).
//      At first, check if the device caused an SRQ (read the status byte).
//      If an SRQ was generated by the device, search for the associated
//      entry in the SRQ queue. If the entry was found, perform the
//      associated callback (in our case Hpib_SRQServiceRoutine which
//      marks the Tcl SRQ procedure for execution).
//
//  END PROCEDURE *********************************************************/

void Hpib_SRQSICLCallback (INST device)
{
    int             idx;

    /* Scan SRQ queue for empty slot */
    for (idx = 0; idx < (N_HPIB_INTERFACES * N_HPIB_DEVICES); ++idx)
    {
        /* Is this the right slot, then remove entry */
        if (srq_queue[idx].dev_inst == device)
        {
            /* Call SRQ service procedure */
            (*srq_queue[idx].service_routine) (srq_queue[idx].client_data);
            return;
        }
    }
}



/*  BEGIN PROCEDURE ********************************************************
//
//  PROCEDURE NAME: Hpib_SrqConfig
//
//  PARAMETERS:     clientData      Data associated with the Tcl command
//                  interp          Tcl interpreter
//                  argc            Argument count for the command
//                  argv            Argument list for the command
//
//  RETURN VALUE:   
//
//  PURPOSE:        Define and configure HPIB SRQ procedures in Tcl.
//
//      usage:  srq -priority 3 -command {xxx}/
//              srq configure ...
//              srq enable
//              srq disable
//              srq delete
//
//  END PROCEDURE *********************************************************/

SRQ_EXPORT int Hpib_SrqConfig (ClientData clientdata, Tcl_Interp *interp, 
                    int argc, const char* argv[])
{
    Hpib_Info*   info;           /* HPIB info structure of the device */
    int          result=0;       /* Tcl return code */
    int          ret;            /* return code of HPIB library */

    info = (Hpib_Info *) clientdata;

    /* Nor enough arguments ? */
    if (argc < 3)
    {
        Tcl_AppendResult (interp,
                          Tcl_GetCommandName(interp,info->hpibCmd),
                          " ",argv[1],": invalid number of arguments",
                          (char*) NULL);
        return TCL_ERROR; 
    }

    /* SRQ created ? */
    if (info->srq_created) {
        /* Command: <dev> srq configure -priority <num> -command <callback> */
        if (0==strcmp(argv[2],"configure"))
        {
            /* Command: <dev> configure */
            if (argc==3)  {
                result =  Tcl_ConfigureInfo(interp, srqConfig,
                            (char *) info, (char *) 0, 0);
            } 	 
            else
            /* Command: <dev> configure -priority
                        <dev> configure -command   */
            if (argc==4) {
                result =  Tcl_ConfigureValue(interp, srqConfig,
                             (char *) info, argv[3], 0);
            } 
            else
            /* Command: <dev> configure "with setting options" */
            if (info->srq_enabled) {
                Tcl_AppendResult(interp,
                     Tcl_GetCommandName(interp,info->hpibCmd),
                     " ",argv[1]," ",argv[2],
                     " not allowed while SRQ is enabled",
                     (char*) NULL);
                result=TCL_ERROR;   
            } 
            else  {
                result = Tcl_ConfigureStruct(interp, srqConfig,
                             argc-3, argv+3, (char *) info, 0);
            }
        }
        else
        /* Command: <dev> srq poll */
        if (0==strcmp(argv[2],"poll")) {
	    result=Hpib_TclPollProc (clientdata,interp,TCL_OK);

        }
        else
        /* Command: <dev> srq enable */
        if (0==strcmp(argv[2],"enable")) {
            if (! info->srq_enabled)  {
		info->srq_enabled = 1;
		result = TCL_OK;
		if ( info->srq_poll_interval <= 0 ) {
		    /* ensable async SRQ callback */
		    if (ionsrq (info->dev_inst, Hpib_SRQSICLCallback))   {
			ret = igeterrno ();

			Tcl_ResetResult(interp);
			Tcl_AppendResult(interp,
					 Tcl_GetCommandName(interp,info->hpibCmd),
					 " ",argv[1]," ",argv[2],":",
					 igeterrstr (ret),
					 (char*) NULL);
			Tcl_SetErrorCode(interp,"HPIB","SRQ", (char*) NULL);
			
			result = TCL_ERROR;
		    }
		}
            }
        }
	else
        /* Command: <dev> srq disable */
        if (0==strcmp (argv[2],"disable"))
        {
            if (info->srq_enabled)   {
                info->srq_enabled = 0;
		result = TCL_OK;

		if ( info->srq_poll_interval <= 0 ) {
		    /** must remove async SRQ handler **/
		    if (ionsrq (info->dev_inst, NULL)) 	{
			ret = igeterrno ();

			Tcl_ResetResult(interp);
			Tcl_AppendResult(interp,
					 Tcl_GetCommandName(interp,info->hpibCmd),
					 " ",argv[1]," ",argv[2],":",
					 igeterrstr (ret),
					 (char*) NULL);
			Tcl_SetErrorCode(interp,"HPIB","SRQ",
					 (char*) NULL);
			
			result = TCL_ERROR;
		    }
		}
	    }
        }
        else
        /* Command: <dev> srq delete */
        if (0==strcmp (argv[2],"delete"))
        {
            if (info->srq_enabled)
            {
                Tcl_AppendResult(interp,
                         Tcl_GetCommandName(interp,info->hpibCmd),
                         " ",argv[1]," ",argv[2],
                         " not allowed while SRQ is enabled",
                         (char*) NULL);

                result = TCL_ERROR;   
            } 
            else
            {
                ret = Hpib_RemoveSRQEntry (info->dev_inst);

               /* AL Remove check to allow a blind deletion of the SRQ
		  if ( ret )
		  {
                    Tcl_ResetResult(interp);
                    Tcl_AppendResult(interp,
                             Tcl_GetCommandName(interp,info->hpibCmd),
                             " ",argv[1]," ",argv[2],":",
                             igeterrstr (ret),
                             (char*) NULL);
                    Tcl_SetErrorCode(interp,"HPIB","SRQ",
                             interp->result, (char*) NULL);

                    result=TCL_ERROR;
                  }
                */

                ckfree (info->srq_command);

                info->srq_command  = 0;
                info->srq_created  = 0;
                info->srq_priority = 0;
            }
        }
        else
        if  (*argv[2]=='-')
        {
            Tcl_AppendResult(interp,
                     "SRQ already registered, option \"",
                     argv[2],"\" invalid ", 
                     (char*) NULL);

            result = TCL_ERROR; 
        }
        else
        {
            Tcl_AppendResult(interp,
                     "invalid hpib ",argv[1]," command \"",
                     argv[2], 
                     (char*) NULL);

            result = TCL_ERROR; 
        }
    }  else  {
        /* check if we start with an option */
        if (*argv[2] == '-')  {
            result = Tcl_ConfigureStruct(interp, srqConfig,
                         argc-2, argv+2, (char *) info, 0);

            if (result == TCL_OK)
            {
                ret = Hpib_AddSRQEntry (info->dev_inst,
                                        info->srq_priority,
                                        Hpib_SRQServiceRoutine,
                                        info);

                if ( ret )
                {
                    Tcl_AppendResult(interp,
			     "\n",
                             Tcl_GetCommandName(interp,info->hpibCmd),
                             " ",argv[1]," could not add to queue:",
                             igeterrstr (ret),
                             (char*) NULL);
                    Tcl_SetErrorCode(interp,"HPIB","SRQ",
                             (char*) NULL);

                    result = TCL_ERROR;
                }
            }

            info->srq_created = (result == TCL_OK);
        } else  {
            Tcl_AppendResult(interp,
                     "invalid hpib ",argv[1]," command \"",
                     argv[2], 
                     "\" (did you already register an SRQ?)\n",
                     (char*) NULL);

            result = TCL_ERROR; 
        }
    }

    return result;
}




/*  BEGIN PROCEDURE ********************************************************
//
//  PROCEDURE NAME: Hpib_TclPollProc
//
//  PARAMETERS:     clientData
//                  interp
//                  cmdResultCode
//
//  RETURN VALUE:   
//
//  PURPOSE:        Procedure for handling polled HPIB SRQs.
//
//      This procedure polls all interfaces for which an SRQ has been enabled
//      a SRQ is pending.
//      If yes, it marks the Tcl SRQ procedure for scheduling 
//      (  Tcl_AsyncMark (Hpib_Handle); )
//
//
//  END PROCEDURE *********************************************************/

int Hpib_TclPollProc (ClientData  clientData, 
                     Tcl_Interp *interp,
                     int         cmdResultCode)
{
    //Hpib_Info*   info;            /* HPIB info structure of the device */
    //Tcl_Interp   *srqInterp;      /* Tcl interpreter to use for evaluating
    //                                 Tcl SRQ command */
    //errState_t   *errStatePtr;    /* structure for saving interpreter state */

    int     ret;
    int     result = cmdResultCode;
    int     any_srq=0;
    int     srq_active;
    int     idx,ifx;
    //int     srqIndex;
    //int     srqPrio;
    //unsigned char   status;

    /* fprintf (stderr, "Entering Tcl SRQ poll ...\n"); */

    /* Avoid recursion */
    if (PollActive)
        return cmdResultCode;
    /* fprintf (stderr, "No recursion.\n"); */
    PollActive++;




    /* Get the interpreter if it wasn't supplied.
       If none is available, bail out. */
    if (interp == NULL)     {
	fprintf (stderr, "No interpreter in SRQ Poll Handler !\n");
	PollActive = 0;
	    return TCL_ERROR;
    }

    for (ifx = 0; ifx < N_HPIB_INTERFACES; ++ifx) {
	if (srq_interfaces[ifx].interface_inst>0) {
	    srq_active=0;
	    if (ret=igpibbusstatus(srq_interfaces[ifx].interface_inst,I_GPIB_BUS_SRQ,&srq_active)!=0) {
		if (result==TCL_OK) Tcl_ResetResult (interp);
		Tcl_AppendResult (interp,
				  " Hpib_TclPollProc : interface '",
				  srq_interfaces[ifx].interface_name,"' : ",
				  igeterrstr (ret),
				  NULL);
		result=TCL_ERROR;
		iclose(srq_interfaces[ifx].interface_inst) ; /* this may produce an error */
		srq_interfaces[ifx].interface_inst=0;
		srq_interfaces[ifx].interface_name[0]='\0';
		srq_interfaces[ifx].device_use=0;
		for (idx = 0; idx < (N_HPIB_INTERFACES * N_HPIB_DEVICES); ++idx)  {
		    if (srq_queue[idx].interface == ifx) {
			srq_queue[idx].interface = -1;
		    }
		}
	    } else {
		if (srq_active) {
		    any_srq++;
		    if (Hpib_srq_debug) {
			fprintf(stderr,"SRQ (polled) occured on interface '%s'\n", srq_interfaces[ifx].interface_name);
		    }
		}
	    }

	}
	
    }


    if (result) {
	fprintf(stderr,"ERROR: SRQ POLL: %s\n",Tcl_GetStringResult(interp));
    } else {
	Tcl_ResetResult (interp);
	Tcl_AppendResult (interp,
			  any_srq ? "1" : "0",
			  NULL);
    }
    if (any_srq) {
	/* Mark the Tcl SRQ procedure for scheduling */
	Tcl_AsyncMark (Hpib_Handle);
    }
    PollActive = 0;
    return result;
}


/*  BEGIN PROCEDURE ********************************************************
//
//  PROCEDURE NAME:  PollTimer
//
//  PARAMETERS:      clientData            Pointer to device information structure
//
//  RETURN VALUE:   None
//
//  PURPOSE:        Set the Tcl async-mark to call the Tcl-SRQ procedure
//                  further.
//
//  END PROCEDURE *********************************************************/
static void         PollTimer(ClientData clientData)
{
    Hpib_Info*   info=(Hpib_Info*) clientData ;   /* HPIB info structure of the device */
    Tcl_Interp   *interp=info->main;              /* Tcl interpreter to use */
    int result;

    Srq_pollTimerHandle = 0; /* say that we are already running*/

    if (interp == NULL)     {
	fprintf (stderr, "ERROR: No interpreter in PollTimer !\n");
	return;
    }

    /*** Polling ***/

    result=Hpib_TclPollProc (clientData,interp,TCL_OK);


    /**** register an event timer if not done by any other routine ***/
    if (Srq_pollTimerHandle == 0) {
	Srq_pollTimerHandle = Tcl_CreateTimerHandler(Hpib_srq_poll_interval,
						     PollTimer,
						     (int *) info);
    }

    if (result==TCL_ERROR) {
	/* If we got an error, call the
	   background error handler (if available).  Otherwise, lose the error. */
	if ((tclSrqBackgroundError != NULL))  (*tclSrqBackgroundError) (interp);
    }


}



/*  BEGIN PROCEDURE ********************************************************
//
//  PROCEDURE NAME: Hpib_TclSrqProc
//
//  PARAMETERS:     clientData
//                  interp
//                  cmdResultCode
//
//  RETURN VALUE:   
//
//  PURPOSE:        Procedure for handling asynchronous HPIB SRQs.
//
//      This procedure is  called at safe times by Tcl_AsyncInvoke whenever
//      a SRQ is pending.
//
//  END PROCEDURE *********************************************************/

int Hpib_TclSrqProc (ClientData  clientData, 
                     Tcl_Interp *interp,
                     int         cmdResultCode)
{
    Hpib_Info*   info;            /* HPIB info structure of the device */
    Tcl_Interp   *srqInterp;      /* Tcl interpreter to use for evaluating
                                     Tcl SRQ command */
    errState_t   *errStatePtr;    /* structure for saving interpreter state */

    int     ret;
    int     result = 0;

    int     idx;
    int     srqIndex;
    int     srqPrio;
    unsigned char   status;

#ifdef SRQ_DEBUG
    fprintf (stderr, "Entering Tcl SRQ check ...\n");
#endif

    /* Avoid recursion */
    if (SrqActive) return cmdResultCode;

    SrqActive++;

    /* Get the interpreter if it wasn't supplied.
       If none is available, bail out. */
    if (interp == NULL)     {
      srqInterp = (Tcl_Interp*) clientData;
      if (srqInterp == NULL)         {
	fprintf (stderr, "No interpreter in SRQ Async Handler !\n");
	SrqActive = 0;
	return cmdResultCode;
      }
    }
    else
    {
        srqInterp = interp;
    }

    /* save interp->result, errorInfo and errorCode */
    errStatePtr = SaveErrorState (srqInterp);
#ifdef SRQ_DEBUG
    fprintf (stderr, "Searching for SRQ routine ...\n"); 
#endif
    /*
     * Make one or more passes over the list of pending SRQs, invoking
     * at most one handler in each pass.  After invoking a handler,
     * go back to the start of the list again so that if a new
     * higher-priority SRQ starts pending while executing a lower
     * priority SRQ, we execute the higher-priority SRQ next.
     * When there are multiple SRQ at the same priority, a round-robin
     * scheme is employed.
     */

    /* Process until all SRQs served */
    for (;;) {
        /* mark as 'No entry found', start scanning */
        srqIndex = -1;
        srqPrio  = -1;

        /* Scan SRQ queue for entry with highest priority */
        /* Begin with srqNextSchedule to implement a round-robin */
        /* algorithm for entries with equal priority */
        for (idx = 0; idx < (N_HPIB_INTERFACES * N_HPIB_DEVICES); ++idx) {
            if (srq_queue[WRAP_IDX (srqNextSchedule + idx)].dev_inst!=0 &&
	        srq_queue[WRAP_IDX (srqNextSchedule + idx)].client_data->srq_enabled &&
                (srq_queue[WRAP_IDX (srqNextSchedule + idx)].client_data->srq_priority > srqPrio))
            {
                /* Is this device requesting service, if not skip to next entry */
                ireadstb (srq_queue[WRAP_IDX (srqNextSchedule + idx)].dev_inst, &status);
		if (Hpib_srq_debug) {
		  info =srq_queue[WRAP_IDX (srqNextSchedule + idx)].client_data;
		  fprintf(stderr, "HPIB srq status byte for '%s' is 0x%x\n",info->interface,status);
		}
		
                if (status & SRQ_STATUS_MASK)
                {
                    srqIndex = WRAP_IDX (srqNextSchedule + idx);
                    srqPrio  = srq_queue[srqIndex].client_data->srq_priority;
                    srq_queue[srqIndex].client_data->poll_status = status;
		    break; /* found a device requesting service at priority srqPrio */
		}
            }
        }


        /*  No more entries to serve ? */
        if (srqIndex < 0) {
#ifdef SRQ_DEBUG
	    fprintf (stderr, "No more entries to serve.\n"); 
#endif
            break;
	}

        /* The next scan for active devices begins with the entry behind */
        srqNextSchedule = WRAP_IDX (srqIndex + 1);
	info =srq_queue[srqIndex].client_data;
	if (Hpib_srq_debug) {
	    fprintf (stderr, "Executing SRQ code for %s\n",info->interface);
	}
	result = EvalSrqCode (srqInterp, srq_queue[srqIndex].client_data);

	if (result)  {
	    Tcl_SetErrorCode (srqInterp,
			      "HPIB","SRQ",
			      Tcl_GetCommandName(srqInterp,info->hpibCmd),
			      (char*) NULL);

            /* An error occured while executing the Tcl callback - disable SRQ */
	    if (info->srq_enabled) {
		info->srq_enabled = 0;
		if ( info->srq_poll_interval <= 0 ) {
		    /* disable async SRQ via signal */
		    ionsrq (info->dev_inst, NULL);
		}
		printf ("SRQ for %d disabled.\n", info->dev_inst);
		
		if (Hpib_RemoveSRQEntry (info->dev_inst))   {
		    printf ("ERROR: could not disable SRQ for %d .\n", info->dev_inst);
		    
                    ret=igeterrno();
		    Tcl_AppendResult (srqInterp,
				      "\nERROR:",
				      Tcl_GetCommandName(srqInterp,info->hpibCmd),
				      " could not disable SRQ:",
				      igeterrstr (ret),
				      (char*) NULL);
		}
	    }
	}
    }
    /* fprintf (stderr, "SRQ done.\n"); */
    /* Restore result and error state if we didn't get an error in SRQ handling. */
    if (result != TCL_ERROR)     {
      RestoreErrorState (srqInterp, errStatePtr);
    }  else  {
      ckfree ((char *) errStatePtr);
      cmdResultCode = TCL_ERROR;
    }

    /* If we got an error and an interpreter was not supplied, call the
       background error handler (if available).  Otherwise, lose the error. */
    if ((result == TCL_ERROR) && (interp == NULL) &&
        (tclSrqBackgroundError != NULL))
        (*tclSrqBackgroundError) (srqInterp);

    SrqActive = 0;
    return cmdResultCode;
}

/*  BEGIN PROCEDURE ********************************************************
//
//  PROCEDURE NAME:  
//
//  PARAMETERS:
//
//  RETURN VALUE:
//
//  PURPOSE:      
//
//  END PROCEDURE *********************************************************/

/*
 *-----------------------------------------------------------------------------
 *
 * EvalSrqCode --
 *     Run code as the result of a SRQ.  The replacements defined in
 *     FormatSrqCode are applied to the command before evaluating it.
 *
 * Parameters:
 *   o interp (I) - The interpreter to run the signal in. If an error
 *     occures, then the result will be left in the interp.
 *   o signalNum (I) - The signal number of the signal that occured.
 * Return:
 *   TCL_OK or TCL_ERROR.
 *-----------------------------------------------------------------------------
 */
static int
EvalSrqCode (Tcl_Interp *interp,  Hpib_Info* info)
{
    int          result;
    Tcl_DString  command;
    int          pending=info->srq_pending;

    /* must be reset before servicing so that no SRQs are lost */
    info->srq_pending=0;
    Tcl_ResetResult (interp);

    /*
     * Format the HPIB name into the command.  
       This also allows the SRQ to be
     * to be reset in the command.
     */
    result = FormatSrqCode (interp, info, pending, &command);

    if (result == TCL_OK)
        result = Tcl_GlobalEval (interp, command.string);

    Tcl_DStringFree (&command);

    if (result == TCL_ERROR)
    {
        Tcl_AddErrorInfo (interp, "\n    while executing SRQ code for HPIB device ");
        Tcl_AddErrorInfo (interp,Tcl_GetCommandName(interp, info->hpibCmd));

        return TCL_ERROR;
    }

    Tcl_ResetResult (interp);
    return TCL_OK;
}

/*  BEGIN PROCEDURE ********************************************************
//
//  PROCEDURE NAME:  
//
//  PARAMETERS:
//
//  RETURN VALUE:
//
//  PURPOSE:      
//
//  END PROCEDURE *********************************************************/

/*
 *-----------------------------------------------------------------------------
 *
 * FormatSrqCode --
 *     Format some dynamic information into the SRQ command. The following
 *     replacements are performed
 *     %% --> %
 *     %H --> name of HPIB device Tcl command 
 *     %E --> error status (bit 1 of poll_status)
 *     %S --> poll_status
 *     %P --> number of pending SRQs

 *
 * Parameters:
 *   o interp (I/O) - The interpreter to return errors in.
 *   o info    (I)  - The HPIB info structure of the device
 *   o pending (I)  - number of SRQs currenly pending
 *   o command (O)  - The resulting command adter the formatting.
 *-----------------------------------------------------------------------------
 */
static int
FormatSrqCode (Tcl_Interp  *interp, Hpib_Info* info, int pending,
	       Tcl_DString *command)
{
    char  *copyPtr, *scanPtr, *hpibName;
    static char chbuf[20];

    Tcl_DStringInit (command);
    //hpibName=Tcl_GetCommandName(interp, info->hpibCmd);

    copyPtr = scanPtr = info->srq_command;

    while (*scanPtr != '\0') {
        if (*scanPtr != '%') {
            scanPtr++;
            continue;
        }
        if (scanPtr [1] == '%') {
            scanPtr += 2;
            continue;
        }
        Tcl_DStringAppend (command, copyPtr, (scanPtr - copyPtr));

        switch (scanPtr [1]) {
          case 'H': {
              Tcl_DStringAppend (command, Tcl_GetCommandName(interp, info->hpibCmd), -1);
              break;
          }
          case 'S': {
	      /* sprintf(chbuf,"0x%X",info->poll_status); */
	      sprintf(chbuf,"%d",info->poll_status);
              Tcl_DStringAppend (command, chbuf, -1);
              break;
          }
          case 'P': {
	      sprintf(chbuf,"%d",pending);
              Tcl_DStringAppend (command, chbuf, -1);
              break;
          }
          case 'E': {
	      chbuf[0]=( info->poll_status & 0x01 ) ? '1' : '0' ;
	      chbuf[1]='\0';  
              Tcl_DStringAppend (command, chbuf, -1);
              break;
          }
          default:
            goto badSpec;
        }
        scanPtr += 2;
        copyPtr = scanPtr;
    }
    Tcl_DStringAppend (command, copyPtr, copyPtr - scanPtr);

    return TCL_OK;

    /*
     * Handle bad % specification currently pointed to by scanPtr.
     */
  badSpec:
    {
        char badSpec [2];
        
        badSpec [0] = scanPtr [1];
        badSpec [1] = '\0';
        Tcl_AppendResult(
	    interp, "bad SRQ trap command formatting ",
	    "specification \"%", badSpec,
	    "\", expected one of \"%%\" or \"%H\" or \"%S\" or \"%P\" or \"%E\"",
	    (char *) NULL);
        return TCL_ERROR;
    }
}

/*  BEGIN PROCEDURE ********************************************************
//
//  PROCEDURE NAME:  
//
//  PARAMETERS:
//
//  RETURN VALUE:
//
//  PURPOSE:      
//
//  END PROCEDURE *********************************************************/

/*
 *-----------------------------------------------------------------------------
 *
 * SaveErrorState --
 *  
 *   Save the error state of the interpreter (result, errorInfo and errorCode).
 *
 * Parameters:
 *   o interp (I) - The interpreter to save. Result will be reset.
 * Returns:
 *   A dynamically allocated structure containing all three strings,  Its
 * allocated as a single malloc block.
 *-----------------------------------------------------------------------------
 */
static errState_t *
SaveErrorState (interp)
    Tcl_Interp *interp;
{
    errState_t *errStatePtr;
    char       *nextPtr;
    int         len;

    const char* errorInfo = Tcl_GetVar (interp, "errorInfo", TCL_GLOBAL_ONLY);
    const char* errorCode = Tcl_GetVar (interp, "errorCode", TCL_GLOBAL_ONLY);

    len = sizeof (errState_t) + strlen (Tcl_GetStringResult(interp)) + 1;
    if (errorInfo != NULL)
        len += strlen (errorInfo) + 1;
    if (errorCode != NULL)
        len += strlen (errorCode) + 1;


    errStatePtr = (errState_t *) ckalloc (len);
    nextPtr = ((char *) errStatePtr) + sizeof (errState_t);

    errStatePtr->result = nextPtr;
    strcpy (errStatePtr->result, Tcl_GetStringResult(interp));
    nextPtr += strlen (Tcl_GetStringResult(interp)) + 1;

    errStatePtr->errorInfo = NULL;
    if (errorInfo != NULL) {
        errStatePtr->errorInfo = nextPtr;
        strcpy (errStatePtr->errorInfo, errorInfo);
        nextPtr += strlen (errorInfo) + 1;
    }

    errStatePtr->errorCode = NULL;
    if (errorCode != NULL) {
        errStatePtr->errorCode = nextPtr;
        strcpy (errStatePtr->errorCode, errorCode);
        nextPtr += strlen (errorCode) + 1;
    }

    Tcl_ResetResult (interp);
    return errStatePtr;
}

/*  BEGIN PROCEDURE ********************************************************
//
//  PROCEDURE NAME:  
//
//  PARAMETERS:
//
//  RETURN VALUE:
//
//  PURPOSE:      
//
//  END PROCEDURE *********************************************************/

/*
 *-----------------------------------------------------------------------------
 *
 * RestoreErrorState --
 *  
 *   Restore the error state of the interpreter that was saved by
 * SaveErrorState.
 *
 * Parameters:
 *   o interp (I) - The interpreter to save.
 *   o errStatePtr (I) - Error state from SaveErrorState.  This structure will
 *     be freed. 
 * Returns:
 *   A dynamically allocated structure containing all three strings,  Its
 * allocated as a single malloc block.
 *-----------------------------------------------------------------------------
 */
static void
RestoreErrorState (interp, errStatePtr)
    Tcl_Interp *interp;
    errState_t *errStatePtr;
{
    Tcl_SetResult (interp, errStatePtr->result, TCL_VOLATILE);
    if (errStatePtr->errorInfo != NULL)
        Tcl_SetVar (interp, "errorInfo", errStatePtr->errorInfo,
                    TCL_GLOBAL_ONLY);
    if (errStatePtr->errorCode != NULL)
        Tcl_SetVar (interp, "errorCode", errStatePtr->errorCode,
                    TCL_GLOBAL_ONLY);

    ckfree ((char *) errStatePtr);
}
