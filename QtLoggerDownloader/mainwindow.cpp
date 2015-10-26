#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QtDebug>

MainWindow::MainWindow(QWidget *parent) :
  QMainWindow(parent),
  ui(new Ui::MainWindow)
{
  ui->setupUi(this);
  serial.setPortName("/dev/ttyUSB0");
  serial.setBaudRate(57600);
  serial.open(QIODevice::ReadWrite);
  connect(&serial, SIGNAL(readyRead()), this, SLOT(parseBytes()));
}

MainWindow::~MainWindow()
{
  if(serial.isOpen()) {
    serial.close();
  }
  delete ui;
}

void MainWindow::parseBytes()
{
  QByteArray jolo = serial.readAll();
  qDebug() << jolo.toHex();

}
