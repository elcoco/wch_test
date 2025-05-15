#include "sconf.h"

uint16_t cfg_crc16(uint8_t* data, uint8_t length)
{
    /* create 2 bytes crc16 from array */
    uint8_t x;
    uint16_t crc = 0xFFFF;

    while (length--) {
        x = crc >> 8 ^ *data++;
        x ^= x>>4;
        crc = (crc << 8) ^ ((uint16_t)(x << 12)) ^ ((uint16_t)(x <<5)) ^ ((uint16_t)x);
    }
    return crc;
}

struct CFGItem* cfg_get_item(struct CFG* cfg, ItemID index)
{
    struct CFGItem* item = cfg->items;
    for (uint16_t i=0 ; i<=cfg->items_size ; i++, item++) {
        if (item->id == index)
            return item;
    }
    return NULL;
}

void cfg_print_mem(struct CFG* cfg)
{
    char header[64] = "";
    sprintf(header, "%5s   %5s %-15s %s", "ADDR", "SIZE", "KEY", "VALUE");
    printf("%s\n", header);
    struct CFGItem* item = cfg->items;
    for (int i=0 ; i<cfg->items_size ; i++, item++) {
        Addr addr = cfg_get_addr(cfg, item->id);
        uint8_t bytes[item->size] = {0};
        cfg_read_bytes(addr, bytes, item->size);
        char buf[64] = "";
        sprintf(buf, "0x%-5x %5d %-15s ", addr, item->size, item->key);
        print_arr(bytes, item->size, buf);
    }

    // print magic address
    struct CFGItem* last_item = &cfg->items[cfg->items_size-1];
    Addr magic_addr = cfg_get_addr(cfg, last_item->id) + last_item->size;
    uint8_t bytes[SCONF_MAGIC_EOL_SIZE] = {0};
    cfg_read_bytes(magic_addr, bytes, SCONF_MAGIC_EOL_SIZE);
    char buf[64] = "";
    sprintf(buf, "0x%-5x %5d %-15s ", magic_addr, SCONF_MAGIC_EOL_SIZE, "magic_number");
    print_arr(bytes, SCONF_MAGIC_EOL_SIZE, buf);
    printf("Total size: %dB\n", cfg_get_total_size(cfg));
}

void print_arr(uint8_t* arr, size_t len, const char* prefix)
{
    // debug
    Serial.print(prefix);
    Serial.print("[ ");
    for (size_t i=0 ; i<len ; i++) {
        Serial.print(arr[i], HEX);
        Serial.print(" ");
    }
    Serial.println("]");
}

Addr cfg_get_addr(struct CFG* cfg, ItemID id)
{
    // get EEPROM offset
    Addr addr = cfg->eeprom_start;
    struct CFGItem* item = cfg->items;

    for (int i=0 ; i<cfg->items_size; i++, item++) {
        if (item->id == id)
            break;
        addr += item->size;
    }
    return addr;
}

int8_t cfg_check_eol(struct CFG* cfg)
{
    /* Check at end of reserved EEPROM space for magic bytes.
     */
    struct CFGItem* item = &cfg->items[cfg->items_size-1];
    Addr addr = cfg_get_addr(cfg, item->id) + item->size;
    uint8_t bytes[SCONF_MAGIC_EOL_SIZE] = {0};
    cfg_read_bytes(addr, bytes, SCONF_MAGIC_EOL_SIZE);
    for (int i=0 ; i<SCONF_MAGIC_EOL_SIZE ; i++) {
        if (bytes[i] != SCONF_MAGIC_EOL[i])
            return -1;
    }
    return 1;
}

size_t cfg_get_total_size(CFG* cfg)
{
    /* Find total size of data in EEPROM */
    struct CFGItem* item = &cfg->items[cfg->items_size-1];
    Addr addr = cfg_get_addr(cfg, item->id) + item->size + SCONF_MAGIC_EOL_SIZE;
    return addr - cfg->eeprom_start;
}

