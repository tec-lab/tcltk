/*-----------------------------------------------------------------------------
//
// (C) 2004     European Space Agency
//              European Space Operations Centre
//              Darmstadt, Germany
//
// -----------------------------------------------------------------------------
//
//  System          MCM4 - Monitoring and Control Module
//
// subsystem:   sicl
// 
// file:        sicl.h
//
// purpose:     COTS layer to the GPIB/LAN controller
//
// comments:    Provides the interface to the Agilent E5810A via the VXI11 protocol
//
// authors:     H. Kubr
//
// history:     Re-used from Siemens project
//
//---------------------------------------------------------------------------*/
#ifndef sicl_h
#define sicl_h
/*
 * Minimalvariante von sicl.h
 */

#include "vxi11core.h"



/*
 * define out Windows specific keywords if not compiling for Windows
 */
#if !defined(_MS_DOS_WIN)
#define _near
#define _far
#define _huge
#define _pascal
#define _export
#endif

/* define buffer size for RPC connection */
#define MAX_SICL_BUFFER  1024*10124 


/*
 * define SICLAPI, SICLAPIV, and SICLCALLBACK appropriately
 */
#if defined(_SICL_WIN32)
#define SICLAPI __cdecl
#define SICLAPIV __cdecl
#define SICLCALLBACK __cdecl
#else
#if defined(_MS_DOS_WIN)
#define SICLCALLBACK _far _pascal _export
#define SICLAPI _far _pascal
#define SICLAPIV _far __cdecl
#else
#define SICLAPI
#define SICLAPIV
#define SICLCALLBACK
#endif
#endif

/*
 * iread termination conditions
 */
#define I_TERM_MAXCNT      1
#define I_TERM_CHR      2
#define I_TERM_END      4
#define I_TERM_NON_BLOCKED 8

/*
 * Definition of INST:
 */

typedef int INST;

/*
 * Error Codes
 */
#define I_ERR_NOERROR        0
#define I_ERR_SYNTAX         1
#define I_ERR_SYMNAME        2
#define I_ERR_BADADDR        3
#define I_ERR_BADID          4
#define I_ERR_PARAM          5
#define I_ERR_NOCONN         6
#define I_ERR_NOPERM         7
#define I_ERR_NOTSUPP        8
#define I_ERR_NORSRC         9
#define I_ERR_NOINTF        10
#define I_ERR_LOCKED        11
#define I_ERR_NOLOCK        12
#define I_ERR_BADFMT        13
#define I_ERR_DATA          14
#define I_ERR_TIMEOUT       15
#define I_ERR_OVERFLOW      16
#define I_ERR_IO            17
#define I_ERR_OS            18
#define I_ERR_BADMAP        19
#define I_ERR_NODEV         20
#define I_ERR_INVLADDR      21
#define I_ERR_NOTIMPL       22
#define I_ERR_ABORTED       23
#define I_ERR_BADCONFIG     24
#define  I_ERR_NOCMDR       25
#define I_ERR_VERSION       26
#define I_ERR_NESTEDIO      27
#define I_ERR_BUSY          28
#define I_ERR_CONNEXISTS    29
#define I_ERR_BADREMADDR    32
#define I_ERR_NOMOREDEV     33  // more than 16 devs on the hpib bus
#define I_ERR_ABORTCHCR     34  // unable to create the abort channel
#define I_ERR_NOABORTCH     35  // no abort channel created
#if !defined(STD_SICL)
#define I_ERR_BUSERR        30
#define I_ERR_BUSERR_RETRY  31
#define I_ERR_INTERNAL     128
#define I_ERR_INTERRUPT    129
#define I_ERR_UNKNOWNERR   130
#endif

/* igpibbusstatus values */
#define I_GPIB_BUS_REM         1
#define I_GPIB_BUS_SRQ         2
#define I_GPIB_BUS_NDAC        3
#define I_GPIB_BUS_SYSCTLR     4
#define I_GPIB_BUS_ACTCTLR     5
#define I_GPIB_BUS_TALKER      6
#define I_GPIB_BUS_LISTENER    7
#define I_GPIB_BUS_ADDR        8
#define I_GPIB_BUS_LINES       9


#define NAME_LEN 32
#define MAX_CONTROLLERS 32
#define MAX_DEVICES 32
#define LOG_SIZE 256

#define I_READ_BUF_SZ       4096
#define I_WRITE_BUF_SZ       128

/* struct Device is needed for comaptibility between the sicl Library and vxi11  */
struct Device
{
        INST            inst;   // identifier used by sicl
        char            name[NAME_LEN];   // the name of the device i.e: hpib,12
        Device_Link     linkid;    // the vxi11 identifier for the link to the device
        int             ctrl;   // is the device commander? 0=no, else yes
        FILE            *tracefile; // filedescriptor for trace
        int             termChar;  // Terminator Character for read
        long            io_timeout; // Read/Write timeout
        long            flags;  // io flags
        long            lock_timeout;  // Time to wait for the device to unlock befor sending I_ERR_LOCKED (Bit 1 in flags has to be set!)
};

struct Controller
{
        char            name[NAME_LEN];
        CLIENT          *ctrlid;
        CLIENT          *abortid;
        int             rpc_socket;
};


/* Convert time for logging */
int datetoString( time_t curr_time, char _far *strTime);


/* Init */
int iinit();


/* Open/Close */
INST SICLAPI iopen(char _far *addr);
INST SICLAPI iopen_err(char _far *addr, int *errornr);
int  SICLAPI iclose(INST id);
INST SICLAPI igetintfsess(INST id);


/* Write/Read */
int SICLAPI iwrite (
   INST id,
   const char _far *buf,
   unsigned long datalen,
   int endi,
   unsigned long _far *actual
);

int SICLAPI iread (
   INST id,
   char _far *buf,
   unsigned long bufsize,
   int _far *reason,
   unsigned long _far *actual
);

int SICLAPI itermchr(INST id,int tchr);
int SICLAPI igettermchr(INST id,int _far *tchr);


/* Device/Interface Control */
int SICLAPI iclear(INST id);
int SICLAPI iabort(INST id);
int SICLAPI ilocal (INST id);
int SICLAPI iremote (INST id);
int SICLAPI ireadstb(INST id,unsigned char _far *stb);
int SICLAPI itrigger(INST id);
int SICLAPI itrace(INST id, FILE *fd); /* Turn on tracing */
int SICLAPI igpibbusstatus (INST id, int request, int _far *result);
char *igetdevname (INST id);
// igetdevaddr returns a copy of a string, which has to be freed by the caller
char *igetdevaddr (INST id);


/* Error Handling */
int SICLAPI igeterrno (void);
int SICLAPI igeterrno_inst (INST id);
char _far *SICLAPI igeterrstr (int errornr);


/* Service Requests */
typedef void (SICLCALLBACK *srqhandler_t)(INST);
int SICLAPI ionsrq(INST id,srqhandler_t shdlr);


/* Locking */
int SICLAPI ilock(INST id);
int SICLAPI iunlock(INST id);
int SICLAPI isetlockwait(INST id,int flag);
int SICLAPI igetlockwait(INST id,int _far *flag);


/* Timeouts */
int SICLAPI itimeout(INST id,long tval);
int SICLAPI igettimeout(INST id,long _far *tval);


/* LAN Specific functions */
int SICLAPI ilantimeout(INST id,long tval);
int SICLAPI ilangettimeout(INST id,long _far *tval);

#endif  /* sicl_h */
