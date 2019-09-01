
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "lib_apds9960.h"
#include "apds9960_common.h"

#define GESTURE_THOLD_OUT   10  // Gesture Out Threshold
#define GESTURE_SENS_1      50
#define GESTURE_SENS_2      20

#define FIFO_PAUSE_TIME_MS  30  // Wait period between FIFO reads

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

    // WTIME: Proximity wait time 2.78 ms
    reg_byte = 0xFF;
    b_is_all_ok = reg_write8(p_apds, APDS9960_WTIME, &reg_byte);

    if (b_is_all_ok)
    {
        // PPULSE: Proximity 16 us, 10 pulses
        reg_byte = APDS_INIT_PPULSE_GEST;
        b_is_all_ok = reg_write8(p_apds, APDS9960_PPULSE, &reg_byte);
    }

    if (b_is_all_ok)
    {
        // CONFIG2
        // -- LED_BOOST: 300 %
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
apds9960_gesture_is_valid(apds9960_t *p_apds, bool *p_value)
{
    apds9960_gstatus_t reg_gstatus;
    bool b_is_all_ok = reg_read8(p_apds, APDS9960_GSTATUS, &reg_gstatus.byte);

    if (!b_is_all_ok)
    {
        ERROR("Error reading Gesture validity.", __FUNCTION__);
    }

    *p_value = reg_gstatus.GVALID;
    return b_is_all_ok;
}

int
apds9960_gesture_read(apds9960_t *p_apds)
{
    const struct timespec FIFO_DELAY = { 0, FIFO_PAUSE_TIME_MS * 1000000 };

    uint8_t fifo_level = 0;
    ssize_t bytes_read = 0;
    uint8_t ds_buffer[128];
    int result = -1;
    int idx;

    bool b_is_all_ok = false;
    bool b_is_valid;            // Gesture is available

    apds9960_enable_t reg_enable;
    apds9960_gesture_data_t *p_gdata = &p_apds->gesture_data;

    // Make sure that power and gesture is on and gesture is available
    b_is_all_ok = reg_read8(p_apds, APDS9960_ENABLE, &reg_enable.byte);
    if (b_is_all_ok)
    {
        b_is_all_ok = apds9960_gesture_is_valid(p_apds, &b_is_valid);
        if (b_is_all_ok && (!b_is_valid || !reg_enable.PON || !reg_enable.GEN))
        {
            b_is_all_ok = false;
        }
    }

    // Endless loop as long as gestures are available
    while (b_is_all_ok)
    {
        // Wait for FIFO to fill up
        nanosleep(&FIFO_DELAY, NULL);

        // Get current gesture availability
        b_is_all_ok = apds9960_gesture_is_valid(p_apds, &b_is_valid);
        if (!b_is_all_ok)
        {
            // Cannot obtain gesture validity
            result = -1;
            break;
        }

        if (!b_is_valid)
        {
            // No more gestures available
            // Use accumulated data to decode gesture
            gesture_decode(p_apds);
            result = p_apds->gesture_motion;
            gesture_reset_params(p_apds);
            break;
        }
        else
        {
            // Get current FIFO level
            b_is_all_ok = reg_read8(p_apds, APDS9960_GFLVL, &fifo_level);

            if (!b_is_all_ok)
            {
                // Cannot get FIFO level
                result = -1;
                break;
            }

            // If there's data in the FIFO, copy datasets into buffer
            if (fifo_level > 0)
            {
                p_gdata->dset_count = 0;

                // Read FIFO
                for (idx = 0; idx < fifo_level; idx ++)
                {
                    // Seems that reading more than 8 bytes at a time from FIFO 
                    // produces erratic results
                    bytes_read = reg_read(p_apds, APDS9960_GFIFO_U, ds_buffer, 4);

                    if (bytes_read == -1)
                    {
                        ERROR("Cannot read FIFO data.", __FUNCTION__);
                        result = -1;
                        break;
                    }

                    // Sort datasets from FIFO into U/D/L/R
                    p_gdata->u[p_gdata->dset_count] = ds_buffer[0];
                    p_gdata->d[p_gdata->dset_count] = ds_buffer[1];
                    p_gdata->l[p_gdata->dset_count] = ds_buffer[2];
                    p_gdata->r[p_gdata->dset_count] = ds_buffer[3];

                    p_gdata->dset_count++;
                }

                // At this point p_gdata holds current gesture datasets
                // p_gdata->dset_count contains number of valid datasets

                // Filter and process gesture data
                if (gesture_process_data(p_apds))
                {
                    if (gesture_decode(p_apds))
                    {
                        // Process multi-gesture sequences here or quit
                        // at the first decoded valid gesture
#   	                    ifdef APDS9960_DEBUG
                        DEBUG("Multi gesture %d\n", __FUNCTION__, p_apds->gesture_motion);
#                           endif
                    }
                }
            }
        }
    }

    return result;
}


/*******************************************************************************
* Private function definitions
*******************************************************************************/

static void
gesture_reset_params(apds9960_t *p_apds)
{
    p_apds->gesture_data.dset_count = 0;
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
    uint8_t idx;

    // Shortcuts to Gesture data structs
    apds9960_gesture_data_t *p_gdata = &p_apds->gesture_data;
    apds9960_gesture_delta_t *p_gdelta = &p_apds->gesture_delta;
    apds9960_gesture_count_t *p_gcount = &p_apds->gesture_count;

    bool b_is_all_ok = false;

    DEBUG("Processing cycles %d", __FUNCTION__, p_gdata->dset_count);

# 	ifdef APDS9960_DEBUG
    log_printf("U: ");
    for (idx = 0; idx < p_gdata->dset_count; idx++)
    {
        log_printf("%02X ", p_gdata->u[idx]);
    }
    log_printf("\n");

    log_printf("D: ");
    for (idx = 0; idx < p_gdata->dset_count; idx++)
    {
        log_printf("%02X ", p_gdata->d[idx]);
    }
    log_printf("\n");

    log_printf("L: ");
    for (idx = 0; idx < p_gdata->dset_count; idx++)
    {
        log_printf("%02X ", p_gdata->l[idx]);
    }
    log_printf("\n");

    log_printf("R: ");
    for (idx = 0; idx < p_gdata->dset_count; idx++)
    {
        log_printf("%02X ", p_gdata->r[idx]);
    }
    log_printf("\n");
#   endif


    // At least 5 gesture integration cycles are required for processing
    // Check gesture data bounds
    if ((p_gdata->dset_count > 4) && (p_gdata->dset_count <= 32))
    {
        // Find the first sample where all UDLR values are above Out threshold
        for (idx = 0; idx < p_gdata->dset_count; idx++)
        {
            if ((p_gdata->u[idx] > GESTURE_THOLD_OUT) &&
                (p_gdata->d[idx] > GESTURE_THOLD_OUT) &&
                (p_gdata->l[idx] > GESTURE_THOLD_OUT) &&
                (p_gdata->r[idx] > GESTURE_THOLD_OUT))
            {
                u_first = p_gdata->u[idx];
                d_first = p_gdata->d[idx];
                l_first = p_gdata->l[idx];
                r_first = p_gdata->r[idx];
                break;
            }
        }

        // Check "first" data validity, all UDLR values must be nonzero
        if ((u_first != 0) && (d_first != 0) && 
            (l_first != 0) && (r_first != 0))
        {
            b_is_all_ok = true;

            // Find the last sample where all UDLR values are above Out threshold
            for (idx = (uint8_t)(p_gdata->dset_count - 1); idx >= 0; idx--)
            {
                if ((p_gdata->u[idx] > GESTURE_THOLD_OUT) &&
                    (p_gdata->d[idx] > GESTURE_THOLD_OUT) &&
                    (p_gdata->l[idx] > GESTURE_THOLD_OUT) &&
                    (p_gdata->r[idx] > GESTURE_THOLD_OUT))
                {
                    u_last = p_gdata->u[idx];
                    d_last = p_gdata->d[idx];
                    l_last = p_gdata->l[idx];
                    r_last = p_gdata->r[idx];
                    break;
                }
            }
        }
        else
        {
            DEBUG("Some first values are zero, skipping.", __FUNCTION__);
        }
    }

    if (b_is_all_ok)
    {
        // Calculate the first vs. last ratio of up/down and left/right
        ud_ratio_first = ((u_first - d_first) * 100) / (u_first + d_first);
        lr_ratio_first = ((l_first - r_first) * 100) / (l_first + r_first);
        ud_ratio_last = ((u_last - d_last) * 100) / (u_last + d_last);
        lr_ratio_last = ((l_last - r_last) * 100) / (l_last + r_last);

        DEBUG("First: U:%d D:%d L:%d R:%d", __FUNCTION__,
            u_first, d_first, l_first, r_first);
        DEBUG("Last: U:%d D:%d L:%d R:%d", __FUNCTION__,
            u_last, d_last, l_last, r_last);
        DEBUG("Ratios: UD Fi/La:%d/%d LR Fi/La:%d/%d", __FUNCTION__, 
            ud_ratio_first, ud_ratio_last, lr_ratio_first, lr_ratio_last);

        // Determine the difference between the first and last ratios
        ud_delta = ud_ratio_last - ud_ratio_first;
        lr_delta = lr_ratio_last - lr_ratio_first;

        DEBUG("Deltas: UD:%d LR:%d", __FUNCTION__, ud_delta, lr_delta);

        // Accumulate the UD and LR delta values
        p_gdelta->ud += ud_delta;
        p_gdelta->lr += lr_delta;

        DEBUG("Accu: UD:%d LR:%d", __FUNCTION__, p_gdelta->ud, p_gdelta->lr);

        // Determine UD gesture
        if (p_gdelta->ud >= GESTURE_SENS_1)
        {
            p_gcount->ud = 1;
        }
        else if (p_gdelta->ud <= -GESTURE_SENS_1)
        {
            p_gcount->ud = -1;
        }
        else
        {
            p_gcount->ud = 0;
        }

        // Determine LR gesture
        if (p_gdelta->lr >= GESTURE_SENS_1)
        {
            p_gcount->lr = 1;
        }
        else if (p_gdelta->lr <= -GESTURE_SENS_1)
        {
            p_gcount->lr = -1;
        }
        else
        {
            p_gcount->lr = 0;
        }


        // Determine Near-Far gesture
        if ((abs(ud_delta) < GESTURE_SENS_2) &&
            (abs(lr_delta) < GESTURE_SENS_2))
        {
            if ((p_gcount->ud == 0) && (p_gcount->lr == 0))
            {
                if ((ud_delta == 0) && (lr_delta == 0))
                {
                    p_gcount->near++;
                }
                else
                {
                    p_gcount->far++;
                }

                if ((p_gcount->near >= 10) && (p_gcount->far >= 2))
                {
                    if ((ud_delta == 0) && (lr_delta == 0))
                    {
                        p_apds->gesture_state = GESTURE_STATE_NEAR;
                    }
                    else if ((ud_delta != 0) && (lr_delta != 0))
                    {
                        p_apds->gesture_state = GESTURE_STATE_FAR;
                    }
                }
            }
            else
            {
                if ((ud_delta == 0) && (lr_delta == 0))
                {
                    p_gcount->near++;
                }

                if (p_gcount->near >= 10)
                {
                    p_gcount->ud = 0;
                    p_gcount->lr = 0;
                    p_gdelta->ud = 0;
                    p_gdelta->lr = 0;
                }
            }
        }

        DEBUG("Count: UD:%d LR:%d Near:%d Far:%d", __FUNCTION__,
            p_gcount->ud, p_gcount->lr, p_gcount->near, p_gcount->far);
    }

    if (!b_is_all_ok)
    {
        DEBUG("Processing failed.\n", __FUNCTION__);
    }

    return b_is_all_ok;
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

    if (b_is_decoded)
    {
        char result[32];

        switch (p_apds->gesture_motion)
        {
        case GESTURE_DIR_UP:
            strcpy(result, "Up");
            break;

        case GESTURE_DIR_DOWN:
            strcpy(result, "Down");
            break;

        case GESTURE_DIR_LEFT:
            strcpy(result, "Left");
            break;

        case GESTURE_DIR_RIGHT:
            strcpy(result, "Right");
            break;

        case GESTURE_DIR_FAR:
            strcpy(result, "Far");
            break;

        case GESTURE_DIR_NEAR:
            strcpy(result, "Near");
            break;

        default:
            strcpy(result, "Up");
            break;
        }
        DEBUG("Decoding result: %s\n", __FUNCTION__, result);
    }
    else
    {
        DEBUG("Decoding failed\n", __FUNCTION__);
    }

    return b_is_decoded;
}

/* [] END OF FILE */
