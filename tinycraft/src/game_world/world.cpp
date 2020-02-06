struct Chunk {
    // location of this chunk in chunk coordinates
    // origin chunk is located at 0,0,0 in world coordinates and 0,0 in chunk coordinates
    // the chunk extends from the origin in the positive direction of
    // each axis, so the origin corresponds to the bottom NW corner of the chunk
    // the voxel at 0,0,0 will have its center appear at 0,0,0 in world coordinates
    u32 x_chunkspace = 0;
    u32 z_chunkspace = 0;
    GLuint vao = 0;
    GLuint buffer = 0;
    int num_triangles = 0;
    bool dirty = false;
    BoundingBox bounding_box_worldspace;
    // this stores a mesh generated on creation
    // it is not kept up to date as the chunk changes
    u8 *original_mesh = nullptr;
};

struct World {
    bool smooth_lighting = false;

    std::mt19937 rand_gen;

    ivec3 size = ivec3(0);
    std::unordered_map<ivec2, Chunk *> chunk_key_to_chunk;
    u8 chunk_mesh_buffer[MAX_CHUNK_MESH_BUFFER_SIZE];

    std::vector<BlockType> block_types_data;
    std::vector<u8> block_light_sky_data;

    // this is a view of block_types_data without the outer layer of blocks
    ThreeArray<BlockType> block_types;
    ThreeArray<u8> block_light_sky;

    vec3 player_start_pos = vec3(0.0f);

    u8vec4 *diffuse_color_map;
};

f32 perlin_noise(f32 x, f32 y, f32 z) {
    const f32 PERLIN_MAG = 0.866f;
    return stb_perlin_noise3(x, y, z, 0, 0, 0) + PERLIN_MAG;
}

bool is_transparent(u8 block_type) {
    return block_type == BlockType_Air;
}

bool is_opaque(u8 block_type) {
    return !is_transparent(block_type);
}

bool is_solid(u8 block_type) {
    return block_type != BlockType_Air;
}

f32 calculate_ambient_brightness(u8 light_level) {
    return pow(0.8f, f32(SUN_LIGHT_LEVEL - light_level));
}

ivec2 get_chunk_key_for_voxel(World *w, ivec3 pos) {
    // find the chunk that owns a particular voxel, the chunk must exist
    // voxels along the edge of the underlying world block grid do not belong to any chunk
    return ivec2(clamp(pos.x, 0, w->size.x - 1) / CHUNK_VOXELS_X, clamp(pos.z, 0, w->size.z - 1) / CHUNK_VOXELS_Z);
}

void place_tree(World *w, int base_x, int base_y, int base_z, int amount) {
    fassert(1 <= amount);
    const int trunk_height = amount;
    for (int y_offset = 0; y_offset < trunk_height; y_offset++) {
        w->block_types.set({base_x, base_y + y_offset, base_z}, BlockType_Log);
    }
    const int leaf_height = amount + 1;
    const f32 leaf_radius = f32(amount);
    auto center = vec3(base_x, base_y + trunk_height, base_z);
    for (int y_offset = 0; y_offset < leaf_height; y_offset++) {
        int limit = leaf_height - y_offset;
        for (int x_offset = -limit; x_offset <= limit; x_offset++) {
            for (int z_offset = -limit; z_offset <= limit; z_offset++) {
                auto p = ivec3(base_x + x_offset, base_y + trunk_height + y_offset, base_z + z_offset);
                if (distance(center, p) > leaf_radius) {
                    continue;
                }
                if (w->block_types.get(p) == BlockType_Air) {
                    w->block_types.set(p, BlockType_Leaves);
                }
            }
        }
    }
}

void append_quad(Buffer *buf, int x, int y, int z, u8 texture_id, u8vec4 diffuse_color, vec4 ambient_brightness, u8 d) {
    for (int i = 0; i < VERTICES_PER_TRIANGLE * 2; i++) {
        // in_pos_relvoxelspace
        auto v = voxel_cube_vertices[d * VERTICES_PER_TRIANGLE * 2 + i];
        // convert to fixed point representation where each dimension has 16 positions within each voxel
        buf->write_u32((int((x + v.x) * 16) << 0) + (int((y + v.y) * 16) << 9) + (int((z + v.z) * 16) << 22));

        // in_normal_index
        buf->write_u8(d);

        // in_diffuse_color
        buf->write_u32((diffuse_color.r << 0) + (diffuse_color.g << 8) + (diffuse_color.b << 16) + (diffuse_color.a << 24));

        // in_ambient_brightness
        // 0: 0, 3 (lower left)
        // 1: 1 (lower right)
        // 2: 2, 4 (upper right)
        // 3: 5 (upper left)
        int corner = Corner_LowerLeft;
        if (i == 1) {
            corner = Corner_LowerRight;
        } else if (i == 2 || i == 4) {
            corner = Corner_UpperRight;
        } else if (i == 5) {
            corner = Corner_UpperLeft;
        }
        buf->write_u8(u8(ambient_brightness[corner] * 255.0f));

        // in_tex_coord
        auto tc = voxel_tex_coords[i];
        buf->write_f32(f32(tc.s));
        // start in top left of texture rather than bottom left
        buf->write_f32(f32(tc.t));
        buf->write_f32(f32(texture_id));
    }
}

