//
//  TestGLWidget.h
//  tasarch
//
//  Created by Leonardo Galli on 02.01.22.
//

#ifndef TestGLWidget_h
#define TestGLWidget_h

#include <QOpenGLWidget>
#include "emu/Core.h"
#include "emu/Renderer.h"
#include "log/logging.h"

namespace tasarch::gui {
    class TestGLWidget : public QOpenGLWidget, public log::WithLogger, public QOpenGLExtraFunctions {
    public:
        TestGLWidget(QWidget *parent);
    protected:
        void initializeGL() override;
        void resizeGL(int w, int h) override;
        void paintGL() override;
        
        Core *core = nullptr;
        GLuint blitFBO = 0;
        
    private:
        bool createFBO();
        void deleteFBO();
    };
} // namespace tasarch::gui

#endif
