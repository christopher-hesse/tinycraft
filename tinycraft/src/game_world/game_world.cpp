// coordinate system is right-handed
// initial position is at 0,0,0 with y up, facing down the negative z axis
// cardinal directions are defined as:
//  up: positive y
//  east: positive x
//  north: negative z
//  west: negative x
//  south: positive z
//  down: negative y
// directions are also defined in the above order in the Direction enum
// and some constant arrays are indexed by direction
//
// voxelspace is integer coordinates like (0, 0, 0)
// the world space coordinates for voxel (0, 0, 0) are from (-0.5, -0.5, -0.5) to (0.5, 0.5, 0.5)

struct Frustum {
    vec4 planes[FRUSTUM_PLANES] = {};
};

struct ShaderVoxel {
    GLint program = -1;
    GLint uniform_proj_view = -1;
    GLint uniform_xz_chunkspace = -1;
    GLint uniform_global_light_direction = -1;
    GLint uniform_global_light_deviation = -1;
    GLint uniform_global_light_min = -1;
    GLint uniform_ambient_color = -1;
    GLint uniform_terrain_texture = -1;
    GLint uniform_terrain_texture_enabled = -1;
    GLint uniform_fog = -1;
    GLint uniform_camera_pos = -1;
    GLint uniform_normals = -1;
    GLint attrib_in_pos_relvoxelspace = -1;
    GLint attrib_in_normal_index = -1;
    GLint attrib_in_diffuse_color = -1;
    GLint attrib_in_ambient_brightness = -1;
    GLint attrib_in_tex_coord = -1;
};

struct ShaderHighlight {
    GLint program = -1;
    GLint uniform_proj_view = -1;
    GLint attrib_in_coords = -1;
};

struct GameWorld : public Game {
    GameWorld(u32 seed, i32 _vision_width, i32 _vision_height, i32 world_chunks, bool _debug_enabled);
    ~GameWorld();
    void init(bool for_agent) override;
    void reset() override;
    void act(Action action, f32 *rew, u8 *done) override;
    void draw() override;

  private:
    int num_steps = 0;
    bool terrain_texture_enabled = true;
    bool debug_enabled = false;
    bool smooth_lighting = true;
    bool clipping = true;
    bool use_minecraft_terrain_texture = false;

    bool on_ground = false;
    bool sprinting = false;

    int steps_since_block_change = 0;

    f32 fog_distance = 100.0f;
    vec3 sky_color = vec3(0.48f, 0.72f, 1.0f);

    vec3 player_pos = vec3(0.0f);
    vec3 player_vel = vec3(0.0f);
    ivec3 player_pos_vs = ivec3(0);
    f32 yaw_radians = 0.0f;
    f32 pitch_radians = 0.0f;
    bool targeting_block = false;
    ivec3 targeted_block_pos = ivec3(0);

    std::mt19937 rand_gen;
    i32 vision_width = -1;
    i32 vision_height = -1;

    ShaderVoxel voxel_shader;

    ShaderHighlight highlight_shader;
    GLuint highlight_vao = 0;
    GLuint highlight_buffer = 0;

    bool flip_y = false;

    vec3 global_light_direction = normalize(vec3(1.0f, 2.0f, 1.0f));
    vec3 global_light_deviation = vec3(0.1f);
    vec3 global_light_min = vec3(0.05f);
    vec3 ambient_color = vec3(1.0f, 1.0f, 1.0f);

    GLuint terrain_texture = 0;

    Canvas *canvas = nullptr;
    World *world = nullptr;
    ivec3 world_size = ivec3(0);

    std::thread *builder_thread = nullptr;
    World *next_world = nullptr;
    bool builder_should_terminate = false;
    std::condition_variable builder_cv;
    std::mutex builder_mutex;

    void mark_chunk_dirty(ivec3 pos);
    void rebuild_dirty_chunks();
    void set_voxel(ivec3 loc, BlockType bt);
    std::vector<Chunk *> find_nearby_chunks(int cx, int cz, int r);
};

ivec3 world_to_voxelspace(vec3 world) {
    return ivec3(round(world.x), round(world.y), round(world.z));
}

BoundingBox bounding_box_for_voxel(ivec3 block_pos) {
    BoundingBox r;
    r.origin = vec3(block_pos) - vec3(0.5f);
    r.size = vec3(1.0f);
    return r;
}

vec2 get_planar_point(const vec3 &p, const u8 &d) {
    if (d == Direction_Up || d == Direction_Down) {
        return vec2(p.x, p.z);
    } else if (d == Direction_East || d == Direction_West) {
        return vec2(p.y, p.z);
    } else if (d == Direction_North || d == Direction_South) {
        return vec2(p.x, p.y);
    } else {
        fassert(false);
    }
}

bool point_inside_quad(vec2 p, vec2 lower, vec2 upper) {
    return lower.x <= p.x && p.x <= upper.x && lower.y <= p.y && p.y <= upper.y;
}

