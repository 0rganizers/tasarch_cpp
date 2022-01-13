#ifndef __TESTRENDERER_H
#define __TESTRENDERER_H

#include <QOpenGLContext>
#include <QOpenGLExtraFunctions>
#include <QSurface>
#include <mutex>
#include <thread>
#include <QThread>
#include <chrono>
#include "log/logging.h"

class TestRenderer : public QOpenGLExtraFunctions, public tasarch::log::WithLogger {
public:
    TestRenderer(QOpenGLContext* context, QSurface* surface);
    ~TestRenderer();

    /// Setup any resources, that can be initialized forever!
    void setup();
    
    /// Called whenever we should resize the output to the specified size!
    void resize(int w, int h);
    
    /// Main method that renders a single frame.
    void render();
    
    /// Runs the render method in a separate thread in a loop until stop is called.
    void run();
    
    /// Stops the rendering process.
    void stop();
    
    GLuint finalOutputBuffer = 0;
    GLuint outputRenderBuffer = 0;
    int width = 1024;
    int height = 768;
    bool didResize = false;
    /// Held whenever the final output of the render process is blitted to the final render texture.
    std::mutex blit_output;
private:
    bool stopped = true;
   
    QThread* render_thread;
    GLuint outputFBO = 0;
    GLuint finalOutputFBO = 0;
    GLuint outputDepthTexture = 0;
    QOpenGLContext* context;
    QSurface* surface;
    
    GLuint vao = 0;
    GLuint vbo = 0;
    unsigned int shader = 0;
    std::chrono::time_point<std::chrono::high_resolution_clock> start_clock;
    
    bool createFBO();
    void deleteFBO();
    bool checkShader(unsigned int shader, GLenum status);
};

#endif /* __TESTRENDERER_H */
