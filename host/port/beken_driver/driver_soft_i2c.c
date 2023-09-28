#include "driver_beken_includes.h"
#include "app_beken_includes.h"
#include "beken_external.h"


#ifdef CONFIG_DRIVER_I2C_SOFT
#define I2C_DELAY		2

#define I2C_CLK_PIN      (app_env_get_pin_num(PIN_SCL))//32 //19			
#define I2C_DAT_PIN      (app_env_get_pin_num(PIN_SDA))//33 //18


#define I2C_CLK_OUT   gpio_config(I2C_CLK_PIN,1)
#define I2C_DAT_OUT  gpio_config(I2C_DAT_PIN,1)
#define I2C_DAT_IN   	  gpio_config(I2C_DAT_PIN,3)

#define I2C_DATA_HIGH   gpio_output(I2C_DAT_PIN,1)
#define I2C_DATA_LOW	gpio_output(I2C_DAT_PIN,0)
#define I2C_CLK_HIGH    gpio_output(I2C_CLK_PIN,1)
#define I2C_CLK_LOW	    gpio_output(I2C_CLK_PIN,0)
#define I2C_DATA_GET	gpio_input(I2C_DAT_PIN)
/*
*/
static void I2C_SendByte(uint8_t dat)
{
    uint8_t i;
	
    I2C_DAT_OUT;
	
    for (i = 0; i < 8; i++)
    {
        if (dat & 0x80)
			I2C_DATA_HIGH;
        else
			I2C_DATA_LOW;

        dat <<= 1;
		delay_us(I2C_DELAY);
        I2C_CLK_HIGH;
		delay_us(I2C_DELAY);
        I2C_CLK_LOW;
    }
}


/*
*/
static uint8_t I2C_ReadByte(void)
{
    uint8_t i;
    uint8_t dat = 0;                     
    
    I2C_DAT_IN;
	
    for (i = 0; i < 8; i++)
    {
		delay_us(I2C_DELAY);
        I2C_CLK_HIGH;
		delay_us(I2C_DELAY);

        dat <<= 1;
        if (I2C_DATA_GET)
			dat++;
		
        I2C_CLK_LOW;
    }
	
    return dat;
}


/*
*/
static void I2C_Send_Nack(void)
{
    I2C_DAT_OUT;
	I2C_DATA_HIGH;

	delay_us(I2C_DELAY);
    I2C_CLK_HIGH;
	delay_us(I2C_DELAY);
    I2C_CLK_LOW;
}      


/*
*/
static void I2C_Send_Ack(void)
{
    I2C_DAT_OUT;
	I2C_DATA_LOW;

	delay_us(I2C_DELAY);
    I2C_CLK_HIGH;
	delay_us(I2C_DELAY);
    I2C_CLK_LOW;
}      


/*
*/
static uint8_t I2C_Receive_Ack(void)
{
    uint8_t ack;
	
    I2C_DAT_IN;
	
	delay_us(I2C_DELAY);
    I2C_CLK_HIGH;
	delay_us(I2C_DELAY);

	ack = I2C_DATA_GET;
    I2C_CLK_LOW;
	
    return ack;
}


/*
	init
*/
static void I2C_Init(void)
{
    I2C_DAT_OUT;
    I2C_DATA_HIGH;

    I2C_CLK_OUT;
    I2C_CLK_HIGH;
}


/*
	start
*/
static void I2C_Start(void)
{
    I2C_Init();

    delay_us(I2C_DELAY);
    I2C_DATA_LOW;
    delay_us(I2C_DELAY);
    I2C_CLK_LOW;
    delay_us(I2C_DELAY);
}


/*
	stop
*/
static void I2C_Stop(void)
{
    I2C_DAT_OUT;
    I2C_DATA_LOW;
    delay_us(I2C_DELAY);
    I2C_CLK_HIGH;
    delay_us(I2C_DELAY);
    I2C_DATA_HIGH;
    delay_us(I2C_DELAY);
}



/*
	addr: register address
	size: size to be read
	d	: buffer
*/
void Soft_I2C_Read(uint8_t dev, uint8_t addr, uint8_t Size, void *d)
{	
	uint8_t i;
	uint8_t *ptr = (uint8_t*)d;

    I2C_Start();           /* I2C start */
    I2C_SendByte(dev);    /* slave device address + W */
	I2C_Receive_Ack();
    
    I2C_SendByte(addr);   /* register address */
	I2C_Receive_Ack();

    I2C_Start();       	  /* need to restart start */
    I2C_SendByte(dev | 0x01);   /* slave device address + R */
	I2C_Receive_Ack();

	for (i = 0; i < Size - 1; i++)
	{
		ptr[i] = I2C_ReadByte(); /* read data :size - 1 */
		I2C_Send_Ack();
	}

	ptr[i] = I2C_ReadByte();   /* read data :the last byte */
	I2C_Send_Nack();

    I2C_Stop();                     		
}


/*
	addr: register address
	size: size to be written
	d	: buffer
*/
void Soft_I2C_Write(uint8_t dev, uint8_t addr, uint8_t Size, void *d)
{
	uint8_t i;
	uint8_t *ptr = (uint8_t*)d;

    I2C_Start();                			
    I2C_SendByte(dev);   /* slave device address + W */
	I2C_Receive_Ack();

    I2C_SendByte(addr);   		/* register address */
	I2C_Receive_Ack();

	for (i = 0; i < Size; i++)
	{
		I2C_SendByte(ptr[i]); 	/* write data */
		I2C_Receive_Ack();
	}

    I2C_Stop();                 			
}
#endif
