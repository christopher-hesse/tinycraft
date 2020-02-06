const size_t IMAGE_SIZE = 1 << 10;

Image create_random_image(u32 width, u32 height) {
    Image r = image_create(width, height);
    for (size_t i = 0; i < width * height * IMAGE_BYTES_PER_PIXEL; i++) {
        r.data[i] = rand() % 255;
    }
    return r;
}

TEST(Image, DiffImage) {
    auto empty = image_create(2, 3);
    auto red = image_fill_color(empty, vec4(1.0, 0.0, 0.0, 1.0));
    auto green = image_fill_color(empty, vec4(0.0, 1.0, 0.0, 1.0));
    // 2 channels are different by 1.0, and average is per channel
    ASSERT_LE(abs(image_diff(red, green) - 2.0 / 4.0), 0.000001);
}

TEST(Image, LoadSave) {
    auto img_orig = image_load(get_resource_path("test_sprite.png"));
    std::string output_filepath = get_output_path("test_image.png");
    image_save(img_orig, output_filepath);
    auto img_loaded = image_load(output_filepath);
    ASSERT_LE(image_diff(img_orig, img_loaded), 0.0);
}

TEST(Image, Resize) {
    auto img_orig = image_load(get_resource_path("test_cat.png"));
    auto small_img = image_resize(img_orig, 100, 100);
    auto enlarged_img = image_resize(small_img, img_orig.width, img_orig.height);
    auto small_img2 = image_resize(enlarged_img, 100, 100);
    ASSERT_LE(image_diff(small_img, small_img2), 0.02);
}

TEST(Image, Alpha) {
    auto img_orig = image_load(get_resource_path("test_cat.png"));
    auto img_orig_copy = image_copy(img_orig);
    auto unmultiplied_img = image_divide_alpha(img_orig);
    ASSERT_LT(image_diff(img_orig, img_orig_copy), 0.0001);
    ASSERT_GT(image_diff(img_orig, unmultiplied_img), 0.01);
    auto multiplied_img = image_multiply_alpha(unmultiplied_img);
    ASSERT_LT(image_diff(img_orig, multiplied_img), 0.0001);
}

TEST(Image, ResizeAlpha) {
    auto img = image_create(2, 1);
    auto data = (u32 *)(img.data);
    // 0xAABBGGRR
    data[0] = 0xFF0000FF;
    data[1] = 0x2000FF00;
    img = image_multiply_alpha(img);
    auto out = image_resize(img, 1, 1);
    out = image_divide_alpha(out);
    // this is srgb space, we also clamp the R value to FF
    // the value is just the one returned by stb_image_resize, it's not calcuated to be correct
    // a more reasonable value can be found here:
    // https://developer.nvidia.com/content/alpha-blending-pre-or-not-pre
    // but this doesn't seem to assume srgb
    ASSERT_EQ(*(u32 *)(out.data), 0x90001CE3);
}

TEST(Image, CompositeLooksCorrect) {
    Image A = create_random_image(128, 128);
    Image B = create_random_image(128, 128);
    A = image_multiply_alpha(A);
    B = image_multiply_alpha(B);
    Image A_orig = image_copy(A);
    Image B_orig = image_copy(B);

    image_composite(&B, &A, 0, 0, COMPOSITE_CPU_SLOW);

    for (size_t i = 0; i < A.width * A.height; i++) {
        size_t base = i * IMAGE_BYTES_PER_PIXEL;

        f32 a_red = A_orig.data[base + 0] / 255.0f;
        f32 a_blue = A_orig.data[base + 1] / 255.0f;
        f32 a_green = A_orig.data[base + 2] / 255.0f;
        f32 a_alpha = A_orig.data[base + 3] / 255.0f;

        f32 b_red = B_orig.data[base + 0] / 255.0f;
        f32 b_blue = B_orig.data[base + 1] / 255.0f;
        f32 b_green = B_orig.data[base + 2] / 255.0f;
        f32 b_alpha = B_orig.data[base + 3] / 255.0f;

        f32 o_alpha = a_alpha + (1.0f - a_alpha) * b_alpha;

        f32 o_red = (a_red + b_red * (1 - a_alpha));
        f32 o_blue = (a_blue + b_blue * (1 - a_alpha));
        f32 o_green = (a_green + b_green * (1 - a_alpha));

        ASSERT_EQ(A.data[base + 0], A_orig.data[base + 0]);
        ASSERT_EQ(A.data[base + 1], A_orig.data[base + 1]);
        ASSERT_EQ(A.data[base + 2], A_orig.data[base + 2]);
        ASSERT_EQ(A.data[base + 3], A_orig.data[base + 3]);

        ASSERT_EQ(B.data[base + 0], round(o_red * 255));
        ASSERT_EQ(B.data[base + 1], round(o_blue * 255));
        ASSERT_EQ(B.data[base + 2], round(o_green * 255));
        ASSERT_EQ(B.data[base + 3], round(o_alpha * 255));
    }
}

TEST(Image, Composite) {
    auto src_img = image_load(get_resource_path("test_sprite.png"));
    src_img = image_resize(src_img, 64, 64);

    auto draw_sprites = [=](Image *dst_img, enum COMPOSITE_MODE comp_mode) {
        for (f32 y = -(f32)(dst_img->height); y <= IMAGE_SIZE; y += 33) {
            for (f32 x = -(f32)(dst_img->width); x <= IMAGE_SIZE; x += 33) {
                image_composite(dst_img, &src_img, x, y, comp_mode);
            }
        }
    };

    auto blank_image = image_create(src_img.width * 2, src_img.height * 2);
    auto dst_img_ref = image_copy(blank_image);
    draw_sprites(&dst_img_ref, COMPOSITE_CPU_SLOW);

    {
        auto dst_img = image_copy(blank_image);
        draw_sprites(&dst_img, COMPOSITE_CPU_ASM);
        ASSERT_LT(image_diff(dst_img_ref, dst_img), 0.0001);
    }
}