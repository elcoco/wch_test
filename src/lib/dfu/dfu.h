#ifndef DFU_H
#define DFU_H

#include <ch32v20x.h>
#include "ch32v20x_exti.h"
#include <core_riscv.h>
#include <stdint.h>

#ifdef CH32V203
  #define BOOTLOADER_ADDRESS 0x1FFF8000
#else
  #define BOOTLOADER_ADDRESS 0x1FFF8000
#endif

void dfu_enter_bootloader(void);

#endif // !DFU_H
