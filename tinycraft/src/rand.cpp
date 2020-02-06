// rand_u32 returns a uniform random u32 in the range [min, max)
u32 rand_u32(std::mt19937 &gen, u32 min, u32 max) {
    fassert(min < max);
    u32 range = max - min;
    u32 val = gen();
    // this result will be biased since
    // if val / range != 0
    // lower numbers will be more likely
    return (val % range) + min;
}

// rand_f32 returns a uniform random f32 in the range [min, max)
f32 rand_f32(std::mt19937 &gen, f32 min, f32 max) {
    fassert(min < max);
    f32 range = max - min;
    f32 val = (f32)((f64)(gen()) / ((f64)(gen.max()) + 1));
    return val * range + min;
}

auto seeded_generator() {
    std::vector<unsigned int> random_data(std::mt19937::state_size);
    std::random_device source;
    std::generate(begin(random_data), end(random_data), [&]() { return source(); });
    std::seed_seq seeds(begin(random_data), end(random_data));
    std::mt19937 gen(seeds);
    return gen;
}