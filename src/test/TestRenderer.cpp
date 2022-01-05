#include "TestRenderer.h"
#include <chrono>

const char *vertexShaderSource =
R"(#version 330 core
layout (location = 0) in vec2 position;
layout (location = 1) in vec2 inTexCoord;

out vec2 texCoord;
void main(){
    texCoord = inTexCoord;
    gl_Position = vec4(position.x, position.y, 0.0f, 1.0f);
})";

const char *fragmentShaderSource =
R"(#version 330 core
in vec2 TexCoords;
uniform vec2 iMouse;
uniform vec2 iResolution;
uniform float iTime;
out vec4 fragColor;
vec2 fragCoord = gl_FragCoord.xy;
#define PI 3.14159265359
#define TWOPI 6.28318530718
/*
 * License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.
 */

// Color computation inspired by: https://www.shadertoy.com/view/Ms2SD1

// I was looking for waves that would look good on a flat 2D plane.
// I didn't really find something to my liking so I tinkered a bit
// (with some inspiration from other shaders obviously).
// I thought the end result was decent so I wanted to share

const float gravity = 1.0;
const float waterTension = 0.01;
const vec3 skyCol1 = vec3(0.2, 0.4, 0.6);
const vec3 skyCol2 = vec3(0.4, 0.7, 1.0);
const vec3 sunCol  =  vec3(8.0,7.0,6.0)/8.0;
const vec3 seaCol1 = vec3(0.1,0.2,0.2);
const vec3 seaCol2 = vec3(0.8,0.9,0.6);

float gravityWave(in vec2 p, float k, float h) {
  float w = sqrt(gravity*k*tanh(k*h));
  return sin(p.y*k + w*iTime);
}

float capillaryWave(in vec2 p, float k, float h) {
  float w = sqrt((gravity*k + waterTension*k*k*k)*tanh(k*h));
  return sin(p.y*k + w*iTime);
}

void rot(inout vec2 p, float a) {
  float c = cos(a);
  float s = sin(a);
  p = vec2(p.x*c + p.y*s, -p.x*s + p.y*c);
}

float seaHeight(in vec2 p) {
  float height = 0.0;

  float k = 1.0;
  float kk = 1.3;
  float a = 0.25;
  float aa = 1.0/(kk*kk);

  float h = 10.0;
  p *= 0.5;

  for (int i = 0; i < 3; ++i) {
    height += a*gravityWave(p + float(i), k, h);
    rot(p, float(i));
    k *= kk;
    a *= aa;
  }
  
  for (int i = 3; i < 7; ++i) {
    height += a*capillaryWave(p + float(i), k, h);
    rot(p, float(i));
    k *= kk;
    a *= aa;
  }

  return height;
}

vec3 seaNormal(in vec2 p, in float d) {
  vec2 eps = vec2(0.001*pow(d, 1.5), 0.0);
  vec3 n = vec3(
    seaHeight(p + eps) - seaHeight(p - eps),
    2.0*eps.x,
    seaHeight(p + eps.yx) - seaHeight(p - eps.yx)
  );
  
  return normalize(n);
}

vec3 sunDirection() {
  vec3 dir = normalize(vec3(0, 0.15, 1));
  return dir;
}

vec3 skyColor(vec3 rd) {
  vec3 sunDir = sunDirection();

  float sunDot = max(dot(rd, sunDir), 0.0);
  
  vec3 final = vec3(0.0);

  final += mix(skyCol1, skyCol2, rd.y);

  final += 0.5*sunCol*pow(sunDot, 20.0);

  final += 4.0*sunCol*pow(sunDot, 400.0);
    
  return final;
}

