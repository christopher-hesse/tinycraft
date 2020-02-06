const f32 TARGET_SEARCH_RADIUS = 10.0f;
const f32 BLOCK_BOUNDING_SPHERE_RADIUS = 0.8684f; // radius of unit sphere
const f32 HIGHLIGHT_OFFSET = 0.002f;

// const i32 WORLD_EPISODE_STEPS = 6000;
const i32 WORLD_EPISODE_STEPS = 10000;

const i32 KEY_MOVE_FORWARD = (1 << 0);
const i32 KEY_MOVE_BACK = (1 << 1);
const i32 KEY_MOVE_LEFT = (1 << 2);
const i32 KEY_MOVE_RIGHT = (1 << 3);
const i32 KEY_MOVE_UP = (1 << 4);
const i32 KEY_MOVE_DOWN = (1 << 5);
const i32 KEY_TURN_LEFT = (1 << 6);
const i32 KEY_TURN_RIGHT = (1 << 7);
const i32 KEY_TURN_UP = (1 << 8);
const i32 KEY_TURN_DOWN = (1 << 9);
const i32 KEY_ATTACK = (1 << 10);
const i32 KEY_USE = (1 << 11);
const i32 KEY_SPRINT = (1 << 12);

// reference location is origin, facing along negative z axis (north)
const int CHUNK_VOXELS_X = 16;
const int CHUNK_VOXELS_Y = 255;
const int CHUNK_VOXELS_Z = 16;

const u8 SUN_LIGHT_LEVEL = 15;

const int BYTES_PER_VERTEX = sizeof(u32) * 2 + 2 + sizeof(f32) * 3;
const int BYTES_PER_TRIANGLE = VERTICES_PER_TRIANGLE * BYTES_PER_VERTEX;
const int BYTES_PER_QUAD = BYTES_PER_TRIANGLE * 2;
const int TRIANGLES_PER_VOXEL = 6 * 2;
const int MAX_CHUNK_MESH_BUFFER_SIZE = CHUNK_VOXELS_X * CHUNK_VOXELS_Y * CHUNK_VOXELS_Z * TRIANGLES_PER_VOXEL * BYTES_PER_TRIANGLE;

const int FRUSTUM_PLANES = 6;
const int CHANGE_BLOCK_COOLDOWN = 8;
const int TEXTURE_GRID_SIZE = 16; // the textures are arranged in the resource file in a 2d grid, this is the size of that grid

const f32 PLAYER_RADIUS = 0.5f;
const f32 MINIMUM_SPEED = 0.01f;
const f32 MAXIMUM_SPEED = 10.0f;
const f32 VELOCITY_DECAY = 0.9f;
const f32 SPRINT_FACTOR = 1.5f;

const vec3 GRAVITY = vec3(0.0f, -0.03f, 0.0f);

enum Direction {
    Direction_Up,    // positive y
    Direction_East,  // positive x
    Direction_North, // negative z
    Direction_West,  // negative x
    Direction_South, // positive z
    Direction_Down,  // negative y
    Direction_Count,
};

// these should align with the voxel_cube_vertices
// mapping corners to vertices:
// Corner_LowerLeft: 0, 3
// Corner_LowerRight: 1
// Corner_UpperRight: 2, 4
// Corner_UpperLeft: 5

enum Corner : u8 {
    Corner_LowerLeft,
    Corner_LowerRight,
    Corner_UpperRight,
    Corner_UpperLeft,
    Corner_Count,
};

