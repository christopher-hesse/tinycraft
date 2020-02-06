# tinycraft

An simple 3D reinforcement learning environment inspired by [Minecraft](https://www.youtube.com/watch?v=ToEFJC1xjU8) and [MineRL](http://minerl.io/).

Features:

* voxels
* light
* reasonably fast
* not a crazy amount of code (3.5k lines of C++ excluding data files, of which about half is the environment itself)
* python interface
* headless software/hardware rendering on linux

Missing things:

* dynamic generation of land chunks (they're all generated at the start of the episode)
* physics system that works reasonably well
* any actual gameplay (items, etc)
* binaries, so you'll have to install from source

## Installation

On Ubuntu 18.04:

```sh
gcloud compute instances create instance-1 --machine-type=n1-standard-8 --accelerator=type=nvidia-tesla-v100,count=1 --image=ubuntu-1804-bionic-v20200129a --image-project=ubuntu-os-cloud --boot-disk-size=50GB
gcloud compute scp --recurse tinycraft instance-1:
gcloud compute ssh instance-1
```

```sh
sudo apt-get update
sudo apt-get install --yes --no-install-recommends curl build-essential
# hardware rendering
# https://cloud.google.com/compute/docs/gpus/install-drivers-gpu#ubuntu-driver-steps
curl -O http://developer.download.nvidia.com/compute/cuda/repos/ubuntu1804/x86_64/cuda-repo-ubuntu1804_10.0.130-1_amd64.deb
sudo dpkg -i cuda-repo-ubuntu1804_10.0.130-1_amd64.deb
sudo apt-key adv --fetch-keys http://developer.download.nvidia.com/compute/cuda/repos/ubuntu1804/x86_64/7fa2af80.pub
sudo apt-get update
sudo apt-get install --yes cuda
# software rendering
sudo apt-get install --yes --no-install-recommends xorg-dev libgl1-mesa-dev xvfb libosmesa6-dev
# build
curl -O https://repo.continuum.io/miniconda/Miniconda3-latest-Linux-x86_64.sh
sh Miniconda3-latest-Linux-x86_64.sh -b
~/miniconda3/bin/conda init
exec -l $SHELL
conda env update --name tinycraft --file tinycraft/env.yaml
conda activate tinycraft
pip install -e tinycraft
```

## Interactive

Use the environment from the perspective of an agent:

```sh
python -m tinycraft.scripts.interactive
```

## Benchmark

```sh
python -m tinycraft.scripts.benchmark --rendering-mode headless-hardware
python -m tinycraft.scripts.benchmark --rendering-mode headless-software
```

### Results

`us_per_step` refers to the microseconds required for all environments in the VecEnv to step

#### `cube` simple test environment

rendering_mode | game | num_envs | num_buffered_steps | num_action_repeats | us_per_step
--- | --- | --- | --- | --- | ---
headless-hardware | cube | 1 | 1 | 1 | 70
headless-hardware | cube | 1 | 1 | 4 | 70
headless-hardware | cube | 1 | 2 | 1 | 45
headless-hardware | cube | 1 | 2 | 4 | 44
headless-hardware | cube | 8 | 1 | 1 | 1142
headless-hardware | cube | 8 | 1 | 4 | 1147
headless-hardware | cube | 8 | 2 | 1 | 716
headless-hardware | cube | 8 | 2 | 4 | 699
headless-software | cube | 1 | 1 | 1 | 231
headless-software | cube | 1 | 1 | 4 | 233
headless-software | cube | 1 | 2 | 1 | 233
headless-software | cube | 1 | 2 | 4 | 235
headless-software | cube | 8 | 1 | 1 | 1531
headless-software | cube | 8 | 1 | 4 | 1502
headless-software | cube | 8 | 2 | 1 | 1516
headless-software | cube | 8 | 2 | 4 | 1498

#### `world` voxel environment

rendering_mode | game | num_envs | num_buffered_steps | num_action_repeats | us_per_step
--- | --- | --- | --- | --- | ---
headless-hardware | world | 1 | 1 | 1 | 113
headless-hardware | world | 1 | 1 | 4 | 152
headless-hardware | world | 1 | 2 | 1 | 76
headless-hardware | world | 1 | 2 | 4 | 108
headless-hardware | world | 8 | 1 | 1 | 1627
headless-hardware | world | 8 | 1 | 4 | 1705
headless-hardware | world | 8 | 2 | 1 | 1045
headless-hardware | world | 8 | 2 | 4 | 1060
headless-software | world | 1 | 1 | 1 | 10716
headless-software | world | 1 | 1 | 4 | 9493
headless-software | world | 1 | 2 | 1 | 7692
headless-software | world | 1 | 2 | 4 | 9962
headless-software | world | 8 | 1 | 1 | 22684
headless-software | world | 8 | 1 | 4 | 22087
headless-software | world | 8 | 2 | 1 | 23138
headless-software | world | 8 | 2 | 4 | 21618

### Conclusions

The single env software rendering is suspiciously slow, there may be a bug somewhere.

An 8-vCPU 1 V100 GCE instance costs $1.93 per hour.  High-CPU instance CPU cores cost about $0.025 per vCPU per hour, so for the same price we could get ~80 vCPUs.  For the GPU to be cost-effective, the hardware rendering mode must be 10x faster than the 8-vCPU software rendering mode.

From the data above it looks like the GPU is faster, but for 8 envs, it's not a huge difference.

With pre-emptible instances, the GPU machine costs $0.80 per hour, while the vCPU cores are $0.0075 per vCPU per hour, for an equivalent machine with ~110 vCPUs.  For pre-emptible instances, the hardware rendering mode would need to be 14x faster than the software one.

Keep in mind that while this environment is simple, it's not extensively tuned for performance and hard to draw conclusions about any other 3D environments.
