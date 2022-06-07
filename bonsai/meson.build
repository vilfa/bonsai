bonsai_src = files(
    'main.c',
    'server.c',
    'util.c',
    'input.c',
    'output.c',

    'input/cursor.c',
    'input/keyboard.c',
    
    'desktop/view.c',
    'desktop/workspace.c',
    'desktop/layers.c',
    'desktop/decoration.c',
    'desktop/idle.c',
    'desktop/lock.c',
)

bonsai_dep = [
    dep_wlroots,
    dep_libinput,
    dep_libudev,
    dep_libevdev,
    dep_xkbcommon,
    dep_glesv2,
    dep_drm,
    dep_wayland_server,
    dep_wayland_client,
    dep_wayland_cursor,
    dep_wayland_egl,
    dep_wayland_protocols,
    dep_pixman,
    dep_cairo,
    dep_math,
]

bonsai_inc = [
    inc_default,
    inc_protocols,
]

executable(
    'bonsai',
    sources : bonsai_src,
    include_directories : bonsai_inc,
    dependencies : bonsai_dep,
    c_args : [
        '-DWLR_USE_UNSTABLE',
        '-O0',
        ]
    )
