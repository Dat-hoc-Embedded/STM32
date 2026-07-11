# STM32 CubeMX / CMake Project Setup Guide

This project was initially failing because CMake was still using an old build cache from a previous project folder named `hahahaah`. That caused errors such as:

- `could not load cache`
- `The current CMakeCache.txt directory ... is different than ...`
- tasks using CMake were ignoring the configured targets

This document explains the cause and the exact steps to fix it.

## 1. Root Cause

The project had stale CMake build files generated from an older folder/project name. In particular, the build tree still pointed to a previous project path such as:

- `E:/CODE/STM32_IDE/STM32_MX/hahahaah/...`

while the current workspace is:

- `E:/CODE/STM32_IDE/STM32_MX/Code_MasteringBook_F411`

Because CMake cache and generated files were created for the old path, the new project could not be configured correctly.

## 2. What Was Fixed

The project name was aligned across the main CMake configuration and the STM32CubeMX `.ioc` file.

### Updated values

- In [CMakeLists.txt](CMakeLists.txt), the project name was changed to `Code_MasteringBook_F411`.
- In [hahahaah.ioc](hahahaah.ioc), the project name and file name were changed to `Code_MasteringBook_F411`.

This helps avoid future confusion caused by generated artifacts still using the old name `hahahaah`.

## 3. How to Fix the Error

Run the following commands from the project root:

```bash
rm -rf build/Debug
cmake --preset Debug
cmake --build build/Debug --target clean --config Debug
```

### Why this works

- `rm -rf build/Debug` removes the old, incorrect build directory.
- `cmake --preset Debug` reconfigures the project from the current workspace.
- `cmake --build ... --target clean` verifies that the build tree is now valid.

## 4. If VS Code CMake Tasks Still Fail

If the error persists in VS Code, do the following:

1. Open the Command Palette.
2. Run: `CMake: Select Configure Preset`
3. Choose `Debug`
4. Run: `CMake: clean rebuild`

You may also need to reload the VS Code window.

## 5. Important Note About the Project Name

The old name `hahahaah` appeared in several generated files and folders. That is usually harmless for the code itself, but it can confuse CMake and STM32CubeMX when projects are moved or renamed.

To keep the project clean:

- use a consistent project name everywhere,
- avoid leaving old build directories behind,
- regenerate the build folder after renaming the project.

## 6. Summary

The main fix was to:

- remove the stale build cache,
- reconfigure CMake from the correct project folder,
- and align the project name across CMake and the `.ioc` file.

After this, the build and clean commands should work normally.
