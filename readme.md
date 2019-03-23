This project is a Linux port of the **anago** 
NES/Famicom flashing and dumping utility 
(from the [Unagi](https://osdn.net/projects/unagi/wiki/FrontPage) project) which is used with the [kazzo](https://osdn.net/projects/unagi/releases/46303) board. This specifically ports version 0.6.0 which was generally more reliable and had a larger script complement than version 0.6.2. That said, I'm sure some investigation could convert missing scripts, and potentially the 0.6.2 version may be more compatible to begin with, but what can you do. I doubt anyone besides myself will use this anyways.

Here is a brief summary of the differences between this codebase and the upstream original:

 1. Per comments in the upstream **porting.txt** document, all Windows references were removed and usually replaced with **unistd.h**. Sleeps were replaced with usleep accordingly.
 2. Static makefile was replaced with a cmake configuration, which simplifies dependency resolution.
 3. Squirrel library upgraded to version 3 per since this is what's already in the Debian repos
 4. Code specific to the unagi GUI removed and code consolidated to a single main folder structure.
 5. Rather than relying on the full kazzo source tree, I simply included the two required H files in this tree. This simplifies the build further.
 6. Script files are not read from the current directory, but rather from **$prefixdir**/share/anago, or from ~/.config/anago. This allows the anago tool to be run from anywhere and thus behaves more like a standard Unix application.

I did start changes by commiting the unmodified upstream code, so you can also browse the change history for more details.

# Build & Install

## Dependencies

This project only requires the following:

 * [Squirrel](http://squirrel-lang.org/)
 * [libusb](https://libusb.info/)
 * [cmake](https://cmake.org/)

And to regenerate the manual page:

 * [txt2man](https://github.com/mvertes/txt2man)

On a Debian/Ubuntu-based distro, these can be installed using the following command:
  
    # apt install libsquirrel-dev libusb-dev cmake

## Build

The build sequence is actually quite simple. For a basic build, start with a terminal in the anago source folder and execute the following:

    $ mkdir build
    $ cd build
    $ cmake ..
    $ make

To regenerate the manual page, if you have **txt2man** installed, use the following:

    $ make manual
    
## Install

After the above is run, simply execute the following, as root, in the build folder. This will install to **/usr/local** by default. Refer to cmake documentation for setting the prefix directory, if alternate destinations are needed.

    # make install
    
# Usage

Refer to include manual page or original readme files for details.