void calculate_light_sky(ThreeArray<u8> block_light_sky, const ThreeArray<BlockType> &block_types) {
    std::vector<u8> ground_level_data(block_types.shape[0] * block_types.shape[2]);
    auto ground_level = NDArray<u8, 2>::create_from_data(ground_level_data.data(), {block_types.shape[0], block_types.shape[2]});

    // process each column, light does not diminish if it has an unobstructed view to the sky
    for (int x = 0; x < block_types.shape[0]; x++) {
        for (int z = 0; z < block_types.shape[2]; z++) {
            for (int y = block_types.shape[1] - 1; y >= 0; y--) {
                auto opaque = is_opaque(block_types.get({x, y, z}));
                if (opaque) {
                    ground_level.set({x, z}, y);
                    break;
                }
                block_light_sky.set({x, y, z}, SUN_LIGHT_LEVEL);
            }
        }
    }

    // for every transparent below-ground block, if there's a non-zero light level next to it
    // we should add that neighbor to the queue
    std::unordered_set<ivec3> seen;
    std::queue<ivec3> spreadLightQueue;
    for (int x = 0; x < block_types.shape[0]; x++) {
        for (int z = 0; z < block_types.shape[2]; z++) {
            for (int y = ground_level.get({x, z}) - 1; y >= 0; y--) {
                if (is_opaque(block_types.get({x, y, z}))) {
                    continue;
                }
                auto pos = ivec3(x, y, z);
                for (int d = 0; d < Direction_Count; d++) {
                    auto np = pos + direction_normals[d];
                    auto light_level = block_light_sky.get({np.x, np.y, np.z});
                    if (light_level != 0) {
                        seen.insert(np);
                        spreadLightQueue.push(np);
                    }
                }
            }
        }
    }

    while (!spreadLightQueue.empty()) {
        auto p = spreadLightQueue.front();
        spreadLightQueue.pop();
        int light_level = block_light_sky.get(p);
        for (int d = 0; d < Direction_Count; d++) {
            auto np = p + direction_normals[d];
            if (block_light_sky.contains(np) && is_transparent(block_types.get(np)) && block_light_sky.get(np) < light_level - 1) {
                block_light_sky.set(np, light_level - 1);
                spreadLightQueue.push(np);
            }
        }
    }
}

void rebuild_chunk(World *w, Chunk *c) {
    // rebuild_chunk builds a mesh for a chunk in the world's chunk_mesh_buffer
    int start_x = (int)(c->x_chunkspace) * CHUNK_VOXELS_X;
    int start_z = (int)(c->z_chunkspace) * CHUNK_VOXELS_Z;
    auto chunk_block_types = w->block_types.window({start_x, 0, start_z}, {CHUNK_VOXELS_X, CHUNK_VOXELS_Y, CHUNK_VOXELS_Z});
    auto chunk_block_light_sky = w->block_light_sky.window({start_x, 0, start_z}, {CHUNK_VOXELS_X, CHUNK_VOXELS_Y, CHUNK_VOXELS_Z});
    auto buf = Buffer(w->chunk_mesh_buffer, NUM_ELEMS(w->chunk_mesh_buffer));

    int num_triangles = 0;
    for (int x = 0; x < chunk_block_types.shape[0]; x++) {
        for (int y = 0; y < chunk_block_types.shape[1]; y++) {
            for (int z = 0; z < chunk_block_types.shape[2]; z++) {
                auto bt = chunk_block_types.get({x, y, z});
                if (bt == BlockType_Air) {
                    continue;
                }
                auto cur = ivec3(x, y, z);
                for (int d = 0; d < Direction_Count; d++) {
                    auto n = cur + direction_normals[d];
                    auto nbt = chunk_block_types.get({n.x, n.y, n.z});
                    if (is_opaque(bt) && is_opaque(nbt)) {
                        continue;
                    }
                    auto neighbor_sky_light_level = chunk_block_light_sky.get({n.x, n.y, n.z});
                    auto neighbor_brightness = calculate_ambient_brightness(neighbor_sky_light_level);
                    auto ambient_brightness = vec4(neighbor_brightness);
                    if (w->smooth_lighting) {
                        for (int corner = Corner_LowerLeft; corner < Corner_Count; corner++) {
                            // find all next neighbors for this corner on the same plane as the neighbor n
                            // start off with the original neighbor's brightness
                            f32 total = neighbor_brightness;
                            for (int offset_idx = 0; offset_idx < 3; offset_idx++) {
                                auto nn = n + planar_neighbor_offsets[d][corner][offset_idx];
                                auto brightness = calculate_ambient_brightness(chunk_block_light_sky.get({nn.x, nn.y, nn.z}));
                                total += brightness;
                            }
                            ambient_brightness[corner] = total / 4.0f;
                        }
                    }
                    auto diffuse_color = w->diffuse_color_map[bt * Direction_Count + d];
                    auto tex_id = block_type_and_direction_to_texture[bt][d];
                    auto bytes_written_before = buf.bytes_written();
                    append_quad(&buf, x, y, z, tex_id, diffuse_color, ambient_brightness, d);
                    dassert(buf.bytes_written() - bytes_written_before == BYTES_PER_QUAD);
                    num_triangles += 2;
                }
            }
        }
    }
    c->num_triangles = num_triangles;
}

