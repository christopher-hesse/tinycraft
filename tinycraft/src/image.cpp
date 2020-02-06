const u8 IMAGE_NUM_CHAN = 4;
const u8 IMAGE_BYTES_PER_PIXEL = 4;

enum COMPOSITE_MODE {
    COMPOSITE_CPU_SLOW,
    COMPOSITE_CPU_ASM,
    COMPOSITE_CPU_IPPI,
    COMPOSITE_CPU_COPY,
};

struct Image {
    u8 *data;
    u32 width;
    u32 height;
    u32 stride;
};

struct Rect {
    f32 x;
    f32 y;
    f32 w;
    f32 h;
};

vec4 u32_to_vec4(u32 p) {
    vec4 r(
        (f32)((p >> 0) & 0xFF),
        (f32)((p >> 8) & 0xFF),
        (f32)((p >> 16) & 0xFF),
        (f32)((p >> 24) & 0xFF));
    return r / 255.0f;
}

u32 vec4_to_u32(vec4 p) {
    p *= 255.0f;
    u32 r = ((u8)(p.r + 0.5f) << 0) |
            ((u8)(p.g + 0.5f) << 8) |
            ((u8)(p.b + 0.5f) << 16) |
            ((u8)(p.a + 0.5f) << 24);
    return r;
}

Image image_from_buffer(void *buf, u32 width, u32 height, u32 stride) {
    Image r;
    r.data = (u8 *)(buf);
    r.width = width;
    r.height = height;
    r.stride = stride;
    return r;
}

Image image_create(u32 width, u32 height) {
    Image r;
    size_t size = width * height * IMAGE_BYTES_PER_PIXEL;
    r.data = (u8 *)(malloc(size));
    memset(r.data, 0xFF, size);
    r.width = width;
    r.height = height;
    r.stride = width * IMAGE_BYTES_PER_PIXEL;
    return r;
}

void image_destroy(const Image &im) {
    free(im.data);
}

vec4 image_get_pixel(const Image &in, ivec2 pos) {
    auto buf = (u32 *)(&in.data[pos.y * in.stride + pos.x * IMAGE_BYTES_PER_PIXEL]);
    return u32_to_vec4(*buf);
}

void image_set_pixel(Image *in, ivec2 pos, vec4 px) {
    auto buf = (u32 *)(&in->data[pos.y * in->stride + pos.x * IMAGE_BYTES_PER_PIXEL]);
    *buf = vec4_to_u32(px);
}

Image image_copy(const Image &in) {
    Image out = image_create(in.width, in.height);
    for (size_t y = 0; y < in.height; y++) {
        for (size_t x = 0; x < in.width; x++) {
            auto px = image_get_pixel(in, ivec2(x, y));
            image_set_pixel(&out, ivec2(x, y), px);
        }
    }
    return out;
}

Image image_sub(const Image &in, ivec2 p, ivec2 size) {
    return image_from_buffer(in.data + p.x * IMAGE_BYTES_PER_PIXEL + p.y * in.stride, size.x, size.y, in.stride);
}

Image image_fill_color(const Image &in, vec4 color) {
    Image out = image_copy(in);
    for (size_t y = 0; y < in.height; y++) {
        for (size_t x = 0; x < in.width; x++) {
            image_set_pixel(&out, ivec2(x, y), color);
        }
    }
    return out;
}

void image_fill_color_in_place(Image *in_out, vec4 color) {
    u32 c = vec4_to_u32(color);
    u8 *row = in_out->data;
    for (size_t y = 0; y < in_out->height; y++) {
        auto pixels = (u32 *)(row);
        for (size_t x = 0; x < in_out->width; x++) {
            pixels[x] = c;
        }
        row += in_out->stride;
    }
}

Image image_flip_y(const Image &in) {
    auto out = image_create(in.width, in.height);
    for (size_t y = 0; y < in.height; y++) {
        for (size_t x = 0; x < in.width; x++) {
            auto px = image_get_pixel(in, ivec2(x, y));
            image_set_pixel(&out, ivec2(x, in.height - 1 - y), px);
        }
    }
    return out;
}

