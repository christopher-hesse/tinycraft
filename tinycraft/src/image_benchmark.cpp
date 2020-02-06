const size_t IMAGE_SIZE = 1 << 10;

void BM_memcpy(benchmark::State &state) {
    auto src_img = image_create(IMAGE_SIZE, IMAGE_SIZE);
    auto dst_img = image_create(IMAGE_SIZE, IMAGE_SIZE);
    size_t size = IMAGE_SIZE * IMAGE_SIZE * IMAGE_BYTES_PER_PIXEL;
    for (auto _ : state) {
        benchmark::DoNotOptimize(memcpy(dst_img.data, src_img.data, size));
    }
}
BENCHMARK(BM_memcpy);

void BM_draw_bitmap_single(benchmark::State &state, enum COMPOSITE_MODE comp_mode) {
    auto src_img = image_create(IMAGE_SIZE, IMAGE_SIZE);
    auto dst_img = image_create(IMAGE_SIZE, IMAGE_SIZE);
    for (auto _ : state) {
        image_composite(&dst_img, &src_img, 0, 0, comp_mode);
    }
}
BENCHMARK_CAPTURE(BM_draw_bitmap_single, cpu_copy, COMPOSITE_CPU_COPY);
BENCHMARK_CAPTURE(BM_draw_bitmap_single, cpu_slow, COMPOSITE_CPU_SLOW);
BENCHMARK_CAPTURE(BM_draw_bitmap_single, cpu_asm, COMPOSITE_CPU_ASM);
#if defined(IPPI)
BENCHMARK_CAPTURE(BM_draw_bitmap_single, cpu_ippi, COMPOSITE_CPU_IPPI);
#endif

void BM_draw_bitmap_multiple(benchmark::State &state, enum COMPOSITE_MODE comp_mode) {
    auto src_img = image_load(get_resource_path("test_sprite.png"));
    src_img = image_resize(src_img, 64, 64);
    auto dst_img = image_create(IMAGE_SIZE, IMAGE_SIZE);
    for (auto _ : state) {
        for (f32 y = 0.0f; y < IMAGE_SIZE; y += src_img.height) {
            for (f32 x = 0.0f; x < IMAGE_SIZE; x += src_img.width) {
                image_composite(&dst_img, &src_img, x, y, comp_mode);
            }
        }
    }
}
BENCHMARK_CAPTURE(BM_draw_bitmap_multiple, cpu_copy, COMPOSITE_CPU_COPY);
BENCHMARK_CAPTURE(BM_draw_bitmap_multiple, cpu_slow, COMPOSITE_CPU_SLOW);
BENCHMARK_CAPTURE(BM_draw_bitmap_multiple, cpu_asm, COMPOSITE_CPU_ASM);
#if defined(IPPI)
BENCHMARK_CAPTURE(BM_draw_bitmap_multiple, cpu_ippi, COMPOSITE_CPU_IPPI);
#endif