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
// file:        sicl.c
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
/*
     This file implements the most important sicl functions using the vxi11 library.
     Modified by Palivec to provide thread-safe functionality for EQGgpibLanLayer.C
     Modified by O. Maenner to retain call-compatible API to HP-SICL
*/


#include <string.h>
#include <time.h>
#include <stdio.h>
#include <netdb.h>
#include <pthread.h>

// windows
//#include <winsock2.h>
//#include <ws2tcpip.h>

#include "sicl.h"
#include "vxi11core.h"

struct Device           device[MAX_CONTROLLERS][MAX_DEVICES];
struct Controller       controller[MAX_CONTROLLERS];
int                     controllerCount;
int                     global_ierrno;
int                     ierrno[MAX_CONTROLLERS * MAX_DEVICES], vxierrno ;
pthread_mutex_t siclMutex;
 
static char const rcsid[] =
                "VXI-SICL $Id: sicl.c,v 1.33 2007-10-18 10:13:25 maenner.o Exp $";

// =============================================================================
//
//  Function Name:  datetoString
//
//  Description:    
// Convert from time_t to string (ISO-format 'yyy.mm.dd hh:mm:ss')
//
// -----------------------------------------------------------------------------
int datetoString( time_t curr_time, char _far *strTime)
{
        struct tm *local_t;
        char buffer[25];

        if (curr_time == 0) buffer[0] = 0;
        else
        {
                // get the ascii representation
                local_t = localtime( &curr_time );
                if(local_t)
                        sprintf(buffer, "%04d.%02d.%02d %02d:%02d:%02d",
                                local_t->tm_year + 1900,
                                local_t->tm_mon + 1,
                                local_t->tm_mday,
                                local_t->tm_hour,
                                local_t->tm_min,
                                local_t->tm_sec
                                );
                else
                        buffer[0] = 0;
        }
        // set return value
        strcpy(strTime, buffer);

        return 0;
}

// =============================================================================
//
//  Function Name:  iinit
//
//  Description:    initiate the devices
//
// -----------------------------------------------------------------------------
int iinit()
{
        int ctrl,dev,i;

        controllerCount = 0;
        
        pthread_mutex_init( &siclMutex, NULL );
        
        for(i = 0; i <= (MAX_CONTROLLERS * MAX_DEVICES); i++) ierrno[i] = I_ERR_NOERROR;
	global_ierrno=0;
        vxierrno = 0;

        for ( ctrl=0; ctrl<MAX_CONTROLLERS; ctrl++)
        {
                for (dev=0; dev<MAX_DEVICES; dev++)
                {
/* hk>2004/10/05*/
                        device[ctrl][dev].inst = 0;
/* hk<2004/10/05*/
                        device[ctrl][dev].linkid = -1;
                        strcpy(device[ctrl][dev].name,"");
                        device[ctrl][dev].ctrl = 0;
                        device[ctrl][dev].tracefile = NULL;
                        device[ctrl][dev].termChar = 0;
                        device[ctrl][dev].io_timeout = 0;
                        device[ctrl][dev].lock_timeout = 0;
                        device[ctrl][dev].flags = 0;
                }
                strcpy(controller[ctrl].name,"");
                controller[ctrl].ctrlid=NULL;
                controller[ctrl].abortid=NULL;
        }
}

// =============================================================================
//
//  Function Name:  iopen
//
//  Description:    Open a device and return a INST value, describing the device
//                  On Error: return = 0 and the errornumber is set
//
// -----------------------------------------------------------------------------
INST SICLAPI iopen(char _far *addr) {
    return iopen_err (addr, &global_ierrno);
}
INST SICLAPI iopen_err (char _far *addr, int *errornr)
{
        int                     ctrl,dev, res=0, c=0, diff=0;
        CLIENT                  *rpcClient = NULL;
        Create_LinkParms        crlp;
        Create_LinkResp         crlr;
        int                     ret;
        char                    serverName[50] = {0};
        char                    deviceName[50] = {0};
        int                     err, resp;      // for the controller test
        struct timeval          timeout;

        vxierrno = 0;
        
        pthread_mutex_lock( &siclMutex );

        // Get servername and devicename out of the parameter
        if(strncmp(addr, "lan[", 4)!=0)   // Syntax error: the right format is: 'lan[servername]:hpib,xx', where xx is the device address on the bus
        {
                *errornr = I_ERR_SYNTAX;
                pthread_mutex_unlock( &siclMutex );
                return 0;
        }

        diff = 4;       // go to the beginning of servername in the string

        for (c=diff; c<strlen(addr); c++)
        {
                if(c-diff > sizeof(serverName))   // if the target string is too small -> avoid segmentation error
                {
                        *errornr = I_ERR_INTERNAL;
                        pthread_mutex_unlock( &siclMutex );
                        return 0;
                }

                if(addr[c] == ']')              // the end of the servername in the string is reached
                {
                        serverName[c-diff] = 0;
                        break;
                }

                serverName[c-diff] = addr[c];  // put this character in serverName and go for the next
        }


        if(addr[++c] != ':')       // Syntax error: the right format is: 'lan[servername]:hpib,xx', where xx is the device address on the bus
        {
                *errornr = I_ERR_SYNTAX;
                pthread_mutex_unlock( &siclMutex );
                return 0;
        }

        diff = ++c;     // go to the beginning of devicename

        for (c; c<strlen(addr); c++)
        {
                if(c-diff > sizeof(deviceName))
                {
                        *errornr = I_ERR_INTERNAL;
                        pthread_mutex_unlock( &siclMutex );
                        return 0;
                }

                deviceName[c-diff] = addr[c];
        }


        // Look whether the controller already exists

        for(ctrl=0;ctrl <= controllerCount; ctrl++)
            if ( (res = strcmp(serverName, controller[ctrl].name )) == 0)
                break;
        
        if (res)   {    // if no controller that name was found

                struct sockaddr_in server_addr;
                struct hostent *hp;
                int rpc_sock=RPC_ANYSOCK;

                /* hk>2004/10/05*/
                for(ctrl=0;ctrl < MAX_CONTROLLERS; ctrl++)
                    if ( controller[ctrl].ctrlid == NULL)
                        break;
                
                if (ctrl==MAX_CONTROLLERS)
                {
                        *errornr = I_ERR_NOMOREDEV;      // no more controllers
                        pthread_mutex_unlock( &siclMutex );
                        return (INST) 0;
                }
                /* hk<2004/10/05*/

                if (strlen(serverName) > NAME_LEN)      // if serverName doesn't fit into the namefield
                                                        // -> abort else there'd be an segmentation fault
                {                                       // if this error occours try to increase NAME_LEN in sicl.h!
                        clnt_destroy(rpcClient);
                        *errornr = I_ERR_INTERNAL;
                        pthread_mutex_unlock( &siclMutex );
                        return (INST) 0;
                }

                hp = gethostbyname(serverName);
                if (hp==NULL) {
                    fprintf(stderr, "iopen(): can't get addr for '%s'\n",serverName);
                    *errornr=I_ERR_BADADDR;
                    pthread_mutex_unlock( &siclMutex );
                    return (INST) 0;
                }
                
                timeout.tv_sec = 1;
                timeout.tv_usec = 0;
#ifdef LINUX
		/* reintroduced by OM*/ 
                bcopy(hp->h_addr, (caddr_t)&server_addr.sin_addr,hp->h_length);
                server_addr.sin_family = AF_INET;
                server_addr.sin_port =  0;
                rpcClient = clnttcp_create(&server_addr, DEVICE_CORE, DEVICE_CORE_VERSION,
                                           &rpc_sock,MAX_SICL_BUFFER,MAX_SICL_BUFFER);
#else
                rpcClient = clnt_create_timed(serverName, DEVICE_CORE, DEVICE_CORE_VERSION, "tcp", &timeout);
/*HK 2004/10/27 replaced clnttcp_create by (newer) clnt_create_timed, which supports a timeout for opening the connection
*/
#endif
                if (rpcClient == NULL)                  // If no connection was built up
                {
                    *errornr=I_ERR_BADADDR;
                    pthread_mutex_unlock( &siclMutex );
                    return (INST) 0;
                }
                
                /* hk>2004/10/05*/
                strcpy(controller[ctrl].name, serverName);   // save the new created RPC Conn in the table for the next lookup
                controller[ctrl].ctrlid = rpcClient;
                controller[ctrl].rpc_socket = rpc_sock;
                if (ctrl >= controllerCount)
                {
                  controllerCount = ctrl +1;
                }
                /* hk<2004/10/05*/
        }
            
        // Now we've found a controller or created a new Controller, lets look for a link to that device, or else create one
            
        for (dev=0; dev<MAX_DEVICES; dev++)
                if ( (res = strcmp(device[ctrl][dev].name, deviceName) ) == 0)
                {
                        *errornr = I_ERR_CONNEXISTS;  // a link to the device already exists, we dont create a new one
                        pthread_mutex_unlock( &siclMutex );
                        return (INST) 0;
                }

        // else: create a new link

        dev=0;
        while ((device[ctrl][dev].linkid > 0) && dev <= MAX_DEVICES) // find a free place in the table
                dev++;
        if(dev>=MAX_DEVICES)    // Table is full: maximum devices are connected to the controller
        {
                *errornr = I_ERR_NOMOREDEV;
                pthread_mutex_unlock( &siclMutex );
                return (INST) 0;
        }

        // fill in the parameters for the connection request to the device
        crlp.clientId = (long) controller[ctrl].ctrlid;  // the cotroller id
        crlp.lockDevice = 0;   // shall the device get locked?
        crlp.lock_timeout = 1000;
        crlp.device = strdup (deviceName);      // device name

        ret = create_link_1(&crlp, controller[ctrl].ctrlid, &crlr);  // create the link

        free(crlp.device);

        //fprintf(stderr, "iopen(): create_link_1 returns '%d', and sets error to '%d'\n",ret,crlr.error); /*hk:2004/10/06*/

        if(ret != 0)
        {
                switch(crlr.error)
                {
                        case 0: *errornr = I_ERR_NOERROR; break;

                        case 1: *errornr = I_ERR_INTERNAL; break;

                        case 3: *errornr = I_ERR_NODEV; break;

                        case 9: *errornr = I_ERR_NORSRC; break;

                        case 11: *errornr = I_ERR_LOCKED; break;

                        case 21: *errornr = I_ERR_INVLADDR; break;

                        default: *errornr = I_ERR_UNKNOWNERR;
                }
                vxierrno = crlr.error;
        }
        else
                *errornr = I_ERR_BADADDR;

        if(*errornr != I_ERR_NOERROR)      // has an error occured?
        {
                res=0;
                for(dev=0;dev<MAX_DEVICES;dev++)        // search for other devices on this controlker
                        if(device[ctrl][dev].linkid != -1) res=1;
                if(!res)
                {                                       // if there are no others: destroy the conn to the controller
                        clnt_destroy(controller[ctrl].ctrlid);
                        controller[ctrl].ctrlid = NULL;
                        controller[ctrl].abortid = NULL;
                        strcpy(controller[ctrl].name, "");
/* hk>2004/10/05*/
                        if (ctrl == controllerCount -1)
/* hk<2004/10/05*/
                            controllerCount--;
                }
                pthread_mutex_unlock( &siclMutex );
                return (INST) 0;
        }

        // else save the device link in the table
        device[ctrl][dev].linkid = crlr.lid;
        device[ctrl][dev].inst = (ctrl*MAX_DEVICES) + dev +1;
        strcpy (device[ctrl][dev].name, deviceName);
        device[ctrl][dev].io_timeout = 1000;
        device[ctrl][dev].lock_timeout = 1000;
        device[ctrl][dev].ctrl = 1;

        err = igpibbusstatus(device[ctrl][dev].inst, 2, &resp);         // check if its a controller

        if(err == 0)
                device[ctrl][dev].ctrl = 1;
        else
                device[ctrl][dev].ctrl = 0;

        /* Create a RPC Connection for the abort channel
         * Uncomment the following lines if you found the correct value vor DEVICE_ASYNC
         * also uncomment the iabort code
         */
        /*      rpcClient = NULL;

                // rpcClient = clnt_create(controller[ctrl].name, DEVICE_ASYNC, DEVICE_ASYNC_VERSION, "tcp");
                struct sockaddr_in server_addr;
                struct hostent *hp;
                int rpc_sock;
                if ((hp = gethostbyname(controller[ctrl].name)) == NULL) {
                    fprintf(stderr, "iopen(): can't get abort channel addr for '%s'\n",controller[ctrl].name);
                    *errornr=I_ERR_BADADDR;
        pthread_mutex_unlock( &siclMutex );
                    return (INST) 0;
                }
                     
                bcopy(hp->h_addr, (caddr_t)&server_addr.sin_addr,hp->h_length);
                server_addr.sin_family = AF_INET;
                server_addr.sin_port =  0;
                rpcClient = clnttcp_create(&server_addr, DEVICE_ASYNC, DEVICE_ASYNC_VERSION,
                                           &rpc_sock,MAX_SICL_BUFFER,MAX_SICL_BUFFER)
 

               if (rpcClient == NULL)
                 *errornr = I_ERR_ABORTCHCR;
               else
                 controller[ctrl].abortid = rpcClient;
          */
        *errornr = I_ERR_NOERROR;
        global_ierrno = ierrno[device[ctrl][dev].inst] = I_ERR_NOERROR;        
        pthread_mutex_unlock( &siclMutex );
        return device[ctrl][dev].inst;
}

