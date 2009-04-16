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

#include "rbsettings.h"

#include <QSettings>

#if defined(Q_OS_LINUX)
#include <unistd.h>
#endif

void RbSettings::open()
{
    // only use built-in rbutil.ini
    devices = new QSettings(":/ini/rbutil.ini", QSettings::IniFormat, 0);
     // portable installation:
    // check for a configuration file in the program folder.
    QFileInfo config;
    config.setFile(QCoreApplication::instance()->applicationDirPath()
            + "/RockboxUtility.ini");
    if(config.isFile())
    {
        userSettings = new QSettings(QCoreApplication::instance()->applicationDirPath()
                + "/RockboxUtility.ini", QSettings::IniFormat, this);
        qDebug() << "config: portable";
    }
    else
    {
        userSettings = new QSettings(QSettings::IniFormat,
            QSettings::UserScope, "rockbox.org", "RockboxUtility",this);
        qDebug() << "config: system";
    }
}

void RbSettings::sync()
{
    userSettings->sync();
#if defined(Q_OS_LINUX)
    // when using sudo it runs rbutil with uid 0 but unfortunately without a
    // proper login shell, thus the configuration file gets placed in the
    // calling users $HOME. This in turn will cause issues if trying to
    // run rbutil later as user. Try to detect this case via the environment
    // variable SUDO_UID and SUDO_GID and if set chown the user config file.
    if(getuid() == 0)
    {
        char* realuser = getenv("SUDO_UID");
        char* realgroup = getenv("SUDO_GID");
        if(realuser != NULL && realgroup != NULL)
        {
            int realuid = atoi(realuser);
            int realgid = atoi(realgroup);
            // chown is attribute warn_unused_result, but in case this fails
            // we can't do anything useful about it. Notifying the user
            // is somewhat pointless. Add hack to suppress compiler warning.
            if(chown(qPrintable(userSettings->fileName()), realuid, realgid));
        }
    }
#endif
}

QVariant RbSettings::deviceSettingCurGet(QString entry,QString def)
{
    QString platform = userSettings->value("platform").toString();
    devices->beginGroup(platform);
    QVariant result = devices->value(entry,def);
    devices->endGroup();
    return result;
}

QVariant RbSettings::userSettingsGroupGet(QString group,QString entry,QVariant def)
{
    userSettings->beginGroup(group);
    QVariant result = userSettings->value(entry,def);
    userSettings->endGroup();
    return result;
}

void RbSettings::userSettingsGroupSet(QString group,QString entry,QVariant value)
{
    userSettings->beginGroup(group);
    userSettings->setValue(entry,value);
    userSettings->endGroup();
}

QString RbSettings::userSettingFilename()
{
    return userSettings->fileName();
}

QString RbSettings::curVersion()
{
    return userSettings->value("rbutil_version").toString();
}

bool RbSettings::cacheOffline()
{
    return userSettings->value("offline").toBool();
}

QString RbSettings::mountpoint()
{
    return userSettings->value("mountpoint").toString();
}

QString RbSettings::manualUrl()
{
    return devices->value("manual_url").toString();
}

QString RbSettings::bleedingUrl()
{
    return devices->value("bleeding_url").toString();
}

QString RbSettings::cachePath()
{
    return userSettings->value("cachepath", QDir::tempPath()).toString();
}

QString RbSettings::build()
{
    return userSettings->value("build").toString();
}

QString RbSettings::bootloaderUrl()
{
    return devices->value("bootloader_url").toString();
}

QString RbSettings::bootloaderInfoUrl()
{
    return devices->value("bootloader_info_url").toString();
}

QString RbSettings::fontUrl()
{
    return devices->value("font_url").toString();
}

QString RbSettings::voiceUrl()
{
    return devices->value("voice_url").toString();
}

QString RbSettings::doomUrl()
{
    return devices->value("doom_url").toString();
}

QString RbSettings::releaseUrl()
{
    return devices->value("release_url").toString();
}

QString RbSettings::dailyUrl()
{
    return devices->value("daily_url").toString();
}

QString RbSettings::serverConfUrl()
{
    return devices->value("server_conf_url").toString();
}

QString RbSettings::genlangUrl()
{
    return devices->value("genlang_url").toString();
}

QString RbSettings::themeUrl()
{
    return devices->value("themes_url").toString();
}

QString RbSettings::bleedingInfo()
{
    return devices->value("bleeding_info").toString();
}

bool RbSettings::cacheDisabled()
{
    return userSettings->value("cachedisable").toBool();
}

