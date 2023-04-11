#include "mainWindow.h"

#include <QWidget>

#include <TH1.h>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent){

  setWindowTitle("FSU DAQ");
  setGeometry(500, 100, 1000, 500);



}

MainWindow::~MainWindow(){


}
