# GL TD Template

Welcome !

This is a small template for pratical sessions (TD) of IMAC E3 with openGL.

## Compiling

You need to install [CMake](https://cmake.org/download/).

To use CMake I recommend this VS Code extension : [ms-vscode.cmake-tools](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cmake-tools). You will need to setup the extension with a compiler. Here is [the tutorial](https://code.visualstudio.com/docs/cpp/cmake-linux). It is based on Linux but at the bottom of the page you will find the explanations to adapt it for [Windows](https://code.visualstudio.com/docs/cpp/config-msvc) and [Mac](https://code.visualstudio.com/docs/cpp/config-clang-mac).

For windows you can also use MSYS2 which is available [here](https://www.msys2.org/). This "émulator" allows you to install libraries in the same way than linux, thanks to the command *pacman*. Follow the installation procedure and you will have access to a terminal like in Linux. Then you will be able to install cmake and OpenGL (the emulator is called *Mesa* which does not require a graphic card). You can look at any module/library thanks to the commande `pacman -Ss <library>` (for exameple `pacman -S mesa`). Then you can install it thanks to `pacman -S <nom_du_module>` (`pacman -S mingw64/mingw-w64-x86_64-mesa` for instance). 

Alternatively, on Linux or in MSYS2, you can just create a *build* folder at the root of this project, open a terminal and run `cmake ..` ; chances are it will detect what compiler you have installed and generate the appropriate Makefile / Visual Studio solution / Xcode project.

Executables will be located in the *bin* folder.

## TD Folders

In each TD** folder, each ex****.cpp file will generate an executable, located in the *bin* folder.
In each ex***.cpp, we initialize [*glfw*](https://www.glfw.org/docs/latest/) (the library that handles the window and user inputs), create a window and then start the app.

When compiling, you can ask to compile only one TD thanks to the command `make TDXX` (like `make TD02`). It will only compile and link executables from the corresponding foler.

## User inputs

To learn more about the parameters you receive in the callbacks, read [glfw's documentation](https://www.glfw.org/docs/latest/input_guide.html).

## Assets

Assets (images, 3D models or shaders for example) are supposed to be located in the assets folder.