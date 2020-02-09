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

typedef struct {
    f32 delta_pitch;
    f32 delta_yaw;
    i32 keys;
} Action;

typedef struct {
    f32 compass_heading;
} Observation;

bool key_pressed(Action act, i32 key) {
    return (act.keys & key) == key;
}

class Game {
  public:
    virtual ~Game(){};
    virtual void init(bool for_agent) = 0;
    virtual void reset() = 0;
    virtual Observation act(Action action, f32 *rew, u8 *done) = 0;
    virtual void draw() = 0;
};
