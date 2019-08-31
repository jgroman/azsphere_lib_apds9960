/***************************************************************************//**
* @file    lib_apds9960.h
* @version 1.0.0
*
* @brief .
*
* @author Jaroslav Groman
*
* @date
*
*******************************************************************************/

#ifndef _LIB_APDS9960_H_
#define _LIB_APDS9960_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include <applibs/i2c.h>

// Uncomment line below to enable debugging messages
#define APDS9960_DEBUG

#define APDS9960_I2C_ADDRESS    0x39

// APDS9960 Registers
#define APDS9960_ENABLE     0x80    // Enable states and interrupts 
#define APDS9960_ATIME      0x81    // ADC integration time
#define APDS9960_WTIME      0x83    // Wait time (non-gesture)
#define APDS9960_AILTL      0x84    // ALS interrupt low threshold low byte
#define APDS9960_AILTH      0x85    // ALS interrupt low threshold high byte
#define APDS9960_AIHTL      0x86    // ALS interrupt high threshold low byte
#define APDS9960_AIHTH      0x87    // ALS interrupt high threshold high byte
#define APDS9960_PILT       0x89    // Proximity interrupt low threshold
#define APDS9960_PIHT       0x8B    // Proximity interrupt high threshold
#define APDS9960_PERS       0x8C    // Interrupt persistence filters (non-gesture)
#define APDS9960_CONFIG1    0x8D    // Configuration register one
#define APDS9960_PPULSE     0x8E    // Proximity pulse count and length
#define APDS9960_CONTROL    0x8F    // Gain control
#define APDS9960_CONFIG2    0x90    // Configuration register two
#define APDS9960_ID         0x92    // Device ID
#define APDS9960_STATUS     0x93    // Device status
#define APDS9960_CDATAL     0x94    // Low byte of clear channel data
#define APDS9960_CDATAH     0x95    // High byte of clear channel data
#define APDS9960_RDATAL     0x96    // Low byte of red channel data
#define APDS9960_RDATAH     0x97    // High byte of red channel data
#define APDS9960_GDATAL     0x98    // Low byte of green channel data
#define APDS9960_GDATAH     0x99    // High byte of green channel data
#define APDS9960_BDATAL     0x9A    // Low byte of blue channel data
#define APDS9960_BDATAH     0x9B    // High byte of blue channel data
#define APDS9960_PDATA      0x9C    // Proximity data
#define APDS9960_POFFSET_UR 0x9D    // Proximity offset for UP and RIGHT photodiodes
#define APDS9960_POFFSET_DL 0x9E    // Proximity offset for DOWN and LEFT photodiodes
#define APDS9960_CONFIG3    0x9F    // Configuration register three
#define APDS9960_GPENTH     0xA0    // Gesture proximity enter threshold
#define APDS9960_GEXTH      0xA1    // Gesture exit threshold
#define APDS9960_GCONF1     0xA2    // Gesture configuration one
#define APDS9960_GCONF2     0xA3    // Gesture configuration two
#define APDS9960_GOFFSET_U  0xA4    // Gesture UP offset register
#define APDS9960_GOFFSET_D  0xA5    // Gesture DOWN offset register
#define APDS9960_GOFFSET_L  0xA7    // Gesture LEFT offset register
#define APDS9960_GOFFSET_R  0xA9    // Gesture RIGHT offset register
#define APDS9960_GPULSE     0xA6    // Gesture pulse count and length
#define APDS9960_GCONF3     0xAA    // Gesture configuration three
#define APDS9960_GCONF4     0xAB    // Gesture configuration four
#define APDS9960_GFLVL      0xAE    // Gesture FIFO level
#define APDS9960_GSTATUS    0xAF    // Gesture status
#define APDS9960_IFORCE     0xE4    // Force interrupt
#define APDS9960_PICLEAR    0xE5    // Proximity interrupt clear
#define APDS9960_CICLEAR    0xE6    // ALS clear channel interrupt clear
#define APDS9960_AICLEAR    0xE7    // All non-gesture interrupts clear
#define APDS9960_GFIFO_U    0xFC    // Gesture FIFO UP value
#define APDS9960_GFIFO_D    0xFD    // Gesture FIFO DOWN value
#define APDS9960_GFIFO_L    0xFE    // Gesture FIFO LEFT value
#define APDS9960_GFIFO_R    0xFF    // Gesture FIFO RIGHT value

