#ifndef ROTENC_H
#define ROTENC_H

#include <ch32v20x.h>
#include "ch32v20x_exti.h"
#include <debug.h>
#include <string.h>

#define ROT_A_PORT GPIOB
#define ROT_A_PIN  GPIO_Pin_3
#define ROT_B_PORT GPIOB
#define ROT_B_PIN  GPIO_Pin_4
#define ROT_SW_PORT GPIOB
#define ROT_SW_PIN  GPIO_Pin_5

#define OLED_ADDR 0x3C
#define DIR_CW 0x10     // Clockwise step.
#define DIR_CCW 0x20    // Anti-clockwise step.

enum RotEncState {
    R_START      = 0x00,
    R_CW_FINAL   = 0x01,
    R_CW_BEGIN   = 0x02,
    R_CW_NEXT    = 0x03,
    R_CCW_BEGIN  = 0x04,
    R_CCW_FINAL  = 0x05,
    R_CCW_NEXT   = 0x06,
};

enum RotDirection {
    R_DIR_NONE,
    R_DIR_CCW,
    R_DIR_CW
};

struct RotEnc {
    u8 n_clicks;
    u8 is_triggered;
    enum RotDirection dir;
    u8 is_pressed;
    enum RotEncState state;
};

void state_debug(enum RotEncState state);
void re_check(struct RotEnc *enc);
struct RotEnc re_init();

#endif // !ROTENC_H
