import argparse
import functools
from .. import interactive_base
from ..scalarize import Scalarize
from ..env import name_to_env_class
from ..subproc_vec_env import SubprocVecEnv
from typing import cast, Any


def create_env(game, resolution, num_buffered_steps):
    # subproc expects a gym environment not a vec env
    return Scalarize(
        name_to_env_class[game](
            num_envs=1,
            debug=True,
            vision_height=resolution,
            vision_width=resolution,
            num_buffered_steps=num_buffered_steps,
        )
    )


class Interactive(interactive_base.Interactive):
    def __init__(self, game, resolution, num_buffered_steps, sync):
        self.game = game
        venv = SubprocVecEnv(
            [functools.partial(create_env, game, resolution, num_buffered_steps)]
        )
        venv = cast(Any, venv)  # disable warning about reward_range
        venv.reward_range = None  # for scalarize
        env = Scalarize(venv)
        self.sync = sync
        super().__init__(tps=60, env=env, sync=sync)

    def get_image(self, obs, env):
        return obs["rgb"]

    def keys_to_act(self, keys):
        if self.game == "cube":
            KEY_LEFT = 1 << 0
            KEY_UP = 1 << 1
            KEY_RIGHT = 1 << 2
            KEY_DOWN = 1 << 3
            act = 0
            if "A" in keys:
                act |= KEY_LEFT
            if "W" in keys:
                act |= KEY_UP
            if "D" in keys:
                act |= KEY_RIGHT
            if "S" in keys:
                act |= KEY_DOWN
            if self.sync and act == 0:
                # noop action
                if "SPACE" in keys:
                    return 0
                else:
                    return None
            return act
        elif self.game == "world":
            KEY_MOVE_FORWARD = 1 << 0
            KEY_MOVE_BACK = 1 << 1
            KEY_MOVE_LEFT = 1 << 2
            KEY_MOVE_RIGHT = 1 << 3
            KEY_MOVE_UP = 1 << 4
            KEY_MOVE_DOWN = 1 << 5
            KEY_TURN_LEFT = 1 << 6
            KEY_TURN_RIGHT = 1 << 7
            KEY_TURN_UP = 1 << 8
            KEY_TURN_DOWN = 1 << 9
            KEY_ATTACK = 1 << 10
            KEY_USE = 1 << 11
            KEY_SPRINT = 1 << 12
            act = 0
            if "W" in keys:
                act |= KEY_MOVE_FORWARD
            if "S" in keys:
                act |= KEY_MOVE_BACK
            if "A" in keys:
                act |= KEY_MOVE_LEFT
            if "D" in keys:
                act |= KEY_MOVE_RIGHT
            if "LSHIFT" in keys:
                act |= KEY_MOVE_DOWN
            if "SPACE" in keys:
                act |= KEY_MOVE_UP
            if "LEFT" in keys:
                act |= KEY_TURN_LEFT
            if "RIGHT" in keys:
                act |= KEY_TURN_RIGHT
            if "UP" in keys:
                act |= KEY_TURN_UP
            if "DOWN" in keys:
                act |= KEY_TURN_DOWN
            if "F" in keys:
                act |= KEY_ATTACK
            if "G" in keys:
                act |= KEY_USE
            if "LCTRL" in keys:
                act |= KEY_SPRINT
            if self.sync and act is 0:
                return None
            return act
        else:
            raise Exception(f"invalid name {self.game}")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--game", default="world")
    parser.add_argument("--sync", action="store_true")
    parser.add_argument("--resolution", default=64, type=int)
    parser.add_argument("--num-buffered-steps", default=1, type=int)
    parser.add_argument("--record-dir", type=str)
    args = parser.parse_args()
    ia = Interactive(args.game, args.resolution, args.num_buffered_steps, args.sync)
    ia.run(record_dir=args.record_dir)


if __name__ == "__main__":
    main()