World *build_world(ivec3 size, u32 seed, u8vec4 *diffuse_color_map, bool smooth_lighting) {
    auto w = new World();
    w->size = size;
    w->rand_gen.seed(seed);
    w->diffuse_color_map = diffuse_color_map;
    w->smooth_lighting = smooth_lighting;

    // add 2 border blocks in each direction;
    int block_count = (w->size.x + 2) * (w->size.y + 2) * (w->size.z + 2);
    w->block_types_data.resize(block_count);
    w->block_light_sky_data.resize(block_count);

    auto raw_block_types = ThreeArray<BlockType>::create_from_data(w->block_types_data.data(), {w->size.x + 2, w->size.y + 2, w->size.z + 2});
    w->block_types = raw_block_types.window({1, 1, 1}, {w->size.x, w->size.y, w->size.z});

    auto raw_block_light_sky = ThreeArray<u8>::create_from_data(w->block_light_sky_data.data(), {w->size.x + 2, w->size.y + 2, w->size.z + 2});
    raw_block_light_sky.fill(SUN_LIGHT_LEVEL);
    w->block_light_sky = raw_block_light_sky.window({1, 1, 1}, {w->size.x, w->size.y, w->size.z});
    w->block_light_sky.fill(0);

    std::vector<u8> ground_level_data(w->block_types.shape[0] * w->block_types.shape[2]);
    auto ground_level = NDArray<u8, 2>::create_from_data(ground_level_data.data(), {w->block_types.shape[0], w->block_types.shape[2]});

    {
        f32 perlin_offset = rand_f32(w->rand_gen, 0.0f, 1.0f);
        for (int x = 0; x < w->size.x; x++) {
            for (int z = 0; z < w->size.z; z++) {
                f32 height = perlin_noise((f32)(x) / 50, perlin_offset, (f32)(z) / 50);
                i32 ground_y = clamp((i32)(round(pow(height, 0.3) * 50)), 0, w->size.y);
                ground_level.set({x, z}, ground_y);
                for (int y = 0; y < w->size.y; y++) {
                    if (y <= ground_y) {
                        enum BlockType bt;
                        int depth = ground_y - y;
                        if (depth == 0) {
                            bt = BlockType_Grass;
                        } else if (depth >= 4) {
                            bt = BlockType_Stone;
                        } else {
                            bt = BlockType_Dirt;
                        }
                        w->block_types.set({x, y, z}, bt);
                    }
                }
            }
        }
    }

    {
        // randomly place some trees
        for (int i = 0; i < w->size.x * w->size.z / 100; i++) {
            int x = rand_u32(w->rand_gen, 10, w->size.x - 10);
            int z = rand_u32(w->rand_gen, 10, w->size.z - 10);
            if (ground_level.contains({x, z})) {
                place_tree(w, x, ground_level.get({x, z}) + 1, z, rand_u32(w->rand_gen, 2, 5));
            }
        }
    }

    calculate_light_sky(w->block_light_sky, w->block_types);

    for (int x = 0; x < w->size.x / CHUNK_VOXELS_X; x++) {
        for (int z = 0; z < w->size.z / CHUNK_VOXELS_Z; z++) {
            auto c = new Chunk();
            c->x_chunkspace = x;
            c->z_chunkspace = z;
            c->bounding_box_worldspace = {vec3(x * CHUNK_VOXELS_X, 0, z * CHUNK_VOXELS_Z) - vec3(0.5f), vec3((x + 1) * CHUNK_VOXELS_X, CHUNK_VOXELS_Y, (z + 1) * CHUNK_VOXELS_Z) - vec3(0.5f)};
            rebuild_chunk(w, c);
            c->original_mesh = new u8[c->num_triangles * BYTES_PER_TRIANGLE];
            memcpy(c->original_mesh, w->chunk_mesh_buffer, c->num_triangles * BYTES_PER_TRIANGLE);
            w->chunk_key_to_chunk[ivec2(c->x_chunkspace, c->z_chunkspace)] = c;
        }
    }

    {
        while (true) {
            int x = rand_u32(w->rand_gen, w->size.x / 2 - 10, w->size.x / 2 + 10);
            int z = rand_u32(w->rand_gen, w->size.z / 2 - 10, w->size.z / 2 + 10);
            w->player_start_pos = vec3(f32(x), ground_level.get({x, z}) + 5.0f, f32(z));
            if (!is_solid(w->block_types.get(w->player_start_pos))) {
                break;
            }
        }
    }
    return w;
}

void destroy_chunk(Chunk *c) {
    delete[] c->original_mesh;
}