// Device ID
#define APDS9960_DEVICE_ID  0xAB    // Part number identification

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


// ENABLE Register bitfields
typedef struct
{
    union
    {
        struct 
        {
            uint8_t PON : 1;    // Power ON
            uint8_t AEN : 1;    // ALS Enable
            uint8_t PEN : 1;    // Proximity Detect Enable
            uint8_t WEN : 1;    // Wait Enable
            uint8_t AIEN : 1;   // ALS Interrupt Enable
            uint8_t PIEN : 1;   // Proximity Interrupt Enable
            uint8_t GEN : 1;    // Gesture Enable
            uint8_t RSVD7 : 1;  // Reserved. Write as 0.
        };
        unsigned char byte;
    };
} apds9960_enable_t;

// PERS Register bitfields
typedef struct
{
    union
    {
        struct 
        {
            uint8_t APERS : 4;    // ALS Interrupt Persistence
            uint8_t PPERS : 4;    // Proximity Interrupt Persistence
        };
        unsigned char byte;
    };
} apds9960_pers_t;

// CONFIG1 Register bitfields
typedef struct
{
    union
    {
        struct
        {
            uint8_t RSVD0 : 1;
            uint8_t WLONG : 1;      // Wait Long
            uint8_t RSVD2 : 6;
        };
        unsigned char byte;
    };
} apds9960_config1_t;

// PPULSE Register bitfields
typedef struct
{
    union
    {
        struct
        {
            uint8_t PPULSE : 6;     // Proximity Pulse Count
            uint8_t PPLEN : 2;      // Proximity Pulse Length
        };
        unsigned char byte;
    };
} apds9960_ppulse_t;

#define PPULSE_PPLEN_4US   0
#define PPULSE_PPLEN_8US   1
#define PPULSE_PPLEN_16US  2
#define PPULSE_PPLEN_32US  3

// CONTROL Register bitfields
typedef struct
{
    union
    {
        struct
        {
            uint8_t AGAIN  : 2; // ALS and Color Gain Control
            uint8_t PGAIN  : 2; // Proximity Gain Control
            uint8_t RSVD4  : 1; // Reserved. Write as 0.
            uint8_t RSVD5  : 1; // Reserved. Write as 0.
            uint8_t LDRIVE : 2; // LED Drive Strength
        };
        unsigned char byte;
    };
} apds9960_control_t;

#define CONTROL_AGAIN_1X   0
#define CONTROL_AGAIN_4X   1
#define CONTROL_AGAIN_16X  2
#define CONTROL_AGAIN_64X  3

#define CONTROL_PGAIN_1X   0
#define CONTROL_PGAIN_2X   1
#define CONTROL_PGAIN_4X   2
#define CONTROL_PGAIN_8X   3

#define CONTROL_LDRIVE_100MA 0
#define CONTROL_LDRIVE_50MA  1
#define CONTROL_LDRIVE_25MA  2
#define CONTROL_LDRIVE_12MA  3

// CONFIG2 Register bitfields
typedef struct
{
    union
    {
        struct
        {
            uint8_t RSVD0     : 1;  // Reserved. Write as 1.
            uint8_t RSVD1     : 3;  // Reserved. Write as 0.
            uint8_t LED_BOOST : 2;  // Additional LDR current
            uint8_t CPSIEN    : 1;  // Clear Photodiode Saturation Interrupt En
            uint8_t PSIEN     : 1;  // Proximity Saturation Interrupt Enable
        };
        unsigned char byte;
};
} apds9960_config2_t;

