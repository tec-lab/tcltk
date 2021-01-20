#define MODULE 8001
/*%%******************* M O D U L E   H E A D E R ****************************
%% BeginOfModule
   tclHPIB.c
%% Identification
   SOU/MSG/RF_SCOE/010102-T-0002/SOE ????
%% Type
   C source file
%% Project
   MSG-RF-SCOE, METOP-RF-SCOE
%% Subsystem
   SCOE-APPLICATION-SW
%% File
   $Source: /cmdata/cvs/Tcl-Packages/tclHPIB/tclHPIB.c,v $
%% VersionAndStatus
   $Revision: 1.24 $ $State: Exp $
%% CompilerOS
   ANSI-C 8.0, HP-UX 8.0
%% AuthorAndDate
   O. Maenner, IVP1,  1995/12/20, 1999/1021
%% Copyright
   (c) by SIEMENS Austria, all rights reserved, confidental.
%% HistoryOfChange
   $Log: tclHPIB.c,v $
   Revision 1.24  2010/08/04 09:03:27  maenner.o
   adapted Makefile for tcl8.5

   Revision 1.23  2007/10/04 12:17:04  maenner.o
   removed copy&paste 'fclose(ifp)' which caused application crash

   Revision 1.22  2007/10/04 11:34:19  maenner.o
   added trace <filenname> device command for VXI-11 tracing

   Revision 1.21  2007/03/20 18:09:39  maenner.o
    corrected syntax error

   Revision 1.20  2007/03/20 12:50:34  maenner.o
   implemented upload_bin with direct data return  return via ByteArrayObj

   Revision 1.19  2003/10/13 15:22:32  cryo
   new method for gerhard uploadbin

   Revision 1.18  2003/07/09 20:50:17  cryo
   error in download

   Revision 1.17  2003/06/27 14:20:13  cryo
   tclHPIB cvs update -d! it works

   Revision 1.16  2003/06/27 11:27:52  cryo
   working on windows

   Revision 1.15  2003/06/27 11:03:59  cryo
   in work

   Revision 1.14  2003/06/26 14:21:53  cryo
   working for windows and linux

   Revision 1.13  2003/06/26 13:48:19  cryo
   working for windows and linux

   Revision 1.12  2003/06/26 12:38:41  cryo
   tclHPIB for windows and unix

   Revision 1.1  2003/06/26 11:53:03  cryo
   tclHPIB

   Revision 1.11  2002/10/04 14:20:49  cryo
   extended delete info

   Revision 1.10  2002/10/04 11:41:25  cryo
   OM increased hpib buffer size to1Meg, upload: continue reading until EOI

   Revision 1.9  2002/04/03 15:35:05  cryo
   more buffer for asa hardcopy

   Revision 1.6  2001/08/17 12:22:12  gras
   tclHPIB v 2.0 / SRQ polling

   Revision 1.5  2000/07/27 13:06:55  gras
   OM new command write_noeoi for downloading MTA SW

   Revision 1.4  2000/03/28 14:30:41  tcl
   OM/HK Compile with Borland Builder 4

   Revision 1.3  1999/12/13 16:58:57  gras
   makefile.vc works for WIN

   Revision 1.2  1999/12/13 13:27:44  gras
   {OM} V1.3 , compiles OK under HP-UX, timeout calc. with cast to long

   Revision 1.1.1.1  1999/11/22 10:34:16  tcl
   Import of dir. structure

   Revision 1.4  1999/10/19 14:55:13  mo-cm
   {OM} added stubs interface for remaining packages

   Revision 1.3  1999/03/24 15:57:45  mo-cm
   HK timeout, lan_timeout corrected

   Revision 1.2  1999/03/10 15:57:39  mo-cm
   HK Before Integration

   Revision 1.1.1.1  1998/10/14 11:05:06  mo-cm
   Imported sources

   Revision 1.24  1998/04/16 18:57:13  msg-cm
   {OM} quick selftest on start-up

   Revision 1.23  1998/04/03 14:55:54  msg-cm
   {OM} added hpib_write_read

   Revision 1.22  1998/03/24 15:11:20  msg-cm
   {OM} addded HPIB timeout command / tclDAQ.c may use tcl function for calibration

   Revision 1.21  1998/03/07 08:25:18  msg-cm
   {OM} TG-SAL_2.1-6 OK

   Revision 1.20  1998/02/03 20:22:36  msg-cm
   {OM} improved basic debugging, SSS isolation now active high, fixed latchVal bug

   Revision 1.19  1998/02/03 14:23:14  msg-cm
   {OM} added command readstb

   Revision 1.18  1997/11/12 12:30:22  msg-cm
   [OM] added command trimlist

   Revision 1.17  1997/11/11 13:26:11  msg-cm
   [OM] HPIB/LAN connection restored on error

   Revision 1.16  1997/10/17 18:49:10  msg-cm
   [OM] Tcl SRQ only executed when Bit 6 in STB

   Revision 1.15  1997/09/18 07:08:51  msg-cm
   [OM] added HPIB trigger / bug fix in fft_wish

   Revision 1.14  1997/08/28 16:24:34  msg-cm
   [OM] power monitoring works
        analog MUX numbers are now in line with HW design
        removed some sample period variables

   Revision 1.13  1997/08/19 06:13:24  msg-cm
   using sigprocmask with SIG_BLOCK/SIG_UNBLOCK

   Revision 1.12  1997/07/22 17:09:24  msg-cm
   [OM] : blocking of SIGALRM during iread/iwrite

   Revision 1.11  1997/07/10 06:18:36  msg-cm
   removing error message for srq delete; version based on OM files

   Revision 1.10  1997/04/21 08:12:07  msg-cm
   First version of HPIB extension with SICL

   Revision 1.9  1996/06/20 16:52:37  maenner
   inreased write time-out

 * Revision 1.8  1996/01/17  13:11:48  leoger
 * New interface definition for hpib-tcl
 *
 * Revision 1.7  1996/01/11  19:18:52  maenner
 * * moved srq command code to tclSRQ.c
 * * dmm_srq.tcl: measurement is restarted properly if timeout (occurs also
 *   with Robert's test routine, possible fault in 7150 multimeter -- needs
 *   a device clear)
 *
 * Revision 1.6  1996/01/11  09:54:28  maenner
 * added module headers and comments
 *
%% Purpose
   Tcl routines for HPIB devices 
%% Resources
*/

#define HPIB_EXPORT /* allocate HPIB data/procedures here */

#include <malloc.h>
#include <signal.h>
#include <stdlib.h>

#include "tclHPIB.h"
#include "tclConfig.h"


/******************************************************************************/

/*
%% GlobalData --- see tclHPIB.h
*/

int Hpib_srq_debug=0;  /* SRQ debbuging flag*/
int Hpib_srq_poll_interval=200;  /* polling interval in microseconds */

/******************************************************************************/

/*
%% Data 
*/
static Tcl_ConfigSpec       hpibDeviceSpec[] = {

{TCL_CONFIG_STRING, "-interface", "interface", "Interface", "",   Tcl_Offset (Hpib_Info, interface), 0},
{TCL_CONFIG_INT,    "-termchar",  "termchar",  "Termchar",   0,   Tcl_Offset (Hpib_Info, termchar),  0},
{TCL_CONFIG_INT,    "-timeout",   "timeout",   "Timeout",   0,    Tcl_Offset (Hpib_Info, timeout),   0},
{TCL_CONFIG_INT,    "-lan_timeout",   "lan_timeout",   "Lan_Timeout",   0,    Tcl_Offset (Hpib_Info, lan_timeout),   0},
{TCL_CONFIG_INT,    "-srq_poll_interval",   "srq_poll_interval",  
                     "Srq_Poll_Interval",   0,  Tcl_Offset (Hpib_Info, srq_poll_interval),   0},
{TCL_CONFIG_END,    NULL,         NULL,        NULL,        NULL, 0,                                 0}

};


static char     hpibResultBuffer[HPIB_RESULT_BUFFER_SIZE];

/******************************************************************************/
extern void Hpib_InitSRQQueue (void);
extern int Hpib_RemoveSRQEntry (INST device);
extern int Tcl_ConfigureInfo(Tcl_Interp *interp,
			     Tcl_ConfigSpec *specs, 
			     char *strucPtr, 
			     char *argvName, int flags);
