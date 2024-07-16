project('fcitx5-varnam', 'cpp', version: '0.0.1', default_options: ['cpp_std=c++17'])

# imports
fs = import('fs')

# dependencies
fcitx5_core_dep = dependency('Fcitx5Core')
fcitx5_module_dep = dependency('Fcitx5Module', components: ['Punctuation'])
varnam_dep = dependency('govarnam')

if get_option('varnam_debug')
  add_global_arguments('-DDEBUG_MODE', language: 'cpp')
endif

config_data = configuration_data()
if (get_option('version') != '')
  config_data.set('version', get_option('version'))
elif (run_command('git', 'describe', check: false).stdout().strip() == '')
  config_data.set('version', meson.project_version())
else
  git_tag = run_command('git', 'describe', '--always', '--dirty', check: false).stdout().strip()
  config_data.set('version', git_tag.strip('v'))
endif

# source
subdir('src')
# icons
subdir('icons')

# install AppStream Metadata file
install_data(
  'com.varnamproject.Fcitx5.Addon.varnamfcitx.metainfo.xml.in',
  rename: 'com.varnamproject.Fcitx5.Addon.varnamfcitx.metainfo.xml',
  install_dir:  get_option('datadir') + '/metainfo',
)