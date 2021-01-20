#include "sicl.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[])
{
        INST            idevice[5] =  {0};
        char            inp[80],response[80], data[80], param[32];
        int             dev, err=0, reason, c, waitlock=0;
        long            actualcnt;
        unsigned char   stb=255;
        FILE            *tracefile;
        long            lantimeout;
        char            *init19[] = {"*rst\n","*cls\n","*ese 60\n","*sre 168\n","stat:oper:enab 32767\n","stat:oper:ptr 32767\n","stat:ques:enab 32767\n","stat:ques:ptr 32767\n","*wai\n"};
	long length=0;
	char blob[60*16000];

        tracefile = fopen("/home/cryo/Spool/hpib_trace","w+");

        iinit();
        if((idevice[0] = iopen("lan[cryo_gw_x]:hpib,1"))==0)
                printf("\nErr: %d, %s", igeterrno(), igeterrstr(igeterrno()));
        else
                printf("\ndevice[0]:%s", igetdevaddr(idevice[0]));
        
	/*if((idevice[1] = iopen("lan[hpram1]:hpib,19"))==0)
                printf("\nErr: %d, %s", igeterrno(), igeterrstr(igeterrno()));
        else
                printf("\ndevice[1]:%s", igetdevaddr(idevice[1]));
        if((idevice[2] = igetintfsess(idevice[1]))==0)
                printf("\nErr: %d, %s", igeterrno(), igeterrstr(igeterrno()));
        else
                printf("\ndevice[2]:%s", igetctrladdr(idevice[2]));
        if((idevice[3] = iopen("lan[cptrgw1]:hpib,3"))==0)
                printf("\nErr: %d, %s", igeterrno(), igeterrstr(igeterrno()));
        else
                printf("\ndevice[3]:%s", igetdevaddr(idevice[3]));

        if((idevice[4] = igetintfsess(idevice[3])) == 0)
                printf("\nErr: %d, %s", igeterrno(), igeterrstr(igeterrno()));
        else
	printf("\ndevice[4]:%s", igetctrladdr(idevice[4]));*/
/*
        resp=iwrite(dev1, text, strlen(text), 13, &lenght);
        printf ("\n write response: %s\n ", igeterrstr(resp));
        printf("\n sent: %s", text);
        bzero(response, sizeof(response));
        resp = iread(dev1, response, sizeof(response), &reason, &lenght);
        printf ("\n read response: %d\n ", resp);
        printf ("read: %s\n", response);
        resp=ireadstb(dev1, &stb);
        printf ("\n ireadstb response: %d\n ", resp);
        printf("\nstb: %d\n", stb);
*/
        if(tracefile == NULL) return 1;
        for(c=0; c<1;c++)
                itrace(idevice[c], tracefile);

/*        for(c=0; c<9; c++)
                iwrite(idevice[0], init19[c], strlen(init19[c]), 0, &actualcnt);
*/
	itimeout(idevice[0], 50000);
        ilantimeout(idevice[0], 50000);
        ilangettimeout(idevice[0], &lantimeout);
        printf("\nLantimeout: %d", lantimeout);
	
	err = iclear(idevice[0]);

        strcpy(data, "SYST:DISP:UPD ON");
	err = iwrite(idevice[0], data, strlen(data), 13, &length);

        strcpy(data, "MMEM:NAME 'd:\\hardcpy.bmp'");
	err = iwrite(idevice[0], data, strlen(data), 13, &length);

        strcpy(data, "HCOP:DEST 'MMEM'");
	err = iwrite(idevice[0], data, strlen(data), 13, &length);

        strcpy(data, "HCOP:DEV:COL OFF");
	err = iwrite(idevice[0], data, strlen(data), 13, &length);

        strcpy(data, "HCOP:DEV:LANG BMP");
	err = iwrite(idevice[0], data, strlen(data), 13, &length);

        strcpy(data, "HCOP:ITEM:ALL");
	err = iwrite(idevice[0], data, strlen(data), 13, &length);

        strcpy(data, "HCOP:IMM");
	err = iwrite(idevice[0], data, strlen(data), 13, &length);

	// check for errors
 	strcpy(data, "SYST:ERR?");
	strcpy(blob, "");
	err = iwrite(idevice[0], data, strlen(data), 13, &length);
	err = iread(idevice[0], blob, sizeof(blob), &reason, &length);

	strcpy(data, "MMEM:DATA? 'd:\\hardcpy.bmp'");
	err = iwrite(idevice[0], data, strlen(data), 13, &length);

	strcpy(blob, "");
	err = iread(idevice[0], blob, sizeof(blob), &reason, &length);

	// check again for errors
 	strcpy(data, "SYST:ERR?");
	strcpy(blob, "");
	err = iwrite(idevice[0], data, strlen(data), 13, &length);
	err = iread(idevice[0], blob, sizeof(blob), &reason, &length);

       do
        {
                memset(inp,0,sizeof(inp));
                printf("\ndevice command: ");
                fgets(inp, sizeof(inp), stdin);

                dev=inp[0]-'0';

                for (c=2; c<sizeof(inp); c++)
                {
                        inp[c-2]=inp[c];
                        if (inp[c-2]=='\n')
                        {
                                inp[c-2]=0;
                                break;
                        }
                }

/*                if(idevice[dev] == 0)
                {
                        printf(" No such Device!");
                        continue;
                }
*/
                if(strcmp(inp,"quit") == 0)
                {
                        break;
                }

                if(strcmp(inp,"local") == 0)
                {
                        err = ilocal(idevice[dev]);
                        if (err != I_ERR_NOERROR)
                                printf("\ndev[%d]: Err: %d,%s", dev, err, igeterrstr(err));
                        continue;
                }

                if(strcmp(inp,"remote") == 0)
                {
                        err = iremote(idevice[dev]);
                        if (err != I_ERR_NOERROR)
                                printf("\ndev[%d]: Err: %d,%s", dev, err, igeterrstr(err));
                        continue;
                }


                if(strcmp(inp, "abort") == 0)
                {
                        err = iabort(idevice[dev]);
                        if (err != I_ERR_NOERROR)
                                printf("\ndev[%d]: Err: %d,%s", dev, err, igeterrstr(err));
                        continue;
                }

                if(strcmp(inp, "clear") == 0)
                {
                        err = iclear(idevice[dev]);
                        if (err != I_ERR_NOERROR)
                                printf("\ndev[%d]: Err: %d,%s", dev, err, igeterrstr(err));
                        continue;
                }

                if(strcmp(inp, "stb") == 0)
                {
                        err = ireadstb(idevice[dev], &stb);
                        if (err != I_ERR_NOERROR)
                                printf("\ndev[%d]: Err: %d,%s", dev, err, igeterrstr(err));
                        else
                                printf("\ndevice[%d]; Stb; %d", dev, stb);
                        continue;
                }

                if(strcmp(inp, "trigger") == 0)
                {
                        err = itrigger(idevice[dev]);

                        if (err != I_ERR_NOERROR)
                                printf("\ndev[%d]: Err: %d,%s", dev, err, igeterrstr(err));
                        continue;
                }

                if(strcmp(inp, "srq") == 0)
                {
                        err= igpibbusstatus(idevice[dev], I_GPIB_BUS_SRQ, &reason);

                        if(err != I_ERR_NOERROR)
                                printf("\ndev[%d]: Err: %d,%s", dev, err, igeterrstr(err));

                        printf("\nsrq line status: %d\n", reason);
                        continue;
                }

                if(strcmp(inp, "lock") == 0)
                {
                        err = ilock(idevice[dev]);
                        if(err != I_ERR_NOERROR)
                                printf("\ndev[%d]: Err: %d,%s", dev, err, igeterrstr(err));
                        continue;
                }

                if(strcmp(inp, "unlock") == 0)
                {
                        err = iunlock(idevice[dev]);
                        if(err != I_ERR_NOERROR)
                                printf("\ndev[%d]: Err: %d,%s", dev, err, igeterrstr(err));
                        continue;
                }

                if(strcmp(inp, "setlockwait") == 0)
                {
                        err = igetlockwait (idevice[dev], &waitlock);
                        if(err != I_ERR_NOERROR)
                                printf("\ndev[%d]: Err: %d,%s", dev, err, igeterrstr(err));

                        err = isetlockwait(idevice[dev], !waitlock);
                        if(err != I_ERR_NOERROR)
                                printf("\ndev[%d]: Err: %d,%s", dev, err, igeterrstr(err));
                        continue;
                }

                if (strcmp(inp,"read") == 0)
                {
                        bzero(response, sizeof(response));
                        reason = 0;
                        actualcnt = 0;

                        err = iread(idevice[dev], response, sizeof(response), &reason, &actualcnt);

                        if (err != I_ERR_NOERROR)
                        {
                                printf("dev[%d]: Err: %d,%s", dev, err, igeterrstr(err));
                                continue;
                        }

                        if ((strlen(response) ==0) || (actualcnt == 0))
                        {
                                printf("\nno Data received\n");
                                continue;
                        }

                        printf("\nReadResp: %s\nDataLenght: %d", response,actualcnt);
                        printf("\nreason: %d\n", reason);
                        continue;
                }

                strcat(inp, "\n");
                strcpy(data, inp);
                actualcnt = 0;

                err = iwrite(idevice[dev], data, strlen(data), 0, &actualcnt);

                printf("\nsent: %s", data);

                if (err != I_ERR_NOERROR)
                {
                        printf("dev[%d]: Err: %d,%s", dev, err, igeterrstr(err));
                        continue;
                }

        } while(1);

        for (c=0; c<5;c++)
                iclose(idevice[c]);
        fclose(tracefile);
}

/*
int main(int argc, char *argv[])
{
        INST dev;
        int err, reason, actualcnt;
        char buff[256];

        dev = iopen("lan[hpram1]:hpib,12");

        strcpy(buff, "*idn?");
        err = iwrite( dev, buff, strlen(buff), 0, &actualcnt);

        printf("write errorcode: %d, %s", err = igeterrno(), igeterrstr(err));

        err = iread( dev, buff, sizeof(buff), &reason, &actualcnt);

        printf("read errorcode: %d, %s", err = igeterrno(), igeterrstr(err));

        bzero(buff, sizeof(buff));

        err = iread(dev, buff, sizeof(buff), & reason, &actualcnt);

        printf("read errorcode: %d, %s", err = igeterrno(), igeterrstr(err));

        iclose(dev);
} */
