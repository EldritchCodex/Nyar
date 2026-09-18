add_executable(nyar-test-app
  NyarLayer.cpp
  NyarTestApp.cpp
)

target_sources(nyar-test-app
  PRIVATE
    FILE_SET modules TYPE CXX_MODULES
    FILES NyarLayer.cppm
)

target_compile_features(nyar-test-app PUBLIC cxx_std_23)

target_link_libraries(nyar-test-app
    PRIVATE
        Nyar::Nyar
        Nodens::Nodens
        Vulkan::cppm
        glfw
)

add_dependencies(nyar-test-app CompileShaders)
