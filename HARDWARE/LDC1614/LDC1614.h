#ifndef _LDC1614_H
#define _LDC1614_H
#include "sys.h"

/***************************************
 * HOST_DEVICE_LDC1612,HOST_DEVICE_LDC1312
 * HOST_DEVICE_LDC1614,HOST_DEVICE_LDC1314
 ***************************************/

#define HOST_DEVICE_LDC1614	//选择芯片型号

 //=========== I2C引脚修改(避免与LCD PB8/PB9冲突) ===========
 // 原来: PB8(SCL) / PB9(SDA) 与 LCD 的 CS/BLK 冲突
 // 改为: PB10(SCL) / PB11(SDA)

 //IO方向设置
#define SDA_IN()  {GPIOB->CRH &= 0XFFFF0FFF; GPIOB->CRH |= (u32)8 << 12;}  // PB11配置为输入
#define SDA_OUT() {GPIOB->CRH &= 0XFFFF0FFF; GPIOB->CRH |= (u32)3 << 12;}  // PB11配置为输出

//IO操作函数	 
#define IIC_SCL    PBout(10) 	// SCL -> PB10
#define IIC_SDA    PBout(11)    // 输出SDA -> PB11
#define READ_SDA   PBin(11) 	// 输入SDA -> PB11

// LDC 指令集
#define LDC13xx16xx_CMD_DATA_MSB_CH0	        0x00
#define LDC13xx16xx_CMD_DATA_LSB_CH0	        0x01
#define LDC13xx16xx_CMD_DATA_MSB_CH1	        0x02
#define LDC13xx16xx_CMD_DATA_LSB_CH1	        0x03
#define LDC13xx16xx_CMD_DATA_MSB_CH2	        0x04
#define LDC13xx16xx_CMD_DATA_LSB_CH2	        0x05
#define LDC13xx16xx_CMD_DATA_MSB_CH3	        0x06
#define LDC13xx16xx_CMD_DATA_LSB_CH3	        0x07
#define LDC13xx16xx_CMD_REF_COUNT_CH0	        0x08
#define LDC13xx16xx_CMD_REF_COUNT_CH1	        0x09
#define LDC13xx16xx_CMD_REF_COUNT_CH2	        0x0A
#define LDC13xx16xx_CMD_REF_COUNT_CH3	        0x0B
#define LDC13xx16xx_CMD_OFFSET_CH0	            0x0C
#define LDC13xx16xx_CMD_OFFSET_CH1	            0x0D
#define LDC13xx16xx_CMD_OFFSET_CH2	            0x0E
#define LDC13xx16xx_CMD_OFFSET_CH3	            0x0F
#define LDC13xx16xx_CMD_SETTLE_COUNT_CH0	    0x10
#define LDC13xx16xx_CMD_SETTLE_COUNT_CH1	    0x11
#define LDC13xx16xx_CMD_SETTLE_COUNT_CH2	    0x12
#define LDC13xx16xx_CMD_SETTLE_COUNT_CH3	    0x13
#define LDC13xx16xx_CMD_CLOCK_DIVIDERS_CH0 	    0x14
#define LDC13xx16xx_CMD_CLOCK_DIVIDERS_CH1 	    0x15
#define LDC13xx16xx_CMD_CLOCK_DIVIDERS_CH2 	    0x16
#define LDC13xx16xx_CMD_CLOCK_DIVIDERS_CH3 	    0x17
#define LDC13xx16xx_CMD_STATUS 	                0x18
#define LDC13xx16xx_CMD_ERROR_CONFIG 	        0x19
#define LDC13xx16xx_CMD_CONFIG 	                0x1A
#define LDC13xx16xx_CMD_MUX_CONFIG 	            0x1B
#define LDC13xx16xx_CMD_RESET_DEVICE 	        0x1C
#define LDC13xx16xx_CMD_SYSTEM_CLOCK_CONFIG	    0x1D
#define LDC13xx16xx_CMD_DRIVE_CURRENT_CH0	    0x1E
#define LDC13xx16xx_CMD_DRIVE_CURRENT_CH1 	    0x1F
#define LDC13xx16xx_CMD_DRIVE_CURRENT_CH2 	    0x20
#define LDC13xx16xx_CMD_DRIVE_CURRENT_CH3	    0x21
#define LDC13xx16xx_CMD_MANUFACTID	            0x7E
#define LDC13xx16xx_CMD_DEVID	                0x7F

/** @name - Defaults Settings - */
//@{
#define EVM_MIN_I2CADDR                         0x2A
#define EVM_MAX_I2CADDR                         0x2B
#define EVM_DEFAULT_I2CADDR EVM_MIN_I2CADDR
#define EVM_DEFAULTS_SIZE                       24 // 13 registers, 0x08 - 0x14
//@}

uint8_t smbus_writeWord(uint8_t SlaveAddress, uint8_t REG_Address, uint16_t data);
uint8_t smbus_readWord(uint8_t SlaveAddress, uint8_t REG_Address, uint16_t* Read);
uint8_t LDC1614_init(void);
uint32_t LDC_get_channel_result(uint8_t channel);

#endif
