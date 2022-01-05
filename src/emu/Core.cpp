#include <cstdarg>
#include <dlfcn.h>

#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include <unistd.h>
#include <unordered_map>
#include <QOpenGLContext>

#include "libretro.h"
#include "Core.h"
#include "Callback.h"
#include "Renderer.h"


static std::unordered_map<std::string, std::string> core_variables = {};
static std::unordered_map<std::string, input_desc> input_descriptors = {};
static retro_hw_context_reset_t context_reset = NULL;
static retro_hw_context_reset_t context_deinit = NULL;

static GLuint get_framebuf(void *_core) {
    Core *core = (Core*) _core;
    return core->get_framebuffer();
}

static bool set_rumble_state(unsigned port, enum retro_rumble_effect effect, uint16_t strength) {
    return true;
}

static void log_func(enum retro_log_level level, const char *fmt, ...) {
    printf("[CORE][%d] ", level);
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
}

// perf stuff

static retro_time_t get_time_usec() {
    struct timespec time;
    clock_gettime(CLOCK_MONOTONIC, &time);
    return (time.tv_sec * 1000) + (time.tv_nsec / 1000000);
}

static retro_perf_tick_t get_perf_counter() {
    return 0;
}

static uint64_t get_cpu_features() {
    return 0; // TODO: cpuid
}

static void perf_log() { // ignore
}

static void perf_register(struct retro_perf_counter *counter) {
}
static void perf_start(struct retro_perf_counter *counter) {
}
static void perf_stop(struct retro_perf_counter *counter) {
}

static retro_proc_address_t get_proc_address_impl(const char *func) {
    return QOpenGLContext::currentContext()->getProcAddress(func);
}

// env callback

static bool env_callback(void *core, unsigned int cmd, void *data) {
    Core *c = (Core*) core;
    switch(cmd) {
    case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY:
    {
        const char** path = (const char **) data;
        //*path = get_current_dir_name(); // docs: this may be null; mupen: I don't think so
        // Since get_current_dir_name doesnt exist on macos maybe just set this to "." or sth
        *path = ".";
        break;
    }
    case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT:
    {
        const enum retro_pixel_format *fmt = (enum retro_pixel_format*) data;
        printf("==> Pixel fmt: %d\n", *fmt);
        break;
    }
    case RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS:
    {
        const struct retro_input_descriptor *inputs = 
            (const struct retro_input_descriptor *) data;
        while(inputs->description) {
            printf("Input description: %s\n", inputs->description);
            std::string name(inputs->description);
            input_descriptors[name] = {
                inputs->port,
                inputs->device,
                inputs->index,
                inputs->id
            };

            c->current_inputs[(input_desc) {
                inputs->port,
                inputs->device,
                inputs->index,
                inputs->id
            }] = 0;
            inputs++;
        }
        break;
    }
    case RETRO_ENVIRONMENT_SET_HW_RENDER:
    {
        struct retro_hw_render_callback *hw = (struct retro_hw_render_callback*) data;
        fprintf(stderr, "[+] HW RENDER: %d\n", hw->context_type);
        if(hw->context_type == RETRO_HW_CONTEXT_OPENGL_CORE) {
            printf("Doing the opengl context!\n");
            context_reset = hw->context_reset;
            context_deinit = hw->context_destroy;
            hw->get_current_framebuffer = (retro_hw_get_current_framebuffer_t) 
                create_callback((void*) &get_framebuf, core);
            hw->get_proc_address = &get_proc_address_impl;
            
        } else {
            return false;
        }
        break;
    }
    case RETRO_ENVIRONMENT_GET_VARIABLE:
    {
        struct retro_variable *vars = (struct retro_variable*) data;
        printf("[*] Getting variable: %s => %s\n", vars->key, core_variables[vars->key].c_str());
        vars->value = core_variables[vars->key].c_str();
        break;
    }
    case RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE:
    {
        *((bool*)data) = false;
        break;
    }
    case RETRO_ENVIRONMENT_SET_VARIABLES:
    {
        const struct retro_variable *vars = (struct retro_variable*) data;
        while(vars->key || vars->value) {
            printf("Key: %s\nValue: %s\n", vars->key, vars->value);
            std::string s(vars->value);
            std::string values = s.substr(s.find(";")+2);
            std::string defaultvalue = values.substr(0, values.find("|"));
            core_variables[std::string(vars->key)] = defaultvalue;
            ++vars;
        }
        break;
    }
    case RETRO_ENVIRONMENT_GET_RUMBLE_INTERFACE:
    {
        struct retro_rumble_interface* rumble = (struct retro_rumble_interface*) data;
        rumble->set_rumble_state = &set_rumble_state;
        break;
    }
    case RETRO_ENVIRONMENT_GET_LOG_INTERFACE:
    {
        struct retro_log_callback *log = (struct retro_log_callback*) data;
        log->log = &log_func;
        break;
    }
    case RETRO_ENVIRONMENT_GET_PERF_INTERFACE:
    {
        struct retro_perf_callback *perf = (struct retro_perf_callback *) data;
        perf->get_cpu_features = &get_cpu_features;
        perf->get_perf_counter = &get_perf_counter;
        perf->get_time_usec = &get_time_usec;
        perf->perf_register = &perf_register;
        perf->perf_start = &perf_start;
        perf->perf_stop = &perf_stop;
        perf->perf_log = &perf_log;
        break;
    }
    case RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION:
    {
        *((unsigned*)data) = 0;
        break;
    }
    case RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY:
    {
        // ignore
        break;
    }
    default:
        fprintf(stderr, "[-] Not handled: %d\n", cmd);
        return false;
    }

    // assume we handled the call correctly
    return true;
}

