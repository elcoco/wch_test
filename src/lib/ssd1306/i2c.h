#ifndef I2C_H
#define I2C_H

#include <ch32v20x_i2c.h>
#include <debug.h>

enum I2CStatus {
    I2C_STATUS_SUCCESS = 0,
    I2C_STATUS_ERR_NO_ACK = -1,
    I2C_STATUS_ERR_UNKNOWN = -2,
};

#define I2C_SCL_PIN GPIO_Pin_6
#define I2C_SDA_PIN GPIO_Pin_7
#define I2C_GPIO_PORT GPIOB

// Max timeout for I2C stuff
#define I2C_ACK_TIMEOUT 5000

void i2c_init(I2C_TypeDef *I2Cx, u16 pin_scl, u16 pin_sda, GPIO_TypeDef *port);
void i2c_scan(I2C_TypeDef *I2Cx);

enum I2CStatus i2c_write_byte(I2C_TypeDef *I2Cx, u8 addr, u8 payload);

enum I2CStatus i2c_start_tx(I2C_TypeDef *I2Cx, u8 addr);
enum I2CStatus i2c_stop(I2C_TypeDef *I2Cx, u8 addr);

#endif // I2C_H
