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

#include "symbols.h"
#include "tag_table.h"

#include "parsetreenode.h"
#include "parsetreemodel.h"

#include "rbimage.h"
#include "rbprogressbar.h"

#include <iostream>

int ParseTreeNode::openConditionals = 0;

/* Root element constructor */
ParseTreeNode::ParseTreeNode(struct skin_element* data)
    : parent(0), element(0), param(0), children()
{
    while(data)
    {
        children.append(new ParseTreeNode(data, this));
        data = data->next;
    }
}

/* Normal element constructor */
ParseTreeNode::ParseTreeNode(struct skin_element* data, ParseTreeNode* parent)
    : parent(parent), element(data), param(0), children()
{
    switch(element->type)
    {

    case TAG:
        for(int i = 0; i < element->params_count; i++)
        {
            if(element->params[i].type == skin_tag_parameter::CODE)
                children.append(new ParseTreeNode(element->params[i].data.code,
                                              this));
            else
                children.append(new ParseTreeNode(&element->params[i], this));
        }
        break;

    case CONDITIONAL:
        for(int i = 0; i < element->params_count; i++)
            children.append(new ParseTreeNode(&data->params[i], this));
        for(int i = 0; i < element->children_count; i++)
            children.append(new ParseTreeNode(data->children[i], this));
        break;

    case SUBLINES:
        for(int i = 0; i < element->children_count; i++)
        {
            children.append(new ParseTreeNode(data->children[i], this));
        }
        break;

case VIEWPORT:
        for(int i = 0; i < element->params_count; i++)
            children.append(new ParseTreeNode(&data->params[i], this));
        /* Deliberate fall-through here */

    case LINE:
        for(int i = 0; i < data->children_count; i++)
        {
            for(struct skin_element* current = data->children[i]; current;
                current = current->next)
            {
                children.append(new ParseTreeNode(current, this));
            }
        }
        break;

    default:
        break;
    }
}

/* Parameter constructor */
ParseTreeNode::ParseTreeNode(skin_tag_parameter *data, ParseTreeNode *parent)
    : parent(parent), element(0), param(data), children()
{

}

ParseTreeNode::~ParseTreeNode()
{
    for(int i = 0; i < children.count(); i++)
        delete children[i];
}

