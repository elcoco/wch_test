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

struct RotEnc enc0;

struct MenuItem item0;
struct MenuItem item1;
struct MenuItem item2;
struct MenuItem item3;
struct MenuItem item4;
struct MenuItem item5;
struct MenuItem item6;
struct MenuItem item7;
struct MenuItem item8;
struct MenuItem item9;

struct Menu sub0;

struct MenuItem sub0_item0;
struct MenuItem sub0_item1;
struct MenuItem sub0_item2;

// WCH-Interrupt-fast enables saving/restoring hardware registers when entering/leaving
// interrupt (Hardware Preamble/Epilogue HPE)
void NMI_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void HardFault_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void EXTI3_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void EXTI4_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void EXTI9_5_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));


void setup_menu(struct Menu *root)
{
    printf("Init menu\n");
    item0 = menu_item_init("Item 0", '0');
    item1 = menu_item_init("Item 1", '1');
    item2 = menu_item_init("Item 2", '2');
    item3 = menu_item_init("Item 3", '3');
    item4 = menu_item_init("Item 4", '4');
    item5 = menu_item_init("Item 5", '5');
    item6 = menu_item_init("Item 6", '6');
    item7 = menu_item_init("Item 7", '7');
    item8 = menu_item_init("Item 8", '8');
    item9 = menu_item_init("Item 9", '9');

    menu_add_item(root, &item0);
    menu_add_item(root, &item1);
    menu_add_item(root, &item2);
    menu_add_item(root, &item3);
    menu_add_item(root, &item4);
    menu_add_item(root, &item5);
    menu_add_item(root, &item6);
    menu_add_item(root, &item7);
    menu_add_item(root, &item8);
    menu_add_item(root, &item9);

    sub0 = menu_init();
    menu_item_add_submenu(&item2, &sub0);

    sub0_item0 = menu_item_init("sub0_Item 0", '0');
    sub0_item1 = menu_item_init("sub0_Item 1", '1');
    sub0_item2 = menu_item_init("sub0_Item 2", '2');

    menu_add_item(&sub0, &sub0_item0);
    menu_add_item(&sub0, &sub0_item1);
    menu_add_item(&sub0, &sub0_item2);
}

void oled_print_menu(struct Oled *oled, struct ViewPort *vp, struct Menu *menu)
{
    for (int i=0 ; i<vp->max_lines ; i++) {
        struct MenuItem *item = vp_get_line(vp, menu, i);
        if (item) {
            oled_clear_line(oled, i);
            if (vp_get_selected(vp, menu) == item)
                oled_printf(oled, 0, i, "> %s", item->title);
            else
                oled_printf(oled, 0, i, "  %s", item->title);
        }
    }
}

struct Menu* on_menu_item_clicked(struct ViewPort *vp, struct Menu *menu)
{
    struct MenuItem *selected = vp_get_selected(vp, menu);
    if (menu_item_is_submenu(selected)) {
        return selected->sub_menu;
    }
    return NULL;
}

int main()
{
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
    SystemCoreClockUpdate();
    Delay_Init();
    USART_Printf_Init(115200);

    re_init(&enc0);

    struct Menu root = menu_init();
    struct ViewPort vp = vp_init(10, 8);
    setup_menu(&root);
    menu_debug(&root);

    i2c_init(I2C1, GPIO_Pin_6, GPIO_Pin_7, I2C_GPIO_PORT); // SCL, SDA

    struct Oled oled;
    if (oled_init(&oled, I2C1, OLED_ADDR) < OLED_STATUS_SUCCESS)
        printf("Failed to init display\n");

    printf("Start\n");

    Delay_Ms(1000);
    oled_set_contrast(&oled, 50);
    oled_clear(&oled);

    //oled_set_px(&oled, 60, 50);


    //u8 num = 66;
    //oled_printf(&oled, 0, 0, "Disko!!! %d 123456789", num);
    //oled_flush(&oled);

    //oled_printf(&oled, 0, 0, "bever!", num);
    //oled_printf(&oled, 0, 3, "rot: %d", enc0.n_clicks);
    //oled_flush(&oled);
    oled_print_menu(&oled, &vp, &root);
    oled_flush(&oled);

    struct Menu *menu = &root;

    while(1) {
        if (enc0.is_triggered) {
            enc0.is_triggered = 0;
            switch (enc0.dir) {
                case R_DIR_CW:
                    vp_down(&vp, menu);
                    vp_print(&vp, menu);
                    break;
                case R_DIR_CCW:
                    vp_up(&vp, menu);
                    vp_print(&vp, menu);
                    break;
            }
            printf("enc0: %d\n", enc0.n_clicks);
            oled_print_menu(&oled, &vp, menu);
            oled_flush(&oled);
            //oled_clear_line(&oled, 3);
            //oled_printf(&oled, 0, 3, "rot: %d", enc0.n_clicks);
            //oled_flush(&oled);
        }
        if (enc0.is_pressed) {
            enc0.is_pressed = 0;
            printf("clicked\n");

            struct Menu *sel;
            if ((sel = on_menu_item_clicked(&vp, menu)) != NULL)
                menu = sel;
        }

        Delay_Ms(100);
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
        EXTI_ClearITPendingBit(EXTI_Line5);     /* Clear Flag */
        enc0.is_pressed = 1;
    }
}

volatile u8 a_prev = -1;


void EXTI3_IRQHandler(void)
{

    if (EXTI_GetITStatus(EXTI_Line3) != RESET) {
        //printf("A\n");
        re_check(&enc0);

        //static int oldClock = -1; // Initialize to an impossible value.
        //u8 a = GPIO_ReadInputDataBit(ROT_A_PORT, ROT_A_PIN);
        //u8 b = GPIO_ReadInputDataBit(ROT_B_PORT, ROT_B_PIN);
        //if(a == a_prev)
        //    return; // was a bounce. Don't count this.
        //            //
        //enc0.is_triggered = 1;

        //if(a ^ b) {
        //      // clockwise move
        //    enc0.n_clicks = (enc0.n_clicks > 0) ? enc0.n_clicks - 1 : 0;
        //} else {
        //   // counterclockwise move
        //    enc0.n_clicks = (enc0.n_clicks < 255) ? enc0.n_clicks + 1 : 255;
        //}
        //a_prev = a; // store clock state for debounce check.





        //u8 a = GPIO_ReadInputDataBit(ROT_A_PORT, ROT_A_PIN);
        //u8 b = GPIO_ReadInputDataBit(ROT_B_PORT, ROT_B_PIN);
        //if (!a) {
        //    enc0.is_triggered = 1;
        //    if (b)
        //        enc0.n_clicks = (enc0.n_clicks > 0) ? enc0.n_clicks - 1 : 0;
        //    else
        //        enc0.n_clicks = (enc0.n_clicks < 255) ? enc0.n_clicks + 1 : 255;
        //}

    }
    EXTI_ClearITPendingBit(EXTI_Line3);     /* Clear Flag */
}

void EXTI4_IRQHandler(void)
{
    if (EXTI_GetITStatus(EXTI_Line4) != RESET) {
        //printf("B\n");
        re_check(&enc0);
        EXTI_ClearITPendingBit(EXTI_Line4);     /* Clear Flag */
    }
}
