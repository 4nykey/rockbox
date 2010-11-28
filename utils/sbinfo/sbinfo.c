/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2010 Bertrik Sikken
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

/*
 * .sb file parser and chunk extractor
 *
 * Based on amsinfo, which is
 * Copyright © 2008 Rafaël Carré <rafael.carre@gmail.com>
 */

#define _ISOC99_SOURCE /* snprintf() */
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "aes128_impl.h"

#if 1 /* ANSI colors */

#	define color(a) printf("%s",a)
char OFF[] 		= { 0x1b, 0x5b, 0x31, 0x3b, '0', '0', 0x6d, '\0' };

char GREY[] 	= { 0x1b, 0x5b, 0x31, 0x3b, '3', '0', 0x6d, '\0' };
char RED[] 		= { 0x1b, 0x5b, 0x31, 0x3b, '3', '1', 0x6d, '\0' };
char GREEN[] 	= { 0x1b, 0x5b, 0x31, 0x3b, '3', '2', 0x6d, '\0' };
char YELLOW[] 	= { 0x1b, 0x5b, 0x31, 0x3b, '3', '3', 0x6d, '\0' };
char BLUE[] 	= { 0x1b, 0x5b, 0x31, 0x3b, '3', '4', 0x6d, '\0' };

#else
	/* disable colors */
#	define color(a)
#endif

#define bug(...) do { fprintf(stderr,"ERROR: "__VA_ARGS__); exit(1); } while(0)
#define bugp(a) do { perror("ERROR: "a); exit(1); } while(0)

/* byte swapping */
#define get32le(a) ((uint32_t) \
    ( buf[a+3] << 24 | buf[a+2] << 16 | buf[a+1] << 8 | buf[a] ))
#define get16le(a) ((uint16_t)( buf[a+1] << 8 | buf[a] ))

/* all blocks are sized as a multiple of 0x1ff */
#define PAD_TO_BOUNDARY(x) (((x) + 0x1ff) & ~0x1ff)

/* If you find a firmware that breaks the known format ^^ */
#define assert(a) do { if(!(a)) { fprintf(stderr,"Assertion \"%s\" failed in %s() line %d!\n\nPlease send us your firmware!\n",#a,__func__,__LINE__); exit(1); } } while(0)

/* globals */

size_t sz;	/* file size */
uint8_t *buf; /* file content */
#define PREFIX_SIZE     128
char out_prefix[PREFIX_SIZE];
const char *key_file;

#define SB_INST_OP(inst)    (((inst) >> 8) & 0xff)
#define SB_INST_UNK(inst)   ((inst) & 0xff)

#define SB_INST_NOP     0x0
#define SB_INST_TAG     0x1
#define SB_INST_LOAD    0x2
#define SB_INST_FILL    0x3
#define SB_INST_JUMP    0x4
#define SB_INST_CALL    0x5
#define SB_INST_MODE    0x6

struct sb_instruction_header_t
{
    uint32_t inst;
} __attribute__((packed));

struct sb_instruction_load_t
{
    struct sb_instruction_header_t hdr;
    uint32_t addr;
    uint32_t len;
    uint32_t crc;
} __attribute__((packed));

struct sb_instruction_fill_t
{
    struct sb_instruction_header_t hdr;
    uint32_t addr;
    uint32_t len;
    uint32_t pattern;
} __attribute__((packed));

struct sb_instruction_call_t
{
    struct sb_instruction_header_t hdr;
    uint32_t addr;
    uint32_t arg;
} __attribute__((packed));

static void *xmalloc(size_t s) /* malloc helper */
{
	void * r = malloc(s);
	if(!r) bugp("malloc");
	return r;
}

static char getchr(int offset)
{
    char c;
    c = buf[offset];
    return isprint(c) ? c : '_';
}

static void getstrle(char string[], int offset)
{
    int i;
    for (i = 0; i < 4; i++) {
        string[i] = getchr(offset + 3 - i);
    }
    string[4] = 0;
}

static void getstrbe(char string[], int offset)
{
    int i;
    for (i = 0; i < 4; i++) {
        string[i] = getchr(offset + i);
    }
    string[4] = 0;
}

static void printhex(int offset, int len)
{
    int i;
    
    for (i = 0; i < len; i++) {
        printf("%02X ", buf[offset + i]);
    }
    printf("\n");
}

static void print_key(byte key[16])
{
    for(int i = 0; i < 16; i++)
        printf("%02X ", key[i]);
}

