bonsai_src = files(
    'main.c',
    'server.c',
    'util.c',

    'config/input.c',
    'config/output.c',
    'config/signal.c',

    'scene/view.c',
    'scene/cursor.c',
    'scene/workspace.c',

    'events/bsi-listeners.c',
    'events/bsi-output.c',
    'events/bsi-input.c',
    'events/bsi-view.c',
    'events/bsi-workspace.c',
)

bonsai_dep = [
    dep_wlroots,
    dep_libinput,
    dep_libdev,
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
