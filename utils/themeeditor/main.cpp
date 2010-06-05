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

#include "skin_parser.h"
#include "skin_debug.h"
#include "editorwindow.h"

#include <cstdlib>
#include <cstdio>
#include <iostream>

#include <QtGui/QApplication>

#include "parsetreemodel.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    QCoreApplication::setApplicationName(QObject::tr("Rockbox Theme Editor"));
    QCoreApplication::setApplicationVersion(QObject::tr("Pre-Alpha"));
    QCoreApplication::setOrganizationName(QObject::tr("Rockbox"));

    EditorWindow mainWindow;
    mainWindow.show();

    return app.exec();

}

