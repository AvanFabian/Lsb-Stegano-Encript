## 1. Dependency Management
- All new external dependencies (specifically Dear ImGui and GLFW) **MUST** be integrated via CMake's `FetchContent` module in the main `CMakeLists.txt` file.
- Do not require manual downloads or pre-compiled libraries. The build must be self-contained and reproducible.

## 2. Code Architecture
- The core steganography logic (existing `encode` and `decode` functions) **MUST NOT** be fundamentally altered. The GUI is a front-end caller to this existing logic.
- All new GUI-specific code **MUST** be placed in a separate file (e.g., `gui.cpp`, `ui_main.cpp`).
- The `main()` function in `main.cpp` will be modified to initialize and run the GUI loop, replacing the `argparse` command-line logic.

## 3. Platform and Compatibility
- The solution **MUST** remain cross-platform (Windows, Linux, macOS). The use of GLFW and OpenGL3 is intended to ensure this.
- Do not use any platform-specific APIs (e.g., Win32 API, Cocoa).

## 4. User Interface (UI)
- The UI must be simple, clean, and intuitive.
- It must expose all the core functionalities available in the original command-line version: selecting input image, embed file, output path, and password.
- A status area **MUST** be implemented to provide feedback to the user (e.g., success messages, error messages, progress).

## 5. Error Handling
- All potential errors from the core logic (e.g., file not found, invalid password, file too large) **MUST** be caught and displayed gracefully in the GUI's status area. The application should not crash.