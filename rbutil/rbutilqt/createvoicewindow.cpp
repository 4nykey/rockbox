/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2007 by Dominik Riebeling
 *   $Id$
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "createvoicewindow.h"
#include "ui_createvoicefrm.h"

#include "browsedirtree.h"
#include "configure.h"

CreateVoiceWindow::CreateVoiceWindow(QWidget *parent) : QDialog(parent)
{
    ui.setupUi(this);
    voicecreator = new VoiceFileCreator(this);
    
    connect(ui.change,SIGNAL(clicked()),this,SLOT(change()));
}

void CreateVoiceWindow::change()
{
    Config *cw = new Config(this,4);
    cw->setSettings(settings);
    connect(cw, SIGNAL(settingsUpdated()), this, SLOT(updateSettings()));
    cw->show();    
}

void CreateVoiceWindow::accept()
{
    logger = new ProgressLoggerGui(this);
    connect(logger,SIGNAL(closed()),this,SLOT(close()));
    logger->show();    
    
    QString lang = ui.comboLanguage->currentText();
    int wvThreshold = ui.wavtrimthreshold->value();
    
    //safe selected language
    settings->setValue(RbSettings::Language, lang);
    settings->setValue(RbSettings::WavtrimThreshold, wvThreshold);
    settings->sync();
    
    //configure voicecreator
    voicecreator->setSettings(settings);
    voicecreator->setMountPoint(settings->value(RbSettings::Mountpoint).toString());
    voicecreator->setTargetId(settings->value(RbSettings::CurTargetId).toInt());
    voicecreator->setLang(lang);
    voicecreator->setWavtrimThreshold(wvThreshold);
       
    //start creating
    voicecreator->createVoiceFile(logger);
}


/** @brief set settings object
 */
void CreateVoiceWindow::setSettings(RbSettings* sett)
{
    settings = sett;
    updateSettings();
}


/** @brief update displayed settings
 */
void CreateVoiceWindow::updateSettings(void)
{
    // fill in language combobox
    QStringList languages = settings->languages();
    languages.sort();
    ui.comboLanguage->addItems(languages);
    // set saved lang
    int sel = ui.comboLanguage->findText(settings->value(RbSettings::VoiceLanguage).toString());
    // if no saved language is found try to figure the language from the UI lang
    if(sel == -1) {
        QString f = settings->value(RbSettings::Language).toString();
        // if no language is set default to english. Make sure not to check an empty string.
        if(f.isEmpty()) f = "english";
        sel = ui.comboLanguage->findText(f, Qt::MatchStartsWith);
        qDebug() << "sel =" << sel;
        // still nothing found?
        if(sel == -1)
            sel = ui.comboLanguage->findText("english", Qt::MatchStartsWith);
    }
    ui.comboLanguage->setCurrentIndex(sel);
    
    QString ttsName = settings->value(RbSettings::Tts).toString();
    TTSBase* tts = TTSBase::getTTS(ttsName);
    tts->setCfg(settings);
    if(tts->configOk())
        ui.labelTtsProfile->setText(tr("Selected TTS engine: <b>%1</b>")
            .arg(TTSBase::getTTSName(ttsName)));
    else
        ui.labelTtsProfile->setText(tr("Selected TTS engine: <b>%1</b>")
            .arg("Invalid TTS configuration!"));
    
    QString encoder = settings->value(RbSettings::CurEncoder).toString();
    // only proceed if encoder setting is set
    EncBase* enc = EncBase::getEncoder(encoder);
    if(enc != NULL) {
        enc->setCfg(settings);
        if(enc->configOk())
            ui.labelEncProfile->setText(tr("Selected encoder: <b>%1</b>")
                .arg(EncBase::getEncoderName(encoder)));
        else
            ui.labelEncProfile->setText(tr("Selected encoder: <b>%1</b>")
                .arg("Invalid encoder configuration!"));
    }
    else
        ui.labelEncProfile->setText(tr("Selected encoder: <b>%1</b>")
            .arg("Invalid encoder configuration!"));
    ui.wavtrimthreshold->setValue(settings->value(RbSettings::WavtrimThreshold).toInt());
    emit settingsUpdated();
}





