"""
Build and run the human interface executable with the requested environment
"""

import argparse
import os
import platform
import sys
import subprocess as sp

from ..tools import build, config_env, BuildOptions


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--release", action="store_true")
    args = parser.parse_args()

    name = "human"
    debug = not args.release
    options = BuildOptions(target=name, debug=debug)
    try:
        output_dir = build(options)
    except Exception:
        print("failed to build")
        sys.exit(1)
    config_env()
    suffix = ""
    if platform.system() == "Windows":
        suffix = ".exe"
    sp.run(os.path.join(output_dir, f"{name}{suffix}"))


if __name__ == "__main__":
    main()
