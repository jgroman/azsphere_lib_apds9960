
#include <stdbool.h>

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

bool
apds9960_als_enable(apds9960_t *p_apds, bool b_is_interrupt_enabled)
{
    bool b_is_all_ok = false;

    // Set CONTROL register
    // -- AGAIN: init value
    apds9960_control_t reg_control;
    b_is_all_ok = reg_read8(p_apds, APDS9960_CONTROL, &reg_control.byte);
    if (b_is_all_ok)
    {
        reg_control.AGAIN = APDS_INIT_AGAIN;
        b_is_all_ok = reg_write8(p_apds, APDS9960_CONTROL, &reg_control.byte);
    }

    // Set ENABLE register
    // -- PON: Power On
    // -- AEN: ALS Enable
    // -- AIEN: b_is_interrupt_enabled
    if (b_is_all_ok)
    {
        apds9960_enable_t reg_enable;
        b_is_all_ok = reg_read8(p_apds, APDS9960_ENABLE, &reg_enable.byte);
        if (b_is_all_ok)
        {
            reg_enable.PON = 1;
            reg_enable.AEN = 1;
            reg_enable.AIEN = b_is_interrupt_enabled;
            b_is_all_ok = reg_write8(p_apds, APDS9960_ENABLE, &reg_enable.byte);
        }
    }

    if (!b_is_all_ok)
    {
        ERROR("Error enabling ALS.", __FUNCTION__);
    }

    return b_is_all_ok;
}

bool
apds9960_als_disable(apds9960_t *p_apds)
{
    // Set ENABLE register
    // -- AEN: ALS Disable
    // -- AIEN: ALS Interrupt Disable
    apds9960_enable_t reg_enable;
    bool b_is_all_ok = reg_read8(p_apds, APDS9960_ENABLE, &reg_enable.byte);

    if (b_is_all_ok)
    {
        reg_enable.AEN = 0;
        reg_enable.AIEN = 0;
        b_is_all_ok = reg_write8(p_apds, APDS9960_ENABLE, &reg_enable.byte);
    }

    if (!b_is_all_ok)
    {
        ERROR("Error disabling ALS.", __FUNCTION__);
    }

    return b_is_all_ok;
}

bool
apds9960_als_read_clear(apds9960_t *p_apds, uint16_t *value_clear)
{
    bool b_is_all_ok = false;
    uint8_t reg_value;

    *value_clear = 0;

    b_is_all_ok = reg_read8(p_apds, APDS9960_CDATAH, &reg_value);
    if (b_is_all_ok)
    {
        *value_clear = (uint16_t)(reg_value << 8);
        b_is_all_ok = reg_read8(p_apds, APDS9960_CDATAL, &reg_value);
        if (b_is_all_ok)
        {
            *value_clear = (uint16_t)(*value_clear | reg_value);
        }
    }

    if (!b_is_all_ok)
    {
        ERROR("Error reading ALS clear light level.", __FUNCTION__);
    }

    return b_is_all_ok;
}

bool
apds9960_als_read_red(apds9960_t *p_apds, uint16_t *value_red)
{
    bool b_is_all_ok = false;
    uint8_t reg_value;

    *value_red = 0;

    b_is_all_ok = reg_read8(p_apds, APDS9960_RDATAH, &reg_value);
    if (b_is_all_ok)
    {
        *value_red = (uint16_t)(reg_value << 8);
        b_is_all_ok = reg_read8(p_apds, APDS9960_RDATAL, &reg_value);
        if (b_is_all_ok)
        {
            *value_red = (uint16_t)(*value_red | reg_value);
        }
    }

    if (!b_is_all_ok)
    {
        ERROR("Error reading ALS red light level.", __FUNCTION__);
    }

    return b_is_all_ok;
}

bool
apds9960_als_read_green(apds9960_t *p_apds, uint16_t *value_green)
{
    bool b_is_all_ok = false;
    uint8_t reg_value;

    *value_green = 0;

    b_is_all_ok = reg_read8(p_apds, APDS9960_GDATAH, &reg_value);
    if (b_is_all_ok)
    {
        *value_green = (uint16_t)(reg_value << 8);
        b_is_all_ok = reg_read8(p_apds, APDS9960_GDATAL, &reg_value);
        if (b_is_all_ok)
        {
            *value_green = (uint16_t)(*value_green | reg_value);
        }
    }

    if (!b_is_all_ok)
    {
        ERROR("Error reading ALS green light level.", __FUNCTION__);
    }

    return b_is_all_ok;
}

bool
apds9960_als_read_blue(apds9960_t *p_apds, uint16_t *value_blue)
{
    bool b_is_all_ok = false;
    uint8_t reg_value;

    *value_blue = 0;

    b_is_all_ok = reg_read8(p_apds, APDS9960_BDATAH, &reg_value);
    if (b_is_all_ok)
    {
        *value_blue = (uint16_t)(reg_value << 8);
        b_is_all_ok = reg_read8(p_apds, APDS9960_BDATAL, &reg_value);
        if (b_is_all_ok)
        {
            *value_blue = (uint16_t)(*value_blue | reg_value);
        }
    }

    if (!b_is_all_ok)
    {
        ERROR("Error reading ALS blue light level.", __FUNCTION__);
    }

    return b_is_all_ok;
}


/*******************************************************************************
* Private function definitions
*******************************************************************************/


/* [] END OF FILE */
