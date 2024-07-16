set(varnam_fcitx_sources
  varnam_engine.cpp
  varnam_state.cpp
  varnam_candidate.cpp
  varnam_utils.cpp
)

add_library(varnamfcitx MODULE ${varnam_fcitx_sources})
target_link_libraries(varnamfcitx Fcitx5::Core Fcitx5::Config PkgConfig::varnam)

if(VARNAM_DEBUG)
  target_compile_definitions(varnamfcitx PRIVATE DEBUG_MODE)
endif()

install(TARGETS varnamfcitx DESTINATION "${CMAKE_INSTALL_LIBDIR}/fcitx5")

configure_file(varnamfcitx-addon.conf.in varnamfcitx-addon.conf)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/varnamfcitx-addon.conf" RENAME varnamfcitx.conf DESTINATION "${FCITX_INSTALL_PKGDATADIR}/addon")
install(FILES "varnamfcitx.conf" DESTINATION "${FCITX_INSTALL_PKGDATADIR}/inputmethod")