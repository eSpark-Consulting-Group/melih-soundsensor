#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDateTime>
#include <QDebug>

MainWindow::MainWindow( QWidget* parent )
: QMainWindow( parent )
, ui( new Ui::MainWindow )
, m_devIo( Q_NULLPTR )
, m_sockIo( Q_NULLPTR )
, m_zwTimer( Q_NULLPTR )
, m_zwController( Q_NULLPTR ) {
    ui->setupUi( this );
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::on_btnQuit_clicked() {
    QApplication::quit();
}

void MainWindow::on_btnOpenDevice_clicked() {
    const QString name( ui->eDeviceName->text().trimmed() );

    if( name.isEmpty() ) {
        return;
    }

    if( m_devIo.isNull() ) {
        m_devIo.reset( new DevIO( name, this ) );

        if( m_devIo->openDevice() ) {
            ui->btnOpenDevice->setDisabled( true );
            ui->btnCloseDevice->setEnabled( true );
            ui->btnSendData->setEnabled( true );
            ui->btnReadData->setEnabled( true );

            connect( m_devIo.data(), SIGNAL( readyRead() ), this, SLOT( onReadyReadDevice() ) );
        }
        else {
            m_devIo.reset( Q_NULLPTR );
        }
    }
}

void MainWindow::on_btnCloseDevice_clicked() {
    if( !m_devIo.isNull() ) {
        disconnect( m_devIo.data(), SIGNAL( readyRead() ), this, SLOT( onReadyReadDevice() ) );

        m_devIo.reset( Q_NULLPTR );

        ui->btnOpenDevice->setEnabled( true );
        ui->btnCloseDevice->setDisabled( true );
        ui->btnSendData->setDisabled( true );
        ui->btnReadData->setDisabled( true );
    }
}

void MainWindow::on_btnSendData_clicked() {
    if( m_devIo.isNull() ) {
        return;
    }

    const QString strDataRaw( ui->eDataToSend->text().trimmed() );

    if( strDataRaw.isEmpty() ) {
        return;
    }

    sendFromStrToDevice( strDataRaw );
}

void MainWindow::on_btnReadData_clicked() {
    if( m_devIo.isNull() ) {
        return;
    }

    QByteArray ba;
    const quint32 dataLenght = m_devIo->readDevice( ba );

    if( !dataLenght ) {
        qDebug() << "[Warn]: zero length read from the device!";
        return;
    }

    QString strDataLog = MainWindow::bArrayToStr( ba, QChar( ',' ) );

    qDebug() << "[Info]: read from the device: " << strDataLog;

    ui->lstDeviceIO->insertItem(
        0, QString( "[%1]-> (%2) %3" ).arg( QTime::currentTime().toString() ).arg( dataLenght ).arg( strDataLog ) );
    ui->lstDeviceIO->scrollToTop();

    if( ui->chkSendAckOnReading->isChecked() ) {
        const QString strDataRaw( ui->eAckToSend->text().trimmed() );

        if( strDataRaw.isEmpty() ) {
            return;
        }

        sendFromStrToDevice( strDataRaw );
    }
}

void MainWindow::onReadyReadDevice() {
    qDebug() << "[Warn]: some data arrived to the device!";

    if( ui->chkReadDeviceOnDataReady->isChecked() ) {
        on_btnReadData_clicked();
    }
}

QByteArray MainWindow::strToBArray( const QString& s, QChar delim ) {
    if( s.isEmpty() ) {
        return QByteArray();
    }

    const QStringList strDataList = s.split( delim, QString::KeepEmptyParts );

    if( !strDataList.size() ) {
        return QByteArray();
    }

    QByteArray ba;
    ba.resize( strDataList.size() );

    for( int i = 0; i < strDataList.size(); ++i ) {
        if( strDataList.at( i ).length() > 2 ) {
            ba.clear();
            break;
        }

        ba[i] = strDataList.at( i ).toInt( nullptr, /*base*/ 16 );
    }

    return ba;
}

QString MainWindow::bArrayToStr( const QByteArray& ba, QChar delim ) {
    QString strDataLog;

    strDataLog.append( ba.toHex( delim.toLatin1() ) );

    return strDataLog;
}

void MainWindow::sendFromStrToDevice( const QString& s ) {
    if( m_devIo.isNull() ) {
        return;
    }

    const QString strDataRaw( s.trimmed() );

    if( strDataRaw.isEmpty() ) {
        return;
    }

    QByteArray ba = MainWindow::strToBArray( strDataRaw, QChar( ',' ) );

    if( !ba.size() ) {
        return;
    }

    const quint32 writtenLenghth = m_devIo->writeDevice( ba );

    if( !writtenLenghth ) {
        qDebug() << "[Error]: zero length written to the device!";
        return;
    }

    if( writtenLenghth != quint32( ba.size() ) ) {
        qDebug() << "[Warn]: sent: " << ba.size() << "written: " << writtenLenghth;
    }

    QString strDataLog = MainWindow::bArrayToStr( ba, QChar( ',' ) );

    ui->lstDeviceIO->insertItem(
        0, QString( "[%1]<- (%2) %3" ).arg( QTime::currentTime().toString() ).arg( writtenLenghth ).arg( strDataLog ) );
    ui->lstDeviceIO->scrollToTop();
}

void MainWindow::sendFromStrToSocket( const QString& s, QTcpSocket* clientSocket ) {
    if( m_sockIo.isNull() || clientSocket == Q_NULLPTR ) {
        qDebug() << "[Error]: socket obj ptr " << m_sockIo.data() << " socket ptr " << clientSocket;
        return;
    }

    if( QAbstractSocket::ConnectedState != clientSocket->state() ) {
        qDebug() << "[Error]: socket is invalid ";
        return;
    }

    const QString strDataRaw( s.trimmed() );

    if( strDataRaw.isEmpty() ) {
        return;
    }

    QByteArray ba = MainWindow::strToBArray( strDataRaw, QChar( ',' ) );

    if( !ba.size() ) {
        return;
    }

    const quint32 writtenLenghth = clientSocket->write( ba );

    if( !writtenLenghth ) {
        qDebug() << "[Error]: zero length written to the socket!";
        return;
    }

    if( writtenLenghth != quint32( ba.size() ) ) {
        qDebug() << "[Warn]: sent: " << ba.size() << "written: " << writtenLenghth;
    }

    QString strDataLog = MainWindow::bArrayToStr( ba, QChar( ',' ) );

    ui->lstSocketIO->insertItem(
        0, QString( "[%1]<- (%2) %3" ).arg( QTime::currentTime().toString() ).arg( writtenLenghth ).arg( strDataLog ) );
    ui->lstSocketIO->scrollToTop();
}

void MainWindow::on_btnOpenSocket_clicked() {
    const QString host( ui->eHostName->text().trimmed().toLatin1().data() );
    const QString port( ui->ePortName->text().trimmed().toLatin1().data() );
    const bool isClient = ui->rbClient->isChecked();

    if( !host.length() && !port.length() ) {
        qDebug() << "[Error]: invalid params!";
        return;
    }

    if( m_sockIo.isNull() ) {
        m_sockIo.reset( new SocketIO( host, port, isClient, this ) );

        if( !m_sockIo->openSocket() ) {
            m_sockIo.reset( Q_NULLPTR );
            return;
        }

        ui->btnOpenSocket->setDisabled( true );
        ui->btnCloseSocket->setEnabled( true );
        ui->btnSendSocket->setEnabled( true );
        ui->btnReadSocket->setEnabled( true );

        connect( m_sockIo.data(), SIGNAL( readyRead() ), this, SLOT( onReadyReadSocket() ) );

        return;
    }
}

void MainWindow::on_btnCloseSocket_clicked() {
    if( !m_sockIo.isNull() ) {
        disconnect( m_sockIo.data(), SIGNAL( readyRead() ), this, SLOT( onReadyReadSocket() ) );

        m_sockIo->closeSocket();

        m_sockIo.reset( Q_NULLPTR );

        ui->btnOpenSocket->setEnabled( true );
        ui->btnCloseSocket->setDisabled( true );
        ui->btnSendSocket->setDisabled( true );
        ui->btnReadSocket->setDisabled( true );
    }
}

void MainWindow::onReadyReadSocket() {
    qDebug() << "[Warn]: some data arrived to the socket!";

    if( ui->chkReadSocketOnDataReady->isChecked() ) {
        on_btnReadSocket_clicked();
    }
}

void MainWindow::on_btnSendSocket_clicked() {
    if( m_sockIo.isNull() ) {
        return;
    }

    const QString strDataRaw( ui->eTcpPayload->text().trimmed() );

    if( strDataRaw.isEmpty() ) {
        return;
    }

    auto clientSocket = m_sockIo->getSocket();
    sendFromStrToSocket( strDataRaw, clientSocket );

    if( ui->chkCloseSocketOnWriting->isChecked() ) {
        qDebug() << "[Info]: the socket (" << clientSocket->socketDescriptor() << ") was closed.";
        m_sockIo->closeSocket( clientSocket );
    }
}

void MainWindow::on_btnReadSocket_clicked() {
    if( m_sockIo.isNull() ) {
        return;
    }

    QByteArray ba;
    QTcpSocket* clientSocket = Q_NULLPTR;
    const quint32 dataLenght = m_sockIo->readSocket( ba, clientSocket );

    if( !dataLenght ) {
        qDebug() << "[Warn]: zero length read from the socket!";
        return;
    }

    QString strDataLog = MainWindow::bArrayToStr( ba, QChar( ',' ) );

    qDebug() << "[Info]: read from the socket: " << strDataLog;

    ui->lstSocketIO->insertItem(
        0, QString( "[%1]-> (%2) %3" ).arg( QTime::currentTime().toString() ).arg( dataLenght ).arg( strDataLog ) );
    ui->lstSocketIO->scrollToTop();

    if( ui->chkSendToSocketOnReading->isChecked() ) {
        const QString strDataRaw( ui->eTcpAckToSend->text().trimmed() );

        if( strDataRaw.isEmpty() ) {
            return;
        }

        sendFromStrToSocket( strDataRaw, clientSocket );
    }

    if( ui->chkCloseSocketOnWriting->isChecked() ) {
        qDebug() << "[Info]: the socket (" << clientSocket->socketDescriptor() << ") was closed.";
        m_sockIo->closeSocket( clientSocket );
    }
}

void MainWindow::on_chkHeartBeat_stateChanged( int arg1 ) {
    switch( arg1 ) {
    case Qt::Unchecked:
        if( !m_zwTimer.isNull() ) {
            m_zwTimer->stop();

            disconnect( m_zwTimer.data(), SIGNAL( timeout() ), this, SLOT( on_HeartBeat_timout() ) );
            m_zwController.reset( Q_NULLPTR );
            m_zwTimer.reset( Q_NULLPTR );
        }
        break;
    case Qt::PartiallyChecked:
        /* DO NOTHING */
        break;
    case Qt::Checked:
        if( m_zwTimer.isNull() ) {
            m_zwController.reset( new DummyZwaveController() );
            m_zwTimer.reset( new QTimer( this ) );

            connect( m_zwTimer.data(), SIGNAL( timeout() ), this, SLOT( on_HeartBeat_timout() ) );
            m_zwTimer->start( ui->sbHeartBeatDuration->value() );
        }
        break;
    }  // switch( arg1 )
}

void MainWindow::on_sbHeartBeatDuration_valueChanged( int arg1 ) {
    if( !m_zwTimer.isNull() ) {
        m_zwTimer->stop();
        m_zwTimer->start( arg1 );
    }
}

void MainWindow::on_HeartBeat_timout() {
    static bool tick = false;

    ui->lbHeartBeatDuration->setText( QString( "%1" ).arg( tick ) );
    tick = !tick;

    if( m_zwController->step() ) {
        /* DO NOTHING */
    }
}