// =============================================================================
//
//  Function Name:  iclose
//
//  Description:    Close device
//
// -----------------------------------------------------------------------------
int SICLAPI iclose(INST id)
{
        int             ctrl = ((id-1) / MAX_DEVICES),  // calculate the controllernumber
                        dev = ((id-1) % MAX_DEVICES);   // calculate the devicenumber
        char            founddev = 0;
        char            strtime[20], log[LOG_SIZE];
        FILE            *tracefile;
        int             ret;
        Device_Error    derr;
        
        pthread_mutex_lock( &siclMutex );


        if( ctrl<0 || ctrl>=MAX_CONTROLLERS )
        {
                pthread_mutex_unlock( &siclMutex );
                return global_ierrno = ierrno[id] = I_ERR_NODEV;
        }

        if( dev<0 || dev>=MAX_DEVICES)
        {
                pthread_mutex_unlock( &siclMutex );
                return global_ierrno = ierrno[id] = I_ERR_NODEV;
        }
        
        if((device[ctrl][dev].inst != id) || (id == 0))
        {
                pthread_mutex_unlock( &siclMutex );
                return global_ierrno = ierrno[id] = I_ERR_BADID;
        }

        tracefile = device[ctrl][dev].tracefile;   // save the tracefile before the link is destroyed

        ret = destroy_link_1(&device[ctrl][dev].linkid, controller[ctrl].ctrlid, &derr);

        //fprintf(stderr, "iclose(): destroy_link_1 returns '%d', and sets error to '%d'\n",ret,derr.error); /*hk:2004/10/06*/

            if (ret != 0)       // Error checking
        {
               switch(derr.error)
                {
                        case 0: global_ierrno = ierrno[id] = I_ERR_NOERROR; break;

                        case 4: global_ierrno = ierrno[id] = I_ERR_INTERNAL; break; /*hk:2004/10/06*/

                        default: global_ierrno = ierrno[id] = I_ERR_UNKNOWNERR; break; /*hk:2004/10/06*/
                }
                vxierrno = derr.error;  // save the original vxi errorcode (small differences to sicl)
                // printf("sicl iclose(): %d\n", derr.error);       
        }
        else
                global_ierrno = ierrno[id] = I_ERR_NODEV;

        if(tracefile != 0)                 // write to the lcgfile
        {
                datetoString(time(NULL), strtime);

                if(global_ierrno = ierrno[id] == I_ERR_NOERROR)
                        sprintf(log, "\n\n%s:[%s(inst:%d)]: SESSION CLOSED",
                                strtime,
                                device[ctrl][dev].name,
                                device[ctrl][dev].inst
                                );
                else
                        sprintf(log, "\n\n%s:[%s(inst:%d)]: close: \nError: %d (vxierror: %d), %s\n",
                                strtime,
                                device[ctrl][dev].name,
                                device[ctrl][dev].inst,
                                ierrno[id],
                                vxierrno,
                                igeterrstr(ierrno[id])
                                );

                fprintf(tracefile, "%s\n", log);
                fflush(tracefile);
        }
        
        device[ctrl][dev].inst = 0;             // clear the table entry of the device
        strcpy(device[ctrl][dev].name,"");
        device[ctrl][dev].linkid = -1;
        device[ctrl][dev].ctrl = 0;
        tracefile = device[ctrl][dev].tracefile;
        device[ctrl][dev].tracefile = NULL;
        device[ctrl][dev].termChar = 0;
        device[ctrl][dev].io_timeout = 0;
        device[ctrl][dev].lock_timeout = 0;
/* hk>2004/10/05*/
        device[ctrl][dev].flags = 0;
/* hk<2004/10/05*/

        for(dev=0; dev < MAX_DEVICES; dev++)    // search for other devices on this controlker
                if(device[ctrl][dev].inst != 0) founddev=1;

        if(!founddev)           // if not: destroy the controller
        {
                printf("sicl iclose(): destroying controller %s\n",controller[ctrl].name);
                
                if(tracefile != NULL)
                {
                        sprintf(log,"\nNo more devices on controller %s. Connection closed", controller[ctrl].name);
                        fprintf(tracefile, "%s\n\n", log);
                        fflush(tracefile);
                }
                clnt_destroy(controller[ctrl].ctrlid);
                strcpy(controller[ctrl].name, "");
                controller[ctrl].ctrlid = 0;
/* hk>2004/10/05*/
                controller[ctrl].abortid = 0;

                if (ctrl == controllerCount -1)
                     controllerCount--;
/* hk<2004/10/05*/
        }

        pthread_mutex_unlock( &siclMutex );
        return global_ierrno = ierrno[id] = I_ERR_NOERROR;
}

// =============================================================================
//
//  Function Name:  igetintfsess
//
//  Description:    
//
// -----------------------------------------------------------------------------
INST SICLAPI igetintfsess(INST id)
{
        int             ctrl = ((id-1) /MAX_DEVICES),   // calculate the controller number
                        dev = ((id-1) % MAX_DEVICES),   // calculate the device number
                        c=0,
                        errn = 0;
        char            name[NAME_LEN], ctrlName[NAME_LEN];

        if( ctrl<0 || ctrl>=MAX_CONTROLLERS )   // Was the id valid?
                return global_ierrno = ierrno[id] = I_ERR_NODEV;

        if( dev<0 || dev>=MAX_DEVICES)
                return global_ierrno = ierrno[id] = I_ERR_NODEV;

        if (device[ctrl][dev].inst != id)
                return global_ierrno = ierrno[id] = I_ERR_BADID;

        for(c=0; c<strlen(device[ctrl][dev].name); c++)   // copy until ',' (hpib,12 -> hpib)
                if(device[ctrl][dev].name[c] == ',')
                        break;
                else
                        name[c]=device[ctrl][dev].name[c];

        name[c] = 0;        // were at the end of the name

        sprintf(ctrlName, "lan[%s]:%s", controller[ctrl].name, name); // build a correct string for iopen
        
        
        /*pal*/

        return iopen_err(ctrlName, &errn);         // call iopen to create the connection
}