Image image_multiply_alpha(const Image &in) {
    auto out = image_create(in.width, in.height);
    for (size_t y = 0; y < in.height; y++) {
        for (size_t x = 0; x < in.width; x++) {
            auto in_px = image_get_pixel(in, ivec2(x, y));
            auto out_px = vec4(in_px.r * in_px.a, in_px.g * in_px.a, in_px.b * in_px.a, in_px.a);
            image_set_pixel(&out, ivec2(x, y), out_px);
        }
    }
    return out;
}

Image image_divide_alpha(const Image &in) {
    Image out = image_create(in.width, in.height);
    for (size_t y = 0; y < in.height; y++) {
        for (size_t x = 0; x < in.width; x++) {
            auto in_px = image_get_pixel(in, ivec2(x, y));
            vec4 out_px;
            if (in_px.a == 0.0f) {
                out_px = vec4(0.0f, 0.0f, 0.0f, 0.0f);
            } else {
                out_px = vec4(in_px.r / in_px.a, in_px.g / in_px.a, in_px.b / in_px.a, in_px.a);
            }
            out_px = clamp(out_px, 0.0f, 1.0f);
            image_set_pixel(&out, ivec2(x, y), out_px);
        }
    }
    return out;
}

Image image_load(const std::string path) {
    int width, height, comp;
    Image tmp;
    tmp.data = stbi_load(path.c_str(), &width, &height, &comp, IMAGE_NUM_CHAN);
    if (tmp.data == nullptr) {
        fatal("failed to load image path=%s", path.c_str());
    }
    if (comp != IMAGE_NUM_CHAN) {
        fatal("incorrect number of channels got=%d expected=%d", comp, IMAGE_NUM_CHAN);
    }
    tmp.width = width;
    tmp.height = height;
    tmp.stride = width * IMAGE_BYTES_PER_PIXEL;
    Image out = image_multiply_alpha(tmp);
    stbi_image_free(tmp.data);
    return out;
}

void image_save(const Image &img, const std::string path) {
    auto out = image_divide_alpha(img);
    auto result = stbi_write_png(path.c_str(), out.width, out.height, IMAGE_NUM_CHAN, out.data, out.stride);
    if (result == 0) {
        fatal("failed to save file to path=%s", path.c_str());
    }
    image_destroy(out);
}

Image image_resize(const Image &in, u32 width, u32 height) {
    auto out = image_create(width, height);
    auto result = stbir_resize_uint8_generic(
        (unsigned char *)(in.data), in.width, in.height, in.stride,
        (unsigned char *)(out.data), out.width, out.height, out.stride, IMAGE_NUM_CHAN, 3, STBIR_FLAG_ALPHA_PREMULTIPLIED, STBIR_EDGE_CLAMP, STBIR_FILTER_DEFAULT, STBIR_COLORSPACE_LINEAR, NULL);
    if (result == 0) {
        fatal("failed to resize");
    }
    // when resizing in rgb color space, we get premultiplied alpha values that are impossible
    // for instance the (premultiplied) red channel > alpha channel
    // clamp values to be valid premultiplied values
    for (size_t y = 0; y < out.height; y++) {
        for (size_t x = 0; x < out.width; x++) {
            auto in_px = image_get_pixel(out, ivec2(x, y));
            auto out_px = clamp(in_px, 0.0f, in_px.a);
            image_set_pixel(&out, ivec2(x, y), out_px);
        }
    }
    return out;
}

Image image_remove_alpha(const Image &in) {
    Image out = image_create(in.width, in.height);
    for (size_t y = 0; y < in.height; y++) {
        for (size_t x = 0; x < in.width; x++) {
            auto in_px = image_get_pixel(in, ivec2(x, y));
            auto out_px = vec4(in_px.r, in_px.g, in_px.b, 1.0f);
            image_set_pixel(&out, ivec2(x, y), out_px);
        }
    }
    return out;
}

