#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QScopedPointer>
#include <QTimer>
#include <QMainWindow>

#include "DevIO.h"
#include "SocketIO.h"
#include "DummyZwaveController.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow( QWidget* parent = 0 );
    ~MainWindow();

private slots:
    void on_btnQuit_clicked();

    void on_btnOpenDevice_clicked();

    void on_btnCloseDevice_clicked();

    void on_btnSendData_clicked();

    void onReadyReadDevice();

    void on_btnReadData_clicked();

    void on_btnOpenSocket_clicked();

    void on_btnCloseSocket_clicked();

    void onReadyReadSocket();

    void on_btnSendSocket_clicked();

    void on_btnReadSocket_clicked();

    void on_chkHeartBeat_stateChanged( int arg1 );

    void on_sbHeartBeatDuration_valueChanged( int arg1 );

    void on_HeartBeat_timout();

private:
    Ui::MainWindow* ui;

    QScopedPointer<DevIO> m_devIo;
    QScopedPointer<SocketIO> m_sockIo;
    QScopedPointer<QTimer> m_zwTimer;
    QScopedPointer<DummyZwaveController> m_zwController;

private:
    static QByteArray strToBArray( const QString& s, QChar delim );
    static QString bArrayToStr( const QByteArray& ba, QChar delim );

    void sendFromStrToDevice( const QString& s );
    void sendFromStrToSocket( const QString& s, QTcpSocket* clientSocket );
};

#endif  // MAINWINDOW_H
