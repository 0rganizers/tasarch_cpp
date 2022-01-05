#include "MainWindow.h"

namespace tasarch::gui {
    ManagedWindow::ManagedWindow(QWidget* parent, QWidget* window, QString title, size_t idx) :
        parent(parent), window(window), title(title), idx(idx)
    {
        // TODO: If user closes the window manually, then the state desyncs.
        // FIX: Create base window class that has an on close signal we can listen on here.
        this->action = new QAction(this->title, this->parent);
        this->action->setCheckable(true);
        QKeySequence seq = QKeySequence::fromString(QString("Ctrl+%1").arg(this->idx));
        this->action->setShortcut(seq);
        QObject::connect(this->action, &QAction::triggered, this, &ManagedWindow::slot_checked);
        this->update_check();
        
    }

    void ManagedWindow::slot_checked()
    {
        if (this->action->isChecked())
        {
            this->window->show();
        } else {
            this->window->hide();
        }
    }

    void ManagedWindow::update_check()
    {
        this->action->setChecked(this->window->isVisible());
    }

    MainWindow::MainWindow(QWidget *parent) :
        QMainWindow(parent), log::WithLogger("mainw")
    {
        setupUi(this);
    }

    void MainWindow::on_actionExit_triggered()
    {
        this->close();
    }

    void MainWindow::add_window(QWidget *window, QString title)
    {
        if (title == "") {
            title = window->windowTitle();
        }
        size_t idx = this->windows.size() + 1;
        this->logger->info("Adding window {} with index {}", title.toStdString(), idx);
        
        auto mgd = new ManagedWindow(this, window, title, idx);
        this->windows.push_back(mgd);
        this->menubar->addMenu("Windows")->addAction(mgd->action);
    }
} // namespace tasarch::gui

