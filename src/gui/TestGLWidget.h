//
//  TestGLWidget.h
//  tasarch
//
//  Created by Leonardo Galli on 02.01.22.
//

#ifndef TestGLWidget_h
#define TestGLWidget_h

#include <QOpenGLWidget>
#include <qopenglcontext.h>
#include <qopenglfunctions.h>
#include <QOpenGLExtraFunctions>
#include "test/TestRenderer.h"
#include "log/logging.h"

namespace tasarch::gui {
    class TestGLWidget : public QOpenGLWidget, public QOpenGLExtraFunctions, public log::WithLogger {
    public:
        TestGLWidget(QWidget *parent);
    protected:
        void initializeGL() override;
        void resizeGL(int w, int h) override;
        void paintGL() override;
        
        TestRenderer* tr = nullptr;
        GLuint blitFBO = 0;
        bool didResize = false;
        
    private:
        bool createFBO();
        void deleteFBO();
    };
} // namespace tasarch::gui


#endif /* TestGLWidget_h */
