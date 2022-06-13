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
    '-DWLR_USE_UNSTABLE',
    '-Wno-unused-parameter',
    '-Wno-unused-result',
    '-Wno-missing-braces',
    '-Wundef',
    '-Wvla',
    language : 'c',
)

cc = meson.get_compiler('c')

### Defaults
prefix = get_option('prefix')
datadir = get_option('datadir')
sysconfdir = get_option('sysconfdir')
add_project_arguments(
    '-DBSI_PREFIX="@0@"'.format(prefix),
    '-DBSI_DATADIR="@0@"'.format(datadir),
    '-DBSI_SYSCONFDIR="@0@"'.format(sysconfdir),
    '-DBSI_USERCONFDIR=".config"',
    language : 'c',
)

if get_option('bsi_software_cursor')
    add_project_arguments('-DBSI_SOFTWARE_CURSOR', language : 'c')
endif

if get_option('bsi_verbose')
    add_project_arguments('-DBSI_DEBUG', language : 'c')
endif

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
dep_xdpw = dependency('xdg-desktop-portal', required : true)
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

ext_brghtctl = find_program('brightnessctl', required : false)
if not ext_brghtctl.found()
    warning('brightnessctl is needed for backlight control')
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
config = configuration_data()
config.set('default_output', 'WL-1')
config.set('default_mode', '1280x720')
config.set('default_refresh', '60')
config.set('default_pointer', 'wayland-pointer-seat0')
config.set('default_accel_speed', '0.0')
config.set('default_accel_profile', 'flat')
config.set('default_scroll_natural', 'yes')
config.set('default_tap', 'yes')
config.set('default_keyboard', 'wayland-keyboard-seat0')
config.set('default_keyboard_layout', 'us')
config.set('default_keyboard_layout_toggle', 'grp:win_space_toggle')
config.set('default_keyboard_model', 'pc105')
config.set('default_repeat_rate', '20')
config.set('default_repeat_delay', '600')
config.set('default_workspace', '3')
config.set('default_wallpaper', 
    join_paths(prefix, datadir, 'backgrounds', 'bonsai', 'Wallpaper-Default.jpg'))

configure_file(
    configuration : config,
    input : 'extern/bonsai/config.in',
    output : '@BASENAME@',
    install_dir : join_paths(sysconfdir, 'bonsai'),
)

# Wallpapers
install_data(
    'assets/Wallpaper-1.jpg',
    'assets/Wallpaper-2.jpg',
    'assets/Wallpaper-3.jpg',
    'assets/Wallpaper-4.jpg',
    'assets/Wallpaper-Default.jpg',
    'assets/LICENSE',
    install_dir : join_paths(datadir, 'backgrounds', 'bonsai'),
)

# Fonts
install_data(
    'extern/fonts/Nerd-Font-Symbols-Complete.ttf',
    'extern/fonts/LICENSE',
    install_dir : join_paths(datadir, 'fonts'),
)

# User configs
if get_option('bsi_user_configs')
    run_command(
        'extern/install.sh',
        capture : true,
        check : true,
        env : { 
            'SRCDIR': '@0@'.format(meson.global_source_root()) 
        }
    )
endif
