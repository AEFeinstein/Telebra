#include <sys/time.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "Qik2s9v1.h"
#include "SerialPort.h"

#define CMD_TIMEOUT_USEC 100000 /*!< 100ms max wait time for a response */

#define START_BYTE 0xAA /*!< Every command starts with this byte to autobaud */

/* All the different possible commands */
typedef enum
{
    GET_FIRMWARE_VERSION = 0x01,
    GET_ERROR_BYTE = 0x02,
    GET_CONFIG_PARAM = 0x03,
    SET_CONFIG_PARAM = 0x04,
    M0_COAST = 0x06,
    M1_COAST = 0x07,
    M0_FORWARD = 0x08,
    M0_FORWARD_128 = 0x09,
    M0_REVERSE = 0x0A,
    M0_REVERSE_128 = 0x0B,
    M1_FORWARD = 0x0C,
    M1_FORWARD_128 = 0x0D,
    M1_REVERSE = 0x0E,
    M1_REVERSE_128 = 0x0F
} QikCommand_t;

volatile QikCommand_t pendingCmd; /*!< Keep track of the current sent command */
uint64_t cmdSentTimestamp; /*!< Keep track of when the last command was sent */

/* Internal function prototypes */
void sendCommand(uint8_t * buf, size_t len, bool expectResponse);
uint64_t getCurrentTime(void);

/**
 * @return The current time in a 64 bit integer
 */
uint64_t getCurrentTime(void)
{
    struct timeval tp;
    gettimeofday(&tp, NULL);
    return tp.tv_usec + (tp.tv_sec * 1000000);
}

/**
 * If there is a current outgoing command, wait for the response to be
 * received. Then send the given command and mark the time and command,
 * if a response is expected
 *
 * @param buf A pointer to the command to send
 * @param len The length of the command to send
 * @param expectResponse true if this command expects a response,
 * false otherwise
 */
void sendCommand(uint8_t * buf, size_t len, bool expectResponse)
{
    /* While there is a pending command that hasn't timed out yet, spin */
    while ((cmdSentTimestamp + CMD_TIMEOUT_USEC) > getCurrentTime()
            && pendingCmd != 0)
    {
; /* You spin me right round, baby right round */
    }

    if(expectResponse)
    {
        /* Mark the current time and pending command */
        cmdSentTimestamp = getCurrentTime();
        pendingCmd = buf[2];
    }
    else
    {
        pendingCmd = 0;
    }

    /* Send the message */
    writeToSerialPort(buf, len);
}

/**
 * Process a response byte
 *
 * @param byte The byte the Qik sent back
 */
void processResponse(uint8_t byte)
{
    switch (pendingCmd)
    {
        case GET_FIRMWARE_VERSION:
        {
            printf("GET_FIRMWARE_VERSION %d\n", byte);
            break;
        }

        case GET_ERROR_BYTE:
        {
            printf("GET_ERROR_BYTE %d\n", byte);
            break;
        }

        case GET_CONFIG_PARAM:
        {
            printf("GET_CONFIGURATION_PARAM %d\n", byte);
            break;
        }

        case SET_CONFIG_PARAM:
        {
            printf("SET_CONFIGURATION_PARAM %d\n", byte);
            break;
        }
        /* These commands do not have responses */
        case M0_COAST:
        case M1_COAST:
        case M0_FORWARD:
        case M0_FORWARD_128:
        case M0_REVERSE:
        case M0_REVERSE_128:
        case M1_FORWARD:
        case M1_FORWARD_128:
        case M1_REVERSE:
        case M1_REVERSE_128:
        {
            break;
        }
    }

    pendingCmd = 0;
}

/**
 * This command returns the a single ASCII byte that represents the version of
 * the firmware running on the Qik. All Qiks produced so far have firmware
 * version ‘1’ or ‘2’.
 *
 * @param deviceId The device ID to send this command to
 */
void getFirmwareVersion(uint8_t deviceId)
{
    uint8_t msg[3];
    msg[0] = START_BYTE;
    msg[1] = deviceId;
    msg[2] = GET_FIRMWARE_VERSION;
    sendCommand(msg, sizeof(msg), true);
}

