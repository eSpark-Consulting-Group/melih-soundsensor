#ifndef DEVIO_H
#define DEVIO_H

#include <QScopedPointer>
#include <QSocketNotifier>
#include <QString>

class DevIO : public QObject {
    Q_OBJECT

private:
    const QString m_name;
    int m_fd;
    QScopedPointer<QSocketNotifier> m_notif;

public:
    explicit DevIO( const QString& name, QObject* parent = nullptr );
    virtual ~DevIO();

    bool openDevice();
    void closeDevice();

    quint32 readDevice( QByteArray& ba );
    quint32 writeDevice( const QByteArray& ba );

public slots:
    void onReadyRead( int fd );

signals:
    void readyRead();
};

#endif  // DEVIO_H
