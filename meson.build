project(
    'bonsai', 'c',
    version : '0.1.0',
    license : 'GPL-3.0',
    meson_version : '>=0.61.0',
    default_options : [
        'c_std=c17',
        'warning_level=2',
        'werror=false',
        'buildtype=debug',
    ]
)

add_project_arguments(
    [
        '-DWLR_USE_UNSTABLE',

        '-Wno-unused-parameter',
		'-Wno-unused-result',
		'-Wno-missing-braces',
		'-Wundef',
		'-Wvla',
    ],
    language : 'c',
)

cc = meson.get_compiler('c')

### Location defaults
prefix = get_option('prefix')
datadir = get_option('datadir')
sysconfdir = get_option('sysconfdir')
add_project_arguments(
    [
        '-DBSI_PREFIX="@0@"'.format(prefix),
        '-DBSI_DATADIR="@0@"'.format(datadir),
        '-DBSI_SYSCONFDIR="@0@"'.format(sysconfdir),
        '-DBSI_USERCONFDIR=".config"',
    ],
    language : 'c',
)

### Dependencies
dep_wlroots = dependency('wlroots', required : true)
dep_libinput = dependency('libinput', required : true)
dep_libevdev = dependency('libevdev', required : false)
dep_libudev = dependency('libudev', required : true)
dep_xkbcommon = dependency('xkbcommon', required : true)
dep_glesv2 = dependency('glesv2', required : true)
dep_drm = dependency('libdrm', required : false)
dep_wayland_server = dependency('wayland-server', required : true)
dep_wayland_client = dependency('wayland-client', required : true)
dep_wayland_cursor = dependency('wayland-cursor', required : true)
dep_wayland_egl = dependency('wayland-egl', required : true)
dep_wayland_protocols = dependency('wayland-protocols', required : true)
dep_pixman = dependency('pixman-1', required : true)
dep_cairo = dependency('cairo', required : true)
dep_math = cc.find_library('m', required : true)

### Optional extern
ext_swaybg = find_program('swaybg', required : false)
if not ext_swaybg.found()
    warning('swaybg is needed for wallpaper support')
endif

ext_swaylock = find_program('swaylock', required : false)
if not ext_swaylock.found()
    warning('swaylock is needed for session locking support')
endif

ext_bemenu = find_program('bemenu', required : false)
if not ext_bemenu.found()
    warning('bemenu is needed for run menu support')
endif

ext_waybar = find_program('waybar', required : false)
if not ext_waybar.found()
    warning('waybar is needed for status bar support')
endif

ext_slurp = find_program('slurp', required : false)
ext_grim = find_program('grim', required : false)
ext_wlclipboard = find_program('wl-copy', required : false)
if not ext_slurp.found() or not ext_grim.found() or not ext_wlclipboard.found()
    warning('slurp, grim & wl_clipboard are needed for screenshot support')
endif

### Include dirs
inc_default = include_directories('include')
inc_protocols = include_directories('include/protocols')

### Subdirs
subdir('bonsai')

### Installation
# Session
install_data(
    'bonsai.desktop',
    install_dir : join_paths(datadir, 'wayland-sessions')
)

# Default configs
install_data(
    'extern/bonsai/config',
    install_dir : join_paths(sysconfdir, 'bonsai')
)
install_data(
    'extern/swaylock/config',
    install_dir : join_paths(sysconfdir, 'swaylock')
)
install_data(
    'extern/waybar/config',
    'extern/waybar/style.css',
    install_dir : join_paths(sysconfdir, 'waybar')
)

# Wallpapers
install_data(
    'assets/Wallpaper-1.jpg',
    'assets/Wallpaper-2.jpg',
    'assets/Wallpaper-3.jpg',
    'assets/Wallpaper-4.jpg',
    'assets/Wallpaper-Default.jpg',
    'assets/LICENSE',
    install_dir : join_paths(datadir, 'backgrounds', 'bonsai')
)

# User configs
if get_option('user-configs')
    run_command(
        'extern/install.sh',
        capture : true,
        check : true,
        env : { 
            'SRCDIR': '@0@'.format(meson.global_source_root()) 
        }
    )
endif
