/***************************************************************************//**
* @file    lib_apds9960.c
* @version 1.0.0
*
* @brief .
*
* @author Jaroslav Groman
*
* @date
*
*******************************************************************************/

#include <stdbool.h>
#include <stdarg.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include <applibs/log.h>

#include "lib_apds9960.h"

#ifdef APDS9960_DEBUG
#define DEBUG(s, f, ...) log_printf("%s %s: " s "\n", "ADPS", f, ## __VA_ARGS__)
#define DEBUG_DEV(s, f, d, ...) log_printf("%s %s (0x%02X): " s "\n", "ADPS", f, d->i2c_addr, ## __VA_ARGS__)
#else
#define DEBUG(s, f, ...)
#define DEBUG_DEV(s, f, d, ...)
#endif // APDS9960_DEBUG

#define ERROR(s, f, ...) log_printf("%s %s: " s "\n", "ADPS9960", f, ## __VA_ARGS__)


#define APDS_INIT_ATIME           219     // 103ms
#define APDS_INIT_WTIME           246     // 27ms
#define APDS_INIT_PROX_PPULSE     0x87    // 16us, 8 pulses
#define APDS_INIT_GESTURE_PPULSE  0x89    // 16us, 10 pulses
#define APDS_INIT_POFFSET_UR      0       // 0 offset
#define APDS_INIT_POFFSET_DL      0       // 0 offset      
#define APDS_INIT_CONFIG1         0x60    // No 12x wait (WTIME) factor
#define APDS_INIT_LDRIVE          LED_DRIVE_100MA
#define APDS_INIT_PGAIN           PGAIN_4X
#define APDS_INIT_AGAIN           AGAIN_4X
#define APDS_INIT_PILT            0       // Low proximity threshold
#define APDS_INIT_PIHT            50      // High proximity threshold
#define APDS_INIT_AILT            0xFFFF  // Force interrupt for calibration
#define APDS_INIT_AIHT            0
#define APDS_INIT_PERS            0x11    // 2 consecutive prox or ALS for int.
#define APDS_INIT_CONFIG2         0x01    // No satur interrupts or LED boost  
#define APDS_INIT_CONFIG3         0       // Enable all photodiodes, no SAI
#define APDS_INIT_GPENTH          40      // Threshold for entering gesture mode
#define APDS_INIT_GEXTH           30      // Threshold for exiting gesture mode    
#define APDS_INIT_GCONF1          0x40    // 4 events for int., 1 for exit
#define APDS_INIT_GGAIN           GGAIN_4X
#define APDS_INIT_GLDRIVE         LED_DRIVE_100MA
#define APDS_INIT_GWTIME          GWTIME_2_8MS
#define APDS_INIT_GOFFSET         0       // No offset scaling for gesture mode
#define APDS_INIT_GPULSE          0xC9    // 32us, 10 pulses
#define APDS_INIT_GCONF3          0       // All diodes active during gesture
#define APDS_INIT_GIEN            0       // Disable gesture interrupts

/*******************************************************************************
* Forward declarations of private functions
*******************************************************************************/

static int
log_printf(const char *p_format, ...);

static int
i2c_write(apds9960_t *p_apds, const uint8_t *p_buf, size_t buf_len);

static int
i2c_read(apds9960_t *p_apds, uint8_t* p_buf, size_t buf_len);

static int
i2c_write_read(apds9960_t *p_apds, const uint8_t *p_buf_write, size_t len_write,
    uint8_t *p_buf_read, size_t len_read);

static bool
reg_read(apds9960_t *p_apds, uint8_t reg_addr, uint8_t *p_data,
    uint32_t data_len);

static bool
reg_write(apds9960_t *p_apds, uint8_t reg_addr, const uint8_t *p_data,
    uint32_t data_len);

/*******************************************************************************
* Global variables
*******************************************************************************/


/*******************************************************************************
* Function definitions
*******************************************************************************/

apds9960_t 
*apds9960_open(int i2c_fd, I2C_DeviceAddress i2c_addr)
{
    apds9960_t *p_apds = NULL;
    bool is_init_ok = true;

    if ((p_apds = malloc(sizeof(apds9960_t))) == NULL)
    {
        // Cannot allocate memory for device descriptor
        is_init_ok = false;
        ERROR("Not enough free memory.", __FUNCTION__);
    }

    // Initialize device descriptor, check device ID
    if (is_init_ok)
    {
        p_apds->i2c_fd = i2c_fd;
        p_apds->i2c_addr = i2c_addr;

        // Check device hardware ID
        DEBUG_DEV("--- Checking hardware ID", __FUNCTION__, p_apds);
        uint8_t reg_byte;
        if (! reg_read(p_apds, APDS9960_ID, &reg_byte, 1))
        {
            is_init_ok = false;
            ERROR("Error reading device ID.", __FUNCTION__);
        }
        else 
        {
            DEBUG_DEV("Read value: 0x%02X", __FUNCTION__, p_apds, reg_byte);
            if (reg_byte != APDS9960_DEVICE_ID)
            {
                is_init_ok = false;
                ERROR("Device ID does not match.", __FUNCTION__);
            }
        }
    }

    // Disable all functions, power off
    if (is_init_ok)
    {
        apds9960_enable_t reg_enable;
        reg_enable.byte = 0;
        is_init_ok = apds9960_set_enable(p_apds, reg_enable);
    }

    // Reload initial values
    uint8_t reg_byte;

    if (is_init_ok)
    {
        // Gesture Enter Threshold
        reg_byte = APDS_INIT_GPENTH;
        is_init_ok = reg_write(p_apds, APDS9960_GPENTH, &reg_byte, 1);
    }

    if (is_init_ok)
    {
        // Gesture Exit Threshold
        reg_byte = APDS_INIT_GEXTH;
        is_init_ok = reg_write(p_apds, APDS9960_GEXTH, &reg_byte, 1);
    }

    if (is_init_ok)
    {
        // 
        reg_byte = APDS_INIT_GCONF1;
        is_init_ok = reg_write(p_apds, APDS9960_GCONF1, &reg_byte, 1);
    }

    return p_apds;
}

