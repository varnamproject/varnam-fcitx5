cmake_minimum_required(VERSION 3.25)

project(fcitx5-varnam VERSION 0.0.2)

find_package(ECM REQUIRED 1.0.0 REQUIRED)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

include(FeatureSummary)
include(GNUInstallDirs)
include(ECMUninstallTarget)

find_package(Fcitx5Core 5.0.2 REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(varnam "govarnam" IMPORTED_TARGET REQUIRED)

include("${FCITX_INSTALL_CMAKECONFIG_DIR}/Fcitx5Utils/Fcitx5CompilerSettings.cmake")
add_definitions(-DFCITX_GETTEXT_DOMAIN=\"fcitx5-varnam\" -D_GNU_SOURCE)
fcitx5_add_i18n_definition()

option(VARNAM_DEBUG "Enable debug logs" OFF)

add_subdirectory(src)
add_subdirectory(icons)

install(FILES "com.varnamproject.Fcitx5.Addon.varnamfcitx.metainfo.xml.in" 
  RENAME com.varnamproject.Fcitx5.Addon.varnamfcitx.metainfo.xml DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)