int Hpib_TclSrqProc (ClientData  clientData, 
                     Tcl_Interp *interp,
                     int         cmdResultCode);
SRQ_EXPORT int Hpib_SrqConfig (ClientData clientdata, Tcl_Interp *interp, 
                    int argc, const char* argv[]);

/*
%% InternalFunctions 
*/
static int      Hpib_CreateCmd (ClientData cdata, Tcl_Interp *interp,
                                int argc, const char *argv[]);

static int      Hpib_Device (ClientData cdata, Tcl_Interp *interp,
                             int argc, const char *argv[]);

static void     Hpib_DeleteDevice (ClientData clientdata);

/*static int      Hpib_trimlist (ClientData cdata, Tcl_Interp *interp,
  int argc, const char *argv[]);*/

static int      Tcl_Hpib_InitSRQQueue (ClientData cdata, Tcl_Interp *interp,
                                int argc, const char *argv[]);

void         block_itimer(void)
{
#ifndef _WINDOWS
  /* sighold(SIGALRM); */
  sigset_t sigBlockSet;

  sigemptyset (&sigBlockSet);
  sigaddset (&sigBlockSet, SIGALRM);
  sigprocmask (SIG_BLOCK, &sigBlockSet, NULL);
#endif
}

void         unblock_itimer(void)
{
#ifndef _WINDOWS
	/* sigrelse(SIGALRM); */
  sigset_t sigBlockSet;

  sigemptyset (&sigBlockSet);
  sigaddset (&sigBlockSet, SIGALRM);
  sigprocmask (SIG_UNBLOCK, &sigBlockSet, NULL);
#endif
}

int Hpib_retry_iopen(Tcl_Interp *interp, Hpib_Info *info) 
{
  int ret ;
  ret = igeterrno ();
  if (ret==I_ERR_NOCONN) {
    /* hpib gateway may have timed out, try to open anew */
    if (Hpib_srq_debug || 1) {
      fprintf(stderr,"HPIB device %s (%s) error: %s\n",
	      Tcl_GetCommandName (interp, info->hpibCmd),
	      info->interface,
	      igeterrstr (ret));
    }
    iclose(info->dev_inst); /* this may generate an error */
    if ((info->dev_inst = iopen (info->interface)) != 0)  {
      /* all went OK */
      return 0;
    } else {
      ret = igeterrno ();
      Tcl_AppendResult (interp,
			"\n and could not open anew: ",
			igeterrstr (ret),
			NULL);
    }
  } 
  return 1;
}


#ifdef DEBUG
/******************************************************************************/

#define BUFSIZE             16

void dump_line (long addr, char *buffer, int valid, FILE *ofp)
{
    short       idx;

    fprintf (ofp, "%06lx: ", addr);

    for (idx = 0; idx < valid; ++idx)
        fprintf (ofp, "%02x ", (*(buffer + idx) & 0xFF));

    for (; idx < BUFSIZE; ++idx)
        fprintf (ofp, "   ");

    fprintf (ofp, "  ");

    for (idx = 0; idx < valid; ++idx)
    {
        if ((*(buffer + idx) >= (char) 0x20) &&
            (*(buffer + idx) <= (char) 0x7E)   )
            fprintf (ofp, "%c", *(buffer + idx));
        else
            fprintf (ofp, ".");
    }

    fprintf (ofp, "\n");
}

/******************************************************************************/
#endif

/*
 * ------------------------------------------------------------------------
 *  Hpib_Init()
 *
 *  Should be invoked whenever a new interpreter is created to add
 *  HPIB facilities.   Adds the "hpib" command to the given interpreter.
 * ------------------------------------------------------------------------
 */
Tcl_AsyncHandler  Hpib_Handle;
extern int
Hpib_Init (Tcl_Interp *interp)
{
    Tcl_CmdInfo     cmdInfo;

    /* 
     * provide package information
     */
    if (Tcl_PkgProvide(interp,"Hpib","2.9") != TCL_OK) {
        return (TCL_ERROR);
    }

    /*
     *  Install [hpib] facilities if not already installed.
     */
    if (Tcl_GetCommandInfo (interp, "hpib", &cmdInfo))
    {
        Tcl_SetResult(interp, "already installed: hpib", TCL_STATIC);
        return (TCL_ERROR);
    }

    /* 
     * Initialize SRQ queue and
     * register an asynchronous handler for HPIB SRQs
     */
    if (Tcl_LinkVar (interp,
		     "hpib_srq_debug",
		     (char *) &Hpib_srq_debug,
		     TCL_LINK_BOOLEAN))
    {
        Tcl_SetResult(interp, "could not link hpib_srq_debug to C",
		      TCL_STATIC);
        return (TCL_ERROR);
    }

    Hpib_InitSRQQueue ();
    Hpib_Handle = Tcl_AsyncCreate (Hpib_TclSrqProc, (ClientData) interp);

    /*
     *  Install "hpib" command.
     */
    Tcl_CreateCommand (interp, "hpib", Hpib_CreateCmd,
                       (ClientData)NULL, (Tcl_CmdDeleteProc*) NULL);

    /*    Tcl_CreateCommand (interp, "trimlist", Hpib_trimlist,
          (ClientData)NULL, (Tcl_CmdDeleteProc*) NULL);*/

    Tcl_CreateCommand (interp, "hpib_initSrqQueue", Tcl_Hpib_InitSRQQueue,
                       (ClientData)NULL, (Tcl_CmdDeleteProc*) NULL);

    return (TCL_OK);
}

/******************************************************************************/

static int  Tcl_Hpib_InitSRQQueue (ClientData cdata, Tcl_Interp *interp,
			     int argc, const char *argv[])
{
    Hpib_InitSRQQueue();
    return TCL_OK;
}

/*static int  Hpib_trimlist (ClientData cdata, Tcl_Interp *interp,
  int argc, char *argv[])*/
/*
 usage:      split_list <string>

             This routine works like the split command, but treats adjacent
             spaces/tabs/Carriage_returns/linefeeds as as single delimiter
*/
/*{
  int L_argc;
  char **L_argv;
  char *string;

  if (argc!=2) {
    Tcl_AppendResult (interp, argv[0],": wrong number of arguments",
		      NULL);
    return (TCL_ERROR); 
  }
  if (Tcl_SplitList(interp, argv[1], &L_argc, &L_argv) ) {
    Tcl_AppendResult (interp, " while in ",argv[0],
		      NULL);
    return (TCL_ERROR); 
  } 
  if ((string=Tcl_Merge(L_argc,L_argv))==NULL) {
    Tcl_AppendResult (interp, " Tcl_Merge failed while in ",argv[0],
		      NULL);
    Tcl_Free((char *) L_argv);
    return (TCL_ERROR); 
  } 
  Tcl_SetResult(interp,string,TCL_DYNAMIC); // string was allcated by Tcl
  Tcl_Free((char *) L_argv);
  return TCL_OK;
}*/





/******************************************************************************/

static void Hpib_DeleteDevice (ClientData clientdata) 
{
    Hpib_Info   *info = (Hpib_Info*) clientdata;

    printf ("deleting hpib device %s\n",info->interface);

    if (info->srq_created)
    {
        Hpib_RemoveSRQEntry (info->dev_inst);
        ckfree ((char *) info->srq_command);
    }

    ckfree ((char *) clientdata);
}

/******************************************************************************/

static int Hpib_CreateCmd (ClientData clientData, Tcl_Interp *interp, 
                           int argc, const char* argv[]) 
