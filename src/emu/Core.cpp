#include <chrono>
#include <csignal>
#include <cstdarg>
#include <dlfcn.h>

#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <ratio>
#include <string>
#include <vector>
#include <fmt/core.h>
#include <pthread.h>
#include <qobject.h>
#include <sched.h>
#include <spdlog/common.h>
#include <sys/wait.h>
#include <unistd.h>
#include <unordered_map>
#include <QOpenGLContext>
#include <QTimer>
#include <QThread>
#include <iostream>
#include <fstream>

#include "libretro.h"
#include "Core.h"
#include "Callback.h"
#include "Renderer.h"
#include "log/logger.h"
#include "log/logging.h"




static std::unordered_map<std::string, std::string> core_variables = {};
static std::unordered_map<std::string, input_desc> input_descriptors = {};
static retro_hw_context_reset_t context_reset = NULL;
static retro_hw_context_reset_t context_deinit = NULL;
static std::shared_ptr<tasarch::log::Logger> logger = tasarch::log::get("core");

static GLuint get_framebuf(void *_core) {
    Core *core = (Core*) _core;
    return core->get_framebuffer();
}

static bool set_rumble_state(unsigned port, enum retro_rumble_effect effect, uint16_t strength) {
    return true;
}

static void log_func(enum retro_log_level level, const char *fmt, ...) {
    spdlog::level::level_enum lvl = spdlog::level::info;
    char msg_buf[0x4000];
    
    // logger->info("Logging with level {}", level);
    
    va_list args;
    va_start(args, fmt);
    vsnprintf(msg_buf, 0x4000-1, fmt, args);
    va_end(args);

    switch (level) {
    case RETRO_LOG_DEBUG:
        lvl = spdlog::level::debug;
        logger->debug(msg_buf);
        break;
    case RETRO_LOG_WARN:
        lvl = spdlog::level::warn;
        logger->warn(msg_buf);
        break;
    case RETRO_LOG_ERROR:
        lvl = spdlog::level::err;
        logger->error(msg_buf);
        break;
    default:
        logger->info(msg_buf);
        break;
    }

    // logger->info("Finished logging");
    
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
        logger->info("==> Pixel fmt: {}", *fmt);
        break;
    }
    case RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS:
    {
        const struct retro_input_descriptor *inputs = 
            (const struct retro_input_descriptor *) data;
        while(inputs->description) {
            logger->info("Input description: {}", inputs->description);
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
        logger->warn("HW RENDER: {}", hw->context_type);
        if(hw->context_type == RETRO_HW_CONTEXT_OPENGL_CORE) {
            logger->info("Doing the opengl context!");
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
        logger->info("Getting variable: {} => {}", vars->key, core_variables[vars->key]);
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
            logger->info("setting Key: {}, Value: {}", vars->key, vars->value);
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
        logger->warn("Not handled: {}", cmd);
        return false;
    }

    // assume we handled the call correctly
    return true;
}

void audio_sample(void *_core, int16_t left, int16_t right) {
    Core *core = (Core*) _core;
    //core->DispatchToMainThread([=]{
        if(core->m_audio) core->m_audio->enqueue(left, right);
    //});
}

size_t audio_sample_batch(void *_core, const int16_t *data, size_t frames) { 
    Core *core = (Core*) _core;
    //core->DispatchToMainThread([=]{
        if(core->m_audio) core->m_audio->enqueue(data, frames);
    //});
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
    logger = tasarch::log::get("core");
    m_core_handle = dlopen(core_path, RTLD_LAZY | RTLD_LOCAL);

    if(!m_core_handle) {
        logger->critical("Could not open {}", core_path);
        exit(1);
    }

    m_retro_load_game = (bool(*)(const struct retro_game_info*))
        dlsym(m_core_handle, "retro_load_game");
    m_retro_get_system_info = (void (*)(avinfo*))
        dlsym(m_core_handle, "retro_get_system_av_info");
    m_retro_run = (void(*)())
        dlsym(m_core_handle, "retro_run");
    m_retro_serialize_size = (size_t(*)(void))
		dlsym(m_core_handle, "retro_serialize_size");
	m_retro_serialize = (bool(*)(void *, size_t))
		dlsym(m_core_handle, "retro_serialize");
	m_retro_unserialize = (bool(*)(const void*, size_t))
		dlsym(m_core_handle, "retro_unserialize");

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
    logger->warn("Context reset: {:p}", (void*)context_reset);
    if(context_reset) { // using hw renderer
        m_renderer = new OpenGLRenderer(context, surface);
    } else {
        m_renderer = new PixelRenderer(context, surface);
    }

    avinfo info = get_info();


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

[[gnu::noinline]]
void no_inline_serialize(Core* core, void* ser_buf, size_t ser_size) {
    core->m_retro_serialize(ser_buf, ser_size);
}

void Core::run() {
    if (!this->m_stopped) {
        // Already running
        return;
    }
    
    avinfo info = get_info();
    m_audio = new AudioOutput(info.timing.sample_rate);

    this->m_stopped = false;
    this->m_render_thread = QThread::create([&]() {
        this->m_renderer->m_context->makeCurrent(this->m_renderer->m_surface);
        avinfo info = get_info();
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        unsigned int cpu = 0;
        unsigned int node = 0;
        getcpu(&cpu, &node);
        // CPU_SET(cpu, &cpuset);
        for (size_t i = 0; i < 2; i++) {
            CPU_SET(i, &cpuset);

        }
        int rc = pthread_setaffinity_np((pthread_t)this->m_render_thread->currentThreadId(), sizeof(cpuset), &cpuset);
        if (rc != 0) {
            logger->critical("failed to set thread affinity!: {}", rc);
        } else {
            logger->critical("Set affinity ok.");
        }
        //m_audio = new AudioOutput(info.timing.sample_rate);
        double average = 0.0;
        size_t num = 0;
        auto prev_loop_start = std::chrono::high_resolution_clock::now();
        double frame_time_s = 1 / info.timing.fps;
        auto frame_time_dur = std::chrono::duration<double>(frame_time_s);
        using sleep_duration = std::chrono::duration<unsigned long, std::micro>;
        using display_duration = std::chrono::duration<double, std::milli>;
        auto frame_time = std::chrono::duration_cast<sleep_duration>(frame_time_dur);
        logger->info("frame time in us: {}", frame_time.count());
        size_t ser_size = 0;
        void* ser_buf = NULL;
        sigignore( SIGCHLD );
        size_t frames = 0;
        pid_t prev_child = 0;
        while (!this->m_stopped) {
            auto loop_start = std::chrono::high_resolution_clock::now();
            run_frame();
            auto run_end = std::chrono::high_resolution_clock::now();
            frames++;

            
            //QThread::usleep((1000000 / info.timing.fps) - std::chrono::duration<double>(end-start).count());
            
            // while(end-start < std::chrono::nanoseconds((int)(1000000000.0 / info.timing.fps))) {
            //     end = std::chrono::high_resolution_clock::now();
            // }
            auto loop_took = std::chrono::duration_cast<display_duration>(loop_start - prev_loop_start);
            auto run_took = std::chrono::duration_cast<display_duration>(run_end - loop_start);
            timing_loop.record(loop_took.count());
            timing_run.record(run_took.count());
            
            if (frames > 60) {

                if (frames % 60 == 0) {
                    if (ser_size == 0) {
                        ser_size = this->m_retro_serialize_size();
                        logger->info("Serialize size: {}", ser_size);
                        ser_buf = malloc(2*ser_size);
                        logger->info("ser_buf @ {:p}", ser_buf);
                    }
                    auto ser_begin = std::chrono::high_resolution_clock::now();
                    // if (prev_child != 0) {
                    //     kill(prev_child, SIGKILL);
                    //     // int status;
                    //     // waitpid(prev_child, &status, 0);
                    //     // logger->info("child exited with {}", status);
                    // }
                    // logger->info("do ser");
                    
                    no_inline_serialize(this, ser_buf, ser_size);
                    
                    // logger->info("do ser fin");
                    // if (!is_parent) {
                    //     logger->info("We are in child with pid {}, writing state to disk...", getpid());
                    //     std::string filename = fmt::format("states/frame_{:04}.sav", frames);
                    //     std::ofstream myfile;
                    //     myfile.open (filename, std::ios::out | std::ios::trunc | std::ios::binary);
                    //     myfile.write((char*)ser_buf, ser_size);
                    //     myfile.close();
                    //     logger->info("killing my self");
                    //     kill(getpid(), SIGKILL);
                    //     exit(0);
                    // } else {
                    //     prev_child = *(pid_t*)ser_buf;
                    //     logger->info("Spawned child: {}", prev_child);
                    // }
                    auto ser_end = std::chrono::high_resolution_clock::now();
                    auto ser_took = std::chrono::duration_cast<display_duration>(ser_end - ser_begin);
                    timing_ser.record(ser_took.count());
                }

                if (frames % 60 == 0) {
                    // fprintf(stderr, "\033[J");
                    logger->info("Current tid: {}", gettid());
                    fprintf(stderr, "%-10s| %5s / %5s / %5s / %5s\n", "name", "min", "max", "avg", "val");
                    fprintf(stderr, "---------------------------------------------\n");
                    // fprintf(stderr, "%-10s|", "loop");
                    // timing_loop.output(stderr);
                    // puts("");
                    // fprintf(stderr, "%-10s|", "run");
                    // timing_run.output(stderr);
                    // puts("");
                    
                    fprintf(stderr, "%-10s| %s\n", "loop", timing_loop.output().c_str());
                    fprintf(stderr, "%-10s| %s\n", "run", timing_run.output().c_str());
                    fprintf(stderr, "%-10s| %s\n", "ser", timing_ser.output().c_str());
                    // fprintf(stderr, "\033[A\033[A\033[A\033[A\033[A");
                }

            }

            auto final = std::chrono::high_resolution_clock::now();
            auto diff = final - loop_start;
            auto diff_sl = std::chrono::duration_cast<sleep_duration>(diff);
            auto rem = frame_time - diff_sl;
            // sleep for the remaining time..
            if (diff_sl < frame_time) {
                // logger->info("sleeping for {} us", rem.count());
                QThread::usleep(rem.count());
            }

            prev_loop_start = loop_start;

            
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

void Core::DispatchToMainThread(std::function<void()> callback)
{
    if(!m_main_thread) return;

    QTimer* timer = new QTimer();
    timer->moveToThread(m_main_thread);
    timer->setSingleShot(true);
    QObject::connect(timer, &QTimer::timeout, [=]()
        {
            callback();
            timer->deleteLater();
        });
    QMetaObject::invokeMethod(timer, "start", Qt::QueuedConnection, Q_ARG(int, 0));
}
