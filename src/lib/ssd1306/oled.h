#ifndef OLED_H
#define OLED_H

#include <debug.h>
#include <string.h>
#include "i2c.h"
#include "oled_fonts.h"
#include <stdarg.h>
#include <core_riscv.h>    // types u8, s8 etc

#define OLED_MAX_PRINTF_BUF 64      // Used in oled_printf()

#define OLED_XSIZE 128
#define OLED_YSIZE  64
#define OLED_BUF_SIZE (OLED_XSIZE*OLED_YSIZE)/8  // Size of display buffer in bytes
#define OLED_NPAGES  8
#define OLED_XCHARS OLED_XSIZE / OLED_FONT_WIDTH

/* OLED commands from manual */
#define OLED_CTRL_CMD                   0x00  // Control char CMD
#define OLED_CTRL_DAT                   0x60  // Control char DATA

#define OLED_CMD_ENTIRE_DISP_ON         0xA4
#define OLED_CMD_ENTIRE_DISP_OFF        0xA3

#define OLED_CMD_DISP_ON                0xAF
#define OLED_CMD_DISP_OFF               0xAE
#define OLED_CMD_CONTRAST_CTRL          0x81

#define OLED_CMD_DISPL_OFFSET           0xD3
#define OLED_CMD_START_LINE             0x40

#define OLED_CMD_SEGMENT_REMAP_0_TO_0   0xA0
#define OLED_CMD_SEGMENT_REMAP_0_TO_127 0xA1  // Reverse column mapping

#define OLED_CMD_COM_DIR_NORMAL         0xC0
#define OLED_CMD_COM_DIR_REVERSED       0xC8  // Reverse rows

#define OLED_CMD_DISP_NORMAL            0xA6
#define OLED_CMD_DISP_INVERTED          0xA7  // Reverse colors

#define OLED_CMD_COM_HW_CFG             0xDA

// Only for hor/ver addressing mode
#define OLED_CMD_PAGE_ADDRESS           0x20
#define OLED_SET_PAGE_ADDRESS_32        0x03
#define OLED_SET_PAGE_ADDRESS_64        0x07

#define OLED_CMD_DISP_CLK_DIV           0xD5

#define OLED_CMD_CHARGE_PUMP            0x8D
#define OLED_SET_CHARGE_PUMP_ENABLE     0x14

#define OLED_CMD_MEMORY_ADDR_MODE       0x20
#define OLED_SET_HOR_ADDR_MODE          0x00  // Horizontal addressing

#define OLED_CMD_MULTIPLEX_RATIO        0xA8
#define OLED_CMD_COL_START_ADDR         0x21
/* End commands from manual */


// Calculate pixel coordinates to buffer pixel position
#define OLED_XY_TO_I(X,Y) (Y * (OLED_XSIZE) + X * OLED_FONT_WIDTH)


struct Oled {
    u8 buf[OLED_BUF_SIZE];   // Display buffer
    u8 *c_bufp;              // Pointer to current position in display buffer
    I2C_TypeDef *i2cx;       // I2C1 or I2C2
    u8 addr;                 // Probably 0xC3
    u8 is_inverted;          // Indicate display inverted colors
};

enum OledStatus {
    OLED_STATUS_SUCCESS =  0,
    OLED_STATUS_ERR_I2C = -1,
    OLED_STATUS_ERR_OOB = -2,
    OLED_STATUS_ERR_OVERFLOW = -3,
};

enum OledStatus oled_init(struct Oled *oled, I2C_TypeDef *I2Cx, u8 addr);
enum OledStatus oled_set_onoff(struct Oled *oled, u8 value);
enum OledStatus oled_set_contrast(struct Oled *oled, u8 value);
enum OledStatus oled_flush(struct Oled *oled);
enum OledStatus oled_set_chr(struct Oled *oled, u8 c);
enum OledStatus oled_set_pos(struct Oled *oled, u8 x, u8 y);
enum OledStatus oled_printf(struct Oled *oled, u8 x, u8 y, const char *fmt, ...);
enum OledStatus oled_set_inverted(struct Oled *oled, u8 value);
enum OledStatus oled_set_px(struct Oled *oled, u8 x, u8 y);
void oled_clear(struct Oled *oled);
void oled_clear_line(struct Oled *oled, u8 y);

#endif // OLED_H
