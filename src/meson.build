# Source Files
varnam_fcitx_sources = [
  'varnam_engine.cpp',
  'varnam_state.cpp',
  'varnam_candidate.cpp',
  'varnam_utils.cpp',
]
# Target: Shared Lib
shared_library(
  'varnamfcitx',
  varnam_fcitx_sources,
  dependencies: [fcitx5_core_dep, fcitx5_module_dep, varnam_dep],
  install: true,
  install_dir: get_option('libdir')  + '/fcitx5',
)

# Input Method registration file
install_data(
  'varnamfcitx.conf',
  install_dir: get_option('datadir') +'/fcitx5/inputmethod',
)

# Addon config file
configure_file(
  configuration: config_data,
  input: 'varnamfcitx-addon.conf.in',
  output: 'varnamfcitx.conf',
  install_dir: get_option('datadir') + '/fcitx5/addon',
)