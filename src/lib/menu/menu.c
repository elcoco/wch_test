#include "menu.h"

static void menu_debug_rec(struct Menu *menu, uint8_t level);
static struct MenuPool menu_pool_init();
static struct MenuItem* menu_pool_get_item();
static struct Menu* menu_pool_get_menu();


struct MenuPool pool;

static struct MenuPool menu_pool_init()
{
    struct MenuPool pool;
    pool.menus_alloc = 0;
    pool.items_alloc = 0;
    return pool;
}

static struct MenuItem* menu_pool_get_item()
{
    if (pool.items_alloc >= MENU_ITEM_POOL_MAX-1) {
        printf("No more item's... sadge...\n");
        return NULL;
    }
    return &(pool.item_pool[pool.items_alloc++]);
}

static struct Menu* menu_pool_get_menu()
{
    if (pool.menus_alloc >= MENU_POOL_MAX-1) {
        printf("No more menu's... sadge...\n");
        return NULL;
    }
    return &(pool.menu_pool[pool.menus_alloc++]);
}

static void menu_debug_rec(struct Menu *menu, uint8_t level)
/* Recursive iter over menu and printf */
// NOTE: this is broken, fix first or your cat will die!
{
    char spaces[32+1] = "";
    for (int i=0 ; i<level*4 ; i++)
        strncat(spaces, " ", 32);

    struct MenuItem *item = menu->item;
    while (item->next != NULL) {
        printf("%s%s\n", spaces, item->title);
        if (menu_item_is_submenu(item))
            menu_debug_rec(item->sub_menu, level+1);
        item = item->next;
    }
}

void menu_debug(struct Menu *menu)
{
    printf("DEBUG\n");
    menu_debug_rec(menu, 0);
    printf("\n");
}

struct Menu* menu_init(struct Menu *parent)
    /* If menu is root menu, parent must be NULL.
     * Since we're not doing any dynamic memory stuff, the "item" struct will be the "go back to parent"
     * item. Leave as NULL if no parent. */
{
    // Initialize storage for menu items only once
    static uint8_t is_initialized = 0;
    if (!is_initialized) {
        pool = menu_pool_init();
        is_initialized = 1;
    }

    struct Menu *menu;
    if ((menu = menu_pool_get_menu()) == NULL)
        return NULL;

    // head of linked list
    menu->item = NULL;
    menu->n_items = 0;
    menu->prev_pos = -1;

    if (parent) {
        struct MenuItem *item;
        if ((item = menu_add_item(menu, MENU_BACK_STR)) != NULL)
            item->parent = parent;
        else
            printf("Failed to add item\n");
    }
    return menu;
}

struct MenuItem* menu_add_item(struct Menu *menu, const char *title)
{
    struct MenuItem *item;
    if ((item = menu_item_init(title)) == NULL)
        return NULL;

    // Find last menu item in linked list and add new item to it
    if (menu->item) {
        struct MenuItem *item_last = menu->item;
        while (item_last->next != NULL)
            item_last = item_last->next;
        item_last->next = item;
    }
    else {
        menu->item = item;
    }

    item->parent = menu;
    menu->n_items++;
    return item;
}

struct MenuItem* menu_get_nth_item(struct Menu *menu, uint8_t line)
{
    /* Get absolute item in linked list by index. */
    if (line >= menu->n_items)      // OOB
        return NULL;

    struct MenuItem *item = menu->item;
    for (uint8_t i=0 ; i<line; i++) {
        if (item == NULL)
            return NULL;
        item = item->next;
    }
    return item;
}

struct Menu* menu_add_submenu(struct Menu *parent, struct Menu *sub, const char *title)
/* Add a menu item to menu->items containing another menu */
{
    struct MenuItem *item;
    if ((item = menu_add_item(parent, title)) == NULL)
        return NULL;

    item->sub_menu = sub;
    return sub;
}

struct MenuItem* menu_item_init(const char *title)
{
    struct MenuItem *item;
    if ((item = menu_pool_get_item()) == NULL)
        return NULL;

    memset(item, 0, sizeof(struct MenuItem));
    strncpy(item->title, title, MENU_MAX_TITLE);
    item->sub_menu = NULL;
    item->parent = NULL;
    item->next = NULL;
    return item;
}

uint8_t menu_item_is_submenu(struct MenuItem *item)
{
    return item->sub_menu != NULL;
}

struct ViewPort vp_init(uint8_t max_cols, uint8_t max_lines)
{
    struct ViewPort vp;
    vp.line_start = 0;
    vp.line_end = max_lines-1;
    vp.max_lines = max_lines;
    vp.max_cols = max_cols;
    vp.pos = 0;
    return vp;
}

void vp_print(struct ViewPort *vp, struct Menu *menu)
    /* Print out menu to serial, might delete later. */
{
    for (int i=vp->line_start ; i<=vp->line_end ; i++) {
        struct MenuItem *item;
        if ((item = menu_get_nth_item(menu, i)) == NULL)
            break;
        if (i == vp->pos)
            printf("> %d: %s\n", i, item->title);
        else
            printf("  %d: %s\n", i, item->title);
    }
    printf("---------\n");
}

void vp_next(struct ViewPort *vp, struct Menu *menu)
{
    // Is on first line
    if (vp->pos == 0)
        return;

    vp->pos--;
    if (vp->pos < vp->line_start) {
        vp->line_start--;
        vp->line_end--;
    }
}

void vp_prev(struct ViewPort *vp, struct Menu *menu)
{
    // Is on last line
    if (vp->pos == menu->n_items-1)
        return;

    vp->pos++;
    if (vp->pos > vp->line_end) {
        vp->line_start++;
        vp->line_end++;
    }
}

struct MenuItem* vp_get_selected(struct ViewPort *vp, struct Menu *menu)
    /* Return selected menu item from viewport */
{
    return menu_get_nth_item(menu, vp->pos);
}

struct MenuItem* vp_get_line(struct ViewPort *vp, struct Menu *menu, uint8_t line)
    /* Get relative item in linked list by index. */
{
    return menu_get_nth_item(menu, vp->line_start + line);
}

void vp_reset(struct ViewPort *vp, uint8_t new_pos)
    /* Set new_pos as first option in menu, 0 to reset */
{
    vp->pos = new_pos;
    vp->line_start = new_pos;
    vp->line_end = new_pos + vp->max_lines;
}

struct Menu* vp_handle_select(struct ViewPort *vp, struct Menu *menu)
    /* Handle select of menu option, return resulting menu */
{
    struct MenuItem *selected = vp_get_selected(vp, menu);
    if (strncmp(selected->title, MENU_BACK_STR, MENU_MAX_TITLE) == 0) {
        printf("go back\n");
        vp_reset(vp, selected->parent->prev_pos);
        menu->prev_pos = -1;
        return selected->parent;
    }
    else if (menu_item_is_submenu(selected)) {
        printf("go in sub\n");
        menu->prev_pos = vp->pos;
        vp_reset(vp, 0);
        return selected->sub_menu;
    }
    else
        return NULL;
}
