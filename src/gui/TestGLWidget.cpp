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

namespace tasarch::gui {

    TestGLWidget::TestGLWidget(QWidget* parent) : QOpenGLWidget(parent), log::WithLogger("tasarch.testgl")
    {
        logger->info("TestGLWidget constructed!");
    }

    auto TestGLWidget::createFBO() -> bool
    {
        // Now we also need to setup the fbo's here, otherwise, we cannot access the texture!
        glGenFramebuffers(1, &this->blitFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, this->blitFBO);
        glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, core->m_renderer->m_texID, 0);
        GLenum error = glGetError();
        if (error != GL_NO_ERROR) {
            logger->error("failed to setup framebuffer {}", error);
            return false;
        }
        
        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if(status != GL_FRAMEBUFFER_COMPLETE) {
            logger->error("framebuffer status is incomplete!: {}", status);
            return false;
        }
        
        logger->debug("blit fbo: {}", this->blitFBO);
        
        return true;
    }

    auto TestGLWidget::deleteFBO() -> void
    {
        glDeleteFramebuffers(1, &this->blitFBO);
    }

    auto TestGLWidget::initializeGL() -> void
    {
        logger->debug("initializing context");
        if (this->core != nullptr) {
            this->core->stop();
            delete this->core;
        }
        QOffscreenSurface* surface = new QOffscreenSurface();
        surface->setFormat(this->format());
        surface->create();
        
        QOpenGLContext* subctx = new QOpenGLContext();
        subctx->setShareContext(QOpenGLContext::currentContext());
        subctx->setFormat(this->format());
        if (!subctx->create()) {
            logger->error("failed to create subctx!");
            return;
        }

#define TEST_DOLPHIN
#ifdef TEST_DOLPHIN 
        logger->info("Creating core...");
        this->core = new Core("/home/leonardo/code/organizers/dolphin/build/dolphin_libretro.so");
        logger->info("Loading game...");
        this->core->load_game("/home/leonardo/ww.iso");
        logger->info("Game loaded!");
#else
        this->core = new Core("/home/aaron/Projects/TASarch/TASarch/desmume_libretro.so");
        this->core->load_game("/home/aaron/Projects/TASarch/TASarch/plat.nds");
#endif

        this->core->init_renderer(subctx, surface);
        this->core->m_main_thread = thread();
        // Need to make current again after setup!
        this->makeCurrent();
        this->initializeOpenGLFunctions();
        
        this->createFBO();
    }

    auto TestGLWidget::resizeGL(int w, int h) -> void
    {
        logger->trace("resize {}x{}", w, h);
    }

    auto TestGLWidget::paintGL() -> void
    {
        this->core->run();
        glBindFramebuffer(GL_FRAMEBUFFER, this->defaultFramebufferObject());
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, this->defaultFramebufferObject());
        glBindFramebuffer(GL_READ_FRAMEBUFFER, this->blitFBO);
        {
            std::scoped_lock lock(this->core->m_renderer->m_blit_output);
     
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, this->defaultFramebufferObject());
            glBindFramebuffer(GL_READ_FRAMEBUFFER, this->blitFBO);
            
            avinfo info = this->core->get_info();
            glBlitFramebuffer(0, 0, info.geometry.max_width, info.geometry.max_height, 
                0, 0, width(), height(), GL_COLOR_BUFFER_BIT, GL_NEAREST);

        }
        this->update();
    }

} // namespace tasarch::gui