/*
 usage:      hpib device    <devName> -interface <ifName> -termchar <termchar>

             Creates an hpib-device with name <devName> and connects it
             with the device <ifName>. <ifName> is in the form of
             "interface,device-number", e.g. {rfequip,3}.
			 termchar is used for iread recognizing the end of string (see itermchr)
*/
{
    Hpib_Info     *info;
    Tcl_CmdInfo    infoPtr;

    /* Check the minimum parameter count. */
    if (argc < 2)
    {
        Tcl_AppendResult (interp, "wrong # of arguments", NULL);
        return (TCL_ERROR); 
    }

    /* Check if an interface or device has to be created. */
    if (strcmp (argv[1], "device") == 0)
    {
        /* Check the minimum parameter count. */
        if (argc < 5)
        {
            Tcl_AppendResult (interp, "wrong # of arguments",
                             NULL);
            return (TCL_ERROR); 
        }

        /* Check if device <devname> already exists. */
        if (Tcl_GetCommandInfo (interp, argv[2], &infoPtr))
        {
            Tcl_AppendResult (interp, "hpib device \"",
                              argv[2],"\" already defined",
                              NULL);
            return (TCL_ERROR); 
        }

        /* Allocate private data structure for further usage. */
        info = (Hpib_Info*) ckalloc (sizeof (Hpib_Info));
        info->main      = interp;
        info->interface = NULL;
        
        /* Open sicl instance not yet */
        info->dev_inst  = 0;
        info->timeout   = 5000;
	info->termchar  = -1;
        info->lan_timeout   = 6000;
        info->srq_poll_interval   = 200; /* milliseconds */

        /* some initialisations for SRQ */
        info->srq_created  = 0;
        info->srq_priority = 0;
        info->srq_enabled  = 0;
        info->srq_pending  = 0;
        info->srq_command  = 0;
        
        /* Read & check parameters. */
        if (Tcl_ConfigureStruct (interp, hpibDeviceSpec, argc-3, argv+3, (char *) info, 0) != TCL_OK)
            return (TCL_ERROR);

        if ( info->srq_poll_interval > 0 ) {
	    /* if we are polling set the poll intervall to the min value specified */
	    Hpib_srq_poll_interval= min(Hpib_srq_poll_interval,info->srq_poll_interval);
	}

        /* After sooo long time, we're able to create the device. */
        info->hpibCmd=
        Tcl_CreateCommand (interp, argv[2], Hpib_Device,
                           (ClientData) info ,
                           (Tcl_CmdDeleteProc *) Hpib_DeleteDevice);
     
        return (TCL_OK);
    }
    /* No interface or device has to be created - ERROR. */
    else
    {
        Tcl_AppendResult (interp, "can't create an \"", argv[1], "\"",
                         NULL);
        return (TCL_ERROR); 
    }
}

/******************************************************************************/

static int Hpib_Device (ClientData clientdata, Tcl_Interp *interp, 
                   int argc, const char* argv[]) 