// line_intersects_box will return the face of the bounding box that the line intersects, but only
// intersection points after line_origin along line_direction
bool line_intersects_box(const vec4 &line_origin, const vec4 &line_direction, const BoundingBox &box, u8 *out_direction, f32 *out_dist) {
    vec3 intersection = vec3(0.0f);
    bool found_intersection = false;
    vec3 best_intersection = vec3(0.0f);
    u8 best_direction = 0;
    f32 best_dist = 0.0f;

    dassert(all(greaterThan(box.size, vec3(0.0f))));

    for (u8 dir = 0; dir < Direction_Count; dir++) {
        auto n = vec3(direction_normals[dir]);

        vec3 plane_point = box.origin + (1.0f + n) * box.size / 2.0f;
        f32 plane_dist = -dot(n, plane_point);
        auto plane = vec4(n.x, n.y, n.z, plane_dist);
        f32 signed_dist;
        if (!line_intersects_plane(plane, line_origin, line_direction, &signed_dist, &intersection)) {
            continue;
        }

        // only match if the intersection is in front of us
        if (signed_dist < 0.0f) {
            continue;
        }

        if (!point_inside_quad(get_planar_point(intersection, dir), get_planar_point(box.origin, dir), get_planar_point(box.origin + box.size, dir))) {
            continue;
        }
        if (!found_intersection || signed_dist < best_dist) {
            best_dist = signed_dist;
            best_intersection = intersection;
            best_direction = dir;
        }
        found_intersection = true;
    }

    if (found_intersection) {
        *out_dist = best_dist;
        *out_direction = best_direction;
        return true;
    } else {
        return false;
    }
}

void append_lines(BoundingBox box, std::vector<vec3> *buf) {
    auto lower = box.origin;
    auto upper = box.origin + box.size;

    auto a = lower;
    auto b = vec3(upper.x, lower.y, lower.z);
    auto c = vec3(upper.x, lower.y, upper.z);
    auto d = vec3(lower.x, lower.y, upper.z);
    auto e = vec3(lower.x, upper.y, lower.z);
    auto f = vec3(upper.x, upper.y, lower.z);
    auto g = vec3(upper.x, upper.y, upper.z);
    auto h = vec3(lower.x, upper.y, upper.z);
    // bottom
    buf->push_back(a);
    buf->push_back(b);
    buf->push_back(b);
    buf->push_back(c);
    buf->push_back(c);
    buf->push_back(d);
    buf->push_back(d);
    buf->push_back(a);
    // top
    buf->push_back(e);
    buf->push_back(f);
    buf->push_back(f);
    buf->push_back(g);
    buf->push_back(g);
    buf->push_back(h);
    buf->push_back(h);
    buf->push_back(e);
    // sides
    buf->push_back(a);
    buf->push_back(e);
    buf->push_back(b);
    buf->push_back(f);
    buf->push_back(c);
    buf->push_back(g);
    buf->push_back(d);
    buf->push_back(h);
}

static const char *voxel_shader_vertex = R"(
#version 140

uniform mat4 proj_view;
uniform uvec2 xz_chunkspace;
uniform vec3[6] normals;
in uint in_pos_relvoxelspace;
in uint in_normal_index;
in vec4 in_diffuse_color;
in uint in_ambient_brightness;
in vec3 in_tex_coord;
out vec3 normal;
out vec4 diffuse_color;
out float ambient_brightness;
out vec3 tex_coord;
out vec3 world_pos;

void main()
{
    uvec2 offset = xz_chunkspace * 16u;  // this is CHUNK_VOXELS_X/CHUNK_VOXELS_Z
    float world_x = float((in_pos_relvoxelspace >> 0u) & 511u) / 16 + offset[0] - 0.5;
    float world_y = float((in_pos_relvoxelspace >> 9u) & 8191u) / 16 - 0.5;
    float world_z = float((in_pos_relvoxelspace >> 22u) & 511u) / 16 + offset[1] - 0.5;
    gl_Position = proj_view * vec4(world_x, world_y, world_z, 1.0);
    diffuse_color = in_diffuse_color;
    ambient_brightness = float(in_ambient_brightness) / 255.0;
    tex_coord = in_tex_coord;
    normal = normals[in_normal_index];
    world_pos = vec3(world_x, world_y, world_z);
}
)";

static const char *voxel_shader_fragment = R"(
#version 140

uniform vec3 global_light_direction;
uniform vec3 global_light_deviation;
uniform vec3 global_light_min;
uniform vec3 ambient_color;
uniform sampler2DArray terrain_texture;
uniform bool terrain_texture_enabled;
uniform vec4 fog;
uniform vec3 camera_pos;
in vec3 normal;
in vec4 diffuse_color;
in float ambient_brightness;
in vec3 tex_coord;
in vec3 world_pos;
out vec4 out_fragcolor;

// from stb_voxel_render.h
vec4 compute_fog(vec4 color, vec3 relative_pos) {
    float frag_distance_squared = dot(relative_pos, relative_pos);
    float inv_view_distance_squared = fog.w;  // fog.w is 1/view_distance**2
    float normalized_distance_squared = frag_distance_squared * inv_view_distance_squared;
    float percent_fog = smoothstep(0.0, 1.0, normalized_distance_squared);
    return vec4(mix(color.rgb, fog.rgb * color.a, percent_fog), color.a);
}

void main()
{
    float global_light_intensity_root = dot(global_light_direction, normal) / 2.0 + 0.5;
    float global_light_intensity = global_light_intensity_root * global_light_intensity_root;
    vec3 global_light = vec3(global_light_intensity * global_light_deviation + global_light_min);
    global_light = clamp(global_light, 0.0, 1.0);
    vec4 texel = diffuse_color;
    if (terrain_texture_enabled) {
        texel = texture(terrain_texture, tex_coord);
        if (diffuse_color.a > 0.0) {
            texel = texel * diffuse_color;
        }
    }
    vec4 lit_color = vec4(texel.xyz * (global_light + ambient_brightness * ambient_color), texel.a);
    vec3 relative_pos = world_pos - camera_pos;
    vec4 fogged_color = compute_fog(lit_color, relative_pos);
    out_fragcolor = fogged_color;
}
)";

static const char *highlight_shader_vertex = R"(
#version 140

