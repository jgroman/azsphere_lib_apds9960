#include <stdbool.h>
#include <errno.h>
#include <string.h>

#include <applibs/log.h>
#include <applibs/i2c.h>

#include "lib_apds9960.h"
#include "apds9960_common.h"

/*******************************************************************************
* Forward declarations of private functions
*******************************************************************************/

static ssize_t
i2c_write(apds9960_t *p_apds, const uint8_t *p_buf, size_t buf_len);

static ssize_t
i2c_write_read(apds9960_t *p_apds, const uint8_t *p_buf_write, size_t len_write,
    uint8_t *p_buf_read, size_t len_read);

/*******************************************************************************
* Global variables
*******************************************************************************/


/*******************************************************************************
* Public function definitions
*******************************************************************************/

int
log_printf(const char *p_format, ...)
{
    va_list args;

    va_start(args, p_format);
    int result = Log_DebugVarArgs(p_format, args);
    va_end(args);

    return result;
}

bool
reg_read8(apds9960_t *p_apds, uint8_t reg_addr, uint8_t *p_data)
{
    bool b_result = false;

    if (reg_read(p_apds, reg_addr, p_data, 1) != -1)
    {
        b_result = true;
    }

    return b_result;
}

bool
reg_write8(apds9960_t *p_apds, uint8_t reg_addr, const uint8_t *p_data)
{
    bool b_result = false;

    if (reg_write(p_apds, reg_addr, p_data, 1) != -1)
    {
        b_result = true;
    }

    return b_result;
}

ssize_t
reg_read(apds9960_t *p_apds, uint8_t reg_addr, uint8_t *p_data,
    uint32_t data_len)
{
    ssize_t result = -1;

    if (p_apds && p_data)
    {
        DEBUG_DEV(" REG READ [%02X] bytes %d", __FUNCTION__, p_apds, reg_addr,
            data_len);

        // Select register and read its data
        result = i2c_write_read(p_apds, &reg_addr, 1, p_data, data_len);

#   	ifdef APDS9960_DEBUG
        if (result != -1)
        {
            log_printf("APDS %s (0x%02X):  READ ",
                __FUNCTION__, p_apds->i2c_addr);
            for (int i = 0; i < data_len; i++) {
                log_printf("%02X ", p_data[i]);
            }
            log_printf("\n");
        }
#       endif
    }
    return result;
}

ssize_t
reg_write(apds9960_t *p_apds, uint8_t reg_addr, const uint8_t *p_data,
    uint32_t data_len)
{
    ssize_t result = -1;

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
        log_printf("APDS %s (0x%02X):  WRITE ",
            __FUNCTION__, p_apds->i2c_addr);
        for (int i = 0; i < data_len; i++) {
            log_printf("%02X ", p_data[i]);
        }
        log_printf("\n");
#		endif

        // Select register and write data
        result = i2c_write(p_apds, buffer, data_len + 1);
    }

    return result;
}

/*******************************************************************************
* Private function definitions
*******************************************************************************/

static ssize_t
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

#if 0
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
#endif

static ssize_t
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

/* [] END OF FILE */
