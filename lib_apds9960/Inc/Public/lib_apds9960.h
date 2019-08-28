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



#ifdef __cplusplus
}
#endif

#endif  // _LIB_APDS9960_H_

/* [] END OF FILE */