void
apds9960_close(apds9960_t *p_apds)
{
    // Disable sensor
    apds9960_enable_t reg_enable;
    reg_enable.byte = 0;
    apds9960_set_enable(p_apds, reg_enable);
    
    // Free allocated memory
    free(p_apds);
}

bool
apds9960_set_enable(apds9960_t *p_apds, apds9960_enable_t reg_enable)
{
    bool result = reg_write(p_apds, APDS9960_ENABLE, &reg_enable.byte, 1);
    if (!result)
    {
        ERROR("Error writing ENABLE register.", __FUNCTION__);
    }
    return result;
}

apds9960_enable_t
apds9960_get_enable(apds9960_t *p_apds)
{
    apds9960_enable_t result;

    if (!reg_read(p_apds, APDS9960_ENABLE, &result.byte, 1))
    {
        ERROR("Error reading ENABLE register.", __FUNCTION__);
        result.byte = 0xFF;
    }
    return result;
}

bool
apds9960_set_gpenth(apds9960_t *p_apds, uint8_t threshold)
{
    bool result = reg_write(p_apds, APDS9960_GPENTH, &threshold, 1);
    if (!result)
    {
        ERROR("Error writing GPENTH register.", __FUNCTION__);
    }
    return result;
}

bool
apds9960_set_atime(apds9960_t *p_apds, uint16_t milliseconds)
{
    return false;
}

/*******************************************************************************
* Private function definitions
*******************************************************************************/

static int
log_printf(const char *p_format, ...)
{
    va_list args;

    va_start(args, p_format);
    int result = Log_DebugVarArgs(p_format, args);
    va_end(args);

    return result;
}

static int
i2c_write(apds9960_t *p_apds, const uint8_t *p_buf, size_t buf_len)
{
    ssize_t result;

    result = I2CMaster_Write(p_apds->i2c_fd, p_apds->i2c_addr, p_buf, buf_len);

    if (result == -1)
    {
        DEBUG_DEV("Error %d (%s) on writing %d byte(s) to I2C addr 0x%02X",
            __FUNCTION__, p_apds, errno, strerror(errno), buf_len,
            p_apds->i2c_addr);
    }

    return result;
}

static int
i2c_read(apds9960_t *p_apds, uint8_t* p_buf, size_t buf_len)
{
    ssize_t result;

    result = I2CMaster_Read(p_apds->i2c_fd, p_apds->i2c_addr, p_buf, buf_len);

    if (result == -1)
    {
        DEBUG_DEV("Error %d (%s) on reading %d byte(s) from I2C addr 0x%02X",
            __FUNCTION__, p_apds, errno, strerror(errno), buf_len, 
            p_apds->i2c_addr);
    }

    return result;
}

static int
i2c_write_read(apds9960_t *p_apds, const uint8_t *p_buf_write, size_t len_write,
    uint8_t *p_buf_read, size_t len_read)
{
    ssize_t result = I2CMaster_WriteThenRead(p_apds->i2c_fd, p_apds->i2c_addr,
        p_buf_write, len_write, p_buf_read, len_read);

    if (result == -1)
    {
        DEBUG_DEV("Error %d (%s) on I2C WR operation at addr 0x%02X",
            __FUNCTION__, p_apds, errno, strerror(errno), p_apds->i2c_addr);
    }

    return result;
}

static bool
reg_read(apds9960_t *p_apds, uint8_t reg_addr, uint8_t *p_data, 
    uint32_t data_len)
{
    bool result = false;

    if (p_apds && p_data)
    {
        DEBUG_DEV(" REG READ [%02X] bytes %d", __FUNCTION__, p_apds, reg_addr,
            data_len);

        ssize_t i2c_result;

        // Select register and read its data
        i2c_result = i2c_write_read(p_apds, &reg_addr, 1, p_data, data_len);

        if (i2c_result != -1)
        {
            result = true;
#			ifdef APDS9960_DEBUG
            log_printf("APDS %s (0x%02X):  READ ", 
                __FUNCTION__, p_apds->i2c_addr);
            for (int i = 0; i < data_len; i++) {
                log_printf("%02X ", p_data[i]);
            }
            log_printf("\n");
#			endif
        }
    }
    return result;
}

static bool
reg_write(apds9960_t *p_apds, uint8_t reg_addr, const uint8_t *p_data,
    uint32_t data_len)
{
    bool result = false;

    if (p_apds && p_data)
    {
        DEBUG_DEV(" REG WRITE [%02X] bytes %d", __FUNCTION__, p_apds, reg_addr,
            data_len);

        uint8_t buffer[data_len + 1];

        buffer[0] = reg_addr;
        for (uint32_t i = 0; i < data_len; i++) {
            buffer[i + 1] = p_data[i];
        }

#		ifdef APDS9960_DEBUG
        log_printf("APDS %s (0x%02X): WRITE ",
            __FUNCTION__, p_apds->i2c_addr);
        for (int i = 0; i < data_len; i++) {
            log_printf("%02X ", p_data[i]);
        }
        log_printf("\n");
#		endif

        // Select register and write data
        ssize_t i2c_result = i2c_write(p_apds, buffer, data_len + 1);

        if (i2c_result != -1)
        {
            result = true;
        }
    }
    return result;
}


/* [] END OF FILE */
