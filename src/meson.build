hyprfire = shared_module('hyprfire',
                     ['hyprfire.cpp', 'utils/crossfade_2.cpp', 'simple_tile_2/tile-plugin.cpp', 'simple_tile_2/tree.cpp', 'simple_tile_2/tree-controller.cpp'],
                     dependencies: [wayfire, wlroots, wfconfig, pixman, json],
                     install: true,
                     install_dir: join_paths(get_option('libdir'), 'wayfire'))