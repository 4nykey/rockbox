/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2012 Amaury Pouly
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <ctype.h>
#include "misc.h"
#include "crypto.h"
#include "sb1.h"

static int sdram_size_table[] = {2, 8, 16, 32, 64};

#define NR_SDRAM_ENTRIES    (int)(sizeof(sdram_size_table) / sizeof(sdram_size_table[0]))

int sb1_sdram_size_by_index(int index)
{
    if(index < 0 || index >= NR_SDRAM_ENTRIES)
        return -1;
    return sdram_size_table[index];
}

int sb1_sdram_index_by_size(int size)
{
    for(int i = 0; i < NR_SDRAM_ENTRIES; i++)
        if(sdram_size_table[i] == size)
            return i;
    return -1;
}

static uint16_t swap16(uint16_t t)
{
    return (t << 8) | (t >> 8);
}

static void fix_version(struct sb1_version_t *ver)
{
    ver->major = swap16(ver->major);
    ver->minor = swap16(ver->minor);
    ver->revision = swap16(ver->revision);
}

enum sb1_error_t sb1_write_file(struct sb1_file_t *sb, const char *filename)
{
    return SB1_ERROR;
}

struct sb1_file_t *sb1_read_file(const char *filename, void *u,
    sb1_color_printf cprintf, enum sb1_error_t *err)
{
    return sb1_read_file_ex(filename, 0, -1, u, cprintf, err);
}

struct sb1_file_t *sb1_read_file_ex(const char *filename, size_t offset, size_t size, void *u,
    sb1_color_printf cprintf, enum sb1_error_t *err)
{
    #define fatal(e, ...) \
        do { if(err) *err = e; \
            cprintf(u, true, GREY, __VA_ARGS__); \
            free(buf); \
            return NULL; } while(0)

    FILE *f = fopen(filename, "rb");
    void *buf = NULL;
    if(f == NULL)
        fatal(SB1_OPEN_ERROR, "Cannot open file for reading\n");
    fseek(f, 0, SEEK_END);
    size_t read_size = ftell(f);
    fseek(f, offset, SEEK_SET);
    if(size != (size_t)-1)
        read_size = size;
    buf = xmalloc(read_size);
    if(fread(buf, read_size, 1, f) != 1)
    {
        fclose(f);
        fatal(SB1_READ_ERROR, "Cannot read file\n");
    }
    fclose(f);

    struct sb1_file_t *ret = sb1_read_memory(buf, read_size, u, cprintf, err);
    free(buf);
    return ret;

    #undef fatal
}

static const char *sb1_cmd_name(int cmd)
{
    switch(cmd)
    {
        case SB1_INST_LOAD: return "load";
        case SB1_INST_FILL: return "fill";
        case SB1_INST_JUMP: return "jump";
        case SB1_INST_CALL: return "call";
        case SB1_INST_MODE: return "mode";
        case SB1_INST_SDRAM: return "sdram";
        default: return "unknown";
    }
}

static const char *sb1_datatype_name(int cmd)
{
    switch(cmd)
    {
        case SB1_DATATYPE_UINT32: return "uint32";
        case SB1_DATATYPE_UINT16: return "uint16";
        case SB1_DATATYPE_UINT8: return "uint8";
        default: return "unknown";
    }
}

