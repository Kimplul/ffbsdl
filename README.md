# ffbsdl
CLI FFB editor written in C with SDL2

# Dependencies

+ SDL2

Installation on Linux is simple, `sudo apt install libsdl2` for Debian-based systems and something very similar for others, please check with your distro.

Installation on Windows is a bit more complicated, but I recommend using [MSYS2](https://www.msys2.org/) and following their installation instructions up to step 7, after which you should run `pacman -S mingw-w64-x84_64-SDL2` before continuing with their instructions.

# Building

`make`

# Running

On Linux:

`./ffbsdl`

On Windows, you will probably want to open up `cmd` or double-click the executable from `explorer.exe`, as there's some weird interaction between MinGW and SDL if you try to run `./ffbsdl`. All input/output seems to be gobbled up for some reason. If you get it to work, please tell me how you did it.

On-screen instructions should hopefully be clear enough to follow without a manual.