uniform mat4 proj_view;
in vec3 in_coords;
void main()
{
    gl_Position = proj_view * vec4(in_coords, 1.0);
}
)";

static const char *highlight_shader_fragment = R"(
#version 140

out vec4 out_fragcolor;

void main()
{
    out_fragcolor = vec4(1.0, 0.4, 0.7, 1.0);
}
)";

ShaderVoxel compile_voxel_shader() {
    ShaderVoxel s;
    s.program = compileProgram(voxel_shader_vertex, voxel_shader_fragment);
    s.uniform_proj_view = get_uniform_location(s.program, "proj_view");
    s.uniform_xz_chunkspace = get_uniform_location(s.program, "xz_chunkspace");
    s.uniform_global_light_direction = get_uniform_location(s.program, "global_light_direction");
    s.uniform_global_light_deviation = get_uniform_location(s.program, "global_light_deviation");
    s.uniform_global_light_min = get_uniform_location(s.program, "global_light_min");
    s.uniform_ambient_color = get_uniform_location(s.program, "ambient_color");
    s.uniform_terrain_texture = get_uniform_location(s.program, "terrain_texture");
    s.uniform_terrain_texture_enabled = get_uniform_location(s.program, "terrain_texture_enabled");
    s.uniform_fog = get_uniform_location(s.program, "fog");
    s.uniform_camera_pos = get_uniform_location(s.program, "camera_pos");
    s.uniform_normals = get_uniform_location(s.program, "normals");
    s.attrib_in_pos_relvoxelspace = get_attrib_location(s.program, "in_pos_relvoxelspace");
    s.attrib_in_normal_index = get_attrib_location(s.program, "in_normal_index");
    s.attrib_in_diffuse_color = get_attrib_location(s.program, "in_diffuse_color");
    s.attrib_in_ambient_brightness = get_attrib_location(s.program, "in_ambient_brightness");
    s.attrib_in_tex_coord = get_attrib_location(s.program, "in_tex_coord");
    return s;
}

ShaderHighlight compile_highlight_shader() {
    ShaderHighlight s;
    s.program = compileProgram(highlight_shader_vertex, highlight_shader_fragment);
    s.uniform_proj_view = get_uniform_location(s.program, "proj_view");
    s.attrib_in_coords = get_attrib_location(s.program, "in_coords");
    return s;
}

struct BuildWorkerContext {
    bool *should_terminate = nullptr;
    World **out_world = nullptr;
    std::condition_variable *cv = nullptr;
    std::mutex *mtx = nullptr;
    std::function<World *()> build_func;
};

void build_worker(BuildWorkerContext ctx) {
    while (true) {
        {
            std::unique_lock<std::mutex> lock(*ctx.mtx);
            while (*ctx.out_world != nullptr) {
                if (*ctx.should_terminate) {
                    return;
                }
                ctx.cv->wait(lock);
            }
        }

        // auto start = glfwGetTime();
        // printf("building world\n");
        World *w = ctx.build_func();
        // auto end = glfwGetTime();
        // printf("build world elapsed %f\n", end - start);

        {
            std::unique_lock<std::mutex> lock(*ctx.mtx);
            *ctx.out_world = w;
            ctx.cv->notify_all();
        }
    }
}

void GameWorld::mark_chunk_dirty(ivec3 pos) {
    world->chunk_key_to_chunk.at(get_chunk_key_for_voxel(world, pos))->dirty = true;
}

void GameWorld::rebuild_dirty_chunks() {
    for (auto it : world->chunk_key_to_chunk) {
        Chunk *c = it.second;
        if (c->dirty) {
            rebuild_chunk(world, c);
            glCheck(glBindBuffer(GL_ARRAY_BUFFER, c->buffer));
            glCheck(glBufferData(GL_ARRAY_BUFFER, c->num_triangles * BYTES_PER_TRIANGLE, world->chunk_mesh_buffer, GL_DYNAMIC_DRAW));
            c->dirty = false;
        }
    }
}

GameWorld::GameWorld(u32 seed, i32 _vision_width, i32 _vision_height, i32 world_chunks, bool _debug_enabled) {
    rand_gen.seed(seed);
    vision_width = _vision_width;
    vision_height = _vision_height;
    debug_enabled = _debug_enabled;
    world_size = ivec3(CHUNK_VOXELS_X * world_chunks, CHUNK_VOXELS_Y, CHUNK_VOXELS_Z * world_chunks);
}

