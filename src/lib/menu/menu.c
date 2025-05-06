#include "menu.h"

static void menu_debug_rec(struct Menu *menu, uint8_t level);


struct Menu menu_init()
{
    struct Menu menu;
    menu.n_items = 0;
    for (int i=0 ; i<MENU_MAX_ITEMS ; i++)
        menu.items[i] = NULL;
    return menu;
}

enum MenuStatus menu_add_item(struct Menu *menu, struct MenuItem *item)
{
    if (menu->n_items >= MENU_MAX_ITEMS)
        return MENU_STATUS_OOB;
    menu->items[(menu->n_items)++] = item;
    item->parent = menu;
    return MENU_STATUS_OK;
}

static void menu_debug_rec(struct Menu *menu, uint8_t level)
{
    /* Recursive iter over menu and printf */
    char spaces[32+1] = "";
    for (int i=0 ; i<level ; i++)
        strncat(spaces, " ", 32);

    for (int i=0 ; i<MENU_MAX_ITEMS ; i++) {
        if (menu->items[i] == NULL)
            break;

        printf("%s%s\n", spaces, menu->items[i]->title);
        if (menu_item_is_submenu(menu->items[i]))
            menu_debug_rec(menu->items[i]->sub_menu, level+1);
    }
}

void menu_debug(struct Menu *menu)
{
    printf("\n");
    menu_debug_rec(menu, 0);
    printf("\n");
}

struct MenuItem menu_item_init(const char *title, const char id)
{
    struct MenuItem item;
    item.title = title;
    item.id = id;
    item.sub_menu = NULL;
    item.parent = NULL;
    return item;
}

enum MenuStatus menu_item_add_submenu(struct MenuItem *item, struct Menu *menu)
{
    item->sub_menu = menu;
    return MENU_STATUS_OK;
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

enum MenuStatus vp_print(struct ViewPort *vp, struct Menu *menu)
{
    for (int i=vp->line_start ; i<=vp->line_end ; i++) {
        if (menu->items[i] == NULL)
            break;
        if (i == vp->pos)
            printf("> %d: %s\n", i, menu->items[i]->title);
        else
            printf("  %d: %s\n", i, menu->items[i]->title);
    }
    printf("---------\n");

    return MENU_STATUS_OK;
}

void vp_up(struct ViewPort *vp, struct Menu *menu)
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

void vp_down(struct ViewPort *vp, struct Menu *menu)
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
    return menu->items[vp->pos];
}

struct MenuItem* vp_get_line(struct ViewPort *vp, struct Menu *menu, uint8_t line)
    /* Return nth menu item from viewport */
{
    if (line < 0 || line >= vp->max_lines)
        return NULL;
    return menu->items[vp->line_start + line];
}















