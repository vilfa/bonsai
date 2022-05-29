project(
    'bonsai', 'c',
    version : '0.1.0',
    license : 'MIT',
    meson_version : '>=0.61.0',
    default_options : [
        'c_std=c17',
        'warning_level=2',
        'werror=false',
        'buildtype=debug',
    ]
)

cc = meson.get_compiler('c')

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
dep_math = cc.find_library('m', required : true)

### Include dirs
inc_default = include_directories('include')
inc_protocols = include_directories('include/protocols')

### Subdirs
subdir('bonsai')