void GameWorld::init(bool for_agent) {
    if (debug_enabled) {
        canvas = canvas_create(vision_width, vision_height, Terminus16Medium);
    }

    std::string terrain_texture_filename = "terrain-kenney.png";
    if (use_minecraft_terrain_texture) {
        terrain_texture_filename = "terrain-minecraft.png";
    }
    auto terrain_image = image_load(get_resource_path(terrain_texture_filename));
    fassert(terrain_image.height == terrain_image.width);
    fassert(terrain_image.height % TEXTURE_GRID_SIZE == 0);
    int texture_size = terrain_image.width / TEXTURE_GRID_SIZE;
    int texture_count = TEXTURE_GRID_SIZE * TEXTURE_GRID_SIZE;
    glGenTextures(1, &terrain_texture);
    {
        // copy the 2d textures into the 3d texture so that we can easily generate mipmaps
        // we have to re-arrange the texture to be in the correct layout for a 3d image
        auto terrain_image_array_data = new u8[terrain_image.width * terrain_image.height * IMAGE_BYTES_PER_PIXEL];
        for (int texture_index = 0; texture_index < texture_count; texture_index++) {
            int texture_x = texture_index % TEXTURE_GRID_SIZE;
            int texture_y = texture_index / TEXTURE_GRID_SIZE;
            auto src = image_flip_y(image_sub(terrain_image, ivec2(texture_x * texture_size, texture_y * texture_size), ivec2(texture_size, texture_size)));
            auto dst = image_from_buffer(terrain_image_array_data + texture_index * texture_size * texture_size * IMAGE_BYTES_PER_PIXEL, texture_size, texture_size, texture_size * IMAGE_BYTES_PER_PIXEL);
            image_composite(&dst, &src, 0, 0, COMPOSITE_CPU_COPY);
            image_destroy(src);
        }
        glBindTexture(GL_TEXTURE_2D_ARRAY, terrain_texture);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_ANISOTROPY, 16);
        auto terrain_image_array = image_from_buffer(terrain_image_array_data, texture_size, texture_size * TEXTURE_GRID_SIZE * TEXTURE_GRID_SIZE, texture_size * IMAGE_BYTES_PER_PIXEL);
        glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, texture_size, texture_size, texture_count, 0, GL_RGBA, GL_UNSIGNED_BYTE, terrain_image_array.data);
        glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
        delete[] terrain_image_array_data;
    }
    image_destroy(terrain_image);

    // when rendering for the agent, we use glReadPixels
    // which will read lines starting in the bottom left corner of the image
    // since numpy expects images to start in the top left corner,
    // we scale the y axis by -1 but this reverses the winding order of
    // triangles, so set switch that to compensate
    flip_y = for_agent;

    voxel_shader = compile_voxel_shader();

    highlight_shader = compile_highlight_shader();

    {
        glCheck(glGenVertexArrays(1, &highlight_vao));
        glCheck(glBindVertexArray(highlight_vao));

        glCheck(glGenBuffers(1, &highlight_buffer));
        glCheck(glBindBuffer(GL_ARRAY_BUFFER, highlight_buffer));

        auto shader = highlight_shader;
        glCheck(glEnableVertexAttribArray(shader.attrib_in_coords));
        glCheck(glVertexAttribPointer(shader.attrib_in_coords, 3, GL_FLOAT, GL_FALSE, sizeof(f32) * 3, BUFFER_OFFSET(0)));
    }

    {

        auto diffuse_color_map = block_type_and_direction_to_diffuse_color;
        if (terrain_texture_enabled) {
            if (use_minecraft_terrain_texture) {
                diffuse_color_map = block_type_and_direction_to_textured_diffuse_color;
            } else {
                diffuse_color_map = block_type_and_direction_to_no_diffuse_color;
            }
        }

        BuildWorkerContext ctx;
        ctx.should_terminate = &builder_should_terminate;
        ctx.out_world = &next_world;
        ctx.cv = &builder_cv;
        ctx.mtx = &builder_mutex;
        ctx.build_func = [=]() -> World * {
            return build_world(world_size, rand_gen(), &diffuse_color_map[0][0], smooth_lighting);
        };
        builder_thread = new std::thread(build_worker, ctx);
        // world = ctx.build_func();
    }
}

GameWorld::~GameWorld() {
    if (builder_thread != nullptr) {
        {
            std::unique_lock<std::mutex> lock(builder_mutex);
            builder_should_terminate = true;
            builder_cv.notify_all();
        }
        builder_thread->join();
        delete builder_thread;
    }
}

void GameWorld::reset() {
    if (world != nullptr) {
        for (auto it : world->chunk_key_to_chunk) {
            Chunk *c = it.second;
            glDeleteVertexArrays(1, &c->vao);
            glDeleteBuffers(1, &c->buffer);
            destroy_chunk(c);
        }
        delete world;
        world = nullptr;
    }

    std::unique_lock<std::mutex> lock(builder_mutex);
    while (next_world == nullptr) {
        builder_cv.wait(lock);
    }
    world = next_world;
    next_world = nullptr;
    builder_cv.notify_all();

    // create vertex array objects for all chunks in the world
    for (auto it : world->chunk_key_to_chunk) {
        Chunk *c = it.second;

        glCheck(glGenVertexArrays(1, &c->vao));
        glCheck(glBindVertexArray(c->vao));

        glCheck(glGenBuffers(1, &c->buffer));
        glCheck(glBindBuffer(GL_ARRAY_BUFFER, c->buffer));

        auto shader = voxel_shader;
        glCheck(glEnableVertexAttribArray(shader.attrib_in_pos_relvoxelspace));
        glCheck(glVertexAttribIPointer(shader.attrib_in_pos_relvoxelspace, 1, GL_UNSIGNED_INT, BYTES_PER_VERTEX, BUFFER_OFFSET(0)));
        glCheck(glEnableVertexAttribArray(shader.attrib_in_normal_index));
        glCheck(glVertexAttribIPointer(shader.attrib_in_normal_index, 1, GL_BYTE, BYTES_PER_VERTEX, BUFFER_OFFSET(sizeof(u32) * 1)));
        glCheck(glEnableVertexAttribArray(shader.attrib_in_diffuse_color));
        glCheck(glVertexAttribPointer(shader.attrib_in_diffuse_color, 4, GL_UNSIGNED_BYTE, GL_TRUE, BYTES_PER_VERTEX, BUFFER_OFFSET(sizeof(u32) * 1 + 1)));
        glCheck(glEnableVertexAttribArray(shader.attrib_in_ambient_brightness));
        glCheck(glVertexAttribIPointer(shader.attrib_in_ambient_brightness, 1, GL_UNSIGNED_BYTE, BYTES_PER_VERTEX, BUFFER_OFFSET(sizeof(u32) * 2 + 1)));
        glCheck(glEnableVertexAttribArray(shader.attrib_in_tex_coord));
        glCheck(glVertexAttribPointer(shader.attrib_in_tex_coord, 3, GL_FLOAT, GL_FALSE, BYTES_PER_VERTEX, BUFFER_OFFSET(sizeof(u32) * 2 + 2)));

        // buffer pre-generated mesh data
        glCheck(glBindBuffer(GL_ARRAY_BUFFER, c->buffer));
        glCheck(glBufferData(GL_ARRAY_BUFFER, c->num_triangles * BYTES_PER_TRIANGLE, c->original_mesh, GL_DYNAMIC_DRAW));
    }

    num_steps = 0;
    player_pos = world->player_start_pos;
    pitch_radians = 0.0f;
    yaw_radians = 0.0f;
    on_ground = false;
    sprinting = false;
    steps_since_block_change = 0;
}

