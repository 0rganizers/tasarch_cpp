#ifndef __MAINWINDOW_H
#define __MAINWINDOW_H

#include <QMainWindow>
#include "ui_mainwindow.h"

namespace tasarch::gui {
    class MainWindow : public QMainWindow, private Ui::MainWindow
    {
        Q_OBJECT

    public:
        explicit MainWindow(QWidget *parent = nullptr);

    private slots:
        void on_actionExit_triggered();
    };
} // namespace tasarch::gui



#endif /* __MAINWINDOW_H */
