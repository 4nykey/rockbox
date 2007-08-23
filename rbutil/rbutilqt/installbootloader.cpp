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
 
#include "installbootloader.h"
#include "irivertools/checksums.h"

BootloaderInstaller::BootloaderInstaller(QObject* parent): QObject(parent) 
{

}

void BootloaderInstaller::install(ProgressloggerInterface* dp)
{
    m_dp = dp;
    m_install = true;
    m_dp->addItem(tr("Starting bootloader installation"),LOGINFO);
    connect(this, SIGNAL(done(bool)), this, SLOT(installEnded(bool)));
    
    if(m_bootloadermethod == "gigabeatf")
    {
        // connect internal signal
        connect(this,SIGNAL(prepare()),this,SLOT(gigabeatPrepare())); 
        connect(this,SIGNAL(finish()),this,SLOT(gigabeatFinish())); 
    }
    else if(m_bootloadermethod == "iaudio")
    {
        // connect internal signal
        connect(this,SIGNAL(prepare()),this,SLOT(iaudioPrepare()));  
        connect(this,SIGNAL(finish()),this,SLOT(iaudioFinish()));       
    }
    else if(m_bootloadermethod == "h10")
    {
        // connect internal signal
        connect(this,SIGNAL(prepare()),this,SLOT(h10Prepare()));  
        connect(this,SIGNAL(finish()),this,SLOT(h10Finish()));       
    }
    else if(m_bootloadermethod == "ipodpatcher")
    {
        // connect internal signal
        connect(this,SIGNAL(prepare()),this,SLOT(ipodPrepare()));  
        connect(this,SIGNAL(finish()),this,SLOT(ipodFinish()));       
    }       
    else if(m_bootloadermethod == "sansapatcher")
    {
        // connect internal signal
        connect(this,SIGNAL(prepare()),this,SLOT(sansaPrepare()));  
        connect(this,SIGNAL(finish()),this,SLOT(sansaFinish()));       
    }       
    else if(m_bootloadermethod == "fwpatcher")
    {
        // connect internal signal
        connect(this,SIGNAL(prepare()),this,SLOT(iriverPrepare()));  
        connect(this,SIGNAL(finish()),this,SLOT(iriverFinish()));       
    }       
    else
    {
        m_dp->addItem(tr("unsupported install Method"),LOGERROR);
        emit done(true);
        return;
    }
    
    emit prepare();
}

void BootloaderInstaller::uninstall(ProgressloggerInterface* dp)
{
    m_dp = dp;
    m_install = false;
    m_dp->addItem(tr("Starting bootloader uninstallation"),LOGINFO);
    connect(this, SIGNAL(done(bool)), this, SLOT(installEnded(bool)));
    
    if(m_bootloadermethod == "gigabeatf")
    {
       // connect internal signal
       connect(this,SIGNAL(prepare()),this,SLOT(gigabeatPrepare())); 
    }
    else if(m_bootloadermethod == "iaudio")
    {
        m_dp->addItem(tr("No uninstallation possible"),LOGWARNING);
        emit done(true);
        return;      
    }
    else if(m_bootloadermethod == "iaudio")
    {
        // connect internal signal
       connect(this,SIGNAL(prepare()),this,SLOT(h10Prepare())); 
    }
    else if(m_bootloadermethod == "ipodpatcher")
    {
        // connect internal signal
        connect(this,SIGNAL(prepare()),this,SLOT(ipodPrepare())); 
    }
    else if(m_bootloadermethod == "sansapatcher")
    {
        // connect internal signal
        connect(this,SIGNAL(prepare()),this,SLOT(sansaPrepare())); 
    }
    else if(m_bootloadermethod == "fwpatcher")
    {
        m_dp->addItem(tr("No uninstallation possible"),LOGWARNING);
        emit done(true);
        return;      
    }
    else
    {
        m_dp->addItem(tr("unsupported install Method"),LOGERROR);
        emit done(true);
        return;
    }
    
    emit prepare();
}

void BootloaderInstaller::downloadRequestFinished(int id, bool error)
{
    qDebug() << "BootloaderInstall::downloadRequestFinished" << id << error;
    qDebug() << "error:" << getter->errorString();

    downloadDone(error);
}

