#include "ssd1306.h"
#include <debug.h>

/* Preconditions:
 *     Set clocks
 *     I2C1->CTRL2->
 *     I2C1->CKCFGR->
 *     
 *     Set rising edge
 *     I2C1->RTR->TRISE = 
 * 
 *     Generate start event
 *     I2C1->CTLR1->START = 1
 *       I2C->STAR2->MSL == 1   // AUTO: sets to Master mode
 *       Start event is generated
 *       I2C->STAR1->SB == 1    // AUTO: SB set when done
 * 
 *     Slave address is written to data register
 *     I2C->DATAR->DATAR
 *     I2c->STAR1->SB == 0      // AUTO: SB reset when done
*/

enum I2CRes i2c_start_tx(I2C_TypeDef *I2Cx, u8 addr);
enum I2CRes i2c_stop(I2C_TypeDef *I2Cx, u8 addr);


// OLED init settings
const uint8_t SSD1306_INIT_CMD[] = {
  0xA8, 0x1F,                   // set multiplex for 128x32
  0x20, 0x01,                   // set vertical memory addressing mode
  0xDA, 0x02,                   // set COM pins hardware configuration to sequential
  0x8D, 0x14,                   // enable charge pump
  0xAF                          // switch on OLED
};


void i2c_init()
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};
    I2C_InitTypeDef I2C_InitStructure = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);

    GPIO_StructInit(&GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = SSD1306_I2C_SCL_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(SSD1306_I2C_GPIO_PORT, &GPIO_InitStructure);

    GPIO_StructInit(&GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = SSD1306_I2C_SDA_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(SSD1306_I2C_GPIO_PORT, &GPIO_InitStructure);


    I2C_StructInit(&I2C_InitStructure);
    // i2cinit.I2C_ClockSpeed = 800000; // doesnt; seem to run in 1mbit, but runs at 800kbit? twice as fast as 400kbit
    I2C_InitStructure.I2C_ClockSpeed = 400000;
    I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
    I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_16_9;
    I2C_InitStructure.I2C_OwnAddress1 = 0x01;
    I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
    // i2cinit.I2C_Ack = I2C_Ack_Disable;
    I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;

    I2C_DeInit(I2C1);
    I2C_Init(I2C1, &I2C_InitStructure);
    I2C_Cmd(I2C1, ENABLE);

    //I2C_AcknowledgeConfig( I2C1, ENABLE );



}

enum I2CRes i2c_recv_byte(I2C_TypeDef *I2Cx, u8 addr, u8 *buf)
{
    u32 timeout = 0;

    while (I2C_GetFlagStatus(I2Cx, I2C_FLAG_BUSY) != RESET);

    I2C_GenerateSTART(I2Cx, ENABLE);
    while (!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_MODE_SELECT));

    // 7 bit address needs to be shifted one position so we can fit the direction bit at the end.
    I2C_Send7bitAddress(I2Cx, addr << 1, I2C_Direction_Receiver);

    // Wait until an ACK is sent back or timeout
    while (!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED) && timeout < SSD1306_I2C_TIMEOUT)
        timeout++;

    // Check if ACK is received
    if (!(I2Cx->STAR1 & I2C_STAR1_TXE))
        return I2C_ERR_NO_ACK;

    // I2C1->STAR1->RXNE == 1 when data is received
    timeout = 0;
    while (I2C_GetFlagStatus(I2Cx, I2C_FLAG_RXNE) == RESET && timeout < SSD1306_I2C_TIMEOUT) 
        timeout++;

    *buf = I2C_ReceiveData(I2Cx);
    I2C_GenerateSTOP(I2Cx, ENABLE);

    return I2C_SUCCESS;
}

void i2c_scan(I2C_TypeDef *I2Cx)
{
    printf("Starting I2C bus scan\n");
    u8 hit_cnt = 0;
    for (u8 addr=1 ; addr<128 ; addr++) {

        enum I2CRes res;

        if ((res = i2c_start_tx(I2Cx, addr)) == I2C_SUCCESS)
            printf("  %d: 0x%0X\n", hit_cnt++, addr);

        i2c_stop(I2Cx, addr);
    }
    printf("Found %d device(s)\n", hit_cnt);
}

enum I2CRes i2c_start_tx(I2C_TypeDef *I2Cx, u8 addr)
{
    while (I2C_GetFlagStatus(I2Cx, I2C_FLAG_BUSY) != RESET);

    I2C_GenerateSTART(I2Cx, ENABLE);
    while (!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_MODE_SELECT));

    // 7 bit address needs to be shifted one position so we can fit the direction bit at the end.
    I2C_Send7bitAddress(I2Cx, addr << 1, I2C_Direction_Transmitter);

    // Wait until an ACK is sent back or timeout
    u32 timeout = 0;
    while (!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED) && timeout < SSD1306_I2C_TIMEOUT)
        timeout++;

    // Check if ACK is received
    if (!(I2Cx->STAR1 & I2C_STAR1_TXE))
        return I2C_ERR_NO_ACK;

    return I2C_SUCCESS;
}

enum I2CRes i2c_stop(I2C_TypeDef *I2Cx, u8 addr)
{
    I2C_GenerateSTART(I2Cx, DISABLE);
    I2C_GenerateSTOP(I2Cx, ENABLE);
    return I2C_SUCCESS;
}

enum I2CRes i2c_write_byte(I2C_TypeDef *I2Cx, u8 addr, u8 payload)
{
    I2C_SendData(I2Cx, payload);
    while (!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_BYTE_TRANSMITTED));
    return I2C_SUCCESS;
}

enum I2CRes ssd1306_write_cmd(I2C_TypeDef *I2Cx, u8 addr, u8 cmd)
{
    /* Write a command to Oled.
       Command control byte + command byte */
    enum I2CRes res;

    if ((res = i2c_start_tx(I2Cx, addr)) < I2C_SUCCESS)
        return res;

    i2c_write_byte(I2Cx, addr, SSD1306_CTRL_CMD);
    i2c_write_byte(I2Cx, addr, cmd);

    i2c_stop(I2Cx, addr);
    return I2C_SUCCESS;
}

enum I2CRes ssd1306_init(I2C_TypeDef *I2Cx, u8 addr)
{
    enum I2CRes res;

    for (u8 i=0 ; i<sizeof(SSD1306_INIT_CMD) ; i++) {
        if ((res = ssd1306_write_cmd(I2C1, addr, SSD1306_INIT_CMD[i])) < I2C_SUCCESS)
            return res;

    }
    return I2C_SUCCESS;
}
