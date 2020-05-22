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


int sendpack(unsigned char* , int , int);
int recpack(int);

int main(int argc, char *argv[])
{ 

	int rc;
    int i;

    unsigned char msg_0[20] = {0x10,0x02,0x50,0x00,0x62,0x10,0x03};         // Aquarite SWG PING (or PROBE)
    int msg_0_len = 7;                                                      // length of message in bytes

    // unsigned char msg_0[20] = {0x01,0x03,0x21,0x01,0x00,0x07,0x5F,0xF4};  //frequency drive RS485 read registers

    unsigned char msg_1[20] = {0x10,0x02,0x50,0x11,0x0a,0x7d,0x10,0x03};    // SWG set % to 10% (0x0a - 5th byte) - change checksum if needed
    int msg_1_len = 8;                                                      // length of message in bytes
	    
    // ----uart area
    int uart0_filestream = -1;

    char devname[20];
    sprintf(devname, "/dev/serial0");

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
    int ret;

    ret = sendpack(msg_0,msg_0_len,uart0_filestream);   // message, its length, and the uart handle
    if(ret == 1) return 0;
    usleep(200000);
    ret = recpack(uart0_filestream);            //uart handle
    if(ret == 1) return 0;
    usleep(10000);
    ret = sendpack(msg_1,msg_1_len,uart0_filestream);
    if(ret == 1) return 0;
    usleep(20000);
    ret = recpack(uart0_filestream);

    return 0;

}

int sendpack(unsigned char* buf, int len, int up)
{

    int count = 0;
    int i,x;

    for (i=0; i<len; i += count) {
        count = write(up, buf + i, len - i);
        if (count < 0)
            {
                printf("UART TX error\n");
                return 1;
            } else {
                printf("TX => ");
                for (x=i;x<(i+count);x++) {
                    printf("%02x ",buf[x]);
                }
                printf("\n");
                return 0;
            }
    }

}

int recpack(int up)
{

    int rx_length;
    unsigned char rx_buffer[256];
    int x;

    rx_length = read(up, (void*)rx_buffer, 255);		//Filestream, buffer to store in, number of bytes to read (max)
    if (rx_length < 0)
    {
        //An error occured (this may occur if RS485 is not powered)
        printf("UART RX ERROR\n");
        return 1;
    }
    else if (rx_length == 0)
    {
        //No data waiting
        printf("No data waiting\n");
        return 2;
    }
    else
    {
        //Bytes received
        rx_buffer[rx_length] = '\0';
        printf("RX <= ");
        for (x=0;x < rx_length; x++) {
            printf("%02x ", rx_buffer[x]);
        }
        printf("\n");
        return 0;
    }

}