QString RbSettings::proxyType()
{
    return userSettings->value("proxytype", "none").toString();
}

QString RbSettings::proxy()
{
    return userSettings->value("proxy").toString();
}

QString RbSettings::ofPath()
{
    return userSettings->value("ofpath").toString();
}

QString RbSettings::curBrand()
{
    QString platform = userSettings->value("platform").toString();
    return brand(platform);
}

QString RbSettings::curName()
{
    QString platform = userSettings->value("platform").toString();
    return name(platform);
}

QString RbSettings::curPlatform()
{
    return userSettings->value("platform").toString();
}

QString RbSettings::curBuildserver_Modelname()
{
    return deviceSettingCurGet("buildserver_modelname").toString();
}

QString RbSettings::curManual()
{
    return deviceSettingCurGet("manualname","rockbox-" +
            devices->value("platform").toString()).toString();
}

QString RbSettings::curBootloaderMethod()
{
    return deviceSettingCurGet("bootloadermethod", "none").toString();
}

QString RbSettings::curBootloaderName()
{
    return deviceSettingCurGet("bootloadername").toString();
}

QString RbSettings::curBootloaderFile()
{
    return deviceSettingCurGet("bootloaderfile").toString();
}

QString RbSettings::curConfigure_Modelname()
{
    return deviceSettingCurGet("configure_modelname").toString();
}

QString RbSettings::curLang()
{
    // QSettings::value only returns the default when the setting
    // doesn't exist. Make sure to return the system language if
    // the language in the configuration is present but empty too.
    QString lang = userSettings->value("lang").toString();
    if(lang.isEmpty())
        lang = QLocale::system().name();
    return lang;
}

QString RbSettings::curEncoder()
{
    return deviceSettingCurGet("encoder").toString();
}

QString RbSettings::curTTS()
{
    return userSettings->value("tts").toString();
}

QString RbSettings::lastTalkedFolder()
{
    return userSettings->value("last_talked_folder").toString();
}

QString RbSettings::voiceLanguage()
{
    return userSettings->value("voicelanguage").toString();
}

int RbSettings::wavtrimTh()
{
    return userSettings->value("wavtrimthreshold",500).toInt();
}

QString RbSettings::ttsPath(QString tts)
{
    return userSettingsGroupGet(tts,"ttspath").toString();
}
QString RbSettings::ttsOptions(QString tts)
{
    return userSettingsGroupGet(tts,"ttsoptions").toString();
}
QString RbSettings::ttsVoice(QString tts)
{
    return userSettingsGroupGet(tts,"ttsvoice","Microsoft Sam").toString();
}
int RbSettings::ttsSpeed(QString tts)
{
    return userSettingsGroupGet(tts,"ttsspeed",0).toInt();
}
QString RbSettings::ttsLang(QString tts)
{
    return userSettingsGroupGet(tts,"ttslanguage","english").toString();
}

bool RbSettings::ttsUseSapi4()
{
    return userSettingsGroupGet("sapi","useSapi4",false).toBool();
}

QString RbSettings::encoderPath(QString enc)
{
    return userSettingsGroupGet(enc,"encoderpath").toString();
}
QString RbSettings::encoderOptions(QString enc)
{
    return userSettingsGroupGet(enc,"encoderoptions").toString();
}

double RbSettings::encoderQuality(QString enc)
{
    return userSettingsGroupGet(enc,"quality",8.f).toDouble();
}
int RbSettings::encoderComplexity(QString enc)
{
    return userSettingsGroupGet(enc,"complexity",10).toInt();
}
double RbSettings::encoderVolume(QString enc)
{
    return userSettingsGroupGet(enc,"volume",1.f).toDouble();
}
bool RbSettings::encoderNarrowband(QString enc)
{
    return userSettingsGroupGet(enc,"narrowband",false).toBool();
}

QStringList RbSettings::allPlatforms()
{
    QStringList result;
    devices->beginGroup("platforms");
    QStringList a = devices->childKeys();
    for(int i = 0; i < a.size(); i++)
    {
        result.append(devices->value(a.at(i), "null").toString());
    }
    devices->endGroup();
    return result;
}

QStringList RbSettings::allLanguages()
{
    QStringList result;
    devices->beginGroup("languages");
    QStringList a = devices->childKeys();
    for(int i = 0; i < a.size(); i++)
    {
        result.append(devices->value(a.at(i), "null").toString());
    }
    devices->endGroup();
    return result;
}

QString RbSettings::name(QString plattform)
{
    devices->beginGroup(plattform);
    QString name = devices->value("name").toString();
    devices->endGroup();
    return name;
}

