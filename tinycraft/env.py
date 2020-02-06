from .libenv import CVecEnv
from . import tools


class BaseEnv(CVecEnv):
    def __init__(
        self,
        num_envs,
        vision_width=64,
        vision_height=64,
        num_buffered_steps=1,
        num_action_repeats=1,
        debug=False,
        reuse_arrays=False,
        rendering_mode="normal",
        build=True,
        options=None,
    ):
        build_options = tools.BuildOptions(
            target="shared_library", debug=debug, rendering_mode=rendering_mode
        )
        if build:
            tools.build(build_options)
        lib_dir = tools.get_output_dir(build_options)
        if options is None:
            options = {}
        merged_options = dict(
            vision_width=vision_width,
            vision_height=vision_height,
            num_buffered_steps=num_buffered_steps,
            num_action_repeats=num_action_repeats,
            **options,
        )
        tools.config_env()
        super().__init__(
            lib_dir=lib_dir,
            lib_name="shared_library",
            num_envs=num_envs,
            options=merged_options,
            reuse_arrays=reuse_arrays,
        )


class WorldEnv(BaseEnv):
    def __init__(self, *, num_envs, world_chunks=8, **kwargs):
        super().__init__(
            num_envs=num_envs,
            options=dict(game="world", world_chunks=world_chunks),
            **kwargs,
        )


class CubeEnv(BaseEnv):
    def __init__(self, *, num_envs, **kwargs):
        super().__init__(num_envs=num_envs, options=dict(game="cube"), **kwargs)


name_to_env_class = {"world": WorldEnv, "cube": CubeEnv}
