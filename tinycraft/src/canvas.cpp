const int COMMAND_BYTES_PER_VERTEX = sizeof(f32) * 8 + 1;
const int COMMAND_BUFFER_SIZE_BYTES = 1048576;
const int COMMAND_BYTES_PER_CHAR = COMMAND_BYTES_PER_VERTEX * VERTICES_PER_QUAD;
const vec4 PRINT_TEXT_COLOR = vec4(1.0f, 1.0f, 1.0f, 1.0f);
const vec4 PRINT_SHADOW_COLOR = vec4(0.2f, 0.2f, 0.2f, 0.5f);
const int PRINT_PAD = 8;

struct ShaderCanvas {
    GLint program = -1;
    GLint uniform_resolution = -1;
    GLint uniform_font_texture = -1;
    GLint attrib_xy = -1;
    GLint attrib_st = -1;
    GLint attrib_color = -1;
    GLint attrib_use_texture = -1;
};

static const char *canvas_shader_vertex = R"(
#version 140

uniform ivec2 resolution;
in vec2 xy;
in vec2 st;
in vec4 color;
in uint use_texture;
out vec2 frag_st;
out vec4 frag_color;
flat out uint frag_use_texture;

void main() {
    gl_Position = vec4((xy / vec2(resolution) - 0.5) * 2.0, 0, 1);
    frag_st = st;
    frag_use_texture = use_texture;
    frag_color = color;
}
)";

static const char *canvas_shader_fragment = R"(
#version 140

uniform sampler2D font_texture;
in vec2 frag_st;
in vec4 frag_color;
flat in uint frag_use_texture;
out vec4 out_fragcolor;

void main() {
    vec4 mask; 
    if (frag_use_texture == 1u) {
        mask = texture(font_texture, frag_st);
    } else {
        mask = vec4(1.0, 1.0, 1.0, 1.0);
    }
    out_fragcolor = frag_color * mask;
}
)";

struct Canvas {
    ShaderCanvas shader;
    ivec2 resolution = ivec2(0);
    vec4 color = vec4(0.0f);
    GLuint font_texture = 0;
    GLuint vao = 0;
    GLuint buffer = 0;
    u8 command_buffer[COMMAND_BUFFER_SIZE_BYTES] = {};
    int command_buffer_offset = 0;
    int num_vertices = 0;
    Font font = {};
    int print_x = 0;
    int print_y = 0;
    char print_buffer[65536] = {};
};

ShaderCanvas compile_canvas_shader() {
    ShaderCanvas s;
    s.program = compileProgram(canvas_shader_vertex, canvas_shader_fragment);
    s.uniform_resolution = get_uniform_location(s.program, "resolution");
    s.uniform_font_texture = get_uniform_location(s.program, "font_texture");
    s.attrib_xy = get_attrib_location(s.program, "xy");
    s.attrib_st = get_attrib_location(s.program, "st");
    s.attrib_color = get_attrib_location(s.program, "color");
    s.attrib_use_texture = get_attrib_location(s.program, "use_texture");
    return s;
}

void canvas_reset(Canvas *c) {
    c->print_x = PRINT_PAD;
    c->print_y = c->resolution.y - PRINT_PAD - c->font.height;
    c->num_vertices = 0;
    c->command_buffer_offset = 0;
}

Canvas *canvas_create(int res_x, int res_y, Font font) {
    auto c = new Canvas();
    c->font = font;
    c->shader = compile_canvas_shader();
    c->resolution = ivec2(res_x, res_y);

    glGenTextures(1, &c->font_texture);
    glBindTexture(GL_TEXTURE_2D, c->font_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, font.width * font.chars, font.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, font.data);

    glCheck(glGenVertexArrays(1, &c->vao));
    glCheck(glBindVertexArray(c->vao));

    glCheck(glGenBuffers(1, &c->buffer));
    glCheck(glBindBuffer(GL_ARRAY_BUFFER, c->buffer));

    glCheck(glEnableVertexAttribArray(c->shader.attrib_xy));
    glCheck(glVertexAttribPointer(c->shader.attrib_xy, 2, GL_FLOAT, GL_FALSE, COMMAND_BYTES_PER_VERTEX, BUFFER_OFFSET(0)));
    glCheck(glEnableVertexAttribArray(c->shader.attrib_st));
    glCheck(glVertexAttribPointer(c->shader.attrib_st, 2, GL_FLOAT, GL_FALSE, COMMAND_BYTES_PER_VERTEX, BUFFER_OFFSET(sizeof(f32) * 2)));
    glCheck(glEnableVertexAttribArray(c->shader.attrib_color));
    glCheck(glVertexAttribPointer(c->shader.attrib_color, 4, GL_FLOAT, GL_FALSE, COMMAND_BYTES_PER_VERTEX, BUFFER_OFFSET(sizeof(f32) * 4)));
    glCheck(glEnableVertexAttribArray(c->shader.attrib_use_texture));
    glCheck(glVertexAttribIPointer(c->shader.attrib_use_texture, 1, GL_BYTE, COMMAND_BYTES_PER_VERTEX, BUFFER_OFFSET(sizeof(f32) * 8)));

    canvas_reset(c);

    return c;
}

