#ifndef XMENU_H
#define XMENU_H

#include <stdint.h>
#include <string.h>
#include <stdio.h>

#define MENU_MAX_TITLE 32
#define MENU_BACK_STR "<-- back"

// Create a pool of items that is used to allocate space for menu items.
// This way we don't have to reserve a lot of memory for every menu.
#define MENU_ITEM_POOL_MAX 20 
#define MENU_POOL_MAX 5 

/* A viewport gives a view on the menu data. It handles navigating the menu
 * and maintains a position */
struct ViewPort {
    uint8_t line_start;     // Absolute i of first visible line
    uint8_t line_end;       // Absolute i of last visible line
    uint8_t pos;            // Current absolute position in menu
    uint8_t max_cols;       // Max menu item length
    uint8_t max_lines;      // Max visible lines
};

struct Menu {
    struct MenuItem *item;    // Linked list head
    uint8_t n_items;          // Amount of items in linked list
    int8_t prev_pos;          // Previous position in menu. This value is 
                              // used to return to previous position when going one level up
};

struct MenuItem {
    const char *title;
    struct MenuItem *next;      // Pointer to next item in linked list
    struct Menu *sub_menu;      // Menu item can contain a submenu
    struct Menu *parent;        // Pointer to parent menu
};

struct StoragePool {
    union {
        struct Menu *menu_pool;      // in case of menu pool
        struct MenuItem *item_pool;  // in case of menu item pool
    } pool;
    size_t n_alloc;         // Amount of allocated structs in pool
    size_t size;
};

void pool_init(struct Menu *mpool, size_t menu_size, struct MenuItem *ipool, size_t item_size);

struct Menu* menu_init(struct Menu *parent);
struct Menu* menu_add_submenu(struct Menu *parent, struct Menu *sub, const char *title);
struct MenuItem* menu_add_item(struct Menu *menu, const char *title);
void menu_debug(struct Menu *menu);

/* ViewPort provides a view on the menu data */
struct ViewPort vp_init(uint8_t max_cols, uint8_t max_lines);
void vp_debug(struct ViewPort *vp, struct Menu *menu);

struct MenuItem* vp_get_selected(struct ViewPort *vp, struct Menu *menu);
struct MenuItem* vp_get_line(struct ViewPort *vp, struct Menu *menu, uint8_t line);

/* Viewport navigation, go to prev/next position in menu */
void vp_next(struct ViewPort *vp, struct Menu *menu);
void vp_prev(struct ViewPort *vp, struct Menu *menu);
struct Menu* vp_handle_select(struct ViewPort *vp, struct Menu *menu);

#endif /* ifndef MENU_H */
