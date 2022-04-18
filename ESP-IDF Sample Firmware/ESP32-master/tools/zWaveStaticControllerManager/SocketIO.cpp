#include "SocketIO.h"

SocketIO::SocketIO( const QString& hostName, const QString& portName, bool isClient, QObject* parent )
: QObject( parent )
, m_hostName( hostName )
, m_portName( portName )
, m_isClient( isClient )
, m_isServer( !isClient )
, m_sock( Q_NULLPTR )
, m_server( Q_NULLPTR )
, m_clientSocks( Q_NULLPTR )
, m_readData( Q_NULLPTR ) {
    /* EMPTY */
}

SocketIO::~SocketIO() {
    SocketIO::closeSocket();
}

bool SocketIO::openSocket() {
    if( !m_sock.isNull() || !m_server.isNull() ) {
        qDebug() << "[Error]: the " << m_hostName.toLocal8Bit() << ":" << m_portName.toLocal8Bit()
                 << " socket is already opened!";

        return false;
    }

    if( m_isClient ) {
        m_sock.reset( new QTcpSocket( this ) );
        m_server.reset( Q_NULLPTR );

        m_sock->connectToHost( m_hostName.trimmed().toLatin1().toStdString().c_str(),
                               m_portName.trimmed().toLatin1().toInt(),
                               QIODevice::ReadWrite,
                               QAbstractSocket::IPv4Protocol );

        connect( m_sock.data(), SIGNAL( readyRead() ), this, SLOT( onReadyRead() ) );

        return true;
    }
    else if( m_isServer ) {
        m_sock.reset( Q_NULLPTR );
        m_server.reset( new QTcpServer( this ) );

        QHostAddress ha( m_hostName.trimmed().toLatin1().toStdString().c_str() );

        if( !m_server->listen( ha, m_portName.trimmed().toInt() ) ) {
            qDebug() << "[Error]: cannot open for listening " << m_hostName.toLocal8Bit() << ":"
                     << m_portName.toLocal8Bit() << " socket!";

            m_server.reset( Q_NULLPTR );
            return false;
        }

        connect( m_server.data(), SIGNAL( newConnection() ), this, SLOT( onNewConnection() ) );

        return true;
    }

    return false;
}

void SocketIO::closeSocket( QTcpSocket* clientConnection ) {
    if( m_sock.isNull() && m_server.isNull() ) {
        // qDebug()<<"[Error]: the " << m_hostName.toLocal8Bit() << ":" <<
        // m_portName.toLocal8Bit() << " socket is already closed!";

        return;
    }

    if( m_isClient ) {
        m_sock->disconnectFromHost();

        disconnect( m_sock.data(), SIGNAL( readyRead() ), this, SLOT( onReadyRead() ) );
    }
    else if( m_isServer ) {
        if( clientConnection ) {
            if( !m_clientSocks.isNull() ) {
                auto i = m_clientSocks->find( clientConnection );

                if( i != m_clientSocks->end() ) {
                    m_clientSocks->erase( i );
                }
            }

            clientConnection->close();

            return;
        }
        else {
            disconnect( m_server.data(), SIGNAL( newConnection() ), this, SLOT( onNewConnection() ) );
        }
    }

    m_sock.reset( Q_NULLPTR );
    m_server.reset( Q_NULLPTR );
    m_readData.reset( Q_NULLPTR );
    m_clientSocks.reset( Q_NULLPTR );
}

quint32 SocketIO::readSocket( QByteArray& ba, QTcpSocket*& socket ) {
    if( !m_readData->size() ) {
        return 0;
    }

    const auto& chunk = m_readData->front();

    ba.resize( chunk.second.size() );

    for( int i = 0; i < ba.size(); ++i ) {
        ba[i] = chunk.second.operator[]( i );
    }

    if( m_isClient ) {
        socket = m_sock.data();
    }
    else if( m_isServer ) {
        socket = chunk.first;
    }

    qDebug() << "[Info]: data:" << ba.size() << " connection:" << ( socket ? socket->socketDescriptor() : -1 );

    m_readData->erase( m_readData->begin() );

    return ba.size();
}