// =============================================================================
//
//  Function Name:  iwrite
//
//  Description:    Write to device
//
// -----------------------------------------------------------------------------
int SICLAPI iwrite (
        INST id,                        // The id of the device
        const char _far *buf,           // the data
        unsigned long datalen,          // length of the data
        int end,                        // end indicator
        unsigned long _far *actualcnt   // the amount of bytes really transmitted
        )
{
        int                     ctrl = ((id-1) / MAX_DEVICES),  // calculate the controller number
                                dev = ((id-1) % MAX_DEVICES);   // calculate the device number
        Device_WriteParms       dwrp;
        Device_WriteResp        dwrr;
        int                     ret;
        char                    strtime[20];
        char                    log[LOG_SIZE];

        vxierrno = 0;

        if( ctrl<0 || ctrl>=MAX_CONTROLLERS )   // Was the id valid?
                return global_ierrno = ierrno[id] = I_ERR_NODEV;

        if( dev<0 || dev>=MAX_DEVICES)
                return global_ierrno = ierrno[id] = I_ERR_NODEV;

        if (device[ctrl][dev].inst != id)
                return global_ierrno = ierrno[id] = I_ERR_BADID;

        // fill in the write parameters
        dwrp.lid = device[ctrl][dev].linkid;
        dwrp.io_timeout = device[ctrl][dev].io_timeout;
        dwrp.lock_timeout = device[ctrl][dev].lock_timeout;
        dwrp.flags = 0;
        if(end)        // if a end indicator was spezified set the flag to use it
                dwrp.flags |= 8;
        dwrp.data.data_len = datalen;
        // dwrp.data.data_val = strdup(buf);
        // use buffer directly without copying
        dwrp.data.data_val = (char*)buf;

        ret = device_write_1 (&dwrp, controller[ctrl].ctrlid, &dwrr); // write to the device

        //fprintf(stderr, "iwrite(): device_write_1 returns '%d', and sets error to '%d'\n",ret,dwrr.error); /*hk:2004/10/06*/
        if(ret != 0)        // Error detection
                switch(dwrr.error)
                {
                        case 0: global_ierrno = ierrno[id] = I_ERR_NOERROR; break;

                        case 3: global_ierrno = ierrno[id] = I_ERR_NODEV; break;

                        case 4: global_ierrno = ierrno[id] = I_ERR_INTERNAL; break;

                        case 5: global_ierrno = ierrno[id] = I_ERR_INTERNAL; break;

                        case 11: global_ierrno = ierrno[id] = I_ERR_LOCKED; break;

                        case 15: global_ierrno = ierrno[id] = I_ERR_TIMEOUT; break;

                        case 17: global_ierrno = ierrno[id] = I_ERR_IO; break;

                        case 23: global_ierrno = ierrno[id] = I_ERR_ABORTED; break;

                        default: global_ierrno = ierrno[id] = I_ERR_UNKNOWNERR; break;
                }

        else
                global_ierrno = ierrno[id] = I_ERR_NODEV;

        if(device[ctrl][dev].tracefile != 0)    // traceing
        {
                datetoString(time(NULL), strtime);

                sprintf(log, "%s:[%s(inst:%d)]: wrote",
                        strtime,
                        device[ctrl][dev].name,
                        device[ctrl][dev].inst
                        );

                if(ret != 0)
                        fprintf(device[ctrl][dev].tracefile,
                                "\n%s: (length: %d) %sError: %d (vxierror: %d), %s",
                                log,
                                dwrp.data.data_len,
                                dwrp.data.data_val,
                                ierrno[id],
                                vxierrno,
                                igeterrstr(ierrno[id])
                                );
                else
                        fprintf(device[ctrl][dev].tracefile,
                                "\n%s:\n Error: %d (vxierror: %d), %s",
                                log,
                                ierrno[id],
                                vxierrno,
                                igeterrstr(ierrno[id])
                                );
                fflush(device[ctrl][dev].tracefile);
        }

        if((actualcnt != NULL) && ret != 0)    // save te real sent size in actualcnt
                *actualcnt = dwrr.size;

        // use buffer directly without copying
        // free(dwrp.data.data_val);

        return ierrno[id];
}

// =============================================================================
//
//  Function Name:  iread
//
//  Description:    Read from device
//
// -----------------------------------------------------------------------------
int SICLAPI iread (
        INST id,                // id of the device
        char _far *buf,         // returnbuffer
        unsigned long bufsize,  // size of returnbuffer
        int _far *reason,       // the reason why the end of data is reached
        unsigned long _far *actualcnt // the amount of data read
        )
{
        int                     ctrl = ((id-1) /MAX_DEVICES),   // calculate the controller number
                                dev = ((id-1) % MAX_DEVICES);   // calculate the device number
        char                    strtime[20], log[LOG_SIZE];
        unsigned long           data_len;
        Device_ReadParms        drdp;
        Device_ReadResp         drdr;
        int                     ret;

        vxierrno = 0;

        if( ctrl<0 || ctrl>=MAX_CONTROLLERS )   // is the id valid?
                return global_ierrno = ierrno[id] = I_ERR_NODEV;

        if( dev<0 || dev>=MAX_DEVICES)
                return global_ierrno = ierrno[id] = I_ERR_NODEV;

        if (device[ctrl][dev].inst != id)
                return global_ierrno = ierrno[id] = I_ERR_BADID;

        // fill in the read parameters
        drdp.lid = device[ctrl][dev].linkid;
        drdp.io_timeout = device[ctrl][dev].io_timeout;
        drdp.lock_timeout = device[ctrl][dev].lock_timeout;
        drdp.flags = device[ctrl][dev].flags;
        drdp.requestSize = bufsize;
        drdp.termChar = device[ctrl][dev].termChar;

        ret = device_read_1 (&drdp, controller[ctrl].ctrlid, &drdr);  // read from the device

        //fprintf(stderr, "iread(): device_read_1 returns '%d', and sets error to '%d'\n",ret,drdr.error); /*hk:2004/10/06*/
        if(ret != 0)                // error check
        {
                vxierrno = drdr.error;
                switch(drdr.error)
                {
                        case 0: global_ierrno = ierrno[id] = I_ERR_NOERROR; break;
                        case 3: global_ierrno = ierrno[id] = I_ERR_NODEV; break;
                        case 4: global_ierrno = ierrno[id] = I_ERR_INTERNAL; break;
                        case 5: global_ierrno = ierrno[id] = I_ERR_INTERNAL; break;
                        case 11: global_ierrno = ierrno[id] = I_ERR_LOCKED; break;
                        case 15: global_ierrno = ierrno[id] = I_ERR_TIMEOUT; break;
                        case 17: global_ierrno = ierrno[id] = I_ERR_IO; break;
                        case 23: global_ierrno = ierrno[id] = I_ERR_ABORTED; break;

                        default: global_ierrno = ierrno[id] = I_ERR_UNKNOWNERR; break;
               }
        }

        else
                global_ierrno = ierrno[id] = I_ERR_NODEV;

        if(device[ctrl][dev].tracefile != 0)
        {
                datetoString(time(NULL), strtime);

                sprintf(log, "%s:[%s(inst:%d)]: read",
                        strtime,
                        device[ctrl][dev].name,
                        device[ctrl][dev].inst
                        );

                fflush(NULL);
                if(ret == 0)
                        fprintf(device[ctrl][dev].tracefile,
                                "\n%s: \n Error: %d (vxierror: %d), %s",
                                log,
                                ierrno[id],
                                vxierrno,
                                igeterrstr(ierrno[id])
                                );
                else
                        fprintf(device[ctrl][dev].tracefile,
                                "\n%s: (length: %d) %sError: %d (vxierror: %d), %s",
                                log,
                                drdr.data.data_len,
                                (drdr.data.data_val ? drdr.data.data_val : ""),
                                ierrno[id],
                                vxierrno,
                                igeterrstr(ierrno[id])
                                );

                fflush(device[ctrl][dev].tracefile);
        }


        if (ierrno[id])             // on error -> exit
                return ierrno[id];

        if (ret != 0)
        {
                data_len=drdr.data.data_len;
                if(reason != NULL)      // give the reason to the questioner
                        *reason = drdr.reason;
        }
        else
                data_len=0;

        if (ret != 0 && drdr.data.data_val) 
        {
          memcpy(buf, drdr.data.data_val, (data_len < bufsize) ? data_len : bufsize);   // fill the data in the buffer

        // free the data buffer, it was allocated in 
        // xdr_Device_ReadResp/xdr_bytes, file vxi11_core.c
          free(drdr.data.data_val);
          drdr.data.data_len = 0;
          drdr.data.data_val = NULL;
          if(actualcnt != NULL)   // save the number of received bytes in actualcnt
                *actualcnt = data_len;
        }
        else
        {
          buf[0] = '\0';
          if(actualcnt != NULL)   // save the number of received bytes in actualcnt
                *actualcnt = 0;
        }
        return ierrno[id];
}

// =============================================================================
//
//  Function Name:  itermchr
//
//  Description:    Set termination char
//
// -----------------------------------------------------------------------------
int SICLAPI itermchr(INST id, int tchr)
{
        int     ctrl = ((id -1) / MAX_DEVICES),         // calculate the controller number
                dev = ((id -1) % MAX_DEVICES);          // calculate the device number

        if( ctrl<0 || ctrl>=MAX_CONTROLLERS )   // is the id valid?
                return global_ierrno = ierrno[id] = I_ERR_NODEV;

        if( dev<0 || dev>=MAX_DEVICES)
                return global_ierrno = ierrno[id] = I_ERR_NODEV;

        if (device[ctrl][dev].inst != id)
                return global_ierrno = ierrno[id] = I_ERR_BADID;

        device[ctrl][dev].termChar = tchr;

        if (tchr != -1) {
            // enable the use of the termination character
            device[ctrl][dev].flags |= 0x80;
        } else {
            // disable the use of the termination character
            device[ctrl][dev].flags &= 0x7F;
        }

        return global_ierrno = ierrno[id] = I_ERR_NOERROR;
}

