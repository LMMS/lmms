libWinTF8
=========

LibWinTF8 is the library handling things related to UTF-8 and Unicode when you want to port your program to Windows.

## Compiling

You need a C++ 11 compliant compiler (GCC / Clang) or MSVC (at least version 12, that is Visual Studio 2013) and CMake to build this library.

On POSIX compatible systems with GCC toolchain, run `./configure && make` to build it.

On Windows with MSVC toolchain, generate a Visual Studio solution from `CMakeLists.txt` and build it.

Considering the C++ ABI compatibility issue, it is recommended to use the same compiler to build libWinTF8 as well as other parts of your software.

## Usage

Upon successful installation, you will be able to access all the functionality with C or C++ by including `libwintf8.h`

Every C++ function is under the `WTF8` namespace.

Every C function is prefixed with `WTF8_`.

## Demo

The following is a simple `cat` program, supporting UTF-8 filename and Unicode output.

```c++
#include <iostream>
#include <vector>
#include <libwintf8.h>

/* Set the Windows console font to Lucida Console, which is the only console font supporting Unicode characters. */
static WTF8::SetConsoleFont set_console_font;

int main() {
    /* WTF8::u8string inherits std::string. In addition to useful conversion methods, introducing a new type makes sure that different encodings do not mix up. */
    std::vector<WTF8::u8string> argv = WTF8::getargv();
    if(argv.size() < 2) {
        WTF8::cerr << "Usage: " << argv[0] << " filename" << std::endl;
        return 1;
    }

    /* WTF8::ifstream / ofstream / fstream accept WTF8::u8string as file name. */
    WTF8::u8string filename = argv[1];
    WTF8::ifstream fileobj(filename);
    if(!fileobj.is_open()) {
        WTF8::cerr << "Error: unable to open file '" << filename << "'" << std::endl;
        return 2;
    }

    /* WTF8::cin / cout / cerr / clog use UTF-8 text. */
    char c;
    while(fileobj.get(c))
        WTF8::cout << c;
    return 0;
}
```

## Documentation

This project is in early development and the documentation is not finished yet. If you enjoy libWinTF8, consider helping me to document it.

Alternatively you can browse and read the source code to understand how it works.

## License

This software is licensed under BSD license. Refer to the [COPYING](COPYING) file for more information.
