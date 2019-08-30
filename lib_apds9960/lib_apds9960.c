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
#define APDS_INIT_PPULSE_PROX     0x87    // 16us, 8 pulses, proximity
#define APDS_INIT_PPULSE_GEST     0x89    // 16us, 10 pulses, gesture
#define APDS_INIT_POFFSET_UR      0       // 0 offset
#define APDS_INIT_POFFSET_DL      0       // 0 offset
#define APDS_INIT_CONFIG1         0x60    // No 12x wait (WTIME) factor
#define APDS_INIT_LDRIVE          CONTROL_LDRIVE_100MA
#define APDS_INIT_PGAIN           CONTROL_PGAIN_4X
#define APDS_INIT_AGAIN           CONTROL_AGAIN_4X
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
#define APDS_INIT_GGAIN           GCONF2_GGAIN_4X
#define APDS_INIT_GLDRIVE         GCONF2_GLDRIVE_100MA
#define APDS_INIT_GWTIME          GCONF2_GWTIME_2MS
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

#if 0
static int
i2c_read(apds9960_t *p_apds, uint8_t* p_buf, size_t buf_len);
#endif

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

    // Reload initial values
    uint8_t reg_byte;

    if (is_init_ok)
    {
        // ENABLE: Disable all functions, power off
        reg_byte = 0;
        is_init_ok = reg_write(p_apds, APDS9960_ENABLE, &reg_byte, 1);
    }

    if (is_init_ok)
    {
        // ATIME: 103 ms
        reg_byte = APDS_INIT_ATIME;
        is_init_ok = reg_write(p_apds, APDS9960_ATIME, &reg_byte, 1);
    }

    if (is_init_ok)
    {
        // WTIME: 27 ms
        reg_byte = APDS_INIT_WTIME;
        is_init_ok = reg_write(p_apds, APDS9960_WTIME, &reg_byte, 1);
    }

    if (is_init_ok)
    {
        // PPULSE: 16us, 8 pulses, optimized for proximity engine
        reg_byte = APDS_INIT_PPULSE_PROX;
        is_init_ok = reg_write(p_apds, APDS9960_PPULSE, &reg_byte, 1);
    }

    if (is_init_ok)
    {
        // POFFSET_UR: 0
        reg_byte = APDS_INIT_POFFSET_UR;
        is_init_ok = reg_write(p_apds, APDS9960_POFFSET_UR, &reg_byte, 1);
    }

    if (is_init_ok)
    {
        // POFFSET_DL: 0
        reg_byte = APDS_INIT_POFFSET_DL;
        is_init_ok = reg_write(p_apds, APDS9960_POFFSET_DL, &reg_byte, 1);
    }

    if (is_init_ok)
    {
        // CONFIG1: No 12x wait (WTIME) factor
        reg_byte = APDS_INIT_CONFIG1;
        is_init_ok = reg_write(p_apds, APDS9960_CONFIG1, &reg_byte, 1);
    }

    if (is_init_ok)
    {
        // CONTROL: 
        // -- LDRIVE: LED Current 100 mA
        // -- PGAIN: 4x
        // -- AGAIN: 4x
        apds9960_control_t reg_control;
        is_init_ok = reg_read(p_apds, APDS9960_CONTROL, &reg_control.byte, 1);
        if (is_init_ok)
        {
            reg_control.LDRIVE = APDS_INIT_LDRIVE;
            reg_control.PGAIN = APDS_INIT_PGAIN;
            reg_control.AGAIN = APDS_INIT_AGAIN;
            is_init_ok = reg_write(p_apds, APDS9960_CONTROL, &reg_control.byte, 1);
        }
    }

    if (is_init_ok)
    {
        // PILT: 0
        reg_byte = APDS_INIT_PILT;
        is_init_ok = reg_write(p_apds, APDS9960_PILT, &reg_byte, 1);
    }

    if (is_init_ok)
    {
        // PIHT: 50
        reg_byte = APDS_INIT_PIHT;
        is_init_ok = reg_write(p_apds, APDS9960_PIHT, &reg_byte, 1);
    }

    if (is_init_ok)
    {
        // AILT: 0xFFFF
        reg_byte = (uint8_t) (APDS_INIT_AILT & 0xFF);
        is_init_ok = reg_write(p_apds, APDS9960_AILTL, &reg_byte, 1);
        if (is_init_ok)
        {
            reg_byte = (uint8_t) (APDS_INIT_AILT >> 8);
            is_init_ok = reg_write(p_apds, APDS9960_AILTH, &reg_byte, 1);
        }
    }

    if (is_init_ok)
    {
        // AIHT: 0
        reg_byte = (uint8_t) (APDS_INIT_AIHT & 0xFF);
        is_init_ok = reg_write(p_apds, APDS9960_AIHTL, &reg_byte, 1);
        if (is_init_ok)
        {
            reg_byte = (uint8_t)(APDS_INIT_AIHT >> 8);
            is_init_ok = reg_write(p_apds, APDS9960_AIHTH, &reg_byte, 1);
        }
    }

    if (is_init_ok)
    {
        // PERS: 2 consecutive prox or ALS for interrupt
        reg_byte = APDS_INIT_PERS;
        is_init_ok = reg_write(p_apds, APDS9960_PERS, &reg_byte, 1);
    }

    if (is_init_ok)
    {
        // CONFIG2: No saturation interrupts or LED boost
        reg_byte = APDS_INIT_CONFIG2;
        is_init_ok = reg_write(p_apds, APDS9960_CONFIG2, &reg_byte, 1);
    }

    if (is_init_ok)
    {
        // CONFIG3: Enable all photodiodes, no SAI
        reg_byte = APDS_INIT_CONFIG3;
        is_init_ok = reg_write(p_apds, APDS9960_CONFIG3, &reg_byte, 1);
    }

    if (is_init_ok)
    {
        // GPENTH: 40
        reg_byte = APDS_INIT_GPENTH;
        is_init_ok = reg_write(p_apds, APDS9960_GPENTH, &reg_byte, 1);
    }

    if (is_init_ok)
    {
        // GEXTH: 30
        reg_byte = APDS_INIT_GEXTH;
        is_init_ok = reg_write(p_apds, APDS9960_GEXTH, &reg_byte, 1);
    }

    if (is_init_ok)
    {
        // GCONF1: 4 events for interrupt, 1 for exit
        reg_byte = APDS_INIT_GCONF1;
        is_init_ok = reg_write(p_apds, APDS9960_GCONF1, &reg_byte, 1);
    }

    if (is_init_ok)
    {
        // GCONF2:
        // -- GGAIN: 4x
        // -- GLDRIVE: 100 mA
        // -- GWTIME: 2.8 ms
        apds9960_gconf2_t reg_gconf2;
        is_init_ok = reg_read(p_apds, APDS9960_GCONF2, &reg_gconf2.byte, 1);
        if (is_init_ok)
        {
            reg_gconf2.GGAIN = APDS_INIT_GGAIN;
            reg_gconf2.GLDRIVE = APDS_INIT_GLDRIVE;
            reg_gconf2.GWTIME = APDS_INIT_GWTIME;
            is_init_ok = reg_write(p_apds, APDS9960_GCONF2, &reg_gconf2.byte, 1);
        }
    }

    if (is_init_ok)
    {
        // GOFFSET_U, GOFFSET_D, GOFFSET_L, GOFFSET_R: 0
        reg_byte = APDS_INIT_GOFFSET;
        is_init_ok = reg_write(p_apds, APDS9960_GOFFSET_U, &reg_byte, 1);

        if (is_init_ok)
        {
            is_init_ok = reg_write(p_apds, APDS9960_GOFFSET_D, &reg_byte, 1);
        }

        if (is_init_ok)
        {
            is_init_ok = reg_write(p_apds, APDS9960_GOFFSET_L, &reg_byte, 1);
        }

        if (is_init_ok)
        {
            is_init_ok = reg_write(p_apds, APDS9960_GOFFSET_R, &reg_byte, 1);
        }
    }

    if (is_init_ok)
    {
        // GPULSE: 32 us, 10 pulses
        reg_byte = APDS_INIT_GPULSE;
        is_init_ok = reg_write(p_apds, APDS9960_GPULSE, &reg_byte, 1);
    }

    if (is_init_ok)
    {
        // GCONF3: All diodes active during gesture
        reg_byte = APDS_INIT_GCONF3;
        is_init_ok = reg_write(p_apds, APDS9960_GCONF3, &reg_byte, 1);
    }

    if (is_init_ok)
    {
        // GCONF4:
        // -- GIEN: 0
        apds9960_gconf4_t reg_gconf4;
        is_init_ok = reg_read(p_apds, APDS9960_GCONF4, &reg_gconf4.byte, 1);
        if (is_init_ok)
        {
            reg_gconf4.GIEN = APDS_INIT_GIEN;
            is_init_ok = reg_write(p_apds, APDS9960_GCONF4, &reg_gconf4.byte, 1);
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
    if (! reg_write(p_apds, APDS9960_ENABLE, &reg_enable.byte, 1))
    {
        ERROR("Error powering off APDS9960.", __FUNCTION__);
    }

    // Free allocated memory
    free(p_apds);
}

bool
apds9960_als_enable(apds9960_t *p_apds, bool b_is_interrupt_enabled)
{
    bool b_is_all_ok = false;

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

    return b_is_all_ok;
}

bool
apds9960_als_disable(apds9960_t *p_apds)
{
    // Set ENABLE register
    // -- AEN: ALS Disable
    // -- AIEN: ALS Interrupt Disable
    apds9960_enable_t reg_enable;
    bool b_is_all_ok = reg_read(p_apds, APDS9960_ENABLE, &reg_enable.byte, 1);

    if (b_is_all_ok)
    {
        reg_enable.AEN = 0;
        reg_enable.AIEN = 0;
        b_is_all_ok = reg_write(p_apds, APDS9960_ENABLE, &reg_enable.byte, 1);
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

    b_is_all_ok = reg_read(p_apds, APDS9960_CDATAH, &reg_value, 1);
    if (b_is_all_ok)
    {
        *value_clear = (uint16_t)(reg_value << 8);
        b_is_all_ok = reg_read(p_apds, APDS9960_CDATAL, &reg_value, 1);
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

    b_is_all_ok = reg_read(p_apds, APDS9960_RDATAH, &reg_value, 1);
    if (b_is_all_ok)
    {
        *value_red = (uint16_t)(reg_value << 8);
        b_is_all_ok = reg_read(p_apds, APDS9960_RDATAL, &reg_value, 1);
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

    b_is_all_ok = reg_read(p_apds, APDS9960_GDATAH, &reg_value, 1);
    if (b_is_all_ok)
    {
        *value_green = (uint16_t)(reg_value << 8);
        b_is_all_ok = reg_read(p_apds, APDS9960_GDATAL, &reg_value, 1);
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

    b_is_all_ok = reg_read(p_apds, APDS9960_BDATAH, &reg_value, 1);
    if (b_is_all_ok)
    {
        *value_blue = (uint16_t)(reg_value << 8);
        b_is_all_ok = reg_read(p_apds, APDS9960_BDATAL, &reg_value, 1);
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

bool
apds9960_proximity_enable(apds9960_t *p_apds, bool b_is_interrupt_enabled)
{
    bool b_is_all_ok = false;

    // Set CONTROL register
    // -- PGAIN: init value
    // -- LDRIVE: init value
    apds9960_control_t reg_control;
    b_is_all_ok = reg_read(p_apds, APDS9960_CONTROL, &reg_control.byte, 1);
    if (b_is_all_ok)
    {
        reg_control.PGAIN = APDS_INIT_PGAIN;
        reg_control.LDRIVE = APDS_INIT_LDRIVE;
        b_is_all_ok = reg_write(p_apds, APDS9960_CONTROL, &reg_control.byte, 1);
    }

    // Set ENABLE register
    // -- PON: Power On
    // -- PEN: Proximity Detect Enable
    // -- PIEN: b_is_interrupt_enabled
    if (b_is_all_ok)
    {
        apds9960_enable_t reg_enable;
        b_is_all_ok = reg_read(p_apds, APDS9960_ENABLE, &reg_enable.byte, 1);
        if (b_is_all_ok)
        {
            reg_enable.PON = 1;
            reg_enable.PEN = 1;
            reg_enable.PIEN = b_is_interrupt_enabled;
            b_is_all_ok = reg_write(p_apds, APDS9960_ENABLE, &reg_enable.byte, 1);
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
    bool b_is_all_ok = reg_read(p_apds, APDS9960_ENABLE, &reg_enable.byte, 1);

    if (b_is_all_ok)
    {
        reg_enable.PEN = 0;
        reg_enable.PIEN = 0;
        b_is_all_ok = reg_write(p_apds, APDS9960_ENABLE, &reg_enable.byte, 1);
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
    return reg_read(p_apds, APDS9960_PDATA, p_value_proximity, 1);
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
        log_printf("APDS %s (0x%02X):  WRITE ",
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
