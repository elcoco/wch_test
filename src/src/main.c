//#include "ch32v20x_gpio.h"
#include <debug.h>
#include <ch32v20x.h>
#include <stdlib.h>
#include <string.h>
#include <ch32v20x_gpio.h>
#include "oled.h"
#include "i2c.h"



#define ROT_A_PORT GPIOB
#define ROT_A_PIN  GPIO_Pin_3

#define ROT_B_PORT GPIOB
#define ROT_B_PIN  GPIO_Pin_4

#define ROT_SW_PORT GPIOB
#define ROT_SW_PIN  GPIO_Pin_5

#define OLED_ADDR 0x3C


struct RotEnc {
    u8 n_clicks;
    u8 is_triggered;
    u8 is_pressed;
} enc0;



void rotenc_init()
{
    memset(&enc0, 0, sizeof(struct RotEnc));

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

    /* GPIOB ----> EXTI_Line5 */
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource5);

    // Should set bit 0 of EXTI->EXTI_INTFR when externally triggered
    EXTI_InitStructure.EXTI_Line = EXTI_Line3 | EXTI_Line5;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);

    NVIC_InitStructure.NVIC_IRQChannel = EXTI3_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

}

int main()
{
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
    SystemCoreClockUpdate();
    Delay_Init();
    USART_Printf_Init(115200);


    rotenc_init();

    i2c_init(I2C1, GPIO_Pin_6, GPIO_Pin_7); // SCL, SDA
    //i2c_scan(I2C1);

    struct Oled oled;
    if (oled_init(&oled, I2C1, OLED_ADDR) < OLED_STATUS_SUCCESS)
        printf("Failed to init display\n");

    printf("Start\n");

    Delay_Ms(1000);
    oled_set_contrast(&oled, 50);
    oled_clear(&oled);

    u8 num = 66;
    oled_printf(&oled, 0, 7, "Disko!!! %d 123456789", num);
    oled_flush(&oled);

    while(1) {
        if (enc0.is_triggered) {
            enc0.is_triggered = 0;
            printf("enc0: %lu\n", enc0.n_clicks);
        }
        if (enc0.is_pressed) {
            enc0.is_pressed = 0;
            printf("clicked\n");
        }

        Delay_Ms(100);
    }
}

// WCH-Interrupt-fast enables saving/restoring hardware registers when entering/leaving
// interrupt (Hardware Preamble/Epilogue HPE)
void NMI_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void HardFault_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void EXTI3_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void EXTI9_5_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));

void NMI_Handler(void)
{
    /* Called when non ignorable event happened */
    printf("NMI MF\n");
}

void HardFault_Handler(void)
{
    printf("Hardfault MF\n");
}

void EXTI9_5_IRQHandler(void)
{
    if (EXTI_GetITStatus(EXTI_Line5) != RESET) {
        EXTI_ClearITPendingBit(EXTI_Line5);     /* Clear Flag */
        enc0.is_pressed = 1;
    }
}

void EXTI3_IRQHandler(void)
{
    if (EXTI_GetITStatus(EXTI_Line3) != RESET) {
        enc0.is_triggered = 1;

        //printf("Run at EXTI\n");
        if (GPIO_ReadInputDataBit(ROT_B_PORT, ROT_B_PIN))
            enc0.n_clicks = (enc0.n_clicks > 0) ? enc0.n_clicks - 1 : 0;
        else
            enc0.n_clicks = (enc0.n_clicks < 255) ? enc0.n_clicks + 1 : 255;

        EXTI_ClearITPendingBit(EXTI_Line3);     /* Clear Flag */
    }
}