void main() {
  vec2 q=fragCoord.xy/iResolution.xy;
  vec2 p = -1.0 + 2.0*q;
  p.x *= iResolution.x/iResolution.y;

  vec3 ro = vec3(0.0, 10.0, 0.0);
  vec3 ww = normalize(vec3(0.0, -0.1, 1.0));
  vec3 uu = normalize(cross( vec3(0.0,1.0,0.0), ww));
  vec3 vv = normalize(cross(ww,uu));
  vec3 rd = normalize(p.x*uu + p.y*vv + 2.5*ww);

  vec3 col = vec3(0.0);

  float dsea = (0.0 - ro.y)/rd.y;
  
  vec3 sunDir = sunDirection();
  
  vec3 sky = skyColor(rd);
    
  if (dsea > 0.0) {
    vec3 p = ro + dsea*rd;
    float h = seaHeight(p.xz);
    vec3 nor = mix(seaNormal(p.xz, dsea), vec3(0.0, 1.0, 0.0), smoothstep(0.0, 200.0, dsea));
    float fre = clamp(1.0 - dot(-nor,rd), 0.0, 1.0);
    fre = pow(fre, 3.0);
    float dif = mix(0.25, 1.0, max(dot(nor,sunDir), 0.0));
    
    vec3 refl = skyColor(reflect(rd, nor));
    vec3 refr = seaCol1 + dif*seaCol2*0.1;
    
    col = mix(refr, 0.9*refl, fre);
      
    float atten = max(1.0 - dot(dsea,dsea) * 0.001, 0.0);
    col += seaCol2 * (p.y - h) * 2.0 * atten;
      
    col = mix(col, sky, 1.0 - exp(-0.01*dsea));
  } else {
    col = sky;
  }

  fragColor = vec4(col,1.0);
}

/*
 * "Seascape" by Alexander Alekseev aka TDM - 2014
 * License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.
 * Contact: tdmaav@gmail.com
 */
