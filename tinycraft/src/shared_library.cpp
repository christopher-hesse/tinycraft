const int BYTES_PER_PIXEL = 3;

void load_library() {
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) {
        exit(EXIT_FAILURE);
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#if !defined(__GNUC__)
    // for some reason setting this means that no EGL configs are matched
    glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_FALSE);
#endif
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_NATIVE_CONTEXT_API);
}

void unload_library() {
    glfwTerminate();
}

// shared library load/unload handlers
#ifdef __GNUC__
int __attribute__((constructor)) library_load(void) {
    load_library();
    return 0;
}

int __attribute__((destructor)) library_unload(void) {
    unload_library();
    return 0;
}
#else
#include <windows.h>

BOOL WINAPI DllMain(
    HINSTANCE hinstDLL,
    DWORD fdwReason,
    LPVOID lpReserved)

{
    switch (fdwReason) {
    case DLL_PROCESS_ATTACH: {
        load_library();
        break;
    }
    case DLL_PROCESS_DETACH: {
        unload_library();
        break;
    }
    }
    return TRUE;
}
#endif

std::vector<std::vector<void *>> convert_bufs(std::vector<struct libenv_space> spaces, int num_envs, void **buf) {
    std::vector<std::vector<void *>> result;
    for (int idx = 0; idx < num_envs; idx++) {
        std::vector<void *> ptrs;
        for (size_t i = 0; i < spaces.size(); i++) {
            ptrs.push_back(buf[i * num_envs + idx]);
        }
        result.push_back(ptrs);
    }
    return result;
}

struct EnvGame {
    Game *game = nullptr;
    GLFWwindow *window = nullptr;
    std::thread *worker_thread = nullptr;
    std::mutex *mtx = nullptr;
    std::condition_variable *cv = nullptr;

    bool running = false;
    bool should_reset = false;
    bool should_terminate = false;

    GLuint fbo = 0;
    GLuint depth_rbo = 0;
    GLuint color_rbo = 0;

    std::vector<GLuint> pbo_buffer;
    std::vector<f32> rew_buffer;
    std::vector<u8> done_buffer;
    std::vector<Observation> obs_buffer;

    std::vector<void *> ob;
    std::vector<void *> act;
    f32 *rew = nullptr;
    u8 *done = nullptr;

    int read_offset = 0;
    int write_offset = 0;
    int discard_acts = 0;
};

struct Env {
    int num_envs = 0;
    int vision_width = 0;
    int vision_height = 0;
    int num_buffered_steps = 0;
    int num_action_repeats = 0;

    std::vector<struct libenv_space> observation_spaces;
    std::vector<struct libenv_space> action_spaces;

    std::vector<EnvGame *> env_games;
};

void wait_for_game_threads(Env *env) {
    for (int idx = 0; idx < env->num_envs; idx++) {
        {
            auto eg = env->env_games[idx];
            std::unique_lock<std::mutex> lock(*eg->mtx);
            while (eg->running) {
                eg->cv->wait(lock);
            }
        }
    }
}

void start_pixel_read(Env *e, GLuint pbo) {
    glCheck(glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo));
    glCheck(glReadPixels(0, 0, e->vision_width, e->vision_height, GL_RGB, GL_UNSIGNED_BYTE, nullptr));
}

void finish_pixel_read(Env *e, void *buf, GLuint pbo) {
    // this is what you'd do without PBOs
    // glCheck(glReadPixels(0, 0, e->vision_width, e->vision_height, GL_RGB, GL_UNSIGNED_BYTE, buf));
    glCheck(glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo));
    GLubyte *src = (GLubyte *)glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
    fassert(src != nullptr);
    memcpy(buf, src, e->vision_width * e->vision_height * BYTES_PER_PIXEL);
    glCheck(glUnmapBuffer(GL_PIXEL_PACK_BUFFER));
}

