# Galaxian Match

![presentation.webp](bin/Data/2D/presentation.webp)

A simple space-themed match-3 game and my first published game. 



## Build Prerequisites

- Ensure **CMake** is installed.
- Install the dependencies for **Urho3D**. You can refer to the official documentation here: [Urho3D Build Instructions](https://u3d.io/docs/_building.html).



## Build Instructions

### Use CMake to generate the build files.

    cmake -S . -B build
    cd build
    make -j $(nproc)

### By default, the following options are used:

- Build Type: Release
- Installation Prefix: `exe/`



## Installation

Once the build is complete, you can install the project using the appropriate installation command.

    make install



## To Do

Complete the implementation of networked gameplay using WebSocket.



## Third-Party Dependencies

This project relies on the following third-party libraries:

- Urho3D (includes SDL, Box2D, ETCPACK, FreeType, GLEW, LZ4, LibCpuId, Mustache, PugiXml, SQLite, STB, StanHull, WebP, nanodbc, rapidjson)
- libdatachannel



## Compatible Platforms

**Tested:** Windows, Linux, Android, Raspberry Pi **Not Tested:** macOS, iOS, Web



## License

This project is licensed under the MIT License. For more details, see the LICENSE file.
