#include <cstdio>
#include <cstdlib>

#include "Renderer.h"
#include "Core.h"

static char *pixels;

static void render_callback(void *userdata, const void *_data, unsigned width,
        unsigned height, size_t pitch) {
    PixelRenderer *renderer = (PixelRenderer*) userdata;
    char *data = (char*) _data;
    renderer->m_context->makeCurrent(renderer->m_surface);
    //glfwMakeContextCurrent(renderer->m_window);

    renderer->glActiveTexture(GL_TEXTURE0);
    renderer->glDisable(GL_DEPTH_TEST);
    renderer->glDisable(GL_STENCIL_TEST);
    renderer->glClear(GL_COLOR_BUFFER_BIT);

    int w, h;
    //glfwGetFramebufferSize(renderer->m_window, &w, &h);
    renderer->glViewport(0, 0, w, h);

    renderer->glBindTexture(GL_TEXTURE_2D, renderer->m_texID);

    for(unsigned y = 0; y < height; y++) {
        for(unsigned x = 0; x < width; x++) {
            unsigned i = y * pitch + x * 2;
            unsigned j = (height - 1 - y) * (width * 4) + x * 4;
            pixels[j+0] = 0xFF;
            pixels[j+1] = (data[i+0] & 0x1f) << 3;
            pixels[j+2] = ((data[i+0] & 0xE0) >> 3) | ((data[i+1] & 7) << 5);
            pixels[j+3] = data[i+1] & 0xF8;
        }
    }
    
    /*for(unsigned y = 0; y < height; y++) {
        for(unsigned x = 0; x < width; x++) {
            unsigned i = y * pitch + x * 4;
            unsigned j = (height - 1 - y) * (width * 4) + x * 4;
            pixels[j+0] = 0xFF;
            pixels[j+1] = x&0xFF;//data[i+1];
            pixels[j+2] = data[i+2];
            pixels[j+3] = data[i+3];
        }
    }*/

    renderer->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, pixels);

    renderer->glUseProgram(renderer->m_shaderID);
    unsigned int loc = renderer->glGetUniformLocation(renderer->m_shaderID, "screenTexture");
    renderer->glUniform1i(loc, 0);

    renderer->glBindVertexArray(renderer->m_vaoID);
    renderer->glDrawArrays(GL_TRIANGLES, 0, 6);
}

GLuint PixelRenderer::init(Core *core, int width, int height) {
    m_context->makeCurrent(m_surface);
    core->set_render_callback(&render_callback, this);

    pixels = (char*) malloc(width * height * 4);

    glGenTextures(1, &m_texID);
    glBindTexture(GL_TEXTURE_2D, m_texID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    load_shaders("assets/shaders/plain.vert", "assets/shaders/plain.frag");
    create_vbo();
    
    core->reset_context();

    return m_texID;
}


