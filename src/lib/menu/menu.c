#include "menu.h"

static void menu_debug_rec(struct Menu *menu, uint8_t level);
static void vp_reset(struct ViewPort *vp, uint8_t new_pos);
static struct MenuItem* menu_get_nth_item(struct Menu *menu, uint8_t line);
static struct MenuItem* menu_item_init(const char *title);
static struct MenuItem* item_pool_alloc_item();
static struct Menu* menu_pool_alloc_menu();

// Storage pool for menus and menu items
static struct StoragePool item_pool;
static struct StoragePool menu_pool;


static struct MenuItem* item_pool_alloc_item()
{
    if (item_pool.n_alloc >= item_pool.size) {
        printf("No more items... sadge...\n");
        return NULL;
    }
    return &(item_pool.pool.item_pool[item_pool.n_alloc++]);
}

static struct Menu* menu_pool_alloc_menu()
{
    if (menu_pool.n_alloc >= menu_pool.size) {
        printf("No more menus... sadge...\n");
        return NULL;
    }
    return &(menu_pool.pool.menu_pool[menu_pool.n_alloc++]);
}

static struct MenuItem* menu_get_nth_item(struct Menu *menu, uint8_t line)
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

static struct MenuItem* menu_item_init(const char *title)
{
    struct MenuItem *item;
    if ((item = item_pool_alloc_item()) == NULL)
        return NULL;

    memset(item, 0, sizeof(struct MenuItem));
    //strncpy(item->title, title, MENU_MAX_TITLE);
    item->title = title;
    item->sub_menu = NULL;
    item->parent = NULL;
    item->next = NULL;
    return item;
}

static void vp_reset(struct ViewPort *vp, uint8_t new_pos)
    /* Set new_pos as first option in menu, 0 to reset */
{
    vp->pos = new_pos;
    vp->line_start = new_pos;
    vp->line_end = new_pos + vp->max_lines;
}

static void menu_debug_rec(struct Menu *menu, uint8_t level)
    /* Recursive iter over menu and printf */
{
    // NOTE: I'm going to boldly assume that we're not going more than 8 levels deep.
    char spaces[32+1] = "";         
    for (int i=0 ; i<level*4 ; i++)
        strncat(spaces, " ", 32);

    struct MenuItem *item = menu->item;
    while (item->next != NULL) {
        printf("%s%s\n", spaces, item->title);
        if (item->sub_menu)
            menu_debug_rec(item->sub_menu, level+1);
        item = item->next;
    }
}

void pool_init(struct Menu *mpool, size_t menu_size, struct MenuItem *ipool, size_t item_size)
    /* We don't want to use malloc so we maintain a stack of structs.
     * The caller is responsible for the storage of all menu's and menu items */
{
    menu_pool.pool.menu_pool = mpool;
    menu_pool.n_alloc = 0;
    menu_pool.size = menu_size;

    item_pool.pool.item_pool = ipool;
    item_pool.n_alloc = 0;
    item_pool.size = item_size;
}

struct Menu* menu_init(struct Menu *parent)
    /* If menu is root menu, parent must be NULL.
     * Since we're not doing any dynamic memory stuff, the "item" struct will be the "go back to parent"
     * item. Leave as NULL if no parent. */
{
    struct Menu *menu;
    if ((menu = menu_pool_alloc_menu()) == NULL)
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

void menu_debug(struct Menu *menu)
{
    printf("DEBUG\n");
    menu_debug_rec(menu, 0);
    printf("\n");
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

struct Menu* menu_add_submenu(struct Menu *parent, struct Menu *sub, const char *title)
/* Add a menu item to menu->items containing another menu */
{
    struct MenuItem *item;
    if ((item = menu_add_item(parent, title)) == NULL)
        return NULL;

    item->sub_menu = sub;
    return sub;
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

void vp_debug(struct ViewPort *vp, struct Menu *menu)
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

void vp_prev(struct ViewPort *vp, struct Menu *menu)
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

void vp_next(struct ViewPort *vp, struct Menu *menu)
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
    /* Return selected menu item from viewport.
     * Function could use a more discriptive name. */
{
    return menu_get_nth_item(menu, vp->pos);
}

struct MenuItem* vp_get_line(struct ViewPort *vp, struct Menu *menu, uint8_t line)
    /* Get relative item in linked list by index. */
{
    return menu_get_nth_item(menu, vp->line_start + line);
}

struct Menu* vp_handle_select(struct ViewPort *vp, struct Menu *menu)
    /* Handle select of menu option, return resulting menu */
{
    struct MenuItem *selected = vp_get_selected(vp, menu);
    if (strncmp(selected->title, MENU_BACK_STR, MENU_MAX_TITLE) == 0) {
        vp_reset(vp, selected->parent->prev_pos); // Restore position
        menu->prev_pos = -1;
        return selected->parent;
    }
    else if (selected->sub_menu) {
        menu->prev_pos = vp->pos;                 // Backup position
        vp_reset(vp, 0);
        return selected->sub_menu;
    }
    else
        return NULL;
}