// =============================================================================
//
//  Function Name:  igettermchr
//
//  Description:    Get term char
//
// -----------------------------------------------------------------------------
int SICLAPI igettermchr(INST id, int _far *tchr)
{
        int     ctrl = ((id -1) / MAX_DEVICES),         // calculate the controller number
                dev = ((id -1) % MAX_DEVICES);          // calculate the device number

        if( ctrl<0 || ctrl>=MAX_CONTROLLERS )   // is the id valid?
                return global_ierrno = ierrno[id] = I_ERR_NODEV;

        if( dev<0 || dev>=MAX_DEVICES)
                return global_ierrno = ierrno[id] = I_ERR_NODEV;

        if (device[ctrl][dev].inst != id)
                return global_ierrno = ierrno[id] = I_ERR_BADID;

        tchr = &(device[ctrl][dev].termChar);   // write the term char into tchr

        return global_ierrno = ierrno[id] = I_ERR_NOERROR;
}

// =============================================================================
//
//  Function Name:  iclear
//
//  Description:    Send a clear device
//
// -----------------------------------------------------------------------------
int SICLAPI iclear( INST id)
{
        int                     ctrl = ((id -1) / MAX_DEVICES),         // calculate the controller number
                                dev = ((id -1) % MAX_DEVICES);          // calculate the device number
        char                    strtime[20], log [LOG_SIZE];
        int                     ret;
        Device_Error            derr;
        Device_GenericParms     dgp;

        if( ctrl<0 || ctrl>=MAX_CONTROLLERS )  // is the id valid?
                return global_ierrno = ierrno[id] = I_ERR_NODEV;

        if( dev<0 || dev>=MAX_DEVICES)
                return global_ierrno = ierrno[id] = I_ERR_NODEV;

        if (device[ctrl][dev].inst != id)
                return global_ierrno = ierrno[id] = I_ERR_BADID;

        vxierrno = 0;


        // fill in the device parameters
        dgp.lid = device[ctrl][dev].linkid;
        dgp.flags = device[ctrl][dev].flags;
        dgp.io_timeout = device[ctrl][dev].io_timeout;
        dgp.lock_timeout = device[ctrl][dev].lock_timeout;

        ret = device_clear_1(&dgp, controller[ctrl].ctrlid, &derr);   // send the clear command

        //fprintf(stderr, "iclear(): device_clear_1 returns '%d', and sets error to '%d'\n",ret,derr.error); /*hk:2004/10/06*/

        if(ret != 0)                // error check
        {
               switch(derr.error)
                {
                        case 0: global_ierrno = ierrno[id] = I_ERR_NOERROR; break;

                        case 4: global_ierrno = ierrno[id] = I_ERR_INTERNAL; break;

                        case 6: global_ierrno = ierrno[id] = I_ERR_INTERNAL; break;

                        case 8: global_ierrno = ierrno[id] = I_ERR_NOTSUPP; break;

                        case 11: global_ierrno = ierrno[id] = I_ERR_LOCKED; break;

                        case 15: global_ierrno = ierrno[id] = I_ERR_TIMEOUT; break;

                        case 17: global_ierrno = ierrno[id] = I_ERR_IO; break;

                        case 23: global_ierrno = ierrno[id] = I_ERR_ABORTED; break;

                        default: global_ierrno = ierrno[id] = I_ERR_UNKNOWNERR; break;
                }
                vxierrno = derr.error;       // save the original vxi errorcode
        }
        else
                global_ierrno = ierrno[id] = I_ERR_NODEV;

        if(device[ctrl][dev].tracefile != 0)    // trace
        {
                datetoString(time(NULL), strtime);

                sprintf(log, "%s:[%s(inst:%d)]: clear",
                        strtime,
                        device[ctrl][dev].name,
                        device[ctrl][dev].inst
                        );

                fprintf(device[ctrl][dev].tracefile,
                        "\n%s Error: %d (vxierror: %d), %s",
                        log, ierrno[id],
                        vxierrno,
                        igeterrstr(ierrno[id])
                        );
                fflush(device[ctrl][dev].tracefile);
        }

        return ierrno[id];
}

// =============================================================================
//
//  Function Name:  iabort
//
//  Description:    
//
// -----------------------------------------------------------------------------
int SICLAPI iabort(INST id)
{
        int                     ctrl = ((id -1) / MAX_DEVICES), // calculate the controller number
                                dev = ((id -1) % MAX_DEVICES);  // calculate the device number
        char                    strtime[20], log[LOG_SIZE];
/*
        int                     ret
        Device_Error            derr;
        Device_Link             dlid;

        vxierrno = 0;
        global_ierrno = ierrno[id] = 0;

        if( ctrl<0 || ctrl>=MAX_CONTROLLERS )   // is the id valid?
                return global_ierrno = ierrno[id] = I_ERR_NODEV;

        if( dev<0 || dev>=MAX_DEVICES)
                return global_ierrno = ierrno[id] = I_ERR_NODEV;

        if (device[ctrl][dev].inst != id)
                return global_ierrno = ierrno[id] = I_ERR_BADID;

        dlid = device[ctrl][dev].linkid;        // give the device_link as parameter

        if(controller[ctrl].abortid)
                ret = device_abort_1(&dlid, controller[ctrl].abortid, &derr);  // call the vxi11 abort function
        else
                global_ierrno = ierrno[id] = I_ERR_NOABORTCH;

        if(ret != 0)                        // error check
        {
                switch(derr.error)
                {
                        case 0: global_ierrno = ierrno[id] = I_ERR_INTERNAL; break;

                        case 4: global_ierrno = ierrno[id] = I_ERR_INTERNAL; break;

                        case 6: global_ierrno = ierrno[id] = I_ERR_INTERNAL; break;

                }
                vxierrno = derr.error;
        }
        else if(!ierrno[id])
                global_ierrno = ierrno[id] = I_ERR_NODEV;

        if(device[ctrl][dev].tracefile != 0)
        {
                datetoString(time(NULL), strtime);

                sprintf(log, "%s:[%s(inst:%d)]: abort",
                        strtime,
                        device[ctrl][dev].name,
                        device[ctrl][dev].inst
                        );

                fprintf(device[ctrl][dev].tracefile,
                        "\n%s Error: %d (vxierror: %d), %s",
                        log,
                        ierrno[id],
                        vxierrno,
                        igeterrstr(ierrno[id])
                        );
                fflush(device[ctrl][dev].tracefile);
        }
*/

        if(device[ctrl][dev].tracefile != 0)
        {
                datetoString(time(NULL), strtime);

                sprintf(log, "%s:[%s(inst:%d)]: abort",
                        strtime,
                        device[ctrl][dev].name,
                        device[ctrl][dev].inst
                        );

                fprintf(device[ctrl][dev].tracefile,
                        "\n%s Error: abort is not yet supported!",
                        log
                        );
                fflush(device[ctrl][dev].tracefile);
        }
        return global_ierrno = ierrno[id] = I_ERR_NOERROR;
}

// =============================================================================
//
//  Function Name:  ilocal
//
//  Description:    
//
// -----------------------------------------------------------------------------
int SICLAPI ilocal(INST id)
{
        int                     ctrl = ((id -1) / MAX_DEVICES), // calculate the controller id
                                dev = ((id -1) % MAX_DEVICES);  // calculate the device id
        char                    strtime[20], log[LOG_SIZE];
        int                     ret;
        Device_Error            derr;
        Device_GenericParms     dgp;

        vxierrno = 0;

        if( ctrl<0 || ctrl>=MAX_CONTROLLERS )   // is the id valid?
                return global_ierrno = ierrno[id] = I_ERR_NODEV;

        if( dev<0 || dev>=MAX_DEVICES)
                return global_ierrno = ierrno[id] = I_ERR_NODEV;

        if (device[ctrl][dev].inst != id)
                return global_ierrno = ierrno[id] = I_ERR_BADID;

        // fill in the parameters
        dgp.lid = device[ctrl][dev].linkid;
        dgp.flags = device[ctrl][dev].flags;
        dgp.io_timeout = device[ctrl][dev].io_timeout;
        dgp.lock_timeout = device[ctrl][dev].lock_timeout;

        ret = device_local_1(&dgp, controller[ctrl].ctrlid, &derr); // call the vxi11 func

        if(ret != 0)                        // error check
        {
                switch(derr.error)
                {
                        case 0: global_ierrno = ierrno[id] = I_ERR_NOERROR; break;

                        case 4: global_ierrno = ierrno[id] = I_ERR_INTERNAL; break;

                        case 6: global_ierrno = ierrno[id] = I_ERR_INTERNAL; break;

                        case 8: global_ierrno = ierrno[id] = I_ERR_NOTSUPP; break;

                        case 11: global_ierrno = ierrno[id] = I_ERR_LOCKED; break;

                        case 15: global_ierrno = ierrno[id] = I_ERR_TIMEOUT; break;

                        case 17: global_ierrno = ierrno[id] = I_ERR_IO; break;

                        case 23: global_ierrno = ierrno[id] = I_ERR_ABORTED; break;

                        default: global_ierrno = ierrno[id] = I_ERR_UNKNOWNERR; break;
                }
                vxierrno = derr.error;         // save the original vxi errorcode
        }
        else
                global_ierrno = ierrno[id] = I_ERR_NODEV;

        if(device[ctrl][dev].tracefile != 0)
        {
                datetoString(time(NULL), strtime);

                sprintf(log, "%s:[%s(inst:%d)]: local",
                        strtime,
                        device[ctrl][dev].name,
                        device[ctrl][dev].inst
                        );

                fprintf(device[ctrl][dev].tracefile,
                        "\n%s Error: %d, %s",
                        log,
                        ierrno[id],
                        igeterrstr(ierrno[id])
                        );
                fflush(device[ctrl][dev].tracefile);
        }

        return ierrno[id];
}