/* verify the firmware header */
static void check(unsigned long filesize)
{
    /* check STMP marker */
    char stmp[5];
    getstrbe(stmp, 0x14);
    assert(strcmp(stmp, "STMP") == 0);
    color(GREEN);

    /* get total size */
    unsigned long totalsize = 16 * get32le(0x1C);
    color(GREEN);
    assert(filesize == totalsize);
}

int convxdigit(char digit, byte *val)
{
    if(digit >= '0' && digit <= '9')
    {
        *val = digit - '0';
        return 0;
    }
    else if(digit >= 'A' && digit <= 'F')
    {
        *val = digit - 'A' + 10;
        return 0;
    }
    else if(digit >= 'a' && digit <= 'f')
    {
        *val = digit - 'a' + 10;
        return 0;
    }
    else
        return 1;
}

typedef byte (*key_array_t)[16];

static key_array_t read_keys(int num_keys)
{
    int size;
    struct stat st;
    int fd = open(key_file,O_RDONLY);
    if(fd == -1)
        bugp("opening key file failed");
    if(fstat(fd,&st) == -1)
        bugp("key file stat() failed");
    size = st.st_size;
    char *buf = xmalloc(size);
    if(read(fd,buf,sz)!=(ssize_t)size)
        bugp("reading key file");
    close(fd);

    key_array_t keys = xmalloc(sizeof(byte[16]) * num_keys);
    int pos = 0;
    for(int i = 0; i < num_keys; i++)
    {
        /* skip ws */
        while(pos < size && isspace(buf[pos]))
            pos++;
        /* enough space ? */
        if((pos + 32) > size)
            bugp("invalid key file (not enough keys)");
        for(int j = 0; j < 16; j++)
        {
            byte a, b;
            if(convxdigit(buf[pos + 2 * j], &a) || convxdigit(buf[pos + 2 * j + 1], &b))
                bugp(" invalid key, it should be a 128-bit key written in hexadecimal\n");
            keys[i][j] = (a << 4) | b;
        }
        pos += 32;
    }
    free(buf);

    return keys;
}

static void cbc_mac(
    byte *in_data, /* Input data */
    byte *out_data, /* Output data (or NULL) */
    int nr_blocks, /* Number of blocks to encrypt/decrypt (one block=16 bytes) */
    byte key[16], /* Key */
    byte iv[16], /* Initialisation Vector */
    byte (*out_cbc_mac)[16], /* CBC-MAC of the result (or NULL) */
    int encrypt /* 1 to encrypt, 0 to decrypt */
    )
{
    byte feedback[16];
    memcpy(feedback, iv, 16);

    if(encrypt)
    {
        /* for each block */
        for(int i = 0; i < nr_blocks; i++)
        {
            /* xor it with feedback */
            xor_(feedback, &in_data[i * 16], 16);
            /* encrypt it using aes */
            EncryptAES(feedback, key, feedback);
            /* write cipher to output */
            if(out_data)
                memcpy(&out_data[i * 16], feedback, 16);
        }
        if(out_cbc_mac)
            memcpy(out_cbc_mac, feedback, 16);
    }
    else
    {
        /* nothing to do ? */
        if(out_data == NULL)
            bugp("can't ask to decrypt with no output buffer");

        /* for each block */
        for(int i = 0; i < nr_blocks; i++)
        {
            /* decrypt it using aes */
            DecryptAES(&in_data[i * 16], key, &out_data[i * 16]);
            /* xor it with iv */
            xor_(&out_data[i * 16], feedback, 16);
            /* copy cipher to iv */
            memcpy(feedback, &in_data[i * 16], 16);
        }
    }
}

#define ROUND_UP(val, round) ((((val) + (round) - 1) / (round)) * (round))

