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
#ifndef PROGRESSLOGGERGUI_H
#define PROGRESSLOGGERGUI_H

#include <QtGui>

#include "progressloggerinterface.h"
#include "ui_installprogressfrm.h"

class ProgressLoggerGui :public ProgressloggerInterface
{
    Q_OBJECT
public:
    ProgressLoggerGui(QObject * parent);

    virtual void addItem(const QString &text);  //adds a string to the list

    virtual void addItem(const QString &text, int flag);  //adds a string to the list

    virtual void setProgressValue(int value);
    virtual void setProgressMax(int max);
    virtual int getProgressMax();

signals:
    virtual void aborted();
    virtual void closed();

public slots:
    virtual void abort();
    virtual void undoAbort();
    virtual void close();
    virtual void show();

private:
    Ui::InstallProgressFrm dp;
    QDialog *downloadProgress;

};

#endif

