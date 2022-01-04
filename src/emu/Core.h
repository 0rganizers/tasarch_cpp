#pragma once

#include "libretro.h"
#include <vector>
#include <string>
#include <map>
#include <QOpenGLContext>
#include <QSurface>
#include <QThread>

typedef struct input_desc {
    unsigned port;
    unsigned device;
    unsigned index;
    unsigned id;
    bool operator==(const input_desc other) const {
        return port == other.port &&
            device == other.device &&
            index == other.index &&
            id == other.id;
    }
    bool operator<(const input_desc other) const {
        return port < other.port ||
            (port == other.port && (device < other.device ||
            (device == other.device && (index < other.index ||
            (index == other.index && id < other.id)))));
    }
} input_desc;

typedef struct retro_system_av_info avinfo;
class Renderer;

class Core {
    private:
        void *m_core_handle;
        void (*m_retro_run)(void);
        bool (*m_retro_load_game)(const struct retro_game_info *game);
        void (*m_retro_get_system_info)(struct retro_system_av_info *info);

        bool m_loaded_game = false;
        int m_framebuffer = 0; // default == 0

        bool m_stopped = true;
        QThread* m_render_thread;

    public:
        Renderer *m_renderer = NULL;

        Core(const char *core_path);
        ~Core();
        
        std::map<input_desc, uint16_t> current_inputs;
        std::map<input_desc, uint16_t> polled_inputs;

        inline void set_framebuffer(unsigned int fb) { m_framebuffer = fb; }
        inline unsigned int get_framebuffer() { return m_framebuffer; }

        void set_render_callback(
        void (*)(void *userdata, const void *data, unsigned width, unsigned height, size_t pitch),
        void *userdata);

        bool load_game(const char *path);
        void run_frame();
        avinfo get_info();

        void press(std::string button);
        void tilt(std::string button, uint16_t value);
        void release(std::string button);
        void wait_for_frame();

        GLuint init_renderer(QOpenGLContext* context, QSurface* surface);
        void reset_context();

        void terminate();

        void run();
        void stop();

};