void canvas_draw_char(Canvas *c, char ch, int x, int y, vec4 color) {
    f32 xleft = f32(x);
    f32 xright = f32(x + c->font.width);
    f32 ybottom = f32(y);
    f32 ytop = f32(y + c->font.height);

    f32 soffset = f32(ch * c->font.width);
    f32 sleft = (soffset + 0.5f) / f32(c->font.width * c->font.chars);
    f32 sright = (soffset + c->font.width - 1 + 0.5f) / (f32)(c->font.width * c->font.chars);
    f32 tbottom = (0.0f + 0.5f) / (f32)c->font.height;
    f32 ttop = (c->font.height - 1 + 0.5f) / (f32)c->font.height;

    vec2 xy_data[] = {
        // bottom left
        {xleft, ybottom},
        // top left
        {xleft, ytop},
        // bottom right
        {xright, ybottom},
        // bottom right
        {xright, ybottom},
        // top left
        {xleft, ytop},
        // top right
        {xright, ytop},
    };

    vec2 st_data[] = {
        // bottom left
        {sleft, tbottom},
        // top left
        {sleft, ttop},
        // bottom right
        {sright, tbottom},
        // bottom right
        {sright, tbottom},
        // top left
        {sleft, ttop},
        // top right
        {sright, ttop},
    };

    auto b = Buffer(c->command_buffer + c->command_buffer_offset, COMMAND_BUFFER_SIZE_BYTES - c->command_buffer_offset);
    for (int vertex_idx = 0; vertex_idx < VERTICES_PER_QUAD; vertex_idx++) {
        // xy
        b.write_f32(xy_data[vertex_idx].x);
        b.write_f32(xy_data[vertex_idx].y);

        // st
        b.write_f32(st_data[vertex_idx].s);
        b.write_f32(st_data[vertex_idx].t);

        // color
        b.write_f32(color.r);
        b.write_f32(color.g);
        b.write_f32(color.b);
        b.write_f32(color.a);

        // use_texture
        b.write_u8(true);
    }
    fassert(b.bytes_written() == COMMAND_BYTES_PER_CHAR);
    c->num_vertices += VERTICES_PER_QUAD;
    c->command_buffer_offset += COMMAND_BYTES_PER_CHAR;
}

void canvas_draw_text(Canvas *c, char *text, int x, int y, vec4 color) {
    size_t text_length = strlen(text);

    for (size_t char_idx = 0; char_idx < text_length; char_idx++) {
        canvas_draw_char(c, text[char_idx], x, y, color);
        x += c->font.width;
    }
}

#ifdef __GNUC__
// enable format string checking
//__format__(archetype, string-index, first-to-check)
// avoid warnings about "format string is not a string literal"
__attribute__((__format__(__printf__, 2, 3)))
#endif
void
canvas_print(Canvas *c, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int n = vsprintf(c->print_buffer, fmt, args);
    va_end(args);
    fassert(n >= 0);
    for (int i = 0; i < n; i++) {
        char ch = c->print_buffer[i];
        if (ch == '\n') {
            c->print_x = PRINT_PAD;
            c->print_y -= c->font.height;
        } else {
            canvas_draw_char(c, ch, c->print_x + 1, c->print_y - 1, PRINT_SHADOW_COLOR);
            canvas_draw_char(c, ch, c->print_x, c->print_y, PRINT_TEXT_COLOR);
            c->print_x += c->font.width;
        }
    }
}

void canvas_render(Canvas *c) {
    auto s = c->shader;

    glCheck(glDisable(GL_DEPTH_TEST));
    glCheck(glDisable(GL_CULL_FACE));

    glCheck(glEnable(GL_BLEND));
    glCheck(glBlendEquation(GL_FUNC_ADD));
    glCheck(glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA));

    glCheck(glUseProgram(s.program));
    glCheck(glUniform2i(s.uniform_resolution, c->resolution[0], c->resolution[1]));
    glCheck(glUniform1i(s.uniform_font_texture, 0));
    glCheck(glActiveTexture(GL_TEXTURE0));
    glCheck(glBindTexture(GL_TEXTURE_2D, c->font_texture));

    glCheck(glBindBuffer(GL_ARRAY_BUFFER, c->buffer));
    glCheck(glBufferData(GL_ARRAY_BUFFER, c->command_buffer_offset, c->command_buffer, GL_DYNAMIC_DRAW));

    glCheck(glBindVertexArray(c->vao));
    glCheck(glDrawArrays(GL_TRIANGLES, 0, c->num_vertices));

    canvas_reset(c);
}