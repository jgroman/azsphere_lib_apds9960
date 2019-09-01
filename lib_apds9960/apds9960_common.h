

#ifndef _APDS9960_COMMON_H_
#define _APDS9960_COMMON_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "lib_apds9960.h"

#ifdef APDS9960_DEBUG
#define DEBUG(s, f, ...) log_printf("%s %s: " s "\n", "ADPS", f, ## __VA_ARGS__)
#define DEBUG_DEV(s, f, d, ...) log_printf("%s %s (0x%02X): " s "\n", "ADPS", f, d->i2c_addr, ## __VA_ARGS__)
#else
#define DEBUG(s, f, ...)
#define DEBUG_DEV(s, f, d, ...)
#endif // APDS9960_DEBUG

#define ERROR(s, f, ...) log_printf("%s %s: " s "\n", "ADPS9960", f, ## __VA_ARGS__)

// Uncomment line below to see I2C debug data
//#define APDS9960_I2C_DEBUG

int
log_printf(const char *p_format, ...);

bool
reg_read8(apds9960_t *p_apds, uint8_t reg_addr, uint8_t *p_data);

bool
reg_write8(apds9960_t *p_apds, uint8_t reg_addr, const uint8_t *p_data);

ssize_t
reg_read(apds9960_t *p_apds, uint8_t reg_addr, uint8_t *p_data,
    uint32_t data_len);

ssize_t
reg_write(apds9960_t *p_apds, uint8_t reg_addr, const uint8_t *p_data,
    uint32_t data_len);

#ifdef __cplusplus
}
#endif

#endif  // _APDS9960_COMMON_H_

/* [] END OF FILE */