void game_worker(Env *e, EnvGame *eg) {
    bool first_loop = true;
    glfwMakeContextCurrent(eg->window);
    glCheck(glBindFramebuffer(GL_FRAMEBUFFER, eg->fbo));
    while (true) {
        std::unique_lock<std::mutex> lock(*eg->mtx);
        while (!eg->running) {
            eg->cv->wait(lock);
        }

        if (eg->should_terminate) {
            glfwMakeContextCurrent(NULL);
            eg->running = false;
            eg->cv->notify_all();
            return;
        }

        if (eg->should_reset) {
            eg->should_reset = false;
            eg->game->reset();
        } else {
            Action act = {};
            if (eg->discard_acts > 0) {
                eg->discard_acts--;
            } else {
                act.delta_yaw = clamp(*(f32 *)(eg->act[0]), -1.0f, 1.0f) * 0.1f;
                act.delta_pitch = clamp(*(f32 *)(eg->act[1]), -1.0f, 1.0f) * 0.1f;
                act.keys = *(i32 *)(eg->act[2]);
            }
            for (int i = 0; i < e->num_action_repeats; i++) {
                Observation obs = eg->game->act(act, &eg->rew_buffer[eg->write_offset], &eg->done_buffer[eg->write_offset]);
                eg->obs_buffer[eg->write_offset] = obs;
                if (eg->done_buffer[eg->write_offset]) {
                    // don't repeat actions across episode boundaries
                    break;
                }
            }
            if (eg->done_buffer[eg->write_offset]) {
                // if we are buffering steps and the episode ends, we will
                // end up submitting actions based on the previous observation
                // discard those actions
                eg->discard_acts = e->num_buffered_steps - 1;
                // also don't repeat actions across episode boundaries
            }
        }

        eg->game->draw();
        start_pixel_read(e, eg->pbo_buffer[eg->write_offset]);
        // on the first time through this loop, all of the pixel buffer objects
        // will be unused, so they will contain a blank screen
        // in order to have them not be blank, copy the contents of the first
        // rendering to all of the PBOs
        // this means the first few frames of the environment will all be the
        // initial render, but at least they are not blank
        if (first_loop) {
            first_loop = false;
            for (int i = 1; i < e->num_buffered_steps; i++) {
                eg->write_offset = (eg->write_offset + 1) % e->num_buffered_steps;
                start_pixel_read(e, eg->pbo_buffer[eg->write_offset]);
            }
        }
        finish_pixel_read(e, eg->ob[0], eg->pbo_buffer[eg->read_offset]);
        auto obs = eg->obs_buffer[eg->read_offset];
        *(f32 *)(eg->ob[1]) = obs.compass_heading;
        *eg->rew = eg->rew_buffer[eg->read_offset];
        *eg->done = eg->done_buffer[eg->read_offset];

        eg->write_offset = (eg->write_offset + 1) % e->num_buffered_steps;
        eg->read_offset = (eg->read_offset + 1) % e->num_buffered_steps;

        eg->running = false;
        eg->cv->notify_all();
    }
}

