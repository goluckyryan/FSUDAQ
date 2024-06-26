#include <QApplication>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>
#include <QFile>

#include "FSUDAQ.h"

#include <QObject>
#include <QDebug>

#include <sys/resource.h>

// class CustomApplication : public QApplication{
// public:
//     CustomApplication(int &argc, char **argv) : QApplication(argc, argv) {}

// protected:
//     bool notify(QObject *receiver, QEvent *event) override{
//         qDebug() << event->type() << "Receiver:" << receiver;
//         return QApplication::notify(receiver, event);
//     }
// };

int main(int argc, char *argv[]){

    // CustomApplication a(argc, argv);
    QApplication a(argc, argv);

    setpriority(PRIO_PROCESS, 0, -20);

    bool isLock = false;
    int pid = 0;
    QFile lockFile(DAQLockFile);
    if( lockFile.open(QIODevice::Text | QIODevice::ReadOnly) ){
        QTextStream in(&lockFile);
        QString line = in.readLine();
        isLock = line.toInt();
        lockFile.close();
    }
    QFile pidFile(PIDFile);
    if( pidFile.open(QIODevice::Text | QIODevice::ReadOnly)){
        QTextStream in(&pidFile);
        QString line = in.readLine();
        pid = line.toInt();
        pidFile.close();
    }
    if( isLock ) {
        qDebug() << "The DAQ program is already opened. PID is " + QString::number(pid) + ", and delete the " + DAQLockFile ;
        QMessageBox msgBox;
        msgBox.setWindowTitle("Oopss....");
        msgBox.setText("The DAQ program is already opened, or crashed perviously. \nPID is " + QString::number(pid) + "\n You can kill the procee by \"kill -9 <pid>\" and delete the " + DAQLockFile + "\n or click the \"Kill\" button");
        msgBox.setIcon(QMessageBox::Information);
    
        QPushButton * kill = msgBox.addButton("Kill and Open New", QMessageBox::AcceptRole);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setDefaultButton(QMessageBox::Ok);
        msgBox.exec();
        if(msgBox.clickedButton() == kill){
            remove(DAQLockFile);
            QProcess::execute("kill", QStringList() << "-9" << QString::number(pid));
        }else{
            return 0;
        }
    }
    lockFile.open(QIODevice::Text | QIODevice::WriteOnly);
    lockFile.write( "1" );
    lockFile.close();
    pidFile.open(QIODevice::Text | QIODevice::WriteOnly);
    pidFile.write(  QString::number(QCoreApplication::applicationPid() ).toStdString().c_str() );
    pidFile.close();

    FSUDAQ w;
    w.show();
    return a.exec();
}