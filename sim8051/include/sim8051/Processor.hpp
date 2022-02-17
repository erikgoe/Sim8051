#include "sim8051/stdafx.hpp"

/// Holds processor context and does the simulation.
class Processor {
    u8 invalid_byte; // Used for invalid access (like accessing invalid direct addresses)
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

    /// Load source code from a HEX-file. Returns true on success.
    bool load_hex_code( const String &file );

    /// Resets all state (except text/code).
    void reset();

    /// Performs one machine cycle (12 ticks on the original MCU).
    void do_cycle();
};
