#pragma once

#include "sim8051/stdafx.hpp"
#include "sim8051/Processor.hpp"

String to_hex_str( u16 val, u8 bytes = 8 );
String to_hex_str_signed( i8 val );

/// Decode instructions to detect single op codes.
void decode_instructions( const Processor &processor, std::vector<u16> &op_code_indices );

/// Decodes instructions and translates them into a humand-readable string with live data from the processor.
String get_decoded_instruction_string( Processor &processor, u16 code_addr );

/// Transfers all characters of an ASCII-String to lower case.
String to_lower( const String &str );

/// Compiles assembly code into machine code and writes it in hex-format to a file.
/// Accepted instruction syntax:
/// Commas between parameters are optional.
/// All numbers are interpreted in base 16.
/// Addresses are generally written in parenthesis or as SFRs.
/// The first parameter is always an address (and thus need no additional parenthesis).
/// Some registers are used as indirection address when written in parenthesis.
/// Keep in mind that assembler and disassembler are distinct systems, i. e. the syntax and registers of one don't
/// always apply to the other.
/// Bits can't be accessed with syntax like "A.1". Still some common bits (like C) can be used (see table rev_sfr_map
/// below) directly.
/// You will most likely need to wrap bit addresses in parenthesis, as they are treated like normal addresses and
/// thus need indirection.
void compile_assembly( const String &code, std::ostream &output );
