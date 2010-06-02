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

#ifndef GENERIC_PARSER_H
#define GENERIC_PARSER_H

#ifdef __cplusplus
extern "C"
{
#endif


#define SKIN_MAX_MEMORY 1048576

/********************************************************************
 ******  A global buffer will be used to store the parse tree *******
 *******************************************************************/
extern char skin_parse_tree[];

/********************************************************************
 ****** Data Structures *********************************************
 *******************************************************************/

/* Possible types of element in a WPS file */
enum skin_element_type
{
    VIEWPORT,
    LINE,
    SUBLINES,
    CONDITIONAL,
    TAG,
    TEXT,
    COMMENT,
};

enum skin_errorcode
{
    MEMORY_LIMIT_EXCEEDED,
    NEWLINE_EXPECTED,
    ILLEGAL_TAG,
    ARGLIST_EXPECTED,
    TOO_MANY_ARGS,
    DEFAULT_NOT_ALLOWED,
    UNEXPECTED_NEWLINE,
    INSUFFICIENT_ARGS,
    INT_EXPECTED,
    SEPERATOR_EXPECTED,
    CLOSE_EXPECTED,
    MULTILINE_EXPECTED
};

/* Holds a tag parameter, either numeric or text */
struct skin_tag_parameter
{
    enum
    {
        NUMERIC,
        STRING,
        CODE,
        DEFAULT
    } type;

    union
    {
        int numeric;
        char* text;
        struct skin_element* code;
    } data;

    char type_code;
            
};

/* Defines an element of a SKIN file */
struct skin_element
{
    /* Defines what type of element it is */
    enum skin_element_type type;

    /* The line on which it's defined in the source file */
    int line;

    /* Text for comments and plaintext */
    char* text;

    /* The tag or conditional name */
    struct tag_info *tag;

    /* Pointer to and size of an array of parameters */
    int params_count;
    struct skin_tag_parameter* params;

    /* Pointer to and size of an array of children */
    int children_count;
    struct skin_element** children;

    /* Link to the next element */
    struct skin_element* next;
};

/***********************************************************************
 ***** Functions *******************************************************
 **********************************************************************/

/* Parses a WPS document and returns a list of skin_element
   structures. */
struct skin_element* skin_parse(const char* document);

/* Memory management functions */
struct skin_element* skin_alloc_element();
struct skin_element** skin_alloc_children(int count);
struct skin_tag_parameter* skin_alloc_params(int count);
char* skin_alloc_string(int length);

void skin_free_tree(struct skin_element* root);

#ifdef __cplusplus
}
#endif

#endif /* GENERIC_PARSER_H */
