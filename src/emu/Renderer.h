#pragma once

#include <QOpenGLContext>
#include <QOpenGLExtraFunctions>
#include <QSurface>
#include <mutex>
#include <thread>
#include <QThread>
#include <chrono>
#include <easyloggingpp/easylogging++.h>

class Core;

class Renderer : public QOpenGLExtraFunctions {
protected:
    void load_shaders(const char *vertpath, const char *fragpath);
    void create_vbo();


public:
    Renderer(QOpenGLContext* context, QSurface* surface)
        : m_context(context), m_surface(surface)
    {
        initializeOpenGLFunctions();
    }
    virtual ~Renderer();
    virtual GLuint init(Core *core, int w, int h) = 0;

    QOpenGLContext *m_context = nullptr;
    QSurface *m_surface = nullptr;

    std::mutex m_blit_output;

    GLuint m_vaoID, m_vboID;
    GLuint m_shaderID;
    GLuint m_texID;
};

class OpenGLRenderer : public Renderer {
public:
    GLuint m_fboID, m_doublebuffer_fboID;
    GLuint m_doublebuffer_texID;
    OpenGLRenderer(QOpenGLContext* context, QSurface* surface)
        : Renderer(context, surface) {}
    GLuint init(Core *core, int w, int h) override;
};

class PixelRenderer : public Renderer {
public:
    PixelRenderer(QOpenGLContext* context, QSurface* surface)
        : Renderer(context, surface) {}
    GLuint init(Core *core, int w, int h) override;
};
