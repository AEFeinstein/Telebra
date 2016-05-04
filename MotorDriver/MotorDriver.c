#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdint.h>

#include "SerialPort.h"
#include "Qik2s9v1.h"

/**
 * The main function. Spin up the serial port in a separate thread and
 * poll the qik for some data
 *
 * @param argc unused
 * @param argv unused
 * @return 1 for an error, 0 for success
 */
int main(__attribute__((unused)) int argc, __attribute__((unused)) char** argv)
{

    /* The path to the serial port on a Raspberry Pi B+ */
    char serialPortPath[] = "/dev/ttyAMA0";

    /* Create and start a thread to read from the serial port */
    pthread_t serialThread;

    if (pthread_create(&serialThread, NULL, readSerial, (void*)serialPortPath))
    {
        fprintf(stderr, "Error creating thread\n");
        return 1;
    }

    /* Do this forever */
    while(1)
    {
        getErrorByte(DEFAULT_DEVICE_ID);
        getFirmwareVersion(DEFAULT_DEVICE_ID);
        getErrorByte(DEFAULT_DEVICE_ID);
        getConfigurationParameter(DEFAULT_DEVICE_ID, DEVICE_ID);
        getErrorByte(DEFAULT_DEVICE_ID);
        getConfigurationParameter(DEFAULT_DEVICE_ID, PWM_PARAMETER);
        getErrorByte(DEFAULT_DEVICE_ID);
        getConfigurationParameter(DEFAULT_DEVICE_ID, SHUTDOWN_MOTOR_ON_ERROR);
        getErrorByte(DEFAULT_DEVICE_ID);
        getConfigurationParameter(DEFAULT_DEVICE_ID, SERIAL_TIMEOUT);
        sleep(5);
    }

    return 0;
}
