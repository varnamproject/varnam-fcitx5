project('fcitx5-varnam', 'cpp', version: '0.0.1', default_options: ['cpp_std=c++17'])

# imports
fs = import('fs')

# dependencies
fcitx5_core_dep = dependency('Fcitx5Core')
fcitx5_module_dep = dependency('Fcitx5Module', components: ['Punctuation'])
varnam_dep = dependency('govarnam')

# fcitx5 install paths
# ---------------------
# The default fcitx5 addon lib install path depends on CMAKE_INSTALL_FULL_LIBDIR variable.
# Since it is not possible to directly access this value, we have to check the directories and
# set the install path.
if (
  fs.exists('/usr/lib/x86_64-linux-gnu/cmake/Fcitx5Utils/Fcitx5CompilerSettings.cmake')
)
  # Debian based distros
  lib_dir = '/usr/lib/x86_64-linux-gnu'
elif (fs.exists('/usr/lib/cmake/Fcitx5Utils/Fcitx5CompilerSettings.cmake'))
  # Arch
  lib_dir = '/usr/lib'
elif (fs.exists('/usr/lib64/cmake/Fcitx5Utils/Fcitx5CompilerSettings.cmake'))
  # Fedora
  lib_dir = '/usr/lib64'
else
  lib_dir = get_option('libdir')
endif

if (fs.exists('/usr/local/share/fcitx5/inputmethod'))
  data_dir = '/usr/local/share'
elif (fs.exists('/usr/share/fcitx5/inputmethod'))
  data_dir = '/usr/share/'
else
  data_dir = get_option('datadir')
endif

if get_option('varnam_debug')
  add_global_arguments('-DDEBUG_MODE', language: 'cpp')
endif

# source
subdir('src')
# icons
subdir('icons')