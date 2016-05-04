#ifndef _QIK_2s9v1_H_
#define _QIK_2s9v1_H_

/* The default device ID to address the qik at */
#define DEFAULT_DEVICE_ID 0x09

/* Enum for the error bits */
typedef enum
{

    DATA_OVERRUN_ERROR = 0x08, /*!< A hardware-level error that occurs when
    serial data is received while the hardware serial receive buffer is full.
    This error should not occur during normal operation. */

    FRAME_ERROR        = 0x10, /*!< A hardware-level error that occurs when a
    byte’s stop bit is not detected at the expected place. This error can occur
    if you are communicating at a baud rate that differs from the qik’s baud
    rate. */

    CRC_ERROR          = 0x20, /*!< This error occurs when the qik is running in
    CRC-enabled mode (i.e. the CRC-enable jumper is in place) and the cyclic
    redundancy check (CRC) byte added to the end of the command packet does not
    match what the qik has computed as that packet’s CRC. In such a case, the
    qik will ignore the command packet and generate a CRC error. */

    FORMAT_ERROR       = 0x40, /*!< This error occurs when the qik receives an
    incorrectly formatted or nonsensical command packet. For example, if the
    command byte does not match a known command, data bytes are outside of the
    allowed range for their particular command, or an unfinished command packet
    is interrupted by another command packet, a format error will be
    generated. */

    TIMEOUT            = 0x80, /*!< It is possible to use a configuration
    parameter to enable the qik’s serial timeout feature. When enabled, the qik
    will generate a timeout error if the timeout period set by the configuration
    parameter elapses. The timeout timer is reset every time a valid command
    packet is received. */

} error_bit_t;

/* Enum for the configuration parameters */
typedef enum
{

    DEVICE_ID               = 0, /*!< This parameter determines which device ID
    the unit will respond to when the Pololu protocol is used. It has a default
    value of 9 (0x09 in hex) and can be set to any value from 0 – 127. When
    setting this parameter, you should only have one qik on your serial line at
    a time. */

    PWM_PARAMETER           = 1, /*!< This parameter determines frequency and
    resolution of the pulse width modulation (PWM) signal used to control motor
    speed. Note that setting this parameter while the motors are running causes
    them to stop.

    The least significant bit (bit 0) selects for 7-bit resolution when cleared
    (i.e. full motor speed is 127) and 8-bit resolution when set (i.e. full
    motor speed is 255). A PWM with 7-bit resolution has twice the frequency of
    one with 8-bit resolution.

    Bit 1 of this parameter selects for high-frequency mode when cleared and
    low-frequency mode when set. Using high-frequency mode puts the PWM
    frequency outside the range of human hearing if you are also in 7-bit mode
    (or very close to it if you are in 8-bit mode), which can help you decrease
    motor noise. Using low frequency mode has the benefit of decreasing power
    losses due to switching.

    The default value for this parameter is 0 (high-frequency 7-bit mode,
    resulting in a PWM frequency of 31.5 kHz).

    Valid values for this parameter are:
    0 = high-frequency, 7-bit mode (PWM frequency of 31.5 kHz)
    1 = high-frequency, 8-bit mode (PWM frequency of 15.7 kHz)
    2 = low-frequency, 7-bit mode (PWM frequency of 7.8 kHz)
    3 = low-frequency, 8-bit mode (PWM frequency of 3.9 kHz) */

    SHUTDOWN_MOTOR_ON_ERROR = 2, /*!< When this parameter has a value of 1, both
    motors M0 and M1 are stopped as a safety precaution whenever an error
    occurs; otherwise, if this parameter has a value of 0, errors will not
    affect the motors. This parameter has a default value of 1 (shut down the
    motors on any error) and valid values for this parameter are 0 or 1. */

    SERIAL_TIMEOUT          = 3, /*!< When this parameter has a value of 0, the
    serial timeout feature is inactive. Otherwise, the value of this parameter
    controls how much time can elapse between receptions of valid command
    packets before a serial timeout error is generated. This can be used as a
    general safety feature to allow the qik to identify when communication with
    the controlling device is lost and shut down the motors as a result
    (assuming the shutdown motors on error parameter set to a value of 1).

    The timeout duration is specified in increments of 262 ms (approximately a
    quarter of a second) and is calculated as the lower four bits (which are
    interpreted as a number from 0 – 15) times two to the upper three bits
    (which are interpreted as a number from 0 – 7). If the lower four bits are
    called x and the upper three bits are called y, the equation for the length
    of the timeout duration would be:

    timeout = 0.262 seconds * x * 2^y

    For example, if the timeout parameter is set as 0x5E (01011110 in binary),
    we have that x = 1110 (binary) = 14 (decimal) and y = 101 (binary) =
    5 (decimal), which results in a timeout duration of

    0.262s * 14 * 2^5 = 117 seconds.

    The maximum timeout duration (arising from a parameter value of 0x7F, or 127
    in decimal) is 8.32 minutes and the minimum timeout duration (arising from a
    parameter value of 1) is 262 ms.

    This parameter has a default value of 0 (serial timeout disabled) and can be
    set to any value from 0 – 127. */

} config_parameter_t;

/* Function Prototypes */

void processResponse(uint8_t byte);

void getFirmwareVersion(uint8_t deviceId);
void getErrorByte(uint8_t deviceId);

void getConfigurationParameter(uint8_t deviceId, config_parameter_t parameter);
void setConfigurationParameter(uint8_t deviceId, config_parameter_t parameter,
                               uint8_t val);

void setM0Coast(uint8_t deviceId);
void setM1Coast(uint8_t deviceId);
void setM0Forward(uint8_t deviceId, uint8_t speed);
void setM0Reverse(uint8_t deviceId, uint8_t speed);
void setM1Forward(uint8_t deviceId, uint8_t speed);
void setM1Reverse(uint8_t deviceId, uint8_t speed);

#endif /* _QIK_2s9v1_H_ */