/**
 * The Qik maintains an error byte, the bits of which, when set, reflect various
 * errors that have been detected since the byte was last read using this
 * command.
 *
 * This error byte will be processed by processResponse().
 *
 * An error will cause the red LED to light and the ERR pin to drive high until
 * this command is called. Calling this command will clear the error byte, turn
 * the red LED off, and set the ERR pin as high-impedance (causing it to be
 * pulled low through the LED). If shutdown-on-error configuration parameter is
 * set to 1, motors M0 and M1 will be stopped as a safety precaution when any of
 * these errors occurs.
 *
 * @param deviceId The device ID to send this command to
 */
void getErrorByte(uint8_t deviceId)
{
    uint8_t msg[3];
    msg[0] = START_BYTE;
    msg[1] = deviceId;
    msg[2] = GET_ERROR_BYTE;
    sendCommand(msg, sizeof(msg), true);
}

/**
 * Request a single configuration parameter from the qik. The single byte
 * returned by the qik will be processed by processResponse().
 *
 * @param deviceId The device ID to send this command to
 * @param parameter The configuration parameter to fetch
 */
void getConfigurationParameter(uint8_t deviceId, config_parameter_t parameter)
{
    uint8_t msg[3];
    msg[0] = START_BYTE;
    msg[1] = deviceId;
    msg[2] = GET_CONFIG_PARAM;
    msg[3] = parameter;
    sendCommand(msg, sizeof(msg), true);
}

/**
 * Set a single configuration parameter on the qik. The qik will reply with a
 * single status byte:
 *
 * 0: Command OK (success)
 * 1: Bad Parameter (invalid parameter number)
 * 2: Bad value (invalid parameter value for the specified parameter number)
 *
 * @param deviceId The device ID to send this command to
 * @param parameter The configuration parameter to set
 * @param val The value to set the configuration parameter to
 */
void setConfigurationParameter(uint8_t deviceId, config_parameter_t parameter,
        uint8_t val)
{
    /* The last two bytes are magic bytes to make sure config parameters
     * don't get accidentally set
     */
    uint8_t msg[7];
    msg[0] = START_BYTE;
    msg[1] = deviceId;
    msg[2] = SET_CONFIG_PARAM;
    msg[3] = parameter;
    msg[4] = val;
    msg[5] = 0x55;
    msg[6] = 0x2A;
    sendCommand(msg, sizeof(msg), true);
}

/**
 * Sets motor 0 output to high impedance, letting it turn freely.
 * This is in contrast to setting speed to 0, which acts as a brake
 *
 * @param deviceId the device Id to send the command to
 */
void setM0Coast(uint8_t deviceId)
{
    uint8_t msg[3];
    msg[0] = START_BYTE;
    msg[1] = deviceId;
    msg[2] = M0_COAST;
    sendCommand(msg, sizeof(msg), false);
}

/**
 * Spin motor 0 forward. In 8 bit mode, the full range from 0-255 is used.
 * In 7 bit mode, the range from 0-127 is equivalent to 128-255.
 *
 * @param deviceId the device Id to send the command to
 * @param speed The speed to set the motor to (0-255)
 */
void setM0Forward(uint8_t deviceId, uint8_t speed)
{
    if(speed > 127)
    {
        uint8_t msg[3];
        msg[0] = START_BYTE;
        msg[1] = deviceId;
        msg[2] = M0_FORWARD_128;
        msg[3] = speed - 128;
        sendCommand(msg, sizeof(msg), false);
    }
    else
    {
        uint8_t msg[3];
        msg[0] = START_BYTE;
        msg[1] = deviceId;
        msg[2] = M0_FORWARD;
        msg[3] = speed;
        sendCommand(msg, sizeof(msg), false);
    }
}

/**
 * Spin motor 0 reverse. In 8 bit mode, the full range from 0-255 is used.
 * In 7 bit mode, the range from 0-127 is equivalent to 128-255.
 *
 * @param deviceId the device Id to send the command to
 * @param speed The speed to set the motor to (0-255)
 */
