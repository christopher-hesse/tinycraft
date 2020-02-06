// const i32 CUBE_EPISODE_STEPS = 200;
const i32 CUBE_EPISODE_STEPS = 10;

const i32 KEY_LEFT = (1 << 0);
const i32 KEY_UP = (1 << 1);
const i32 KEY_RIGHT = (1 << 2);
const i32 KEY_DOWN = (1 << 3);

struct GameCube : public Game {
    GameCube(u32 seed, i32 _vision_width, i32 _vision_height);
    void init(bool for_agent) override;
    void reset() override;
    void act(i32 action, f32 *rew, u8 *done) override;
    void draw() override;

  private:
    int num_steps = 0;

    f32 yaw_radians = 0;
    f32 pitch_radians = 0;
    std::mt19937 rand_gen;
    i32 vision_width = -1;
    i32 vision_height = -1;

    GLFWwindow *window = nullptr;
    GLuint program = -1;
    GLuint vao = -1;
    GLuint mvp_location = -1;

    bool flip_y = false;
};

static const Vertex cube_vertices[] = {
    // top
    {-0.5f, +0.5f, -0.5f},
    {+0.5f, +0.5f, +0.5f},
    {+0.5f, +0.5f, -0.5f},

    {+0.5f, +0.5f, +0.5f},
    {-0.5f, +0.5f, -0.5f},
    {-0.5f, +0.5f, +0.5f},

    // right
    {+0.5f, -0.5f, -0.5f},
    {+0.5f, +0.5f, +0.5f},
    {+0.5f, -0.5f, +0.5f},

    {+0.5f, +0.5f, +0.5f},
    {+0.5f, -0.5f, -0.5f},
    {+0.5f, +0.5f, -0.5f},

    // back
    {+0.5f, +0.5f, -0.5f},
    {-0.5f, -0.5f, -0.5f},
    {-0.5f, +0.5f, -0.5f},

    {-0.5f, -0.5f, -0.5f},
    {+0.5f, +0.5f, -0.5f},
    {+0.5f, -0.5f, -0.5f},

    // left
    {-0.5f, -0.5f, +0.5f},
    {-0.5f, +0.5f, -0.5f},
    {-0.5f, -0.5f, -0.5f},

    {-0.5f, +0.5f, -0.5f},
    {-0.5f, -0.5f, +0.5f},
    {-0.5f, +0.5f, +0.5f},

    // front
    {+0.5f, -0.5f, +0.5f},
    {-0.5f, +0.5f, +0.5f},
    {-0.5f, -0.5f, +0.5f},

    {-0.5f, +0.5f, +0.5f},
    {+0.5f, -0.5f, +0.5f},
    {+0.5f, +0.5f, +0.5f},

    // bottom
    {-0.5f, -0.5f, -0.5f},
    {+0.5f, -0.5f, +0.5f},
    {-0.5f, -0.5f, +0.5f},

    {+0.5f, -0.5f, +0.5f},
    {-0.5f, -0.5f, -0.5f},
    {+0.5f, -0.5f, -0.5f},
};

static const Vertex cube_normals[] = {
    // top
    {0.0f, 1.0f, 0.0f},
    {0.0f, 1.0f, 0.0f},
    {0.0f, 1.0f, 0.0f},

    {0.0f, 1.0f, 0.0f},
    {0.0f, 1.0f, 0.0f},
    {0.0f, 1.0f, 0.0f},

    // right
    {1.0f, 0.0f, 0.0f},
    {1.0f, 0.0f, 0.0f},
    {1.0f, 0.0f, 0.0f},

    {1.0f, 0.0f, 0.0f},
    {1.0f, 0.0f, 0.0f},
    {1.0f, 0.0f, 0.0f},

    // back
    {0.0f, 0.0f, -1.0f},
    {0.0f, 0.0f, -1.0f},
    {0.0f, 0.0f, -1.0f},

    {0.0f, 0.0f, -1.0f},
    {0.0f, 0.0f, -1.0f},
    {0.0f, 0.0f, -1.0f},

    // left
    {-1.0f, 0.0f, 0.0f},
    {-1.0f, 0.0f, 0.0f},
    {-1.0f, 0.0f, 0.0f},

    {-1.0f, 0.0f, 0.0f},
    {-1.0f, 0.0f, 0.0f},
    {-1.0f, 0.0f, 0.0f},

    // front
    {0.0f, 0.0f, 1.0f},
    {0.0f, 0.0f, 1.0f},
    {0.0f, 0.0f, 1.0f},

    {0.0f, 0.0f, 1.0f},
    {0.0f, 0.0f, 1.0f},
    {0.0f, 0.0f, 1.0f},

    // bottom
    {0.0f, -1.0f, 0.0f},
    {0.0f, -1.0f, 0.0f},
    {0.0f, -1.0f, 0.0f},

    {0.0f, -1.0f, 0.0f},
    {0.0f, -1.0f, 0.0f},
    {0.0f, -1.0f, 0.0f},
};

