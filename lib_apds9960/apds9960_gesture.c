
#include <stdbool.h>

#include "lib_apds9960.h"
#include "apds9960_common.h"

/*******************************************************************************
* Forward declarations of private functions
*******************************************************************************/

static void
gesture_reset_params(apds9960_t *p_apds);

/*******************************************************************************
* Global variables
*******************************************************************************/


/*******************************************************************************
* Public function definitions
*******************************************************************************/

bool
apds9960_gesture_enable(apds9960_t *p_apds, bool b_is_interrupt_enabled)
{
    uint8_t reg_byte;
    bool b_is_all_ok = false;

    gesture_reset_params(p_apds);

    // WTIME
    reg_byte = 0xFF;
    b_is_all_ok = reg_write8(p_apds, APDS9960_WTIME, &reg_byte);



    /*
    // Set CONTROL register
    // -- AGAIN: init value
    apds9960_control_t reg_control;
    b_is_all_ok = reg_read(p_apds, APDS9960_CONTROL, &reg_control.byte, 1);
    if (b_is_all_ok)
    {
        reg_control.AGAIN = APDS_INIT_AGAIN;
        b_is_all_ok = reg_write(p_apds, APDS9960_CONTROL, &reg_control.byte, 1);
    }

    // Set ENABLE register
    // -- PON: Power On
    // -- AEN: ALS Enable
    // -- AIEN: b_is_interrupt_enabled
    if (b_is_all_ok)
    {
        apds9960_enable_t reg_enable;
        b_is_all_ok = reg_read(p_apds, APDS9960_ENABLE, &reg_enable.byte, 1);
        if (b_is_all_ok)
        {
            reg_enable.PON = 1;
            reg_enable.AEN = 1;
            reg_enable.AIEN = b_is_interrupt_enabled;
            b_is_all_ok = reg_write(p_apds, APDS9960_ENABLE, &reg_enable.byte, 1);
        }
    }

    if (!b_is_all_ok)
    {
        ERROR("Error enabling ALS.", __FUNCTION__);
    }
    */
    return b_is_all_ok;
}


/*******************************************************************************
* Private function definitions
*******************************************************************************/

static void
gesture_reset_params(apds9960_t *p_apds)
{
    p_apds->gesture_data.index = 0;
    p_apds->gesture_data.total_gestures = 0;
    p_apds->gesture_delta.lr = 0;
    p_apds->gesture_delta.ud = 0;
    p_apds->gesture_count.lr = 0;
    p_apds->gesture_count.ud = 0;
    p_apds->gesture_count.near = 0;
    p_apds->gesture_count.far = 0;
    p_apds->gesture_state = 0;
    p_apds->gesture_motion = GESTURE_DIR_NONE;
}


/* [] END OF FILE */
