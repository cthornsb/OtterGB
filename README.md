# OtterGB

README for OtterGB
Last updated July 23rd, 2021

## About

OtterGB is a cross-platform Gameboy and Gameboy Color emulator written entirely
in C++. It is completely open source. Feel free to distribute, fork, and modify
the code in any way you want. All I ask that you include a link back here.

Author: [Cory Thornsberry](https://github.com/cthornsb)

Email: <desertedotter@gmail.com>

Repository: <https://github.com/cthornsb/OtterGB>

## Acknowledgements

OtterGB would not be possible without the following resources:

### Graphics and Audio Libraries

[OpenGL](https://www.opengl.org/)

[OpenGL Extension Wrangler (GLEW)](http://glew.sourceforge.net) or on [github](https://github.com/nigels-com/glew)

[Graphics Library Framework (GLFW)](https://www.glfw.org) or on [github](https://github.com/glfw/glfw)

[Simple OpenGL Image Library (SOIL)](https://github.com/paralin/soil)

[PortAudio](http://www.portaudio.com) or on [github](https://github.com/PortAudio/portaudio)

### Documentation

[Pan Docs](https://gbdev.io/pandocs/)

[The GBDev Wiki](https://gbdev.gg8.se/wiki/articles/Main_Page)

[Gameboy CPU (LR35902) instruction set](https://www.pastraiser.com/cpu/gameboy/gameboy_opcodes.html)

[Blargg's test ROMs](https://gbdev.gg8.se/wiki/articles/Test_ROMs)

## Building and Installing

OtterGB requires audio and graphical classes from *[OtterEngine](https://github.com/cthornsb/OtterEngine)*.
Follow the instructions in the OtterEngine README to install the headers and libraries
we will need for OtterGB.

### Windows

Using Visual Studio, clone the *[OtterGB](https://github.com/cthornsb/OtterGB)* repository
and open it. Generate the default CMakeCache and open the CMake settings. In the field
*CMake command arguments:*, input the following:

```bash
    -DOTTER_DIRECTORY="/path/to/OtterEngine"
```

The CMake variable **OTTER_DIRECTORY** allows CMake to load the required
environment scripts OtterGB needs in order to build. And that should be all that is needed.

Save the CMake settings. Once CMake finishes generating, build the project by clicking 
`Build->Build All` followed by `Build->Install OtterGB` 
to install the executable to the install directory (`OtterGB/out/install/` by default).

### Linux (Ubuntu)

Compiling on Linux is relatively easy. Once you have installed OtterEngine, build OtterGB using:

```bash
    git clone https://github.com/cthornsb/OtterGB
    cd OtterGB
    mkdir build
    cd build
    cmake -DOTTER_DIRECTORY="/path/to/OtterEngine" ..
```

The CMake variable **OTTER_DIRECTORY** allows CMake to load the required
environment scripts OtterGB needs in order to build. And that should be all that is needed.
If all goes well and CMake completes successfully, type:

```bash
    make install
```

To build and install OtterGB. The default binary install directory is `OtterGB/install/bin`, 
this is where the *ottergb* executable is located.

### Additional Build Options

| Option           | Default | Description |
|------------------|---------|-------------|
| CMAKE_BUILD_TYPE | Release | CMake build type: None Debug Release RelWithDebInfo MinSizeRel |
| INSTALL_DIR      |         | Install Prefix                                                 |
| BUILD_TOOLS      | OFF     | Build and install emulator tools                               |
| ENABLE_AUDIO     | ON      | Build with support for audio output (Requires PortAudio)       |
| ENABLE_DEBUGGER  | OFF     | Build with support for gui debugger (Requires QT4)             |
| ENABLE_DEBUG_OUTPUT | OFF  | Enable extra debug output                                      |
| INSTALL_DLLS     | ON      | (Windows only) Copy required DLLs when installing executable   |

Emulation of the LR35902 CPU is relatively CPU intensive. Unless you have a beefy
PC, using a build type of *Debug* will probably result in emulation framerates
that are too slow to use. If you require debug symbols, *RelWithDebInfo*
typically works at the same speed as *Release*.

## Getting started

Both the Linux and Windows version of OtterGB support input configuration files.
By default, OtterGB will attempt to load a config file named `default.cfg` which
must be placed in the same directory as the executable. The config file contains
many different emulator options that you can use to tweak performance. 

All possible emulator options are listed in the `default.cfg` file supplied with the
executable. Before running the program for the first time, the variables **ROM_DIRECTORY** and
**ROM_FILENAME** should be set by the user.

**ROM_DIRECTORY** is used to specify the directory where your ROM files are placed. 
It may be an absolute path or a relative path (with respect to the executable). If 
the ROMs are in the same directory as the executable, just leave it as `.` or `./` 
or leave the option unspecified.

**ROM_FILENAME** specifies the file to load on boot, but you can switch it in the emulator 
console (press \` and then type `file <filename>`) or drag and drop a DMG or CGB file 
onto the emulator window to load it. If **ROM_FILENAME** is not specified or if OtterGB
fails to load the file, the emulator will boot into the OtterGB console instead. See the 
[OtterGB Console](#ottergb-console-and-lr35902-interpreter) section below for more information.

High resolution timing calls are platform dependent and can take a significant amount of
time. To alleviate this problem, OtterGB keeps a running average of the timing offset in
order to tweak keep the average framerate near the target of 59.73 fps. It is possible to
modify the target framerate by using the variable **TARGET_FRAMERATE**.

### Windows builds

The Windows version does not support command line options but instead supports system 
paths being dropped on the *.exe* file. If the extension of the dropped file is *.cfg*,
OtterGB will load it as a config file (instead of `default.cfg`), otherwise it will attempt 
to load it as a ROM file.

Using VSync in windowed mode seems to cause inconsistent framerates on Windows, although
this might vary by machine. Using VSync in fullscreen mode seems to work fine.
VSync is automatically enabled when entering fullscreen mode. Press F11 at any time to 
enter fullscreen mode.

### Linux builds

OtterGB may be run on Linux with or without command line arguments. The accepted
command line arguments are shown below:

| Option              | Args           | Description                                                |
|:--------------------|:--------------:|:-----------------------------------------------------------|
| --help (-h)         |                | Print command syntax                                       |
| --config (-c)       | `<filename>`   | Specify an input configuration file                        |
| --input (-i)        | `<filename>`   | Specify an input ROM file                                  |
| --framerate (-F)    | `<fps>`        | Set target framerate (default=59.73)                       |
| --volume (-V)       | `<volume>`     | Set initial output volume (in range 0 to 1)                |
| --verbose (-v)      |                | Toggle verbose output mode                                 |
| --palette (-p)      | `<palette>`    | Set palette number for DMG games (base 16)                 |
| --scale-factor (-S) | `<N>`          | Set the integer size multiplier for screen pixels (default 2) |
| --force-color (-C)  |                | Use CGB mode for original DMG games                        |
| --no-load-sram (-n) |                | Do not load external cartridge RAM (SRAM) at boot          |
| --debug (-d)        |                | Enable debugging support                                   |
| --tile-viewer (-T)  |                | Enable VRAM tile viewer (if debug gui enabled)             |
| --layer-viewer (-L) |                | Enable BG/WIN layer viewer (if debug gui enabled)          |

The `-c` flag allows the use of a config file just as is used for Windows, but many
config file options are duplicated with command line options and the command flags
take priority.

If a config file is not used (with the `-c` flag), the input filename must be
specified, otherwise the program will boot to the console.

### Using bootstrap ROMs

OtterGB supports custom bootstrap ROMs on startup. Bootstraps have a maximum 
length of 16 kB and must contain compiled LR35902 code. Bootstraps must not use
ROM addresses 0x100 to 0x200 (byte numbers 256 to 512) because the cartridge ROM 
header needs to be visible to the emulator. Bootstrap program entry point must be 
byte 0, although you could use a `JP` at byte 0 to jump to the start of the program.

By default, bootstrap programs are expected to exist in a subdirectory named 
`./bootstraps/` which must be placed in the same directory as the executable in
order to function. The expected filenames for the DMG and CGB bootstrap ROMs
are `dmg_boot.bin` and `cgb_boot.bin` respectively. You may also specify bootstrap
filenames by using config file variables **BOOTSTRAP_PATH_DMG** and **BOOTSTRAP_PATH_CGB**
for DMG and CGB games respectively. If OtterGB fails to locate the bootstrap ROMs, it will 
simply print a warning and start the input ROM immediately.

The standard DMG and CGB bootstrap ROMs are not included with OtterGB because
they contain copyrighted material, but you can find them online.

## Running ROMs

While the emulator is running, press ESC to exit and press F1 for default button
mapping. Default game controls are shown below, but they may be changed using the
config file.

### DMG / CGB button inputs:

| Button | Keyboard Key |
|-------:|:-------------|
|  Start | Enter        |
| Select | Tab          |
|      B | j            |
|      A | k            |
|     Up | w (up)       |
|   Down | s (down)     |
|   Left | a (left)     |
|  Right | d (right)    |

### System functions:

| Key | Function |
|----:|:---------|
| F1  | Display help screen |
| F2  | Pause emulation     |
| F3  | Resume emulation    |
| F4  | Reset emulator      |
| F5  | Quicksave state     |
| F6  | Decrease frame-skip (higher CPU usage) |
| F7  | Increase frame-skip (lower CPU usage)  |
| F8  | Save cart SRAM to `sram.dat`        |
| F9  | Quickload state                     |
| F10 | Start/stop midi recording `out.mid` |
| F11 | Enable/disable full screen mode |
| F12 | Take screenshot                 |
| \`  | Open interpreter console        |
| -   | Decrease volume                 |
| +   | Increase volume                 |
| c   | Change currently active gamepad |
| f   | Show/hide FPS counter on screen |
| m   | Mute output audio               |
| n   | Break after the next frame      |
| p   | Switch to the next DMG color palette |

### Controller support

OtterGB supports 360-style and PS-style controllers. Simply plug in a controller
and start using it using the default mapping below (bindings may be changed using
the config file).

| Button | Controller Button |
|-------:|:------------------|
| Start  | Start             |
| Select | Back              |
|      B | A (or X)          |
|      A | B (or O)          |
|     Up | DPad up           |
|   Down | DPad down         |
|   Left | DPad left         |
|  Right | DPad right        |

Press [GUIDE/HOME] to pause emulation (does not work on all platforms because some
bind the guide button to an on-screen menu e.g. Windows).

### Fullscreen and windowed modes

When booting, OtterGB starts in windowed mode. The window may be resized to any size
you wish. Pillarboxing or letterboxing will be added for windows with an aspect ratio
that does not match that of the DMG / CGB (1.111). Pillar[letter]boxing may be 
disabled by setting **UNLOCK_ASPECT_RATIO** to *true*.

OtterGB will automatically pause emulation whenever the window loses focus, and will
resume emulation when it regains focus.

To switch to fullscreen mode, press F11 at any time. VSync is automatically enabled
when in fullscreen mode, regardless of previous settings, but you may disable it from
the [console](#ottergb-console-and-lr35902-interpreter). Pressing F11 again will return 
to windowed mode.

### Save RAM

OtterGB supports saving and loading of cartridge save ram (SRAM) for ROMs which
support this feature. By default, SRAM is loaded automatically when the ROM is
loaded (using the filename of the ROM, if it exists) and it is saved automatically 
when the emulator is closed. If the emulator crashes, SRAM may not save successfully.
To avoid potential loss of save data, SRAM may be saved at any time by pressing F8.

### Savestates

Savestates are supported by OtterGB, but are currently still in development and may
be buggy. They are not guaranteed to work properly and their format is subject to 
change in future updates. Pressing F5 will save the current emulator state and 
pressing F9 will load the saved state. This feature usually works, but sometimes 
the emulator will hang and will need to be reset by pressing F4.

### Screenshots

Screenshots may be saved at any time by pressing F12. Screenshots will be saved
in the same directory as the executable and will have the game title, the date,
and the time in the filename. Currently only BMP output is supported, but this
will be changed to PNG in the future.

### Wav and Midi recording

OtterGB supports recording wav and midi files. Press F10 while the emulator is 
running to start a wav recording. Alternatively, pressing alt^F10 will start a 
midi file recording. Press F10 again to finalize the output wav or midi file.
Currently, all output files will have the same name ('out.wav' or 'out.mid')
so be sure to back up files if you plan to record more than one.

Wav files are recorded with stereo output using the sample rate specified in 
the config file. Recordings have a bit depth of 8 bits per channel.

Midi file recording works fairly well, but may not produce pleasant sounding
midi files if the ROM being recorded makes heavy use of the frequency sweep on
channel 1 or if it changes note frequency very rapidly.

## OtterGB console and LR35902 interpreter

OtterGB contains a built in console and LR35902 interpreter. To enable the console,
press \` at any time while the emulator is running. The console supports many
built-in commands as well as allowing direct read/write access to all CPU registers,
system registers, and all 64 kB of system memory.

All CPU and system registers may be used by name in mathematical expressions. The 8
8-bit LR35902 CPU registers are *a*, *b*, *c*, *d*, *e*, *f*, *h*, and *l*. The 6 16-bit 
registers are *ab*, *cd*, *ef*, *hl*, *sp*, and *pc* (*sp* and *pc* are the stack pointer 
and program counter respectively). System registers are 8-bit values and may be used by 
their standardized names (e.g. *LCDC*, *NR10*, etc; not case sensitive).

There are no sanity checks used in the console. You can read or write any register
or memory location you want. You can also do something stupid like change the program
counter or stack pointer, although these will almost certainly hang the current ROM.

If you crash the ROM while playing around you can reset it by typing *reset* in the
console or by pressing F4 outside the console.

The following built-in commands are available:

| Command       | Args           | Description          |
|:--------------|:--------------:|:---------------------|
| quit          |                | Exit emulator        |
| exit          |                | Exit emulator        |
| close         |                | Close console        |
| help          | [command]      | Print list of commands or syntax for *command* |
| about         |                | Print program information |
| a             | [value]        | Print or set *A* register |
| b             | [value]        | Print or set *B* register |
| c             | [value]        | Print or set *C* register |
| d             | [value]        | Print or set *D* register |
| e             | [value]        | Print or set *E* register |
| f             | [value]        | Print or set *F* register |
| h             | [value]        | Print or set *H* register |
| l             | [value]        | Print or set *L* register |
| d8            | [value]        | Print or set *d8* immediate |
| af            | [value]        | Print or set *AF* register |
| bc            | [value]        | Print or set *BC* register |
| de            | [value]        | Print or set *DE* register |
| hl            | [value]        | Print or set *HL* register |
| pc            | [value]        | Print or set program counter *PC* |
| sp            | [value]        | Print or set stack pointer *SP*   |
| d16           | [value]        | Print or set *d16* immediate      |
| inst          |                | Print most recent cpu instruction |
| read          | `<addr>`         | Read byte at address     |
| write         | `<addr> <value>` | Write byte to address    |
| rreg          | `<name>`         | Read system register     |
| wreg          | `<name> <value>` | Write system register    |
| hex           | `<value>`        | Convert value to hex     |
| bin           | `<value>`        | Convert value to binary  |
| dec           | `<value>`        | Convert value to decimal |
| cls           |                | Clear screen   |
| reset         |                | Reset emulator |
| qsave         | [filename]     | Quicksave      |
| qload         | [filename]     | Quickload      |
| dir           | [path]         | Set ROM directory (or print if no argument) |
| file          | [filename]     | Set ROM filename (or print if no argument)  |
| vsync         |                | Toggle VSync on or off                      |
| record        | [filename]     | Start/Stop recording CPU state to log file  |
