# -----------------------------------------------------------------------------
# Example Target
# -----------------------------------------------------------------------------
# Keep tutorial executable separate from evolving renderer module.
add_executable(nyar-test-app
  NyarLayer.cpp
  NyarTestApp.cpp
)

# Register layer module interface; implementation remains in NyarLayer.cpp.
target_sources(nyar-test-app
  PRIVATE
    FILE_SET modules TYPE CXX_MODULES
    FILES NyarLayer.cppm
)

# The example uses C++23 modules and standard-library facilities.
target_compile_features(nyar-test-app PUBLIC cxx_std_23)

# -----------------------------------------------------------------------------
# Dependencies
# -----------------------------------------------------------------------------
# Nyar is a placeholder module. Nodens supplies lifecycle; Vulkan-Hpp and GLFW
# supply the tutorial rendering path.
target_link_libraries(nyar-test-app
    PRIVATE
        Nyar::Nyar
        Nodens::Nodens
        Vulkan::cppm
        glfw
)

# -----------------------------------------------------------------------------
# Generated Assets
# -----------------------------------------------------------------------------
# Ensure shader bytecode exists before building the executable.
add_dependencies(nyar-test-app CompileShaders)