f32 image_diff(const Image &a, const Image &b) {
    fassert(a.width == b.width);
    fassert(a.height == b.height);
    f32 diff = 0.0f;
    for (size_t y = 0; y < a.height; y++) {
        for (size_t x = 0; x < a.width; x++) {
            auto a_px = image_get_pixel(a, ivec2(x, y));
            auto b_px = image_get_pixel(b, ivec2(x, y));
            auto px_diff = abs(a_px - b_px);
            diff += dot(px_diff, vec4(1));
        }
    }
    return diff / (f32)(a.width * a.height * IMAGE_NUM_CHAN);
}

void image_convert_to_rgb(const Image &in, void *out_buf) {
    auto in_row = (u8 *)(in.data);
    auto out_row = (u8 *)(out_buf);
    for (u32 y = 0; y < in.height; y += 1) {
        auto in_pixels = (u32 *)(in_row);
        auto out_channels = (u8 *)(out_row);
        for (u32 x = 0; x < in.width; x += 1) {
            u32 px = *in_pixels;
            *out_channels = (px & 0x000000FF) >> 0;
            out_channels++;
            *out_channels = (px & 0x0000FF00) >> 8;
            out_channels++;
            *out_channels = (px & 0x00FF0000) >> 16;
            out_channels++;
            in_pixels++;
        }
        in_row += in.stride;
        out_row += in.width * 3;
    }
}

Rect rect_create(f32 x, f32 y, f32 width, f32 height) {
    Rect r;
    r.x = x;
    r.y = y;
    r.w = width;
    r.h = height;
    return r;
}

Rect rect_intersect(Rect a, Rect b) {
    f32 min_x = std::max(b.x, a.x);
    f32 min_y = std::max(b.y, a.y);
    f32 max_x = std::min(a.x + a.w, b.x + b.w);
    f32 max_y = std::min(a.y + a.h, b.y + b.h);
    if (min_x >= max_x || min_y >= max_y) {
        return rect_create(0.0f, 0.0f, 0.0f, 0.0f);
    }
    return rect_create(min_x, min_y, max_x - min_x, max_y - min_y);
}

#if defined(__AVX2__)
#define avx_fvec __m256
#define avx_ivec __m256i
#define set1_ps _mm256_set1_ps
#define set1_epi32 _mm256_set1_epi32
#define load_si _mm256_load_si256
#define store_si _mm256_store_si256
#define cvtepi32_ps _mm256_cvtepi32_ps
#define and_si _mm256_and_si256
#define andnot_si _mm256_andnot_si256
#define or_si _mm256_or_si256
#define srai_epi32 _mm256_srai_epi32
#define mul_ps _mm256_mul_ps
#define sub_ps _mm256_sub_ps
#define fmadd_ps _mm256_fmadd_ps
#define div_ps _mm256_div_ps
#define rcp_ps _mm256_rcp_ps
#define sqrt_ps _mm256_sqrt_ps
#define cvtps_epi32 _mm256_cvtps_epi32
#define slli_epi32 _mm256_slli_epi32
#define slli_si _mm256_slli_si256
#else
#define avx_fvec __m128
#define avx_ivec __m128i
#define set1_ps _mm_set1_ps
#define set1_epi32 _mm_set1_epi32
#define load_si _mm_load_si128
#define store_si _mm_store_si128
#define cvtepi32_ps _mm_cvtepi32_ps
#define and_si _mm_and_si128
#define andnot_si _mm_andnot_si128
#define or_si _mm_or_si128
#define srai_epi32 _mm_srai_epi32
#define mul_ps _mm_mul_ps
#define sub_ps _mm_sub_ps
#define fmadd_ps _mm_fmadd_ps
#define div_ps _mm_div_ps
#define rcp_ps _mm_rcp_ps
#define sqrt_ps _mm_sqrt_ps
#define cvtps_epi32 _mm_cvtps_epi32
#define slli_epi32 _mm_slli_epi32
#define slli_si _mm_slli_si128
#endif

