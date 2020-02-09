const f32 EPSILON = 1e-5f;

f32 distance(vec3 v, vec3 w) {
    f32 a = v.x - w.x;
    f32 b = v.y - w.y;
    f32 c = v.z - w.z;
    return sqrt(a * a + b * b + c * c);
}

f32 magnitude(vec3 v) {
    return sqrt(dot(v, v));
}

    f32 calculated_closeness(vec3 start, vec3 end, vec3 current) {
        return 1.0f - magnitude(current - end) / magnitude(start - end);
    }

bool box_contains(ivec3 pos, ivec3 lower, ivec3 upper) {
    return lower.x <= pos.x && pos.x < upper.x && lower.y <= pos.y && pos.y < upper.y && lower.z <= pos.z && pos.z < upper.z;
}

bool line_intersects_sphere(vec3 line_origin, vec3 line_direction, vec3 sphere_origin, f32 sphere_radius, bool forward_only) {
    // https://en.wikipedia.org/wiki/Line%E2%80%93sphere_intersection
    // l = line_direction
    // o = line_origin
    // c = sphere_origin
    f32 a = dot(line_direction, line_origin - sphere_origin);
    vec3 b = line_origin - sphere_origin;
    f32 test = a * a - (dot(b, b) - sphere_radius * sphere_radius);
    if (test < 0.0f) {
        return false;
    }
    if (!forward_only) {
        return true;
    }
    // we now need to check if the intersection is in front of the origin
    // there are two distances, -a +/- sqrt(test)
    // we only need to check the first distance since the second one will be less than the first
    f32 d = -a + sqrt(test);
    return d >= 0.0f;
}

vec3 normalize(vec3 v) {
    return v / magnitude(v);
}

bool line_intersects_plane(const vec4 &plane, const vec4 &line_origin, const vec4 &line_direction, f32 *out_signed_dist, vec3 *out_point) {
    f32 denominator = dot(plane, line_direction);
    if (abs(denominator) < EPSILON) {
        // line is parallel to plane
        return false;
    }
    f32 t = -dot(plane, line_origin) / denominator;
    *out_signed_dist = t;
    *out_point = line_origin + t * line_direction;
    return true;
}