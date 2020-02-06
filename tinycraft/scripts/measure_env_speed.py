import argparse
import numpy as np
import time
from ..env import name_to_env_class


NUM_STEPS_PER_PRINT = 1000


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--game", default="world")
    parser.add_argument("--num-envs", default=1, type=int)
    parser.add_argument("--num-buffered-steps", default=1, type=int)
    parser.add_argument("--num-action-repeats", default=1, type=int)
    parser.add_argument("--skip-build", action="store_true")
    parser.add_argument("--rendering-mode", default="normal")
    args = parser.parse_args()

    env = name_to_env_class[args.game](
        num_envs=args.num_envs,
        num_buffered_steps=args.num_buffered_steps,
        num_action_repeats=args.num_action_repeats,
        build=not args.skip_build,
        rendering_mode=args.rendering_mode,
    )
    actions = np.array(
        [np.zeros_like(env.action_space.sample()) for i in range(env.num_envs)],
        dtype=np.uint8,
    )

    env.reset()
    start = time.time()
    step_count = 0

    while True:
        env.step(actions)
        step_count += 1
        if step_count % NUM_STEPS_PER_PRINT == 0:
            print(
                f"step_count {step_count} us_per_step {int((time.time() - start) * int(1e6) / NUM_STEPS_PER_PRINT)}"
            )
            start = time.time()


if __name__ == "__main__":
    main()
