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


#ifndef TALKFILE_H
#define TALKFILE_H

#include "progressloggerinterface.h"

class TalkFileCreator :public QObject
{
    Q_OBJECT

public:
    TalkFileCreator(QObject* parent=0);

    bool createTalkFiles(ProgressloggerInterface* logger);

    void setTTSexe(QString exe){m_TTSexec=exe;}
    void setEncexe(QString exe){m_EncExec=exe;}

    void setTTsType(QString tts) { m_curTTS = tts; }
    void setTTsOpts(QString opts) {m_TTSOpts=opts;}
    void setTTsTemplate(QString t) { m_curTTSTemplate = t; }

    void setEncType(QString enc) { m_curEnc = enc; }
    void setEncOpts(QString opts) {m_EncOpts=opts;}
    void setEncTemplate(QString t) { m_curEncTemplate = t; }

    void setDir(QString dir){m_dir = dir; }
    void setMountPoint(QString mountpoint) {m_mountpoint =mountpoint; }

    void setOverwriteTalk(bool ov) {m_overwriteTalk = ov;}
    void setOverwriteWav(bool ov) {m_overwriteWav = ov;}
    void setRemoveWav(bool ov) {m_removeWav = ov;}
    void setRecursive(bool ov) {m_recursive = ov;}
    void setStripExtensions(bool ov) {m_stripExtensions = ov;}

private slots:
    void abort();

private:

    bool initTTS();
    bool stopTTS();
    bool initEncoder();

    bool encode(QString input,QString output);
    bool voice(QString text,QString wavfile);

    QString m_dir;
    QString m_mountpoint;
    QString m_curTTS;
    QString m_TTSexec;
    QString m_TTSOpts;
    QString m_curTTSTemplate;

    QString m_curEnc;
    QString m_EncExec;
    QString m_EncOpts;
    QString m_curEncTemplate;

    bool m_overwriteTalk;
    bool m_overwriteWav;
    bool m_removeWav;
    bool m_recursive;
    bool m_stripExtensions;

    ProgressloggerInterface* m_logger;

    bool m_abort;
};

#endif
