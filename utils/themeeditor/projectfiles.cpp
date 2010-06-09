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

#include "projectfiles.h"

ProjectFiles::ProjectFiles(QHash<QString, QString>& settings,
                           ProjectModel* model, ProjectNode* parent)
                               : parentLink(parent)
{
    base = settings.value("themebase");

    QList<QString> keys;
    keys.append("wps");
    keys.append("rwps");
    keys.append("sbs");
    keys.append("rsbs");
    keys.append("fms");
    keys.append("rfms");

    for(int i = 0; i < keys.count(); i++)
    {
        QString file = settings.value(keys[i], "");
        if(file != "" && file != "-")
        {
            file.replace("/.rockbox/", "");
            children.append(new ProjectFile(file, model, this));
        }
    }
}

ProjectFiles::~ProjectFiles()
{
    for(int i = 0; i < children.count(); i++)
        delete children[i];
}

ProjectNode* ProjectFiles::parent() const
{
    return parentLink;
}

ProjectNode* ProjectFiles::child(int row) const
{
    if(row >= 0 && row < children.count())
        return children[row];

    return 0;
}

int ProjectFiles::numChildren() const
{
    return children.count();
}

int ProjectFiles::row() const
{
    return parentLink->indexOf(const_cast<ProjectFiles*>(this));
}

QVariant ProjectFiles::data(int column) const
{
    if(column == 0)
        return QObject::tr("Project Files");
    else
        return QVariant();
}

Qt::ItemFlags ProjectFiles::flags(int column) const
{
    if(column == 0)
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    else
        return 0;
}

void ProjectFiles::activated()
{

}

/* Project File functions */
ProjectFile::ProjectFile(QString file, ProjectModel* model,
                         ProjectNode* parent)
                             :parentLink(parent), file(file)
{
    this->model = model;
}

ProjectFile::~ProjectFile()
{

}

QVariant ProjectFile::data(int column) const
{
    if(column == 0)
        return file;
    else
        return QVariant();
}

Qt::ItemFlags ProjectFile::flags(int column) const
{
    if(column == 0)
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    else
        return 0;
}

void ProjectFile::activated()
{
    QString base = dynamic_cast<ProjectFiles*>(parentLink)->getBase();
    model->loadFile(base + "/" + file);
}

