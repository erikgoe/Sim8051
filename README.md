# Sim8051
A simple simulator for the 8051 architecture. Includes a disassembler and a simple assembly editor (+assembler) for a more streamlined workflow.

![Screenshot of the simulator user interface](https://github.com/erikgoe/Sim8051/blob/main/doc/screenshot.png "User interface")

## Why?
I need a simulator for my little compiler project and didn't find a simple one that compiles on linux, so I thought it would make sense to "quickly" write my own.

## Usage notes
* GUI docking: I recommend to create a proper layout by moving the sub-windows to the window edges.
* The simulator is designed following the documentation in the "sources" section below.
* Vague syntax specification for the integrated assembler can be found in Encoding.hpp. Available mnemonics can be found in keil's documentation (see sources section).
* A few keyboard shortcuts are supported: Space (single step), R (reset MCU), CTRL+Enter while editing (save & compile), P (run/pause), L (reload all files and compile).
* Breakpoints can be set by clicking on the left column in assembly view. You can also change the "break instruction".
* The simulation does not mirror the hardware one-to-one. Some features like interrupts might trigger one cycle too late or ports may behave differently.
* Labels must contain at least one non-hexadecimal character to be usable.
* Labels can be used with any jump instructions and intstruction 0x90 (mov dptr, <value/label>)

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
    TBD (probably similar to the linux procedure)...

## Missing things
* Interrupts
* Timer
* Power modes

## Other notes
In theory you can use this code in your own project by just including Processor.hpp/.cpp (+stdafx.hpp) and you'll have a full simulator at your service.
The "Processor" and "Encoding+Processor" modules are designed to be independent of "main", which mostly implements gui stuff. The only thing that needs to be provided is a generic "void log( const std::string & )" function for logging (see stdafx.hpp).

## Sources
* https://en.wikipedia.org/wiki/Intel_8051
* https://opencores.org/websvn/filedetails?repname=8051&path=%2F8051%2Ftrunk%2Fdoc%2Fpdf%2Foc8051_spec.pdf
* https://www.keil.com/support/man/docs/is51
* https://technobyte.org/8051-special-function-registers-sfr/#Program_status_word_PSW
