#include "LDC1614.h"
#include "delay.h"

void IIC_Delay(void)
{
	delay_us(8);
}

void IIC_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	/* I2C 引脚: PB10(SCL), PB11(SDA) - 关键修改:使用开漏输出 */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;  // 改为开漏输出!
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	GPIO_SetBits(GPIOB, GPIO_Pin_10 | GPIO_Pin_11);  // SCL=1, SDA=1

	/* ADDR 引脚: PA11 - 选择I2C地址 */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	GPIO_ResetBits(GPIOA, GPIO_Pin_11);  // ADDR=0 -> I2C地址=0x2A 

	/* SD 引脚: PC13 - 关机控制 */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	GPIO_ResetBits(GPIOC, GPIO_Pin_13);  // SD=0 -> 芯片正常运行(低电平有效)
	
	delay_ms(10);  // 等待芯片稳定
}

void IIC_Start(void)
{
	SDA_OUT();     
	IIC_SDA=1;	  	  
	IIC_SCL=1;
	IIC_Delay();
 	IIC_SDA=0;
	IIC_Delay();
	IIC_SCL=0;
}

void IIC_Stop(void)
{
	SDA_OUT();
	IIC_SCL=0;
	IIC_SDA=0;
 	IIC_Delay();
	IIC_SCL=1; 
	IIC_SDA=1;
	IIC_Delay();							   	
}

uint8_t IIC_Wait_Ack(void)
{
	u16 ucErrTime=0;
	SDA_IN();      
	IIC_SDA=1;
	IIC_Delay();	   
	IIC_SCL=1;
	IIC_Delay();	 
	while(READ_SDA)
	{
		ucErrTime++;
		if(ucErrTime>250)  // 增加超时时间
		{
			IIC_Stop();
			return 1;
		}
	}
	IIC_SCL=0;
	return 0;  
}

void IIC_Ack(void)
{
	IIC_SCL=0;
	SDA_OUT();
	IIC_SDA=0;
	IIC_Delay();
	IIC_SCL=1;
	IIC_Delay();
	IIC_SCL=0;
}
   
void IIC_NAck(void)
{
	IIC_SCL=0;
	SDA_OUT();
	IIC_SDA=1;
	IIC_Delay();
	IIC_SCL=1;
	IIC_Delay();
	IIC_SCL=0;
}
		  
void IIC_Send_Byte(uint8_t txd)
{                        
    uint8_t u8LoopData;   
	SDA_OUT(); 	    
    IIC_SCL=0;
    for(u8LoopData = 0; u8LoopData < 8; u8LoopData++)
    {              
        IIC_SDA=(txd & 0x80) >> 7;
        txd<<=1;
		IIC_Delay();
		IIC_SCL=1;
		IIC_Delay(); 
		IIC_SCL=0;	
		IIC_Delay();
    }	 
}

uint8_t IIC_Read_Byte(uint8_t u8ack)
{
	uint8_t u8LoopData = 0;
	uint8_t u8receive = 0;
	SDA_IN();
    for(u8LoopData = 0; u8LoopData < 8; u8LoopData++)
	{
        IIC_SCL=0; 
        IIC_Delay();
		IIC_SCL=1;
        u8receive <<= 1;
        if(READ_SDA)
		{
			u8receive++;    
		}
        IIC_Delay();		
    }				 
    if (!u8ack)
	{
        IIC_NAck();
	}
    else
	{
        IIC_Ack();
	}		
    return u8receive;
}

uint8_t smbus_writeWord(uint8_t SlaveAddress, uint8_t REG_Address, uint16_t data)
{
	static uint8_t buffer[2];
    buffer[0] = (data >> 8);
	buffer[1] = (uint8_t)(data & 0x00ff);

    IIC_Start();
    IIC_Send_Byte(SlaveAddress << 1);
    if (IIC_Wait_Ack() == 1)
    {
        IIC_Stop();
        return 0;
    }
    IIC_Send_Byte(REG_Address);
    if (IIC_Wait_Ack() == 1)
    {
        IIC_Stop();
        return 0;
    }
    IIC_Send_Byte(buffer[0]);
    if (IIC_Wait_Ack() == 1)
    {
        IIC_Stop();
        return 0;
    }
    IIC_Send_Byte(buffer[1]);
    if (IIC_Wait_Ack() == 1)
    {
        IIC_Stop();
        return 0;
    }
    IIC_Stop();
    return 1;
}