struct sb1_file_t *sb1_read_memory(void *_buf, size_t filesize, void *u,
    sb1_color_printf cprintf, enum sb1_error_t *err)
{
    struct sb1_file_t *file = NULL;
    uint8_t *buf = _buf;

    #define printf(c, ...) cprintf(u, false, c, __VA_ARGS__)
    #define fatal(e, ...) \
        do { if(err) *err = e; \
            cprintf(u, true, GREY, __VA_ARGS__); \
            sb1_free(file); \
            return NULL; } while(0)
    #define print_hex(c, p, len, nl) \
        do { printf(c, ""); print_hex(p, len, nl); } while(0)

    file = xmalloc(sizeof(struct sb1_file_t));
    memset(file, 0, sizeof(struct sb1_file_t));
    struct sb1_header_t *header = (struct sb1_header_t *)buf;

    if(memcmp(header->signature, "STMP", 4) != 0)
        fatal(SB1_FORMAT_ERROR, "Bad signature\n");
    if(header->image_size > filesize)
        fatal(SB1_FORMAT_ERROR, "File too small (should be at least %d bytes)\n",
            header->image_size);
    if(header->header_size != sizeof(struct sb1_header_t))
        fatal(SB1_FORMAT_ERROR, "Bad header size\n");

    printf(BLUE, "Basic info:\n");
    printf(GREEN, "  ROM version: ");
    printf(YELLOW, "%x\n", header->rom_version);
    printf(GREEN, "  Userdata offset: ");
    printf(YELLOW, "%x\n", header->userdata_offset);
    printf(GREEN, "  Pad: ");
    printf(YELLOW, "%x\n", header->pad2);

    struct sb1_version_t product_ver = header->product_ver;
    fix_version(&product_ver);
    struct sb1_version_t component_ver = header->component_ver;
    fix_version(&component_ver);

    printf(GREEN, "  Product version: ");
    printf(YELLOW, "%X.%X.%X\n", product_ver.major, product_ver.minor, product_ver.revision);
    printf(GREEN, "  Component version: ");
    printf(YELLOW, "%X.%X.%X\n", component_ver.major, component_ver.minor, component_ver.revision);
    
    printf(GREEN, "  Drive tag: ");
    printf(YELLOW, "%x\n", header->drive_tag);

    /* copy rom version, padding and drive tag */
    /* copy versions */
    memcpy(&file->product_ver, &product_ver, sizeof(product_ver));
    memcpy(&file->component_ver, &component_ver, sizeof(component_ver));
    file->rom_version = header->rom_version;
    file->pad2 = header->pad2;
    file->drive_tag = header->drive_tag;

    /* reduce size w.r.t to userdata part */
    uint32_t userdata_size = 0;
    if(header->userdata_offset != 0)
    {
        userdata_size = header->image_size - header->userdata_offset;
        header->image_size -= userdata_size;
    }

    if(header->image_size % SECTOR_SIZE)
    {
        if(g_force)
            printf(GREY, "Image size is not a multiple of sector size\n");
        else
            fatal(SB1_FORMAT_ERROR, "Image size is not a multiple of sector size\n");
    }

    /* find key */
    union xorcrypt_key_t key[2];
    bool valid_key = false;
    uint8_t sector[SECTOR_SIZE];

    for(int i = 0; i < g_nr_keys; i++)
    {
        if(!g_key_array[i].method == CRYPTO_XOR_KEY)
            continue;
        /* copy key and data because it's modified by the crypto code */
        memcpy(key, g_key_array[i].u.xor_key, sizeof(key));
        memcpy(sector, header + 1, SECTOR_SIZE - header->header_size);
        /* try to decrypt the first sector */
        uint32_t mark = xor_decrypt(key, sector, SECTOR_SIZE - 4 - header->header_size);
        if(mark != *(uint32_t *)&sector[SECTOR_SIZE - 4 - header->header_size])
            continue;
        /* found ! */
        valid_key = true;
        /* copy key again it's modified by the crypto code */
        memcpy(key, g_key_array[i].u.xor_key, sizeof(key));
        break;
    }

    printf(BLUE, "Crypto\n");
    for(int i = 0; i < 2; i++)
    {
        printf(RED, "  Key %d\n", i);
        printf(OFF, "    ");
        for(int j = 0; j < 64; j++)
        {
            printf(YELLOW, "%02x ", key[i].key[j]);
            if((j + 1) % 16 == 0)
            {
                printf(OFF, "\n");
                if(j + 1 != 64)
                    printf(OFF, "    ");
            }
        }
    }
    
    if(!valid_key)
        fatal(SB1_NO_VALID_KEY, "No valid key found\n");

    /* decrypt image in-place (and removing crypto markers) */
    void *ptr = header + 1;
    void *copy_ptr = header + 1;
    int offset = header->header_size;
    for(unsigned i = 0; i < header->image_size / SECTOR_SIZE; i++)
    {
        int size = SECTOR_SIZE - 4 - offset;
        uint32_t mark = xor_decrypt(key, ptr, size);
        if(mark != *(uint32_t *)(ptr + size))
            fatal(SB1_CHECKSUM_ERROR, "Crypto mark mismatch\n");
        memmove(copy_ptr, ptr, size);

        ptr += size + 4;
        copy_ptr += size;
        offset = 0;
    }

    /* reduce image size given the removed marks */
    header->image_size -= header->image_size / SECTOR_SIZE;

    printf(BLUE, "Commands\n");
    struct sb1_cmd_header_t *cmd = (void *)(header + 1);
    while((void *)cmd < (void *)header + header->image_size)
    {
        printf(GREEN, "  Command");
        printf(YELLOW, " %#x\n", cmd->cmd);
        printf(YELLOW, "    Size:");
        printf(RED, " %#x\n", SB1_CMD_SIZE(cmd->cmd));
        printf(YELLOW, "    Critical:");
        printf(RED, " %d\n", SB1_CMD_CRITICAL(cmd->cmd));
        printf(YELLOW, "    Data Type:");
        printf(RED, " %#x ", SB1_CMD_DATATYPE(cmd->cmd));
        printf(GREEN, "(%s)\n", sb1_datatype_name(SB1_CMD_DATATYPE(cmd->cmd)));
        printf(YELLOW, "    Bytes:");
        printf(RED, " %#x\n", SB1_CMD_BYTES(cmd->cmd));
        printf(YELLOW, "    Boot:");
        printf(RED, " %#x ", SB1_CMD_BOOT(cmd->cmd));
        printf(GREEN, "(%s)\n", sb1_cmd_name(SB1_CMD_BOOT(cmd->cmd)));
        printf(YELLOW, "    Addr:");
        printf(RED, " %#x", cmd->addr);

        if(SB1_CMD_BOOT(cmd->cmd) == SB1_INST_SDRAM)
            printf(GREEN, " (Chip Select=%d, Size=%d)", SB1_ADDR_SDRAM_CS(cmd->addr),
                    sb1_sdram_size_by_index(SB1_ADDR_SDRAM_SZ(cmd->addr)));
        printf(OFF, "\n");
        if(SB1_CMD_BOOT(cmd->cmd) == SB1_INST_FILL)
        {
            printf(YELLOW, "    Pattern:");
            printf(RED, " %#x\n", *(uint32_t *)(cmd + 1));
        }

        /* copy command */
        struct sb1_inst_t inst;
        memset(&inst, 0, sizeof(inst));
        inst.cmd = SB1_CMD_BOOT(cmd->cmd);
        inst.critical = SB1_CMD_CRITICAL(cmd->cmd);
        inst.datatype = SB1_CMD_DATATYPE(cmd->cmd);
        inst.size = SB1_CMD_BYTES(cmd->cmd);

        switch(SB1_CMD_BOOT(cmd->cmd))
        {
            case SB1_INST_SDRAM:
                inst.sdram.chip_select = SB1_ADDR_SDRAM_CS(cmd->addr);
                inst.sdram.size_index = SB1_ADDR_SDRAM_SZ(cmd->addr);
                break;
            case SB1_INST_MODE:
                inst.mode = cmd->addr;
                break;
            case SB1_INST_LOAD:
                inst.data = malloc(inst.size);
                memcpy(inst.data, cmd + 1, inst.size);
                /* fallthrough */
            default:
                inst.addr = cmd->addr;
                break;
        }

        file->insts = augment_array(file->insts, sizeof(inst), file->nr_insts, &inst, 1);
        file->nr_insts++;

        /* last instruction ? */
        if(SB1_CMD_BOOT(cmd->cmd) == SB1_INST_JUMP ||
                SB1_CMD_BOOT(cmd->cmd) == SB1_INST_MODE)
            break;

        cmd = (void *)cmd + 4 + 4 * SB1_CMD_SIZE(cmd->cmd);
    }

    /* copy userdata */
    file->userdata_size = userdata_size;
    if(userdata_size > 0)
    {
        file->userdata = malloc(userdata_size);
        memcpy(file->userdata, (void *)header + header->userdata_offset, userdata_size);
    }

    return file;
    #undef printf
    #undef fatal
    #undef print_hex
}