QString ParseTreeNode::genCode() const
{
    QString buffer = "";

    if(element)
    {
        switch(element->type)
        {
        case UNKNOWN:
            break;
        case VIEWPORT:
            /* Generating the Viewport tag, if necessary */
            if(element->tag)
            {
                buffer.append(TAGSYM);
                buffer.append(element->tag->name);
                buffer.append(ARGLISTOPENSYM);
                for(int i = 0; i < element->params_count; i++)
                {
                    buffer.append(children[i]->genCode());
                    if(i != element->params_count - 1)
                        buffer.append(ARGLISTSEPERATESYM);
                }
                buffer.append(ARGLISTCLOSESYM);
                buffer.append('\n');
            }

            for(int i = element->params_count; i < children.count(); i++)
                buffer.append(children[i]->genCode());
            break;

        case LINE:
            for(int i = 0; i < children.count(); i++)
            {
                buffer.append(children[i]->genCode());
            }
            if(openConditionals == 0
               && !(parent && parent->element->type == SUBLINES))
            {
                buffer.append('\n');
            }
            break;

        case SUBLINES:
            for(int i = 0; i < children.count(); i++)
            {
                buffer.append(children[i]->genCode());
                if(i != children.count() - 1)
                    buffer.append(MULTILINESYM);
            }
            if(openConditionals == 0)
                buffer.append('\n');
            break;

        case CONDITIONAL:
            openConditionals++;

            /* Inserting the tag part */
            buffer.append(TAGSYM);
            buffer.append(CONDITIONSYM);
            buffer.append(element->tag->name);
            if(element->params_count > 0)
            {
                buffer.append(ARGLISTOPENSYM);
                for(int i = 0; i < element->params_count; i++)
                {
                    buffer.append(children[i]->genCode());
                    if( i != element->params_count - 1)
                        buffer.append(ARGLISTSEPERATESYM);
                    buffer.append(ARGLISTCLOSESYM);
                }
            }

            /* Inserting the sublines */
            buffer.append(ENUMLISTOPENSYM);
            for(int i = element->params_count; i < children.count(); i++)
            {
                buffer.append(children[i]->genCode());
                if(i != children.count() - 1)
                    buffer.append(ENUMLISTSEPERATESYM);
            }
            buffer.append(ENUMLISTCLOSESYM);
            openConditionals--;
            break;

        case TAG:
            buffer.append(TAGSYM);
            buffer.append(element->tag->name);

            if(element->params_count > 0)
            {
                /* Rendering parameters if there are any */
                buffer.append(ARGLISTOPENSYM);
                for(int i = 0; i < children.count(); i++)
                {
                    buffer.append(children[i]->genCode());
                    if(i != children.count() - 1)
                        buffer.append(ARGLISTSEPERATESYM);
                }
                buffer.append(ARGLISTCLOSESYM);
            }
            if(element->tag->params[strlen(element->tag->params) - 1] == '\n')
                buffer.append('\n');
            break;

        case TEXT:
            for(char* cursor = (char*)element->data; *cursor; cursor++)
            {
                if(find_escape_character(*cursor))
                    buffer.append(TAGSYM);
                buffer.append(*cursor);
            }
            break;

        case COMMENT:
            buffer.append(COMMENTSYM);
            buffer.append((char*)element->data);
            buffer.append('\n');
            break;
        }
    }
    else if(param)
    {
        switch(param->type)
        {
        case skin_tag_parameter::STRING:
            for(char* cursor = param->data.text; *cursor; cursor++)
            {
                if(find_escape_character(*cursor))
                    buffer.append(TAGSYM);
                buffer.append(*cursor);
            }
            break;

        case skin_tag_parameter::NUMERIC:
            buffer.append(QString::number(param->data.numeric, 10));
            break;

        case skin_tag_parameter::DEFAULT:
            buffer.append(DEFAULTSYM);
            break;

        case skin_tag_parameter::CODE:
            buffer.append(QObject::tr("This doesn't belong here"));
            break;

        }
    }
    else
    {
        for(int i = 0; i < children.count(); i++)
            buffer.append(children[i]->genCode());
    }

    return buffer;
}

/* A more or less random hashing algorithm */
int ParseTreeNode::genHash() const
{
    int hash = 0;
    char *text;

    if(element)
    {
        hash += element->type;
        switch(element->type)
        {
        case UNKNOWN:
            break;
        case VIEWPORT:
        case LINE:
        case SUBLINES:
        case CONDITIONAL:
            hash += element->children_count;
            break;

        case TAG:
            for(unsigned int i = 0; i < strlen(element->tag->name); i++)
                hash += element->tag->name[i];
            break;

        case COMMENT:
        case TEXT:
            text = (char*)element->data;
            for(unsigned int i = 0; i < strlen(text); i++)
            {
                if(i % 2)
                    hash += text[i] % element->type;
                else
                    hash += text[i] % element->type * 2;
            }
            break;
        }

    }

    if(param)
    {
        hash += param->type;
        switch(param->type)
        {
        case skin_tag_parameter::DEFAULT:
        case skin_tag_parameter::CODE:
            break;

        case skin_tag_parameter::NUMERIC:
            hash += param->data.numeric * (param->data.numeric / 4);
            break;

        case skin_tag_parameter::STRING:
            for(unsigned int i = 0; i < strlen(param->data.text); i++)
            {
                if(i % 2)
                    hash += param->data.text[i] * 2;
                else
                    hash += param->data.text[i];
            }
            break;
        }
    }

    for(int i = 0; i < children.count(); i++)
    {
        hash += children[i]->genHash();
    }

    return hash;
}

ParseTreeNode* ParseTreeNode::child(int row)
{
    if(row < 0 || row >= children.count())
        return 0;

    return children[row];
}

int ParseTreeNode::numChildren() const
{
        return children.count();
}


