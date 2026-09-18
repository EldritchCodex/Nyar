[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg?style=flat-square)](https://opensource.org/licenses/MIT) 
![Supported platforms: Linux ](https://img.shields.io/badge/Supported%20platforms-Linux-blue.svg?style=flat-square)

[![GitHub milestone details](https://img.shields.io/github/milestones/progress/eldritchcodex/nyar/1?style=flat-square)](https://github.com/eldritchcodex/nyar/milestone/1)
[![GitHub milestone details](https://img.shields.io/github/milestones/progress/eldritchcodex/nyar/3?style=flat-square)](https://github.com/eldritchcodex/nyar/milestone/3)
[![GitHub milestone details](https://img.shields.io/github/milestones/progress/eldritchcodex/nyar/2?style=flat-square)](https://github.com/eldritchcodex/nyar/milestone/2)

> ⚠️ **Development Status:** Nyar is currently in **active, experimental development** with no stable release. It serves primarily as a learning ground and playground for exploring real-time rendering Vulkan architecture and modern C++20/23 features. APIs are highly subject to change.


## Nyar: A Vulkan real-time renderer

This project is designed to work as a separate module for the [Nodens](https://github.com/EldritchCodex/Nodens) framework. 
Its design and implementation draw from established principles in real-time rendering and modern engine architecture and tools (e.g. Slang shaders, dynamic rendering, etc.)

- **Immediate Goal:** Follow and complete the official [Khronos Vulkan "Drawing a triangle" tutorial](https://docs.vulkan.org/tutorial/latest/03_Drawing_a_triangle/00_Setup/00_Base_code.html) to establish a solid baseline.
- **Future Direction:** Incrementally refactor and morph the working tutorial code into a dedicated renderer API.


## Development Environment

Nyar uses EldritchCodex's shared Linux graphics image through `.devcontainer/devcontainer.json`:

```text
ghcr.io/eldritchcodex/linux-graphics-dev:main
```

Open Nyar in Zed or Visual Studio Code, then reopen it in the Dev Container.
NVIDIA hosts need a working host driver and NVIDIA Container Toolkit. Intel/AMD
hosts use the `/dev/dri` configuration. See the [Nodens toolchain guide](https://github.com/EldritchCodex/Nodens/wiki/Building-and-Toolchain)
for GPU setup details.


## References
- Akenine-Möller, T., Haines, E., Hoffman, N., Pesce, A., Iwanicki, M., & Hillaire, S. (2019). **Real-time rendering** (Fourth edition). CRC Press.
- Castorina, M., & Sassone, G. (2023). **Mastering graphics programming with vulkan: Develop a modern rendering engine from first principles to state-of-the-art techniques**. Packt Publishing.
- Eisemann, E. (Ed.). (2012). **Real-time shadows**. CRC Press.
- Gregory, J. (2019). **Game engine architecture** (Third edition). CRC Press, Taylor & Francis Group.
- Lengyel, E. (2019). **Foundations of game engine development. Volume 2: Rendering**. Terathon Software LLC.
- Marrs, A., Shirley, P., & Wald, I. (Eds.). (2021). **Ray Tracing Gems II: Next Generation Real-Time Rendering with DXR, Vulkan, and OptiX**. Apress. https://doi.org/10.1007/978-1-4842-7185-8
- Pharr, M., Jakob, W., & Humphreys, G. (2017). **Physically based rendering: From theory to implementation** (Third edition). Morgan Kaufmann Publishers/Elsevier. https://www.pbr-book.org/
- **Vulkan Documentation: Vulkan Documentation Project**. (n.d.). Retrieved August 31, 2026, from https://docs.vulkan.org/spec/latest/index.html
- Woo, A. P., Pierre. (2019). **Shadow Algorithms Data Miner**. CRC PRESS.
