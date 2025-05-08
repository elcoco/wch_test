#include "rotenc.h"

const unsigned char ttable[7][4] = {
    {R_START,    R_CW_BEGIN,  R_CCW_BEGIN, R_START},           // R_START
    {R_CW_NEXT,  R_START,     R_CW_FINAL,  R_START | DIR_CW},  // R_CW_FINAL
    {R_CW_NEXT,  R_CW_BEGIN,  R_START,     R_START},           // R_CW_BEGIN
    {R_CW_NEXT,  R_CW_BEGIN,  R_CW_FINAL,  R_START},           // R_CW_NEXT
    {R_CCW_NEXT, R_START,     R_CCW_BEGIN, R_START},           // R_CCW_BEGIN
    {R_CCW_NEXT, R_CCW_FINAL, R_START,     R_START | DIR_CCW}, // R_CCW_FINAL
    {R_CCW_NEXT, R_CCW_FINAL, R_CCW_BEGIN, R_START},           // R_CCW_NEXT
};

void state_debug(enum RotEncState state)
{
    switch (state) {
        case R_START:
            printf("R_START");
            break;
        case R_CW_FINAL:
            printf("R_CW_FINAL");
            break;
        case R_CW_BEGIN:
            printf("R_CW_BEGIN");
            break;
        case R_CW_NEXT:
            printf("R_CW_NEXT");
            break;
        case R_CCW_BEGIN:
            printf("R_CCW_BEGIN");
            break;
        case R_CCW_FINAL:
            printf("R_CCW_FINAL");
            break;
        case R_CCW_NEXT:
            printf("R_CCW_NEXT");
            break;
        case R_START | DIR_CW:
            printf("CW");
            break;
        case R_START | DIR_CCW:
            printf("CCW");
            break;
    };
}

void re_check(struct RotEnc *enc)
{
    /* Source: http://www.buxtronix.net/2011/10/rotary-encoders-done-properly.html
     *
     * | Pos | A | B |
     * |-----|---|---|
     * | 0   | 0 | 0 |
     * | 1/4 | 1 | 0 |
     * | 1/2 | 1 | 1 |
     * | 3/4 | 0 | 1 |
     * | 1   | 0 | 0 |
     */

    u8 a = GPIO_ReadInputDataBit(ROT_A_PORT, ROT_A_PIN);
    u8 b = GPIO_ReadInputDataBit(ROT_B_PORT, ROT_B_PIN);

    //state_debug(enc->state);
    //printf(" => ");

    enc->state = ttable[enc->state & 0xf][(a << 1) | b];

    if (enc->state & DIR_CW) {
        enc->n_clicks--;
        enc->is_triggered = 1;
        enc->dir = R_DIR_CW;
    }
    else if (enc->state & DIR_CCW) {
        enc->n_clicks++;
        enc->is_triggered = 1;
        enc->dir = R_DIR_CCW;
    }
    //state_debug(enc->state);
    //printf("\n");
}

struct RotEnc re_init()
{
    struct RotEnc enc;
    memset(&enc, 0, sizeof(struct RotEnc));

    GPIO_InitTypeDef ROT_A_InitStructure = {0};
    GPIO_InitTypeDef ROT_B_InitStructure = {0};
    GPIO_InitTypeDef ROT_SW_InitStructure = {0};
    EXTI_InitTypeDef EXTI_InitStructure = {0};
    NVIC_InitTypeDef NVIC_InitStructure = {0};

    // RCC->APB2PCENR - set Alternative Functions IO Clock, GPIO port clock
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO | RCC_APB2Periph_GPIOB, ENABLE);

    GPIO_StructInit(&ROT_SW_InitStructure);
    ROT_SW_InitStructure.GPIO_Pin = ROT_SW_PIN;
    ROT_SW_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(ROT_SW_PORT, &ROT_SW_InitStructure);

    GPIO_StructInit(&ROT_A_InitStructure);
    ROT_A_InitStructure.GPIO_Pin = ROT_A_PIN;
    ROT_A_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(ROT_A_PORT, &ROT_A_InitStructure);

    GPIO_StructInit(&ROT_B_InitStructure);
    ROT_B_InitStructure.GPIO_Pin = ROT_B_PIN;
    ROT_B_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(ROT_B_PORT, &ROT_B_InitStructure);


    /* GPIOB ----> EXTI_Line3 */
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource3);
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource4);

    /* GPIOB ----> EXTI_Line5 */
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource5);

    // Encoder :: Should set bit 0 of EXTI->EXTI_INTFR when externally triggered
    EXTI_StructInit(&EXTI_InitStructure);
    EXTI_InitStructure.EXTI_Line = EXTI_Line3 | EXTI_Line4;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);

    NVIC_InitStructure.NVIC_IRQChannel = EXTI3_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    NVIC_InitStructure.NVIC_IRQChannel = EXTI4_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    // Button
    EXTI_StructInit(&EXTI_InitStructure);
    EXTI_InitStructure.EXTI_Line = EXTI_Line5;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);

    NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    return enc;
}
