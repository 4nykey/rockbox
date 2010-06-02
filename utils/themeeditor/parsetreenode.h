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

#ifndef PARSETREENODE_H
#define PARSETREENODE_H

#include "skin_parser.h"

#include <QString>
#include <QVariant>
#include <QList>

class ParseTreeNode
{
public:
    ParseTreeNode(struct skin_element* data);
    ParseTreeNode(struct skin_element* data, ParseTreeNode* parent);
    ParseTreeNode(struct skin_tag_parameter* data, ParseTreeNode* parent);
    virtual ~ParseTreeNode();

    QString genCode() const;
    bool isParam() const{ if(param) return true; else return false; }
    struct skin_tag_parameter* getParam(){ return param;}
    struct skin_element* getElement(){return element;}

    ParseTreeNode* child(int row);
    int numChildren() const;
    QVariant data(int column) const;
    int getRow() const;
    ParseTreeNode* getParent() const;

private:
    ParseTreeNode* parent;
    struct skin_element* element;
    struct skin_tag_parameter* param;
    QList<ParseTreeNode*> children;

    static int openConditionals;

};

#endif // PARSETREENODE_H