void BootloaderInstaller::downloadDone(bool error)
{
    qDebug() << "Install::downloadDone, error:" << error;
    
     // update progress bar
     
    int max = m_dp->getProgressMax();
    if(max == 0) {
        max = 100;
        m_dp->setProgressMax(max);
    }
    m_dp->setProgressValue(max);
    if(getter->httpResponse() != 200) {
        m_dp->addItem(tr("Download error: received HTTP error %1.").arg(getter->httpResponse()),LOGERROR);
        m_dp->abort();
        emit done(true);
        return;
    }
    if(error) {
        m_dp->addItem(tr("Download error: %1").arg(getter->errorString()),LOGERROR);
        m_dp->abort();
        emit done(true);
        return;
    }
    else m_dp->addItem(tr("Download finished."),LOGOK);
    
    emit finish();
   
}

void BootloaderInstaller::updateDataReadProgress(int read, int total)
{
    m_dp->setProgressMax(total);
    m_dp->setProgressValue(read);
    qDebug() << "progress:" << read << "/" << total;
}


void BootloaderInstaller::installEnded(bool error)
{
    (void) error;
    m_dp->abort();
}


/**************************************************
*** gigabeat secific code 
***************************************************/

void BootloaderInstaller::gigabeatPrepare()
{
    if(m_install)         // Installation
    {
        QString url = m_bootloaderUrlBase + "/gigabeat/" + m_bootloadername;
      
        m_dp->addItem(tr("Downloading file %1.%2")
            .arg(QFileInfo(url).baseName(), QFileInfo(url).completeSuffix()),LOGINFO);

        // temporary file needs to be opened to get the filename
        downloadFile.open();
        m_tempfilename = downloadFile.fileName();
        downloadFile.close();
        // get the real file.
        getter = new HttpGet(this);
        getter->setProxy(m_proxy);
        getter->setFile(&downloadFile);
        getter->getFile(QUrl(url));
        // connect signals from HttpGet
        connect(getter, SIGNAL(done(bool)), this, SLOT(downloadDone(bool)));
        connect(getter, SIGNAL(dataReadProgress(int, int)), this, SLOT(updateDataReadProgress(int, int)));
        connect(m_dp, SIGNAL(aborted()), getter, SLOT(abort())); 
   }
   else                 //UnInstallation
   {
        QString firmware = m_mountpoint + "/GBSYSTEM/FWIMG/FWIMG01.DAT";
        QString firmwareOrig = firmware.append(".ORIG");
        
        QFileInfo firmwareOrigFI(firmwareOrig);
        
        // check if original firmware exists
        if(!firmwareOrigFI.exists())
        {
             m_dp->addItem(tr("Could not find the Original Firmware at: %1")
                    .arg(firmwareOrig),LOGERROR);
            emit done(true);
            return;        
        }
        
        QFile firmwareFile(firmware);
        QFile firmwareOrigFile(firmwareOrig);
        
        //remove modified firmware
        if(!firmwareFile.remove())
        {
             m_dp->addItem(tr("Could not remove the Firmware at: %1")
                    .arg(firmware),LOGERROR);
            emit done(true);
            return;        
        }
        
        //copy  original firmware
        if(!firmwareOrigFile.copy(firmware))   
        {
             m_dp->addItem(tr("Could not copy the Firmware from: %1 to %2")
                    .arg(firmwareOrig,firmware),LOGERROR);
            emit done(true);
            return;        
        }
        
        emit done(false);  //success
   }
   
}

