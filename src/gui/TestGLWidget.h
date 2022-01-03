//
//  TestGLWidget.h
//  tasarch
//
//  Created by Leonardo Galli on 02.01.22.
//

#ifndef TestGLWidget_h
#define TestGLWidget_h

#include <QOpenGLWidget>
#include "test/TestRenderer.h"

class TestGLWidget : public QOpenGLWidget {
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


#endif /* TestGLWidget_h */
