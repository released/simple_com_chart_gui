# Native MFC Port (Dialog + Resource)

This folder contains the full native MFC port (dialog + menu resources + core logic).

## One-click install (C++ build tools)
```
scripts\setup\setup_vs_cpp.bat
```

## Build

### CMake
```
scripts\build\build_cmake_release.bat
```

### Visual Studio project (sln/vcxproj)
```
scripts\build\build_vs_release.bat
```

### Build all
```
scripts\build\build_all.bat
```

### Clean build outputs
```
scripts\build\clean_build.bat
```

## Notes
- MFC is built via CMake (`CMAKE_MFC_FLAG 1` = static MFC).
- Static CRT (/MT) is enabled for portable exe.
- UI/overlay/status behavior matches the Python tool.
- Build outputs:
  - CMake: `build/cmake/`
  - MSBuild: `build/vs/`
- Logs are written to `app.log` next to the exe.
- USB-UART bridges still require their driver installed.
