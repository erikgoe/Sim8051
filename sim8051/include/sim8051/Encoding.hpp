#pragma once

#include "sim8051/stdafx.hpp"
#include "sim8051/Processor.hpp"

String to_hex_str( u16 val, u8 bytes = 8 );
String to_hex_str_signed( i8 val );

/// Decode instructions to detect single op codes.
void decode_instructions( const Processor &processor, std::vector<u16> &op_code_indices );

/// Decodes instructions and translates them into a humand-readable string with live data from the processor.
String get_decoded_instruction_string( Processor &processor, u16 code_addr );

/// Compiles assembly code into machine code and writes it in hex-format to a file.
void compile_assembly( const String &code, std::ostream &output );