#define CONFIG2_LED_BOOST_100   0   // LED Boost Current 100%
#define CONFIG2_LED_BOOST_150   1   // LED Boost Current 150%
#define CONFIG2_LED_BOOST_200   2   // LED Boost Current 200%
#define CONFIG2_LED_BOOST_300   3   // LED Boost Current 300%

// STATUS Register bitfields
typedef struct
{
    union
    {
        struct
        {
            uint8_t AVALID : 1;     // ALS Valid
            uint8_t PVALID : 1;     // Proximity Valid
            uint8_t GINT   : 1;     // Gesture Interrupt
            uint8_t RSVD3  : 1;     
            uint8_t AINT   : 1;     // ALS Interrupt
            uint8_t PINT   : 1;     // Proximity Interrupt
            uint8_t PGSAT  : 1;     // Previous Cycle Analog Saturation Event
            uint8_t CPSAT  : 1;     // Clear photodiode Saturation
        };
        unsigned char byte;
    };
} apds9960_status_t;

// CONFIG3 Register bitfields
typedef struct
{
    union
    {
        struct
        {
            uint8_t PMASK_R : 1;    // Proximity Mask RIGHT Enable
            uint8_t PMASK_L : 1;    // Proximity Mask LEFT Enable
            uint8_t PMASK_D : 1;    // Proximity Mask DOWN Enable
            uint8_t PMASK_U : 1;    // Proximity Mask UP Enable
            uint8_t SAI     : 1;    // Sleep After Interrupt
            uint8_t PCMP    : 1;    // Proximity Gain Compensation Enable
            uint8_t RSVD6   : 2;
        };
        unsigned char byte;
    };
} apds9960_config3_t;

// GCONF1 Register bitfields
typedef struct
{
    union
    {
        struct
        {
            uint8_t GEXPERS : 2;    // Gesture Exit Persistence
            uint8_t GEXMSK  : 4;    // Gesture Exit Mask
            uint8_t GFIFOTH : 2;    // Gesture FIFO Threshold
        };
        unsigned char byte;
};
} apds9960_gconf1_t;

// GCONF2 Register bitfields
typedef struct
{
    union
    {
        struct
        {
            uint8_t GWTIME  : 3;    // Gesture Wait Time
            uint8_t GLDRIVE : 2;    // Gesture LED Drive Strength
            uint8_t GGAIN   : 2;    // Gesture Gain Control
            uint8_t RSVD7   : 1;
        };
        unsigned char byte;
    };
} apds9960_gconf2_t;

#define GCONF2_GWTIME_0MS    0
#define GCONF2_GWTIME_2MS    1
#define GCONF2_GWTIME_5MS    2
#define GCONF2_GWTIME_8MS    3
#define GCONF2_GWTIME_14MS   4
#define GCONF2_GWTIME_22MS   5
#define GCONF2_GWTIME_30MS   6
#define GCONF2_GWTIME_39MS   7

#define GCONF2_GLDRIVE_100MA    0
#define GCONF2_GLDRIVE_50MA     1
#define GCONF2_GLDRIVE_25MA     2
#define GCONF2_GLDRIVE_12MA     3

#define GCONF2_GGAIN_1X    0
#define GCONF2_GGAIN_2X    1
#define GCONF2_GGAIN_4X    2
#define GCONF2_GGAIN_8X    3


// GPULSE Register bitfields
typedef struct
{
    union
    {
        struct
        {
            uint8_t GPULSE : 6;    // Number of Gesture Pulses
            uint8_t GPLEN : 2;     // Gesture Pulse Length
        };
        unsigned char byte;
    };
} apds9960_gpulse_t;

#define GPULSE_GPLEN_4US   0
#define GPULSE_GPLEN_8US   1
#define GPULSE_GPLEN_16US  2
#define GPULSE_GPLEN_32US  3

// GCONF3 Register bitfields
typedef struct
{
    union
    {
        struct
        {
            uint8_t GDIMS : 2;    // Gesture Dimension Select
            uint8_t RSVD2 : 6;
        };
        unsigned char byte;
    };
} apds9960_gconf3_t;

