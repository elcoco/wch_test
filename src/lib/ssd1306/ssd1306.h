#ifndef SSD1306_H
#define SSD1306_H

#include <ch32v20x_i2c.h>

#define SSD1306_I2C_PERIPH_PORT RCC_APB2Periph_GPIOB

#define SSD1306_I2C_SCL_PIN GPIO_Pin_6
#define SSD1306_I2C_SDA_PIN GPIO_Pin_7
#define SSD1306_I2C_GPIO_PORT GPIOB

#define SSD1306_CTRL_CMD 0x00
#define SSD1306_CTRL_DAT 0x60

// Max timeout for I2C stuff
#define SSD1306_I2C_TIMEOUT 5000

enum I2CRes {
    I2C_SUCCESS = 0,
    I2C_ERR_NO_ACK = -1,
    I2C_ERR_UNKNOWN = -2,
};

void i2c_init();
void i2c_scan(I2C_TypeDef *I2Cx);

enum I2CRes ssd1306_init(I2C_TypeDef *I2Cx, u8 addr);

#endif // SSD1306_H