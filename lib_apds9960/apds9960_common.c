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
#   	ifdef APDS9960_I2C_DEBUG
        DEBUG_DEV(" REG READ [%02X] bytes %d", __FUNCTION__, p_apds, reg_addr,
            data_len);
#       endif

        // Select register and read its data
        result = I2CMaster_WriteThenRead(p_apds->i2c_fd, p_apds->i2c_addr,
            &reg_addr, 1, p_data, data_len);

        if (result == -1)
        {
#   	    ifdef APDS9960_I2C_DEBUG
            DEBUG_DEV("Error %d (%s) on I2C WR operation at addr 0x%02X",
                __FUNCTION__, p_apds, errno, strerror(errno), p_apds->i2c_addr);
#           endif
        }
        else
        {
    #   	ifdef APDS9960_I2C_DEBUG
            log_printf("APDS %s (0x%02X):  READ ",
                __FUNCTION__, p_apds->i2c_addr);
            for (int i = 0; i < data_len; i++) {
                log_printf("%02X ", p_data[i]);
            }
            log_printf("\n");
#           endif
        }
    }
    
    // Return length of read data only
    return result - 1;
}

ssize_t
reg_write(apds9960_t *p_apds, uint8_t reg_addr, const uint8_t *p_data,
    uint32_t data_len)
{
    ssize_t result = -1;

    if (p_apds && p_data)
    {
#   	ifdef APDS9960_I2C_DEBUG
        DEBUG_DEV(" REG WRITE [%02X] bytes %d", __FUNCTION__, p_apds, reg_addr,
            data_len);
#       endif

        uint8_t buffer[data_len + 1];

        buffer[0] = reg_addr;
        for (uint32_t i = 0; i < data_len; i++) {
            buffer[i + 1] = p_data[i];
        }

#		ifdef APDS9960_I2C_DEBUG
        log_printf("APDS %s (0x%02X):  WRITE ",
            __FUNCTION__, p_apds->i2c_addr);
        for (int i = 0; i < data_len; i++) {
            log_printf("%02X ", p_data[i]);
        }
        log_printf("\n");
#		endif

        // Select register and write data
        result = I2CMaster_Write(p_apds->i2c_fd, p_apds->i2c_addr, buffer, 
            data_len + 1);

        if (result == -1)
        {
#		    ifdef APDS9960_I2C_DEBUG
            DEBUG_DEV("Error %d (%s) on writing %d byte(s) to I2C addr 0x%02X",
                __FUNCTION__, p_apds, errno, strerror(errno), data_len + 1,
                p_apds->i2c_addr);
#           endif
        }
    }

    return result;
}

/*******************************************************************************
* Private function definitions
*******************************************************************************/

/* [] END OF FILE */
