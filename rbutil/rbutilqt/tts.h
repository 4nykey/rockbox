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
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

 
#ifndef TTS_H
#define TTS_H

#include "rbsettings.h"
#include <QtCore>
#include <QProcess>
#include <QProgressDialog>
#include <QDateTime>
#include <QRegExp>
#include <QTcpSocket>

#ifndef CONSOLE
#include "ttsgui.h"
#else
#include "ttsguicli.h"
#endif

enum TTSStatus{ FatalError, NoError, Warning };
class TTSSapi;
#if defined(Q_OS_LINUX)
class TTSFestival;
#endif
class TTSBase : public QObject
{
    Q_OBJECT
    public:
        TTSBase();
        virtual TTSStatus voice(QString text,QString wavfile, QString* errStr)
        { (void) text; (void) wavfile; (void) errStr; return FatalError;}
        virtual bool start(QString *errStr) { (void)errStr; return false; }
        virtual bool stop() { return false; }
        virtual void showCfg(){}
        virtual bool configOk() { return false; }

        virtual void setCfg(RbSettings* sett) { settings = sett; }
        
        static TTSBase* getTTS(QString ttsname);
        static QStringList getTTSList();
        static QString getTTSName(QString tts);
        
    public slots:
        virtual void accept(void){}
        virtual void reject(void){}
        virtual void reset(void){}
        
    private:
        //inits the tts List
        static void initTTSList();

    protected:
        RbSettings* settings;
        static QMap<QString,QString> ttsList;
        static QMap<QString,TTSBase*> ttsCache;
};

class TTSSapi : public TTSBase
{
 Q_OBJECT
    public:
        TTSSapi();
        virtual TTSStatus voice(QString text,QString wavfile, QString *errStr);
        virtual bool start(QString *errStr);
        virtual bool stop();
        virtual void showCfg();
        virtual bool configOk();
    
        QStringList getVoiceList(QString language);
    private:
        QProcess* voicescript;
        QTextStream* voicestream;
        QString defaultLanguage;
        
        QString m_TTSexec;
        QString m_TTSOpts;
        QString m_TTSTemplate;
        QString m_TTSLanguage;
        QString m_TTSVoice;
        QString m_TTSSpeed;
        bool m_sapi4;
};


class TTSExes : public TTSBase
{
    Q_OBJECT
    public:
        TTSExes(QString name);
        virtual TTSStatus voice(QString text,QString wavfile, QString *errStr);
        virtual bool start(QString *errStr);
        virtual bool stop() {return true;}
        virtual void showCfg();
        virtual bool configOk();

        virtual void setCfg(RbSettings* sett);
        
    private:
        QString m_name;
        QString m_TTSexec;
        QString m_TTSOpts;
        QString m_TTSTemplate;
        QMap<QString,QString> m_TemplateMap;
};

class TTSFestival : public TTSBase
{
	Q_OBJECT
public:
	~TTSFestival();
	virtual bool configOk();
	virtual bool start(QString *errStr);
	virtual bool stop();
	virtual void showCfg();
	virtual TTSStatus voice(QString text,QString wavfile,  QString *errStr);

	QStringList  getVoiceList();
	QString 	 getVoiceInfo(QString voice);
private:
	inline void	startServer();
	inline void	ensureServerRunning();
	QString	queryServer(QString query, int timeout = -1);
	QProcess serverProcess;
	QStringList voices;
	QMap<QString, QString> voiceDescriptions;
};

#endif
