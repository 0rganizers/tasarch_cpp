#include "mainwindow.h"

#include <easyloggingpp/easylogging++.h>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent)
{
    setupUi(this);
}

void MainWindow::on_actionExit_triggered()
{
    this->close();
}