void BootloaderInstaller::gigabeatFinish()  
{
    // this step is only need for installation, so no code for uninstall here

    m_dp->addItem(tr("Finishing bootloader install"),LOGINFO);

    QString firmware = m_mountpoint + "/GBSYSTEM/FWIMG/" + m_bootloadername;
    
    QFileInfo firmwareFI(firmware);
    
    // check if firmware exists
    if(!firmwareFI.exists())
    {
        m_dp->addItem(tr("Could not find the Firmware at: %1")
            .arg(firmware),LOGERROR);
        emit done(true);
        return;        
    }
    
    QString firmwareOrig = firmware;
    firmwareOrig.append(".ORIG");
    QFileInfo firmwareOrigFI(firmwareOrig);
    
    // rename the firmware, if there is no original firmware there
    if(!firmwareOrigFI.exists())
    {
        QFile firmwareFile(firmware);
        if(!firmwareFile.rename(firmwareOrig))
        {
            m_dp->addItem(tr("Could not rename: %1 to %2")
                                .arg(firmware,firmwareOrig),LOGERROR);
            emit done(true);
            return;
        }
    }
    else // or remove the normal firmware, if the original is there
    {
        QFile firmwareFile(firmware);
        firmwareFile.remove();
    }
    
    //copy the firmware
    if(!downloadFile.copy(firmware))
    {
        m_dp->addItem(tr("Could not copy: %1 to %2")
                                .arg(m_tempfilename,firmware),LOGERROR);
        emit done(true);
        return;
    }
    
    downloadFile.remove();

    m_dp->addItem(tr("Bootloader install finished successfully."),LOGOK);
    m_dp->addItem(tr("To finish the Bootloader installation, follow the steps below."),LOGINFO);
    m_dp->addItem(tr("1. Eject/Unmount your Device."),LOGINFO);
    m_dp->addItem(tr("2. Unplug USB and any Power adapters."),LOGINFO);
    m_dp->addItem(tr("3. Hold POWER to turn the Device off."),LOGINFO);
    m_dp->addItem(tr("4. Toggle the Battery switch on the Device."),LOGINFO);
    m_dp->addItem(tr("5. Hold POWER to boot the Rockbox bootloader."),LOGINFO);
                
    
    m_dp->abort();
    
    emit done(false);  // success

}

/**************************************************
*** iaudio secific code 
***************************************************/
void BootloaderInstaller::iaudioPrepare()
{

    QString url = m_bootloaderUrlBase + "/iaudio/" + m_bootloadername;
      
    m_dp->addItem(tr("Downloading file %1.%2")
          .arg(QFileInfo(url).baseName(), QFileInfo(url).completeSuffix()),LOGINFO);

    // temporary file needs to be opened to get the filename
    downloadFile.open();
    m_tempfilename = downloadFile.fileName();
    downloadFile.close();
    // get the real file.
    getter = new HttpGet(this);
    getter->setProxy(m_proxy);
    getter->setFile(&downloadFile);
    getter->getFile(QUrl(url));
    // connect signals from HttpGet
    connect(getter, SIGNAL(done(bool)), this, SLOT(downloadDone(bool)));
    connect(getter, SIGNAL(dataReadProgress(int, int)), this, SLOT(updateDataReadProgress(int, int)));   
    connect(m_dp, SIGNAL(aborted()), getter, SLOT(abort())); 
}  

void BootloaderInstaller::iaudioFinish()
{
    QString firmware = m_mountpoint + "/FIRMWARE/" + m_bootloadername;
    
    //copy the firmware
    if(!downloadFile.copy(firmware))
    {
        m_dp->addItem(tr("Could not copy: %1 to %2")
                                .arg(m_tempfilename,firmware),LOGERROR);
        emit done(true);
        return;
    }
    
    downloadFile.remove();

    m_dp->addItem(tr("Bootloader install finished successfully."),LOGOK);
    m_dp->addItem(tr("To finish the Bootloader installation, follow the steps below."),LOGINFO);
    m_dp->addItem(tr("1. Eject/Unmount your Device."),LOGINFO);
    m_dp->addItem(tr("2. Turn you Device OFF."),LOGINFO);
    m_dp->addItem(tr("3. Insert Charger."),LOGINFO);
    
    m_dp->abort();
    
    emit done(false);  // success
   
}  