void GameWorld::act(Action a, f32 *rew, u8 *done) {
    if (debug_enabled) {
        canvas_reset(canvas);
    }

    mat4 player_direction = mat4(1.0f);
    player_direction = rotate(player_direction, yaw_radians, vec3(0.0f, 1.0f, 0.0f));
    player_direction = rotate(player_direction, pitch_radians, vec3(1.0f, 0.0f, 0.0f));

    vec3 player_facing = vec3(player_direction * vec4(0.0f, 0.0f, -1.0f, 0.0f));
    player_pos_vs = world_to_voxelspace(player_pos);

    // draw a line between the player and where the player is looking
    // search along that line for the voxel that the player is looking at
    std::vector<ivec3> line_blocks;
    for (f32 offset = 1.0f; offset < TARGET_SEARCH_RADIUS; offset += 1.0f) {
        line_blocks.push_back(world_to_voxelspace(player_pos + player_facing * offset));
    }
    // also search neighbors of blocks along the line, since the line may be a little off
    std::unordered_set<ivec3> blocks_to_check;
    for (auto block : line_blocks) {
        blocks_to_check.insert(block);
        for (int d = 0; d < Direction_Count; d++) {
            auto neighbor_pos_vs = block + neighbor_offsets[d];
            blocks_to_check.insert(neighbor_pos_vs);
        }
    }

    bool block_found = false;
    f32 best_block_dist = 0.0f;
    auto best_block_pos_vs = ivec3(0);
    u8 best_block_direction;

    for (auto block_pos_vs : blocks_to_check) {
        // check if block pos is on existing block grid
        if (!world->block_types.contains(block_pos_vs)) {
            continue;
        }
        // check if we are looking at this block
        if (!line_intersects_sphere(player_pos, player_facing, block_pos_vs, BLOCK_BOUNDING_SPHERE_RADIUS, true)) {
            continue;
        }
        if (!is_solid(world->block_types.get(block_pos_vs))) {
            continue;
        }
        BoundingBox box = bounding_box_for_voxel(block_pos_vs);
        f32 dist;
        u8 direction;
        if (!line_intersects_box(vec4(player_pos, 1.0f), vec4(player_facing, 0.0f), box, &direction, &dist)) {
            continue;
        }
        if (!block_found || dist < best_block_dist) {
            best_block_dist = dist;
            best_block_pos_vs = block_pos_vs;
            best_block_direction = direction;
        }
        block_found = true;
    }

    if (block_found) {
        targeting_block = true;
        targeted_block_pos = best_block_pos_vs;
    } else {
        targeting_block = false;
    }

    steps_since_block_change++;
    if (steps_since_block_change >= CHANGE_BLOCK_COOLDOWN) {
        if (block_found && key_pressed(a, KEY_ATTACK)) {
            set_voxel(best_block_pos_vs, BlockType_Air);
            steps_since_block_change = 0;
        }

        if (block_found && key_pressed(a, KEY_USE)) {
            auto place_block_pos_vs = best_block_pos_vs + direction_normals[best_block_direction];
            if (world->block_types.contains(place_block_pos_vs) && world->block_types.get(place_block_pos_vs) == BlockType_Air && place_block_pos_vs != player_pos_vs) {
                set_voxel(place_block_pos_vs, BlockType_Leaves);
                steps_since_block_change = 0;
            }
        }
    }

    sprinting = key_pressed(a, KEY_SPRINT);

    auto movement = vec4(0.0f);
    if (key_pressed(a, KEY_MOVE_FORWARD)) {
        movement.z -= 1.0f;
    }
    if (key_pressed(a, KEY_MOVE_BACK)) {
        movement.z += 1.0f;
    }
    if (key_pressed(a, KEY_MOVE_LEFT)) {
        movement.x -= 1.0f;
    }
    if (key_pressed(a, KEY_MOVE_RIGHT)) {
        movement.x += 1.0f;
    }
    if (movement != vec4(0.0f)) {
        movement = vec4(normalize(vec3(movement)), 0.0f) * 0.02f;
    }
    if (sprinting) {
        movement *= SPRINT_FACTOR;
    }

    // decay velocity
    if (clipping) {
        // don't decay vertical velocity
        player_vel = player_vel * vec3(VELOCITY_DECAY, 1.0f, VELOCITY_DECAY);
    } else {
        player_vel = player_vel * VELOCITY_DECAY;
    }

    if (magnitude(player_vel) < MINIMUM_SPEED) {
        player_vel = vec3(0.0f);
    }
    if (clipping) {
        // if clipping is enabled, movement works a bit differently
        // the player_direction is always on the x-z plane
        mat4 movement_direction = mat4(1.0f);
        movement_direction = rotate(movement_direction, yaw_radians, vec3(0.0f, 1.0f, 0.0f));
        if (on_ground && key_pressed(a, KEY_MOVE_UP)) {
            movement.y += 0.4f;
        }
        player_vel += vec3(movement_direction * movement) + GRAVITY;
    } else {
        if (key_pressed(a, KEY_MOVE_UP)) {
            player_vel.y += 0.02f;
        }
        if (key_pressed(a, KEY_MOVE_DOWN)) {
            player_vel.y -= 0.02f;
        }
        player_vel = player_vel + vec3(player_direction * movement);
    }
    auto xz_vel = player_vel * vec3(1.0f, 0.0f, 1.0f);
    // cap xz velocity
    f32 xz_vel_mag = magnitude(xz_vel);
    if (xz_vel_mag > MAXIMUM_SPEED) {
        xz_vel = xz_vel / xz_vel_mag * MAXIMUM_SPEED;
    }
    player_vel = vec3(xz_vel.x, clamp(player_vel.y, -20.0f, 10.0f), xz_vel.z);

    // attempt to move in the indicated direction
    auto new_player_pos = player_pos + player_vel;
    auto new_player_pos_vs = world_to_voxelspace(new_player_pos);

    on_ground = false;
    if (clipping) {
        // check for collision with each neighbor voxel
        for (int d = 0; d < Direction_Count; d++) {
            auto neighbor_offset = neighbor_offsets[d];
            auto neighbor_pos_vs = new_player_pos_vs + neighbor_offset;
            // figure out the distance from the player's new world space
            // pos to the world space pos of the nearest point on the plane
            // of the closest face of this neighbor voxel

            // if we mask out all dimensions besides the relevant ones we can
            // easily get this distance
            // for the voxel at (0, 0, 0) in voxel space, the top face will be at (0, 0.5, 0)
            // and the bottom face will be at (0, -0.5, 0)
            auto near_neighbor_pos = vec3(neighbor_pos_vs * abs(neighbor_offset)) - vec3(neighbor_offset) / 2.0f;
            f32 dist = distance(new_player_pos * abs(vec3(neighbor_offset)), near_neighbor_pos);
            if (debug_enabled) {
                canvas_print(canvas, "dist: %f", dist);
            }

            bool has_face_collision = false;
            if (dist <= PLAYER_RADIUS) {
                if (!world->block_types.contains(neighbor_pos_vs) || is_solid(world->block_types.get(neighbor_pos_vs))) {
                    has_face_collision = true;
                    if (d == Direction_Down) {
                        on_ground = true;
                    }
                    if (debug_enabled) {
                        canvas_print(canvas, " X");
                    }
                }
            }

            if (debug_enabled) {
                canvas_print(canvas, "\n");
            }

            if (has_face_collision) {
                // only zero it out if it's in this direction
                if (any(greaterThan(player_vel * vec3(neighbor_offset), vec3(0.0f)))) {
                    player_vel -= player_vel * abs(vec3(neighbor_offset));
                }
            }
        }

        auto final_new_player_pos = player_pos + player_vel;
        // make sure we didn't somehow end up inside a solid block
        if (is_solid(world->block_types.get(world_to_voxelspace(final_new_player_pos)))) {
            player_vel = vec3(0.0f);
        } else {
            player_pos = final_new_player_pos;
        }
    } else {
        player_pos = new_player_pos;
    }

    if (debug_enabled) {
        canvas_print(canvas, "on_ground: %d\n", on_ground);
    }
        yaw_radians +=a.delta_yaw;
        pitch_radians += a.delta_pitch;
    if (yaw_radians > PI) {
        yaw_radians -= 2 * PI;
    } else if (yaw_radians < -PI) {
        yaw_radians += 2 * PI;
    }
    pitch_radians = clamp(pitch_radians, -PI / 2, PI / 2);
    num_steps++;
    *rew = 0.0f;
    *done = num_steps >= WORLD_EPISODE_STEPS;
    if (*done) {
        reset();
    }

    if (debug_enabled) {
        canvas_print(canvas, "xyz: %s\n", to_string(player_pos).c_str());
        canvas_print(canvas, "vel: %s\n", to_string(player_vel).c_str());
        canvas_print(canvas, "block: %s\n", to_string(player_pos_vs).c_str());
        auto player_chunk_key = get_chunk_key_for_voxel(world, player_pos_vs);
        auto chunk = world->chunk_key_to_chunk.at(player_chunk_key);
        canvas_print(canvas, "chunk: %d %d\n", chunk->x_chunkspace, chunk->z_chunkspace);
        if (targeting_block) {
            canvas_print(canvas, "targeted block: %s\n", to_string(targeted_block_pos).c_str());
        }

        if (world->block_light_sky.contains({player_pos_vs.x, player_pos_vs.y, player_pos_vs.z})) {
            auto block_light = world->block_light_sky.get({player_pos_vs.x, player_pos_vs.y, player_pos_vs.z});
            canvas_print(canvas, "light: %d\n", block_light);
        }
        canvas_print(canvas, "pitch: %f yaw: %f\n", pitch_radians, yaw_radians);
    }
}