/*
const int NUM_STEPS = 8;
//const float PI         = 3.141592;
const float EPSILON    = 1e-3;
#define EPSILON_NRM (0.1 / iResolution.x)
#define AA

// sea
const int ITER_GEOMETRY = 3;
const int ITER_FRAGMENT = 5;
const float SEA_HEIGHT = 0.6;
const float SEA_CHOPPY = 4.0;
const float SEA_SPEED = 0.8;
const float SEA_FREQ = 0.16;
const vec3 SEA_BASE = vec3(0.0,0.09,0.18);
const vec3 SEA_WATER_COLOR = vec3(0.8,0.9,0.6)*0.6;
#define SEA_TIME (1.0 + iTime * SEA_SPEED)
const mat2 octave_m = mat2(1.6,1.2,-1.2,1.6);

// math
mat3 fromEuler(vec3 ang) {
    vec2 a1 = vec2(sin(ang.x),cos(ang.x));
    vec2 a2 = vec2(sin(ang.y),cos(ang.y));
    vec2 a3 = vec2(sin(ang.z),cos(ang.z));
    mat3 m;
    m[0] = vec3(a1.y*a3.y+a1.x*a2.x*a3.x,a1.y*a2.x*a3.x+a3.y*a1.x,-a2.y*a3.x);
    m[1] = vec3(-a2.y*a1.x,a1.y*a2.y,a2.x);
    m[2] = vec3(a3.y*a1.x*a2.x+a1.y*a3.x,a1.x*a3.x-a1.y*a3.y*a2.x,a2.y*a3.y);
    return m;
}
float hash( vec2 p ) {
    float h = dot(p,vec2(127.1,311.7));
    return fract(sin(h)*43758.5453123);
}
float noise( in vec2 p ) {
    vec2 i = floor( p );
    vec2 f = fract( p );
    vec2 u = f*f*(3.0-2.0*f);
    return -1.0+2.0*mix( mix( hash( i + vec2(0.0,0.0) ),
                     hash( i + vec2(1.0,0.0) ), u.x),
                mix( hash( i + vec2(0.0,1.0) ),
                     hash( i + vec2(1.0,1.0) ), u.x), u.y);
}

// lighting
float diffuse(vec3 n,vec3 l,float p) {
    return pow(dot(n,l) * 0.4 + 0.6,p);
}
float specular(vec3 n,vec3 l,vec3 e,float s) {
    float nrm = (s + 8.0) / (PI * 8.0);
    return pow(max(dot(reflect(e,n),l),0.0),s) * nrm;
}

// sky
vec3 getSkyColor(vec3 e) {
    e.y = (max(e.y,0.0)*0.8+0.2)*0.8;
    return vec3(pow(1.0-e.y,2.0), 1.0-e.y, 0.6+(1.0-e.y)*0.4) * 1.1;
}

// sea
float sea_octave(vec2 uv, float choppy) {
    uv += noise(uv);
    vec2 wv = 1.0-abs(sin(uv));
    vec2 swv = abs(cos(uv));
    wv = mix(wv,swv,wv);
    return pow(1.0-pow(wv.x * wv.y,0.65),choppy);
}

float map(vec3 p) {
    float freq = SEA_FREQ;
    float amp = SEA_HEIGHT;
    float choppy = SEA_CHOPPY;
    vec2 uv = p.xz; uv.x *= 0.75;
    
    float d, h = 0.0;
    for(int i = 0; i < ITER_GEOMETRY; i++) {
        d = sea_octave((uv+SEA_TIME)*freq,choppy);
        d += sea_octave((uv-SEA_TIME)*freq,choppy);
        h += d * amp;
        uv *= octave_m; freq *= 1.9; amp *= 0.22;
        choppy = mix(choppy,1.0,0.2);
    }
    return p.y - h;
}

float map_detailed(vec3 p) {
    float freq = SEA_FREQ;
    float amp = SEA_HEIGHT;
    float choppy = SEA_CHOPPY;
    vec2 uv = p.xz; uv.x *= 0.75;
    
    float d, h = 0.0;
    for(int i = 0; i < ITER_FRAGMENT; i++) {
        d = sea_octave((uv+SEA_TIME)*freq,choppy);
        d += sea_octave((uv-SEA_TIME)*freq,choppy);
        h += d * amp;
        uv *= octave_m; freq *= 1.9; amp *= 0.22;
        choppy = mix(choppy,1.0,0.2);
    }
    return p.y - h;
}

vec3 getSeaColor(vec3 p, vec3 n, vec3 l, vec3 eye, vec3 dist) {
    float fresnel = clamp(1.0 - dot(n,-eye), 0.0, 1.0);
    fresnel = pow(fresnel,3.0) * 0.5;
        
    vec3 reflected = getSkyColor(reflect(eye,n));
    vec3 refracted = SEA_BASE + diffuse(n,l,80.0) * SEA_WATER_COLOR * 0.12;
    
    vec3 color = mix(refracted,reflected,fresnel);
    
    float atten = max(1.0 - dot(dist,dist) * 0.001, 0.0);
    color += SEA_WATER_COLOR * (p.y - SEA_HEIGHT) * 0.18 * atten;
    
    color += vec3(specular(n,l,eye,60.0));
    
    return color;
}

// tracing
vec3 getNormal(vec3 p, float eps) {
    vec3 n;
    n.y = map_detailed(p);
    n.x = map_detailed(vec3(p.x+eps,p.y,p.z)) - n.y;
    n.z = map_detailed(vec3(p.x,p.y,p.z+eps)) - n.y;
    n.y = eps;
    return normalize(n);
}

float heightMapTracing(vec3 ori, vec3 dir, out vec3 p) {
    float tm = 0.0;
    float tx = 1000.0;
    float hx = map(ori + dir * tx);
    if(hx > 0.0) {
        p = ori + dir * tx;
        return tx;
    }
    float hm = map(ori + dir * tm);
    float tmid = 0.0;
    for(int i = 0; i < NUM_STEPS; i++) {
        tmid = mix(tm,tx, hm/(hm-hx));
        p = ori + dir * tmid;
        float hmid = map(p);
        if(hmid < 0.0) {
            tx = tmid;
            hx = hmid;
        } else {
            tm = tmid;
            hm = hmid;
        }
    }
    return tmid;
}

vec3 getPixel(in vec2 coord, float time) {
    vec2 uv = coord / iResolution.xy;
    uv = uv * 2.0 - 1.0;
    uv.x *= iResolution.x / iResolution.y;
        
    // ray
    vec3 ang = vec3(sin(time*3.0)*0.1,sin(time)*0.2+0.3,time);
    vec3 ori = vec3(0.0,3.5,time*5.0);
    vec3 dir = normalize(vec3(uv.xy,-2.0)); dir.z += length(uv) * 0.14;
    dir = normalize(dir) * fromEuler(ang);
    
    // tracing
    vec3 p;
    heightMapTracing(ori,dir,p);
    vec3 dist = p - ori;
    vec3 n = getNormal(p, dot(dist,dist) * EPSILON_NRM);
    vec3 light = normalize(vec3(0.0,1.0,0.8));
             
    // color
    return mix(
        getSkyColor(dir),
        getSeaColor(p,n,light,dir,dist),
        pow(smoothstep(0.0,-0.02,dir.y),0.2));
}

// main
void main() {
    float time = iTime * 0.3 + iMouse.x*0.01;
    
#ifdef AA
    vec3 color = vec3(0.0);
    for(int i = -1; i <= 1; i++) {
        for(int j = -1; j <= 1; j++) {
            vec2 uv = fragCoord+vec2(i,j)/3.0;
            color += getPixel(uv, time);
        }
    }
    color /= 9.0;
#else
    vec3 color = getPixel(fragCoord, time);
#endif
    
    // post
    fragColor = vec4(pow(color,vec3(0.65)), 1.0);
}*/

)";