/**************************************************
*** h10 secific code 
***************************************************/
void BootloaderInstaller::h10Prepare()
{
    if(m_install)         // Installation
    {
        QString url = m_bootloaderUrlBase + "/iriver/" + m_bootloadername;
      
        m_dp->addItem(tr("Downloading file %1.%2")
          .arg(QFileInfo(url).baseName(), QFileInfo(url).completeSuffix()),LOGINFO);

        // temporary file needs to be opened to get the filename
        downloadFile.open();
        m_tempfilename = downloadFile.fileName();
        downloadFile.close();
        // get the real file.
        getter = new HttpGet(this);
        getter->setProxy(m_proxy);
        getter->setFile(&downloadFile);
        getter->getFile(QUrl(url));
        // connect signals from HttpGet
        connect(getter, SIGNAL(done(bool)), this, SLOT(downloadDone(bool)));
        connect(getter, SIGNAL(dataReadProgress(int, int)), this, SLOT(updateDataReadProgress(int, int)));   
        connect(m_dp, SIGNAL(aborted()), getter, SLOT(abort())); 
    }
    else             // Uninstallation
    {
    
        QString firmwarename = m_bootloadername.section('/', -1);

        QString firmware = m_mountpoint + "/SYSTEM/" + firmwarename;
        QString firmwareOrig = m_mountpoint + "/SYSTEM/Original.mi4";
    
        QFileInfo firmwareFI(firmware);
        if(!firmwareFI.exists())  //Firmware dosent exists on player
        {
            firmware = m_mountpoint + "/SYSTEM/H10EMP.mi4";   //attempt other firmwarename
            firmwareFI.setFile(firmware);
            if(!firmwareFI.exists())  //Firmware dosent exists on player
            {
                m_dp->addItem(tr("Firmware doesn not exist: %1")
                                .arg(firmware),LOGERROR);
                emit done(true);
                return;
            }
        }
       
        QFileInfo firmwareOrigFI(firmwareOrig);
        if(!firmwareOrigFI.exists())  //Original Firmware dosent exists on player
        {
            m_dp->addItem(tr("Original Firmware doesn not exist: %1")
                                .arg(firmwareOrig),LOGERROR);
            emit done(true);
            return;
        }
       
        QFile firmwareFile(firmware);
        QFile firmwareOrigFile(firmwareOrig);
        
        //remove modified firmware
        if(!firmwareFile.remove())
        {
             m_dp->addItem(tr("Could not remove the Firmware at: %1")
                    .arg(firmware),LOGERROR);
            emit done(true);
            return;        
        }
        
        //copy  original firmware
        if(!firmwareOrigFile.copy(firmware))   
        {
             m_dp->addItem(tr("Could not copy the Firmware from: %1 to %2")
                    .arg(firmwareOrig,firmware),LOGERROR);
            emit done(true);
            return;        
        }
        
        emit done(false);  //success
    
    }
}

void BootloaderInstaller::h10Finish()
{
    QString firmwarename = m_bootloadername.section('/', -1);

    QString firmware = m_mountpoint + "/SYSTEM/" + firmwarename;
    QString firmwareOrig = m_mountpoint + "/SYSTEM/Original.mi4";
    
    QFileInfo firmwareFI(firmware);
    
    if(!firmwareFI.exists())  //Firmware dosent exists on player
    {
        firmware = m_mountpoint + "/SYSTEM/H10EMP.mi4";   //attempt other firmwarename
        firmwareFI.setFile(firmware);
        if(!firmwareFI.exists())  //Firmware dosent exists on player
        {
            m_dp->addItem(tr("Firmware does not exist: %1")
                                .arg(firmware),LOGERROR);
            emit done(true);
            return;
        }
    }
    
    QFileInfo firmwareOrigFI(firmwareOrig);
    
    if(!firmwareOrigFI.exists())  //there is already a original firmware
    {
        QFile firmwareFile(firmware);
        if(!firmwareFile.rename(firmwareOrig)) //rename Firmware to Original
        {
            m_dp->addItem(tr("Could not rename: %1 to %2")
                                .arg(firmware,firmwareOrig),LOGERROR);
            emit done(true);
            return;
        }
    }
    else
    {
        QFile firmwareFile(firmware);
        firmwareFile.remove();
    }
        //copy the firmware
    if(!downloadFile.copy(firmware))
    {
        m_dp->addItem(tr("Could not copy: %1 to %2")
                                .arg(m_tempfilename,firmware),LOGERROR);
        emit done(true);
        return;
    }
    
    downloadFile.remove();

    m_dp->addItem(tr("Bootloader install finished successfully."),LOGOK);
    m_dp->abort();
    
    emit done(false);  // success

}

/**************************************************
*** ipod secific code 
***************************************************/
int verbose =0;
// reserves memory for ipodpatcher
bool initIpodpatcher()
{
    if (ipod_alloc_buffer(&sectorbuf,BUFFER_SIZE) < 0) return true;
      else return false;
}

