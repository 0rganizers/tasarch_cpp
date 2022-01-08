#ifndef __MAINWINDOW_H
#define __MAINWINDOW_H

#include <QMainWindow>
#include <vector>
#include "log/logging.h"
#include "ui_mainwindow.h"

namespace tasarch::gui {
    class ManagedWindow : public QObject
    {
        Q_OBJECT
    public:
        ManagedWindow(QWidget* parent, QWidget* window, QString title, size_t idx);
        
        QWidget* parent;
        QWidget* window;
        QString title;
        size_t idx;
        QAction* action;
        
    private slots:
        void slot_checked();
        
    private:
        void update_check();
    };

    class MainWindow : public QMainWindow, log::WithLogger, private Ui::MainWindow
    {
        Q_OBJECT

    public:
        explicit MainWindow(QWidget *parent = nullptr);
        
        void add_window(QWidget* window, QString title = "");

    private slots:
        void on_actionExit_triggered();
        void on_actionReloadConf();
        
    private:
        std::vector<ManagedWindow*> windows;
    };
} // namespace tasarch::gui



#endif /* __MAINWINDOW_H */