std::vector<Chunk *> GameWorld::find_nearby_chunks(int cx, int cz, int radius) {
    std::vector<Chunk *> result;
    for (int r = 0; r < radius; r++) {
        for (int x = -r; x <= r; x += 1) {
            int step = 1;
            if (abs(x) != r) {
                step = 2 * r;
            }
            for (int z = -r; z <= r; z += step) {
                auto key = ivec2(cx + x, cz + z);
                if (map_contains(world->chunk_key_to_chunk, key)) {
                    auto c = world->chunk_key_to_chunk.at(key);
                    result.push_back(c);
                }
            }
        }
    }
    return result;
}

// https://fgiesen.wordpress.com/2012/08/31/frustum-planes-from-the-projection-matrix/
Frustum get_frustum_from_projection(mat4 p) {
    // matrix is stored in column-major order, transpose to get row-major
    auto tp = transpose(p);
    Frustum r;
    for (int i = 0; i < FRUSTUM_PLANES / 2; i++) {
        r.planes[2 * i] = tp[3] + tp[i];
        r.planes[2 * i + 1] = tp[3] - tp[i];
    }
    return r;
}

bool box_is_before_plane(const BoundingBox &b, const vec4 &p) {
    // return true if any part of the box is before the plane
    f32 d = 0.0f;
    dassert(all(greaterThan(b.size, vec3(0.0f))));
    auto lower = b.origin;
    auto upper = b.origin + b.size;
    if (p.x > 0.0f) {
        d += upper.x * p.x;
    } else {
        d += lower.x * p.x;
    }
    if (p.y > 0.0f) {
        d += upper.y * p.y;
    } else {
        d += lower.y * p.y;
    }
    if (p.z > 0.0f) {
        d += upper.z * p.z;
    } else {
        d += lower.z * p.z;
    }
    return d + p.w >= 0.0f;
}

