//#include "ch32v20x_gpio.h"
#include <debug.h>
#include <ch32v20x.h>
#include <stdlib.h>
#include <string.h>
#include <ch32v20x_gpio.h>
#include "ch32v20x_exti.h"

#include "oled.h"
#include "i2c.h"
#include "rotenc.h"
#include "menu.h"
#include "dfu.h"
#include "millis.h"

struct RotEnc enc0;
struct Oled oled;
struct Menu *root;
struct ViewPort vp;

// Storage for menu library
#define MENU_POOL_MAX 5
#define ITEM_POOL_MAX 32

struct MenuItem item_pool[ITEM_POOL_MAX];
struct Menu menu_pool[MENU_POOL_MAX];

// WCH-Interrupt-fast enables saving/restoring hardware registers when entering/leaving
// interrupt (Hardware Preamble/Epilogue HPE)
void NMI_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void HardFault_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void EXTI3_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void EXTI4_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void EXTI9_5_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));

void on_item_clicked_cb(struct MenuItem *self);
void setup_menu(struct Menu *root);


void setup_menu(struct Menu *root)
{
    printf("Init menu\n");

    struct Menu *sub0 = menu_init(root);
    menu_add_item(sub0, "sub0_item0", &on_item_clicked_cb);
    menu_add_item(sub0, "sub0_item1", &on_item_clicked_cb);
    menu_add_item(sub0, "sub0_item2", &on_item_clicked_cb);

    menu_add_item(root, "item0", &on_item_clicked_cb);
    menu_add_item(root, "item1", &on_item_clicked_cb);
    menu_add_item(root, "item2_abcdefghijklmnopqrstuvwxyz", &on_item_clicked_cb);
    menu_add_submenu(root, sub0, "bever sub");
    menu_add_item(root, "item3", &on_item_clicked_cb);
    menu_add_item(root, "item4", &on_item_clicked_cb);
    menu_add_item(root, "item5", &on_item_clicked_cb);
    menu_add_item(root, "item6", &on_item_clicked_cb);
    menu_add_item(root, "item7 lkjlkllllllllllllllllllllllllll", &on_item_clicked_cb);
    menu_add_item(root, "item8", &on_item_clicked_cb);
    menu_add_item(root, "item9", &on_item_clicked_cb);
}

void oled_print_menu(struct Oled *oled, struct ViewPort *vp, struct Menu *menu)
{
    for (int i=0 ; i<vp->max_lines ; i++) {
        struct MenuItem *item = vp_get_line(vp, menu, i);
        if (item) {
            oled_clear_line(oled, i);
            if (vp_get_selected(vp, menu) == item)
                oled_printf(oled, 0, i, ">%s", item->title);
            else
                oled_printf(oled, 0, i, " %s", item->title);
        }
    }
}

void on_item_clicked_cb(struct MenuItem *self)
{
    printf("Clicked: %s\n", self->title);
    oled_clear(&oled);
    oled_printf_centered(&oled, 0, 4, "CLICKED %s MF!", self->title);
    oled_flush(&oled);

    Delay_Ms(2000);

    oled_clear(&oled);
    oled_print_menu(&oled, &vp, root);
    oled_flush(&oled);
}

char* graph_bar(float value, float low, float high, char lchar, char rchar, char *buf, size_t length)
    /* Get a nice graph bar that we can display when a menu item is selected */
{
    char *ptr = buf;
    int8_t level = (value / (high-low)) * (length-2);
    int i;

    *ptr++ = '[';

    for (i=0 ; i<level && i<length ; i++, ptr++)
        *ptr = lchar;
    for (; i<length-2 ; i++, ptr++)
        *ptr = rchar;

    *ptr++ = ']';
    *ptr = '\0';
    return buf;
}

void check_boot()
{
    GPIO_InitTypeDef ROT_SW_InitStructure = {0};

    // RCC->APB2PCENR - set Alternative Functions IO Clock, GPIO port clock
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    GPIO_StructInit(&ROT_SW_InitStructure);
    ROT_SW_InitStructure.GPIO_Pin = ROT_SW_PIN;
    ROT_SW_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(ROT_SW_PORT, &ROT_SW_InitStructure);

    printf("Checking DFU\n");

    u32 t_start = millis();

    while (!GPIO_ReadInputDataBit(ROT_SW_PORT, ROT_SW_PIN)) {
        if (millis() - t_start > 1000) {
            printf("Triggered DFU boot\n");

            while (!GPIO_ReadInputDataBit(ROT_SW_PORT, ROT_SW_PIN))
                asm("nop");
            printf("Released\n");
            Delay_Ms(500);

        }
    }
}

int main()
{
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
    SystemCoreClockUpdate();

    Delay_Init();
    USART_Printf_Init(115200);

    millis_init();
    check_boot();

    char buf[12+1] = "";
    graph_bar(33, 0, 100, 'x', '.', buf, 12);
    printf("%d%% %s\n", 33, buf);

    re_init(&enc0);

    pool_init(menu_pool, MENU_POOL_MAX, item_pool, ITEM_POOL_MAX);
    root = menu_init(NULL);
    vp = vp_init(10, 7);
    setup_menu(root);
    menu_debug(root);

    i2c_init(I2C1, GPIO_Pin_6, GPIO_Pin_7, I2C_GPIO_PORT); // SCL, SDA

    if (oled_init(&oled, I2C1, OLED_ADDR) < OLED_STATUS_SUCCESS)
        printf("Failed to init display\n");

    printf("Start\n");

    Delay_Ms(1000);
    oled_set_contrast(&oled, 50);
    oled_clear(&oled);
    oled_print_menu(&oled, &vp, root);

    oled_flush(&oled);

    struct Menu *menu = root;
    u8 prev_clicks = 0;

    while(1) {
        if (enc0.is_triggered) {
            enc0.is_triggered = 0;
            switch (enc0.dir) {
                case R_DIR_CW:
                    for (int i=0 ; i<prev_clicks-enc0.n_clicks ; i++)
                        vp_next(&vp, menu);

                    vp_debug(&vp, menu);
                    break;
                case R_DIR_CCW:
                    for (int i=0 ; i<enc0.n_clicks-prev_clicks ; i++)
                        vp_prev(&vp, menu);
                    vp_debug(&vp, menu);
                    break;
            }
            printf("enc0: %d\n", enc0.n_clicks);
            prev_clicks = enc0.n_clicks;
        }
        if (enc0.is_pressed) {
            enc0.is_pressed = 0;
            printf("clicked\n");

            struct Menu *sel;
            if ((sel = vp_handle_clicked(&vp, menu)) != NULL) {
                menu = sel;
                //vp_reset(&vp);
            }
            else {
                printf("execute\n");
            }
        }

        oled_clear(&oled);
        oled_print_menu(&oled, &vp, menu);
        oled_flush(&oled);

        Delay_Ms(10);
    }
}


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
        enc0.is_pressed = 1;
    }
    EXTI_ClearITPendingBit(EXTI_Line5);     /* Clear Flag */
}

void EXTI3_IRQHandler(void)
{
    if (EXTI_GetITStatus(EXTI_Line3) != RESET) {
        re_check(&enc0);
    }
    EXTI_ClearITPendingBit(EXTI_Line3);     /* Clear Flag */
}

void EXTI4_IRQHandler(void)
{
    if (EXTI_GetITStatus(EXTI_Line4) != RESET) {
        re_check(&enc0);
    }
    EXTI_ClearITPendingBit(EXTI_Line4);     /* Clear Flag */
}
