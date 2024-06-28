Snake Game for Raspberry Pi Pico
================================

![](../img/snake.jpeg)

Dependencies
------------
To build and run this project, you will need:

- Raspberry Pi Pico SDK
- arm-none-eabi-gcc

Building
--------
Use the VS Code shortcut that is created by the Pico SDK.

The easiest way to build the project is by using the "CMake" extension in VS Code.

Once you have all the packages and extensions installed, you can build everything by either clicking the CMake extension and clicking the build icon or by clicking "Build" on the status bar of VS Code.

Step-by-Step Instructions
-------------------------
1. Install dependencies
- Follow the instructions to install the Pico SDK from the official documentation.
- Install the arm-none-eabi-gcc toolchain from ARM's official site.

2. Clone the repository 
- Make sure to include the "libdvi" submodule to display graphics

3. Set up the Environment
- Ensure that the PICO_SDK_PATH environment variable is set correctly
- export PICO_SDK_PATH=/path/to/pico-sdk

4. Build the project
- If on Windows, use the VS Code shortcut created by the Pico SDK. If on Mac, make sure you have the path of the Pico SDK correctly referenced.  
- Configure the project using CMake: Ctrl + Shift + P -> CMake: Configure.
- Build the project

5. Flash the firmware
- Hold the BOOTSEL button down on the Pico and plug it into a USB port
- Copy the generated UF2 file to the Pico

Running the Game
----------------
After flashing the firmware, the game will start automatically. You can use the arrow keys or WASD to control the snake's movement.

Code Overview
-------------

Here is a brief overview of the main components of the code:

- main.c: Contains the main game logic, including initialization of the framebuffer, drawing functions, snake movement logic, and the main loop
- hid_app.c: Handles the HID (Human Interface Device) functions using the TinyUSB library
- tusb_config.h: Configuration for TinyUSB
- CMakeLists.txt: CMake build configuration file
- pico_sdk_import.cmake: Imports the Pico SDK
- libdvi submodule: Provides the DVI output functionality, including:
  - dvi.h, dvi_serialiser.h, common_dvi_pin_configs.h, tmds_encode.h: Libraries from the PicoDVI library for handling DVI output