QString RbSettings::brand(QString plattform)
{
    devices->beginGroup(plattform);
    QString brand = devices->value("brand").toString();
    devices->endGroup();
    return brand;
}

QMap<int, QString> RbSettings::usbIdMap(enum MapType type)
{
    QMap<int, QString> map;
     // get a list of ID -> target name
    QStringList platforms;
    devices->beginGroup("platforms");
    platforms = devices->childKeys();
    devices->endGroup();

    QString t;
    switch(type) {
        case MapDevice:
            t = "usbid";
            break;
        case MapError:
            t = "usberror";
            break;
        case MapIncompatible:
            t = "usbincompat";
            break;
    }

    for(int i = 0; i < platforms.size(); i++)
    {
        devices->beginGroup("platforms");
        QString target = devices->value(platforms.at(i)).toString();
        devices->endGroup();
        devices->beginGroup(target);
        QStringList ids = devices->value(t).toStringList();
        int j = ids.size();
        while(j--)
            map.insert(ids.at(j).toInt(0, 16), target);

        devices->endGroup();
    }
    return map;
}


QString RbSettings::curResolution()
{
    return deviceSettingCurGet("resolution").toString();
}

int RbSettings::curTargetId()
{
    return deviceSettingCurGet("targetid").toInt();
}

void RbSettings::setCurVersion(QString version)
{
    userSettings->setValue("rbutil_version",version);
}

void RbSettings::setOfPath(QString path)
{
    userSettings->setValue("ofpath",path);
}

void RbSettings::setCachePath(QString path)
{
    userSettings->setValue("cachepath", path);
}

void RbSettings::setBuild(QString build)
{
    userSettings->setValue("build", build);
}

void RbSettings::setLastTalkedDir(QString dir)
{
    userSettings->setValue("last_talked_folder", dir);
}

void RbSettings::setVoiceLanguage(QString dir)
{
    userSettings->setValue("voicelanguage", dir);
}

void RbSettings::setWavtrimTh(int th)
{
    userSettings->setValue("wavtrimthreshold", th);
}

void RbSettings::setProxy(QString proxy)
{
    userSettings->setValue("proxy", proxy);
}

void RbSettings::setProxyType(QString proxytype)
{
    userSettings->setValue("proxytype", proxytype);
}

void RbSettings::setLang(QString lang)
{
    userSettings->setValue("lang", lang);
}

void RbSettings::setMountpoint(QString mp)
{
    userSettings->setValue("mountpoint",mp);
}

void RbSettings::setCurPlatform(QString platt)
{
    userSettings->setValue("platform",platt);
}


void RbSettings::setCacheDisable(bool on)
{
    userSettings->setValue("cachedisable",on);
}

void RbSettings::setCacheOffline(bool on)
{
    userSettings->setValue("offline",on);
}

void RbSettings::setCurTTS(QString tts)
{
    userSettings->setValue("tts",tts);
}

void RbSettings::setTTSPath(QString tts, QString path)
{
    userSettingsGroupSet(tts,"ttspath",path);
}

void RbSettings::setTTSOptions(QString tts, QString options)
{
    userSettingsGroupSet(tts,"ttsoptions",options);
}

void RbSettings::setTTSVoice(QString tts, QString voice)
{
    userSettingsGroupSet(tts,"ttsvoice",voice);
}

void RbSettings::setTTSSpeed(QString tts, int speed)
{
    userSettingsGroupSet(tts,"ttsspeed",speed);
}

void RbSettings::setTTSLang(QString tts, QString lang)
{
    userSettingsGroupSet(tts,"ttslanguage",lang);
}

void RbSettings::setTTSUseSapi4(bool value)
{
    userSettingsGroupSet("sapi","useSapi4",value);
}

void RbSettings::setEncoderPath(QString enc, QString path)
{
    userSettingsGroupSet(enc,"encoderpath",path);
}

void RbSettings::setEncoderOptions(QString enc, QString options)
{
    userSettingsGroupSet(enc,"encoderoptions",options);
}

void RbSettings::setEncoderQuality(QString enc, double q)
{
    userSettingsGroupSet(enc,"quality",q);
}
void RbSettings::setEncoderComplexity(QString enc, int c)
{
    userSettingsGroupSet(enc,"complexity",c);
}
void RbSettings::setEncoderVolume(QString enc,double v)
{
    userSettingsGroupSet(enc,"volume",v);
}
void RbSettings::setEncoderNarrowband(QString enc,bool nb)
{
    userSettingsGroupSet(enc,"narrowband",nb);
}

