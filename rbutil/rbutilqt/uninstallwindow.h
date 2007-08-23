/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2007 by Dominik Wenger
 *   $Id$
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef UNINSTALLWINDOW_H
#define UNINSTALLWINDOW_H

#include <QtGui>

#include <QSettings>

#include "ui_uninstallfrm.h"
#include "progressloggergui.h"
#include "uninstall.h"

class UninstallWindow : public QDialog
{
    Q_OBJECT
    public:
        UninstallWindow(QWidget *parent = 0);
        void setUserSettings(QSettings*);
        void setDeviceSettings(QSettings*);

    public slots:
        void accept(void);

    private slots:
        void selectionChanged();
        void UninstallMethodChanged(bool complete);
    private:
        Uninstaller* uninstaller;
        Ui::UninstallFrm ui;
        ProgressLoggerGui* logger;
        QSettings *devices;
        QSettings *userSettings;

};


#endif
