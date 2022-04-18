#include "DevIO.h"

#include <fcntl.h>
#include <unistd.h>

#include <QDebug>
#include <QSocketNotifier>

DevIO::DevIO( const QString& name, QObject* parent )
: QObject( parent )
, m_name( name )
, m_fd( 0 )
, m_notif( Q_NULLPTR ) {
    /* EMPTY */
}

DevIO::~DevIO() {
    DevIO::closeDevice();
}

bool DevIO::openDevice() {
    if( m_fd > 0 ) {
        qDebug() << "[Error]: the " << m_name.toLocal8Bit() << " file is already opened!";

        return false;
    }

    m_fd = ::open( m_name.toLocal8Bit(), O_NONBLOCK | O_RDWR );

    if( m_fd < 0 ) {
        qDebug() << "[Error]: Cannot open " << m_name.toLocal8Bit() << " file!";

        m_fd = 0;

        return false;
    }

    qDebug() << "[Info]: descriptor: " << m_fd;

    m_notif.reset( new QSocketNotifier( m_fd, QSocketNotifier::Read ) );
    connect( m_notif.data(), SIGNAL( activated( int ) ), this, SLOT( onReadyRead( int ) ) );

    return true;
}

void DevIO::closeDevice() {
    if( m_fd > 0 ) {
        m_notif.reset( Q_NULLPTR );
        disconnect( m_notif.data(), SIGNAL( activated( int ) ), this, SLOT( onReadyRead() ) );

        ::close( m_fd );
    }

    m_fd = 0;
}

quint32 DevIO::readDevice( QByteArray& ba ) {
    const quint32 buffLenghth = 256;
    char buff[buffLenghth] = { 0 };

    const ssize_t readLength = ::read( m_fd, buff, buffLenghth );

    if( readLength >= 0 ) {
        qDebug() << "[Info]: read length: " << readLength;

        ba.resize( readLength );

        for( int i = 0; i < readLength; ++i ) {
            ba[i] = buff[i];
        }

        return readLength;
    }
    else {
        qDebug() << "[Error]: cannot read: " << readLength;
        qDebug() << "[Error]: errno: (" << errno << ") " << ::strerror( errno );
    }

    return 0;
}

quint32 DevIO::writeDevice( const QByteArray& ba ) {
    if( !ba.size() ) {
        return 0;
    }

    return ::write( m_fd, ba.data(), ba.size() );
}

void DevIO::onReadyRead( int fd ) {
    qDebug() << "[Info]: read ready, socket:: " << fd;
    emit readyRead();
}
