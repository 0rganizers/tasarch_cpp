#include "Renderer.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

static char *read_file(const char *path) {
    FILE *f = fopen(path, "r");
    fseek(f, 0, SEEK_END);
    size_t len = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *data = (char*) malloc(len + 1);
    fread(data, 1, len, f);
    data[len] = 0;
    return data;
}

void Renderer::load_shaders(const char *vert, const char *frag) {
    const char *vert_src = read_file(vert);
    const char *frag_src = read_file(frag);
    GLint status;

    GLuint vertID = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertID, 1, &vert_src, NULL);
    glCompileShader(vertID);
    glGetShaderiv(vertID, GL_COMPILE_STATUS, &status);
    if(status != GL_TRUE) {
        int log_len;
        glGetShaderiv(vertID, GL_INFO_LOG_LENGTH, &log_len);
        char buf[log_len + 1];
        glGetShaderInfoLog(vertID, log_len, NULL, buf);
        buf[log_len] = 0;
        fprintf(stderr, "Could not compile vertex shader: %s\n", buf);
        exit(1);
    }

    GLuint fragID = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragID, 1, &frag_src, NULL);
    glCompileShader(fragID);
    glGetShaderiv(fragID, GL_COMPILE_STATUS, &status);
    if(status != GL_TRUE) {
        int log_len;
        glGetShaderiv(fragID, GL_INFO_LOG_LENGTH, &log_len);
        char buf[log_len + 1];
        glGetShaderInfoLog(fragID, log_len, NULL, buf);
        buf[log_len] = 0;
        fprintf(stderr, "Could not compile fragment shader: %s\n", buf);
        exit(1);
    }

    m_shaderID = glCreateProgram();
    glAttachShader(m_shaderID, vertID);
    glAttachShader(m_shaderID, fragID);
    glLinkProgram(m_shaderID);
    glDeleteShader(vertID);
    glDeleteShader(fragID);
    glUseProgram(0);

    free((void*) vert_src);
    free((void*) frag_src);
}

GLfloat rect_vertices[12] = {
    -1.0, -1.0,
     1.0, -1.0,
     1.0,  1.0,

     1.0,  1.0,
    -1.0,  1.0,
    -1.0, -1.0,
};

void Renderer::create_vbo() {
    glGenBuffers(1, &m_vboID);
    glBindBuffer(GL_ARRAY_BUFFER, m_vboID);
    glBufferData(GL_ARRAY_BUFFER, 12 * sizeof(GLfloat), rect_vertices, GL_STATIC_DRAW);

    glGenVertexArrays(1, &m_vaoID);
    glBindVertexArray(m_vaoID);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL);
    glBindVertexArray(0);
}

Renderer::~Renderer() {
}
