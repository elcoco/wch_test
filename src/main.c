#include "debug.h"
#include <ch32v20x.h>
#include <stdlib.h>

int main()
{
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
    SystemCoreClockUpdate();
    Delay_Init();
    USART_Printf_Init(115200);
    printf("SystemClk:%d\r\n", SystemCoreClock);
    printf( "ChipID:%08x\r\n", DBGMCU_GetCHIPID() );
    printf("This is printf example\r\n");

    while(1) {
        printf("This is printf example\r\n");
    }
}
