#include "musicdownloadstatuslabel.h"
#include "musicapplication.h"
#include "musictextdownloadthread.h"
#include "musicdatadownloadthread.h"
#include "musicdata2downloadthread.h"
#include "musicbgthemedownload.h"
#include "musicnetworkthread.h"
#include "musicsettingmanager.h"
#include "musicconnectionpool.h"

#include <QLabel>

MusicDownloadStatusLabel::MusicDownloadStatusLabel(QWidget *w)
    : QObject(w)
{
    m_parentWidget = static_cast<MusicApplication*>(w);
    m_downloadLrcThread = NULL;

    M_CONNECTION->setValue("MusicDownloadStatusLabel", this);
    M_CONNECTION->connect("MusicSongSearchOnlineTableWidget", "MusicDownloadStatusLabel");
    M_CONNECTION->connect("MusicNetworkThread", "MusicDownloadStatusLabel");
}

MusicDownloadStatusLabel::~MusicDownloadStatusLabel()
{
    delete m_downloadLrcThread;
}

void MusicDownloadStatusLabel::showDownLoadInfoFor(MusicObject::DownLoadType type)
{
    QString stringType;
    switch(type)
    {
        case MusicObject::DisConnection:
            stringType = ":/download/disconnection";
            break;
        case MusicObject::DownLoading:
            stringType = ":/download/downloading";
            break;
        case MusicObject::Buffing:
            stringType = ":/download/buffing";
            break;
        case MusicObject::Waiting:
            break;
        default:
            break;
    }
    ///Start up the information for show download - message;
    m_movieLabel->setPixmap( QPixmap(stringType) );
}

void MusicDownloadStatusLabel::showDownLoadInfoFinished(const QString &type)
{
    ///If the lyrics download finished immediately loaded to display
    if(type == "Lrc")
    {
        m_parentWidget->musicLoadCurrentSongLrc();
    }
    m_movieLabel->clear();
}

void MusicDownloadStatusLabel::networkConnectionStateChanged(bool state)
{
    state ? showDownLoadInfoFinished( QString() )
          : showDownLoadInfoFor(MusicObject::DisConnection);
}

bool MusicDownloadStatusLabel::checkSettingParameterValue() const
{
    return ( M_SETTING->value(MusicSettingManager::ShowInlineLrcChoiced).toBool() ||
             M_SETTING->value(MusicSettingManager::ShowDesktopLrcChoiced).toBool() )
             ? true : false;
}

void MusicDownloadStatusLabel::musicCheckHasLrcAlready()
{
    if( checkSettingParameterValue() )
    {///Check there is no opening lyrics display mode
       if( m_parentWidget->checkMusicListCurrentIndex() )
       {
           return;
       }
       QString filename = m_parentWidget->getCurrentFileName();
       ///Check if the file exists
       QFile file(MusicObject::getAppDir() + LRC_DOWNLOAD + filename + LRC_FILE);
       if(file.exists())
       {
           return;
       }

       if(m_downloadLrcThread)
       {
           delete m_downloadLrcThread;
           m_downloadLrcThread = NULL;
       }
       ///Start the request query
       m_downloadLrcThread = new MusicDownLoadQueryThread(this);
       m_downloadLrcThread->startSearchSong(MusicQuery, filename);
       connect(m_downloadLrcThread, SIGNAL(resolvedSuccess()), SLOT(musicHaveNoLrcAlready()));
       showDownLoadInfoFor(MusicObject::Buffing);
    }
}

void MusicDownloadStatusLabel::musicHaveNoLrcAlready()
{
    if(!M_NETWORK->isOnline())
    {   //no network connection
        showDownLoadInfoFor(MusicObject::DisConnection);
        return;
    }
    DownloadSongInfos musicSongInfo(m_downloadLrcThread->getMusicSongInfo());
    if(!musicSongInfo.isEmpty())
    {
        QString filename = m_parentWidget->getCurrentFileName();
        ///download lrc
        MusicTextDownLoadThread* lrc = new MusicTextDownLoadThread(musicSongInfo[0].m_lrcUrl,
                    MusicObject::getAppDir() + LRC_DOWNLOAD + filename + LRC_FILE, Download_Lrc, this);
        lrc->startToDownload();

        int count = filename.split('-').count();
        filename = filename.split('-').front().trimmed();
        ///download art picture
        (new MusicData2DownloadThread(musicSongInfo[0].m_smallPicUrl, MusicObject::getAppDir() +
             ART_DOWNLOAD + filename + SKN_FILE, Download_SmlBG, this))->startToDownload();
        ///download big picture
        new MusicBgThemeDownload( count == 1 ? musicSongInfo[0].m_singerName : filename, filename, this);
    }
    else
    {
        showDownLoadInfoFinished("find error!");
    }
}