libenv_venv *libenv_make(int num_envs, const struct libenv_options options) {
    i32 vision_width = 512;
    i32 vision_height = 512;
    i32 num_buffered_steps = 1;
    i32 num_action_repeats = 1;
    i32 seed = -1;
    std::string game = "";

    auto vo = EnvOptions(options);
    vo.consume_int("vision_width", &vision_width);
    vo.consume_int("vision_height", &vision_height);
    vo.consume_int("num_buffered_steps", &num_buffered_steps);
    vo.consume_int("num_action_repeats", &num_action_repeats);
    fassert(num_buffered_steps > 0);
    vo.consume_int("seed", &seed);
    vo.consume_string("game", &game);

    auto env = new Env();
    env->vision_width = vision_width;
    env->vision_height = vision_height;
    env->num_buffered_steps = num_buffered_steps;
    env->num_action_repeats = num_action_repeats;
    env->num_envs = num_envs;
    env->env_games.resize(num_envs);

    i32 num_actions = 512;

    std::mt19937 seed_gen;
    if (seed == -1) {
        seed_gen = seeded_generator();
    } else {
        seed_gen.seed(seed);
    }

    for (int idx = 0; idx < num_envs; idx++) {
        GLFWwindow *w = glfwCreateWindow(env->vision_width, env->vision_height, "tinycraft", NULL, NULL);
        if (!w) {
            glfwTerminate();
            exit(EXIT_FAILURE);
        }
        glfwMakeContextCurrent(w);
        gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
        glCheck(glViewport(0, 0, env->vision_width, env->vision_height));
        Game *g = nullptr;
        if (game == "world") {
            i32 world_chunks = 8;
            vo.consume_int("world_chunks", &world_chunks);
            vo.ensure_empty();
            g = new GameWorld(seed_gen(), vision_width, vision_height, world_chunks, false);
        } else if (game == "cube") {
            vo.ensure_empty();
            g = new GameCube(seed_gen(), vision_width, vision_height);
        } else {
            fatal("invalid game %s", game.c_str());
        }
        g->init(true);
        auto eg = new EnvGame();
        eg->cv = new std::condition_variable();
        eg->mtx = new std::mutex();
        eg->game = g;
        eg->window = w;
        eg->pbo_buffer.resize(num_buffered_steps);
        eg->rew_buffer.resize(num_buffered_steps);
        eg->done_buffer.resize(num_buffered_steps);
        eg->obs_buffer.resize(num_buffered_steps);
        // setup framebuffer and pixel buffer objects
        {
            glCheck(glGenFramebuffers(1, &eg->fbo));
            glCheck(glGenRenderbuffers(1, &eg->depth_rbo));
            glCheck(glGenRenderbuffers(1, &eg->color_rbo));

            glCheck(glBindFramebuffer(GL_FRAMEBUFFER, eg->fbo));
            glCheck(glBindRenderbuffer(GL_RENDERBUFFER, eg->depth_rbo));
            glCheck(glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, vision_width, vision_height));
            glCheck(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, eg->depth_rbo));
            glCheck(glBindRenderbuffer(GL_RENDERBUFFER, 0));
            glCheck(glBindRenderbuffer(GL_RENDERBUFFER, eg->color_rbo));
            glCheck(glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, vision_width, vision_height));
            glCheck(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, eg->color_rbo));
            glCheck(glBindRenderbuffer(GL_RENDERBUFFER, 0));
            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
                fatal("incomplete framebuffer");
            }
            glCheck(glBindFramebuffer(GL_FRAMEBUFFER, 0));

            glCheck(glGenBuffers(env->num_buffered_steps, &eg->pbo_buffer[0]));
            for (int offset = 0; offset < env->num_buffered_steps; offset++) {
                glCheck(glBindBuffer(GL_PIXEL_PACK_BUFFER, eg->pbo_buffer[offset]));
                glCheck(glBufferData(GL_PIXEL_PACK_BUFFER, vision_width * vision_height * BYTES_PER_PIXEL, 0, GL_STREAM_READ));
                glCheck(glBindBuffer(GL_PIXEL_PACK_BUFFER, 0));
            }
        }
        glfwMakeContextCurrent(NULL);

        eg->worker_thread = new std::thread(game_worker, env, eg);
        env->env_games[idx] = eg;
    }

    {
        struct libenv_space s;
        strcpy(s.name, "rgb");
        s.shape[0] = vision_height;
        s.shape[1] = vision_width;
        s.shape[2] = 3;
        s.ndim = 3;
        s.type = LIBENV_SPACE_TYPE_BOX;
        s.dtype = LIBENV_DTYPE_UINT8;
        s.low.uint8 = 0;
        s.high.uint8 = 255;
        env->observation_spaces.push_back(s);
    }

    {
        struct libenv_space s;
        strcpy(s.name, "compass_heading");
        s.shape[0] = 1;
        s.ndim = 1;
        s.type = LIBENV_SPACE_TYPE_BOX;
        s.dtype = LIBENV_DTYPE_FLOAT32;
        s.low.uint8 = -1.0f;
        s.high.uint8 = 1.0f;
        env->observation_spaces.push_back(s);
    }

    {
        struct libenv_space s;
        strcpy(s.name, "delta_yaw");
        s.shape[0] = 1;
        s.ndim = 1;
        s.type = LIBENV_SPACE_TYPE_BOX;
        s.dtype = LIBENV_DTYPE_FLOAT32;
        s.low.float32 = -1.0f;
        s.high.float32 = 1.0f;
        env->action_spaces.push_back(s);
    }

    {
        struct libenv_space s;
        strcpy(s.name, "delta_pitch");
        s.shape[0] = 1;
        s.ndim = 1;
        s.type = LIBENV_SPACE_TYPE_BOX;
        s.dtype = LIBENV_DTYPE_FLOAT32;
        s.low.float32 = -1.0f;
        s.high.float32 = 1.0f;
        env->action_spaces.push_back(s);
    }

    {
        struct libenv_space s;
        strcpy(s.name, "keys");
        s.shape[0] = 1;
        s.ndim = 1;
        s.type = LIBENV_SPACE_TYPE_DISCRETE;
        s.dtype = LIBENV_DTYPE_INT32;
        s.low.int32 = 0;
        s.high.int32 = num_actions - 1;
        env->action_spaces.push_back(s);
    }
    return env;
}