int8_t cfg_fmt(struct CFG* cfg)
{
    /* Set reserved memory to zero and write magic bytes */
    struct CFGItem* item = &cfg->items[cfg->items_size-1];
    Addr magic_addr = cfg_get_addr(cfg, item->id) + item->size;

    size_t size = magic_addr - cfg->eeprom_start;
    uint8_t mem[size] = {0};
    print_arr(mem, size, "MEM");
    cfg_write_bytes(cfg->eeprom_start, mem, size);

    // write magic bytes
    cfg_write_bytes(magic_addr, (uint8_t*)SCONF_MAGIC_EOL, SCONF_MAGIC_EOL_SIZE);
    return 1;
}

void cfg_set_defaults(struct CFG* cfg)
{
    /* Write defaults to EEPROM */
    cfg_fmt(cfg);

    struct CFGItem* item = cfg->items;
    for (int i=0 ; i<cfg->items_size ; i++, item++)
        cfg_set_str(cfg, item->id, item->def_value);
}

void cfg_init(struct CFG* cfg, struct CFGItem* items, uint16_t items_size, 
              struct CFGAction* actions, uint16_t actions_size, Addr eeprom_start)
{
    cfg->eeprom_start = eeprom_start;
    cfg->items = items;
    cfg->actions = actions;
    cfg->items_size = items_size;
    cfg->actions_size = actions_size;

    EEPROM.begin(cfg_get_total_size(cfg));

    // if magic bytes are not found, write defaults to EEPROM
    if (cfg_check_eol(cfg) < 0) {
        printf("Magic bytes not found, formatting data!\n");
        cfg_set_defaults(cfg);
    }
}

void cfg_read_bytes(uint16_t addr, uint8_t* buf, size_t size)
{
    // mem 12345 -> [1,2,3,4,5]
    for (size_t a=addr, i=0 ; a<addr+(size) ; a++, i++) {
        uint8_t b = EEPROM.read(a);
        buf[i] = b;
    }
    //print_arr(buf, size, "READ");
}

void cfg_write_bytes(uint16_t addr, uint8_t* bytes, size_t size)
{
    for (size_t i=0 ; i<size ; i++) {
        EEPROM.write(addr++, bytes[i]);
    }
    EEPROM.commit();
    //print_arr(bytes, size, "WRITTEN");
}

int8_t cfg_set_str(struct CFG* cfg, ItemID index, const char* value)
{
    /* Does the same thing as cfg_set() but will accept value as string.
     * String will be converted to it's configured datatype
     */
    struct CFGItem* item = cfg_get_item(cfg, index);
    if (item == NULL)
        return -1;

    if (item->dtype == TYPE_UINT) {
        uint64_t nvalue = atoi(value);
        return cfg_set(cfg, item->id, &nvalue);
    }
    else if (item->dtype == TYPE_BOOL) {
        uint8_t nvalue = atoi(value);
        return cfg_set(cfg, item->id, &nvalue);
    }
    else if (item->dtype == TYPE_INT) {
        int64_t nvalue = atoi(value);
        return cfg_set(cfg, item->id, &nvalue);
    }
    else if (item->dtype == TYPE_FLOAT) {
        float nvalue = atof(value);
        return cfg_set(cfg, item->id, &nvalue);
    }
    else if (item->dtype == TYPE_STRING) {
        return cfg_set(cfg, item->id, value);
    }
    else {
        return -1;
    }
}

int8_t cfg_set(struct CFG* cfg, ItemID index, const void* value)
{
    struct CFGItem* item = cfg_get_item(cfg, index);
    if (item == NULL)
        return -1;

    Addr addr = cfg_get_addr(cfg, item->id);
    cfg_write_bytes(addr, (unsigned char*)value, item->size);
    return 1;
}

void* cfg_get(struct CFG* cfg, ItemID index, void* buf)
{
    struct CFGItem* item = cfg_get_item(cfg, index);
    if (item == NULL)
        return NULL;

    Addr addr = cfg_get_addr(cfg, item->id);
    cfg_read_bytes(addr, (unsigned char*)buf, item->size);
    return buf;
}