static void extract_section(int data_sec, char name[5], byte *buf, int size, const char *indent)
{
    char filename[PREFIX_SIZE + 16];
    snprintf(filename, sizeof filename, "%s%s.bin", out_prefix, name);
    FILE *fd = fopen(filename, "wb");
    if (fd != NULL) {
        fwrite(buf, size, 1, fd);
        fclose(fd);
    }
    if(data_sec)
        return;
    /* Pretty print the content */
    int pos = 0;
    while(pos < size)
    {
        struct sb_instruction_header_t *hdr = (struct sb_instruction_header_t *)&buf[pos];
        if(SB_INST_OP(hdr->inst) == SB_INST_LOAD)
        {
            struct sb_instruction_load_t *load = (struct sb_instruction_load_t *)&buf[pos];
            color(RED);
            printf("%sLOAD", indent);
            color(OFF);printf(" | ");
            color(BLUE);
            printf("addr=%#08x", load->addr);
            color(OFF);printf(" | ");
            color(GREEN);
            printf("len=%#08x", load->len);
            color(OFF);printf(" | ");
            color(YELLOW);
            printf("crc=%#08x\n", load->crc);
            color(OFF);

            pos += load->len + sizeof(struct sb_instruction_load_t);
            // unsure about rounding
            pos = ROUND_UP(pos, 16);
        }
        else if(SB_INST_OP(hdr->inst) == SB_INST_FILL)
        {
            struct sb_instruction_fill_t *fill = (struct sb_instruction_fill_t *)&buf[pos];
            color(RED);
            printf("%sFILL", indent);
            color(OFF);printf(" | ");
            color(BLUE);
            printf("addr=%#08x", fill->addr);
            color(OFF);printf(" | ");
            color(GREEN);
            printf("len=%#08x", fill->len);
            color(OFF);printf(" | ");
            color(YELLOW);
            printf("pattern=%#08x\n", fill->pattern);
            color(OFF);

            pos += sizeof(struct sb_instruction_fill_t);
            // fixme: useless as pos is a multiple of 16 and fill struct is 4-bytes wide ?
            pos = ROUND_UP(pos, 16);
        }
        else if(SB_INST_OP(hdr->inst) == SB_INST_CALL ||
                SB_INST_OP(hdr->inst) == SB_INST_JUMP)
        {
            int is_call = (SB_INST_OP(hdr->inst) == SB_INST_CALL);
            struct sb_instruction_call_t *call = (struct sb_instruction_call_t *)&buf[pos];
            color(RED);
            if(is_call)
                printf("%sCALL", indent);
            else
                printf("%sJUMP", indent);
            color(OFF);printf(" | ");
            color(BLUE);
            printf("addr=%#08x", call->addr);
            color(OFF);printf(" | ");
            color(GREEN);
            printf("arg=%#08x\n", call->arg);
            color(OFF);

            pos += sizeof(struct sb_instruction_call_t);
            // fixme: useless as pos is a multiple of 16 and call struct is 4-bytes wide ?
            pos = ROUND_UP(pos, 16);
        }
        else
        {
            color(RED);
            printf("Unknown instruction %d at address %#08lx\n", SB_INST_OP(hdr->inst), (unsigned long)pos);
            break;
        }
    }
}