// =============================================================================
//
//  Function Name:  iremote
//
//  Description:    
//
// -----------------------------------------------------------------------------
int SICLAPI iremote(INST id)
{
        int                     ctrl = ((id -1) / MAX_DEVICES), // calculate the controller number
                                dev = ((id -1) % MAX_DEVICES);  // calculate the device number
        char                    strtime[20], log[LOG_SIZE];
        int                     ret;
        Device_Error            derr;
        Device_GenericParms     dgp;

        vxierrno = 0;

        if( ctrl<0 || ctrl>=MAX_CONTROLLERS )   // is the id valid?
                return global_ierrno = ierrno[id] = I_ERR_NODEV;

        if( dev<0 || dev>=MAX_DEVICES)
                return global_ierrno = ierrno[id] = I_ERR_NODEV;

        if (device[ctrl][dev].inst != id)
                return global_ierrno = ierrno[id] = I_ERR_BADID;

        // fill in the parameters
        dgp.lid = device[ctrl][dev].linkid;
        dgp.flags = device[ctrl][dev].flags;
        dgp.io_timeout = device[ctrl][dev].io_timeout;
        dgp.lock_timeout = device[ctrl][dev].lock_timeout;

        // call the vxi11 function
        ret = device_remote_1(&dgp, controller[ctrl].ctrlid, &derr);

        if(ret != 0)                        // error check
        {
                switch(derr.error)
                {
                        case 0: global_ierrno = ierrno[id] = I_ERR_NOERROR; break;

                        case 4: global_ierrno = ierrno[id] = I_ERR_INTERNAL; break;

                        case 6: global_ierrno = ierrno[id] = I_ERR_INTERNAL; break;

                        case 8: global_ierrno = ierrno[id] = I_ERR_NOTSUPP; break;

                        case 11: global_ierrno = ierrno[id] = I_ERR_LOCKED; break;

                        case 15: global_ierrno = ierrno[id] = I_ERR_TIMEOUT; break;

                        case 17: global_ierrno = ierrno[id] = I_ERR_IO; break;

                        case 23: global_ierrno = ierrno[id] = I_ERR_ABORTED; break;

                        default: global_ierrno = ierrno[id] = I_ERR_UNKNOWNERR; break;
                }
                vxierrno = derr.error;
        }
        else
                global_ierrno = ierrno[id] = I_ERR_NODEV;

        if(device[ctrl][dev].tracefile != 0)    // trace
        {
                datetoString(time(NULL), strtime);

                sprintf(log, "%s:[%s(inst:%d)]: remote",
                        strtime,
                        device[ctrl][dev].name,
                        device[ctrl][dev].inst
                        );

                fprintf(device[ctrl][dev].tracefile,
                        "\n%s Error: %d (vxierror: %d), %s",
                        log,
                        ierrno[id],
                        vxierrno,
                        igeterrstr(ierrno[id])
                        );
                fflush(device[ctrl][dev].tracefile);
        }

        return ierrno[id];
}

// =============================================================================
//
//  Function Name:  ireadstb
//
//  Description:    
//
// -----------------------------------------------------------------------------
int SICLAPI ireadstb(INST id, unsigned char _far *stb)
{
        int                     ctrl = ((id -1) / MAX_DEVICES), // calculate the controller number
                                dev = ((id -1) % MAX_DEVICES);  // calculate the device number
        char                    strtime[20], log[LOG_SIZE];
        Device_GenericParms     dgp;
        Device_ReadStbResp      drstbr;
        int                     ret;

        vxierrno = 0;

        if( ctrl<0 || ctrl>=MAX_CONTROLLERS )   // is the id valid?
                return global_ierrno = ierrno[id] = I_ERR_NODEV;

        if( dev<0 || dev>=MAX_DEVICES)
                return global_ierrno = ierrno[id] = I_ERR_NODEV;

        if (device[ctrl][dev].inst != id)
                return global_ierrno = ierrno[id] = I_ERR_BADID;

        // fill in the parameters
        dgp.lid = device[ctrl][dev].linkid;
        dgp.flags = device[ctrl][dev].flags;
        dgp.io_timeout = device[ctrl][dev].io_timeout;
        dgp.lock_timeout = device[ctrl][dev].lock_timeout;

        ret = device_readstb_1(&dgp, controller[ctrl].ctrlid, &drstbr);       // call the vxi11 function

        //fprintf(stderr, "ireadstb(): device_readstb_1 returns '%d', and sets error to '%d'\n",ret,drstbr.error); /*hk:2004/10/06*/

        if(ret != 0)                      // error check
        {
                switch(drstbr.error)
                {
                        case 0: global_ierrno = ierrno[id] = I_ERR_NOERROR; break;

                        case 4: global_ierrno = ierrno[id] = I_ERR_INTERNAL; break;

                        case 6: global_ierrno = ierrno[id] = I_ERR_INTERNAL; break;

                        case 8: global_ierrno = ierrno[id] = I_ERR_NOTSUPP; break;

                        case 11: global_ierrno = ierrno[id] = I_ERR_LOCKED; break;

                        case 15: global_ierrno = ierrno[id] = I_ERR_TIMEOUT; break;

                        case 17: global_ierrno = ierrno[id] = I_ERR_IO; break;

                        case 23: global_ierrno = ierrno[id] = I_ERR_ABORTED; break;

                        default: global_ierrno = ierrno[id] = I_ERR_UNKNOWNERR; break;
                }
                vxierrno = drstbr.error;
        }
        else
                global_ierrno = ierrno[id] = I_ERR_NODEV;

        if(device[ctrl][dev].tracefile != 0)    // trace
        {
                datetoString(time(NULL), strtime);

                sprintf(log, "%s:[%s(inst:%d)]: ireadstb: %d",
                        strtime,
                        device[ctrl][dev].name,
                        device[ctrl][dev].inst,
                        drstbr.stb
                        );

                if(ret != 0)
                        fprintf(device[ctrl][dev].tracefile,
                                "\n%s Statusbyte: %d \n Error: %d (vxierror: %d), %s",
                                log,
                                drstbr.stb,
                                ierrno[id],
                                vxierrno,
                                igeterrstr(ierrno[id])
                                );
                else
                        fprintf(device[ctrl][dev].tracefile,
                                "\n%s Error, %d, %s",
                                log,
                                ierrno[id],
                                igeterrstr(ierrno[id])
                                );

                fflush(device[ctrl][dev].tracefile);
        }

        if(ierrno[id]==0)   // if no error occured save the stb
                *stb = drstbr.stb;
        else
                stb = NULL; // else set the pointer to NULL

        return ierrno[id];
}

// =============================================================================
//
//  Function Name:  itrigger
//
//  Description:    
//
// -----------------------------------------------------------------------------
int SICLAPI itrigger(INST id)
{
        int                     ctrl = ((id -1) / MAX_DEVICES),         // calculate the controller number
                                dev = ((id -1) % MAX_DEVICES);          // calculate the device number
        char                    strtime[20], log[LOG_SIZE];
        int                     ret;
        Device_Error            derr;
        Device_GenericParms     dgp;

        vxierrno = 0;

        if( ctrl<0 || ctrl>=MAX_CONTROLLERS )   // is the id valid?
                return global_ierrno = ierrno[id] = I_ERR_NODEV;

        if( dev<0 || dev>=MAX_DEVICES)
                return global_ierrno = ierrno[id] = I_ERR_NODEV;

        if (device[ctrl][dev].inst != id)
                return global_ierrno = ierrno[id] = I_ERR_BADID;

        // fill in the parameters
        dgp.lid = device[ctrl][dev].linkid;
        dgp.flags = device[ctrl][dev].flags;
        dgp.io_timeout = device[ctrl][dev].io_timeout;
        dgp.lock_timeout = device[ctrl][dev].lock_timeout;

        ret = device_trigger_1(&dgp, controller[ctrl].ctrlid, &derr);         // call the vxi11 function

        if(ret != 0)                        // errorcheck
        {
                switch(derr.error)
                {
                        case 0: global_ierrno = ierrno[id] = I_ERR_NOERROR; break;

                        case 4: global_ierrno = ierrno[id] = I_ERR_INTERNAL; break;

                        case 6: global_ierrno = ierrno[id] = I_ERR_INTERNAL; break;

                        case 8: global_ierrno = ierrno[id] = I_ERR_NOTSUPP; break;

                        case 11: global_ierrno = ierrno[id] = I_ERR_LOCKED; break;

                        case 15: global_ierrno = ierrno[id] = I_ERR_TIMEOUT; break;

                        case 17: global_ierrno = ierrno[id] = I_ERR_IO; break;

                        case 23: global_ierrno = ierrno[id] = I_ERR_ABORTED; break;

                        default: global_ierrno = ierrno[id] = I_ERR_UNKNOWNERR; break;
                }
                vxierrno = derr.error;
        }
        else
                global_ierrno = ierrno[id] = I_ERR_NODEV;

        if(device[ctrl][dev].tracefile != 0)    // trace
        {
                datetoString(time(NULL), strtime);

                sprintf(log, "%s:[%s(inst:%d)]: trigger",
                        strtime,
                        device[ctrl][dev].name,
                        device[ctrl][dev].inst
                        );

                fprintf(device[ctrl][dev].tracefile,
                        "\n%s Error: %d (vxierror: %d), %s",
                        log,
                        ierrno[id],
                        vxierrno,
                        igeterrstr(ierrno[id])
                        );
                fflush(device[ctrl][dev].tracefile);
        }

        return ierrno[id];
}

