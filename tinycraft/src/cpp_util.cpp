typedef intptr_t intptr;
typedef uintptr_t uintptr;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float f32;
typedef double f64;

const f32 PI = 3.14159265358979323846f;

#define NUM_ELEMS(arr) (sizeof(arr) / sizeof(arr[0]))

#ifdef __GNUC__
#define UNUSED(x) UNUSED_##x __attribute__((__unused__))
#else
#define UNUSED(x) UNUSED_##x
#endif

bool breakpoint() {
    // do nothing, this is here so that debuggers can easily catch the assert failure below
    return 0;
}

#define fassert(cond)                                                          \
    do {                                                                       \
        if (!(cond)) {                                                         \
            printf("fassert failed %s at %s:%d\n", #cond, __FILE__, __LINE__); \
            breakpoint();                                                      \
            exit(EXIT_FAILURE);                                                \
        }                                                                      \
    } while (0)

#ifdef NDEBUG
#define dassert(cond) \
    do {              \
        (void)(cond); \
    } while (0)
#else
#define dassert(cond)                                                          \
    do {                                                                       \
        if (!(cond)) {                                                         \
            printf("dassert failed %s at %s:%d\n", #cond, __FILE__, __LINE__); \
            breakpoint();                                                      \
            exit(EXIT_FAILURE);                                                \
        }                                                                      \
    } while (0)
#endif

template <typename T, typename K>
bool map_contains(const T &container, const K &key) {
    auto it = container.find(key);
    return it != container.end();
}

#define set_contains map_contains

// const maps can't be indexed with []
template <typename T, typename K>
auto map_get(const T &container, const K &key) {
    auto it = container.find(key);
    fassert(it != container.end());
    return it->second;
}

#ifdef __GNUC__
// enable format string checking
//__format__(archetype, string-index, first-to-check)
// avoid warnings about "format string is not a string literal"
__attribute__((__format__(__printf__, 1, 2)))
#endif
void
fatal(const char *fmt, ...) {
    printf("fatal: ");
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf("\n");
    exit(EXIT_FAILURE);
}

struct Buffer {
    u8 *start;
    u8 *buf;
    u8 *end;

    Buffer(u8 *b, int l) {
        start = b;
        buf = b;
        end = b + l;
    };

    size_t bytes_written() {
        return buf - start;
    };

    void write_f32(f32 v) {
        dassert(buf + sizeof(f32) < end);
        *(f32 *)(buf) = v;
        buf += sizeof(f32);
    };

    void write_u32(u32 v) {
        dassert(buf + sizeof(u32) < end);
        *(u32 *)(buf) = v;
        buf += sizeof(u32);
    };

    void write_u8(u8 v) {
        dassert(buf + sizeof(u8) < end);
        *(u8 *)(buf) = v;
        buf += sizeof(u8);
    };
};

f32 lerp(f32 a, f32 b, f32 t) {
    if (t <= 0.0f) {
        return a;
    } else if (t >= 1.0f) {
        return b;
    } else {
        return a * (1 - t) + b * t;
    }
}