QVariant ParseTreeNode::data(int column) const
{
    switch(column)
    {
    case ParseTreeModel::typeColumn:
        if(element)
        {
            switch(element->type)
            {
            case UNKNOWN:
                return QObject::tr("Unknown");
            case VIEWPORT:
                return QObject::tr("Viewport");

            case LINE:
                return QObject::tr("Logical Line");

            case SUBLINES:
                return QObject::tr("Alternator");

            case COMMENT:
                return QObject::tr("Comment");

            case CONDITIONAL:
                return QObject::tr("Conditional Tag");

            case TAG:
                return QObject::tr("Tag");

            case TEXT:
                return QObject::tr("Plaintext");
            }
        }
        else if(param)
        {
            switch(param->type)
            {
            case skin_tag_parameter::STRING:
                return QObject::tr("String");

            case skin_tag_parameter::NUMERIC:
                return QObject::tr("Number");

            case skin_tag_parameter::DEFAULT:
                return QObject::tr("Default Argument");

            case skin_tag_parameter::CODE:
                return QObject::tr("This doesn't belong here");
            }
        }
        else
        {
            return QObject::tr("Root");
        }

        break;

    case ParseTreeModel::valueColumn:
        if(element)
        {
            switch(element->type)
            {
            case UNKNOWN:
            case VIEWPORT:
            case LINE:
            case SUBLINES:
                return QString();

            case CONDITIONAL:
                return QString(element->tag->name);

            case TEXT:
            case COMMENT:
                return QString((char*)element->data);

            case TAG:
                return QString(element->tag->name);
            }
        }
        else if(param)
        {
            switch(param->type)
            {
            case skin_tag_parameter::DEFAULT:
                return QObject::tr("-");

            case skin_tag_parameter::STRING:
                return QString(param->data.text);

            case skin_tag_parameter::NUMERIC:
                return QString::number(param->data.numeric, 10);

            case skin_tag_parameter::CODE:
                return QObject::tr("Seriously, something's wrong here");
            }
        }
        else
        {
            return QString();
        }
        break;

    case ParseTreeModel::lineColumn:
        if(element)
            return QString::number(element->line, 10);
        else
            return QString();
        break;
    }

    return QVariant();
}


int ParseTreeNode::getRow() const
{
    if(!parent)
        return -1;

    return parent->children.indexOf(const_cast<ParseTreeNode*>(this));
}

ParseTreeNode* ParseTreeNode::getParent() const
{
    return parent;
}

/* This version is called for the root node and for viewports */
void ParseTreeNode::render(const RBRenderInfo& info)
{
    /* Parameters don't get rendered */
    if(!element && param)
        return;

    /* If we're at the root, we need to render each viewport */
    if(!element && !param)
    {
        for(int i = 0; i < children.count(); i++)
        {
            children[i]->render(info);
        }

        return;
    }

    if(element->type != VIEWPORT)
    {
        std::cerr << QObject::tr("Error in parse tree").toStdString()
                << std::endl;
        return;
    }

    rendered = new RBViewport(element, info);

    for(int i = element->params_count; i < children.count(); i++)
        children[i]->render(info, dynamic_cast<RBViewport*>(rendered));

}

/* This version is called for logical lines, tags, conditionals and such */
void ParseTreeNode::render(const RBRenderInfo &info, RBViewport* viewport,
                           bool noBreak)
{
    if(element->type == LINE)
    {
        for(int i = 0; i < children.count(); i++)
            children[i]->render(info, viewport);
        if(!noBreak)
            viewport->newLine();
    }
    else if(element->type == TEXT)
    {
        viewport->write(QString(static_cast<char*>(element->data)));
    }
    else if(element->type == TAG)
    {
        if(!execTag(info, viewport))
            viewport->write(evalTag(info).toString());
    }
    else if(element->type == CONDITIONAL)
    {
        int child = evalTag(info, true, element->children_count).toInt();
        children[element->params_count + child]->render(info, viewport, true);
    }
    else if(element->type == SUBLINES)
    {
        /* First we build a list of the times for each branch */
        QList<double> times;
        for(int i = 0; i < children.count() ; i++)
            times.append(findBranchTime(children[i], info));

        /* Now we figure out which branch to select */
        double timeLeft = info.device()->data(QString("?pc")).toDouble();
        int branch = 0;
        while(timeLeft > 0)
        {
            timeLeft -= times[branch];
            if(timeLeft >= 0)
                branch++;
            else
                break;
            if(branch >= times.count())
                branch = 0;
        }

        /* In case we end up on a disabled branch, skip ahead.  If we find that
         * all the branches are disabled, don't render anything
         */
        int originalBranch = branch;
        while(times[branch] == 0)
        {
            branch++;
            if(branch == originalBranch)
            {
                branch = -1;
                break;
            }
            if(branch >= times.count())
                branch = 0;
        }

        /* ...and finally render the selected branch */
        if(branch >= 0)
            children[branch]->render(info, viewport, true);
    }
}

