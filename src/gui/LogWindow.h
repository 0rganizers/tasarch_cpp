#ifndef __LOGWINDOW_H
#define __LOGWINDOW_H

#include <QWidget>
#include "ui_logwindow.h"
#include <string>

namespace tasarch::gui {
    class LogWindow : public QWidget, private Ui::LogWindow
    {
        Q_OBJECT

    public:
        explicit LogWindow(QWidget *parent = nullptr);
        Q_INVOKABLE void append(const QString msg);
        
        void testAppend();
    };
} // namespace tasarch::gui



#endif /* __LOGWINDOW_H */
