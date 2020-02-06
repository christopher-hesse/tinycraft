import argparse
from ..tools import build, BuildOptions


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--name", required=True)
    args = parser.parse_args()

    try:
        build(BuildOptions(target=args.name, debug=True))
    except Exception:
        print("failed to build")

if __name__ == "__main__":
    main()