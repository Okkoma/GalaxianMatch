# Use cases for the new Urho3D/UrhoDiscover CMake modules in an u3d project.

- In addition to the U3D documentation "Using Urho3D library" https://u3d.io/docs/_using_library.html .
- Use the provided cmake scripts or cmake directly in a terminal https://u3d.io/docs/_building.html#Build_Scripts .

---

## Use Case 1:

### You want to build your project and link against a known u3d library.

This is the classic way. The engine has been compiled once, and you use the u3d libraries in your project.

- **Settings:**
  - `URHO3D_HOME` set (in `CMakeLists.txt` or with the cmake argument `-D..`)
- **Module behavior:**
  - no search, skip fetch
- **Result:**
  - Project configured and ready to build against the prebuilt u3d library.

---

## Use Case 2:

### You want to build your project with your customized u3d source included in your project source.

You can tweak the engine directly in your project: specific u3d compilation options and custom sources (you surely do not include some u3d modules like Docs, Samples or Tools to reduce the build time).

**Note:** don't include u3d sources in app/src/main/cpp if app/CMakeLists.txt uses GLOB RECURSE to find the project sources. This will include u3d sources twice (for the u3d target and for your project target). Instead, put them in app/src/thirdparty/u3d.

- **Settings:**
  - Don't set `URHO3D_HOME` as CMake argument `-D..`.
  - For Android in app:build.gradle.kts, don't set `BUILD_STAGING_DIR` and `JNI_DIR`.
- **Module behavior:**
  - The module searches for u3d sources in the project folder.
  - If there are no u3d sources included, fallback to use case 4.
- **Result:**
  - If the sources are found, the project is configured and ready to build from the u3d sources. u3d is built first, then the project.

---

## Use Case 3:

### You are cloning a project from a Git repository that depends on u3d sources.

This is a convenient way to fetch u3d if you don't want to do it yourself. Clone the project and let it configure itself.

- **Settings:**
  - Don't set `URHO3D_HOME` as CMake argument `-D..`.
  - For Android in app:build.gradle.kts, don't set `BUILD_STAGING_DIR` and `JNI_DIR`.
  - If the project uses a specific git repository for u3d, `GIT_U3D_REPO` should already be set in the `CMakeLists.txt` project file.
  - Otherwise you need to set GIT_U3D_REPO.
- **Module behavior:**
  - The module tries to fetch u3d from GIT_U3D_REPO.
  - If the repository is not available, fallback to use case 4.
- **Result:**
  - If the repository is available, same result as use case 2.

---

## Use Case 4:

### For a new app build, you want to test with a different u3d version from your own u3d base.

This can be useful for testing, switching between versions for a same target system.
This is not designed to switch from one target system (Windows) to another (Web).

- **Settings:**
  - Don't set `URHO3D_HOME` as CMake argument `-D..`.
  - Set `URHO3D_SEARCH_PATH`, the path must contain your(s) u3d distribution(s).
    (you can set it in `CMakeLists.txt`, or with the CMake argument `-D..`)
  - The module also uses the `ENV{URHO3D_HOME}` folder (with the same meaning as `URHO3D_SEARCH_PATH`).
- **Module behavior:**
  - The module searches for u3d distributions in `URHO3D_SEARCH_PATH` and `ENV{URHO3D_HOME}` folders.
  - The search is done only once and the result list is cached.
    - Consequence: If you add or remove an u3d folder on your hard drive, you need to restart with -DFORCEDISCOVER=1.
- **Result:**
  - If a folder is found, it is configured and the project is ready to build.
  - Then in cmake-gui you need to select a folder from the `${YOURAPP}_URHO_SELECT` dropdown list. This selection will become the new `URHO3D_HOME`.
  - After running the configure/generate steps in cmake-gui, your project is configured and ready to build with the selected u3d.
  - You can switch to a different source/build/sdks in the `${YOURAPP}_URHO_SELECT` dropdown list and restart the configure/generate process.

!!! Sorry, not compatible with Android Gradle. Go to use case 1.


