cmake_minimum_required(VERSION 3.30)

# Required by CMake's experimental support for importing the standard library.
set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD
    "f35a9ac6-8463-4d38-8eec-5d6008153e7d"
)

project(Nyar VERSION 0.0.0 LANGUAGES CXX)

# CMake and compiler configuration shared by Nyar and its examples.
set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING
    "Available build configurations" FORCE)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL
    "Export compiler commands" FORCE)
set(CMAKE_CXX_COMPILER_IMPORT_STD ON)
set(CMAKE_CXX_MODULE_STD ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_program(SLANGC_EXECUTABLE NAMES slangc REQUIRED)
find_package(glfw3 REQUIRED)
find_package(Vulkan REQUIRED)

include(FetchContent)
FetchContent_Declare(nodens
    GIT_REPOSITORY https://github.com/EldritchCodex/Nodens.git
    GIT_TAG dev
)
FetchContent_MakeAvailable(nodens)

# Creates a target that compiles Slang source files into one SPIR-V file.
function(add_slang_shader_target TARGET)
  cmake_parse_arguments(SHADER "" "" "SOURCES" ${ARGN})

  if(NOT SHADER_SOURCES)
    message(FATAL_ERROR
            "add_slang_shader_target(${TARGET}) requires SOURCES")
  endif()

  set(output_dir "${CMAKE_CURRENT_BINARY_DIR}/shaders/${TARGET}")
  set(output_file "${output_dir}/slang.spv")

  add_custom_command(
        OUTPUT "${output_file}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${output_dir}"
        COMMAND "${SLANGC_EXECUTABLE}"
            ${SHADER_SOURCES}
            -target spirv
            -profile spirv_1_4
            -emit-spirv-directly
            -fvk-use-entrypoint-name
            -entry vertMain
            -entry fragMain
            -o "${output_file}"
        DEPENDS ${SHADER_SOURCES}
        COMMENT "Compiling Slang shaders"
        VERBATIM
    )

  add_custom_target("${TARGET}" DEPENDS "${output_file}")
endfunction()

# Vulkan-Hpp's C++ module is kept as a separate target so applications can
# import it without depending on Nyar's eventual renderer API.
add_library(VulkanCppModule STATIC)
add_library(Vulkan::cppm ALIAS VulkanCppModule)

target_compile_definitions(VulkanCppModule PUBLIC
    VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
    VULKAN_HPP_NO_STRUCT_CONSTRUCTORS=1
    VULKAN_HPP_CXX_MODULE_EXPERIMENTAL_WARNING=1
)

target_include_directories(VulkanCppModule PRIVATE "${Vulkan_INCLUDE_DIR}")
target_link_libraries(VulkanCppModule PUBLIC Vulkan::Vulkan)
set_target_properties(VulkanCppModule PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

target_sources(VulkanCppModule PUBLIC
    FILE_SET vulkan_modules TYPE CXX_MODULES
    BASE_DIRS "${Vulkan_INCLUDE_DIR}"
    FILES "${Vulkan_INCLUDE_DIR}/vulkan/vulkan.cppm"
)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  target_compile_options(VulkanCppModule PRIVATE
        -Wno-reserved-module-identifier
        -Wno-include-angled-in-module-purview
    )
endif()

add_slang_shader_target(CompileShaders
    SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/shaders/shader.slang"
)

# Nyar starts as an empty library. Its module interface provides a valid,
# dependency-free placeholder for the renderer API that will be added later.
add_library(Nyar STATIC)
add_library(Nyar::Nyar ALIAS Nyar)
target_sources(Nyar PUBLIC
    FILE_SET nyar_modules TYPE CXX_MODULES
    BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/src"
    FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/nyar.cppm"
)
target_compile_features(Nyar PUBLIC cxx_std_23)

add_subdirectory(examples)
