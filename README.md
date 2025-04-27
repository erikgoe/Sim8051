# Sim8051
A simple simulator for the 8051 architecture. Includes a disassembler and a simple assembly editor (+assembler) for a more streamlined workflow.

![Screenshot of the simulator user interface](https://github.com/erikgoe/Sim8051/blob/main/doc/screenshot.png "User interface")

Head to the releases section to download the application or build it using the instructions below. Some simple example programs can be found in the `examples` directory. Please be aware that not every instruction has been extensively tested, so expect bugs.

Feel free to open an issue, if you find a bug or have other suggestions/ideas or questions.

## Why?
I need a simulator for my little compiler project and didn't find a simple one that compiles on linux, so I thought it would make sense to "quickly" write my own.

## Features
* Simulate the microcontroller with its whole ISA.
* Load programs from Intel hex files.
* Timers.
* Interrupts.
* External ram.
* Assembly view with decoded instructions and live-update of register content.
* Ste-by-step execution and breakpoints (address and instruction based).
* Assembly editor with integrated assembler that allows an easier workflow.
* Simple decimal to hexadecimal converter (also vice versa).
* Flexible GUI: dock or hide windows according to your preferences.

## Usage notes
* GUI docking: I recommend to create a proper layout by moving the sub-windows to the window edges.
* The simulator is designed following the documentation in the "sources" section below.
* Vague syntax specification for the integrated assembler can be found in Encoding.hpp. Available mnemonics can be found in keil's documentation (see sources section).
* A few keyboard shortcuts are supported: Space (single step), R (reset MCU), CTRL+Enter while editing (save & compile), P (run/pause), L (reload all files and compile).
* Breakpoints can be set by clicking on the left column in assembly view. You can also change the "break instruction".
* The simulation does not mirror the hardware one-to-one. Some features like interrupts might trigger one cycle too late or ports may behave differently.
* Labels must contain at least one non-hexadecimal character to be usable.
* Labels can be used with any jump instructions and instruction 0x90 (mov dptr, <value/label>)

## Dependencies
Install them with a package manager like "pacman" or follow the instructions on their website.

* SFML: https://github.com/SFML/sfml
* IMGUI: (see building)

## Building
This project uses cmake as build system.

SFML ist the only dependency which must be installed manually, the rest is included in the building instructions.

### Linux
    mkdir deps && cd deps
    git clone https://github.com/ocornut/imgui
    git clone https://github.com/eliasdaler/imgui-sfml
    git -C imgui checkout docking # only necessary until docking branch is merged
    cat imgui-sfml/imconfig-SFML.h >> imgui/imconfig.h
    cd ..
    
    mkdir build && cd build
    cmake ..
    make

### Windows
    mkdir deps && cd deps
    git clone https://github.com/ocornut/imgui
    git clone https://github.com/eliasdaler/imgui-sfml
    git -C imgui checkout docking # only necessary until docking branch is merged

Copy `imgui-sfml/imconfig-SFML.h` to `imgui/imconfig.h`.
The following assumes you have downloaded SFML into `deps/sfml/SFML-3.0.1`. If not, update the paths accordingly.

    mkdir build && cd build
    cmake .. -DSFML_DIR="deps/sfml/SFML-3.0.1/lib/cmake/SFML"
    cmake --build . -j 8

If the build fails with missing include paths or .lib files, try to add `-DSFML_INCLUDE_DIR="deps/sfml/SFML-3.0.1/include" -DSFML_LIB_DIR="deps/sfml/SFML-3.0.1/lib"` to the cmake command.

## Features that might be added some day
* Serial port/UART
* A/D converter
* Models that can be connected to the ports and played around with.
* Inline editing of SFR registers and memory while the simulator is running.
* Support for more assembly features (like variables, including other files, "meta" mnemonics).
* Memory breakpoints

## Other notes
In theory you can use this code in your own project by just including Processor.hpp/.cpp (+stdafx.hpp) and you'll have a full simulator at your service.
The "Processor" and "Encoding+Processor" modules are designed to be independent of "main", which mostly implements gui stuff. The only thing that needs to be provided is a generic "void log( const std::string & )" function for logging (see stdafx.hpp).

## Sources
* https://en.wikipedia.org/wiki/Intel_8051
* https://opencores.org/websvn/filedetails?repname=8051&path=%2F8051%2Ftrunk%2Fdoc%2Fpdf%2Foc8051_spec.pdf
* https://www.keil.com/support/man/docs/is51
* https://technobyte.org/8051-special-function-registers-sfr/#Program_status_word_PSW