#define GCONF2_GDIMS_ALL   0
#define GCONF2_GDIMS_UD    1
#define GCONF2_GDIMS_LR    2

// GCONF4 Register bitfields
typedef struct
{
    union
    {
        struct
        {
            uint8_t GMODE     : 1;    // Gesture Mode
            uint8_t GIEN      : 1;    // Gesture Interrupt Enable
            uint8_t GFIFO_CLR : 1;    // Clear GFIFO
            uint8_t RSVD3     : 5;
        };
        unsigned char byte;
    };
} apds9960_gconf4_t;

// GSTATUS Register bitfields
typedef struct {
    union {
        struct {
            uint8_t GVALID : 1;    // Gesture FIFO Data
            uint8_t GFOV   : 1;    // Gesture FIFO Overflow
            uint8_t RSVD2  : 6;
        };
        unsigned char byte;
    };
} apds9960_gstatus_t;

typedef struct
{
    uint8_t u[32];      // Data buffer Up
    uint8_t d[32];      // Data buffer Down
    uint8_t l[32];      // Data buffer Left
    uint8_t r[32];      // Data buffer Right
    uint8_t index;
    uint8_t count;      // Gesture count
    uint8_t thold_in;   // Gesture Threshold In
    uint8_t thold_out;  // Gesture Threshold Out
} apds9960_gesture_data_t;

typedef struct
{
    int ud;
    int lr;
} apds9960_gesture_delta_t;

typedef struct
{
    int ud;
    int lr;
    int near;
    int far;
} apds9960_gesture_count_t;

typedef struct {
    int i2c_fd;                                 // I2C interface file descriptor
    I2C_DeviceAddress i2c_addr;                 // I2C device address
    apds9960_gesture_data_t gesture_data;
    apds9960_gesture_delta_t gesture_delta;
    apds9960_gesture_count_t gesture_count;
    int gesture_state;
    int gesture_motion;
} apds9960_t;

enum {
    GESTURE_DIR_NONE,
    GESTURE_DIR_LEFT,
    GESTURE_DIR_RIGHT,
    GESTURE_DIR_UP,
    GESTURE_DIR_DOWN,
    GESTURE_DIR_NEAR,
    GESTURE_DIR_FAR,
    GESTURE_DIR_ALL
};

enum {
    GESTURE_STATE_NA,
    GESTURE_STATE_NEAR,
    GESTURE_STATE_FAR,
    GESTURE_STATE_ALL
};

apds9960_t
*apds9960_open(int i2c_fd, I2C_DeviceAddress i2c_addr);

void
apds9960_close(apds9960_t *p_apds);

// apds9960_als

bool
apds9960_als_enable(apds9960_t *p_apds, bool b_are_interrupts_enabled);

bool
apds9960_als_disable(apds9960_t *p_apds);

bool
apds9960_als_read_clear(apds9960_t *p_apds, uint16_t *value_clear);

bool
apds9960_als_read_red(apds9960_t *p_apds, uint16_t *value_red);

bool
apds9960_als_read_green(apds9960_t *p_apds, uint16_t *value_green);

bool
apds9960_als_read_blue(apds9960_t *p_apds, uint16_t *value_blue);

// apds9960_proximity

bool
apds9960_proximity_enable(apds9960_t *p_apds, bool b_is_interrupt_enabled);

bool
apds9960_proximity_disable(apds9960_t *p_apds);

bool
apds9960_proximity_read(apds9960_t *p_apds, uint8_t *p_value_proximity);

// apds9960_gesture

bool
apds9960_gesture_enable(apds9960_t *p_apds, bool b_is_interrupt_enabled);

bool
apds9960_gesture_disable(apds9960_t *p_apds);

bool
apds9960_gesture_is_valid(apds9960_t *p_apds, bool *p_value);

int
apds9960_gesture_read(apds9960_t *p_apds);


#ifdef __cplusplus
}
#endif

#endif  // _LIB_APDS9960_H_

/* [] END OF FILE */
