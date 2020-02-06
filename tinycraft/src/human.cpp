const int WINDOW_WIDTH = 640;
const int WINDOW_HEIGHT = 640;

static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

typedef i32 (*get_act_fn)(GLFWwindow *window);

i32 get_act_cube(GLFWwindow *window) {
    i32 act = 0;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        act |= KEY_UP;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        act |= KEY_DOWN;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        act |= KEY_LEFT;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        act |= KEY_RIGHT;
    }
    return act;
}

i32 get_act_world(GLFWwindow *window) {
    i32 act = 0;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        act |= KEY_MOVE_FORWARD;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        act |= KEY_MOVE_BACK;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        act |= KEY_MOVE_LEFT;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        act |= KEY_MOVE_RIGHT;
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
        act |= KEY_MOVE_DOWN;
    }
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        act |= KEY_MOVE_UP;
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
        act |= KEY_TURN_LEFT;
    }
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
        act |= KEY_TURN_RIGHT;
    }
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
        act |= KEY_TURN_UP;
    }
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
        act |= KEY_TURN_DOWN;
    }
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
        act |= KEY_ATTACK;
    }
    if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS) {
        act |= KEY_USE;
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
        act |= KEY_SPRINT;
    }
    return act;
}

int main(int argc, char *argv[]) {
    if (argc == 2) {
        // the debugger doesn't support setting environment variables
        // so use a command line argument instead
        global_resources_dir = argv[1];
    }
    std::string game = "world";
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) {
        exit(EXIT_FAILURE);
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_NATIVE_CONTEXT_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    GLFWwindow *window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "tinycraft", NULL, NULL);
    if (!window) {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    glfwSetKeyCallback(window, key_callback);
    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    glfwSwapInterval(1);
    Game *g = nullptr;
    get_act_fn get_act;
    if (game == "world") {
        g = new GameWorld(0, WINDOW_WIDTH, WINDOW_HEIGHT, 8, true);
        get_act = get_act_world;
    } else if (game == "cube") {
        g = new GameCube(0, WINDOW_WIDTH, WINDOW_HEIGHT);
        get_act = get_act_cube;
    } else {
        fatal("invalid game %s", game.c_str());
    }
    g->init(false);
    g->reset();
    f32 rew;
    u8 done;

    while (!glfwWindowShouldClose(window)) {
        g->act(get_act(window), &rew, &done);
        g->draw();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}