TestRenderer::TestRenderer(QOpenGLContext* context, QSurface* surface) : QOpenGLExtraFunctions(context), tasarch::log::WithLogger("tasarch.testrenderer")
{
    this->context = context;
    this->surface = surface;
    this->initializeOpenGLFunctions();
}

auto TestRenderer::createFBO() -> bool
{
    GLenum error = 0;
#define CHECK_GL(msg)     error = glGetError(); \
    if (error != GL_NO_ERROR) { \
        logger->error("opengl had error: {}, msg: {}", error, msg); \
        return false; \
    }
    
    logger->debug("setting up output buffer");
    
    glGenFramebuffers(1, &this->outputFBO);
    CHECK_GL("failed to create framebuffer");
    glBindFramebuffer(GL_FRAMEBUFFER, this->outputFBO);
    CHECK_GL("failed to bind framebuffer");
    
    glGenTextures(1, &this->outputRenderBuffer);
    CHECK_GL("failed to gen render buffer");
    
    glBindTexture(GL_TEXTURE_2D, this->outputRenderBuffer);
    CHECK_GL("failed to bind render buffer");
    
    // Give an empty image to OpenGL ( the last "0" ), we can resize later!
    glTexImage2D(GL_TEXTURE_2D, 0,GL_RGB, this->width, this->height, 0,GL_RGB, GL_UNSIGNED_BYTE, 0);
    CHECK_GL("failed to create texture");
    
    // Poor filtering. Needed !
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    CHECK_GL("failed to set filtering");
    
    // The depth buffer, we dont care about saving this
    glGenRenderbuffers(1, &this->outputDepthTexture);
    glBindRenderbuffer(GL_RENDERBUFFER, this->outputDepthTexture);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, this->width, this->height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, this->outputDepthTexture);
    CHECK_GL("something depth buffer");
    
    // Set "renderedTexture" as our colour attachement #0
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, this->outputRenderBuffer, 0);

    // Set the list of draw buffers.
    GLenum DrawBuffers[1] = {GL_COLOR_ATTACHMENT0};
    glDrawBuffers(1, DrawBuffers); // "1" is the size of DrawBuffers
    
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if(status != GL_FRAMEBUFFER_COMPLETE) {
        logger->error("framebuffer status is incomplete!: {}", status);
        return false;
    }
    
    logger->debug("output fbo: {}, output buffer: {}", this->outputFBO, this->outputRenderBuffer);
    
    logger->debug("setting up final output buffer");
    
    glGenFramebuffers(1, &this->finalOutputFBO);
    CHECK_GL("failed to create framebuffer");
    glBindFramebuffer(GL_FRAMEBUFFER, this->finalOutputFBO);
    CHECK_GL("failed to bind framebuffer");
    
    glGenTextures(1, &this->finalOutputBuffer);
    CHECK_GL("failed to gen render buffer");
    
    glBindTexture(GL_TEXTURE_2D, this->finalOutputBuffer);
    CHECK_GL("failed to bind render buffer");
    
    // Give an empty image to OpenGL ( the last "0" ), we can resize later!
    glTexImage2D(GL_TEXTURE_2D, 0,GL_RGB, this->width, this->height, 0,GL_RGB, GL_UNSIGNED_BYTE, 0);
    CHECK_GL("failed to create texture");
    
    // Poor filtering. Needed !
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    CHECK_GL("failed to set filtering");
    
    // Set "renderedTexture" as our colour attachement #0
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, this->finalOutputBuffer, 0);
    
    // Set the list of draw buffers.
    DrawBuffers[0] = {GL_COLOR_ATTACHMENT0};
    glDrawBuffers(1, DrawBuffers); // "1" is the size of DrawBuffers
    
    status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if(status != GL_FRAMEBUFFER_COMPLETE) {
        logger->error("framebuffer status is incomplete!: {}", status);
        return false;
    }
    
    logger->debug("final output fbo: {}, final output buffer: {}", this->finalOutputFBO, this->finalOutputBuffer);
    
    return true;
}

