/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *   Copyright (C) 2013 by Dominik Riebeling
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <QtNetwork>
#include <QtDebug>

#include <QNetworkAccessManager>
#include <QNetworkRequest>

#include "httpget.h"

QString HttpGet::m_globalUserAgent; //< globally set user agent for requests
QDir HttpGet::m_globalCache; //< global cach path value for new objects
QNetworkProxy HttpGet::m_globalProxy;

HttpGet::HttpGet(QObject *parent)
    : QObject(parent)
{
    m_mgr = new QNetworkAccessManager(this);
    m_cache = NULL;
    m_cachedir = m_globalCache;
    setCache(true);
    connect(m_mgr, SIGNAL(finished(QNetworkReply*)),
            this, SLOT(requestFinished(QNetworkReply*)));
    m_outputFile = NULL;
    m_lastServerTimestamp = QDateTime();
    m_proxy = QNetworkProxy::NoProxy;
    m_reply = NULL;
}


//! @brief set cache path
//  @param d new directory to use as cache path
void HttpGet::setCache(const QDir& d)
{
    if(m_cache && m_cachedir == d.absolutePath())
        return;
    m_cachedir = d.absolutePath();
    setCache(true);
}


/** @brief enable / disable cache useage
 *  @param c set cache usage
 */
void HttpGet::setCache(bool c)
{
    // don't change cache if it's already (un)set.
    if(c && m_cache) return;
    if(!c && !m_cache) return;
    // don't delete the old cache directly, it might still be in use. Just
    // instruct it to delete itself later.
    if(m_cache) m_cache->deleteLater();
    m_cache = NULL;

    QString path = m_cachedir.absolutePath();

    if(!c || m_cachedir.absolutePath().isEmpty()) {
        qDebug() << "[HttpGet] disabling download cache";
    }
    else {
        // append the cache path to make it unique in case the path points to
        // the system temporary path. In that case using it directly might
        // cause problems. Extra path also used in configure dialog.
        path += "/rbutil-cache";
        qDebug() << "[HttpGet] setting cache folder to" << path;
        m_cache = new QNetworkDiskCache(this);
        m_cache->setCacheDirectory(path);
    }
    m_mgr->setCache(m_cache);
}


/** @brief read all downloaded data into a buffer
 *  @return data
 */
QByteArray HttpGet::readAll()
{
    return m_data;
}


void HttpGet::setProxy(const QUrl &proxy)
{
    qDebug() << "[HttpGet] Proxy set to" << proxy;
    m_proxy.setType(QNetworkProxy::HttpProxy);
    m_proxy.setHostName(proxy.host());
    m_proxy.setPort(proxy.port());
    m_proxy.setUser(proxy.userName());
    m_proxy.setPassword(proxy.password());
    m_mgr->setProxy(m_proxy);
}


void HttpGet::setProxy(bool enable)
{
    if(enable) m_mgr->setProxy(m_proxy);
    else m_mgr->setProxy(QNetworkProxy::NoProxy);
}


void HttpGet::setFile(QFile *file)
{
    m_outputFile = file;
}


void HttpGet::abort()
{
    if(m_reply) m_reply->abort();
}


void HttpGet::requestFinished(QNetworkReply* reply)
{
    m_lastStatusCode
        = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    qDebug() << "[HttpGet] Request finished, status code:" << m_lastStatusCode;
    m_lastServerTimestamp
        = reply->header(QNetworkRequest::LastModifiedHeader).toDateTime().toLocalTime();
    qDebug() << "[HttpGet] Data from cache:"
             << reply->attribute(QNetworkRequest::SourceIsFromCacheAttribute).toBool();
    m_lastRequestCached =
        reply->attribute(QNetworkRequest::SourceIsFromCacheAttribute).toBool();
    if(reply->attribute(QNetworkRequest::RedirectionTargetAttribute).isValid()) {
        // handle relative URLs using QUrl::resolved()
        QUrl org = reply->request().url();
        QUrl url = QUrl(org).resolved(
                reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl());
        // reconstruct query
#if QT_VERSION < 0x050000
        QList<QPair<QByteArray, QByteArray> > qitms = org.encodedQueryItems();
        for(int i = 0; i < qitms.size(); ++i)
            url.addEncodedQueryItem(qitms.at(i).first, qitms.at(i).second);
#else
        url.setQuery(org.query());
#endif
        qDebug() << "[HttpGet] Redirected to" << url;
        startRequest(url);
        return;
    }
    else if(m_lastStatusCode == 200) {
        m_data = reply->readAll();
        if(m_outputFile && m_outputFile->open(QIODevice::WriteOnly)) {
            m_outputFile->write(m_data);
            m_outputFile->close();
        }
        emit done(false);
    }
    else {
        m_data.clear();
        emit done(true);
    }
    reply->deleteLater();
    m_reply = NULL;
}


void HttpGet::downloadProgress(qint64 received, qint64 total)
{
    emit dataReadProgress((int)received, (int)total);
}


void HttpGet::startRequest(QUrl url)
{
    qDebug() << "[HttpGet] Request started";
    QNetworkRequest req(url);
    if(!m_globalUserAgent.isEmpty())
        req.setRawHeader("User-Agent", m_globalUserAgent.toLatin1());

    m_reply = m_mgr->get(req);
    connect(m_reply, SIGNAL(error(QNetworkReply::NetworkError)),
            this, SLOT(networkError(QNetworkReply::NetworkError)));
    connect(m_reply, SIGNAL(downloadProgress(qint64, qint64)),
            this, SLOT(downloadProgress(qint64, qint64)));
}


void HttpGet::networkError(QNetworkReply::NetworkError error)
{
    qDebug() << "[HttpGet] NetworkError occured:"
             << error << m_reply->errorString();
    m_lastErrorString = m_reply->errorString();
}


bool HttpGet::getFile(const QUrl &url)
{
    qDebug() << "[HttpGet] Get URI" << url.toString();
    m_data.clear();
    startRequest(url);

    return false;
}


QString HttpGet::errorString(void)
{
    return m_lastErrorString;
}


int HttpGet::httpResponse(void)
{
    return m_lastStatusCode;
}

