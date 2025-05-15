#ifndef SCONF_H
#define SCONF_H


#include <Arduino.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include <EEPROM.h>

#define ARR_SIZE(a, b) {sizeof(a) / sizeof(b)}

// look for these bytes at end of reserved EEPROM SPACE
// If magic number is not found, format the whole thing
const uint8_t SCONF_MAGIC_EOL[] = {0x42, 0x00, 0x69};
const uint8_t SCONF_MAGIC_EOL_SIZE = 3;

// holds location in EEPROM
typedef uint16_t Addr;
typedef uint8_t  ItemID;

enum CFGType {
    TYPE_INT,
    TYPE_UINT,
    TYPE_FLOAT,
    TYPE_STRING,
    TYPE_CHAR,
    TYPE_BOOL
};

/* Main config struct, holds all data */
struct CFG {
    struct CFGItem* items;
    struct CFGAction* actions;
    uint16_t items_size;
    uint16_t actions_size;
    Addr eeprom_start;
};

/* Item in config. Is settable over serial and written to EEPROM like setting wifi passwords etc... */
struct CFGItem {
    // unique identifier to refer to item when getting or setting
    const ItemID id;

    // used to identify item in console
    const char* key;

    // size in bytes
    const size_t size;

    // default value
    const char* def_value;

    // used to display data properly in console
    const enum CFGType dtype;
};

/* Action that can be run over serial, like wifi connect etc...
 * Nothing here is written to EEPROM */
struct CFGAction;
struct CFGAction {
    const uint8_t id;

    // used to identify item in console
    const char* key;

    // the callback function to run
    void(*callback)(struct CFG* cfg);
};

// public
void cfg_init(struct CFG* cfg, struct CFGItem* items, uint16_t items_size, struct CFGAction* actions, uint16_t actions_size, Addr eeprom_start);
void* cfg_get(struct CFG* cfg, ItemID index, void* buf);
int8_t cfg_set(struct CFG* cfg, ItemID index, const void* value);
int8_t cfg_set_str(struct CFG* cfg, ItemID index, const char* value);
void cfg_set_defaults(struct CFG* cfg);
void cfg_print_mem(struct CFG* cfg);
size_t cfg_get_total_size(CFG* cfg);

// debug
void print_arr(uint8_t* arr, size_t len, const char* prefix);

// private
Addr cfg_get_addr(struct CFG* cfg, ItemID id);
int8_t cfg_check_eol(struct CFG* cfg);
void cfg_read_bytes(uint16_t addr, uint8_t* buf, size_t size);
void cfg_write_bytes(uint16_t addr, uint8_t* bytes, size_t size);
int8_t cfg_fmt(struct CFG* cfg);

#endif /* ifndef SCONF_H */