quint32 SocketIO::writeSocket( const QByteArray& ba ) {
    if( m_isClient ) {
        if( m_sock.isNull() ) {
            qDebug() << "[Error]: client is destroyed";
            return 0;
        }

        return writeSocket( ba, m_sock.data() );
    }
    else if( m_isServer ) {
        if( m_server.isNull() ) {
            qDebug() << "[Error]: suspicious write: server is destroyed";
            return 0;
        }

        qDebug() << "[Error]: suspicious write: no socket provided";
        return 0;
    }

    return 0;
}

quint32 SocketIO::writeSocket( const QByteArray& ba, QTcpSocket* socket ) {
    if( socket == Q_NULLPTR || !ba.size() ) {
        qDebug() << "[Error]: suspicious write: not valid args";
        return 0;
    }

    return socket->write( ba );
}

QTcpSocket* SocketIO::getSocket() const {
    if( ( m_isClient && m_sock.isNull() ) || ( m_isServer && m_server.isNull() ) ) {
        return Q_NULLPTR;
    }

    if( m_isClient ) {
        return m_sock.data();
    }
    else if( m_isServer ) {
        if( !m_readData.isNull() ) {
            if( m_readData->size() ) {
                return m_readData->first().first;
            }
        }

        if( !m_clientSocks.isNull() ) {
            if( m_clientSocks->size() ) {
                return *m_clientSocks->begin();
            }
        }
    }

    return Q_NULLPTR;
}

void SocketIO::onNewConnection() {
    if( m_server.isNull() ) {
        qDebug() << "[Error]: suspicious connection: server is destroyed";
        return;
    }

    QTcpSocket* clientConnection = m_server->nextPendingConnection();
    connect( clientConnection, &QAbstractSocket::disconnected, clientConnection, &QObject::deleteLater );

    qDebug() << "[Info]: incoming connection:" << clientConnection->socketDescriptor();

    if( m_clientSocks.isNull() ) {
        m_clientSocks.reset( new QSet<QTcpSocket*>( { clientConnection } ) );
    }
    else {
        m_clientSocks->insert( clientConnection );
    }

    connect( clientConnection, SIGNAL( readyRead() ), this, SLOT( onReadyRead() ) );
    connect( clientConnection, SIGNAL( disconnected() ), this, SLOT( onDisconnected() ) );
}

void SocketIO::onReadyRead() {
    QObject* rawSender = QObject::sender();

    auto* clientConnection = dynamic_cast<QTcpSocket*>( rawSender );

    if( clientConnection == Q_NULLPTR ) {
        qDebug() << "[Error]: suspicious readyRead signal: cannot dynamic_cast to "
                    "QTcpSocket";
        return;
    }

    QByteArray baRaw = clientConnection->readAll();

    if( baRaw.size() ) {
        qDebug() << "[Info]: read " << baRaw.size() << " bytes";

        if( m_readData.isNull() ) {
            m_readData.reset( new QList<QPair<QTcpSocket*, QByteArray>>( { { clientConnection, baRaw } } ) );
        }
        else {
            m_readData->push_back( { clientConnection, baRaw } );
        }

        emit readyRead();
    }
    else {
        qDebug() << "[Info]: nothing to read, closing connection:" << clientConnection->socketDescriptor();

        if( !m_clientSocks.isNull() ) {
            auto i = m_clientSocks->find( clientConnection );
            if( i != m_clientSocks->end() ) {
                m_clientSocks->erase( i );
            }
        }
        clientConnection->close();
    }
}

void SocketIO::onDisconnected() {
    QObject* rawSender = QObject::sender();

    auto* clientConnection = dynamic_cast<QTcpSocket*>( rawSender );

    if( clientConnection == Q_NULLPTR ) {
        qDebug() << "[Error]: suspicious disconnected signal: cannot dynamic_cast "
                    "to QTcpSocket";
        return;
    }

    if( !m_clientSocks.isNull() ) {
        auto i = m_clientSocks->find( clientConnection );

        if( i != m_clientSocks->end() ) {
            qDebug() << "[Info]: sanitized m_clientSocks";
            m_clientSocks->erase( i );
        }
    }

    if( !m_readData.isNull() ) {
        for( auto i = m_readData->begin(); i != m_readData->end(); /* EMPTY */ ) {
            if( i->first == clientConnection ) {
                qDebug() << "[Info]: sanitized m_readData";
                i = m_readData->erase( i );
            }
            else {
                ++i;
            }
        }
    }
}
