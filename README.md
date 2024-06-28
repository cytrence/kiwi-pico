Cytrence Kiwi Demonstration with Raspberry Pi Pico
=========================================

Here at Cytrence, we are revolutionizing how computers and microcontrollers interact with our portable laptops. The Kiwi allows you to connect all sorts of devices, from mini PCs to microcontrollers to your laptop, to act as an external monitor as well as utilize its keyboard and mouse functionality.

To showcase the Kiwi and its uses, we have created a simple snake game to demonstrate its capabilities and showcase the seamless integration of our device with the Raspberry Pi Pico. The game utilizes the TinyUSB library to handle keyboard inputs, allowing the player to control the snake using keyboard controls. The display is managed through a DVI connection, with the snake dynamically rendering graphics in real-time to the connected screen.

Traditional Setup with Raspberry Pi Pico
----------------------------------------
In a typical setup, you would connect a Raspberry Pi Pico with a DVI sock to an external monitor to display its graphics, and you would use an external keyboard to control the game.

![](img/traditionalSetup.jpeg)

While this setup works just fine, it is not portable and can be annoying to set up.

Simplified Setup with the Cytrence Kiwi
------------------------------
The Kiwi simplifies the setup drastically by allowing the user to use their laptop as the external monitor and its built-in keyboard to play the game. This completely gets rid of the need for two large peripherals and streamlines the setup.

![](img/kiwiSetup.jpeg)

With the Kiwi, the DVI sock from the Raspberry Pi Pico connects directly to the Kiwi and connects to the laptop. The snake game can now be played completely within the CytrenceKiwi application window, using your laptop’s keyboard for control.

Debugging Made Easy with UART
-----------------------------
Another powerful feature of the Kiwi is its ability to connect to any compatible device’s UART or serial port. This functionality is particularly useful for the development of microcontrollers. By connecting the UART Tx, Rx, and Gnd from the Raspberry Pi Pico to the Kiwi, you can easily display debug information on your laptop, streamlining the development and debugging processes. With all of the Uart connections set, you can open up the Kiwi's built in terminal and use it as an equivalent to PuTTY on Windows or screen/minicom on Mac OS.

![](img/kiwiSetupDebug.jpeg)

While developing the snake game, the debug terminal was integral to checking initializations and I/O.