/* Methods:                      HPIB-Command
   open                        / open SICL instance;  returns (int) INST
   close                       / close SICL instance
   local_lockout               / prevent local mode
   goto_local                  / enter local mode 
   clear                       / selective_device_clear
   trigger                     / device trigger or Group Execute Trigger (GET)
   timeout <ms>                / device timeout (-1 means timeout from open)
   write <string>              / write_to_hpib
   write_noretry <string>      / write_to_hpib  (no LAN retry)
   write_noeoi   <string>      / write_to_hpib  without EOI
   read                        / read_from_hpib
   readstb                     / read status byte
   readstb_noretry             / read status byte (no LAN retry)
   write_read <string>         / write_to_hpib followed by read
   write_read_noretry <string> / write_to_hpib followed by read (no LAN retry)
   sicl_handle                 / get SICL handle
   upload <file>			   / load binary data to local file on disk
   uploadbin <file>			   / load binary data to local file on disk
   download <prefix> <file>      / upload binary data to device
                               / File contents in decimal ASCII values
                               / First entry is count of following values
   srq -priority 3 -command {} / add_to_srq_queue
   srq configure
   srq enable  
   srq disable 
   srq delete                  / delete_from_srq_queue
   NIY / print_queue

 */
{
    Hpib_Info      *info;           /* HPIB info structure of the device */
    int             result=0;       /* Tcl return code */
    int             ret;            /* return code of SICL library */
    int             reason;
    int             lan_retry=0;
    int             write_eoi=1;
    long            timeout;
    unsigned long   wrlength;
    unsigned long   rdlength;

    unsigned long     idx;
    int     value;
    int     valcount;
    char   *writebuf;
    char   *bufp;
    char    asciibuf[128];
    FILE   *ifp;
    FILE   *tracefd=NULL;

    info = (Hpib_Info *) clientdata;

    if (strcmp (argv[1], "open") == 0)
    {
        if (info->dev_inst != 0)
        {
            Tcl_ResetResult (interp);
            Tcl_AppendResult (interp,
                              Tcl_GetCommandName (interp, info->hpibCmd),
                              " ", argv[1], ":",
                              "is already open",
                              NULL);
            Tcl_SetErrorCode (interp, "HPIB", "OPEN",
                              Tcl_GetStringResult(interp), NULL);
            return (TCL_ERROR); 
        }
        if ((info->dev_inst = iopen (info->interface)) == 0)
        {
            ret = igeterrno ();
	    
            Tcl_ResetResult (interp);
            Tcl_AppendResult (interp,
                              Tcl_GetCommandName (interp, info->hpibCmd),
                              " ", argv[1], ":",
                              igeterrstr (ret),
                              NULL);
	    if (Hpib_retry_iopen(interp,info)) {	    
              /* interface could not be opened again */
	      
	      Tcl_SetErrorCode (interp, "HPIB", "OPEN",
                              Tcl_GetStringResult(interp), NULL);
	      return (TCL_ERROR); 
	    }
        }

        if (itimeout (info->dev_inst, info->timeout) != 0)
        {
            ret = igeterrno ();

            Tcl_ResetResult (interp);
            Tcl_AppendResult (interp,
                              Tcl_GetCommandName (interp, info->hpibCmd),
                              " ", argv[1], ":",
                              igeterrstr (ret),
                              NULL);
            Tcl_SetErrorCode (interp, "HPIB", "TIMEOUT",
                              Tcl_GetStringResult(interp), NULL);
            return (TCL_ERROR); 
        }

	if (itermchr (info->dev_inst, info->termchar) != 0)
	{
	    ret = igeterrno ();
	    
            Tcl_ResetResult (interp);
            Tcl_AppendResult (interp,
                              Tcl_GetCommandName (interp, info->hpibCmd),
                              " ", argv[1], ":",
                              igeterrstr (ret),
                              NULL);
            Tcl_SetErrorCode (interp, "HPIB", "TERMCHAR",
                              Tcl_GetStringResult(interp), NULL);
            return (TCL_ERROR); 
        }

	Tcl_ResetResult (interp);
        Tcl_SetObjResult(interp, Tcl_NewIntObj(info->dev_inst));
	//sprintf (Tcl_GetStringResult(interp), "%d", info->dev_inst);
    }
	else if (strcmp (argv[1], "termchar") == 0)
    {
	int termchar = -1;
        if (info->dev_inst == 0)
        {
            Tcl_ResetResult (interp);
            Tcl_AppendResult (interp,
                              Tcl_GetCommandName (interp, info->hpibCmd),
                              " ", argv[1], ":",
                              "is closed",
                              NULL);
            Tcl_SetErrorCode (interp, "HPIB", "CLEAR",
                              Tcl_GetStringResult(interp), NULL);
            return (TCL_ERROR); 
        }
	if (Tcl_GetInt(interp,argv[2],&termchar)) {
	    Tcl_AppendResult (interp,
			      argv[0]," ",argv[1],
			      ": non-integer termchar '",argv[2],
			      "' specified",
			      NULL);
	    return (TCL_ERROR); 
	}
	if (itermchr (info->dev_inst, termchar) != 0)
	{
	    ret = igeterrno ();
	    
            Tcl_ResetResult (interp);
            Tcl_AppendResult (interp,
                              Tcl_GetCommandName (interp, info->hpibCmd),
                              " ", argv[1], ":",
                              igeterrstr (ret),
                              NULL);
            Tcl_SetErrorCode (interp, "HPIB", "TERMCHAR",
                              Tcl_GetStringResult(interp), NULL);
            return (TCL_ERROR); 
        }
	info->termchar = termchar;
	Tcl_ResetResult (interp);
        Tcl_SetObjResult(interp, Tcl_NewIntObj(info->dev_inst));
	//sprintf (Tcl_GetStringResult(interp), "%d", info->dev_inst);
    }
    else if (strcmp (argv[1], "sicl_handle") == 0)
    {
        if (info->dev_inst == 0)
        {
            Tcl_ResetResult (interp);
            Tcl_AppendResult (interp,
                              Tcl_GetCommandName (interp, info->hpibCmd),
                              " ", argv[1], ":",
                              "not opened yet",
                              NULL);
            Tcl_SetErrorCode (interp, "HPIB", "SICL_HANDLE",
                              Tcl_GetStringResult(interp), NULL);
            return (TCL_ERROR); 
        }
	Tcl_ResetResult (interp);
        Tcl_SetObjResult(interp, Tcl_NewIntObj(info->dev_inst));
	//sprintf (Tcl_GetStringResult(interp), "%d", info->dev_inst);
    }
    else if (strcmp (argv[1], "close") == 0)
    {
        if (info->dev_inst == 0)
        {
            Tcl_ResetResult (interp);
            Tcl_AppendResult (interp,
                              Tcl_GetCommandName (interp, info->hpibCmd),
                              " ", argv[1], ":",
                              "is already closed",
                              NULL);
            Tcl_SetErrorCode (interp, "HPIB", "CLOSE",
                              Tcl_GetStringResult(interp), NULL);
            return (TCL_ERROR); 
        }

        Hpib_RemoveSRQEntry (info->dev_inst);

        if (ret = iclose (info->dev_inst))
        {
            info->dev_inst  = 0;
            ret = igeterrno ();

            Tcl_ResetResult (interp);
            Tcl_AppendResult (interp,
                              Tcl_GetCommandName (interp, info->hpibCmd),
                              " ", argv[1], ":",
                              igeterrstr (ret),
                              NULL);
            Tcl_SetErrorCode (interp, "HPIB", "CLOSE",
                              Tcl_GetStringResult(interp), NULL);
            return (TCL_ERROR); 
        }

        info->dev_inst  = 0;
    }
    else if (strcmp (argv[1], "local_lockout") == 0)
    {
        if (info->dev_inst == 0)
        {
            Tcl_ResetResult (interp);
            Tcl_AppendResult (interp,
                              Tcl_GetCommandName (interp, info->hpibCmd),
                              " ", argv[1], ":",
                              "is closed",
                              NULL);
            Tcl_SetErrorCode (interp, "HPIB", "LOCKOUT",
                              Tcl_GetStringResult(interp), NULL);
            return (TCL_ERROR); 
        }

        if (iremote (info->dev_inst))
        {
            ret = igeterrno ();

            Tcl_ResetResult (interp);
            Tcl_AppendResult (interp,
                              Tcl_GetCommandName (interp, info->hpibCmd),
                              " ", argv[1], ":",
                              igeterrstr (ret),
                              NULL);
            Tcl_SetErrorCode (interp, "HPIB", "REMOTE",
                              Tcl_GetStringResult(interp), NULL);
            return (TCL_ERROR); 
        }
    }
    else if (strcmp (argv[1], "goto_local") == 0)
    {
        if (info->dev_inst == 0)
        {
            Tcl_ResetResult (interp);
            Tcl_AppendResult (interp,
                              Tcl_GetCommandName (interp, info->hpibCmd),
                              " ", argv[1], ":",
                              "is closed",
                              NULL);
            Tcl_SetErrorCode (interp, "HPIB", "LOCAL",
                              Tcl_GetStringResult(interp), NULL);
            return (TCL_ERROR); 
        }

        if (ilocal (info->dev_inst))
        {
            ret = igeterrno ();

            Tcl_ResetResult (interp);
            Tcl_AppendResult (interp,
                              Tcl_GetCommandName (interp, info->hpibCmd),
                              " ", argv[1], ":",
                              igeterrstr (ret),
                              NULL);
            Tcl_SetErrorCode (interp, "HPIB", "LOCAL",
                              Tcl_GetStringResult(interp), NULL);
            return (TCL_ERROR); 
        }
    }
    else if (strcmp (argv[1], "clear") == 0)
    {
        if (info->dev_inst == 0)
        {
            Tcl_ResetResult (interp);
            Tcl_AppendResult (interp,
                              Tcl_GetCommandName (interp, info->hpibCmd),
                              " ", argv[1], ":",
                              "is closed",
                              NULL);
            Tcl_SetErrorCode (interp, "HPIB", "CLEAR",
                              Tcl_GetStringResult(interp), NULL);
            return (TCL_ERROR); 
        }

        if ((ret=iclear (info->dev_inst)))
        {
            ret = igeterrno ();

            Tcl_ResetResult (interp);
            Tcl_AppendResult (interp,
                              Tcl_GetCommandName (interp, info->hpibCmd),
                              " ", argv[1], ":",
                              igeterrstr (ret),
                              NULL);
	    if (!Hpib_retry_iopen(interp,info)) {
	      /* interface could be opened again, retry the read */
	      ret = iclear (info->dev_inst);
	      if (ret)  {
		ret = igeterrno ();
		Tcl_AppendResult (interp,
				  "\n and could not clear after new open: ",
				  igeterrstr (ret),
				  NULL);
	      }
	    }
	}
	if (ret) {
            Tcl_SetErrorCode (interp, "HPIB", "CLEAR",
                              Tcl_GetStringResult(interp), NULL);
            return (TCL_ERROR); 
        }
    }
    else if (strcmp (argv[1], "trigger") == 0)
    {
        if (info->dev_inst == 0)
        {
            Tcl_ResetResult (interp);
            Tcl_AppendResult (interp,
                              Tcl_GetCommandName (interp, info->hpibCmd),
                              " ", argv[1], ":",
                              "is closed",
                              NULL);
            Tcl_SetErrorCode (interp, "HPIB", "CLEAR",
                              Tcl_GetStringResult(interp), NULL);
            return (TCL_ERROR); 
        }

        if ((ret=itrigger (info->dev_inst)))
        {
            ret = igeterrno ();

            Tcl_ResetResult (interp);
            Tcl_AppendResult (interp,
                              Tcl_GetCommandName (interp, info->hpibCmd),
                              " ", argv[1], ":",
                              igeterrstr (ret),
                              NULL);
	    if (!Hpib_retry_iopen(interp,info)) {
	      /* interface could be opened again, retry the command */
	      ret = itrigger (info->dev_inst);
	      if (ret)  {
		ret = igeterrno ();
		Tcl_AppendResult (interp,
				  "\n and could not trigger after new open: ",
				  igeterrstr (ret),
				  NULL);
	      }
	    }
	}
	if (ret) {
            Tcl_SetErrorCode (interp, "HPIB", "CLEAR",
                              Tcl_GetStringResult(interp), NULL);
            return (TCL_ERROR); 
        }
    }
    else if (strcmp (argv[1], "trace") == 0)
    {
	/// int SICLAPI itrace(INST id, FILE *fd); /* Turn on tracing */
        if (info->dev_inst == 0)
        {
            Tcl_ResetResult (interp);
            Tcl_AppendResult (interp,
                              Tcl_GetCommandName (interp, info->hpibCmd),
                              " ", argv[1], ":",
                              "is closed",
                              NULL);
            Tcl_SetErrorCode (interp, "HPIB", "CLEAR",
                              Tcl_GetStringResult(interp), NULL);
            return (TCL_ERROR); 
        }
	if (strlen(argv[2])>0) {
	    if ((tracefd = fopen(argv[2], "w+b")) == NULL) {
		Tcl_AppendResult(interp,
			     Tcl_GetCommandName(interp,info->hpibCmd),
			     " ",argv[2],": can't open tracefile ",argv[2]," for writing",
			     NULL);
	    
		return (TCL_ERROR); 
	    }
	} else if (tracefd) {
	    ///### stop trace
	    if (fclose(tracefd)) {
		Tcl_AppendResult(interp,
			     Tcl_GetCommandName(interp,info->hpibCmd),
			     " : can't close tracefile ",
			     NULL);
		
		return (TCL_ERROR); 
	    }
	}

        /*if ((ret=itrace (info->dev_inst,tracefd)))
        {
            ret = igeterrno ();

            Tcl_ResetResult (interp);
            Tcl_AppendResult (interp,
                              Tcl_GetCommandName (interp, info->hpibCmd),
                              " ", argv[1], ":",
                              igeterrstr (ret),
                              NULL);
	    if (ret) {
		Tcl_SetErrorCode (interp, "HPIB", "TRACE",
				  Tcl_GetStringResult(interp), NULL);
		return (TCL_ERROR); 
	    }
            }*/
    }
    else if (strcmp (argv[1], "timeout") == 0)
    {
        long new_timeout;
	int ih;
        if (info->dev_inst == 0)
        {
            Tcl_ResetResult (interp);
            Tcl_AppendResult (interp,
                              Tcl_GetCommandName (interp, info->hpibCmd),
                              " ", argv[1], ":",
                              "is closed",
                              NULL);
            Tcl_SetErrorCode (interp, "HPIB", "CLEAR",
                              Tcl_GetStringResult(interp), NULL);
            return (TCL_ERROR); 
        }
	if (Tcl_GetInt(interp,argv[2],&ih)) {
	  Tcl_AppendResult (interp,
			    argv[0]," ",argv[1],
			  ": non-integer timeout '",argv[2],
			  "' specified",
			  NULL);
	  return (TCL_ERROR); 
	}
	if (ih<0) {
	  /* reset to value specified at open */
	  new_timeout=info->timeout;
	} else {
	  new_timeout=ih;
	}

        if (itimeout (info->dev_inst, new_timeout) != 0)
        {
            ret = igeterrno ();

            Tcl_ResetResult (interp);
            Tcl_AppendResult (interp,
                              Tcl_GetCommandName (interp, info->hpibCmd),
                              " ", argv[1], ":",
                              igeterrstr (ret),
                              NULL);
            Tcl_SetErrorCode (interp, "HPIB", "TIMEOUT",
                              Tcl_GetStringResult(interp), NULL);
            return (TCL_ERROR); 
        }
 
    }
    else if (strcmp (argv[1], "lan_timeout") == 0)
    {
        long new_timeout;
	int ih;
        if (info->dev_inst == 0)
        {
            Tcl_ResetResult (interp);
            Tcl_AppendResult (interp,
                              Tcl_GetCommandName (interp, info->hpibCmd),
                              " ", argv[1], ":",
                              "is closed",
                              NULL);
            Tcl_SetErrorCode (interp, "HPIB", "CLEAR",
                              Tcl_GetStringResult(interp), NULL);
            return (TCL_ERROR); 
        }
	if (Tcl_GetInt(interp,argv[2],&ih)) {
	  Tcl_AppendResult (interp,
			    argv[0]," ",argv[1],
			  ": non-integer timeout '",argv[2],
			  "' specified",
			  NULL);
	  return (TCL_ERROR); 
	}
	if (ih<0) {
	  /* reset to value specified at open */
	  new_timeout=info->lan_timeout;
	} else {
	  new_timeout=ih;
	}

        if (ilantimeout (info->dev_inst, new_timeout) != 0)
        {
            ret = igeterrno ();

            Tcl_ResetResult (interp);
            Tcl_AppendResult (interp,
                              Tcl_GetCommandName (interp, info->hpibCmd),
                              " ", argv[1], ":",
                              igeterrstr (ret),
                              NULL);
            Tcl_SetErrorCode (interp, "HPIB", "TIMEOUT",
                              Tcl_GetStringResult(interp), NULL);
            return (TCL_ERROR); 
        }
 
    }
    else if (strcmp (argv[1], "readstb" ) == 0 ||
	       (lan_retry=strcmp(argv[1],"readstb_noretry")) == 0)
    {
        unsigned char status;
        if (info->dev_inst == 0)
        {
            Tcl_ResetResult (interp);
            Tcl_AppendResult (interp,
                              Tcl_GetCommandName (interp, info->hpibCmd),
                              " ", argv[1], ":",
                              "is closed",
                              NULL);
            Tcl_SetErrorCode (interp, "HPIB", "CLEAR",
                              Tcl_GetStringResult(interp), NULL);
            return (TCL_ERROR); 
        }

        if ((ret=ireadstb (info->dev_inst, &status)))
        {
            ret = igeterrno ();

            Tcl_ResetResult (interp);
            Tcl_AppendResult (interp,
                              Tcl_GetCommandName (interp, info->hpibCmd),
                              " ", argv[1], ":",
                              igeterrstr (ret),
                              NULL);
	    if (lan_retry!=0 && !Hpib_retry_iopen(interp,info)) {
	      /* interface could be opened again, retry the command */
	      ret=ireadstb (info->dev_inst, &status);
	      if (ret)  {
		ret = igeterrno ();
		Tcl_AppendResult (interp,
				  "\n and could not trigger after new open: ",
				  igeterrstr (ret),
				  NULL);
	      }
	    }
	}
	if (ret) {
            Tcl_SetErrorCode (interp, "HPIB", "READSTB",
                              Tcl_GetStringResult(interp), NULL);
            return (TCL_ERROR); 
        }
	Tcl_ResetResult (interp);
        Tcl_SetObjResult(interp, Tcl_NewIntObj(status));
	//sprintf (Tcl_GetStringResult(interp), "%d", status);
    }
	else if (strcmp(argv[1],"write") == 0 ||
	       (lan_retry=strcmp(argv[1],"write_noretry")) == 0 ||
	       (write_eoi=strcmp(argv[1],"write_noeoi")) == 0	)    
	{
	
        if (argc != 3)
        {
            Tcl_AppendResult (interp,
                              Tcl_GetCommandName (interp, info->hpibCmd),
                              " ", argv[1], ": invalid number of arguments",
                              NULL);
            return (TCL_ERROR); 
        }

        if (info->dev_inst == 0)
        {
            Tcl_ResetResult (interp);
            Tcl_AppendResult (interp,
                              Tcl_GetCommandName (interp, info->hpibCmd),
                              " ", argv[1], ":",
                              "is closed",
                              NULL);
            Tcl_SetErrorCode (interp, "HPIB", "WRITE",
                              Tcl_GetStringResult(interp), NULL);
            return (TCL_ERROR); 
        }

        wrlength = strlen (argv[2]);
        timeout  = (long) (HPIB_DEFAULT_TIMEOUT * (1 + wrlength / 1000.));

	block_itimer();
        ret = iwrite (info->dev_inst, argv[2], wrlength, write_eoi, NULL);
        unblock_itimer();
	if (ret)  {
            ret = igeterrno ();
	    Tcl_ResetResult (interp);
	    Tcl_AppendResult (interp,
			      Tcl_GetCommandName (interp, info->hpibCmd),
			      " ", argv[1], ":",
			      igeterrstr (ret),
			      NULL);
	    if (lan_retry!=0 && !Hpib_retry_iopen(interp,info)) {
	      /* interface could be opened again, retry the write */
	      block_itimer();
	      ret = iwrite (info->dev_inst, argv[2], wrlength, write_eoi, NULL);
	      unblock_itimer();
	      if (ret)  {
		ret = igeterrno ();
		Tcl_AppendResult (interp,
				  "\n and could not write after new open: ",
				  igeterrstr (ret),
				  NULL);
	      }
     
	    }
	}
	if (ret) {
	  Tcl_SetErrorCode (interp, "HPIB", "WRITE",
			    Tcl_GetStringResult(interp), NULL);
	  return (TCL_ERROR); 
	}

    }
	else if (strcmp(argv[1],"write_read") == 0 ||
		(lan_retry=strcmp(argv[1],"write_read_noretry")) == 0  )    
	{
        if (argc != 3)
        {
            Tcl_AppendResult (interp,
                              Tcl_GetCommandName (interp, info->hpibCmd),
                              " ", argv[1], ": invalid number of arguments",
                              NULL);
            return (TCL_ERROR); 
        }
	
        if (info->dev_inst == 0)
        {
            Tcl_ResetResult (interp);
            Tcl_AppendResult (interp,
                              Tcl_GetCommandName (interp, info->hpibCmd),
                              " ", argv[1], ":",
                              "is closed",
                              NULL);
            Tcl_SetErrorCode (interp, "HPIB", "WRITE_READ",
                              Tcl_GetStringResult(interp), NULL);
            return (TCL_ERROR); 
        }

        wrlength = strlen (argv[2]);
        timeout  = (long) (HPIB_DEFAULT_TIMEOUT * (1 + wrlength / 1000.));

	block_itimer();
        ret = iwrite (info->dev_inst, argv[2], wrlength, 1, NULL);
        unblock_itimer();
	if (ret)  {
            ret = igeterrno ();
	    Tcl_ResetResult (interp);
	    Tcl_AppendResult (interp,
			      Tcl_GetCommandName (interp, info->hpibCmd),
			      " ", argv[1], ":",
			      igeterrstr (ret),
			      NULL);
	    if (lan_retry!=0 && !Hpib_retry_iopen(interp,info)) {
		/* interface could be opened again, retry the write */
		block_itimer();
		ret = iwrite (info->dev_inst, argv[2], wrlength, 1, NULL);
		unblock_itimer();
		if (ret)  {
		    ret = igeterrno ();
		    Tcl_AppendResult (interp,
				      "\n and could not write after new open: ",
				      igeterrstr (ret),
				      NULL);
		}
		
	    }
	}
	if (ret) {
	    Tcl_SetErrorCode (interp, "HPIB", "WRITE",
			    Tcl_GetStringResult(interp), NULL);
	    return (TCL_ERROR); 
	}

	block_itimer();
	ret = iread (info->dev_inst, 
		     hpibResultBuffer,
		     sizeof (hpibResultBuffer), &reason, &rdlength);
	unblock_itimer();
	if (ret)  {
	    ret = igeterrno ();
	    Tcl_ResetResult (interp);
	    Tcl_AppendResult (interp,
			      Tcl_GetCommandName (interp, info->hpibCmd),
			      " ", argv[1], ":",
			      igeterrstr (ret),
			      NULL);
	    Tcl_SetErrorCode (interp, "HPIB", "READ",
			      Tcl_GetStringResult(interp), NULL);
	    return (TCL_ERROR); 
	}
	if (rdlength < (sizeof (hpibResultBuffer) - 1)) {
            if (hpibResultBuffer[0] == '#')
            {
                if (hpibResultBuffer[1] == '8')  {
                    for (idx = 10; idx < (rdlength - 1); ++idx) {
                        sprintf (asciibuf, "%d", hpibResultBuffer[idx] & 0xFF);
                        Tcl_AppendElement (interp, asciibuf);
                    }
                }
            } else {
                hpibResultBuffer[rdlength] = '\0';
                Tcl_SetResult (interp, hpibResultBuffer, NULL);
            }
            return (TCL_OK);
        } else {
	    hpibResultBuffer[rdlength] = '\0';
	    Tcl_SetResult (interp, hpibResultBuffer, NULL);
	    /** ?? Tcl_GetStringResult(interp)[TCL_RESULT_SIZE] = '\0'; **/
	    Tcl_AppendResult (interp, "\nError: ",
                              Tcl_GetCommandName (interp, info->hpibCmd),
                              " ", argv[1], ": buffer too small!", NULL);
            return (TCL_ERROR); 
        }
	

    }
	else if (strcmp (argv[1], "read") == 0)  
	{

	if (argc != 2) {
            Tcl_AppendResult(interp,
                             Tcl_GetCommandName(interp,info->hpibCmd),
                             " ",argv[1],": invalid number of arguments",
                             NULL);
            return (TCL_ERROR); 
	}

	if (info->dev_inst == 0) {
	    Tcl_ResetResult (interp);
	    Tcl_AppendResult (interp,
			      Tcl_GetCommandName (interp, info->hpibCmd),
			      " ", argv[1], ":",
			      "is closed",
			      NULL);
	    Tcl_SetErrorCode (interp, "HPIB", "READ",
			      Tcl_GetStringResult(interp), NULL);
	    return (TCL_ERROR); 
	}

	block_itimer();
	ret = iread (info->dev_inst, 
		     hpibResultBuffer,
		     sizeof (hpibResultBuffer), &reason, &rdlength);
	unblock_itimer();
	if (ret)  {
	    ret = igeterrno ();
	    Tcl_ResetResult (interp);
	    Tcl_AppendResult (interp,
			      Tcl_GetCommandName (interp, info->hpibCmd),
			      " ", argv[1], ":",
			      igeterrstr (ret),
			      NULL);
	    
	    if (!Hpib_retry_iopen(interp,info)) {
		/* interface could be opened again, retry the read */
		block_itimer();
		ret = iread (info->dev_inst, 
			     hpibResultBuffer,
			     sizeof (hpibResultBuffer), &reason, &rdlength);
		unblock_itimer();
		if (ret)  {
		    ret = igeterrno ();
		    Tcl_AppendResult (interp,
				      "\n and could not read after new open: ",
				      igeterrstr (ret),
				      NULL);
		}
	    }
	}
	if (ret) {
	    Tcl_SetErrorCode (interp, "HPIB", "READ",
			      Tcl_GetStringResult(interp), NULL);
	    return (TCL_ERROR); 
	}
	if (rdlength < (sizeof (hpibResultBuffer) - 1)) {
            if (hpibResultBuffer[0] == '#')
            {
                if (hpibResultBuffer[1] <= '8' && hpibResultBuffer[1] >= '1')
                {
                    for (idx = hpibResultBuffer[1] - '0' + 2; idx < (rdlength - 1); ++idx)
                    {
                        sprintf (asciibuf, "%d", hpibResultBuffer[idx] & 0xFF);
                        Tcl_AppendElement (interp, asciibuf);
                    }
                } else {
		    /* something wrong, put in first two bytes*/ 
		    hpibResultBuffer[2] = '\0';
		    Tcl_SetResult (interp, hpibResultBuffer, NULL);
		}
            }
            else
            {
                hpibResultBuffer[rdlength] = '\0';
                Tcl_SetResult (interp, hpibResultBuffer, NULL);
            }
	    
            return (TCL_OK);
        }
        else
        {
	    hpibResultBuffer[rdlength] = '\0';
	    Tcl_SetResult (interp, hpibResultBuffer, NULL);
	    /** ?? Tcl_GetStringResult(interp)[TCL_RESULT_SIZE] = '\0'; **/
	    Tcl_AppendResult (interp, "\nError: ",
                              Tcl_GetCommandName (interp, info->hpibCmd),
                              " ", argv[1], ": buffer too small!", NULL);
            return (TCL_ERROR); 
        }
    }
    else if (strcmp (argv[1], "upload") == 0)  
	{
	int iopen_retries=0;
	char * write_pos=&hpibResultBuffer[0];
	unsigned long data_len;
	int valdigits = 0;
	unsigned long datasize = 0;
	if (argc != 3) {
	    Tcl_AppendResult(interp,
			     Tcl_GetCommandName(interp,info->hpibCmd),
			     " ",argv[1],": invalid number of arguments",
			     NULL);
	    return(TCL_ERROR); 
	}
	
	if (info->dev_inst == 0) {
	    Tcl_ResetResult(interp);
	    Tcl_AppendResult(interp,
			     Tcl_GetCommandName(interp, info->hpibCmd),
			     " ", argv[1], ":",
			     "is closed",
			     NULL);
	    Tcl_SetErrorCode(interp, "HPIB", "READ", Tcl_GetStringResult(interp), NULL);
	    return(TCL_ERROR); 
	}

	// we have to switch the termchar if enabled
	// deactivate it so binary data can flow
	if (info->termchar != -1) {
	    if (itermchr (info->dev_inst, -1) != 0)
	    {
		ret = igeterrno ();
		
		Tcl_ResetResult (interp);
		Tcl_AppendResult (interp,
				  Tcl_GetCommandName (interp, info->hpibCmd),
				  " ", argv[1], ":",
				  "could not reset termchar to -1",
				  NULL);
		Tcl_SetErrorCode (interp, "HPIB", "TERMCHAR",
				  Tcl_GetStringResult(interp), NULL);
		return (TCL_ERROR); 
	    }
	}

	// read size of digits that tell us the amount of data to read
	ret = iread(info->dev_inst, hpibResultBuffer, 2, &reason, &data_len);
	if (ret) {
		ret = igeterrno();
		Tcl_ResetResult(interp);
		Tcl_AppendResult(interp,
				 Tcl_GetCommandName (interp, info->hpibCmd),
				 " ", argv[1], ":",
				 igeterrstr(ret),
				 NULL);
     
		Tcl_SetErrorCode(interp, "HPIB", "READ", Tcl_GetStringResult(interp), NULL);
		return (TCL_ERROR); 
	}
	if (hpibResultBuffer[0] != '#') {
		Tcl_ResetResult(interp);
		Tcl_AppendResult(interp,
				 Tcl_GetCommandName (interp, info->hpibCmd),
				 " ", argv[1], ":",
				 "invalid data, expected # not found",
				 NULL);
     
		Tcl_SetErrorCode(interp, "HPIB", "READ", Tcl_GetStringResult(interp), NULL);
		return (TCL_ERROR); 
	}
	valdigits = hpibResultBuffer[1] - '0';
	// now we can read the digits
	ret = iread(info->dev_inst, hpibResultBuffer, valdigits, &reason, &data_len);
	hpibResultBuffer[valdigits] = 0;
	if (ret) {
		ret = igeterrno();
		Tcl_ResetResult(interp);
		Tcl_AppendResult(interp,
				 Tcl_GetCommandName (interp, info->hpibCmd),
				 " ", argv[1], ":",
				 igeterrstr(ret),
				 NULL);
     
		Tcl_SetErrorCode(interp, "HPIB", "READ", Tcl_GetStringResult(interp), NULL);
		return (TCL_ERROR); 
	}
	// now convert the asci data size to long
	datasize = atol(hpibResultBuffer);
	// check if we have enough buffer
	if (datasize > sizeof(hpibResultBuffer)) {
	    Tcl_ResetResult(interp);
	    Tcl_AppendResult(interp,
			     Tcl_GetCommandName (interp, info->hpibCmd),
			     " ", argv[1], ":",
			     "not enough internal buffer to copy data",
			     NULL);
	    
	    Tcl_SetErrorCode(interp, "HPIB", "READ", Tcl_GetStringResult(interp), NULL);
	    return (TCL_ERROR); 
	}

	rdlength=0;
	while (1) {
	    block_itimer();
	    ret = iread(info->dev_inst, write_pos, sizeof(hpibResultBuffer) - rdlength, &reason, &data_len);
	    unblock_itimer();
	    if (ret) {
		ret = igeterrno();
		Tcl_ResetResult(interp);
		Tcl_AppendResult(interp,
				 Tcl_GetCommandName (interp, info->hpibCmd),
				 " ", argv[1], ":",
				 igeterrstr(ret),
				 NULL);
     
		if (0 == iopen_retries++ && !Hpib_retry_iopen(interp,info)) {
		    /* interface could be opened again, retry the read */ 
		    continue;
		}
		Tcl_SetErrorCode(interp, "HPIB", "READ", Tcl_GetStringResult(interp), NULL);
		return (TCL_ERROR); 
	    }
	    rdlength = rdlength + data_len;
	    write_pos = write_pos + data_len;

	    if (data_len == 0) break;

	    if (reason & I_TERM_MAXCNT ) {
		break; /* if buffer is filled */
	    }
	    if (reason & I_TERM_END ) {
		break; /* if the END indicator arrived. */
	    }
	    if (info->termchar >=0 && (reason & I_TERM_CHR)) {
		break; /* if termchar was defined and is received */
	    }
	    if (rdlength >= datasize) {
		break; /* we have read enough data */
	    }	    
	}

	// activate the termchar again
	if (info->termchar != -1)
	{
	    if (itermchr (info->dev_inst, info->termchar) != 0)
	    {
		ret = igeterrno ();
		
		Tcl_ResetResult (interp);
		Tcl_AppendResult (interp,
				  Tcl_GetCommandName (interp, info->hpibCmd),
				  " ", argv[1], ":",
				  "could not set termchar",
				  NULL);
		Tcl_SetErrorCode (interp, "HPIB", "TERMCHAR",
				  Tcl_GetStringResult(interp), NULL);
		return (TCL_ERROR); 
	    }
	}

	// write data to disk
	if ((ifp = fopen(argv[2], "w+b")) == NULL) {
	    Tcl_AppendResult(interp,
			     Tcl_GetCommandName(interp,info->hpibCmd),
			     " ",argv[2],": can't open ",argv[2]," for writing",
			     NULL);
	    
	    return (TCL_ERROR); 
	}
	value = fwrite(&hpibResultBuffer, sizeof(char), datasize, ifp);
	fclose(ifp);
	// if writing went wrong
	if ((unsigned)value != datasize) {
	    Tcl_AppendResult(interp,
			     Tcl_GetCommandName(interp,info->hpibCmd),
			     " ",argv[2],": can't write file",
			     NULL);
	    return (TCL_ERROR);
	}

	// append result
	sprintf(hpibResultBuffer, "%li", datasize);
	Tcl_SetResult (interp, hpibResultBuffer, NULL);
	return (TCL_OK);

	if (rdlength < (sizeof (hpibResultBuffer) - 1)) {
            if (hpibResultBuffer[0] == '#')  {
                if (hpibResultBuffer[1] <= '8' && hpibResultBuffer[1] >= '1')  {
		    int start = hpibResultBuffer[1] - '0' + 2;
		    if ((ifp = fopen(argv[2], "w+b")) == NULL) {
			Tcl_AppendResult(interp,
					 Tcl_GetCommandName(interp,info->hpibCmd),
					 " ",argv[2],": can't open ",argv[2]," for writing",
					 NULL);
			
			return (TCL_ERROR); 
		    }
		    
		    // save file to disk
		    value = fwrite(&hpibResultBuffer[start], sizeof(char), rdlength - start - 1, ifp);
		    fclose(ifp);
		    
		    // append result
		    sprintf(&hpibResultBuffer[start], " %i", value);
		    Tcl_SetResult (interp, hpibResultBuffer, NULL);
		    
		    // if writing went wrong
		    if ((unsigned)value != (rdlength - start - 1))
		    {
			Tcl_AppendResult(interp,
					 Tcl_GetCommandName(interp,info->hpibCmd),
					 " ",argv[2],": can't write file",
					 NULL);
			return (TCL_ERROR);
		    }
                } else {
		    /* something wrong, put in first two bytes*/ 
		    hpibResultBuffer[2] = '\0';
		    Tcl_SetResult (interp, hpibResultBuffer, NULL);
		}
            }
            else
            {
                hpibResultBuffer[rdlength] = '\0';
                Tcl_SetResult (interp, hpibResultBuffer, NULL);
            }

            return (TCL_OK);
        }
        else
        {
			hpibResultBuffer[0] = '\0';
			Tcl_SetResult (interp, hpibResultBuffer, NULL);
			/** ?? Tcl_GetStringResult(interp)[TCL_RESULT_SIZE] = '\0'; **/
			Tcl_AppendResult(interp, "\nError: ",
							 Tcl_GetCommandName (interp, info->hpibCmd),
							 " ", argv[1], ": buffer too small!", NULL);
            return (TCL_ERROR); 
        }
    }
	else if (strcmp (argv[1], "uploadbin") == 0)  
	{
		int iopen_retries=0;
		char * write_pos=&hpibResultBuffer[0];
		unsigned long data_len;
		int valdigits = 0;
		unsigned long datasize = 0;
		if (argc > 3) {
			Tcl_AppendResult(interp,
					 Tcl_GetCommandName(interp,info->hpibCmd),
					 " ",argv[1],": invalid number of arguments",
					 NULL);
			return(TCL_ERROR); 
		}
		
		if (info->dev_inst == 0) {
			Tcl_ResetResult(interp);
			Tcl_AppendResult(interp,
					 Tcl_GetCommandName(interp, info->hpibCmd),
					 " ", argv[1], ":",
					 "is closed",
					 NULL);
			Tcl_SetErrorCode(interp, "HPIB", "READ", Tcl_GetStringResult(interp), NULL);
			return(TCL_ERROR); 
		}

		if (argc == 3) {
		  // write data to disk
			if ((ifp = fopen(argv[2], "w+b")) == NULL) {
				Tcl_AppendResult(interp,
					 Tcl_GetCommandName(interp,info->hpibCmd),
					 " ",argv[2],": can't open ",argv[2]," for writing",
					 NULL);
			
				return (TCL_ERROR); 
			}

			ret = 0;
			while (ret == 0) {
				ret = iread(info->dev_inst, hpibResultBuffer, HPIB_RESULT_BUFFER_SIZE, &reason, &data_len);

				// check for error
				if (ret) {
					Tcl_AppendResult(interp,
						 Tcl_GetCommandName(interp,info->hpibCmd),
						 " ",argv[2],": iread error",
						 NULL);
					fclose(ifp);
					return (TCL_ERROR);
				}

				datasize += data_len;

				// write to file
				value = fwrite(&hpibResultBuffer, sizeof(char), data_len, ifp);
				// check for error
				if ((unsigned)value != data_len) {
					Tcl_AppendResult(interp,
						 Tcl_GetCommandName(interp,info->hpibCmd),
						 " ",argv[2],": can't write file",
						 NULL);
					fclose(ifp);
					return (TCL_ERROR);
				}

				// check if we are finished
				if (reason & I_TERM_END ) {
					break; /* if the END indicator arrived. */
				}
				if (reason & I_TERM_CHR) {
					break; /* if termchar was defined and is received */
				}
			}

			fclose(ifp);

			sprintf(hpibResultBuffer, "%li", datasize);
			Tcl_SetResult (interp, hpibResultBuffer, NULL);
			return (TCL_OK);
			
		} else {
		  // write data to object
		  int byteLen=0;
		  int newlength=0;
		  char* bytes=NULL;
		  int ix;
		  Tcl_Obj* byteObj=Tcl_NewByteArrayObj(hpibResultBuffer, 0);
		  if (byteObj==NULL) {
		    Tcl_AppendResult(interp,
				     Tcl_GetCommandName(interp,info->hpibCmd),
				     " ",argv[2],": can't allocate byteObj",
				     NULL);
		    return (TCL_ERROR); 
		  }

		  ret = 0;
		  while (ret == 0) {
		    ret = iread(info->dev_inst, hpibResultBuffer, HPIB_RESULT_BUFFER_SIZE, &reason, &data_len);

		    // check for error
		    if (ret) {
		      Tcl_AppendResult(interp,
				       Tcl_GetCommandName(interp,info->hpibCmd),
				       " ",argv[2],": iread error",
				       NULL);
		      return (TCL_ERROR);
		    }
		    
		    datasize += data_len;

		    // write to Obj
		    bytes=Tcl_SetByteArrayLength(byteObj, byteLen+data_len);
		    if (bytes==NULL) {
		      Tcl_AppendResult(interp,
				       Tcl_GetCommandName(interp,info->hpibCmd),
				       " ",argv[2],": can't încrease byteObj length",
				       NULL);
		      return (TCL_ERROR); 
		    }
		    for (ix=0;ix<data_len;++ix) {
		      bytes[byteLen+ix]=hpibResultBuffer[ix];
		    }
		    byteLen=byteLen+data_len;

		    // check if we are finished
		    if (reason & I_TERM_END ) {
		      break; /* if the END indicator arrived. */
		    }
		    if (reason & I_TERM_CHR) {
		      break; /* if termchar was defined and is received */
		    }
		  }
		  Tcl_SetObjResult(interp,byteObj);
		  return (TCL_OK);

		}
	}
    else if (strcmp (argv[1], "download") == 0)
    {
        if (argc != 4)
        {
            Tcl_AppendResult(interp,
                             Tcl_GetCommandName(interp,info->hpibCmd),
                             " ",argv[1],": invalid number of arguments",
                             NULL);
            return (TCL_ERROR); 
        }

        if (info->dev_inst == 0)
        {
            Tcl_ResetResult (interp);
            Tcl_AppendResult (interp,
                              Tcl_GetCommandName (interp, info->hpibCmd),
                              " ", argv[1], ":",
                              "is closed",
                              NULL);
            Tcl_SetErrorCode (interp, "HPIB", "READ",
                              Tcl_GetStringResult(interp), NULL);
            return (TCL_ERROR); 
        }

        if ((ifp = fopen (argv[3], "r")) == NULL)  {
            Tcl_AppendResult(interp,
                             Tcl_GetCommandName(interp,info->hpibCmd),
                             " ",argv[3],": can't open ",argv[3]," for reading",
                             NULL);
            return (TCL_ERROR); 
        }


 
	/* the next read is a dummy read only to ignore the first line */
	while ((ret=fgetc(ifp))!=EOF && ret !='\n');
	if (ret == EOF)   {
	    fclose (ifp);
	    Tcl_AppendResult(interp,
			     Tcl_GetCommandName(interp,info->hpibCmd),
			     " ",argv[1],": can't read dummy header from file ",
			     argv[3],
			     NULL);
	    return (TCL_ERROR); 
	} 
 

        if (fscanf (ifp, "%d", &valcount) != 1)  {
            fclose (ifp);
            Tcl_AppendResult(interp,
                             Tcl_GetCommandName(interp,info->hpibCmd),
                             " ",argv[1],": can't read value count from file",
                             NULL);
            return (TCL_ERROR); 
        }

        wrlength = strlen (argv[2]) + 11 + valcount;
        timeout  = (long) (HPIB_DEFAULT_TIMEOUT * (1 + wrlength / 1000.));

        /* Allocate buffer for <prefix> #8<count><binary data> */
        /*     if ((writebuf = malloc (wrlength)) == NULL)  { */
	if ((writebuf = calloc (1,wrlength)) == NULL) {
            fclose (ifp);
            sprintf (asciibuf, "%li", wrlength);
            Tcl_AppendResult(interp,
                             Tcl_GetCommandName(interp,info->hpibCmd),
                             " ",argv[1],": can't allocate ",asciibuf," bytes",
                             NULL);
            return (TCL_ERROR); 
        }

        /* Write ASCII header data */
	bufp = writebuf;
        memcpy (bufp, argv[2], strlen (argv[2]));
	bufp+=strlen(argv[2]);
        sprintf(bufp, " #8%08d", valcount);
	bufp+=11;
        for (idx = 2; valcount > 0; --valcount, ++bufp, ++idx)  {
            if ((ret = fscanf (ifp, "%d", &value)) != 1) {
                free (writebuf);
                fclose (ifp);
		sprintf (asciibuf, "%li %s", idx, (ret==EOF)?"(EOF)":"(Format error?)");
                Tcl_AppendResult(interp,
                                 Tcl_GetCommandName(interp,info->hpibCmd),
                                 " ",argv[1],": can't read value at line ",
                                 asciibuf,
                                 NULL);
                return (TCL_ERROR); 
            }

            if ((value < 0) || (value > 255)) {
                free (writebuf);
                fclose (ifp);
		sprintf (asciibuf, "val(%d) at line %li", value, idx);
                Tcl_AppendResult(interp,
                                 Tcl_GetCommandName(interp,info->hpibCmd),
                                 " ",argv[1],": value out of range ",asciibuf,
                                 NULL);
                return (TCL_ERROR); 
            }
            *bufp = (unsigned char) value;
	    printf (" --- %li %d %#x %#x\n",idx,value,value, *bufp);
        }
#ifndef DEBUG
        block_itimer();

	// we have to switch the termchar if enabled
	// deactivate it so binary data can flow
	if (info->termchar != -1) itermchr (info->dev_inst, -1);

	ret = iwrite (info->dev_inst, writebuf, wrlength, 1, NULL);
	
	// activate the termachar again
	if (info->termchar != -1) itermchr (info->dev_inst, info->termchar);
	
        unblock_itimer();
        if (ret)  {
            free (writebuf);
            fclose (ifp);

            ret = igeterrno ();
	    
            Tcl_ResetResult (interp);
            Tcl_AppendResult (interp,
                              Tcl_GetCommandName (interp, info->hpibCmd),
                              " ", argv[1], ":",
                              igeterrstr (ret),
                              NULL);
            Tcl_SetErrorCode (interp, "HPIB", "WRITE",
                              Tcl_GetStringResult(interp), NULL);
            return (TCL_ERROR); 
        }
#else
        for (idx = 0; idx < wrlength; idx += BUFSIZE)
        {
        
            dump_line (idx, &writebuf[idx],
                       ((idx+BUFSIZE) > wrlength)?(wrlength-idx):BUFSIZE,
                       stdout);
        }
#endif
        free (writebuf);
        fclose (ifp);
    }
    else
    if (strcmp (argv[1], "srq") == 0)
    {
        result = Hpib_SrqConfig (clientdata, interp, argc, argv);
    }
    else
    if (strcmp (argv[1], "configure") == 0)
    {
        if (argc == 2)
        {
            result = Tcl_ConfigureInfo (interp, hpibDeviceSpec,
                                        (char *) info, (char *) 0, 0);
        }
        else
        {    
            if (info->dev_inst != 0)
            {
                Tcl_ResetResult (interp);
                Tcl_AppendResult (interp,
                                  Tcl_GetCommandName (interp, info->hpibCmd),
                                  " ", argv[1], ":",
                                  "is open, close before configure",
                                  NULL);
                Tcl_SetErrorCode (interp, "HPIB", "READ",
                                  Tcl_GetStringResult(interp), NULL);
                return (TCL_ERROR); 
            }

            result = Tcl_ConfigureStruct (interp, hpibDeviceSpec,
                                          argc-2, argv+2, (char *) info, 0);
        }
    }    
    else
    {
        /* Unknown device option */
        Tcl_AppendResult (interp,
                          "invalid hpib device command \"",
                          argv[1], 
                          "\"\n",
                          NULL);
        result = TCL_ERROR; 
    }

    return result;
}

/******************************************************************************/
