void audio_sample(void *_core, int16_t left, int16_t right) {
    Core *core = (Core*) _core;
    if(core->m_audio) core->m_audio->enqueue(left, right);
}

size_t audio_sample_batch(void *_core, const int16_t *data, size_t frames) { 
    Core *core = (Core*) _core;
    if(core->m_audio) core->m_audio->enqueue(data, frames);
    return frames;
}

void input_poll(void *data) {
    Core *core = (Core*) data;
    /*for(auto elem : core->current_inputs) {
        core->polled_inputs[elem.first] = elem.second;
    }*/
    core->polled_inputs = core->current_inputs;
}

int16_t input_state(void *data, unsigned port, unsigned device, unsigned index, unsigned id) { 
    Core *core = (Core*) data;
    return core->polled_inputs[(input_desc) {
                port,
                device,
                index,
                id
            }];
}

Core::Core(const char *core_path) {
    m_core_handle = dlopen(core_path, RTLD_LAZY | RTLD_LOCAL);

    if(!m_core_handle) {
        fprintf(stderr, "Could not open %s\n", core_path);
        exit(1);
    }

    m_retro_load_game = (bool(*)(const struct retro_game_info*))
        dlsym(m_core_handle, "retro_load_game");
    m_retro_get_system_info = (void (*)(avinfo*))
        dlsym(m_core_handle, "retro_get_system_av_info");
    m_retro_run = (void(*)())
        dlsym(m_core_handle, "retro_run");

    void (*set_env)(void*) = (void (*)(void*)) dlsym(m_core_handle, "retro_set_environment");
    set_env(create_callback((void*) &env_callback, (void*) this));

    void (*init)() = (void (*)()) dlsym(m_core_handle, "retro_init");
    init();

    // Set Video refresh callback
    //void (*set_video_refresh)(void*) = 
    //    (void (*)(void*)) dlsym(m_core_handle, "retro_set_video_refresh");
    //set_video_refresh((void*) video_refresh);

    // Set single audio sample callback
    void (*set_audio_sample)(void*) = 
        (void (*)(void*)) dlsym(m_core_handle, "retro_set_audio_sample");
    set_audio_sample((void*) create_callback((void*) audio_sample, (void*) this));

    // Set batch audio sample callback
    void (*set_batch_audio_sample)(void*) = 
        (void (*)(void*)) dlsym(m_core_handle, "retro_set_audio_sample_batch");
    set_batch_audio_sample((void*) create_callback((void*) audio_sample_batch, (void*) this));

    // Set input poll callback
    void (*set_input_poll)(void*) = 
        (void (*)(void*)) dlsym(m_core_handle, "retro_set_input_poll");
    set_input_poll(create_callback((void*) input_poll, (void*) this));
    
    // Set input fetch callback
    void (*set_input_state)(void*) = 
        (void (*)(void*)) dlsym(m_core_handle, "retro_set_input_state");
    set_input_state(create_callback((void*) input_state, (void*) this));

}