// =============================================================================
//
//  Function Name:  ilock
//
//  Description:    
//
// -----------------------------------------------------------------------------
int SICLAPI ilock(INST id)
{
        int                     ctrl = ((id -1) / MAX_DEVICES),         // calculate the controller number
                                dev = ((id -1) % MAX_DEVICES);          // calculate the device number
        char                    strtime[20], log[LOG_SIZE];
        int                     ret;
        Device_Error            derr;
        Device_LockParms        dlockp;

        vxierrno = 0;

        if( ctrl<0 || ctrl>=MAX_CONTROLLERS )   // is the id valid?
                return global_ierrno = ierrno[id] = I_ERR_NODEV;

        if( dev<0 || dev>=MAX_DEVICES)
                return global_ierrno = ierrno[id] = I_ERR_NODEV;

        if (device[ctrl][dev].inst != id)
                return global_ierrno = ierrno[id] = I_ERR_BADID;

        // fill in the paramters
        dlockp.lid = device[ctrl][dev].linkid;
        dlockp.flags = device[ctrl][dev].flags;
        dlockp.lock_timeout = device[ctrl][dev].lock_timeout;

        // call the vxi11 function
        ret = device_lock_1(&dlockp, controller[ctrl].ctrlid, &derr);

        if(ret != 0)                        // error check
        {
                switch(derr.error)
                {
                        case 0: global_ierrno = ierrno[id] = I_ERR_NOERROR; break;

                        case 4: global_ierrno = ierrno[id] = I_ERR_INTERNAL; break;

                        case 6: global_ierrno = ierrno[id] = I_ERR_INTERNAL; break;

                        case 8: global_ierrno = ierrno[id] = I_ERR_NOTSUPP; break;

                        case 11: global_ierrno = ierrno[id] = I_ERR_LOCKED; break;

                        case 15: global_ierrno = ierrno[id] = I_ERR_TIMEOUT; break;

                        case 17: global_ierrno = ierrno[id] = I_ERR_IO; break;

                        case 23: global_ierrno = ierrno[id] = I_ERR_ABORTED; break;

                        default: global_ierrno = ierrno[id] = I_ERR_UNKNOWNERR; break;
                }
                vxierrno = derr.error;
        }
        else
                global_ierrno = ierrno[id] = I_ERR_NODEV;

        if(device[ctrl][dev].tracefile != 0)    // trace
        {
                datetoString(time(NULL), strtime);

                sprintf(log, "%s:[%s(inst:%d)]: locked",
                        strtime,
                        device[ctrl][dev].name,
                        device[ctrl][dev].inst
                        );

                fprintf(device[ctrl][dev].tracefile,
                        "\n%s Error: %d (vxierror: %d), %s",
                        log,
                        ierrno[id],
                        vxierrno,
                        igeterrstr(ierrno[id])
                        );
                fflush(device[ctrl][dev].tracefile);
        }

        return ierrno[id];
}

// =============================================================================
//
//  Function Name:  iunlock
//
//  Description:    
//
// -----------------------------------------------------------------------------
int SICLAPI iunlock(INST id)
{
        int                     ctrl = ((id -1) / MAX_DEVICES),         // calculate the controller number
                                dev = ((id -1) % MAX_DEVICES);          // calculate the devcie number
        char                    strtime[20], log[LOG_SIZE];
        int                     ret;
        Device_Error            derr;
        Device_Link             dlid;

        vxierrno = 0;

        if( ctrl<0 || ctrl>=MAX_CONTROLLERS )   // is the id valid?
                return global_ierrno = ierrno[id] = I_ERR_NODEV;

        if( dev<0 || dev>=MAX_DEVICES)
                return global_ierrno = ierrno[id] = I_ERR_NODEV;

        if (device[ctrl][dev].inst != id)
                return global_ierrno = ierrno[id] = I_ERR_BADID;

        dlid = device[ctrl][dev].linkid;        // give the link id as parameter

        ret = device_unlock_1(&dlid, controller[ctrl].ctrlid, &derr);         // call the vxi11 function

        if(ret != 0)                        // error check
        {        switch(derr.error)
                {
                        case 0: global_ierrno = ierrno[id] = I_ERR_NOERROR; break;

                        case 4: global_ierrno = ierrno[id] = I_ERR_INTERNAL; break;

                        case 6: global_ierrno = ierrno[id] = I_ERR_INTERNAL; break;

                        case 8: global_ierrno = ierrno[id] = I_ERR_NOTSUPP; break;

                        case 11: global_ierrno = ierrno[id] = I_ERR_LOCKED; break;

                        case 12: global_ierrno = ierrno[id] = I_ERR_NOLOCK; break;

                        case 15: global_ierrno = ierrno[id] = I_ERR_TIMEOUT; break;

                        case 17: global_ierrno = ierrno[id] = I_ERR_IO; break;

                        case 23: global_ierrno = ierrno[id] = I_ERR_ABORTED; break;

                        default: global_ierrno = ierrno[id] = I_ERR_UNKNOWNERR; break;
                }
                vxierrno = derr.error;
        }
        else
                global_ierrno = ierrno[id] = I_ERR_NODEV;

        if(device[ctrl][dev].tracefile != 0)
        {
                datetoString(time(NULL), strtime);

                sprintf(log, "%s:[%s(inst:%d)]: unlocked",
                        strtime,
                        device[ctrl][dev].name,
                        device[ctrl][dev].inst
                        );

                fprintf(device[ctrl][dev].tracefile,
                        "\n%s Error: %d (vxierror: %d), %s",
                        log,
                        ierrno[id],
                        vxierrno,
                        igeterrstr(ierrno[id])
                        );
                fflush(device[ctrl][dev].tracefile);
        }

        return ierrno[id];
}

// =============================================================================
//
//  Function Name:  isetlockwait
//
//  Description:    
//
// -----------------------------------------------------------------------------
int SICLAPI isetlockwait(INST id, int flag)
{
        int                     ctrl = ((id -1) / MAX_DEVICES),         // calculate the controller number
                                dev = ((id -1) % MAX_DEVICES);          // calculate the device number

        vxierrno = 0;

        if( ctrl<0 || ctrl>=MAX_CONTROLLERS )           // is the id valid?
                return global_ierrno = ierrno[id] = I_ERR_NODEV;

        if( dev<0 || dev>=MAX_DEVICES)
                return global_ierrno = ierrno[id] = I_ERR_NODEV;

        if (device[ctrl][dev].inst != id)
                return global_ierrno = ierrno[id] = I_ERR_BADID;

        if (flag) device[ctrl][dev].flags |= 1;         // if flag is true set the lockwait flag
        else device[ctrl][dev].flags &=254;             // else clear the flag
        return global_ierrno = ierrno[id] = I_ERR_NOERROR;
}

// =============================================================================
//
//  Function Name:  igetlockwait
//
//  Description:    
//
// -----------------------------------------------------------------------------
int SICLAPI igetlockwait(INST id, int _far *flag)
{
        int                     ctrl = ((id -1) / MAX_DEVICES),         // calculate the controller number
                                dev = ((id -1) % MAX_DEVICES);          // calculate the device number

        vxierrno = 0;

        if( ctrl<0 || ctrl>=MAX_CONTROLLERS )           // is the id valid?
                return global_ierrno = ierrno[id] = I_ERR_NODEV;

        if( dev<0 || dev>=MAX_DEVICES)
                return global_ierrno = ierrno[id] = I_ERR_NODEV;

        if (device[ctrl][dev].inst != id)
                return global_ierrno = ierrno[id] = I_ERR_BADID;

        *flag = device[ctrl][dev].flags &= 1;           // return the lockwait flag
        return global_ierrno = ierrno[id] = I_ERR_NOERROR;
}

// =============================================================================
//
//  Function Name:  itimeout
//
//  Description:    
//
// -----------------------------------------------------------------------------
int SICLAPI itimeout(INST id, long tval)
{
        int                     ctrl = ((id -1) / MAX_DEVICES),         // calculate the controller number
                                dev = ((id -1) % MAX_DEVICES);          // calculate the device number

        vxierrno = 0;

        if( ctrl<0 || ctrl>=MAX_CONTROLLERS )           // is the id valid?
                return global_ierrno = ierrno[id] = I_ERR_NODEV;

        if( dev<0 || dev>=MAX_DEVICES)
                return global_ierrno = ierrno[id] = I_ERR_NODEV;

        if (device[ctrl][dev].inst != id)
                return global_ierrno = ierrno[id] = I_ERR_BADID;

        device[ctrl][dev].io_timeout = tval;    // set the timeouts
        device[ctrl][dev].lock_timeout = tval;

        return global_ierrno = ierrno[id] = 0;
}

// =============================================================================
//
//  Function Name:  igettimeout
//
//  Description:    
//
// -----------------------------------------------------------------------------
int SICLAPI igettimeout(INST id, long _far *tval)
{
        int                     ctrl = ((id -1) / MAX_DEVICES),         // calculate the controller number
                                dev = ((id -1) % MAX_DEVICES);          // calculate the device number

        vxierrno = 0;

        if( ctrl<0 || ctrl>=MAX_CONTROLLERS )           // is the id valid?
                return global_ierrno = ierrno[id] = I_ERR_NODEV;

        if( dev<0 || dev>=MAX_DEVICES)
                return global_ierrno = ierrno[id] = I_ERR_NODEV;

        if (device[ctrl][dev].inst != id)
                return global_ierrno = ierrno[id] = I_ERR_BADID;

        *tval = device[ctrl][dev].io_timeout;   // return the timeout value
}

// =============================================================================
//
//  Function Name:  ilantimeout
//
//  Description:    set rpc timeout value
// 
//  OM 2007-10-18 : the parameter tval must be in milliseconds as for itimeout !!!
//
// -----------------------------------------------------------------------------
int SICLAPI ilantimeout(INST id,long tval)
{
        //struct timeval timeout = {tval, 0};
        struct timeval timeout;
        timeout.tv_sec = tval / 1000; // OM 2007-10-18 : tval im lilliseconds
        timeout.tv_usec = 1000 * (tval % 1000);

        device_setlantimeout(&timeout);
        return global_ierrno = ierrno[id] = I_ERR_NOERROR;
}

