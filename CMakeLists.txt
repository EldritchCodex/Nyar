cmake_minimum_required(VERSION 3.30)

# Required by CMake's experimental support for importing the standard library.
set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD
    "f35a9ac6-8463-4d38-8eec-5d6008153e7d"
)

project(
    Nyar
    VERSION 0.0.0
    LANGUAGES CXX
)


# ==============================================================================
# General build configuration
# ==============================================================================

# Configuration names used by multi-configuration generators
# (e.g. Ninja Multi-Config)
set(
    CMAKE_CONFIGURATION_TYPES
    "Debug;Release"
    CACHE STRING
    "Available build configurations"
    FORCE
)

# Generate compile_commands.json for clangd.
set(
    CMAKE_EXPORT_COMPILE_COMMANDS
    ON
    CACHE BOOL
    "Export compiler commands"
    FORCE
)


# ==============================================================================
# C++ language configuration
# ==============================================================================

# Enable CMake's support for importing the C++ standard library and C++ modules.
set(CMAKE_CXX_COMPILER_IMPORT_STD ON)
set(CMAKE_CXX_MODULE_STD ON)

# Require the selected C++ standard and disable compiler-specific extensions.
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Use C++23 for the project by default.
set(CMAKE_CXX_STANDARD 23)

# This setting documents that the project intends to use the Vulkan C++ module.
# The Vulkan module target is configured further below.
set(ENABLE_CPP20_MODULE ON)


# ==============================================================================
# Compiler-specific options
# ==============================================================================

# Clang currently reports warnings for some experimental module features used
# by the Vulkan C++ module. These warnings are disabled for now.
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(
        -Wno-reserved-module-identifier
        -Wno-include-angled-in-module-purview
    )
endif()


# ==============================================================================
# External tools and dependencies
# ==============================================================================

# Locate the Slang compiler.
find_program(
    SLANGC_EXECUTABLE
    NAMES slangc
    REQUIRED
)

# Locate libraries.
find_package(glfw3 REQUIRED)
find_package(Vulkan REQUIRED)


# ==============================================================================
# Shader compilation helper
# ==============================================================================

# Creates a build target that compiles Slang source files into a SPIR-V file.
#
# Example:
#
#     add_slang_shader_target(
#         CompileShaders
#         SOURCES
#             "${CMAKE_CURRENT_SOURCE_DIR}/shaders/shader.slang"
#     )
#
# The generated shader is placed in the build directory rather than the source
# directory so generated files do not modify the source tree.
function(add_slang_shader_target TARGET)
    # Parse the named SOURCES argument passed to this function.
    cmake_parse_arguments(
        SHADER
        ""
        ""
        "SOURCES"
        ${ARGN}
    )

    if (NOT SHADER_SOURCES)
        message(
            FATAL_ERROR
            "add_slang_shader_target(${TARGET}) requires SOURCES"
        )
    endif()

    # Store generated shader files under:
    #
    #     <build-directory>/shaders/<target-name>/
    #
    set(SHADER_OUTPUT_DIR
        "${CMAKE_CURRENT_BINARY_DIR}/shaders/${TARGET}"
    )

    set(SHADER_OUTPUT
        "${SHADER_OUTPUT_DIR}/slang.spv"
    )

    # Describe how the SPIR-V file is generated.
    #
    # CMake reruns this command whenever one of the source files listed in
    # DEPENDS is newer than the generated output file.
    add_custom_command(
        OUTPUT "${SHADER_OUTPUT}"

        # Create the output directory before invoking slangc.
        COMMAND
            "${CMAKE_COMMAND}" -E make_directory
            "${SHADER_OUTPUT_DIR}"

        # Compile both vert and frag entry points into one SPIR-V file.
        COMMAND
            "${SLANGC_EXECUTABLE}"
            ${SHADER_SOURCES}
            -target spirv
            -profile spirv_1_4
            -emit-spirv-directly
            -fvk-use-entrypoint-name
            -entry vertMain
            -entry fragMain
            -o "${SHADER_OUTPUT}"

        # The shader is rebuilt when any listed input source changes.
        DEPENDS
            ${SHADER_SOURCES}

        COMMENT "Compiling Slang shaders"
        VERBATIM
    )

    # Expose the custom command as a named build target.
    #
    # This allows the shader compilation to be invoked explicitly with:
    #
    #     cmake --build build --target CompileShaders
    add_custom_target(
        "${TARGET}"
        DEPENDS "${SHADER_OUTPUT}"
    )
endfunction()


# ==============================================================================
# Vulkan C++ module
# ==============================================================================

# Create a target containing the Vulkan C++ module interface.
add_library(VulkanCppModule)

# Give the module target a namespaced alias so consumers can link against it
# using the same style as other Vulkan targets.
add_library(Vulkan::cppm ALIAS VulkanCppModule)

# Configuration required by vulkan.hpp and the Vulkan C++ module.
target_compile_definitions(
    VulkanCppModule
    PUBLIC
        VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
        VULKAN_HPP_NO_STRUCT_CONSTRUCTORS=1
        VULKAN_HPP_CXX_MODULE_EXPERIMENTAL_WARNING=1
)

# The Vulkan module includes headers from the Vulkan SDK.
target_include_directories(
    VulkanCppModule
    PRIVATE
        "${Vulkan_INCLUDE_DIR}"
)

# The module target still links against the regular Vulkan loader library.
target_link_libraries(
    VulkanCppModule
    PUBLIC
        Vulkan::Vulkan
)

# The Vulkan module is compiled as C++20, independently of the project's
# default C++23 setting.
set_target_properties(
    VulkanCppModule
    PROPERTIES
        CXX_STANDARD 20
)

# Register the Vulkan C++ module interface file with CMake.
target_sources(
    VulkanCppModule
    PUBLIC
        FILE_SET cxx_modules
        TYPE CXX_MODULES
        BASE_DIRS
            "${Vulkan_INCLUDE_DIR}"
        FILES
            "${Vulkan_INCLUDE_DIR}/vulkan/vulkan.cppm"
)


# ==============================================================================
# Shader targets
# ==============================================================================

# Compile the Slang shader source into SPIR-V.
add_slang_shader_target(
    CompileShaders
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/shaders/shader.slang"
)


# ==============================================================================
# Application target
# ==============================================================================

add_executable(
    Nyar
    src/main.cpp
)

# Ensure shaders are compiled before the application target is considered
# complete.
add_dependencies(
    Nyar
    CompileShaders
)

# Link the Vulkan C++ module and GLFW to the application.
target_link_libraries(
    Nyar
    PRIVATE
        Vulkan::cppm
        glfw
)
