/*
 * SerialPort.c
 *
 *  Created on: Oct 1, 2015
 *      Author: adam
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/signal.h>
#include <termios.h>
#include <ftw.h>
#include <string.h>
#include <stdint.h>

#include "SerialPort.h"
#include "Qik2s9v1.h"

#define UART_RX_BUFSIZE 1024

int SerialPortFileDescrptor = -1;  /*!< The opened serial port descriptor */
struct termios origAttr;           /*!< The original serial port attributes */

/**
 * Spun up in a separate thread, this loops until there is something to be read
 * It reads bytes from the serial port into a buffer
 * This is run on another thread
 *
 * @param vp    unused
 */
void* readSerial(void* vp)
{
    int i;
    uint8_t incBuf[UART_RX_BUFSIZE];

    initializeSerialPort((char*)vp);

    while(1)
    {
        int numRead = read(SerialPortFileDescrptor, incBuf, UART_RX_BUFSIZE);

        for (i = 0; i < numRead; i++)
        {
            /* read in received data */
            processResponse(incBuf[i]);
        }
    }

    cleanUpSerialPort();
}

/**
 * Initialize the given serial port
 *
 * @param serialPort A path to the serial port. Can't be NULL
 */
void initializeSerialPort(char* serialPort)
{
    struct termios termAttr;
    speed_t baudRate;

    /* Attempt to open the serial port */
    SerialPortFileDescrptor = open(serialPort,
      O_RDWR     | /* Read & write */
      O_NONBLOCK | /* Non-blocking reads */
      O_ASYNC    | /* Asynchronous operation */
      O_NOCTTY);   /* Dont make this the controlling terminal for the process */

    /* If the open fails, report it and exit the program */
    if (SerialPortFileDescrptor == -1)
    {
        fprintf(stderr, "open_port: Unable to open %s\n", serialPort);
        fprintf(stderr, "try \"sudo chmod o+rw %s\"\n", serialPort);
        exit(EXIT_FAILURE);
    }
    else
    {
        printf("open_port: %s\n", serialPort);
    }

    /* Get the current serial port attributes */
    tcgetattr(SerialPortFileDescrptor, &origAttr);
    tcgetattr(SerialPortFileDescrptor, &termAttr);

    /* Set Control modes */
    baudRate = B38400;             /* Set the I/O baud at 38400*/
    cfsetispeed(&termAttr, baudRate);
    cfsetospeed(&termAttr, baudRate);
    termAttr.c_cflag &= ~PARENB;            /* Turn off even parity */
    termAttr.c_cflag &= ~PARODD;            /* Turn off odd parity */
    termAttr.c_cflag &= ~CSTOPB;            /* Send one stop bit */
    termAttr.c_cflag &= ~(CS5 | CS6 | CS7); /* Set char size to 8 bits */
    termAttr.c_cflag |=  CS8;
    termAttr.c_cflag |=  CLOCAL;            /* Ignore modem status lines */
    termAttr.c_cflag |=  CREAD;             /* Enable receiver */
    termAttr.c_cflag &= ~HUPCL;             /* Don't hang up on last close */

    /* Set Local modes */
    termAttr.c_lflag &= ~ICANON; /* Turn off canonical input */
    termAttr.c_lflag &= ~ECHO;   /* Disable echo */
    termAttr.c_lflag &= ~ECHOE;  /* Disable echo erase character */
    termAttr.c_lflag &= ~ECHOK;  /* Disable echo KILL */
    termAttr.c_lflag &= ~ECHONL; /* Disable echo NL */
    termAttr.c_lflag &= ~ISIG;   /* Disable signals */
    termAttr.c_lflag &= ~IEXTEN; /* Disable extended input char processing */
    termAttr.c_lflag |=  NOFLSH; /* Disable flush after interrupt or quit */
    termAttr.c_lflag &= ~TOSTOP; /* Don't send SIGTTOU for background output */

    /* Set Input modes */
    termAttr.c_iflag &= ~BRKINT; /* Don't signal interrupt on break */
    termAttr.c_iflag &= ~ICRNL;  /* Don't map CR to NL on input */
    termAttr.c_iflag &= ~INLCR;  /* Don't map NL to CR on input */
    termAttr.c_iflag &= ~IGNBRK; /* Don't ignore break condition */
    termAttr.c_iflag &= ~IGNCR;  /* Don't ignore CR */
    termAttr.c_iflag |=  INPCK;  /* Enable input parity check */
    termAttr.c_iflag |=  IGNPAR; /* Ignore characters with parity errors */
    termAttr.c_iflag &= ~PARMRK; /* Don't mark parity errors */
    termAttr.c_iflag &= ~ISTRIP; /* Don't strip 8th bit from character */
    termAttr.c_iflag &= ~IXANY;  /* Disable any character to restart output */
    termAttr.c_iflag &= ~IXOFF;  /* Disable start/stop input control */
    termAttr.c_iflag &= ~IXON;   /* Disable start/stop output control */

    /* Output modes */
    termAttr.c_oflag &= ~OPOST;  /* Don't post-process output */
    termAttr.c_oflag &= ~ONLCR;  /* Don't map NL to CR-NL on output */
    termAttr.c_oflag &= ~OCRNL;  /* Don't map CR to NL on output */
    termAttr.c_oflag |=  ONOCR;  /* No CR output at column 0 */
    termAttr.c_oflag |=  ONLRET; /* NL performs CR function */
    termAttr.c_oflag |=  OFILL;  /* Use fill characters for delay */
    termAttr.c_oflag &= ~NL1;    /* Select newline delays */
    termAttr.c_oflag |=  NL0;
    termAttr.c_oflag &= ~(CR1 | CR2 | CR3); /* Select carriage-return delays */
    termAttr.c_oflag |=  CR0;
    termAttr.c_oflag &= ~(TAB1 | TAB2 | TAB3); /* Select tab delays */
    termAttr.c_oflag |=  TAB0;
    termAttr.c_oflag &= ~BS1;    /* Select backspace delays */
    termAttr.c_oflag |=  BS0;
    termAttr.c_oflag &= ~VT1;    /* Select vertical-tab delays */
    termAttr.c_oflag |=  VT0;
    termAttr.c_oflag &= ~FF1;    /* Select form-feed delays */
    termAttr.c_oflag |=  FF0;

    termAttr.c_cc[VTIME] = 0;    /* Inter-character timer unused */
    termAttr.c_cc[VMIN]  = 0;    /* Don't wait for a minimum number of chars */

    /* Set the parameters */
    tcsetattr(SerialPortFileDescrptor, TCSANOW, &termAttr);
}

/**
 * Close the serial port when we're all done
 */
void cleanUpSerialPort(void)
{
    tcsetattr(SerialPortFileDescrptor, TCSANOW, &origAttr);
    close(SerialPortFileDescrptor);
}

/**
 * Send a buffer of data over the serial port
 *
 * @param buf The buffer of data to send
 * @param len The length of the buffer to send
 */
void writeToSerialPort(const void * buf, size_t len)
{
    if(SerialPortFileDescrptor != -1)
    {
        write(SerialPortFileDescrptor, buf, len);
    }
}
