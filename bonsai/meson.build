bonsai_src = files(
    'main.c',
    'server.c',
    'util.c',

    'config/input.c',
    'config/output.c',
    'config/global.c',

    'desktop/view.c',
    'desktop/cursor.c',
    'desktop/workspace.c',
    'desktop/layer.c',

    'events/evglobal.c',
    'events/evoutput.c',
    'events/evinput.c',
    'events/evview.c',
    'events/evworkspace.c',
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
        '-DWLR_USE_UNSTABLE'
        ]
    )
