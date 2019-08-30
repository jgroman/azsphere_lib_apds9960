
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

#include "lib_apds9960.h"
#include "apds9960_common.h"

/*******************************************************************************
* Forward declarations of private functions
*******************************************************************************/

static void
gesture_reset_params(apds9960_t *p_apds);

static bool
gesture_process_data(apds9960_t *p_apds);

static bool
gesture_decode(apds9960_t *p_apds);

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

    if (b_is_all_ok)
    {
        // PPULSE
        reg_byte = APDS_INIT_PPULSE_GEST;
        b_is_all_ok = reg_write8(p_apds, APDS9960_PPULSE, &reg_byte);
    }

    if (b_is_all_ok)
    {
        // CONFIG2
        // -- LED_BOOST: 300 mA
        apds9960_config2_t reg_config2;
        b_is_all_ok = reg_read8(p_apds, APDS9960_CONFIG2, &reg_config2.byte);

        if (b_is_all_ok)
        {
            reg_config2.LED_BOOST = CONFIG2_LED_BOOST_300;
            b_is_all_ok = reg_write8(p_apds, APDS9960_CONFIG2, &reg_config2.byte);
        }
    }

    if (b_is_all_ok)
    {
        // GCONF4
        // -- GMODE: Gesture Mode Enabled
        // -- GIEN: b_is_interrupt_enabled
        apds9960_gconf4_t reg_gconf4;
        b_is_all_ok = reg_read8(p_apds, APDS9960_GCONF4, &reg_gconf4.byte);

        if (b_is_all_ok)
        {
            reg_gconf4.GMODE = 1;
            reg_gconf4.GIEN = b_is_interrupt_enabled;
            b_is_all_ok = reg_write8(p_apds, APDS9960_GCONF4, &reg_gconf4.byte);
        }
    }

    if (b_is_all_ok)
    {
        // ENABLE
        // -- PON: Power On
        // -- GEN: Gesture Enable
        // -- WEN: Wait Enable
        // -- PEN: Proximity Enable
        apds9960_enable_t reg_enable;
        b_is_all_ok = reg_read8(p_apds, APDS9960_ENABLE, &reg_enable.byte);
        if (b_is_all_ok)
        {
            reg_enable.PON = 1;
            reg_enable.GEN = 1;
            reg_enable.WEN = 1;
            reg_enable.PEN = 1;
            b_is_all_ok = reg_write8(p_apds, APDS9960_ENABLE, &reg_enable.byte);
        }
    }

    if (!b_is_all_ok)
    {
        ERROR("Error enabling Gesture sensor.", __FUNCTION__);
    }

    return b_is_all_ok;
}

bool
apds9960_gesture_disable(apds9960_t *p_apds)
{
    uint8_t reg_byte;
    bool b_is_all_ok = false;

    gesture_reset_params(p_apds);

    // GCONF4
    // -- GMODE: Gesture Mode Disabled
    // -- GIEN: Interrupt Disabled
    apds9960_gconf4_t reg_gconf4;
    b_is_all_ok = reg_read8(p_apds, APDS9960_GCONF4, &reg_gconf4.byte);
    if (b_is_all_ok)
    {
        reg_gconf4.GMODE = 0;
        reg_gconf4.GIEN = 0;
        b_is_all_ok = reg_write8(p_apds, APDS9960_GCONF4, &reg_gconf4.byte);
    }

    if (b_is_all_ok)
    {
        // ENABLE
        // -- GEN: Gesture Disable
        apds9960_enable_t reg_enable;
        b_is_all_ok = reg_read8(p_apds, APDS9960_ENABLE, &reg_enable.byte);
        if (b_is_all_ok)
        {
            reg_enable.GEN = 0;
            b_is_all_ok = reg_write8(p_apds, APDS9960_ENABLE, &reg_enable.byte);
        }
    }

    if (!b_is_all_ok)
    {
        ERROR("Error disabling Gesture sensor.", __FUNCTION__);
    }

    return b_is_all_ok;
}

bool
apds9960_gesture_is_available(apds9960_t *p_apds, bool *p_value)
{
    apds9960_gstatus_t reg_gstatus;
    bool b_is_all_ok = reg_read8(p_apds, APDS9960_GSTATUS, &reg_gstatus.byte);

    if (!b_is_all_ok)
    {
        ERROR("Error getting Gesture availability.", __FUNCTION__);
    }

    *p_value = reg_gstatus.GVALID;
    return b_is_all_ok;
}

