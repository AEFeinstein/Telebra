#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdint.h>
#include <pigpio.h>

#include "SerialPort.h"
#include "Qik2s9v1.h"
#include "httpd.h"

#define ERROR_PIN 4

/* Function declarations */
uint8_t initializeGpio(void);
void errorFunc(__attribute__((unused)) int gpio, __attribute__((unused)) int level,
        __attribute__((unused)) uint32_t tick);

/**
 * This interrupt is called when the error GPIO goes high and requests
 * the error byte from the Qik
 *
 * @param gpio  unused
 * @param level unused
 * @param tick  unused
 */
void errorFunc(__attribute__((unused)) int gpio, __attribute__((unused)) int level,
        __attribute__((unused)) uint32_t tick)
{
    getErrorByte(DEFAULT_DEVICE_ID);
}

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

    /* The port to serve the webpage on */
    uint16_t port = 43742;

    /* Threads */
    pthread_t serialThread;
    pthread_t httpdThread;

    /* Initialize and setup the GPIO */
    if (0 == initializeGpio())
    {
        fprintf(stderr, "Error initializing GPIO\n");
        return 1;
    }

    /* Create and start a thread to read from the serial port */
    if (pthread_create(&serialThread, NULL, readSerial, (void*) serialPortPath))
    {
        fprintf(stderr, "Error creating serial thread\n");
        return 1;
    }

    /* Create and start a thread to do web stuff */
    if (pthread_create(&httpdThread, NULL, httpdMain, (void*) (&port)))
    {
        fprintf(stderr, "Error creating httpd thread\n");
        return 1;
    }

    /* Get some initial info */
    getFirmwareVersion(DEFAULT_DEVICE_ID);
    getConfigurationParameter(DEFAULT_DEVICE_ID, DEVICE_ID);
    getConfigurationParameter(DEFAULT_DEVICE_ID, PWM_PARAMETER);
    getConfigurationParameter(DEFAULT_DEVICE_ID, SHUTDOWN_MOTOR_ON_ERROR);
    getConfigurationParameter(DEFAULT_DEVICE_ID, SERIAL_TIMEOUT);

    /* Do this forever */
    while (1)
    {
        processQikState();
    }

    gpioTerminate();

    return 0;
}

/**
 * Initialize the GPIO and set the interrupt for the ERROR_PIN
 *
 * @return 0 if something failed, 1 for success
 */
uint8_t initializeGpio(void)
{
    /* Initialize the GPIO */
    if (PI_INIT_FAILED == gpioInitialise())
    {
        return 0;
    }

    /* Configure the ERROR_PIN as an input with a pulldown */
    if (0 != gpioSetMode(ERROR_PIN, PI_INPUT))
    {
        return 0;
    }
    if (0 != gpioSetPullUpDown(ERROR_PIN, PI_PUD_DOWN))
    {
        return 0;
    }

    /* If the ERROR_PIN ever rises, call errorFunc() */
    if (0 != gpioSetISRFunc(ERROR_PIN, RISING_EDGE, 0, errorFunc))
    {
        return 0;
    }

    /* Check ERROR_PIN before continuing */
    if(gpioRead(ERROR_PIN) != 0)
    {
        /* Starting out in the error state */
        getErrorByte(DEFAULT_DEVICE_ID);
    }
    return 1;
}
