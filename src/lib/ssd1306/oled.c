#include "oled.h"

static enum OledStatus oled_write_cmd(struct Oled *oled, u8 cmd);

static enum OledStatus oled_write_cmd(struct Oled *oled, u8 cmd)
{
    /* Write a command to Oled.
       Control byte + command byte */
    enum I2CRes res;

    if ((res = i2c_start_tx(oled->i2cx, oled->addr)) < I2C_SUCCESS)
        return OLED_STATUS_ERR_I2C;

    i2c_write_byte(oled->i2cx, oled->addr, OLED_CTRL_CMD);
    i2c_write_byte(oled->i2cx, oled->addr, cmd);

    i2c_stop(oled->i2cx, oled->addr);
    return OLED_STATUS_SUCCESS;
}

enum OledStatus oled_init(struct Oled *oled, I2C_TypeDef *I2Cx, u8 addr)
{
    oled->i2cx = I2Cx;
    oled->addr = addr;
    oled->c_bufp = oled->buf;
    oled_clear(oled);

    // Init sequence from manual p64
    {
        // Set y resolution
        oled_write_cmd(oled, OLED_CMD_MULTIPLEX_RATIO); 
        oled_write_cmd(oled, OLED_YSIZE-1);

        // Set display offset D3h, 00h
        oled_write_cmd(oled, OLED_CMD_DISPL_OFFSET); 
        oled_write_cmd(oled, 0x00);

        //// IS THIS OK????????
        // Set start line 40h
        oled_write_cmd(oled, OLED_CMD_START_LINE); 
        oled_write_cmd(oled, 0x00);

        // Set segment remap A0h/A1h
        //oled_write_cmd(oled, OLED_CMD_SEGMENT_REMAP_0_TO_0); 
        oled_write_cmd(oled, OLED_CMD_SEGMENT_REMAP_0_TO_127); 

        // Set COM output scan direction C0h/C8h
        //oled_write_cmd(oled, OLED_CMD_COM_DIR_NORMAL); 
        oled_write_cmd(oled, OLED_CMD_COM_DIR_REVERSED); 

        // Set COM Pins hardware configuration
        //oled_write_cmd(I2Cx, oled->addr, OLED_CMD_COM_HW_CFG); 
        //oled_write_cmd(I2Cx, oled->addr, cff); 

        oled_set_contrast(oled, 255);
        oled_write_cmd(oled, OLED_CMD_ENTIRE_DISP_ON);

        // Set Normal/inverse display A6h
        oled_write_cmd(oled, OLED_CMD_DISP_NORMAL);

        // Set Osc Frequency D5h, 80h
        oled_write_cmd(oled, OLED_CMD_DISP_CLK_DIV);
        oled_write_cmd(oled, 0x80);

        oled_write_cmd(oled, OLED_CMD_CHARGE_PUMP);
        oled_write_cmd(oled, OLED_SET_CHARGE_PUMP_ENABLE);

        oled_set_onoff(oled, 1);
    }

    oled_write_cmd(oled, OLED_CMD_PAGE_ADDRESS);
    oled_write_cmd(oled, OLED_SET_PAGE_ADDRESS_64);

    oled_write_cmd(oled, OLED_CMD_MEMORY_ADDR_MODE);
    oled_write_cmd(oled, OLED_SET_HOR_ADDR_MODE);

    // Set display pointer to 0
    oled_write_cmd(oled, OLED_CMD_COL_START_ADDR);
    oled_write_cmd(oled, 0x00);
    oled_write_cmd(oled, 127);
    return OLED_STATUS_SUCCESS;
}

enum OledStatus oled_set_onoff(struct Oled *oled, u8 value)
{
    return oled_write_cmd(oled, (value) ? OLED_CMD_DISP_ON : OLED_CMD_DISP_OFF);
}

enum OledStatus oled_set_contrast(struct Oled *oled, u8 value)
{
    /* Set contrast in 256 steps */
    enum OledStatus res;
    if ((res = oled_write_cmd(oled, OLED_CMD_CONTRAST_CTRL)) < OLED_STATUS_SUCCESS)
        return res;
    return oled_write_cmd(oled, value);
}

enum OledStatus oled_set_pos(struct Oled *oled, u8 x, u8 y)
{
    /* Set pointer to buffer to the right position so we can print a char.
     * Position is in chars.
     */
    oled->c_bufp = oled->buf + OLED_XY_TO_I(x, y);
    return OLED_STATUS_SUCCESS;
}

enum OledStatus oled_clear(struct Oled *oled)
{
    memset(oled->buf, 0x00, OLED_BUF_SIZE);
    return OLED_STATUS_SUCCESS;
}

enum OledStatus oled_set_chr(struct Oled *oled, u8 c)
{
    c -= OLED_ASCII_OFFSET;

    for (u8 i=0 ; i<OLED_FONT_WIDTH ; i++)
        *(oled->c_bufp)++ = font6x8[c][i];

    return OLED_STATUS_SUCCESS;
}

enum OledStatus oled_printf(struct Oled *oled, u8 x, u8 y, const char *fmt, ...)
{
    char buf[OLED_MAX_PRINTF_BUF] = "";
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, OLED_MAX_PRINTF_BUF-2, fmt, args);
    va_end(args);

    oled_set_pos(oled, x, y);
    for (char *p=buf ; *p!='\0' ; p++) {

        // Do not wrap to next line
        if (x + (p-buf) >= OLED_XCHARS)
            return OLED_STATUS_ERR_OVERFLOW;

        oled_set_chr(oled, *p);
    }
    return OLED_STATUS_SUCCESS;
}

enum OledStatus oled_flush(struct Oled *oled)
{
    enum I2CRes res;

    // Reset col pointer
    oled_write_cmd(oled, OLED_CMD_COL_START_ADDR);
    oled_write_cmd(oled, 0x00);
    oled_write_cmd(oled, 127);

    if ((res = i2c_start_tx(oled->i2cx, oled->addr)) < I2C_SUCCESS)
        return OLED_STATUS_ERR_I2C;
        
    i2c_write_byte(oled->i2cx, oled->addr, OLED_CTRL_DAT);
    oled_set_pos(oled, 0, 0);

    for (int i=0 ; i<OLED_BUF_SIZE ; i++)
        i2c_write_byte(oled->i2cx, oled->addr, *oled->c_bufp++);

    i2c_stop(oled->i2cx, oled->addr);
    return OLED_STATUS_SUCCESS;
}