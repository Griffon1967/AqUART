//---------------------------------------------------------------------------------------
//
// Author: G.Faley
// Date: May 2020
//
// Program to read/write UART port for Goldline RS485.
// Ensure UART is enabled in Config and switched to /dev/serial0
// 
//
// gcc -o aquart aquart.c
//
//---------------------------------------------------------------------------------------
#include <stdio.h> //For standard things
#include <stdlib.h>    //malloc
#include <string.h>    //memset
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <termios.h>		//Used for UART
#include <unistd.h>			//Used for UART
#include <sys/syslog.h>


int sendpack(unsigned char* , int);
int recpack( unsigned char* , int , int );
void PrintData (unsigned char* , int);

int main(int argc, char *argv[])
{ 

	int rc;
    int i;
    int rx_len;

    // receiving buffer
    unsigned char rx_buffer[255];

    // probe packet
    // unsigned char msg_0[20] = {0x10,0x02,0x50,0x00,0x62,0x10,0x03};         // Aquarite SWG PING (or PROBE)
    unsigned char msg_probe[02] = {0x00,0xff}; 
    // set salt % packet
    unsigned char msg_setsalt[20] = {0x11,0x01e,0xff}; //,0x10,0x03};    // SWG set % to 10% (0x0a - 5th byte) - change checksum if needed
    // request device ID packet
    unsigned char msg_vers[20] = {0x14,0x00,0xff}; //,0x10,0x03};                                                    // length of message in bytes
    unsigned char msg_test[20] = {0x05,0x20,0x00,0x00,0x01,0xff};
    // ----uart area
    int uart0_filestream = -1;

    char devname[20];
    sprintf(devname, "/dev/ttyAMA1");

    for (i = 1; i < argc; i++) {

        if (i == 1){ //name of serial device
            // addr = (uint16_t)atoi(argv[i]);
            sprintf(devname,"/dev/%s",argv[i]);
        }
    }

    printf("using device --> %s\n",devname);
    uart0_filestream = open(devname, O_RDWR  | O_NOCTTY | O_NONBLOCK | O_NDELAY);		//Open in non blocking read/write mode
    if (uart0_filestream < 0)
	{
		//ERROR - CAN'T OPEN SERIAL PORT
		printf("Error - Unable to open UART.  Ensure it is not in use by another application\n");
	}
	printf("open UART \n");
    struct termios options;
    rc = tcgetattr(uart0_filestream, &options);
    if (rc < 0) {
	    printf("failed to get attr: %d\n", rc);	
    }
    printf("read attrs cflag = %d\n",options.c_cflag);
    options.c_cflag = B9600 | CS8 | CLOCAL | CREAD;	// Aquarite SWG
    // options.c_cflag = B9600 | CS8 | CSTOPB | CLOCAL | CREAD;	// GS Drives VFD (1hp motor)
    options.c_iflag = IGNPAR;
    options.c_oflag = 0;
    options.c_lflag = 0;
    tcflush(uart0_filestream, TCIFLUSH);
    rc = tcsetattr(uart0_filestream, TCSANOW, &options);
    if (rc < 0) {
	    printf("failed to set attr: %d\n", rc);	
    }
    rc = tcgetattr(uart0_filestream, &options);
	printf("sent attrs %d\n",options.c_cflag);

    int x;


    rc = sendpack(msg_probe,uart0_filestream);   // message, its length, and the uart handle
    if(rc == 1) return 0;
    usleep(500000);
    rc = recpack( rx_buffer, rx_len, uart0_filestream); ;            //uart handle
    if(rc == 3) return 0;
    usleep(10000);
    rc = sendpack(msg_vers,uart0_filestream);
    if(rc == 1) return 0;
    usleep(500000);
    rc = recpack( rx_buffer, rx_len, uart0_filestream); 
    if(rc == 3) return 0;
    // usleep(10000);
    // for(x=0;x<30;x++) {
    //     msg_vers[3] = 20 + x;
    //     msg_vers[5] = 118 + x;
        // rc = sendpack(msg_vers,uart0_filestream);
        // if(rc == 1) return 0;
        // usleep(20000);
        // rc = recpack( rx_buffer, rx_len, uart0_filestream); 
        // if(rc == 3) return 0;
        // sleep(2);
    // }

    return 0;

}

int sendpack(unsigned char* buf, int up)
{

    int count = 0;
    int i = 3;
    int done = 0;
    int checkval;
    int len = 0;
    int x = 0;

    const char pckhdr[3] = {0x10,0x02,0x50};
    unsigned char packbuild[30];

    checkval = 0x62;
    memcpy(packbuild,pckhdr,3);

    // build remainder of packet
    while(done == 0){
        if(buf[x] == 0xff) { //last byte of packet - to be ignored
            packbuild[i] = checkval;
            packbuild[i + 1] = 0x10;
            packbuild[i + 2] = 0x03;
            len = i + 3;
            done = 1;
        } else {
            checkval = checkval + buf[x];
            packbuild[i] = buf[x];
            x++;
            i++;
            if(x > 30) done = 2; //message getting too big must be error
        }
    }

    if (done == 2) return 1; //incoming packet was not correct in some regard

    for (i=0; i<len; i += count) {
        count = write(up, packbuild + i, len - i);
        if (count < 0)
            {
                printf("UART TX error\n");
                return 1;
            } else {
                printf("TX => ");
                for (x=i;x<(i+count);x++) {
                    printf("%02x ",packbuild[x]);
                }
                printf("\n");
                return 0;
            }
    }

}

int recpack(unsigned char* buf, int qq, int up)
{

    int rx_length;
    unsigned char rawbuf[256];
    int done = 0;

    qq = 0;
    
    while(done == 0) {
        rx_length = read(up, (void*)rawbuf, 255);		//Filestream, buffer to store in, number of bytes to read (max)

        if (rx_length < 0)
        {
            //An error occured (this may occur if RS485 is not powered)
            printf("UART RX ERROR\n");
            done = 3;
        }
        else if (rx_length == 0)
        {
            //No data waiting
            done = 1;
        }
        else
        {
            //Bytes received

            memcpy(&buf[qq],&rawbuf[0],rx_length);
            qq=qq+rx_length;						
            done = 1;
        }
    }

    if (qq > 0) {
        printf("RX <= ");
        for (int x=0;x < qq; x++) {
            printf("%02x ", buf[x]);
        }
        printf("\n");
    } else {
        printf("No Data Recvd \n");
    }

    return done;
}

void PrintData (unsigned char* data , int Size)
{
    unsigned short i;
     
    printf("\n");
    
    for(i=0 ; i < Size ; i++)
    {		    	
        printf(" %02X",(unsigned int)data[i]);
                     
        if( i==Size-1)  //print the last spaces
        {
			            printf("\n");
		}
    }
    
}
