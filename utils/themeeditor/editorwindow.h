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

#ifndef EDITORWINDOW_H
#define EDITORWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QItemSelectionModel>

#include "parsetreemodel.h"
#include "skinhighlighter.h"
#include "skindocument.h"
#include "configdocument.h"
#include "preferencesdialog.h"
#include "skinviewer.h"

class ProjectModel;
class TabContent;

namespace Ui
{
    class EditorWindow;
}

class EditorWindow : public QMainWindow
{
    Q_OBJECT
public:
    EditorWindow(QWidget *parent = 0);
    ~EditorWindow();

    /* A public function so external widgets can load files */
    void loadTabFromSkinFile(QString fileName);
    void loadConfigTab(ConfigDocument* doc);

protected:
    virtual void closeEvent(QCloseEvent* event);

public slots:
    void configFileChanged(QString configFile);

private slots:
    void showPanel();
    void newTab();
    void shiftTab(int index);
    bool closeTab(int index);
    void closeCurrent();
    void saveCurrent();
    void saveCurrentAs();
    void openFile();
    void openProject();
    void tabTitleChanged(QString title);
    void updateCurrent(); /* Generates code in the current tab */
    void lineChanged(int line); /* Used for auto-expand */

private:
    /* Setup functions */
    void loadSettings();
    void saveSettings();
    void setupUI();
    void setupMenus();
    void addTab(TabContent* doc);
    void expandLine(ParseTreeModel* model, QModelIndex parent, int line);
    void sizeColumns();

    Ui::EditorWindow *ui;
    PreferencesDialog* prefs;
    QLabel* parseStatus;
    ProjectModel* project;
    QItemSelectionModel* parseTreeSelection;
    SkinViewer* viewer;
};

#endif // EDITORWINDOW_H
