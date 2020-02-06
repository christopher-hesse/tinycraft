#define BUFFER_OFFSET(i) ((void *)(i))

const int VERTICES_PER_TRIANGLE = 3;
const int VERTICES_PER_QUAD = VERTICES_PER_TRIANGLE * 2;

static const char *glErrorString(GLenum error) {
    switch (error) {
    case GL_FRAMEBUFFER_UNDEFINED:
        return "GL_FRAMEBUFFER_UNDEFINED";
    case GL_FRAMEBUFFER_COMPLETE:
        return "GL_FRAMEBUFFER_COMPLETE";
    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
        return "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT";
    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
        return "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT";
    case GL_FRAMEBUFFER_UNSUPPORTED:
        return "GL_FRAMEBUFFER_UNSUPPORTED";
    case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
        return "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE";
    case GL_INVALID_FRAMEBUFFER_OPERATION:
        return "GL_INVALID_FRAMEBUFFER_OPERATION";
    case GL_INVALID_OPERATION:
        return "GL_INVALID_OPERATION";
    default:
        return NULL;
    }
}

void checkGLError(const char *file, int line) {
    GLenum err = glGetError();
    if (err == GL_NO_ERROR) {
        return;
    }
    const char *str = glErrorString(err);
    if (str == NULL) {
        printf("gl: error %lx %s:%d\n", (unsigned long)err, file, line);
    } else {
        printf("gl: error %s %s:%d\n", str, file, line);
    }
}

#if defined(DEBUG_BUILD)
#define glCheck(call)                     \
    do {                                  \
        call;                             \
        checkGLError(__FILE__, __LINE__); \
    } while (0)
#else
#define glCheck(call) call
#endif

GLint getAttribLocation(GLuint program, const GLchar *name) {
    GLint loc = glGetAttribLocation(program, name);
    if (loc == -1) {
        printf("gl: failed to get attrib location %s\n", name);
    }
    return loc;
}

GLint getUniformLocation(GLuint program, const GLchar *name) {
    GLint loc = glGetUniformLocation(program, name);
    if (loc == -1) {
        printf("gl: failed to get uniform location %s\n", name);
    }
    return loc;
}

GLuint compileShader(const char *const source, GLuint type) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status == false) {
        GLint len;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
        char *buf = new char[len]();
        glGetShaderInfoLog(shader, len, NULL, buf);
        fatal("gl: %s", buf);
        delete[] buf; // unreachable
    }
    return shader;
}

GLuint compileProgram(const char *const vertexShaderSource, const char *const fragmentShaderSource) {
    GLuint program = glCreateProgram();
    GLuint vertexShader = compileShader(vertexShaderSource, GL_VERTEX_SHADER);
    GLuint fragmentShader = compileShader(fragmentShaderSource, GL_FRAGMENT_SHADER);
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    GLint status;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (status == false) {
        GLint len;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &len);
        char *buf = new char[len]();
        glGetProgramInfoLog(program, len, NULL, buf);
        fatal("gl: %s", buf);
        delete[] buf; // unreachable
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return program;
}

void glfw_error_callback(int error, const char *description) {
    fprintf(stderr, "GLFW Error (%d): %s\n", error, description);
}

GLint get_uniform_location(GLuint program, const char *name) {
    GLint result = getUniformLocation(program, name);
    if (result == -1) {
        printf("failed to find uniform %s\n", name);
    }
    return result;
}

GLint get_attrib_location(GLuint program, const char *name) {
    GLint result = getAttribLocation(program, name);
    if (result == -1) {
        printf("failed to find attrib %s\n", name);
    }
    return result;
}