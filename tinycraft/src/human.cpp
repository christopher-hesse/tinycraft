const int WINDOW_WIDTH = 640;
const int WINDOW_HEIGHT = 640;

static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

typedef Action (*get_act_fn)(GLFWwindow *, f64, f64);

Action get_act_cube(GLFWwindow *window, f64 delta_xpos, f64 delta_ypos) {
    Action act = {};
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        act.keys |= KEY_UP;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        act.keys |= KEY_DOWN;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        act.keys |= KEY_LEFT;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        act.keys |= KEY_RIGHT;
    }
    return act;
}

Action get_act_world(GLFWwindow *window, f64 delta_xpos, f64 delta_ypos) {
    Action act = {};
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        act.keys |= KEY_MOVE_FORWARD;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        act.keys |= KEY_MOVE_BACK;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        act.keys |= KEY_MOVE_LEFT;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        act.keys |= KEY_MOVE_RIGHT;
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
        act.keys |= KEY_MOVE_DOWN;
    }
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        act.keys |= KEY_MOVE_UP;
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
        act.delta_yaw += 0.03f;
    }
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
        act.delta_yaw -= 0.03f;
    }
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
        act.delta_pitch += 0.03f;
    }
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
        act.delta_pitch -= 0.03f;
    }
    act.delta_yaw -= f32(delta_xpos / 1000.0);
    act.delta_pitch -= f32(delta_ypos / 1000.0);
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS|| glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)  {
        act.keys |= KEY_ATTACK;
    }
    if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS|| glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
        act.keys |= KEY_USE;
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
        act.keys |= KEY_SPRINT;
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
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    if (glfwRawMouseMotionSupported()) {
    glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    }
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
    f32 rew = 0.0f;
    u8 done = false;
    f64 prev_xpos = 0.0;
    f64 prev_ypos = 0.0;
        glfwGetCursorPos(window, &prev_xpos, &prev_ypos);

    while (!glfwWindowShouldClose(window)) {
        f64 xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        auto act = get_act(window, xpos-prev_xpos, ypos-prev_ypos);
        prev_xpos = xpos;
        prev_ypos = ypos;
        g->act(act, &rew, &done);
        g->draw();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}