// =============================================================================
//
//  Function Name:  ilangettimeout
//
//  Description:    return rpc timeout value (in milliseconds !)
//
// -----------------------------------------------------------------------------
int SICLAPI ilangettimeout(INST id,long _far *tval)
{
        struct timeval timeout;

        device_getlantimeout(&timeout);
        *tval = 1000*timeout.tv_sec +  timeout.tv_usec/1000 ;
        return global_ierrno = ierrno[id] = I_ERR_NOERROR;
}

// =============================================================================
//
//  Function Name:  igeterrno_inst
//
//  Description:   
//  return last error of inst 
//
// -----------------------------------------------------------------------------
int igeterrno_inst(INST id)
{
        return ierrno[id];  // return the instrument error value
}
int igeterrno()
{
        return global_ierrno;  // return the global error value
}

// =============================================================================
//
//  Function Name:  igetvxierrno
//
//  Description:    
//
// -----------------------------------------------------------------------------
int igetvxierrno()
{
        return vxierrno; // return the original vxi errorcode, returned by the used vxi functions
                         // NOT compatible with igeterrstr!!!
}

/* igeterrstr: Es gibt zwei Versionen: bei einer wird der errorstring mit strdup zurückgegeben, und muss
 * daher vom Anwender mit free freigegeben werden. Für unsere Anwendung meines Erachtens ungünstig, da
 * Speicherlecks geradezu heraufbeschworen werden.
 */

/*
// =============================================================================
//
//  Function Name:  igeterrstr
//
//  Description:    return error string with strdup (memory leak!)
//
// -----------------------------------------------------------------------------
char _far * SICLAPI igeterrstr(int errno)
{                               // returns the errorstrings to the SICL errorcodes
        switch(errno)
        {
                case I_ERR_NOERROR: return strdup("No Error");

                case I_ERR_SYNTAX: return strdup("Syntax error");

                case I_ERR_SYMNAME: return strdup("Invalid symbolic name");

                case I_ERR_BADADDR: return strdup("Bad address");

                case I_ERR_PARAM: return strdup("Invalid Parameter");

                case I_ERR_NOCONN: return strdup("No Connection");

                case I_ERR_NOPERM: return strdup("No permission");

                case I_ERR_NOTSUPP: return strdup("Operation not supported");

                case I_ERR_NORSRC: return strdup("Out of resources");

                case I_ERR_NOINTF: return strdup("Interface is not active");

                case I_ERR_LOCKED: return strdup("Locked by another user");

                case I_ERR_NOLOCK: return strdup("Interface not locked");

                case I_ERR_BADFMT: return strdup("Bad format");

                case I_ERR_DATA: return strdup("Data integrity violation");

                case I_ERR_TIMEOUT: return strdup("Timeout occured");

                case I_ERR_OVERFLOW: return strdup("Arithmetic overflow");

                case I_ERR_IO: return strdup("Generic I/O error");

                case I_ERR_OS: return strdup("Generic O.S. error");

                case I_ERR_BADMAP: return strdup("Invalid map request");

                case I_ERR_NODEV: return strdup("Device is not active or available");

                case I_ERR_INVLADDR: return strdup("Invalid address");

                case I_ERR_NOTIMPL: return strdup("Operation not implemented");

                case I_ERR_ABORTED: return strdup("Externally aborted");

                case I_ERR_BADCONFIG: return strdup("Invalid configuration");

                case I_ERR_NOCMDR: return strdup("Commander session is not active or available");

                case I_ERR_VERSION: return strdup("Version incompatibility");

                case I_ERR_CONNEXISTS: return strdup("Connection already established");

                case I_ERR_INTERNAL: return strdup("Internal error");

                case I_ERR_NOMOREDEV: return strdup("No more devices possible on the controller");

                case I_ERR_ABORTCHCR: return strdup("Unable to create abort channel");

                case I_ERR_NOABORTCH: return strdup("No abort channel created");
                default: return strdup("Unknown Error");
        }
}
*/

/* Die zweite Version verfügt über einen localen statischen string, in den sie die Beschreibung des Fehler
 * speichert. Als Rückgabewert liefert sie einen Pointer auf den localen String. Diese Funktion ist allerdings
 * mit Vorsicht zu verwenden, da der String ja auch von außen mainpuliert werden kann, speicherlecks können
 * dafür keine entstehen.
 * Für welche Funktion Sie sich entscheiden, liegt ganz an ihnen, beide erzieln das gleiche Ergebnis
 */
/*
// =============================================================================
//
//  Function Name:  igeterrstr
//
//  Description:    return errstr with static variable (not threadsafe)
//
// -----------------------------------------------------------------------------
char _far * SICLAPI igeterrstr(int errno)
{                               // returns the errorstrings to the SICL errorcodes
        static char errstr[80];

        switch(errno)
        {
                case I_ERR_NOERROR: return strcpy(errstr,"No Error");

                case I_ERR_SYNTAX: return strcpy(errstr,"Syntax error");

                case I_ERR_SYMNAME: return strcpy(errstr,"Invalid symbolic name");

                case I_ERR_BADADDR: return strcpy(errstr,"Bad address");

                case I_ERR_PARAM: return strcpy(errstr,"Invalid Parameter");

                case I_ERR_NOCONN: return strcpy(errstr,"No Connection");

                case I_ERR_NOPERM: return strcpy(errstr,"No permission");

                case I_ERR_NOTSUPP: return strcpy(errstr,"Operation not supported");

                case I_ERR_NORSRC: return strcpy(errstr,"Out of resources");

                case I_ERR_NOINTF: return strcpy(errstr,"Interface is not active");

                case I_ERR_LOCKED: return strcpy(errstr,"Locked by another user");

                case I_ERR_NOLOCK: return strcpy(errstr,"Interface not locked");

                case I_ERR_BADFMT: return strcpy(errstr,"Bad format");

                case I_ERR_DATA: return strcpy(errstr,"Data integrity violation");

                case I_ERR_TIMEOUT: return strcpy(errstr,"Timeout occured");

                case I_ERR_OVERFLOW: return strcpy(errstr,"Arithmetic overflow");

                case I_ERR_IO: return strcpy(errstr,"Generic I/O error");

                case I_ERR_OS: return strcpy(errstr,"Generic O.S. error");

                case I_ERR_BADMAP: return strcpy(errstr,"Invalid map request");

                case I_ERR_NODEV: return strcpy(errstr,"Device is not active or available");

                case I_ERR_INVLADDR: return strcpy(errstr,"Invalid address");

                case I_ERR_NOTIMPL: return strcpy(errstr,"Operation not implemented");

                case I_ERR_ABORTED: return strcpy(errstr,"Externally aborted");

                case I_ERR_BADCONFIG: return strcpy(errstr,"Invalid configuration");

                case I_ERR_NOCMDR: return strcpy(errstr,"Commander session is not active or available");

                case I_ERR_VERSION: return strcpy(errstr,"Version incompatibility");

                case I_ERR_CONNEXISTS: return strcpy(errstr,"Connection already established");

                case I_ERR_INTERNAL: return strcpy(errstr,"Internal error");

                case I_ERR_NOMOREDEV: return strcpy(errstr,"No more devices possible on the controller");

                case I_ERR_ABORTCHCR: return strcpy(errstr,"Unable to create abort channel");

                case I_ERR_NOABORTCH: return strcpy(errstr,"No abort channel created");
                default: return strcpy(errstr,"Unknown Error");
        }
}
*/

/* Another variant of the above without the static variable and thus threadsafe. */

// =============================================================================
//
//  Function Name:  igeterrstr
//
//  Description:    return error string
//
// -----------------------------------------------------------------------------
char _far * SICLAPI igeterrstr(int errornr)
{                               // returns the errorstrings to the SICL errorcodes


        switch(errornr)
        {
                case I_ERR_NOERROR: return "OK";

                case I_ERR_SYNTAX: return "Syntax error";

                case I_ERR_SYMNAME: return "Invalid symbolic name";

                case I_ERR_BADADDR: return "Bad address";

                case I_ERR_PARAM: return "Invalid Parameter";

                case I_ERR_NOCONN: return "No Connection";

                case I_ERR_NOPERM: return "No permission";

                case I_ERR_NOTSUPP: return "Operation not supported";

                case I_ERR_NORSRC: return "Out of resources";

                case I_ERR_NOINTF: return "Interface is not active";

                case I_ERR_LOCKED: return "Locked by another user";

                case I_ERR_NOLOCK: return "Interface not locked";

                case I_ERR_BADFMT: return "Bad format";

                case I_ERR_DATA: return "Data integrity violation";

                case I_ERR_TIMEOUT: return "Timeout occured";

                case I_ERR_OVERFLOW: return "Arithmetic overflow";

                case I_ERR_IO: return "Generic I/O error";

                case I_ERR_OS: return "Generic O.S. error";

                case I_ERR_BADMAP: return "Invalid map request";

                case I_ERR_NODEV: return "Device is not active or available";

                case I_ERR_INVLADDR: return "Invalid address";

                case I_ERR_NOTIMPL: return "Operation not implemented";

                case I_ERR_ABORTED: return "Externally aborted";

                case I_ERR_BADCONFIG: return "Invalid configuration";

                case I_ERR_NOCMDR: return "Commander session is not active or available";

                case I_ERR_VERSION: return "Version incompatibility";

                case I_ERR_CONNEXISTS: return "Connection already established";

                case I_ERR_INTERNAL: return "Internal error";

                case I_ERR_NOMOREDEV: return "No more devices possible on the controller";

                case I_ERR_ABORTCHCR: return "Unable to create abort channel";

                case I_ERR_NOABORTCH: return "No abort channel created";
                default: return "Unknown Error";
        }
}


