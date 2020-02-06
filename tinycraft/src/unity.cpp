#include <stdio.h>
#include <inttypes.h>
#include <math.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#ifdef __GNUC__
#include <x86intrin.h>
#endif

#include <string>
#include <vector>
#include <algorithm>
#include <future>
#include <sstream>
#include <unordered_set>
#include <unordered_map>
#include <random>
#include <thread>
#include <initializer_list>
#include <queue>

#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb_image_resize.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#define STB_PERLIN_IMPLEMENTATION
#include <stb_perlin.h>

#include "libenv.h"

using namespace glm;

#include "cpp_util.cpp"
#include "ndarray.cpp"
#include "threearray.cpp"
#include "rand.cpp"
#include "image.cpp"
#include "envoptions.cpp"
#include "gl_util.cpp"
#include "glm_util.cpp"
#include "font.c"
#include "canvas.cpp"
#include "resources.cpp"
#include "game.cpp"
#include "game_cube/game_cube.cpp"
#include "game_world/constants.cpp"
#include "game_world/block_types.c"
#include "game_world/world.cpp"
#include "game_world/game_world.cpp"