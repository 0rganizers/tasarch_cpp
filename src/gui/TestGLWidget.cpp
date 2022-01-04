//
//  TestGLWidget.cpp
//  tasarch
//
//  Created by Leonardo Galli on 02.01.22.
//

#include "TestGLWidget.h"
#include <QOpenGLContext>
#include <QOpenGLExtraFunctions>
#include <QOffscreenSurface>
#include <easyloggingpp/easylogging++.h>

TestGLWidget::TestGLWidget(QWidget* parent) : QOpenGLWidget(parent)
{
//    QSurfaceFormat form;
//    form.setMajorVersion(4);
//    form.setMinorVersion(1);
//    form.setRenderableType(QSurfaceFormat::OpenGL);
//    form.setProfile(QSurfaceFormat::CoreProfile);
//    form.setDepthBufferSize(32);
//    this->setFormat(form);
}

TestGLWidget::~TestGLWidget() {
    delete core;
}

auto TestGLWidget::createFBO() -> bool
{
    QOpenGLExtraFunctions *funcs = QOpenGLContext::currentContext()->extraFunctions();
    // Now we also need to setup the fbo's here, otherwise, we cannot access the texture!
    funcs->glGenFramebuffers(1, &this->blitFBO);
    funcs->glBindFramebuffer(GL_FRAMEBUFFER, this->blitFBO);
    funcs->glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_texID, 0);
    GLenum error = funcs->glGetError();
    if (error != GL_NO_ERROR) {
        LOG(ERROR) << "failed to setup framebuffer " << error << ", texID: " << m_texID;
        return false;
    }
    
    GLenum status = funcs->glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if(status != GL_FRAMEBUFFER_COMPLETE) {
        LOG(ERROR) << "framebuffer status is incomplete!: " << status;
        return false;
    }
    
    LOG(INFO) << "blit fbo: " << this->blitFBO;
    
    return true;
}

auto TestGLWidget::deleteFBO() -> void
{
    QOpenGLFunctions *funcs = QOpenGLContext::currentContext()->functions();
    funcs->glDeleteFramebuffers(1, &this->blitFBO);
}

auto TestGLWidget::initializeGL() -> void
{
    LOG(INFO) << "initializing context!";
    if (this->core != nullptr) {
        //this->tr->stop();
        delete this->core;
    }
    QOffscreenSurface* surface = new QOffscreenSurface();
    surface->setFormat(this->format());
    surface->create();
    
    QOpenGLContext* subctx = new QOpenGLContext();
    subctx->setShareContext(QOpenGLContext::currentContext());
    subctx->setFormat(this->format());
    if (!subctx->create()) {
        LOG(ERROR) << "failed to create subctx!";
        return;
    }
    
    //this->tr = new TestRenderer(subctx, surface);
    //this->tr->setup();

//#define TEST_DOLPHIN
#ifdef TEST_DOLPHIN 
    this->core = new Core("/home/aaron/Projects/TASarch/TASarch/dolphin_libretro.so");
    this->core->load_game("/home/aaron/Projects/isos/ww.iso");
#else
    this->core = new Core("/home/aaron/Projects/TASarch/TASarch/desmume_libretro.so");
    this->core->load_game("/home/aaron/Projects/TASarch/TASarch/plat.nds");
#endif

    this->m_texID = this->core->init_renderer(subctx, surface);

    // Need to make current again after setup!
    this->makeCurrent();
    
    this->createFBO();
}

auto TestGLWidget::resizeGL(int w, int h) -> void
{
    LOG(INFO) << "resize " << w << ", " << h;
    //this->tr->resize(w, h);
    //this->core
    m_width = w;
    m_height = h;
    this->didResize = true;
}

auto TestGLWidget::paintGL() -> void
{
//    LOG(INFO) << "paintGL";
    //this->tr->run();
    core->run();
    // This only happens, if we did resize, but the renderer already created the new buffers!
    // We never need to resize our own framebuffer, it will always be the max size of the game!
    // The only thing we need to change is where we render to...
    /*if (this->didResize) {
        // so recreate our buffers as well!
        this->deleteFBO();
        this->createFBO();
        this->didResize = false;
    }*/

    auto f = QOpenGLContext::currentContext()->extraFunctions();
    f->glFlush();
    f->glFinish();
    f->glDisable(GL_DEPTH_TEST);
    f->glDisable(GL_BLEND);
    {
        std::scoped_lock lock(this->core->m_renderer->m_blit_output);
     
        f->glBindFramebuffer(GL_DRAW_FRAMEBUFFER, this->defaultFramebufferObject());
        f->glBindFramebuffer(GL_READ_FRAMEBUFFER, this->blitFBO);
        
        avinfo info = this->core->get_info();
        f->glBlitFramebuffer(0, 0, info.geometry.max_width, info.geometry.max_height, 
                0, 0, m_width, m_height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    }

    this->update();
}