bool ParseTreeNode::execTag(const RBRenderInfo& info, RBViewport* viewport)
{

    QString filename;
    QString id;
    int x, y, tiles, tile, maxWidth, maxHeight, width, height;
    char c, hAlign, vAlign;
    RBImage* image;

    /* Two switch statements to narrow down the tag name */
    switch(element->tag->name[0])
    {

    case 'a':
        switch(element->tag->name[1])
        {
        case 'c':
            /* %ac */
            viewport->alignText(RBViewport::Center);
            return true;

        case 'l':
            /* %al */
            viewport->alignText(RBViewport::Left);
            return true;

        case 'r':
            /* %ar */
            viewport->alignText(RBViewport::Right);
            return true;
        }

        return false;

    case 'p':
        switch(element->tag->name[1])
        {
        case 'b':
            /* %pb */
            new RBProgressBar(viewport, info, element->params_count,
                              element->params);
            return true;
        }

        return false;

    case 'w':
        switch(element->tag->name[1])
        {
        case 'd':
            /* %wd */
            info.screen()->breakSBS();
            return true;

        case 'e':
            /* %we */
            /* Totally extraneous */
            return true;

        case 'i':
            /* %wi */
            viewport->enableStatusBar();
            return true;
        }

        return false;

    case 'x':
        switch(element->tag->name[1])
        {
        case 'd':
            /* %xd */
            id = "";
            id.append(element->params[0].data.text[0]);
            c = element->params[0].data.text[1];

            if(c == '\0')
            {
                tile = 1;
            }
            else
            {
                if(isupper(c))
                    tile = c - 'A' + 25;
                else
                    tile = c - 'a';
            }

            if(info.screen()->getImage(id))
            {
                image = new RBImage(*(info.screen()->getImage(id)), viewport);
                image->setTile(tile);
                image->show();
            }

            return true;

        case 'l':
            /* %xl */
            id = element->params[0].data.text;
            filename = info.settings()->value("imagepath", "") + "/" +
                       element->params[1].data.text;
            x = element->params[2].data.numeric;
            y = element->params[3].data.numeric;
            if(element->params_count > 4)
                tiles = element->params[4].data.numeric;
            else
                tiles = 1;

            info.screen()->loadImage(id, new RBImage(filename, tiles, x, y,
                                                     viewport));
            return true;

        case '\0':
            /* %x */
            id = element->params[0].data.text;
            filename = info.settings()->value("imagepath", "") + "/" +
                       element->params[1].data.text;
            x = element->params[2].data.numeric;
            y = element->params[3].data.numeric;
            image = new RBImage(filename, 1, x, y, viewport);
            info.screen()->loadImage(id, new RBImage(filename, 1, x, y,
                                                     viewport));
            info.screen()->getImage(id)->show();
            return true;

        }

        return false;

    case 'C':
        switch(element->tag->name[1])
        {
        case 'd':
            /* %Cd */
            info.screen()->showAlbumArt(viewport);
            return true;

        case 'l':
            /* %Cl */
            x = element->params[0].data.numeric;
            y = element->params[1].data.numeric;
            maxWidth = element->params[2].data.numeric;
            maxHeight = element->params[3].data.numeric;
            hAlign = element->params_count > 4
                     ? element->params[4].data.text[0] : 'c';
            vAlign = element->params_count > 5
                     ? element->params[5].data.text[0] : 'c';
            width = info.device()->data("artwidth").toInt();
            height = info.device()->data("artheight").toInt();
            info.screen()->setAlbumArt(new RBAlbumArt(viewport, x, y, maxWidth,
                                                      maxHeight, width, height,
                                                      hAlign, vAlign));
            return true;
        }

        return false;

    case 'F':

        switch(element->tag->name[1])
        {

        case 'l':
            /* %Fl */
            x = element->params[0].data.numeric;
            filename = info.settings()->value("themebase", "") + "/fonts/" +
                       element->params[1].data.text;
            info.screen()->loadFont(x, new RBFont(filename));
            return true;

        }

        return false;

    case 'V':

        switch(element->tag->name[1])
        {

        case 'b':
            /* %Vb */
            viewport->setBGColor(RBScreen::
                                 stringToColor(QString(element->params[0].
                                                       data.text),
                                               Qt::white));
            return true;

        case 'd':
            /* %Vd */
            id = element->params[0].data.text;
            info.screen()->showViewport(id);
            return true;

        case 'f':
            /* %Vf */
            viewport->setFGColor(RBScreen::
                                 stringToColor(QString(element->params[0].
                                                       data.text),
                                               Qt::black));
            return true;

        case 'p':
            /* %Vp */
            viewport->showPlaylist(info, element->params[0].data.numeric,
                                   element->params[1].data.code,
                                   element->params[2].data.code);
            return true;

        case 'I':
            /* %VI */
            info.screen()->makeCustomUI(element->params[0].data.text);
            return true;

        }

        return false;

    case 'X':

        switch(element->tag->name[1])
        {
        case '\0':
            /* %X */
            filename = QString(element->params[0].data.text);
            info.screen()->setBackdrop(filename);
            return true;
        }

        return false;

    }

    return false;

}

