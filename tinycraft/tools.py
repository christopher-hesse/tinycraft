import os
import subprocess as sp
import platform
import tempfile
import contextlib
import threading
from typing import NamedTuple, List

import filelock

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
BUILD_ROOT = os.environ.get(
    "TINYCRAFT_BUILD_ROOT", os.path.join(SCRIPT_DIR, "..", "build", platform.system())
)

global_build_lock = threading.Lock()
global_builds = set()


class BuildOptions(NamedTuple):
    target: str
    debug: bool = False
    rendering_mode: str = "normal"


@contextlib.contextmanager
def chdir(newdir: str):
    curdir = os.getcwd()
    try:
        os.chdir(newdir)
        yield
    finally:
        os.chdir(curdir)


def run(cmd: List[str]) -> sp.CompletedProcess:
    return sp.run(cmd, stdout=sp.PIPE, stderr=sp.STDOUT, encoding="utf8")


def check(proc: sp.CompletedProcess) -> None:
    if proc.returncode != 0:
        print(f"RUN FAILED {proc.args}:\n{proc.stdout}")
        raise Exception("failed to build from source")


def get_build_dir(options: BuildOptions) -> str:
    return os.path.join(BUILD_ROOT, "-".join(str(opt) for opt in options))


def get_output_dir(options: BuildOptions) -> str:
    build_dir = get_build_dir(options)
    if platform.system() == "Windows":
        if options.debug:
            build_type = "Debug"
        else:
            build_type = "RelWithDebInfo"
        # MSVC puts the output in a subfolder for some reason
        return os.path.join(build_dir, build_type)
    else:
        return build_dir


def build(options: BuildOptions) -> str:
    build_dir = get_build_dir(options)
    os.makedirs(build_dir, exist_ok=True)
    with chdir(build_dir), global_build_lock:
        if options not in global_builds:
            with filelock.FileLock(".build-lock"):
                generator = "Unix Makefiles"
                if platform.system() == "Windows":
                    generator = "Visual Studio 15 2017 Win64"

                if options.debug:
                    build_type = "Debug"
                else:
                    build_type = "RelWithDebInfo"

                if not os.path.exists("CMakeCache.txt"):
                    cmd = ["cmake", SCRIPT_DIR, "-G", generator]
                    if platform.system() != "Windows":
                        cmd.append(f"-DCMAKE_BUILD_TYPE={build_type}")
                    if platform.system() == "Linux":
                        if options.rendering_mode == "normal":
                            pass
                        elif options.rendering_mode == "headless-software":
                            cmd.append("-DLINUX_HEADLESS_SOFTWARE_RENDERING=ON")
                        elif options.rendering_mode == "headless-hardware":
                            cmd.append("-DLINUX_HEADLESS_HARDWARE_RENDERING=ON")
                        else:
                            raise Exception("invalid rendering mode")
                    else:
                        assert (
                            options.rendering_mode == "normal"
                        ), "alternative rendering modes only supported on linux"
                    check(run(cmd))

                # visual studio projects have to have the debug/release thing specified at build time rather than configure time
                # specify it both places just in case
                check(
                    run(
                        [
                            "cmake",
                            "--build",
                            ".",
                            "--config",
                            build_type,
                            "--target",
                            options.target,
                        ]
                    )
                )
            global_builds.add(options)

    return get_output_dir(options)


def config_env():
    os.environ["RESOURCES_DIR"] = os.path.join(SCRIPT_DIR, "resources")
    os.environ["OUTPUT_DIR"] = tempfile.mkdtemp()