void sb1_free(struct sb1_file_t *file)
{
    if(!file) return;

    for(int i = 0; i < file->nr_insts; i++)
        free(file->insts[i].data);
    free(file->insts);
    free(file->userdata);
    free(file);
}

void sb1_dump(struct sb1_file_t *file, void *u, sb1_color_printf cprintf)
{
    #define printf(c, ...) cprintf(u, false, c, __VA_ARGS__)
    #define print_hex(c, p, len, nl) \
        do { printf(c, ""); print_hex(p, len, nl); } while(0)

    #define TREE    RED
    #define HEADER  GREEN
    #define TEXT    YELLOW
    #define TEXT2   BLUE
    #define TEXT3   RED
    #define SEP     OFF

    printf(BLUE, "SB1 File\n");
    printf(TREE, "+-");
    printf(HEADER, "Rom Ver: ");
    printf(TEXT, "%x\n", file->rom_version);
    printf(TREE, "+-");
    printf(HEADER, "Pad: ");
    printf(TEXT, "%x\n", file->pad2);
    printf(TREE, "+-");
    printf(HEADER, "Drive Tag: ");
    printf(TEXT, "%x\n", file->drive_tag);
    printf(TREE, "+-");
    printf(HEADER, "Product Version: ");
    printf(TEXT, "%X.%X.%X\n", file->product_ver.major, file->product_ver.minor,
        file->product_ver.revision);
    printf(TREE, "+-");
    printf(HEADER, "Component Version: ");
    printf(TEXT, "%X.%X.%X\n", file->component_ver.major, file->component_ver.minor,
        file->component_ver.revision);

    for(int j = 0; j < file->nr_insts; j++)
    {
        struct sb1_inst_t *inst = &file->insts[j];
        printf(TREE, "+-");
        printf(HEADER, "Command\n");
        printf(TREE, "|  +-");
        switch(inst->cmd)
        {
            case SB1_INST_CALL:
            case SB1_INST_JUMP:
                printf(HEADER, "%s", inst->cmd == SB1_INST_CALL ? "CALL" : "JUMP");
                printf(SEP, " | ");
                printf(TEXT3, "crit=%d", inst->critical);
                printf(SEP, " | ");
                printf(TEXT, "addr=0x%08x\n", inst->addr);
                break;
            case SB1_INST_LOAD:
                printf(HEADER, "LOAD");
                printf(SEP, " | ");
                printf(TEXT3, "crit=%d", inst->critical);
                printf(SEP, " | ");
                printf(TEXT, "addr=0x%08x", inst->addr);
                printf(SEP, " | ");
                printf(TEXT2, "len=0x%08x\n", inst->size);
                break;
            case SB1_INST_FILL:
                printf(HEADER, "FILL");
                printf(SEP, " | ");
                printf(TEXT3, "crit=%d", inst->critical);
                printf(SEP, " | ");
                printf(TEXT, "addr=0x%08x", inst->addr);
                printf(SEP, " | ");
                printf(TEXT2, "len=0x%08x", inst->size);
                printf(SEP, " | ");
                printf(TEXT2, "pattern=0x%08x\n", inst->pattern);
                break;
            case SB1_INST_MODE:
                printf(HEADER, "MODE");
                printf(SEP, " | ");
                printf(TEXT3, "crit=%d", inst->critical);
                printf(SEP, " | ");
                printf(TEXT, "mode=0x%08x\n", inst->addr);
                break;
            case SB1_INST_SDRAM:
                printf(HEADER, "SRAM");
                printf(SEP, " | ");
                printf(TEXT3, "crit=%d", inst->critical);
                printf(SEP, " | ");
                printf(TEXT, "chip_select=%d", inst->sdram.chip_select);
                printf(SEP, " | ");
                printf(TEXT2, "chip_size=%d\n", sb1_sdram_size_by_index(inst->sdram.size_index));
                break;
            default:
                printf(GREY, "[Unknown instruction %x]\n", inst->cmd);
                break;
        }
    }

    #undef printf
    #undef print_hex
}
 