bool box_is_in_frustum(const BoundingBox &b, const Frustum &f) {
    for (int i = 0; i < FRUSTUM_PLANES; i++) {
        if (!box_is_before_plane(b, f.planes[i])) {
            return false;
        }
    }
    return true;
}

void GameWorld::draw() {
    glCheck(glClearColor(sky_color.r, sky_color.g, sky_color.b, 1.0f));
    glCheck(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
    f32 ratio = (f32)(vision_width) / (f32)(vision_height);

    // increase fov when moving fast (but not when jumping)
    vec3 vel = player_vel;
    if (clipping) {
        vel = vec3(vel.x, 0.0f, vel.z);
    }
    f32 speed = magnitude(vel);
    f32 fov = PI / 3.0f;
    if (clipping && speed > 0.2f) {
        fov = lerp(PI / 3.0f, PI / 2.0f, (speed - 0.2f) / 0.1f);
    }

    mat4 proj = perspective(fov, ratio, 0.1f, 1000.0f);
    mat4 flip = mat4(1.0f);
    if (flip_y) {
        flip = scale(flip, vec3(1.0f, -1.0f, 1.0f));
    }
    mat4 view = mat4(1.0f);
    view = rotate(view, -pitch_radians, vec3(1.0f, 0.0f, 0.0f));
    view = rotate(view, -yaw_radians, vec3(0.0f, 1.0f, 0.0f));
    view = translate(view, -player_pos);

    mat4 proj_view = proj * flip * view;

    auto frustum = get_frustum_from_projection(proj_view);

    glCheck(glEnable(GL_DEPTH_TEST));
    glCheck(glEnable(GL_CULL_FACE));
    if (flip_y) {
        glCheck(glFrontFace(GL_CW));
    }

    glCheck(glEnable(GL_BLEND));
    glCheck(glBlendEquation(GL_FUNC_ADD));
    // we are using pre-multiplied alpha http://apoorvaj.io/alpha-compositing-opengl-blending-and-premultiplied-alpha.html
    glCheck(glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA));

    {
        auto shader = voxel_shader;
        glCheck(glUseProgram(shader.program));
        glCheck(glUniformMatrix4fv(shader.uniform_proj_view, 1, GL_FALSE, (GLfloat *)&proj_view[0]));
        glCheck(glUniform3fv(shader.uniform_global_light_direction, 1, (GLfloat *)&global_light_direction[0]));
        glCheck(glUniform3fv(shader.uniform_global_light_deviation, 1, (GLfloat *)&global_light_deviation[0]));
        glCheck(glUniform3fv(shader.uniform_global_light_min, 1, (GLfloat *)&global_light_min[0]));
        glCheck(glUniform3fv(shader.uniform_ambient_color, 1, (GLfloat *)&ambient_color[0]));
        glCheck(glUniform1i(shader.uniform_terrain_texture, 0));
        glCheck(glActiveTexture(GL_TEXTURE0));
        glCheck(glBindTexture(GL_TEXTURE_2D, terrain_texture));
        glCheck(glUniform1i(shader.uniform_terrain_texture_enabled, terrain_texture_enabled));

        auto fog = vec4(sky_color, 1.0 / (fog_distance * fog_distance));
        glCheck(glUniform4fv(shader.uniform_fog, 1, (GLfloat *)&fog[0]));
        glCheck(glUniform3fv(shader.uniform_camera_pos, 1, (GLfloat *)&player_pos[0]));
        glCheck(glUniform3fv(shader.uniform_normals, NUM_ELEMS(uniform_normals), (GLfloat *)&uniform_normals[0]));

        rebuild_dirty_chunks();

        auto player_chunk_key = get_chunk_key_for_voxel(world, player_pos_vs);
        auto player_chunk = world->chunk_key_to_chunk.at(player_chunk_key);
        int chunk_view_radius = int(ceil(fog_distance / CHUNK_VOXELS_X));
        for (auto c : find_nearby_chunks(player_chunk->x_chunkspace, player_chunk->z_chunkspace, chunk_view_radius)) {
            if (box_is_in_frustum(c->bounding_box_worldspace, frustum)) {
                glCheck(glBindVertexArray(c->vao));
                glCheck(glUniform2ui(shader.uniform_xz_chunkspace, c->x_chunkspace, c->z_chunkspace));
                glCheck(glDrawArrays(GL_TRIANGLES, 0, c->num_triangles * VERTICES_PER_TRIANGLE));
            }
        }
    }

    if (targeting_block) {
        auto shader = highlight_shader;
        glCheck(glUseProgram(highlight_shader.program));
        glCheck(glUniformMatrix4fv(shader.uniform_proj_view, 1, GL_FALSE, (GLfloat *)&proj_view[0]));
        glCheck(glBindVertexArray(highlight_vao));
        glCheck(glBindBuffer(GL_ARRAY_BUFFER, highlight_buffer));
        std::vector<vec3> buf;
        BoundingBox box = bounding_box_for_voxel(targeted_block_pos);
        box.origin -= HIGHLIGHT_OFFSET;
        box.size += 2 * HIGHLIGHT_OFFSET;
        append_lines(box, &buf);
        glCheck(glBufferData(GL_ARRAY_BUFFER, sizeof(vec3) * buf.size(), buf.data(), GL_DYNAMIC_DRAW));
        glCheck(glDrawArrays(GL_LINES, 0, GLsizei(buf.size())));
    }

    if (debug_enabled) {
        canvas_render(canvas);
    }
}

