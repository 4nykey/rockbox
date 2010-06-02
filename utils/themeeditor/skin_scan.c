/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2010 Robert Bieber
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
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "skin_scan.h"
#include "skin_debug.h"
#include "symbols.h"
#include "skin_parser.h"

/* Scanning Functions */

/* Simple function to advance a char* past a comment */
void skip_comment(char** document)
{
    for(/*NO INIT*/;**document != '\n' && **document != '\0'; (*document)++);
    if(**document == '\n')
        (*document)++;
}

void skip_whitespace(char** document)
{
    for(/*NO INIT*/; **document == ' ' || **document == '\t'; (*document)++);
}

char* scan_string(char** document)
{

    char* cursor = *document;
    int length = 0;
    char* buffer = NULL;
    int i;

    while(*cursor != ARGLISTSEPERATESYM && *cursor != ARGLISTCLOSESYM &&
          *cursor != '\0')
    {
        if(*cursor == COMMENTSYM)
        {
            skip_comment(&cursor);
            continue;
        }

        if(*cursor == TAGSYM)
            cursor++;

        if(*cursor == '\n')
        {
            skin_error(UNEXPECTED_NEWLINE);
            return NULL;
        }

        length++;
        cursor++;
    }

    /* Copying the string */
    cursor = *document;
    buffer = skin_alloc_string(length);
    buffer[length] = '\0';
    for(i = 0; i < length; i++)
    {
        if(*cursor == TAGSYM)
            cursor++;

        if(*cursor == COMMENTSYM)
        {
            skip_comment(&cursor);
            i--;
            continue;
        }

        buffer[i] = *cursor;
        cursor++;
    }

    *document = cursor;
    return buffer;
}

int scan_int(char** document)
{

    char* cursor = *document;
    int length = 0;
    char* buffer = NULL;
    int retval;
    int i;

    while(isdigit(*cursor) || *cursor == COMMENTSYM || *cursor == '-')
    {
        if(*cursor == COMMENTSYM)
        {
            skip_comment(&cursor);
            continue;
        }

        length++;
        cursor++;
    }

    buffer = skin_alloc_string(length);

    /* Copying to the buffer while avoiding comments */
    cursor = *document;
    buffer[length] = '\0';
    for(i = 0; i < length; i++)
    {
        if(*cursor == COMMENTSYM)
        {
            skip_comment(&cursor);
            i--;
            continue;
        }

        buffer[i] = *cursor;
        cursor++;

    }
    retval = atoi(buffer);
    free(buffer);

    *document = cursor;
    return retval;
}

int check_viewport(char* document)
{
    if(strlen(document) < 3)
        return 0;

    if(document[0] != TAGSYM)
        return 0;

    if(document[1] != 'V')
        return 0;

    if(document[2] != ARGLISTOPENSYM
       && document[2] != 'l'
       && document[2] != 'i')
        return 0;

    return 1;
}