void BootloaderInstaller::ipodPrepare()
{
    m_dp->addItem(tr("Searching for ipods"),LOGINFO);
    struct ipod_t ipod;

    int n = ipod_scan(&ipod);
    if (n == 0)
    {
        m_dp->addItem(tr("No Ipods found"),LOGERROR);
        emit done(true);
        return;
    }
    if (n > 1)
    {
        m_dp->addItem(tr("Too many Ipods found"),LOGERROR);
        emit done(true);
    }
    
    if(m_install)         // Installation
    {
    
        QString url = m_bootloaderUrlBase + "/ipod/bootloader-" + m_bootloadername + ".ipod";
      
        m_dp->addItem(tr("Downloading file %1.%2")
          .arg(QFileInfo(url).baseName(), QFileInfo(url).completeSuffix()),LOGINFO);

        // temporary file needs to be opened to get the filename
        downloadFile.open();
        m_tempfilename = downloadFile.fileName();
        downloadFile.close();
        // get the real file.
        getter = new HttpGet(this);
        getter->setProxy(m_proxy);
        getter->setFile(&downloadFile);
        getter->getFile(QUrl(url));
        // connect signals from HttpGet
        connect(getter, SIGNAL(done(bool)), this, SLOT(downloadDone(bool)));
        connect(getter, SIGNAL(dataReadProgress(int, int)), this, SLOT(updateDataReadProgress(int, int)));   
        connect(m_dp, SIGNAL(aborted()), getter, SLOT(abort())); 
    }
    else                 // Uninstallation
    {
        if (ipod_open(&ipod, 0) < 0)
        {
            m_dp->addItem(tr("could not open ipod"),LOGERROR);
            emit done(true);
            return;
        }

        if (read_partinfo(&ipod,0) < 0)
        {
            m_dp->addItem(tr("could not read partitiontable"),LOGERROR);
            emit done(true);
            return;
        }

        if (ipod.pinfo[0].start==0)
        {
            m_dp->addItem(tr("No partition 0 on disk"),LOGERROR);
        
            int i;
            double sectors_per_MB = (1024.0*1024.0)/ipod.sector_size;
            m_dp->addItem(tr("[INFO] Part    Start Sector    End Sector   Size (MB)   Type\n"),LOGINFO);
            for ( i = 0; i < 4; i++ ) 
            {
                if (ipod.pinfo[i].start != 0) 
                {
                    m_dp->addItem(tr("[INFO]    %1      %2    %3  %4   %5 (%6)").arg(
                        i).arg(
                        ipod.pinfo[i].start).arg(
                        ipod.pinfo[i].start+ipod.pinfo[i].size-1).arg(
                        ipod.pinfo[i].size/sectors_per_MB).arg(
                        get_parttype(ipod.pinfo[i].type)).arg(
                        ipod.pinfo[i].type),LOGINFO);
                }
            }
            emit done(true);
            return;
        }

        read_directory(&ipod);

        if (ipod.nimages <= 0)
        {
            m_dp->addItem(tr("Failed to read firmware directory"),LOGERROR);
            emit done(true);
            return;
        }
        if (getmodel(&ipod,(ipod.ipod_directory[0].vers>>8)) < 0)
        {
            m_dp->addItem(tr("Unknown version number in firmware (%1)").arg(
                ipod.ipod_directory[0].vers),LOGERROR);
            emit done(true);
            return;
        }

        if (ipod.macpod)
        {
            m_dp->addItem(tr("Warning this is a MacPod, Rockbox doesnt work on this. Convert it to WinPod"),LOGWARNING);
        }
        
        if (ipod_reopen_rw(&ipod) < 0) 
        {
            m_dp->addItem(tr("Could not open Ipod in RW mode"),LOGERROR);
            emit done(true);
            return;
        }
        
        if (ipod.ipod_directory[0].entryOffset==0) {
            m_dp->addItem(tr("No bootloader detected."),LOGERROR);
            emit done(true);
            return;
        }
       
        if (delete_bootloader(&ipod)==0) 
        {
            m_dp->addItem(tr("Successfully removed Bootloader"),LOGOK);
            emit done(false);
            ipod_close(&ipod);
            return;      
        }
        else 
        {
            m_dp->addItem(tr("--delete-bootloader failed."),LOGERROR);
            emit done(true);
            ipod_close(&ipod);
            return;          
        }    
    }   
}

