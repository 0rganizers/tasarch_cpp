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
#include "emu/Core.h"
#include "emu/Renderer.h"

class TestGLWidget : public QOpenGLWidget {
public:
    TestGLWidget(QWidget *parent);
    ~TestGLWidget();

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;
    
    //TestRenderer* tr = nullptr;
    Core *core = nullptr;
    int m_width = 0, m_height = 0;
    GLuint m_texID = 0, blitFBO = 0;
    bool didResize = false;
    
private:
    bool createFBO();
    void deleteFBO();
};


#endif /* TestGLWidget_h */
