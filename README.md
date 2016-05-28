# WiiWhiteboard
Windows multi-screen-capable Wiimote whiteboard.

This program reads data from Wiimotes connected to the computer via bluetooth, and simulates mouse-clicks based on the IR dots they see.

Original idea and proof-of-concept program by Johnny Chung Lee, http://johnnylee.net/projects/wii/ (but his program won't work on 64-bit Windows and won't work with multiple screens / multiple Wiimotes).

# Quick guide
After you run the program, it queries the system for any connected Wiimotes, and connects to all of them. Then a calibration dialog is shown on the first monitor, you can either calibrate any Wiimote for that screen, or use the "Skip this screen" button if you don't want any Wiimote to work on that screen; this moves the dialog to the next display. When you've calibrated all screens, pressing the "Start" button will hide the program and the Wiimotes will start working as a mouse on the calibrated screens.

The calibration dialog has a "Show raw data" button, which opens another dialog in which you can see the coords of the points seen by each of the Wiimotes. You can use this dialog to position your Wiimotes for the best results - so that they cover the entire screen, but are as close as possible to it.

# Compiling
This program has been tested with MS Visual Studio 2013 Community Edition, there are no special SDKs needed, other than the default Windows SDK which comes with the Visual Studio.
Other compilers may be able to compile the program, but are currently untested.

There are no plans to make this program multi-platform.