auto TestRenderer::deleteFBO() -> void
{
    glDeleteFramebuffers(1, &this->outputFBO);
    glDeleteTextures(1, &this->outputRenderBuffer);
    glDeleteRenderbuffers(1, &this->outputDepthTexture);
    
    glDeleteFramebuffers(1, &this->finalOutputFBO);
    glDeleteTextures(1, &this->finalOutputBuffer);
}

auto TestRenderer::checkShader(unsigned int shader, GLenum status) -> bool {
    GLint isCompiled = 0;
    if (status == GL_COMPILE_STATUS) {
        glGetShaderiv(shader, status, &isCompiled);
    } else {
        glGetProgramiv(shader, status, &isCompiled);
    }
    
    if(isCompiled == GL_FALSE)
    {
        GLint maxLength = 0;
        if (status == GL_COMPILE_STATUS) {
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);
        } else {
            glGetProgramiv(shader, GL_INFO_LOG_LENGTH, &maxLength);
        }

        // The maxLength includes the NULL character
        std::vector<GLchar> errorLog(maxLength);
        if (status == GL_COMPILE_STATUS) {
            glGetShaderInfoLog(shader, maxLength, &maxLength, &errorLog[0]);
        } else {
            glGetProgramInfoLog(shader, maxLength, &maxLength, &errorLog[0]);
        }
        
        std::string errorlog(errorLog.begin(), errorLog.end());
        logger->error("shader errord:\n", errorlog);
        // Provide the infolog in whatever manor you deem best.
        // Exit with failure.
        glDeleteShader(shader); // Don't leak the shader.
        return false;
    }
    
    return true;
}