u8 calculate_brightness(int current, ivec3 loc, const ThreeArray<u8> &light) {
    for (int d = 0; d < Direction_Count; d++) {
        auto n = direction_normals[d];
        int light_level = light.get({loc.x + n.x, loc.y + n.y, loc.z + n.z});
        // full strength sky light from above does not decay
        if (!(d == Direction_Up && light_level == SUN_LIGHT_LEVEL)) {
            light_level--;
        }
        if (light_level > current) {
            current = light_level;
        }
    }
    return current;
}

// set_voxel sets the voxel to the desired type and then updates lighting based on the new voxel
void GameWorld::set_voxel(ivec3 pos, BlockType bt) {
    auto obt = world->block_types.get({pos.x, pos.y, pos.z});
    bool add_opaque = is_transparent(obt) && is_opaque(bt);
    bool remove_opaque = is_opaque(obt) && is_transparent(bt);
    world->block_types.set({pos.x, pos.y, pos.z}, bt);
    mark_chunk_dirty(pos);
    // since we added or removed a block, we need to update all chunks containing
    // the neighbors of this block
    for (int d = 0; d < Direction_Count; d++) {
        auto p = pos + direction_normals[d];
        mark_chunk_dirty(p);
    }
    std::queue<ivec3> spreadLightQueue;

    if (remove_opaque) {
        if (world->block_light_sky.get({pos.x, pos.y + 1, pos.z}) == SUN_LIGHT_LEVEL) {
            // if the above block contains sky light, enqueue all empty blocks below this one
            // after setting them to have sky light in them
            for (int y = pos.y; y >= 0; y--) {
                auto p = ivec3(pos.x, y, pos.z);
                if (is_opaque(world->block_types.get({p.x, p.y, p.z}))) {
                    break;
                }
                world->block_light_sky.set({p.x, p.y, p.z}, SUN_LIGHT_LEVEL);
                mark_chunk_dirty(p);
                spreadLightQueue.push(p);
            }
        } else {
            // calculate light only for the now transparent block
            // then spread that light outward using the queue
            int light_level = calculate_brightness(0, pos, world->block_light_sky);
            world->block_light_sky.set({pos.x, pos.y, pos.z}, light_level);
            mark_chunk_dirty(pos);
            spreadLightQueue.push(pos);
        }
    } else if (add_opaque) {
        std::queue<ivec3> zeroLightQueue;
        zeroLightQueue.push(pos);
        while (!zeroLightQueue.empty()) {
            auto p = zeroLightQueue.front();
            zeroLightQueue.pop();
            int light_level = world->block_light_sky.get({p.x, p.y, p.z});
            world->block_light_sky.set({p.x, p.y, p.z}, 0);
            for (int d = 0; d < Direction_Count; d++) {
                auto np = p + direction_normals[d];
                if (!world->block_light_sky.contains({np.x, np.y, np.z}) || is_opaque(world->block_types.get({np.x, np.y, np.z}))) {
                    continue;
                }
                auto neighbor_light_level = world->block_light_sky.get({np.x, np.y, np.z});
                if (neighbor_light_level == 0) {
                    continue;
                }

                if (neighbor_light_level == SUN_LIGHT_LEVEL && np.x == pos.x && np.y < pos.y && np.z == pos.z) {
                    // we've added an opaque block, so there may be a column of sun light below the added block
                    // we want to zero this, not add this to the spread light queue, so we have special handling
                    // for this case
                    zeroLightQueue.push(np);
                    continue;
                }

                if (neighbor_light_level < light_level) {
                    // this neighbor is less than the current light level, so the light coming from the
                    // removed block may have been the cause, zero it out and we will re-calculate it later
                    zeroLightQueue.push(np);
                } else {
                    // the neighbor has more light than if it was caused by the current light level
                    // we should add it to the spread light queue
                    spreadLightQueue.push(np);
                }
            }
        }

        while (!spreadLightQueue.empty()) {
            auto p = spreadLightQueue.front();
            spreadLightQueue.pop();
            int light_level = world->block_light_sky.get({p.x, p.y, p.z});
            for (int d = 0; d < Direction_Count; d++) {
                auto np = p + direction_normals[d];
                if (world->block_light_sky.contains({np.x, np.y, np.z}) && is_transparent(world->block_types.get({np.x, np.y, np.z})) && world->block_light_sky.get({np.x, np.y, np.z}) < light_level - 1) {
                    world->block_light_sky.set({np.x, np.y, np.z}, light_level - 1);
                    mark_chunk_dirty(np);
                    spreadLightQueue.push(np);
                }
            }
        }
    }
}