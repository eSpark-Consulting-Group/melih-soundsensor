#ifndef SOCKETIO_H
#define SOCKETIO_H

#include <QByteArray>
#include <QList>
#include <QObject>
#include <QPair>
#include <QScopedPointer>
#include <QSet>
#include <QSocketNotifier>
#include <QTcpServer>
#include <QTcpSocket>

class SocketIO : public QObject {
    Q_OBJECT

private:
    const QString m_hostName;
    const QString m_portName;
    const bool m_isClient;
    const bool m_isServer;

    QScopedPointer<QTcpSocket> m_sock;
    QScopedPointer<QTcpServer> m_server;
    QScopedPointer<QSet<QTcpSocket*>> m_clientSocks;
    QScopedPointer<QList<QPair<QTcpSocket*, QByteArray>>> m_readData;

public:
    explicit SocketIO( const QString& hostName, const QString& portName, bool isClient, QObject* parent = nullptr );
    virtual ~SocketIO();

    bool openSocket();
    void closeSocket( QTcpSocket* clientConnection = Q_NULLPTR );

    quint32 readSocket( QByteArray& ba, QTcpSocket*& socket );
    quint32 writeSocket( const QByteArray& ba );
    quint32 writeSocket( const QByteArray& ba, QTcpSocket* socket );

    QTcpSocket* getSocket() const;

signals:
    void readyRead();

public slots:

private slots:
    void onNewConnection();
    void onReadyRead();
    void onDisconnected();
};

#endif  // SOCKETIO_H
