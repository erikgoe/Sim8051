# Sim8051
Simulator for the 8051 architecture.

See https://www.keil.com/support/man/docs/is51 for instruction set.

## Why?
I need a simulator for my little compiler project and didn't find a simple one that compiles on linux, so I thought it would make sense to quickly write my own.

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
    cat imgui-sfml/imconfig-SFML.h >> imgui/imconfig.h
    cd ..
    
    mkdir build && cd build
    cmake ..
    make
