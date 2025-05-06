#include "i2c.h"

void i2c_init(I2C_TypeDef *I2Cx, u16 pin_scl, u16 pin_sda, GPIO_TypeDef *port)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};
    I2C_InitTypeDef I2C_InitStructure = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);

    GPIO_StructInit(&GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = pin_scl;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(port, &GPIO_InitStructure);

    GPIO_StructInit(&GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = pin_sda;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(port, &GPIO_InitStructure);

    I2C_StructInit(&I2C_InitStructure);
    I2C_InitStructure.I2C_ClockSpeed = 400000;
    I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
    I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_16_9;
    I2C_InitStructure.I2C_OwnAddress1 = 0x01;
    I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
    I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;

    I2C_DeInit(I2Cx);
    I2C_Init(I2Cx, &I2C_InitStructure);
    I2C_Cmd(I2Cx, ENABLE);
}

enum I2CStatus i2c_recv_byte(I2C_TypeDef *I2Cx, u8 addr, u8 *buf)
{
    u32 timeout = 0;

    while (I2C_GetFlagStatus(I2Cx, I2C_FLAG_BUSY) != RESET);

    I2C_GenerateSTART(I2Cx, ENABLE);
    while (!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_MODE_SELECT));

    // 7 bit address needs to be shifted one position so we can fit the direction bit at the end.
    I2C_Send7bitAddress(I2Cx, addr << 1, I2C_Direction_Receiver);

    // Wait until an ACK is sent back or timeout
    while (!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED) && timeout < I2C_ACK_TIMEOUT)
        timeout++;

    // Check if ACK is received
    if (!(I2Cx->STAR1 & I2C_STAR1_TXE))
        return I2C_STATUS_ERR_NO_ACK;

    // I2C1->STAR1->RXNE == 1 when data is received
    timeout = 0;
    while (I2C_GetFlagStatus(I2Cx, I2C_FLAG_RXNE) == RESET && timeout < I2C_ACK_TIMEOUT) 
        timeout++;

    *buf = I2C_ReceiveData(I2Cx);
    I2C_GenerateSTOP(I2Cx, ENABLE);

    return I2C_STATUS_SUCCESS;
}

void i2c_scan(I2C_TypeDef *I2Cx)
{
    printf("Starting I2C bus scan\n");
    u8 hit_cnt = 0;
    for (u8 addr=1 ; addr<128 ; addr++) {

        enum I2CStatus res;

        if ((res = i2c_start_tx(I2Cx, addr)) == I2C_STATUS_SUCCESS)
            printf("  %d: 0x%0X\n", hit_cnt++, addr);

        i2c_stop(I2Cx, addr);
    }
    printf("Found %d device(s)\n", hit_cnt);
}

enum I2CStatus i2c_start_tx(I2C_TypeDef *I2Cx, u8 addr)
{
    while (I2C_GetFlagStatus(I2Cx, I2C_FLAG_BUSY) != RESET);

    I2C_GenerateSTART(I2Cx, ENABLE);
    while (!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_MODE_SELECT));

    // 7 bit address needs to be shifted one position so we can fit the direction bit at the end.
    I2C_Send7bitAddress(I2Cx, addr << 1, I2C_Direction_Transmitter);

    // Wait until an ACK is sent back or timeout
    u32 timeout = 0;
    while (!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED) && timeout < I2C_ACK_TIMEOUT)
        timeout++;

    // Check if ACK is received
    if (!(I2Cx->STAR1 & I2C_STAR1_TXE))
        return I2C_STATUS_ERR_NO_ACK;

    return I2C_STATUS_SUCCESS;
}

enum I2CStatus i2c_stop(I2C_TypeDef *I2Cx, u8 addr)
{
    I2C_GenerateSTART(I2Cx, DISABLE);
    I2C_GenerateSTOP(I2Cx, ENABLE);
    return I2C_STATUS_SUCCESS;
}

enum I2CStatus i2c_write_byte(I2C_TypeDef *I2Cx, u8 addr, u8 payload)
{
    /* Send byte and wait until register is empty */
    I2C_SendData(I2Cx, payload);
    while (!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_BYTE_TRANSMITTED));
    return I2C_STATUS_SUCCESS;
}
