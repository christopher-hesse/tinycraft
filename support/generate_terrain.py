import os
import imageio
import numpy as np

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
INPUT_DIR = os.path.join(
    SCRIPT_DIR, "..", "third-party", "kenney-voxel-pack", "PNG", "Tiles"
)
TEXTURE_SIZE = 128

TEXTURES = {
    0: "grass_top",
    1: "stone",
    2: "dirt",
    3: "dirt_grass",
    20: "trunk_side",
    21: "trunk_top",
    53: "leaves",
}


def set_texture(combined_img, texture_id, tex_img):
    texture_row = texture_id // 16
    texture_col = texture_id % 16
    combined_img[
        texture_row * TEXTURE_SIZE : (texture_row + 1) * TEXTURE_SIZE,
        texture_col * TEXTURE_SIZE : (texture_col + 1) * TEXTURE_SIZE,
        :,
    ] = tex_img


def main():
    images = {}
    for filename in os.listdir(INPUT_DIR):
        name, _ = os.path.splitext(filename)
        images[name] = imageio.imread(os.path.join(INPUT_DIR, filename))

    out_img = np.zeros((TEXTURE_SIZE * 16, TEXTURE_SIZE * 16, 4), dtype=np.uint8)
    for texture_id in range(256):
        set_texture(out_img, texture_id, images["snow"])

    for texture_id, name in TEXTURES.items():
        print(texture_id, name)
        set_texture(out_img, texture_id, images[name])

    imageio.imsave("terrain.png", out_img)


if __name__ == "__main__":
    main()
