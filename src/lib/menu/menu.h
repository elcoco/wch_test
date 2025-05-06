#ifndef XMENU_H
#define XMENU_H

#include <stdint.h>
#include <string.h>
#include <stdio.h>

#define MENU_MAX_ITEMS 20
#define MENU_MAX_TITLE 32

/* A viewport gives a view on the menu data. It handles navigating the menu
 * and maintains a position */
struct ViewPort {
    uint8_t line_start;     // i of first visible line
    uint8_t line_end;       // i of last visible line
    uint8_t pos;         // Current position in menu
    uint8_t max_cols;   // Max menu item length
    uint8_t max_lines;   // Max visible lines
};

struct Menu {
    struct MenuItem *items[MENU_MAX_ITEMS];
    uint8_t n_items;
};

struct MenuItem {
    const char *title;
    char id;
    struct Menu *sub_menu;      // menu item can be a submenu
    struct Menu *parent;
};

enum MenuStatus {
    MENU_STATUS_OK    = 0,
    MENU_STATUS_OOB   = -1,
    MENU_STATUS_ERROR = -2
};

struct Menu menu_init();
enum MenuStatus menu_add_item(struct Menu *menu, struct MenuItem *item);
void menu_debug(struct Menu *menu);

struct MenuItem menu_item_init(const char *title, const char id);
enum MenuStatus menu_item_add_submenu(struct MenuItem *item, struct Menu *menu);
uint8_t menu_item_is_submenu(struct MenuItem *item);

struct ViewPort vp_init(uint8_t max_cols, uint8_t max_lines);
enum MenuStatus vp_print(struct ViewPort *vp, struct Menu *menu);

void vp_up(struct ViewPort *vp, struct Menu *menu);
void vp_down(struct ViewPort *vp, struct Menu *menu);
struct MenuItem* vp_get_selected(struct ViewPort *vp, struct Menu *menu);
struct MenuItem* vp_get_line(struct ViewPort *vp, struct Menu *menu, uint8_t line);

#endif /* ifndef MENU_H */
