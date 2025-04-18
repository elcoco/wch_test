//#include "ch32v20x_gpio.h"
#include "ch32v20x_gpio.h"
#include <debug.h>
#include <ch32v20x.h>
#include <stdlib.h>


enum RotDir {
    ROT_DIR_LEFT,
    ROT_DIR_RIGHT,
    ROT_DIR_UNDEFINED
};


#define ROT_A_PORT GPIOB
#define ROT_A_PIN  GPIO_Pin_3

#define ROT_B_PORT GPIOB
#define ROT_B_PIN  GPIO_Pin_4

#define ROT_SW_PORT GPIOB
#define ROT_SW_PIN  GPIO_Pin_5



struct RotEnc {
    GPIO_InitTypeDef a;
    GPIO_InitTypeDef b;
    enum RotDir dir;
} enc0;


void EXTI0_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));

void EXTI0_IRQHandler(void)
{
    printf("Triggered\n");

    // Should set bit 0 of EXTI->EXTI_INTFR when triggered

    if (EXTI_GetITStatus(EXTI_Line0) != RESET) {
        printf("Run at EXTI\n");
        EXTI_ClearITPendingBit(EXTI_Line0);     /* Clear Flag */
    }
}

void setup_GPIO()
{
    GPIO_InitTypeDef ROT_A_InitStructure = {0};
    //GPIO_InitTypeDef ROT_B_InitStructure = {0};
    //GPIO_InitTypeDef ROT_SW_InitStructure = {0};
    EXTI_InitTypeDef EXTI_InitStructure = {0};
    NVIC_InitTypeDef NVIC_InitStructure = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO | RCC_APB2Periph_GPIOB, ENABLE);

    //GPIO_StructInit(&ROT_SW_InitStructure);
    //ROT_SW_InitStructure.GPIO_Pin = ROT_SW_PIN;
    //ROT_SW_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    //GPIO_Init(ROT_SW_PORT, &ROT_SW_InitStructure);

    GPIO_StructInit(&ROT_A_InitStructure);
    ROT_A_InitStructure.GPIO_Pin = ROT_A_PIN;
    ROT_A_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(ROT_A_PORT, &ROT_A_InitStructure);

    //GPIO_StructInit(&ROT_B_InitStructure);
    //ROT_B_InitStructure.GPIO_Pin = ROT_B_PIN;
    //ROT_B_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    //GPIO_Init(ROT_B_PORT, &ROT_B_InitStructure);


    /* GPIOB ----> EXTI_Line0 */
    //GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource3);

    EXTI_InitStructure.EXTI_Line = EXTI_Line0;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);

    NVIC_InitStructure.NVIC_IRQChannel = EXTI0_IRQn;
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

    setup_GPIO();


    // RCC_APB2PCENR.IOPBEN, p54



    printf("SystemClk:%d\r\n", SystemCoreClock);
    printf( "ChipID:%08x\r\n", DBGMCU_GetCHIPID() );
    printf("This is printf example\r\n");

    while(1) {
        Delay_Ms(100);

        //u8 state = GPIO_ReadInputDataBit(GPIOB, pin_rotsw.GPIO_Pin);
        //u8 sw_state = GPIO_ReadInputDataBit(ROT_SW_PORT, ROT_SW_PIN);
        //u8 a_state = GPIO_ReadInputDataBit(ROT_A_PORT, ROT_A_PIN);
        

        
        
    }
}
