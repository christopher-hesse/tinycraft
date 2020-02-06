class Game {
  public:
    virtual ~Game(){};
    virtual void init(bool for_agent) = 0;
    virtual void reset() = 0;
    virtual void act(i32 action, f32 *rew, u8 *done) = 0;
    virtual void draw() = 0;
};

// things common to multiple games

typedef struct {
    float r, g, b;
} Color;

typedef struct {
    float x, y, z;
} Vertex;

typedef struct {
    float s, t;
} TexCoords;

typedef struct {
    vec3 origin = vec3(0.0f);
    vec3 size = vec3(0.0f);
} BoundingBox;

bool key_pressed(i32 act, i32 key) {
    return (act & key) == key;
}