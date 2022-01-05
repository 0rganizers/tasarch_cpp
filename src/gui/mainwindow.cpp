#include "MainWindow.h"

namespace tasarch::gui {
    MainWindow::MainWindow(QWidget *parent) :
        QMainWindow(parent)
    {
        setupUi(this);
    }

    void MainWindow::on_actionExit_triggered()
    {
        this->close();
    }
} // namespace tasarch::gui