void BootloaderInstaller::ipodFinish()
{
    struct ipod_t ipod;
    ipod_scan(&ipod);
    
    if (ipod_open(&ipod, 0) < 0)
    {
        m_dp->addItem(tr("could not open ipod"),LOGERROR);
        emit done(true);
        return;
    }

    if (read_partinfo(&ipod,0) < 0)
    {
        m_dp->addItem(tr("could not read partitiontable"),LOGERROR);
        emit done(true);
        return;
    }

    if (ipod.pinfo[0].start==0)
    {
        m_dp->addItem(tr("No partition 0 on disk"),LOGERROR);
        
        int i;
        double sectors_per_MB = (1024.0*1024.0)/ipod.sector_size;

        m_dp->addItem(tr("[INFO] Part    Start Sector    End Sector   Size (MB)   Type\n"),LOGINFO);
        
        for ( i = 0; i < 4; i++ ) 
        {
         if (ipod.pinfo[i].start != 0) 
         {
            m_dp->addItem(tr("[INFO]    %1      %2    %3  %4   %5 (%6)").arg(
                    i).arg(
                   ipod.pinfo[i].start).arg(
                   ipod.pinfo[i].start+ipod.pinfo[i].size-1).arg(
                   ipod.pinfo[i].size/sectors_per_MB).arg(
                   get_parttype(ipod.pinfo[i].type)).arg(
                   ipod.pinfo[i].type),LOGWARNING);
         }
        }
        emit done(true);
        return;
    }

    read_directory(&ipod);

    if (ipod.nimages <= 0)
    {
        m_dp->addItem(tr("Failed to read firmware directory"),LOGERROR);
        emit done(true);
        return;
    }
    if (getmodel(&ipod,(ipod.ipod_directory[0].vers>>8)) < 0)
    {
        m_dp->addItem(tr("Unknown version number in firmware (%1)").arg(
                ipod.ipod_directory[0].vers),LOGERROR);
        emit done(true);
        return;
    }

    if (ipod.macpod)
    {
        m_dp->addItem(tr("Warning this is a MacPod, Rockbox doesnt work on this. Convert it to WinPod"),LOGWARNING);
    }

    if (ipod_reopen_rw(&ipod) < 0) 
    {
        m_dp->addItem(tr("Could not open Ipod in RW mode"),LOGERROR);
        emit done(true);
        return;
    }

    if (add_bootloader(&ipod, m_tempfilename.toLatin1().data(), FILETYPE_DOT_IPOD)==0) 
    {
        m_dp->addItem(tr("Successfully added Bootloader"),LOGOK);
        emit done(false);
        ipod_close(&ipod);
        return;      
    }
    else 
    {
        m_dp->addItem(tr("failed to add Bootloader"),LOGERROR);
        ipod_close(&ipod);
        emit done(true);
        return;      
    }       
}

/**************************************************
*** sansa secific code 
***************************************************/
// reserves memory for sansapatcher
bool initSansapatcher()
{
      if (sansa_alloc_buffer(&sectorbuf,BUFFER_SIZE) < 0) return true;
      else return false;
}


void BootloaderInstaller::sansaPrepare()
{
    m_dp->addItem(tr("Searching for sansas"),LOGINFO);
    struct sansa_t sansa;

    int n = sansa_scan(&sansa);
    if (n == 0)
    {
        m_dp->addItem(tr("No Sansa found"),LOGERROR);
        emit done(true);
        return;
    }
    if (n > 1)
    {
        m_dp->addItem(tr("Too many Sansas found"),LOGERROR);
        emit done(true);
    }
    
    if(m_install)         // Installation
    {
        QString url = m_bootloaderUrlBase + "/sandisk-sansa/e200/" + m_bootloadername;
      
        m_dp->addItem(tr("Downloading file %1.%2")
          .arg(QFileInfo(url).baseName(), QFileInfo(url).completeSuffix()),LOGINFO);

        // temporary file needs to be opened to get the filename
        downloadFile.open();
        m_tempfilename = downloadFile.fileName();
        downloadFile.close();
        // get the real file.
        getter = new HttpGet(this);
        getter->setProxy(m_proxy);
        getter->setFile(&downloadFile);
        getter->getFile(QUrl(url));
        // connect signals from HttpGet
        connect(getter, SIGNAL(done(bool)), this, SLOT(downloadDone(bool)));
        connect(getter, SIGNAL(dataReadProgress(int, int)), this, SLOT(updateDataReadProgress(int, int)));   
        connect(m_dp, SIGNAL(aborted()), getter, SLOT(abort())); 
    }
    else                 // Uninstallation
    {
      
        if (sansa_open(&sansa, 0) < 0)
        {
            m_dp->addItem(tr("could not open Sansa"),LOGERROR);
            emit done(true);
            return;
        }
    
        if (sansa_read_partinfo(&sansa,0) < 0)
        {
            m_dp->addItem(tr("could not read partitiontable"),LOGERROR);
            emit done(true);
            return;
        }

        int i = is_e200(&sansa);
        if (i < 0) {
            m_dp->addItem(tr("Disk is not an E200 (%1), aborting.").arg(i),LOGERROR);
            emit done(true);
            return;
        }

        if (sansa.hasoldbootloader)
        {
            m_dp->addItem(tr("********************************************\n"
                                       "OLD ROCKBOX INSTALLATION DETECTED, ABORTING.\n"
                                       "You must reinstall the original Sansa firmware before running\n"
                                       "sansapatcher for the first time.\n"
                                       "See http://www.rockbox.org/twiki/bin/view/Main/SansaE200Install\n"
                                       "*********************************************\n"),LOGERROR);
            emit done(true);
            return;
        }
        

        if (sansa_reopen_rw(&sansa) < 0) 
        {
            m_dp->addItem(tr("Could not open Sansa in RW mode"),LOGERROR);
            emit done(true);
            return;
        }
        
        if (sansa_delete_bootloader(&sansa)==0) 
        {
            m_dp->addItem(tr("Successfully removed Bootloader"),LOGOK);
            emit done(false);
            sansa_close(&sansa);
            return;      
        }
        else 
        {
            m_dp->addItem(tr("--delete-bootloader failed."),LOGERROR);
            emit done(true);
            sansa_close(&sansa);
            return;          
        }        
    }    
}

