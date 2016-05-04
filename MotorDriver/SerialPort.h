/*
 * SerialPort.h
 *
 *  Created on: Oct 1, 2015
 *      Author: adam
 */

#ifndef _SERIALPORT_H_
#define _SERIALPORT_H_

/* Function prototypes */
void* readSerial(void* vp);
void initializeSerialPort(char*);
void cleanUpSerialPort(void);
void writeToSerialPort(const void * buf, size_t len);

#endif /* _SERIALPORT_H_ */
