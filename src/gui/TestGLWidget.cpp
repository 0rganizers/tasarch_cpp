//
//  TestGLWidget.cpp
//  tasarch
//
//  Created by Leonardo Galli on 02.01.22.
//

#include "TestGLWidget.h"
#include <QOpenGLContext>
#include <QOpenGLFunctions>
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

auto TestGLWidget::createFBO() -> bool
{
    // Now we also need to setup the fbo's here, otherwise, we cannot access the texture!
    glGenFramebuffers(1, &this->blitFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, this->blitFBO);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, this->tr->finalOutputBuffer, 0);
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        LOG(ERROR) << "failed to setup framebuffer";
        return false;
    }
    
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if(status != GL_FRAMEBUFFER_COMPLETE) {
        LOG(ERROR) << "framebuffer status is incomplete!: " << status;
        return false;
    }
    
    LOG(INFO) << "blit fbo: " << this->blitFBO;
    
    return true;
}

auto TestGLWidget::deleteFBO() -> void
{
    glDeleteFramebuffers(1, &this->blitFBO);
}

auto TestGLWidget::initializeGL() -> void
{
    LOG(INFO) << "initializing context!";
    if (this->tr != nullptr) {
        this->tr->stop();
        delete this->tr;
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
    
    this->tr = new TestRenderer(subctx, surface);
    this->tr->setup();
    // Need to make current again after setup!
    this->makeCurrent();
    
    this->createFBO();
}

auto TestGLWidget::resizeGL(int w, int h) -> void
{
    LOG(INFO) << "resize " << w << ", " << h;
    this->tr->resize(w, h);
    this->didResize = true;
}

auto TestGLWidget::paintGL() -> void
{
//    LOG(INFO) << "paintGL";
    this->tr->run();
    // This only happens, if we did resize, but the renderer already created the new buffers!
    if (this->didResize && !this->tr->didResize) {
        // so recreate our buffers as well!
        this->deleteFBO();
        this->createFBO();
        this->didResize = false;
    }
    auto f = QOpenGLContext::currentContext()->functions();
    glBindFramebuffer(GL_FRAMEBUFFER, this->defaultFramebufferObject());
    f->glBindFramebuffer(GL_DRAW_FRAMEBUFFER, this->defaultFramebufferObject());
    f->glBindFramebuffer(GL_READ_FRAMEBUFFER, this->blitFBO);
    {
        std::scoped_lock lock(this->tr->blit_output);
        glBlitFramebuffer(0, 0, this->tr->width, this->tr->height, 0, 0, this->tr->width, this->tr->height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
        glFlush();
        glFinish();
    }
    this->update();
}