void Core::terminate() {
    if(m_renderer) {
        delete m_renderer;
        m_renderer = NULL;
    }

    if(m_audio) {
        delete m_audio;
        m_audio = nullptr;
    }

    if(m_core_handle) {
        if(context_deinit) context_deinit();
    }
    if(m_core_handle && m_loaded_game) {
        void (*unload)() = (void (*)()) dlsym(m_core_handle, "retro_unload_game");
        if(unload) unload();
        m_loaded_game = false;
    }

    if(m_core_handle) {
        void (*deinit)() = (void (*)()) dlsym(m_core_handle, "retro_deinit");
	if(deinit) deinit();
	dlclose(m_core_handle);
        m_core_handle = NULL;
    }
}

Core::~Core() {
    terminate();
}

void Core::set_render_callback(
        void (*func)(void *userdata, 
            const void *data, 
            unsigned width, 
            unsigned height, 
            size_t pitch), 
        void *userdata) {
    void (*set_video_refresh)(void*) = 
        (void (*)(void*)) dlsym(m_core_handle, "retro_set_video_refresh");
    set_video_refresh(create_callback((void*) func, userdata));
}

bool Core::load_game(const char *game) {
    struct retro_game_info info;
    info.path = game;

    // alternatively: stat & mmap
    
    FILE *f = fopen(game, "r");
    fseek(f, 0, SEEK_END);
    size_t file_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    void *data = malloc(file_size);
    fread(data, 1, file_size, f);
    fclose(f);

    info.data = data;
    info.size = file_size;
    info.meta = NULL;

    bool ret = this->m_retro_load_game(&info);
    if(ret) {
        void (*set_port_device)(int, int) = 
            (void (*)(int, int)) dlsym(m_core_handle, "retro_set_controller_port_device");
        set_port_device(0, RETRO_DEVICE_JOYPAD);
        m_loaded_game = true;
    }
    return ret;
}

avinfo Core::get_info() {
    avinfo info;

    memset(&info, 0, sizeof(avinfo));
    this->m_retro_get_system_info(&info);

    return info;
}

void Core::run_frame() {
    this->m_retro_run();
}

void Core::reset_context() {
    if(context_reset) context_reset();
}

GLuint Core::init_renderer(QOpenGLContext* context, QSurface* surface) {
    printf("[!] Context reset: %p\n", context_reset);
    if(context_reset) { // using hw renderer
        m_renderer = new OpenGLRenderer(context, surface);
    } else {
        m_renderer = new PixelRenderer(context, surface);
    }

    avinfo info = get_info();

    m_audio = new AudioOutput(info.timing.sample_rate);

    return m_renderer->init(this, info.geometry.max_width, info.geometry.max_height);
}

void Core::press(std::string name) {
    auto desc_it = input_descriptors.find(name);
    if(desc_it != input_descriptors.end()) {
        input_desc desc = desc_it->second;

        current_inputs[desc] = 1;
    }
}

void Core::tilt(std::string name, uint16_t value) {
    auto desc_it = input_descriptors.find(name);
    if(desc_it != input_descriptors.end()) {
        input_desc desc = desc_it->second;

        current_inputs[desc] = value;
    }
}

void Core::release(std::string name) {
    auto desc_it = input_descriptors.find(name);
    if(desc_it != input_descriptors.end()) {
        input_desc desc = desc_it->second;

        current_inputs[desc] = 0;
    }
}

void Core::wait_for_frame() {
    run_frame();
}

void Core::run() {
    if (!this->m_stopped) {
        // Already running
        return;
    }
    this->m_stopped = false;
    this->m_render_thread = QThread::create([&]() {
        this->m_renderer->m_context->makeCurrent(this->m_renderer->m_surface);
        avinfo info = get_info();
        double average = 0.0;
        size_t num = 0;
        while (!this->m_stopped) {
            auto start = std::chrono::high_resolution_clock::now();
            run_frame();
            auto end = std::chrono::high_resolution_clock::now();

            // sleep for the remaining time...
            //QThread::usleep((1000000 / info.timing.fps) - std::chrono::duration<double>(end-start).count());

            while(end-start < std::chrono::nanoseconds((int)(1000000000.0 / info.timing.fps)))
                end = std::chrono::high_resolution_clock::now();
        }

        // the callback to render might be off by a few frames because it's
        // rendering asynchronously, TODO: find a better way to wait for that
        // to finish
        QThread::sleep(1);
    });
    this->m_renderer->m_context->moveToThread(this->m_render_thread);
    this->m_render_thread->start();
}

void Core::stop() {
    if (this->m_stopped) {
        // not running
        return;
    }
    this->m_stopped = true;
    this->m_render_thread->quit();
    this->m_render_thread->wait();
    delete this->m_render_thread;
}