void image_composite(Image *dst, const Image *src, f32 target_x, f32 target_y, enum COMPOSITE_MODE comp_mode) {
    // intersect rects in dst image coordinates
    auto src_rect = rect_create(target_x, target_y, (f32)(src->width), (f32)(src->height));
    auto dst_rect = rect_create(0.0f, 0.0f, (f32)(dst->width), (f32)(dst->height));
    auto rect = rect_intersect(src_rect, dst_rect);
    auto min_x = (size_t)(roundf(rect.x));
    auto min_y = (size_t)(roundf(rect.y));
    auto max_x = (size_t)(roundf(rect.x + rect.w));
    auto max_y = (size_t)(roundf(rect.y + rect.h));
    if (min_x >= max_x || min_y >= max_y) {
        return;
    }

    auto dst_row = dst->data + (min_y * dst->width + min_x) * IMAGE_BYTES_PER_PIXEL;
    auto src_x = min_x - (size_t)(round(target_x));
    auto src_y = min_y - (size_t)(round(target_y));
    auto src_row = src->data + (src_y * src->width + src_x) * IMAGE_BYTES_PER_PIXEL;

#if defined(__GNUC__)
    // we don't support asm for GCC yet
    if (comp_mode == COMPOSITE_CPU_ASM) {
        comp_mode = COMPOSITE_CPU_SLOW;
    }
#endif

    if (comp_mode == COMPOSITE_CPU_SLOW) {
        for (size_t y = min_y; y < max_y; y++) {
            auto dst_pixels = (u32 *)(dst_row);
            auto src_pixels = (u32 *)(src_row);
            for (size_t x = min_x; x < max_x; x++) {
                auto src_pixel = u32_to_vec4(*src_pixels);
                auto dst_pixel = u32_to_vec4(*dst_pixels);
                auto output_pixel = src_pixel + dst_pixel * (1.0f - src_pixel.a);
                *dst_pixels = vec4_to_u32(output_pixel);
                dst_pixels++;
                src_pixels++;
            }
            dst_row += dst->stride;
            src_row += src->stride;
        }
    } else if (comp_mode == COMPOSITE_CPU_COPY) {
        for (size_t y = min_y; y < max_y; y++) {
            auto dst_pixels = (u32 *)(dst_row);
            auto src_pixels = (u32 *)(src_row);
            for (size_t x = min_x; x < max_x; x++) {
                *dst_pixels = *src_pixels;
                dst_pixels++;
                src_pixels++;
            }
            dst_row += dst->stride;
            src_row += src->stride;
        }
    } else if (comp_mode == COMPOSITE_CPU_ASM) {
#if defined(__GNUC__)
        fatal("asm not supported for gcc");
#else
        const size_t pixels_per_loop = sizeof(avx_ivec) / IMAGE_BYTES_PER_PIXEL;

        avx_fvec one = set1_ps(1.0f);
        avx_fvec inv_255 = set1_ps(1.0f / 255.0f);
        avx_ivec mask_ff = set1_epi32(0xFF);

        // setup a mask for the end of each row so we can avoid overwriting pixels on the next row
        u32 m[pixels_per_loop] = {0xFFFFFFFF};
        avx_ivec mask_pixel = load_si((const avx_ivec *)(m));
        size_t end_of_row_pixel_count = (max_x - min_x) % pixels_per_loop;
        avx_ivec end_of_row_pixel_mask = set1_epi32(0);
        if (end_of_row_pixel_count > 0) {
            for (int i = pixels_per_loop - 1; i >= 0; i--) {
                end_of_row_pixel_mask = slli_si(end_of_row_pixel_mask, 4);
                if (i < end_of_row_pixel_count) {
                    end_of_row_pixel_mask = or_si(end_of_row_pixel_mask, mask_pixel);
                }
            }
        }

        for (size_t y = min_y; y < max_y; y++) {
            auto dst_pixels = (avx_ivec *)(dst_row);
            auto src_pixels = (avx_ivec *)(src_row);
            for (size_t x = min_x; x < max_x; x += pixels_per_loop) {
                bool last_loop = x + pixels_per_loop > max_x;

                // load pixels_per_loop pixels from each buffer
                avx_ivec src_pixel_u32 = load_si(src_pixels);
                avx_ivec dst_pixel_u32 = load_si(dst_pixels);

                // collect each channel into a different vec and convert to float
                avx_fvec src_pixel_r = cvtepi32_ps(and_si(src_pixel_u32, mask_ff));
                avx_fvec src_pixel_g = cvtepi32_ps(and_si(srai_epi32(src_pixel_u32, 8), mask_ff));
                avx_fvec src_pixel_b = cvtepi32_ps(and_si(srai_epi32(src_pixel_u32, 16), mask_ff));
                avx_fvec src_pixel_a = cvtepi32_ps(and_si(srai_epi32(src_pixel_u32, 24), mask_ff));

                avx_fvec dst_pixel_r = cvtepi32_ps(and_si(dst_pixel_u32, mask_ff));
                avx_fvec dst_pixel_g = cvtepi32_ps(and_si(srai_epi32(dst_pixel_u32, 8), mask_ff));
                avx_fvec dst_pixel_b = cvtepi32_ps(and_si(srai_epi32(dst_pixel_u32, 16), mask_ff));
                avx_fvec dst_pixel_a = cvtepi32_ps(and_si(srai_epi32(dst_pixel_u32, 24), mask_ff));

                avx_fvec src_pixel_a_norm = mul_ps(src_pixel_a, inv_255);

                // calculate output alpha
                avx_fvec inv_src_pixel_a_norm = sub_ps(one, src_pixel_a_norm);
                avx_fvec out_pixel_a = fmadd_ps(dst_pixel_a, inv_src_pixel_a_norm, src_pixel_a);

                // produce unscaled output pixel
                avx_fvec out_pixel_r = fmadd_ps(dst_pixel_r, inv_src_pixel_a_norm, src_pixel_r);
                avx_fvec out_pixel_g = fmadd_ps(dst_pixel_g, inv_src_pixel_a_norm, src_pixel_g);
                avx_fvec out_pixel_b = fmadd_ps(dst_pixel_b, inv_src_pixel_a_norm, src_pixel_b);

                // convert to int and mask + shift to reassemble the pixels
                avx_ivec out_pixel_u32 = and_si(cvtps_epi32(out_pixel_r), mask_ff);
                out_pixel_u32 = or_si(out_pixel_u32, slli_epi32(and_si(cvtps_epi32(out_pixel_g), mask_ff), 8));
                out_pixel_u32 = or_si(out_pixel_u32, slli_epi32(and_si(cvtps_epi32(out_pixel_b), mask_ff), 16));
                out_pixel_u32 = or_si(out_pixel_u32, slli_epi32(and_si(cvtps_epi32(out_pixel_a), mask_ff), 24));

                // if we go past the end of the line, combine out_pixel with dst_pixel
                if (last_loop) {
                    avx_ivec masked_out_pixels = and_si(end_of_row_pixel_mask, out_pixel_u32);
                    avx_ivec masked_dst_pixels = andnot_si(end_of_row_pixel_mask, dst_pixel_u32);
                    out_pixel_u32 = or_si(masked_out_pixels, masked_dst_pixels);
                }

                store_si(dst_pixels, out_pixel_u32);

                dst_pixels++;
                src_pixels++;
            }
            dst_row += dst->stride;
            src_row += src->stride;
        }
#endif
    } else if (comp_mode == COMPOSITE_CPU_IPPI) {
#if defined(IPPI)
        // https://software.intel.com/en-us/ipp-dev-reference-alphacomp
        IppiSize roiSize;
        roiSize.width = (u32)(max_x - min_x);
        roiSize.height = (u32)(max_y - min_y);
        auto st = ippiAlphaComp_8u_AC4IR(src_row, src->stride, dst_row, dst->stride, roiSize, ippAlphaOverPremul);
        if (st != ippStsNoErr) {
            fatal("ippi call failed %s", ippGetStatusString(st));
        }
#else
        fatal("no ippi support");
#endif
    } else {
        fatal("invalid composite mode");
    }
}