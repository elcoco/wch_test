#include "oled.h"

static enum OledStatus oled_write_cmd(struct Oled *oled, u8 cmd);

static enum OledStatus oled_write_cmd(struct Oled *oled, u8 cmd)
{
    /* Write a command to Oled.
       Control byte + command byte */
    enum I2CStatus res;

    if ((res = i2c_start_tx(oled->i2cx, oled->addr)) < I2C_STATUS_SUCCESS)
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
    oled->is_inverted = 0;
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

enum OledStatus oled_set_inverted(struct Oled *oled, u8 value)
{
    oled->is_inverted = value;
    return oled_write_cmd(oled, (value) ? OLED_CMD_DISP_INVERTED : OLED_CMD_DISP_NORMAL);
}

enum OledStatus oled_set_contrast(struct Oled *oled, u8 value)
{
    /* Set contrast in 256 steps */
    enum OledStatus res;
    if ((res = oled_write_cmd(oled, OLED_CMD_CONTRAST_CTRL)) < OLED_STATUS_SUCCESS)
        return res;
    return oled_write_cmd(oled, value);
}

enum OledStatus oled_set_px(struct Oled *oled, u8 x, u8 y)
{
    /* Set pixel by pixel xy coordinates. */
    // NOTE: Doesn't work yet!

    // find page
    // find char in page
    // find position in char
    // set bit in char in buffer
    u8 page = y / OLED_FONT_HEIGHT;
    u8 chr_ypos = y % page;
    u8 col = x / OLED_FONT_WIDTH;
    u8 chr_xpos = x % col;

    printf("Setting pixel: %d x %d\n", x, y);
    printf("page: %d\n", page);
    printf("ypos: %d\n", chr_ypos);
    printf("col: %d\n", col);
    printf("chr_xpos: %d\n", chr_xpos);

    //oled->buf[i] |= (0x01 << pxl);
}

enum OledStatus oled_set_pos(struct Oled *oled, u8 x, u8 y)
{
    /* Set pointer to buffer to the right position so we can print a char.
     * Position is in chars.
     */
    oled->c_bufp = oled->buf + OLED_XY_TO_I(x, y);
    return OLED_STATUS_SUCCESS;
}

void oled_clear(struct Oled *oled)
{
    memset(oled->buf, 0x00, OLED_BUF_SIZE);
}

void oled_clear_line(struct Oled *oled, u8 y)
{
    for (u8 c=0 ; c<OLED_XCHARS ; c++) {
        for (u8 x=0 ; x<OLED_FONT_WIDTH ; x++)
            oled->buf[OLED_XY_TO_I(c,y) + x] = 0x00;
    }
}

enum OledStatus oled_set_chr(struct Oled *oled, u8 c)
{
    c -= OLED_ASCII_OFFSET;

    for (u8 i=0 ; i<OLED_FONT_WIDTH ; i++)
        *oled->c_bufp++ = font6x8[c][i];

    return OLED_STATUS_SUCCESS;
}

enum OledStatus oled_print(struct Oled *oled, u8 x, u8 y, const char *data)
{
    oled_set_pos(oled, x, y);
    for (char *p=data ; *p!='\0' ; p++) {

        // Do not wrap to next line
        if (x + (p-data) >= OLED_XCHARS)
            return OLED_STATUS_ERR_OVERFLOW;

        oled_set_chr(oled, *p);
    }
    return OLED_STATUS_SUCCESS;
}

enum OledStatus oled_printf(struct Oled *oled, u8 x, u8 y, const char *fmt, ...)
{
    char buf[OLED_MAX_PRINTF_BUF] = "";
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, OLED_MAX_PRINTF_BUF-1, fmt, args);
    va_end(args);
    return oled_print(oled, x, y, buf);
}

enum OledStatus oled_printf_centered(struct Oled *oled, u8 x, u8 y, const char *fmt, ...)
{
    char buf[OLED_MAX_PRINTF_BUF] = "";
    char buf_centered[OLED_MAX_PRINTF_BUF] = "";

    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, OLED_MAX_PRINTF_BUF-1, fmt, args);
    va_end(args);

    if (strnlen(buf, OLED_MAX_PRINTF_BUF) >= OLED_XCHARS)
        return oled_print(oled, x, y, buf);

    int8_t lspaces = (OLED_XCHARS - strnlen(buf, OLED_MAX_PRINTF_BUF))/2;
    for (int i=0 ; i<lspaces ; i++)
        buf_centered[i] = ' ';

    strncat(buf_centered, buf, OLED_XCHARS);
    return oled_print(oled, x, y, buf_centered);
}

enum OledStatus oled_flush(struct Oled *oled)
{
    enum I2CStatus res;

    // Reset col pointer
    oled_write_cmd(oled, OLED_CMD_COL_START_ADDR);
    oled_write_cmd(oled, 0x00);
    oled_write_cmd(oled, 127);

    if ((res = i2c_start_tx(oled->i2cx, oled->addr)) < I2C_STATUS_SUCCESS)
        return OLED_STATUS_ERR_I2C;
        
    i2c_write_byte(oled->i2cx, oled->addr, OLED_CTRL_DAT);
    oled_set_pos(oled, 0, 0);

    for (int i=0 ; i<OLED_BUF_SIZE ; i++)
        i2c_write_byte(oled->i2cx, oled->addr, *oled->c_bufp++);

    i2c_stop(oled->i2cx, oled->addr);
    return OLED_STATUS_SUCCESS;
}
