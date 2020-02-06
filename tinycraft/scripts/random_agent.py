import imageio
import os
import argparse
import numpy as np
from ..env import name_to_env_class


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--game", default="world")
    parser.add_argument("--rendering-mode", default="normal")
    args = parser.parse_args()

    env = name_to_env_class[args.game](
        num_envs=1,
        vision_height=512,
        vision_width=512,
        rendering_mode=args.rendering_mode,
    )

    writer = imageio.get_writer(os.path.join(f"random_agent.mp4"), fps=60, quality=9)

    obs = env.reset()
    writer.append_data(obs["rgb"][0])

    for _ in range(1000):
        actions = np.array(
            [env.action_space.sample() for i in range(env.num_envs)], dtype=np.uint8
        )
        obs, _, _, _ = env.step(actions)
        writer.append_data(obs["rgb"][0])


if __name__ == "__main__":
    main()
