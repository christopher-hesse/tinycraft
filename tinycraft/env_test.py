import numpy as np
import pytest

import subprocess as sp
import os
import platform

from .env import name_to_env_class

from .tools import build, config_env, BuildOptions

# avoid loading two copies of env.dll into the same process
DEBUG = False
RENDERING_MODE = "headless-software" if platform.system() == "Linux" else "normal"


@pytest.mark.parametrize("game", ["cube", "world"])
def test_env_works(game):
    env = name_to_env_class[game](
        num_envs=1,
        vision_width=640,
        vision_height=640,
        debug=DEBUG,
        rendering_mode=RENDERING_MODE,
    )
    obs = env.reset()
    act = np.array(
        [env.action_space.sample() for i in range(env.num_envs)], dtype=np.uint8
    )
    assert not np.array_equal(obs, np.zeros_like(obs))
    env.step(act)
    env.step(act)
    env.close()


@pytest.mark.parametrize("game", ["cube", "world"])
@pytest.mark.parametrize("num_envs", [1, 2])
def test_env_speed(game, num_envs, benchmark):
    env = name_to_env_class[game](
        num_envs=num_envs, debug=DEBUG, reuse_arrays=True, rendering_mode=RENDERING_MODE
    )
    actions = np.array(
        [np.zeros_like(env.action_space.sample()) for i in range(env.num_envs)],
        dtype=np.uint8,
    )

    def rollout(max_steps):
        env.reset()
        step_count = 0
        while step_count < max_steps:
            env.step_async(actions)
            env.step_wait()
            step_count += 1

    benchmark(lambda: rollout(1000))


def test_cpp_tests():
    """
    Run C++ based tests
    """
    build_dir = build(BuildOptions(debug=True, target="tests"))
    config_env()
    sp.run([os.path.join(build_dir, "tests")], check=True)


def test_cpp_benchmarks():
    """
    Run C++ based benchmarks
    """
    build_dir = build(BuildOptions(debug=False, target="benchmarks"))
    config_env()
    sp.run([os.path.join(build_dir, "benchmarks")], check=True)