void setM0Reverse(uint8_t deviceId, uint8_t speed)
{
    if(speed > 127)
    {
        uint8_t msg[3];
        msg[0] = START_BYTE;
        msg[1] = deviceId;
        msg[2] = M0_REVERSE_128;
        msg[3] = speed - 128;
        sendCommand(msg, sizeof(msg), false);
    }
    else
    {
        uint8_t msg[3];
        msg[0] = START_BYTE;
        msg[1] = deviceId;
        msg[2] = M0_REVERSE;
        msg[3] = speed;
        sendCommand(msg, sizeof(msg), false);
    }
}

/**
 * Sets motor 1 output to high impedance, letting it turn freely.
 * This is in contrast to setting speed to 0, which acts as a brake
 *
 * @param deviceId the device Id to send the command to
 */
void setM1Coast(uint8_t deviceId)
{
    uint8_t msg[3];
    msg[0] = START_BYTE;
    msg[1] = deviceId;
    msg[2] = M1_COAST;
    sendCommand(msg, sizeof(msg), false);
}

/**
 * Spin motor 1 forward. In 8 bit mode, the full range from 0-255 is used.
 * In 7 bit mode, the range from 0-127 is equivalent to 128-255.
 *
 * @param deviceId the device Id to send the command to
 * @param speed The speed to set the motor to (0-255)
 */
void setM1Forward(uint8_t deviceId, uint8_t speed)
{
    if(speed > 127)
    {
        uint8_t msg[3];
        msg[0] = START_BYTE;
        msg[1] = deviceId;
        msg[2] = M1_FORWARD_128;
        msg[3] = speed - 128;
        sendCommand(msg, sizeof(msg), false);
    }
    else
    {
        uint8_t msg[3];
        msg[0] = START_BYTE;
        msg[1] = deviceId;
        msg[2] = M1_FORWARD;
        msg[3] = speed;
        sendCommand(msg, sizeof(msg), false);
    }
}

/**
 * Spin motor 1 reverse. In 8 bit mode, the full range from 0-255 is used.
 * In 7 bit mode, the range from 0-127 is equivalent to 128-255.
 *
 * @param deviceId the device Id to send the command to
 * @param speed The speed to set the motor to (0-255)
 */
void setM1Reverse(uint8_t deviceId, uint8_t speed)
{
    if(speed > 127)
    {
        uint8_t msg[3];
        msg[0] = START_BYTE;
        msg[1] = deviceId;
        msg[2] = M1_REVERSE_128;
        msg[3] = speed - 128;
        sendCommand(msg, sizeof(msg), false);
    }
    else
    {
        uint8_t msg[3];
        msg[0] = START_BYTE;
        msg[1] = deviceId;
        msg[2] = M1_REVERSE;
        msg[3] = speed;
        sendCommand(msg, sizeof(msg), false);
    }
}

/**
 * Process a POST to motor_control.c
 *
 * @param postContent a string command: (UP|DOWN|LEFT|RIGHT)_(START_STOP)
 * @param contentLength the length of the content
 */
void processMotorControl(char* postContent, int32_t contentLength)
{
    uint8_t speed;
    char *dir, *start;
    const char delim[2] = "_";

    printf("processMotorControl (%d) %s\n", contentLength, postContent);

    /* get the first token */
    dir = strtok(postContent, delim);
    start = strtok(NULL, delim);

    if(0 == strcmp(start, "START"))
    {
        speed = 0xFF;
    }
    else if(0 == strcmp(start, "STOP"))
    {
        speed = 0;
    }
    else
    {
        return;
    }

    if(0 == strcmp(dir, "UP"))
    {
        setM0Forward(DEFAULT_DEVICE_ID, speed);
        setM1Forward(DEFAULT_DEVICE_ID, speed);
    }
    else if (0 == strcmp(dir, "DOWN"))
    {
        setM0Reverse(DEFAULT_DEVICE_ID, speed);
        setM1Reverse(DEFAULT_DEVICE_ID, speed);
    }
    else if (0 == strcmp(dir, "LEFT"))
    {
        setM0Forward(DEFAULT_DEVICE_ID, speed);
        setM1Reverse(DEFAULT_DEVICE_ID, speed);
    }
    else if (0 == strcmp(dir, "RIGHT"))
    {
        setM0Reverse(DEFAULT_DEVICE_ID, speed);
        setM1Forward(DEFAULT_DEVICE_ID, speed);
    }
    else
    {
        return;
    }
}