int libenv_get_spaces(libenv_venv *handle, enum libenv_spaces_name name, struct libenv_space *out_spaces) {
    auto env = static_cast<Env *>(handle);
    std::vector<struct libenv_space> spaces;
    if (name == LIBENV_SPACES_OBSERVATION) {
        spaces = env->observation_spaces;
    } else if (name == LIBENV_SPACES_ACTION) {
        spaces = env->action_spaces;
    }

    if (out_spaces != NULL) {
        for (size_t i = 0; i < spaces.size(); i++) {
            out_spaces[i] = spaces[i];
        }
    }
    return (int)(spaces.size());
}

void libenv_reset(libenv_venv *handle, struct libenv_step *step) {
    auto env = static_cast<Env *>(handle);
    wait_for_game_threads(env);

    auto obs = convert_bufs(env->observation_spaces, env->num_envs, step->obs);
    auto rews = step->rews;
    auto dones = step->dones;
    for (int idx = 0; idx < env->num_envs; idx++) {
        auto eg = env->env_games[idx];
        std::unique_lock<std::mutex> lock(*eg->mtx);
        eg->ob = obs[idx];
        eg->rew = &rews[idx];
        eg->done = &dones[idx];
        eg->should_reset = true;
        eg->running = true;
        eg->cv->notify_all();
    }
    wait_for_game_threads(env);
}

void libenv_step_async(libenv_venv *handle, void **acts, struct libenv_step *step) {
    auto env = static_cast<Env *>(handle);
    wait_for_game_threads(env);

    glfwPollEvents();
    auto converted_obs = convert_bufs(env->observation_spaces, env->num_envs, step->obs);
    auto converted_acts = convert_bufs(env->action_spaces, env->num_envs, acts);
    auto rews = step->rews;
    auto dones = step->dones;
    for (int idx = 0; idx < env->num_envs; idx++) {
        auto eg = env->env_games[idx];
        std::unique_lock<std::mutex> lock(*eg->mtx);
        eg->ob = converted_obs[idx];
        eg->act = converted_acts[idx];
        eg->rew = &rews[idx];
        eg->done = &dones[idx];
        eg->running = true;
        eg->cv->notify_all();
    }
}

void libenv_step_wait(libenv_venv *handle) {
    auto env = static_cast<Env *>(handle);
    wait_for_game_threads(env);
}

bool libenv_render(libenv_venv *UNUSED(handle), const char *UNUSED(mode), void **UNUSED(frames)) {
    return false;
}

void libenv_close(libenv_venv *handle) {
    auto env = static_cast<Env *>(handle);
    wait_for_game_threads(env);
    for (int idx = 0; idx < env->num_envs; idx++) {
        auto eg = env->env_games[idx];
        {
            std::unique_lock<std::mutex> lock(*eg->mtx);
            eg->should_terminate = true;
            eg->running = true;
            eg->cv->notify_all();
        }
        eg->worker_thread->join();
        glfwMakeContextCurrent(eg->window);
        glDeleteFramebuffers(1, &eg->fbo);
        glDeleteRenderbuffers(1, &eg->depth_rbo);
        glDeleteRenderbuffers(1, &eg->color_rbo);
        glDeleteRenderbuffers(env->num_buffered_steps, &eg->pbo_buffer[0]);
        glfwDestroyWindow(eg->window);
        glfwMakeContextCurrent(NULL);
        delete eg->game;
        delete eg->mtx;
        delete eg->cv;
        delete eg->worker_thread;
        delete eg;
    }
    delete env;
}