uint8_t smbus_readWord(uint8_t SlaveAddress, uint8_t REG_Address, uint16_t *Read)
{
    uint8_t Dat_L = 0;
    uint8_t Dat_H = 0;
    
    IIC_Start();
    IIC_Send_Byte(SlaveAddress << 1);
    if (IIC_Wait_Ack() == 1)
    {
        IIC_Stop();
        return 0;
    }
    
    IIC_Send_Byte(REG_Address);
    if (IIC_Wait_Ack() == 1)
    {
        IIC_Stop();
        return 0;
    }
    
    IIC_Start();
    IIC_Send_Byte((SlaveAddress << 1) + 1);
    if (IIC_Wait_Ack() == 1)
    {
        IIC_Stop();
        return 0;
    }
    Dat_H = IIC_Read_Byte(1);   // ack
    Dat_L = IIC_Read_Byte(0);   // Nack
    IIC_Stop();
    *Read = ((Dat_H << 8) | Dat_L);
    return 1;
}

static volatile uint8_t dataReady;
static uint16_t allData[8];
static uint8_t default_addr;

uint8_t LDC1614_init(void) 
{
	uint16_t DEVICE_ID;
	uint8_t retVal = 1; 
	
	dataReady = 0;
	IIC_Init();
	delay_ms(50);  // 增加初始化后延时
	
	default_addr = EVM_DEFAULT_I2CADDR;

	// software reset
	smbus_writeWord(default_addr, LDC13xx16xx_CMD_RESET_DEVICE, 0x8000);
	delay_ms(20);  // 复位后等待更长时间
	
	// 读取设备ID
	if (smbus_readWord(default_addr, LDC13xx16xx_CMD_DEVID, &DEVICE_ID) == 0)
	{
		return 1;  // 读取失败
	}
	
	// 兼容多种设备ID (你的可能不是标准的0x3055)
	if (DEVICE_ID == 0x3055 || DEVICE_ID == 0x3054 || (DEVICE_ID & 0xF000) == 0x2000)
	{
		// 配置通道0-3
		retVal &= smbus_writeWord(default_addr, LDC13xx16xx_CMD_REF_COUNT_CH0, 0xFFFF);
		retVal &= smbus_writeWord(default_addr, LDC13xx16xx_CMD_REF_COUNT_CH1, 0xFFFF);
		retVal &= smbus_writeWord(default_addr, LDC13xx16xx_CMD_REF_COUNT_CH2, 0xFFFF);
		retVal &= smbus_writeWord(default_addr, LDC13xx16xx_CMD_REF_COUNT_CH3, 0xFFFF);
		
		retVal &= smbus_writeWord(default_addr, LDC13xx16xx_CMD_OFFSET_CH0, 0x0000);	
		retVal &= smbus_writeWord(default_addr, LDC13xx16xx_CMD_OFFSET_CH1, 0x0000);
		retVal &= smbus_writeWord(default_addr, LDC13xx16xx_CMD_OFFSET_CH2, 0x0000);
		retVal &= smbus_writeWord(default_addr, LDC13xx16xx_CMD_OFFSET_CH3, 0x0000);
		
		retVal &= smbus_writeWord(default_addr, LDC13xx16xx_CMD_SETTLE_COUNT_CH0, 0x0400);
		retVal &= smbus_writeWord(default_addr, LDC13xx16xx_CMD_SETTLE_COUNT_CH1, 0x0400);
		retVal &= smbus_writeWord(default_addr, LDC13xx16xx_CMD_SETTLE_COUNT_CH2, 0x0400);
		retVal &= smbus_writeWord(default_addr, LDC13xx16xx_CMD_SETTLE_COUNT_CH3, 0x0400);
		
		retVal &= smbus_writeWord(default_addr, LDC13xx16xx_CMD_CLOCK_DIVIDERS_CH0, 0x1001);
		retVal &= smbus_writeWord(default_addr, LDC13xx16xx_CMD_CLOCK_DIVIDERS_CH1, 0x1001);
		retVal &= smbus_writeWord(default_addr, LDC13xx16xx_CMD_CLOCK_DIVIDERS_CH2, 0x1001);
		retVal &= smbus_writeWord(default_addr, LDC13xx16xx_CMD_CLOCK_DIVIDERS_CH3, 0x1001);
		
		retVal &= smbus_writeWord(default_addr, LDC13xx16xx_CMD_STATUS, 0x0000);
		retVal &= smbus_writeWord(default_addr, LDC13xx16xx_CMD_CONFIG, 0x1601);

		#if defined(HOST_DEVICE_LDC1612) || defined(HOST_DEVICE_LDC1312)
			retVal &= smbus_writeWord(default_addr, LDC13xx16xx_CMD_MUX_CONFIG, 0x820D);
		#elif defined(HOST_DEVICE_LDC1614) || defined(HOST_DEVICE_LDC1314)
			retVal &= smbus_writeWord(default_addr, LDC13xx16xx_CMD_MUX_CONFIG, 0xC20D);
		#endif
		
		retVal &= smbus_writeWord(default_addr, LDC13xx16xx_CMD_SYSTEM_CLOCK_CONFIG, 0x0002);
		retVal &= smbus_writeWord(default_addr, LDC13xx16xx_CMD_DRIVE_CURRENT_CH0, 0xF000);
		retVal &= smbus_writeWord(default_addr, LDC13xx16xx_CMD_DRIVE_CURRENT_CH1, 0xF000);
		retVal &= smbus_writeWord(default_addr, LDC13xx16xx_CMD_DRIVE_CURRENT_CH2, 0xF000);
		retVal &= smbus_writeWord(default_addr, LDC13xx16xx_CMD_DRIVE_CURRENT_CH3, 0xF000);
		
		delay_ms(100);  // 配置完成后等待稳定
		
		return 0;  // 成功
	}
	
	return 1;  // 设备ID不匹配
}