void BootloaderInstaller::sansaFinish()
{
    struct sansa_t sansa;
    sansa_scan(&sansa);
    
    if (sansa_open(&sansa, 0) < 0)
    {
        m_dp->addItem(tr("could not open Sansa"),LOGERROR);
        emit done(true);
        return;
    }
    
    if (sansa_read_partinfo(&sansa,0) < 0)
    {
        m_dp->addItem(tr("could not read partitiontable"),LOGERROR);
        emit done(true);
        return;
    }


    int i = is_e200(&sansa);
    if (i < 0) {
    
        m_dp->addItem(tr("Disk is not an E200 (%1), aborting.").arg(i),LOGERROR);
        emit done(true);
        return;
    }

    if (sansa.hasoldbootloader)
    {
        m_dp->addItem(tr("********************************************\n"
                                       "OLD ROCKBOX INSTALLATION DETECTED, ABORTING.\n"
                                       "You must reinstall the original Sansa firmware before running\n"
                                       "sansapatcher for the first time.\n"
                                       "See http://www.rockbox.org/twiki/bin/view/Main/SansaE200Install\n"
                                       "*********************************************\n"),LOGERROR);
        emit done(true);
        return;
    }

    if (sansa_reopen_rw(&sansa) < 0) 
    {
        m_dp->addItem(tr("Could not open Sansa in RW mode"),LOGERROR);
        emit done(true);
        return;
    }

    if (sansa_add_bootloader(&sansa, m_tempfilename.toLatin1().data(), FILETYPE_MI4)==0) 
    {
        m_dp->addItem(tr("Successfully added Bootloader"),LOGOK);
        emit done(false);
        sansa_close(&sansa);
        return;      
    }
    else 
    {
        m_dp->addItem(tr("failed to add Bootloader"),LOGERROR);
        sansa_close(&sansa);
        emit done(true);
        return;      
    }       

}

/**************************************************
*** iriver /fwpatcher  secific code 
***************************************************/

void BootloaderInstaller::iriverPrepare()
{
    char md5sum_str[32];
    if (!FileMD5(m_origfirmware, md5sum_str)) {
        m_dp->addItem(tr("Could not MD5Sum original firmware"),LOGERROR);
        emit done(true);
        return;
    }
    
    /* Check firmware against md5sums in h120sums and h100sums */
    series = 0;
    table_entry = intable(md5sum_str, &h120pairs[0],
                          sizeof(h120pairs)/sizeof(struct sumpairs));
    if (table_entry >= 0) {
        series = 120;
    }
    else 
    {
        table_entry = intable(md5sum_str, &h100pairs[0],
                              sizeof(h100pairs)/sizeof(struct sumpairs));
        if (table_entry >= 0) 
        {
            series = 100;
        }
        else 
        {
            table_entry = intable(md5sum_str, &h300pairs[0],
                                  sizeof(h300pairs)/sizeof(struct sumpairs));
            if (table_entry >= 0)
                series = 300;
        }
    }
    if (series == 0)
    {
        m_dp->addItem(tr("Could not detect firmware type"),LOGERROR);
        emit done(true);
        return;
    }
    
    QString url = m_bootloaderUrlBase + "/iriver/" + m_bootloadername;
      
    m_dp->addItem(tr("Downloading file %1.%2")
        .arg(QFileInfo(url).baseName(), QFileInfo(url).completeSuffix()),LOGINFO);

    // temporary file needs to be opened to get the filename
    downloadFile.open();
    m_tempfilename = downloadFile.fileName();
    downloadFile.close();
    // get the real file.
    getter = new HttpGet(this);
    getter->setProxy(m_proxy);
    getter->setFile(&downloadFile);
    getter->getFile(QUrl(url));
    // connect signals from HttpGet
    connect(getter, SIGNAL(done(bool)), this, SLOT(downloadDone(bool)));
    connect(getter, SIGNAL(dataReadProgress(int, int)), this, SLOT(updateDataReadProgress(int, int)));   
    connect(m_dp, SIGNAL(aborted()), getter, SLOT(abort())); 
}

