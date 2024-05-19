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
  install_dir: lib_dir + '/fcitx5',
  name_prefix: '',
)

# Input Method registration file
install_data(
  'varnamfcitx.conf',
  install_dir: data_dir + '/fcitx5/inputmethod',
)

# Addon config file
install_data(
  'varnamfcitx-addon.conf.in',
  rename: 'varnamfcitx.conf',
  install_dir: data_dir + '/fcitx5/addon',
)