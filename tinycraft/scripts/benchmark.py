import argparse
import numpy as np
import time
from ..env import name_to_env_class


NUM_STEPS = 10000


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--rendering-mode", default="normal")
    args = parser.parse_args()

    print(
        "rendering_mode | game | num_envs | num_buffered_steps | num_action_repeats | us_per_step"
    )
    print("--- | --- | --- | --- | --- | ---")

    for game in ["cube", "world"]:
        for num_envs in [1, 8]:
            for num_buffered_steps in [1, 2]:
                for num_action_repeats in [1, 4]:
                    env = name_to_env_class[game](
                        num_envs=num_envs,
                        num_buffered_steps=num_buffered_steps,
                        num_action_repeats=num_action_repeats,
                        rendering_mode=args.rendering_mode,
                    )
                    actions = np.array(
                        [
                            np.zeros_like(env.action_space.sample())
                            for i in range(env.num_envs)
                        ],
                        dtype=np.uint8,
                    )

                    env.reset()
                    start = time.time()
                    for _ in range(NUM_STEPS):
                        env.step(actions)
                    elapsed = time.time() - start
                    print(
                        f"{args.rendering_mode} | {game} | {num_envs} | {num_buffered_steps} | {num_action_repeats} | {int(elapsed * 1e6 / NUM_STEPS)}"
                    )


if __name__ == "__main__":
    main()
