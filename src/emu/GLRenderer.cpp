#include <cstdio>
#include <cstdlib>

#include "Renderer.h"
#include "Core.h"

static void render_callback(void *userdata, const void *data, unsigned width,
        unsigned height, size_t pitch) {
    OpenGLRenderer *renderer = (OpenGLRenderer*) userdata;

    GLenum error = renderer->glGetError();
    if (error != GL_NO_ERROR) {
        //LOG(ERROR) << "failed somewhere: " << error;
    }
    renderer->glViewport(0, 0, width, height);
    renderer->glBindFramebuffer(GL_DRAW_FRAMEBUFFER, renderer->m_fboID);
    renderer->glBindFramebuffer(GL_READ_FRAMEBUFFER, renderer->m_doublebuffer_fboID);
    {
        std::scoped_lock lock(renderer->m_blit_output);
        renderer->glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    }
    renderer->glBindFramebuffer(GL_DRAW_FRAMEBUFFER, renderer->m_doublebuffer_fboID);
}

GLuint OpenGLRenderer::init(Core *core, int width, int height) {
    m_context->makeCurrent(m_surface);
    core->set_render_callback(&render_callback, this);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    // create render target for the core
    glGenFramebuffers(1, &m_fboID);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fboID);

    glGenTextures(1, &m_texID);
    //LOG(INFO) << "Generated texture with ID: " << m_texID;
    glBindTexture(GL_TEXTURE_2D, m_texID);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, width, height);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_texID, 0);

    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        fprintf(stderr, "Could not create framebuffer\n");
        exit(1);
    }


    // create double buffered framebuffer
    glGenFramebuffers(1, &m_doublebuffer_fboID);
    glBindFramebuffer(GL_FRAMEBUFFER, m_doublebuffer_fboID);

    glGenTextures(1, &m_doublebuffer_texID);
    glBindTexture(GL_TEXTURE_2D, m_doublebuffer_texID);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, width, height);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_doublebuffer_texID, 0);

    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        fprintf(stderr, "Could not create framebuffer\n");
        exit(1);
    }
    
    //LOG(INFO) << "Framebuffer ID: " << m_fboID;
    core->set_framebuffer(m_doublebuffer_fboID);

    load_shaders("assets/shaders/plain.vert", "assets/shaders/plain.frag");
    create_vbo();
    
    core->reset_context();

    return m_texID;
}

