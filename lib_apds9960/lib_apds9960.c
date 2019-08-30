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
        if (! reg_read8(p_apds, APDS9960_ID, &reg_byte))
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

    // Reload initial values
    uint8_t reg_byte;

    if (is_init_ok)
    {
        // ENABLE: Disable all functions, power off
        reg_byte = 0;
        is_init_ok = reg_write8(p_apds, APDS9960_ENABLE, &reg_byte);
    }

    if (is_init_ok)
    {
        // ATIME: 103 ms
        reg_byte = APDS_INIT_ATIME;
        is_init_ok = reg_write8(p_apds, APDS9960_ATIME, &reg_byte);
    }

    if (is_init_ok)
    {
        // WTIME: 27 ms
        reg_byte = APDS_INIT_WTIME;
        is_init_ok = reg_write8(p_apds, APDS9960_WTIME, &reg_byte);
    }

    if (is_init_ok)
    {
        // PPULSE: 16us, 8 pulses, optimized for proximity engine
        reg_byte = APDS_INIT_PPULSE_PROX;
        is_init_ok = reg_write8(p_apds, APDS9960_PPULSE, &reg_byte);
    }

    if (is_init_ok)
    {
        // POFFSET_UR: 0
        reg_byte = APDS_INIT_POFFSET_UR;
        is_init_ok = reg_write8(p_apds, APDS9960_POFFSET_UR, &reg_byte);
    }

    if (is_init_ok)
    {
        // POFFSET_DL: 0
        reg_byte = APDS_INIT_POFFSET_DL;
        is_init_ok = reg_write8(p_apds, APDS9960_POFFSET_DL, &reg_byte);
    }

    if (is_init_ok)
    {
        // CONFIG1: No 12x wait (WTIME) factor
        reg_byte = APDS_INIT_CONFIG1;
        is_init_ok = reg_write8(p_apds, APDS9960_CONFIG1, &reg_byte);
    }

    if (is_init_ok)
    {
        // CONTROL: 
        // -- LDRIVE: LED Current 100 mA
        // -- PGAIN: 4x
        // -- AGAIN: 4x
        apds9960_control_t reg_control;
        is_init_ok = reg_read8(p_apds, APDS9960_CONTROL, &reg_control.byte);
        if (is_init_ok)
        {
            reg_control.LDRIVE = APDS_INIT_LDRIVE;
            reg_control.PGAIN = APDS_INIT_PGAIN;
            reg_control.AGAIN = APDS_INIT_AGAIN;
            is_init_ok = reg_write8(p_apds, APDS9960_CONTROL, &reg_control.byte);
        }
    }

    if (is_init_ok)
    {
        // PILT: 0
        reg_byte = APDS_INIT_PILT;
        is_init_ok = reg_write8(p_apds, APDS9960_PILT, &reg_byte);
    }

    if (is_init_ok)
    {
        // PIHT: 50
        reg_byte = APDS_INIT_PIHT;
        is_init_ok = reg_write8(p_apds, APDS9960_PIHT, &reg_byte);
    }

    if (is_init_ok)
    {
        // AILT: 0xFFFF
        reg_byte = (uint8_t) (APDS_INIT_AILT & 0xFF);
        is_init_ok = reg_write8(p_apds, APDS9960_AILTL, &reg_byte);
        if (is_init_ok)
        {
            reg_byte = (uint8_t) (APDS_INIT_AILT >> 8);
            is_init_ok = reg_write8(p_apds, APDS9960_AILTH, &reg_byte);
        }
    }

    if (is_init_ok)
    {
        // AIHT: 0
        reg_byte = (uint8_t) (APDS_INIT_AIHT & 0xFF);
        is_init_ok = reg_write8(p_apds, APDS9960_AIHTL, &reg_byte);
        if (is_init_ok)
        {
            reg_byte = (uint8_t)(APDS_INIT_AIHT >> 8);
            is_init_ok = reg_write8(p_apds, APDS9960_AIHTH, &reg_byte);
        }
    }

    if (is_init_ok)
    {
        // PERS: 2 consecutive prox or ALS for interrupt
        reg_byte = APDS_INIT_PERS;
        is_init_ok = reg_write8(p_apds, APDS9960_PERS, &reg_byte);
    }

    if (is_init_ok)
    {
        // CONFIG2: No saturation interrupts or LED boost
        reg_byte = APDS_INIT_CONFIG2;
        is_init_ok = reg_write8(p_apds, APDS9960_CONFIG2, &reg_byte);
    }

    if (is_init_ok)
    {
        // CONFIG3: Enable all photodiodes, no SAI
        reg_byte = APDS_INIT_CONFIG3;
        is_init_ok = reg_write8(p_apds, APDS9960_CONFIG3, &reg_byte);
    }

    if (is_init_ok)
    {
        // GPENTH: 40
        reg_byte = APDS_INIT_GPENTH;
        is_init_ok = reg_write8(p_apds, APDS9960_GPENTH, &reg_byte);
    }

    if (is_init_ok)
    {
        // GEXTH: 30
        reg_byte = APDS_INIT_GEXTH;
        is_init_ok = reg_write8(p_apds, APDS9960_GEXTH, &reg_byte);
    }

    if (is_init_ok)
    {
        // GCONF1: 4 events for interrupt, 1 for exit
        reg_byte = APDS_INIT_GCONF1;
        is_init_ok = reg_write8(p_apds, APDS9960_GCONF1, &reg_byte);
    }

    if (is_init_ok)
    {
        // GCONF2:
        // -- GGAIN: 4x
        // -- GLDRIVE: 100 mA
        // -- GWTIME: 2.8 ms
        apds9960_gconf2_t reg_gconf2;
        is_init_ok = reg_read8(p_apds, APDS9960_GCONF2, &reg_gconf2.byte);
        if (is_init_ok)
        {
            reg_gconf2.GGAIN = APDS_INIT_GGAIN;
            reg_gconf2.GLDRIVE = APDS_INIT_GLDRIVE;
            reg_gconf2.GWTIME = APDS_INIT_GWTIME;
            is_init_ok = reg_write8(p_apds, APDS9960_GCONF2, &reg_gconf2.byte);
        }
    }

    if (is_init_ok)
    {
        // GOFFSET_U, GOFFSET_D, GOFFSET_L, GOFFSET_R: 0
        reg_byte = APDS_INIT_GOFFSET;
        is_init_ok = reg_write8(p_apds, APDS9960_GOFFSET_U, &reg_byte);

        if (is_init_ok)
        {
            is_init_ok = reg_write8(p_apds, APDS9960_GOFFSET_D, &reg_byte);
        }

        if (is_init_ok)
        {
            is_init_ok = reg_write8(p_apds, APDS9960_GOFFSET_L, &reg_byte);
        }

        if (is_init_ok)
        {
            is_init_ok = reg_write8(p_apds, APDS9960_GOFFSET_R, &reg_byte);
        }
    }

    if (is_init_ok)
    {
        // GPULSE: 32 us, 10 pulses
        reg_byte = APDS_INIT_GPULSE;
        is_init_ok = reg_write8(p_apds, APDS9960_GPULSE, &reg_byte);
    }

    if (is_init_ok)
    {
        // GCONF3: All diodes active during gesture
        reg_byte = APDS_INIT_GCONF3;
        is_init_ok = reg_write8(p_apds, APDS9960_GCONF3, &reg_byte);
    }

    if (is_init_ok)
    {
        // GCONF4:
        // -- GIEN: 0
        apds9960_gconf4_t reg_gconf4;
        is_init_ok = reg_read8(p_apds, APDS9960_GCONF4, &reg_gconf4.byte);
        if (is_init_ok)
        {
            reg_gconf4.GIEN = APDS_INIT_GIEN;
            is_init_ok = reg_write8(p_apds, APDS9960_GCONF4, &reg_gconf4.byte);
        }
    }

    if (!is_init_ok)
    {
        ERROR("APDS9960 initialization failed.", __FUNCTION__);
        free(p_apds);
        p_apds = NULL;
    }

    return p_apds;
}

void
apds9960_close(apds9960_t *p_apds)
{
    // Disable sensor functions, power off

    // Set CONTROL register to 0
    apds9960_enable_t reg_enable;
    reg_enable.byte = 0;
    if (! reg_write8(p_apds, APDS9960_ENABLE, &reg_enable.byte))
    {
        ERROR("Error powering off APDS9960.", __FUNCTION__);
    }

    // Free allocated memory
    free(p_apds);
}

/*******************************************************************************
* Private function definitions
*******************************************************************************/

/* [] END OF FILE */