static const Color cube_colors[] = {
    // top - white
    {1.0f, 1.0f, 1.0f},
    {1.0f, 1.0f, 1.0f},
    {1.0f, 1.0f, 1.0f},

    {1.0f, 1.0f, 1.0f},
    {1.0f, 1.0f, 1.0f},
    {1.0f, 1.0f, 1.0f},

    // right - green
    {0.0f, 163.0f / 255.0f, 104.0f / 255.0f},
    {0.0f, 163.0f / 255.0f, 104.0f / 255.0f},
    {0.0f, 163.0f / 255.0f, 104.0f / 255.0f},

    {0.0f, 163.0f / 255.0f, 104.0f / 255.0f},
    {0.0f, 163.0f / 255.0f, 104.0f / 255.0f},
    {0.0f, 163.0f / 255.0f, 104.0f / 255.0f},

    // back - yellow
    {1.0f, 211.0f / 255.0f, 0.0f},
    {1.0f, 211.0f / 255.0f, 0.0f},
    {1.0f, 211.0f / 255.0f, 0.0f},

    {1.0f, 211.0f / 255.0f, 0.0f},
    {1.0f, 211.0f / 255.0f, 0.0f},
    {1.0f, 211.0f / 255.0f, 0.0f},

    // left - black
    {0.0f, 0.0f, 0.0f},
    {0.0f, 0.0f, 0.0f},
    {0.0f, 0.0f, 0.0f},

    {0.0f, 0.0f, 0.0f},
    {0.0f, 0.0f, 0.0f},
    {0.0f, 0.0f, 0.0f},

    // front - red
    {196.0f / 255.0f, 2.0f / 255.0f, 51.0f / 255.0f},
    {196.0f / 255.0f, 2.0f / 255.0f, 51.0f / 255.0f},
    {196.0f / 255.0f, 2.0f / 255.0f, 51.0f / 255.0f},

    {196.0f / 255.0f, 2.0f / 255.0f, 51.0f / 255.0f},
    {196.0f / 255.0f, 2.0f / 255.0f, 51.0f / 255.0f},
    {196.0f / 255.0f, 2.0f / 255.0f, 51.0f / 255.0f},

    // bottom - blue
    {0.0f, 136.0f / 255.0f, 191.0f / 255.0f},
    {0.0f, 136.0f / 255.0f, 191.0f / 255.0f},
    {0.0f, 136.0f / 255.0f, 191.0f / 255.0f},

    {0.0f, 136.0f / 255.0f, 191.0f / 255.0f},
    {0.0f, 136.0f / 255.0f, 191.0f / 255.0f},
    {0.0f, 136.0f / 255.0f, 191.0f / 255.0f},
};

static const char *cube_vertex_shader_text = R"(
#version 140
uniform mat4 mvp;
in vec3 in_color;
in vec3 in_pos;
out vec3 color;
void main()
{
    gl_Position = mvp * vec4(in_pos, 1.0);
    color = in_color;
}
)";

static const char *cube_fragment_shader_text = R"(
#version 140
in vec3 color;
out vec4 out_fragcolor;
void main()
{
    out_fragcolor = vec4(color, 1.0);
}
)";

GameCube::GameCube(u32 seed, i32 _vision_width, i32 _vision_height) {
    rand_gen.seed(seed);
    vision_width = _vision_width;
    vision_height = _vision_height;
}

