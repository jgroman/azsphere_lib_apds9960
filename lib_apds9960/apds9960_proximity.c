
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
apds9960_proximity_enable(apds9960_t *p_apds, bool b_is_interrupt_enabled)
{
    bool b_is_all_ok = false;

    // Set CONTROL register
    // -- PGAIN: init value
    // -- LDRIVE: init value
    apds9960_control_t reg_control;
    b_is_all_ok = reg_read8(p_apds, APDS9960_CONTROL, &reg_control.byte);
    if (b_is_all_ok)
    {
        reg_control.PGAIN = APDS_INIT_PGAIN;
        reg_control.LDRIVE = APDS_INIT_LDRIVE;
        b_is_all_ok = reg_write8(p_apds, APDS9960_CONTROL, &reg_control.byte);
    }

    // Set ENABLE register
    // -- PON: Power On
    // -- PEN: Proximity Detect Enable
    // -- PIEN: b_is_interrupt_enabled
    if (b_is_all_ok)
    {
        apds9960_enable_t reg_enable;
        b_is_all_ok = reg_read8(p_apds, APDS9960_ENABLE, &reg_enable.byte);
        if (b_is_all_ok)
        {
            reg_enable.PON = 1;
            reg_enable.PEN = 1;
            reg_enable.PIEN = b_is_interrupt_enabled;
            b_is_all_ok = reg_write8(p_apds, APDS9960_ENABLE, &reg_enable.byte);
        }
    }

    if (!b_is_all_ok)
    {
        ERROR("Error enabling proximity.", __FUNCTION__);
    }

    return b_is_all_ok;
}

bool
apds9960_proximity_disable(apds9960_t *p_apds)
{
    // Set ENABLE register
    // -- PEN: Proximity Disable
    // -- PIEN: Proximity Interrupt Disable
    apds9960_enable_t reg_enable;
    bool b_is_all_ok = reg_read8(p_apds, APDS9960_ENABLE, &reg_enable.byte);

    if (b_is_all_ok)
    {
        reg_enable.PEN = 0;
        reg_enable.PIEN = 0;
        b_is_all_ok = reg_write8(p_apds, APDS9960_ENABLE, &reg_enable.byte);
    }

    if (!b_is_all_ok)
    {
        ERROR("Error disabling proximity.", __FUNCTION__);
    }

    return b_is_all_ok;
}

bool
apds9960_proximity_read(apds9960_t *p_apds, uint8_t *p_value_proximity)
{
    *p_value_proximity = 0;
    return reg_read8(p_apds, APDS9960_PDATA, p_value_proximity);
}


/*******************************************************************************
* Private function definitions
*******************************************************************************/


/* [] END OF FILE */
