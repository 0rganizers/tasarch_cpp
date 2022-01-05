#include "LogWindow.h"
#include "log/logging.h"

namespace tasarch::gui {
    LogWindow::LogWindow(QWidget *parent) :
        QWidget(parent)
    {
        setupUi(this);
        
        // Very hacky workaround to allow opening links externally!
        auto &clist = viewer->children();
        for (QObject *pObj: clist)
        {
            QString cname = pObj->metaObject()->className();
            if (cname == "QPlainTextEditControl")
                pObj->setProperty("openExternalLinks", true);
        }
    }

    auto LogWindow::append(const QString msg) -> void
    {
        this->viewer->appendHtml(msg);
    }

    auto LogWindow::testAppend() -> void
    {
        this->viewer->appendHtml("Normal <em>Bold</em> <i>Italics</i> <font color=\"#666666\">Dim</font> <span style=\"color: green;\">span</span> <font color=\"Aqua\">Colored</font> <a href=\"vscode://file/Users/leonardogalli/Code/CTF/organizers/tasarch_cpp/src/test/TestRenderer.cpp:471\">vscode link</a> <a href=\"xcode://file/Users/leonardogalli/Code/CTF/organizers/tasarch_cpp/src/test/TestRenderer.cpp:471\">xcode link</a>");
    }
} // namespace tasarch::gui