QVariant ParseTreeNode::evalTag(const RBRenderInfo& info, bool conditional,
                                int branches)
{
    if(!conditional)
    {
        return info.device()->data(QString(element->tag->name));
    }
    else
    {
        /* If we're evaluating for a conditional, we return the child branch
         * index that should be selected.  For true/false values, this is
         * 0 for true, 1 for false, and we also have to make sure not to
         * ever exceed the number of available children
         */

        int child;
        QVariant val = info.device()->data("?" + QString(element->tag->name));
        if(val.isNull())
            val = info.device()->data(QString(element->tag->name));

        if(val.isNull())
        {
            child = 1;
        }
        else if(QString(element->tag->name) == "bl")
        {
            /* bl has to be scaled to the number of available children, but it
             * also has an initial -1 value for an unknown state */
            child = val.toInt();
            if(child == -1)
            {
                child = 0;
            }
            else
            {
                child = ((branches - 1) * child / 100) + 1;
            }
        }
        else if(QString(element->tag->name) == "px")
        {
            child = val.toInt();
            child = branches * child / 100;
        }
        else if(val.type() == QVariant::Bool)
        {
            /* Boolean values have to be reversed, because conditionals are
             * always of the form %?tag<true|false>
             */
            if(val.toBool())
                child = 0;
            else
                child = 1;
        }
        else if(val.type() == QVariant::String)
        {
            if(val.toString().length() > 0)
                child = 0;
            else
                child = 1;
        }
        else
        {
            child = val.toInt();
        }

        if(child < 0)
            child = 0;

        if(child < branches)
            return child;
        else
            return branches - 1;
    }
}

double ParseTreeNode::findBranchTime(ParseTreeNode *branch,
                                     const RBRenderInfo& info)
{
    double retval = 2;
    for(int i = 0; i < branch->children.count(); i++)
    {
        ParseTreeNode* current = branch->children[i];
        if(current->element->type == TAG)
        {
            if(current->element->tag->name[0] == 't'
               && current->element->tag->name[1] == '\0')
            {
                retval = atof(current->element->params[0].data.text);
            }
        }
        else if(current->element->type == CONDITIONAL)
        {
            retval = findConditionalTime(current, info);
        }
    }
    return retval;
}

double ParseTreeNode::findConditionalTime(ParseTreeNode *conditional,
                                          const RBRenderInfo& info)
{
    int child = conditional->evalTag(info, true,
                                     conditional->children.count()).toInt();
    return findBranchTime(conditional->children[child], info);
}