auto TestRenderer::setup() -> void
{
    this->context->makeCurrent(surface);
    
    const GLubyte *renderer = glGetString( GL_RENDERER );
    const GLubyte *vendor = glGetString( GL_VENDOR );
    const GLubyte *version = glGetString( GL_VERSION );
    const GLubyte *glslVersion =
           glGetString( GL_SHADING_LANGUAGE_VERSION );
     
    GLint major = 0, minor = 0;
    glGetIntegerv(GL_MAJOR_VERSION, &major);
    glGetIntegerv(GL_MINOR_VERSION, &minor);
    
    logger->info(R"(opengl information:
    renderer: {}
    vendor: {}
    glsl version: {}
    reported version??: {}.{}
)", renderer, vendor, version, glslVersion, major, minor);
    
    if (!this->createFBO()) {
        this->context->doneCurrent();
        return;
    }
    
    logger->debug("setting up vertices and shader");
    
    float quadVerts[] = {
        -1.0, -1.0,     0.0, 0.0,
        -1.0, 1.0,      0.0, 1.0,
        1.0, -1.0,      1.0, 0.0,

        1.0, -1.0,      1.0, 0.0,
        -1.0, 1.0,      0.0, 1.0,
        1.0, 1.0,       1.0, 1.0
    };

    glGenVertexArrays(1, &this->vao);
    glBindVertexArray(this->vao);

    glGenBuffers(1, &this->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVerts), quadVerts, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<void*>(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
    
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    
    logger->debug("compiled vertex shader");
    
    if (!this->checkShader(vertexShader, GL_COMPILE_STATUS)) {
        this->context->doneCurrent();
        return;
    }

    // fragment shader
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    // check for shader compile errors
    logger->debug("compiled fragment shader");
    
    if (!this->checkShader(fragmentShader, GL_COMPILE_STATUS)) {
        this->context->doneCurrent();
        return;
    }


    // link shaders
    this->shader = glCreateProgram();
    glAttachShader(this->shader, vertexShader);
    glAttachShader(this->shader, fragmentShader);
    glLinkProgram(this->shader);
    // check for linking errors
    
    logger->debug("linked shaders");
    
    if (!this->checkShader(this->shader, GL_LINK_STATUS)) {
        this->context->doneCurrent();
        return;
    }


    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    glUseProgram(this->shader);
    float screen[2] = {(float)this->width, (float)this->height};
    glUniform2fv(glGetUniformLocation(this->shader, "iResolution"), 1, &screen[0]);
    
    this->context->doneCurrent();
}

auto TestRenderer::resize(int w, int h) -> void
{
    this->width = w;
    this->height = h;
    this->didResize = true;
}

auto TestRenderer::render() -> void
{
    if (this->didResize) {
        this->deleteFBO();
        this->createFBO();
        this->didResize = false;
    }
    // TODO make this more interesting!
    glBindFramebuffer(GL_FRAMEBUFFER, this->outputFBO);
//    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0,0,this->width,this->height);
//    glClearColor(1.0, 0.0, 0.0, 1.0);
    
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//    glClear(GL_COLOR_BUFFER_BIT);
    
    auto curr_clock = std::chrono::high_resolution_clock::now();
//    double diff = ((((double)(curr_clock - start_clock)) / CLOCKS_PER_SEC));
    std::chrono::duration<float> diff = curr_clock - start_clock;

//    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glUseProgram(this->shader);
    float screen[2] = {(float)this->width, (float)this->height};
    
    glUniform2fv(glGetUniformLocation(this->shader, "iResolution"), 1, &screen[0]);
    glUniform1f(glGetUniformLocation(this->shader, "iTime"), diff.count());
    glBindVertexArray(this->vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glFlush();
    
    glFinish();
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    // I think we dont need the lock for these?
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, this->finalOutputFBO);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, this->outputFBO);
    {
        std::scoped_lock lock(this->blit_output);
        glBlitFramebuffer(0, 0, this->width, this->height, 0, 0, this->width, this->height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
        glFlush();
        glFinish();
    }
//    prev_clock = curr_clock;
}

auto TestRenderer::run() -> void
{
    if (!this->stopped) {
        // Already running
        return;
    }
    this->stopped = false;
    this->render_thread = QThread::create([&](){
        logger->info("render thread started");
        this->context->makeCurrent(this->surface);
        this->start_clock = std::chrono::high_resolution_clock::now();
        double average = 0.0;
        size_t num = 0;
        while (!this->stopped) {
//            TIMED_SCOPE(renderTimer, "render");
//            clock_t start = clock();
            auto start = std::chrono::high_resolution_clock::now();
            this->render();
//            clock_t end = clock();
            auto end = std::chrono::high_resolution_clock::now();
//            double sec_diff = ((double)(end - start)) / CLOCKS_PER_SEC;
            std::chrono::duration<double> diff = end - start;
            double sec_diff = diff.count();
            average += sec_diff;
            num++;
            if (num == 10) {
                average = average / num;
                num = 0;
            }
//            QThread::msleep(10);
        }
    });
    this->context->moveToThread(this->render_thread);
    this->render_thread->start();
}

auto TestRenderer::stop() -> void
{
    if (this->stopped) {
        // not running
        return;
    }
    this->stopped = true;
    this->render_thread->quit();
    this->render_thread->wait();
    delete this->render_thread;
}

TestRenderer::~TestRenderer()
{
    if (!this->stopped) {
        this->stopped = true;
        // Dont wait here, maybe we should?
        this->render_thread->quit();
        delete this->render_thread;
    }

    this->deleteFBO();
    delete this->context;
    delete this->surface;
}