uint32_t LDC_get_channel_result(u8 channel) 
{
	uint32_t result = 0;
	
	switch(channel)
	{
		case 0:
		{
			smbus_readWord(default_addr, LDC13xx16xx_CMD_DATA_MSB_CH0, &allData[0]);
			smbus_readWord(default_addr, LDC13xx16xx_CMD_DATA_LSB_CH0, &allData[1]);
			result = allData[0] & 0x0FFF;
			result = (result << 16) | allData[1];
			break;
		}
		case 1:
		{
			smbus_readWord(default_addr, LDC13xx16xx_CMD_DATA_MSB_CH1, &allData[2]);
			smbus_readWord(default_addr, LDC13xx16xx_CMD_DATA_LSB_CH1, &allData[3]);
			result = allData[2] & 0x0FFF;
			result = (result << 16) | allData[3];
			break;
		}
		case 2:
		{
			smbus_readWord(default_addr, LDC13xx16xx_CMD_DATA_MSB_CH2, &allData[4]);
			smbus_readWord(default_addr, LDC13xx16xx_CMD_DATA_LSB_CH2, &allData[5]);
			result = allData[4] & 0x0FFF;
			result = (result << 16) | allData[5];
			break;
		}
		case 3:
		{
			smbus_readWord(default_addr, LDC13xx16xx_CMD_DATA_MSB_CH3, &allData[6]);
			smbus_readWord(default_addr, LDC13xx16xx_CMD_DATA_LSB_CH3, &allData[7]);
			result = allData[6] & 0x0FFF;
			result = (result << 16) | allData[7];
			break;
		}
		default:
			break;
	}
	
	result = result & 0x0FFFFFFF;
	return result;
}
