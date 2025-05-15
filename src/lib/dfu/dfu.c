#include "dfu.h"

typedef void (*BootloaderFunction)(void);

void dfu_enter_bootloader(void)
{
    NVIC_DisableIRQ(EXTI3_IRQn | EXTI4_IRQn | EXTI9_5_IRQn);
    //RCC_DeInit();

    uint32_t sp = *((uint32_t *)BOOTLOADER_ADDRESS);          // stack pointer, points to top of stack
    uint32_t pc = *((uint32_t *)(BOOTLOADER_ADDRESS + 4));    // program counter, points to next instruction to execute
    __asm__ volatile ("mv sp, %0\n"
                      "jr %1\n" 
                      :: "r"(sp), "r"(pc));

   //NVIC_SystemReset();

    while (1) {
        asm("nop");
    }
}

