mir_add_wrapped_executable(mir_demo_client_wayland wayland_client.c)
target_link_libraries     (mir_demo_client_wayland PkgConfig::WAYLAND_CLIENT)


mir_add_wrapped_executable(mir_demo_client_wayland_egl_spinner spinner.cpp)
target_link_libraries     (mir_demo_client_wayland_egl_spinner miral-spinner)