void GameCube::init(bool for_agent) {
    this->flip_y = for_agent;
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    if (this->flip_y) {
        glFrontFace(GL_CW);
    }

    glCheck(glGenVertexArrays(1, &this->vao));
    glCheck(glBindVertexArray(this->vao));
    this->program = compileProgram(cube_vertex_shader_text, cube_fragment_shader_text);
    this->mvp_location = getUniformLocation(this->program, "mvp");
    GLint vpos_location = getAttribLocation(this->program, "in_pos");
    GLint vcol_location = getAttribLocation(this->program, "in_color");

    GLuint vertex_buffer;
    glCheck(glGenBuffers(1, &vertex_buffer));
    glCheck(glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer));
    glCheck(glBufferData(GL_ARRAY_BUFFER, sizeof(cube_vertices), cube_vertices, GL_STATIC_DRAW));
    glCheck(glEnableVertexAttribArray(vpos_location));
    glCheck(glVertexAttribPointer(vpos_location, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr));
    GLuint color_buffer;
    glCheck(glGenBuffers(1, &color_buffer));
    glCheck(glBindBuffer(GL_ARRAY_BUFFER, color_buffer));
    glCheck(glBufferData(GL_ARRAY_BUFFER, sizeof(cube_colors), cube_colors, GL_STATIC_DRAW));
    glCheck(glEnableVertexAttribArray(vcol_location));
    glCheck(glVertexAttribPointer(vcol_location, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr));
}

void GameCube::reset() {
    this->num_steps = 0;
    this->pitch_radians = rand_f32(this->rand_gen, -PI / 2, PI / 2);
    this->yaw_radians = rand_f32(this->rand_gen, -PI, PI);
    this->pitch_radians = 0;
    this->yaw_radians = 0;
}

void GameCube::act(i32 a, f32 *rew, u8 *done) {
    if (key_pressed(a, KEY_LEFT)) {
        this->yaw_radians -= 0.1f;
    }
    if (key_pressed(a, KEY_RIGHT)) {
        this->yaw_radians += 0.1f;
    }
    if (key_pressed(a, KEY_UP)) {
        this->pitch_radians += 0.1f;
    }
    if (key_pressed(a, KEY_DOWN)) {
        this->pitch_radians -= 0.1f;
    }
    if (this->yaw_radians > PI) {
        this->yaw_radians -= 2 * PI;
    } else if (this->yaw_radians < -PI) {
        this->yaw_radians += 2 * PI;
    }
    this->pitch_radians = clamp(this->pitch_radians, -PI / 2, PI / 2);
    this->num_steps++;
    // reward is negative distance from correct orientation
    // normalized to be a maximum of 1 on each step
    f32 normalized_distance = (fabs(this->pitch_radians / (PI / 2)) + fabs(this->yaw_radians / PI)) / 2;
    // normalize reward so that the return is in the range 0-1
    *rew = (1 - normalized_distance) / CUBE_EPISODE_STEPS;
    *done = this->num_steps >= CUBE_EPISODE_STEPS;
    if (*done) {
        reset();
    }
}

void GameCube::draw() {
    glCheck(glClearColor(0.2f, 0.2f, 0.2f, 1.0f));
    glCheck(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
    f32 ratio = (f32)(this->vision_width) / (f32)(this->vision_height);
    mat4 proj = perspective(PI / 3, ratio, 0.1f, 100.0f);
    mat4 flip = mat4(1.0f);
    if (this->flip_y) {
        flip = scale(flip, vec3(1.0f, -1.0f, 1.0f));
    }
    mat4 view = lookAt(vec3(0, 0, 1.5f), vec3(0, 0, 0), vec3(0, 1, 0));
    mat4 model = rotate(mat4(1.0f), this->pitch_radians, vec3(1.0f, 0.0f, 0.0f));
    model = model * rotate(mat4(1.0f), this->yaw_radians, vec3(0.0f, 1.0f, 0.0f));
    mat4 mvp = proj * flip * view * model;
    glCheck(glUseProgram(this->program));
    glCheck(glUniformMatrix4fv(this->mvp_location, 1, GL_FALSE, (GLfloat *)&mvp[0]));
    glCheck(glBindVertexArray(this->vao));
    glCheck(glDrawArrays(GL_TRIANGLES, 0, NUM_ELEMS(cube_vertices) * 3));
}