#pragma once

#include "sim8051/stdafx.hpp"

/// Holds processor context and does the simulation.
class Processor {
    u8 invalid_byte; // Used for invalid access (like accessing invalid direct addresses)

    bool int0_in_mem = false;
    bool int1_in_mem = false;
    bool is_in_interrupt = false;
    bool is_in_high_prio_intr = false;
    bool was_in_interrupt = false; // One instruction after RETI is always executed (see specifaction):

public:
    /// Returns the value at a direct address.
    u8 &direct_acc( u8 addr );
    /// Returns whether bit is set.
    bool is_bit_set( u8 bit_addr );
    /// Sets or clears a bit.
    void set_bit_to( u8 bit_addr, bool value );

    std::array<u8, 128> sfr; // Special Function Registers address space.
    std::array<u8, 256> iram; // Internal RAM.
    std::array<u8, 64 * 1024> xram; // External RAM.
    std::array<u8, 64 * 1024> text; // Source code.
    u16 pc = 0; // Program Counter.

    /// Metadata
    size_t cycle_count = 0;

    // Breakpoint functionality
    u8 break_instruction = 0; // Always break on this instruction (could be set to 0xA5 to "disable").
    std::vector<u16> break_addresses; // Break always on these addresses.
    std::function<void( Processor & )> break_callback = []( auto && ) {}; // Called on a breakpoint.

    /// Load source code from a HEX-file. Returns true on success.
    bool load_hex_code( const String &file );

    /// Resets all state (except ram and text/code).
    void reset();

    /// Resets all state (except text/code).
    void full_reset();

    /// Performs one or more machine cycles (multiple 12 ticks on the original MCU).
    /// The cycle count depends on the executed instruction. Always one instruction is executed.
    void do_cycle();
};