int
apds9960_gesture_read(apds9960_t *p_apds)
{
    uint8_t fifo_level = 0;
    uint8_t bytes_read = 0;
    uint8_t fifo_data[128];
    uint8_t gstatus;
    int result = -1;
    int i;

    apds9960_enable_t reg_enable;
    apds9960_gstatus_t reg_gstatus;


    return result;
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

static bool
gesture_process_data(apds9960_t *p_apds)
{
    uint8_t u_first = 0;
    uint8_t d_first = 0;
    uint8_t l_first = 0;
    uint8_t r_first = 0;
    uint8_t u_last = 0;
    uint8_t d_last = 0;
    uint8_t l_last = 0;
    uint8_t r_last = 0;
    int ud_ratio_first;
    int lr_ratio_first;
    int ud_ratio_last;
    int lr_ratio_last;
    int ud_delta;
    int lr_delta;
    int i;
}

static bool
gesture_decode(apds9960_t *p_apds)
{
    bool b_is_decoded = false;

    if (p_apds->gesture_state == GESTURE_STATE_NEAR)
    {
        p_apds->gesture_motion = GESTURE_DIR_NEAR;
        b_is_decoded = true;
    }
    else if (p_apds->gesture_state == GESTURE_STATE_FAR)
    {
        p_apds->gesture_motion = GESTURE_DIR_FAR;
        b_is_decoded = true;
    }

    if (!b_is_decoded)
    {
        b_is_decoded = true;

        if ((p_apds->gesture_count.ud == -1) && 
            (p_apds->gesture_count.lr == 0))
        {
            p_apds->gesture_motion = GESTURE_DIR_UP;
        }
        else if ((p_apds->gesture_count.ud == 1) &&
            (p_apds->gesture_count.lr == 0))
        {
            p_apds->gesture_motion = GESTURE_DIR_DOWN;
        }
        else if ((p_apds->gesture_count.ud == 0) &&
            (p_apds->gesture_count.lr == 1))
        {
            p_apds->gesture_motion = GESTURE_DIR_RIGHT;
        }
        else if ((p_apds->gesture_count.ud == 0) &&
            (p_apds->gesture_count.lr == -1))
        {
            p_apds->gesture_motion = GESTURE_DIR_LEFT;
        }
        else if ((p_apds->gesture_count.ud == -1) &&
            (p_apds->gesture_count.lr == 1))
        {
            if (abs(p_apds->gesture_delta.ud) > abs(p_apds->gesture_delta.lr))
            {
                p_apds->gesture_motion = GESTURE_DIR_UP;
            }
            else
            {
                p_apds->gesture_motion = GESTURE_DIR_RIGHT;
            }
        }
        else if ((p_apds->gesture_count.ud == 1) &&
            (p_apds->gesture_count.lr == -1))
        {
            if (abs(p_apds->gesture_delta.ud) > abs(p_apds->gesture_delta.lr))
            {
                p_apds->gesture_motion = GESTURE_DIR_DOWN;
            }
            else
            {
                p_apds->gesture_motion = GESTURE_DIR_LEFT;
            }
        }
        else if ((p_apds->gesture_count.ud == -1) &&
            (p_apds->gesture_count.lr == -1))
        {
            if (abs(p_apds->gesture_delta.ud) > abs(p_apds->gesture_delta.lr))
            {
                p_apds->gesture_motion = GESTURE_DIR_UP;
            }
            else
            {
                p_apds->gesture_motion = GESTURE_DIR_LEFT;
            }
        }
        else if ((p_apds->gesture_count.ud == 1) &&
            (p_apds->gesture_count.lr == 1))
        {
            if (abs(p_apds->gesture_delta.ud) > abs(p_apds->gesture_delta.lr))
            {
                p_apds->gesture_motion = GESTURE_DIR_DOWN;
            }
            else
            {
                p_apds->gesture_motion = GESTURE_DIR_RIGHT;
            }
        }
        else
        {
            b_is_decoded = false;
        }
    }

    return b_is_decoded;
}

/* [] END OF FILE */