// =============================================================================
//
//  Function Name:  itrace
//
//  Description:    
//
// -----------------------------------------------------------------------------
int SICLAPI itrace(INST id, FILE *tracefile)
{
        int                     ctrl = ((id -1) / MAX_DEVICES),         // calculate the controller number
                                dev = ((id -1) % MAX_DEVICES);          // calculate the device number

        vxierrno = 0;

        if( ctrl<0 || ctrl>=MAX_CONTROLLERS )   // is the id valid?
                return global_ierrno = ierrno[id] = I_ERR_NODEV;

        if( dev<0 || dev>=MAX_DEVICES)
                return global_ierrno = ierrno[id] = I_ERR_NODEV;

        if (device[ctrl][dev].inst != id)
                return global_ierrno = ierrno[id] = I_ERR_BADID;

        device[ctrl][dev].tracefile = tracefile;        // save tbe tracefile pointer in the device table for further use

        return global_ierrno = ierrno[id] = I_ERR_NOERROR;
}

// =============================================================================
//
//  Function Name:  igpibbusstatus
//
//  Description:    
//
// -----------------------------------------------------------------------------
int SICLAPI igpibbusstatus(INST id, int request, int _far *result)
{
        int                     ctrl = ((id -1) / MAX_DEVICES),         // calculate the controller number
                                dev = ((id -1) % MAX_DEVICES),          // calculate the devcie number
                                cmd;
        char                    strtime[20], log[LOG_SIZE];
        short                   datain, status;
        Device_DocmdParms       ddcmdp;
        Device_DocmdResp        ddcmdr;
        int                     ret;

        vxierrno = 0;
        global_ierrno = ierrno[id] = 0;

        if( ctrl<0 || ctrl>=MAX_CONTROLLERS )   // is the id valid?
                global_ierrno = ierrno[id] = I_ERR_NODEV;

        if( dev<0 || dev>=MAX_DEVICES)
                global_ierrno = ierrno[id] = I_ERR_NODEV;

        if (device[ctrl][dev].inst != id)
                global_ierrno = ierrno[id] = I_ERR_BADID;

        if (device[ctrl][dev].ctrl == 0)
                global_ierrno = ierrno[id] = I_ERR_NOCMDR;

        if ((request <1) || (request > 9))
                global_ierrno = ierrno[id] = I_ERR_PARAM;

        if(!ierrno[id])
        {                                                                // many parameters
                ddcmdp.lid = device[ctrl][dev].linkid;                  // the link id
                ddcmdp.flags = device[ctrl][dev].flags;                 // flags
                ddcmdp.io_timeout = device[ctrl][dev].io_timeout;       // the io timeout
                ddcmdp.lock_timeout = device[ctrl][dev].lock_timeout;     // the lock timeout
                ddcmdp.network_order = 0;                               // network order
                datain = request;                                       // the line which is querried
                ddcmdp.data_in.data_in_val =(char *) &datain;           // ---"---
                ddcmdp.data_in.data_in_len = sizeof(datain);            // the size of data in (has to be 2!)
                ddcmdp.cmd = 0x020001;                                  // the command to querry the line represented by datain
                ddcmdp.datasize = sizeof(datain);                       // the size of thecommand ddcmdp.cmd (has to be 2!)

                ret = device_docmd_1(&ddcmdp, controller[ctrl].ctrlid, &ddcmdr); // call the vxi11 function
        }
        else
                ret = 0;

        if(ret != 0 && !ierrno[id])                      // error check
        {
                switch(ddcmdr.error)
                {
                        case 0: global_ierrno = ierrno[id] = I_ERR_NOERROR; break;

                        case 4: global_ierrno = ierrno[id] = I_ERR_INTERNAL; break;

                        case 6: global_ierrno = ierrno[id] = I_ERR_INTERNAL; break;

                        case 8: global_ierrno = ierrno[id] = I_ERR_NOTSUPP; break;

                        case 11: global_ierrno = ierrno[id] = I_ERR_LOCKED; break;

                        case 15: global_ierrno = ierrno[id] = I_ERR_TIMEOUT; break;

                        case 17: global_ierrno = ierrno[id] = I_ERR_IO; break;

                        case 23: global_ierrno = ierrno[id] = I_ERR_ABORTED; break;

                        default: global_ierrno = ierrno[id] = I_ERR_UNKNOWNERR; break;
                }
                vxierrno = ddcmdr.error;
        }
        else if(!ierrno[id])
                global_ierrno = ierrno[id] = I_ERR_NODEV;

        if (!ierrno[id])
        {
                memcpy(&status, ddcmdr.data_out.data_out_val, 2);      // safe the two significant bytes in status

                *result = status;       // the result of igpibbusstatus is status
        }

        if(device[ctrl][dev].tracefile != 0)    // trace
        {
                datetoString(time(NULL), strtime);

                sprintf(log, "\n%s:[%s(inst:%d)]: ",
                        strtime,
                        device[ctrl][dev].name,
                        device[ctrl][dev].inst
                        );

                if(ret != 0 && !ierrno[id])
                        fprintf(device[ctrl][dev].tracefile,
                                "%s gpibbusstatus: cmd: %ld request: %d result: %d (got:%s) resultlen: %d \nError: %d, (vxierror: %d) %s",
                                log,
                                ddcmdp.cmd,
                                request,
                                status,
                                ddcmdr.data_out.data_out_val,
                                ddcmdr.data_out.data_out_len,
                                ierrno[id],
                                vxierrno,
                                igeterrstr(ierrno[id])
                                );
                else if (ret != 0 && ierrno[id])
                        fprintf(device[ctrl][dev].tracefile,
                                "%s gpibbusstatus: cmd: %ld request: %d \nError: %d (vixerror: %d), %s",
                                log,
                                ddcmdp.cmd,
                                request,
                                ierrno[id],
                                vxierrno,
                                igeterrstr(ierrno[id])
                                );
                else
                        fprintf(device[ctrl][dev].tracefile,
                                "%s gpibbusstatus: Error: %d (vxierror: %d), %s",
                                log,
                                ierrno[id],
                                vxierrno,
                                igeterrstr(ierrno[id])
                                );

                fflush(device[ctrl][dev].tracefile);
        }

        if (ret != 0 && ddcmdr.data_out.data_out_val != NULL)
        {
        // free the data buffer, it was allocated in 
        // xdr_Device_DocmdResp/xdr_bytes, file vxi11_core.c
          free(ddcmdr.data_out.data_out_val);
          ddcmdr.data_out.data_out_len = 0;
          ddcmdr.data_out.data_out_val = NULL;
        }
        return ierrno[id];
}

// =============================================================================
//
//  Function Name:  igetdevname
//
//  Description:    
//
// -----------------------------------------------------------------------------
char * SICLAPI igetdevname(INST id)
{
        int             ctrl = ((id-1) /MAX_DEVICES),   // calculate the controller number
                        dev = ((id-1) % MAX_DEVICES);   // calculate the device number

        global_ierrno = ierrno[id] = 0;

        if( ctrl<0 || ctrl>=MAX_CONTROLLERS )   // is the id valid?
                global_ierrno = ierrno[id] = I_ERR_NODEV;

        if( dev<0 || dev>=MAX_DEVICES)
                global_ierrno = ierrno[id] = I_ERR_NODEV;

        if (device[ctrl][dev].inst != id)
                global_ierrno = ierrno[id] = I_ERR_BADID;

        if(ierrno[id])
                return NULL;

        return device[ctrl][dev].name;  // return the device name
}


// =============================================================================
//
//  Function Name:  igetdevaddr
//
//  Description:    
//
// -----------------------------------------------------------------------------
char * SICLAPI igetdevaddr(INST id)
{
        int             ctrl = ((id-1) /MAX_DEVICES),   // calculate the controller number
                        dev = ((id-1) % MAX_DEVICES);   // calculate the device number
        char     addr[NAME_LEN];
        global_ierrno = ierrno[id] = 0;

        if( ctrl<0 || ctrl>=MAX_CONTROLLERS )   // is the id valid?
                global_ierrno = ierrno[id] = I_ERR_NODEV;

        if( dev<0 || dev>=MAX_DEVICES)
                global_ierrno = ierrno[id] = I_ERR_NODEV;

        if (device[ctrl][dev].inst != id)
                global_ierrno = ierrno[id] = I_ERR_BADID;

        if(ierrno[id])
                return NULL;
        sprintf(addr, "lan[%s]:%s", controller[ctrl].name, device[ctrl][dev].name);
        return strdup(addr);            // build and return a addr iopen could use
}

// =============================================================================
//
//  Function Name:  igetctrlname
//
//  Description:    
//
// -----------------------------------------------------------------------------
char * SICLAPI igetctrlname(INST id)
{
        int             ctrl = ((id-1) /MAX_DEVICES);   // calculate the controller number

        global_ierrno = ierrno[id] = 0;

        if( ctrl<0 || ctrl>=MAX_CONTROLLERS )   // is the id valid?
                global_ierrno = ierrno[id] = I_ERR_NODEV;

        if(controller[ctrl].ctrlid)
                return controller[ctrl].name;   // return the controller name (dns name)

        return NULL;
}

// =============================================================================
//
//  Function Name:  igetctrladdr
//
//  Description:    
//
// -----------------------------------------------------------------------------
char * SICLAPI igetctrladdr(INST id)
{
        int             ctrl = ((id-1) /MAX_DEVICES);   // calculate the controller number
        char            addr[NAME_LEN];

        sprintf(addr, "lan[%s]:hpib", controller[ctrl].name);   // create and return a string that could be used to create a commander session.
                                                                // only pass this addr to iopen to create a interface session
        return strdup(addr);
}

// =============================================================================
//
//  Function Name:  ionsrq
//
//  Description:    
//
// -----------------------------------------------------------------------------
int SICLAPI ionsrq(INST id,srqhandler_t shdlr)
{
        fprintf(stderr,"ionsrq not available for vxi");
        return global_ierrno = ierrno[id] = I_ERR_NOTSUPP;
}