static void extract(unsigned long filesize)
{
    /* Basic header info */
    color(BLUE);
    printf("Basic info:\n");
    color(GREEN);
    printf("\tHeader SHA-1: ");
    printhex(0, 20);
    printf("\tFlags: ");
    printhex(0x18, 4);
    printf("\tTotal file size : %ld\n", filesize);
    
    /* Sizes and offsets */
    color(BLUE);
    printf("Sizes and offsets:\n");
    color(GREEN);
    int num_enc = get16le(0x28);
    printf("\t# of encryption keys = %d\n", num_enc);
    int num_chunks = get16le(0x2E);
    printf("\t# of chunk headers = %d\n", num_chunks);

    /* Versions */
    color(BLUE);
    printf("Versions\n");
    color(GREEN);

    printf("\tRandom 1: ");
    printhex(0x32, 6);
    printf("\tRandom 2: ");
    printhex(0x5A, 6);
    
    uint64_t micros_l = get32le(0x38);
    uint64_t micros_h = get32le(0x3c);
    uint64_t micros = ((uint64_t)micros_h << 32) | micros_l;
    time_t seconds = (micros / (uint64_t)1000000L);
    seconds += 946684800; /* 2000/1/1 0:00:00 */
    struct tm *time = gmtime(&seconds);
    color(GREEN);
    printf("\tCreation date/time = %s", asctime(time));

    int p_maj = get32le(0x40);
    int p_min = get32le(0x44);
    int p_sub = get32le(0x48);
    int c_maj = get32le(0x4C);
    int c_min = get32le(0x50);
    int c_sub = get32le(0x54);
    color(GREEN);
    printf("\tProduct version   = %X.%X.%X\n", p_maj, p_min, p_sub);
    printf("\tComponent version = %X.%X.%X\n", c_maj, c_min, c_sub);

    /* encryption cbc-mac */
    key_array_t keys = NULL; /* array of 16-bytes keys */
    byte real_key[16];
    if(num_enc > 0)
    {
        keys = read_keys(num_enc);
        color(BLUE),
        printf("Encryption data\n");
        for(int i = 0; i < num_enc; i++)
        {
            color(RED);
            printf("\tKey %d: ", i);
            print_key(keys[i]);
            printf("\n");
            color(GREEN);
            printf("\t\tCBC-MAC of headers: ");
            /* copy the cbc mac */
            byte hdr_cbc_mac[16];
            memcpy(hdr_cbc_mac, &buf[0x60 + 16 * num_chunks + 32 * i], 16);
            print_key(hdr_cbc_mac);
            /* check it */
            byte computed_cbc_mac[16];
            byte zero[16];
            memset(zero, 0, 16);
            cbc_mac(buf, NULL, 6 + num_chunks, keys[i], zero, &computed_cbc_mac, 1);
            color(RED);
            if(memcmp(hdr_cbc_mac, computed_cbc_mac, 16) == 0)
                printf(" Ok\n");
            else
                printf(" Failed\n");
            color(GREEN);
            
            printf("\t\tEncrypted key     : ");
            byte (*encrypted_key)[16];
            encrypted_key = (key_array_t)&buf[0x60 + 16 * num_chunks + 32 * i + 16];
            print_key(*encrypted_key);
            printf("\n");
            /* decrypt */
            byte decrypted_key[16];
            byte iv[16];
            memcpy(iv, buf, 16); /* uses the first 16-bytes of SHA-1 sig as IV */
            cbc_mac(*encrypted_key, decrypted_key, 1, keys[i], iv, NULL, 0);
            printf("\t\tDecrypted key     : ");
            print_key(decrypted_key);
            /* cross-check or copy */
            if(i == 0)
                memcpy(real_key, decrypted_key, 16);
            else if(memcmp(real_key, decrypted_key, 16) == 0)
            {
                color(RED);
                printf(" Cross-Check Ok");
            }
            else
            {
                color(RED);
                printf(" Cross-Check Failed");
            }
            printf("\n");
        }
    }

    /* chunks */
    color(BLUE);
    printf("Chunks\n");

    for (int i = 0; i < num_chunks; i++) {
        uint32_t ofs = 0x60 + (i * 16);
    
        char name[5];
        getstrle(name, ofs + 0);
        int pos = 16 * get32le(ofs + 4);
        int size = 16 * get32le(ofs + 8);
        int flags = get32le(ofs + 12);
        int data_sec = (flags == 2);
        int encrypted = !data_sec && (num_enc > 0);
    
        color(GREEN);
        printf("\tChunk '%s'\n", name);
        printf("\t\tpos   = %8x - %8x\n", pos, pos+size);
        printf("\t\tlen   = %8x\n", size);
        printf("\t\tflags = %8x", flags);
        color(RED);
        if(data_sec)
            printf("  Data Section");
        else
            printf("  Boot Section");
        if(encrypted)
            printf(" (Encrypted)");
        printf("\n");
        
        /* save it */
        byte *sec = xmalloc(size);
        if(encrypted)
            cbc_mac(buf + pos, sec, size / 16, real_key, buf, NULL, 0);
        else
            memcpy(sec, buf + pos, size);
        
        extract_section(data_sec, name, sec, size, "\t\t\t");
        free(sec);
    }
    
    /* final signature */
    color(BLUE);
    printf("Final signature:\n\t");
    color(GREEN);
    printhex(filesize - 32, 16);
    printf("\t");
    printhex(filesize - 16, 16);
}

int main(int argc, const char **argv)
{
    int fd;
    struct stat st;
    if(argc != 3 && argc != 4)
        bug("Usage: %s <firmware> <key file> [<out prefix>]\n",*argv);

    if(argc == 4)
        snprintf(out_prefix, PREFIX_SIZE, "%s", argv[3]);
    else
        strcpy(out_prefix, "");

    if( (fd = open(argv[1],O_RDONLY)) == -1 )
        bugp("opening firmware failed");

    key_file = argv[2];

    if(fstat(fd,&st) == -1)
        bugp("firmware stat() failed");
    sz = st.st_size;

    buf=xmalloc(sz);
    if(read(fd,buf,sz)!=(ssize_t)sz) /* load the whole file into memory */
        bugp("reading firmware");

    close(fd);

    check(st.st_size);	/* verify header and checksums */
    extract(st.st_size);	/* split in blocks */

    color(OFF);

    free(buf);
    return 0;
}