static const Vertex voxel_cube_vertices[TRIANGLES_PER_VOXEL * VERTICES_PER_TRIANGLE] = {
    // top
    {0.0f, 1.0f, 1.0f},
    {1.0f, 1.0f, 1.0f},
    {1.0f, 1.0f, 0.0f},

    {0.0f, 1.0f, 1.0f},
    {1.0f, 1.0f, 0.0f},
    {0.0f, 1.0f, 0.0f},

    // right
    {1.0f, 0.0f, 1.0f},
    {1.0f, 0.0f, 0.0f},
    {1.0f, 1.0f, 0.0f},

    {1.0f, 0.0f, 1.0f},
    {1.0f, 1.0f, 0.0f},
    {1.0f, 1.0f, 1.0f},

    // back
    {1.0f, 0.0f, 0.0f},
    {0.0f, 0.0f, 0.0f},
    {0.0f, 1.0f, 0.0f},

    {1.0f, 0.0f, 0.0f},
    {0.0f, 1.0f, 0.0f},
    {1.0f, 1.0f, 0.0f},

    // left
    {0.0f, 0.0f, 0.0f},
    {0.0f, 0.0f, 1.0f},
    {0.0f, 1.0f, 1.0f},

    {0.0f, 0.0f, 0.0f},
    {0.0f, 1.0f, 1.0f},
    {0.0f, 1.0f, 0.0f},

    // front
    {0.0f, 0.0f, 1.0f},
    {1.0f, 0.0f, 1.0f},
    {1.0f, 1.0f, 1.0f},

    {0.0f, 0.0f, 1.0f},
    {1.0f, 1.0f, 1.0f},
    {0.0f, 1.0f, 1.0f},

    // bottom
    {0.0f, 0.0f, 0.0f},
    {1.0f, 0.0f, 0.0f},
    {1.0f, 0.0f, 1.0f},

    {0.0f, 0.0f, 0.0f},
    {1.0f, 0.0f, 1.0f},
    {0.0f, 0.0f, 1.0f},
};

static const TexCoords voxel_tex_coords[Direction_Count] = {
    {0.0f, 0.0f},
    {1.0f, 0.0f},
    {1.0f, 1.0f},

    {0.0f, 0.0f},
    {1.0f, 1.0f},
    {0.0f, 1.0f},
};

static const ivec3 direction_normals[Direction_Count] = {
    {0, 1, 0},
    {1, 0, 0},
    {0, 0, -1},
    {-1, 0, 0},
    {0, 0, 1},
    {0, -1, 0},
};

static const vec3 uniform_normals[Direction_Count] = {
    {0.0f, 1.0f, 0.0f},
    {1.0f, 0.0f, 0.0f},
    {0.0f, 0.0f, -1.0f},
    {-1.0f, 0.0f, 0.0f},
    {0.0f, 0.0f, 1.0f},
    {0.0f, -1.0f, 0.0f},
};

// these are the same as direction_normals
static const ivec3 neighbor_offsets[Direction_Count] = {
    {0, 1, 0},
    {1, 0, 0},
    {0, 0, -1},
    {-1, 0, 0},
    {0, 0, 1},
    {0, -1, 0},
};

// generated with generate_planar_neighbors.py
static const ivec3 planar_neighbor_offsets[Direction_Count][Corner_Count][3] = {
    {
        {{-1, 0, 1}, {-1, 0, 0}, {0, 0, 1}},
        {{0, 0, 1}, {1, 0, 0}, {1, 0, 1}},
        {{0, 0, -1}, {1, 0, -1}, {1, 0, 0}},
        {{-1, 0, 0}, {-1, 0, -1}, {0, 0, -1}},
    },
    {
        {{0, -1, 1}, {0, 0, 1}, {0, -1, 0}},
        {{0, -1, 0}, {0, 0, -1}, {0, -1, -1}},
        {{0, 1, 0}, {0, 1, -1}, {0, 0, -1}},
        {{0, 0, 1}, {0, 1, 1}, {0, 1, 0}},
    },
    {
        {{1, -1, 0}, {1, 0, 0}, {0, -1, 0}},
        {{0, -1, 0}, {-1, 0, 0}, {-1, -1, 0}},
        {{0, 1, 0}, {-1, 1, 0}, {-1, 0, 0}},
        {{1, 0, 0}, {1, 1, 0}, {0, 1, 0}},
    },
    {
        {{0, -1, -1}, {0, 0, -1}, {0, -1, 0}},
        {{0, -1, 0}, {0, 0, 1}, {0, -1, 1}},
        {{0, 1, 0}, {0, 1, 1}, {0, 0, 1}},
        {{0, 0, -1}, {0, 1, -1}, {0, 1, 0}},
    },
    {
        {{-1, -1, 0}, {-1, 0, 0}, {0, -1, 0}},
        {{0, -1, 0}, {1, 0, 0}, {1, -1, 0}},
        {{0, 1, 0}, {1, 1, 0}, {1, 0, 0}},
        {{-1, 0, 0}, {-1, 1, 0}, {0, 1, 0}},
    },
    {
        {{-1, 0, -1}, {-1, 0, 0}, {0, 0, -1}},
        {{0, 0, -1}, {1, 0, 0}, {1, 0, -1}},
        {{0, 0, 1}, {1, 0, 1}, {1, 0, 0}},
        {{-1, 0, 0}, {-1, 0, 1}, {0, 0, 1}},
    },
};