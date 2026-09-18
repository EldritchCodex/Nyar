# Nyar examples

This directory contains executable examples for developing Nyar's Vulkan integration with Nodens.

## `nyar-test-app`

`nyar-test-app` is currently the Khronos Vulkan **Hello Triangle** tutorial adapted to run through Nodens. It is not yet the Nyar renderer. It provides a known-good baseline that will be transformed incrementally into the renderer architecture exposed by [`src/nyar.cppm`](../src/nyar.cppm).

The example uses:

- Nodens for the application and layer lifecycle.
- GLFW for windowing, events, and Vulkan surface creation.
- Vulkan-Hpp RAII for Vulkan resource management.
- Slang for shader compilation to SPIR-V.

## Source layout

- `NyarTestApp.cpp` creates the Nodens application, installs `NyarLayer`, and starts the main loop.
- `NyarLayer.cppm` declares `NyarLayer` and contains tutorial Vulkan setup, swapchain management, pipeline creation, command recording, synchronization, and cleanup.
- `NyarLayer.cpp` implements layer lifecycle and per-frame callbacks.
- `../src/nyar.cppm` is the future public renderer-module boundary. It currently contains only the module declaration.

## Build and run

From the repository root:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --target nyar-test-app
./build/examples/nyar-test-app/nyar-test-app
```

Run from the repository root because the example loads `shaders/slang.spv` through a relative path.

Required tooling includes CMake 3.30+, a C++23 compiler with module support, GLFW, Vulkan, `slangc`, and a working Nodens fetch. Debug builds enable `VK_LAYER_KHRONOS_validation`.

## Runtime flow

`NyarLayer::OnAttach()` initializes, in order:

1. GLFW window.
2. Vulkan instance and validation messenger.
3. Window surface.
4. Suitable physical and logical device.
5. Swapchain and image views.
6. Shader modules and graphics pipeline.
7. Command buffers and synchronization objects.

`OnUpdate()` polls GLFW and renders one frame. `drawFrame()` acquires a swapchain image, records dynamic-rendering commands, submits work, presents the image, and handles resize or out-of-date swapchains. `OnDetach()` waits for the device and releases GLFW and swapchain resources.

The physical-device selection requires Vulkan 1.3+, graphics and presentation support, `VK_KHR_swapchain`, shader draw parameters, dynamic rendering, synchronization2, and extended dynamic state.

## Development scope

The example currently draws one procedural triangle. It has no renderer API, vertex buffers, descriptors, depth attachment, scene data, or configurable window settings. GLFW remains managed directly because Nodens runs in headless mode for this integration test.

Nodens still needs refactoring to support Vulkan-backed windows through its own windowing layer. Track this work in [Nodens issue #30](https://github.com/EldritchCodex/Nodens/issues/30).

Future changes should move tutorial responsibilities behind the renderer architecture in `src/nyar.cppm` while keeping `nyar-test-app` as a practical Vulkan validation program.