void BootloaderInstaller::iriverFinish()
{
    // Patch firmware
    char md5sum_str[32];
    struct sumpairs *sums = 0;
    int origin = 0;

    /* get pointer to the correct bootloader.bin */
    switch(series) {
        case 100:
            sums = &h100pairs[0];
            origin = 0x1f0000;
            break;
        case 120:
            sums = &h120pairs[0];
            origin = 0x1f0000;
            break;
        case 300:
            sums = &h300pairs[0];
            origin = 0x3f0000;
            break;
    }
    
    // temporary files needs to be opened to get the filename
    QTemporaryFile firmwareBin, newBin, newHex;
    firmwareBin.open();
    newBin.open();
    newHex.open();
    QString firmwareBinName = firmwareBin.fileName();
    QString newBinName = newBin.fileName();
    QString newHexName = newHex.fileName();
    firmwareBin.close();
    newBin.close();
    newHex.close();
    
    // iriver decode
    if (iriver_decode(m_origfirmware, firmwareBinName, FALSE, STRIP_NONE,m_dp) == -1) 
    {
        m_dp->addItem(tr("Error in descramble"),LOGERROR);
        firmwareBin.remove();
        newBin.remove();
        newHex.remove();
        emit done(true);
        return;
    }
    //  mkboot
    if (!mkboot(firmwareBinName, newBinName, m_tempfilename, origin,m_dp)) 
    {
        m_dp->addItem(tr("Error in patching"),LOGERROR);
        firmwareBin.remove();
        newBin.remove();
        newHex.remove();
        emit done(true);
        return;
    }
    // iriver_encode
    if (iriver_encode(newBinName, newHexName, FALSE,m_dp) == -1) 
    {
        m_dp->addItem(tr("Error in scramble"),LOGERROR);
        firmwareBin.remove();
        newBin.remove();
        newHex.remove();
        emit done(true);
        return;
    }
    
    /* now md5sum it */
    if (!FileMD5(newHexName, md5sum_str)) 
    {
        m_dp->addItem(tr("Error in checksumming"),LOGERROR);
        firmwareBin.remove();
        newBin.remove();
        newHex.remove();
        emit done(true);
        return;
    }
    if (strncmp(sums[table_entry].patched, md5sum_str, 32) == 0) {
        /* delete temp files */
        firmwareBin.remove();
        newBin.remove();
    }
    
    // Load patched Firmware to player
    QString dest;
    if(series == 100)
            dest = m_mountpoint + "/ihp_100.hex";
    else if(series == 120)
            dest = m_mountpoint + "/ihp_120.hex";
    else if(series == 300)
            dest = m_mountpoint + "/H300.hex";
    // copy file
    if(!newHex.copy(dest))
    {
        m_dp->addItem(tr("Could not copy: %1 to %2")
                                .arg(newHexName,dest),LOGERROR);
        emit done(true);
        return;
    }
    
    downloadFile.remove();
    newHex.remove();

    m_dp->addItem(tr("Bootloader install finished successfully."),LOGOK);
    m_dp->addItem(tr("To finish the Bootloader installation, follow the steps below."),LOGINFO);
    m_dp->addItem(tr("1. Eject/Unmount your Device."),LOGINFO);
    m_dp->addItem(tr("2. Boot into the original Firmware."),LOGINFO);
    m_dp->addItem(tr("3. Use the Firmware flash option in the Original Firmware."),LOGINFO);
    m_dp->addItem(tr("4. Reboot."),LOGINFO);
    m_dp->abort();
    
    emit done(false);  // success


}
