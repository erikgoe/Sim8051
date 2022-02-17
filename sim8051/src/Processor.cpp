#include "sim8051/stdafx.hpp"
#include "sim8051/Processor.hpp"

constexpr std::array<u8, 24> valid_sfr_addresses = { 0xE0, 0xF0, 0xD0, 0xB8, 0xA8, 0x82, 0x83, 0x80,
                                                     0x90, 0xA0, 0xB0, 0x87, 0x98, 0x99, 0x88, 0xC8,
                                                     0x89, 0x9A, 0x9B, 0xCC, 0x9C, 0x9D, 0xCD, 0x81 };

u8 &Processor::direct_acc( u8 addr ) {
    if ( addr < 0x80 ) {
        return iram[addr];
    } else {
        if ( std::find( valid_sfr_addresses.begin(), valid_sfr_addresses.end(), addr ) ) {
            return sfr[addr - 0x80];
        } else {
            log( "Invalid access to sfr at address " + to_string( addr ) + ", PC: " + to_string( pc ) );
            invalid_byte = 0;
            return invalid_byte;
        }
    }
}

bool Processor::is_bit_set( u8 bit_addr ) {
    if ( bit_addr < 0x80 ) {
        return ( iram[0x20 + ( ( bit_addr & 0b11111000 ) >> 3 )] >> ( bit_addr & 0b111 ) ) & 1;
    } else {
        return ( direct_acc( 0x80 + ( bit_addr & 0b01111000 ) ) >> ( bit_addr & 0b111 ) ) & 1;
    }
}

void Processor::set_bit_to( u8 bit_addr, bool value ) {
    u8 *byte;
    if ( bit_addr < 0x80 ) {
        byte = &iram[0x20 + ( ( bit_addr & 0b11111000 ) >> 3 )];
    } else {
        byte = &direct_acc( 0x80 + ( bit_addr & 0b01111000 ) );
    }
    if ( value ) {
        ( *byte ) |= 1 >> ( bit_addr & 0b111 );
    } else {
        ( *byte ) &= ~( 1 << ( bit_addr & 0b111 ) );
    }
}

bool Processor::load_hex_code( const String &file ) {
    // Clear state
    text.fill( 0 );
    reset();

    // Open file
    std::ifstream stream( file );
    if ( !stream.good() ) {
        log( "Failed to load hex file!" );
        return false;
    }


    // Load file
    String str;
    size_t line_ctr = 0;
    while ( std::getline( stream, str ) ) {
        u8 checksum = 0;
        line_ctr++;

        if ( str.find( ':' ) == str.npos ) {
            log( "Synatax error at line " + to_string( line_ctr ) );
            return false;
        }
        str = str.substr( str.find( ':' ) ); // Ignore leading text.

        if ( str.substr( 7, 2 ) == "00" ) {
            // Normal data line.
            size_t size = stoi( str.substr( 1, 2 ), 0, 16 );
            size_t addr = stoi( str.substr( 3, 4 ), 0, 16 );
            checksum += size + ( addr & 0xff ) + ( addr >> 8 );
            for ( size_t i = 0; i < size; i++ ) {
                u8 byte = stoi( str.substr( 9 + i * 2, 2 ), 0, 16 );
                text[addr + i] = byte;
                checksum += byte;
            }
            if ( static_cast<u8>( checksum + stoi( str.substr( 9 + size * 2, 2 ), 0, 16 ) ) != 0 ) {
                log( "Checksum error at line " + to_string( line_ctr ) );
                return false;
            }
        } else if ( str.substr( 7, 2 ) == "01" ) {
            // End line
            if ( str != ":00000001FF" ) {
                log( "Syntax error at termination line " + to_string( line_ctr ) );
                return false;
            }
            return true;
        }
    }

    log( "File not terminated properly!" );
    return false;
}

void Processor::reset() {
    sfr.fill( 0 );
    iram.fill( 0 );
    xram.fill( 0 );
    pc = 0;
    cycle_count = 0;
}

bool parity_of_byte( u8 byte ) {
    bool p = false;
    for ( u8 i = 0; i < 8; i++ ) {
        if ( byte & ( 1 << i ) )
            p = !p;
    }
    return p;
}

void Processor::do_cycle() {
    u8 instr = text[pc];
    u8 arg1 = text[pc + (u16) 1];
    u8 arg2 = text[pc + (u16) 2];

    // Common constants
    constexpr u8 parity_addr = 0xD0; // Address of parity bit.
    constexpr u8 overflow_addr = 0xD2; // Address of overflow flag.
    constexpr u8 auxilary_addr = 0xD6; // Address of auxilary carry.
    constexpr u8 carry_addr = 0xD7; // Address of carry bit.
    constexpr u8 acc_0_addr = 0xE0; // Address of first bit in accumulator.
    constexpr u8 acc_7_addr = 0xE7; // Address of last bit in accumulator.

    // SFRs
    auto &a = direct_acc( 0xE0 );
    auto &b = direct_acc( 0xF0 );
    auto &psw = direct_acc( 0xD0 );
    // auto &ip = direct_acc(0xB8);
    // auto &ie = direct_acc(0xA8);
    auto &dpl = direct_acc( 0x82 );
    auto &dph = direct_acc( 0x83 );
    // auto &p0 = direct_acc(0x80);
    // auto &p1 = direct_acc(0x90);
    auto &p2 = direct_acc( 0xA0 );
    // auto &p3 = direct_acc(0xB0);
    // auto &pcon = direct_acc(0x87);
    // auto &scon = direct_acc(0x98);
    // auto &sbuf = direct_acc(0x99);
    // auto &tcon = direct_acc(0x88);
    // auto &t2con = direct_acc(0xC8);
    // auto &tmod = direct_acc(0x89);
    // auto &tl0 = direct_acc(0x9A);
    // auto &tl1 = direct_acc(0x9B);
    // auto &tl2 = direct_acc(0xCC);
    // auto &th0 = direct_acc(0x9C);
    // auto &th1 = direct_acc(0x9D);
    // auto &th2 = direct_acc(0xCD);
    auto &sp = direct_acc( 0x81 );

    auto bank_nr = ( psw & 0x18 ) >> 3;
    auto *r0_ptr = &iram[8 * bank_nr];
    auto &r0 = *( r0_ptr + 0x00 );
    auto &r1 = *( r0_ptr + 0x01 );
    auto &r2 = *( r0_ptr + 0x02 );
    auto &r3 = *( r0_ptr + 0x03 );
    auto &r4 = *( r0_ptr + 0x04 );
    auto &r5 = *( r0_ptr + 0x05 );
    auto &r6 = *( r0_ptr + 0x06 );
    auto &r7 = *( r0_ptr + 0x07 );

    // Utility data.
    bool bit = false;
    i16 result;
    u16 inc_pc = 1;

    // Execute the instruction.
    u8 ls_nibble = instr & 0xf;
    u8 ms_nibble = ( instr & 0xf0 ) >> 4;
    if ( ls_nibble > 3 ) {
        // Regular instruction
        u8 *value;
        u8 *second_operand;
        if ( ls_nibble == 4 ) {
            // Immediate
            value = &arg1;
            second_operand = &arg2;
            inc_pc = 2;
        } else if ( ls_nibble == 5 ) {
            // Direct access
            value = &direct_acc( arg1 );
            second_operand = &arg2;
            inc_pc = 2;
        } else if ( ls_nibble == 6 ) {
            // Indirect R0 access
            value = &iram[r0];
            second_operand = &arg1;
        } else if ( ls_nibble == 7 ) {
            // Indirect R1 access
            value = &iram[r1];
            second_operand = &arg1;
        } else {
            // Register
            value = r0_ptr + ls_nibble - 8;
            second_operand = &arg1;
        }

        // Process the instruction
        switch ( ms_nibble ) {
        case 0x0: // INC operand
            if ( ls_nibble == 4 ) {
                a++;
                set_bit_to( parity_addr, parity_of_byte( a ) );
            } else {
                value++;
            }
            break;
        case 0x1: // DEC operand
            if ( ls_nibble == 4 ) {
                a--;
                set_bit_to( parity_addr, parity_of_byte( a ) );
            } else {
                value--;
            }
            break;
        case 0x2: // ADD A,operand
            result = static_cast<i16>( a ) + static_cast<i16>( *value );
            set_bit_to( overflow_addr, result > 0xff );
            set_bit_to( carry_addr, result < 0 );
            set_bit_to( auxilary_addr, ( a & 0b1000 ) == 1 && ( static_cast<u8>( result ) & 0b1000 ) == 0 );
            a = result;
            set_bit_to( parity_addr, parity_of_byte( a ) );
            break;
        case 0x3: // ADDC A,operand
            result = static_cast<i16>( a ) + static_cast<i16>( *value ) + ( is_bit_set( carry_addr ) ? 1 : 0 );
            set_bit_to( overflow_addr, result > 0xff );
            set_bit_to( carry_addr, result < 0 );
            set_bit_to( auxilary_addr, ( a & 0b1000 ) == 1 && ( static_cast<u8>( result ) & 0b1000 ) == 0 );
            a = result;
            set_bit_to( parity_addr, parity_of_byte( a ) );
            break;
        case 0x4: // ORL A,operand
            a |= *value;
            set_bit_to( parity_addr, parity_of_byte( a ) );
            break;
        case 0x5: // ANL A,operand
            a &= *value;
            set_bit_to( parity_addr, parity_of_byte( a ) );
            break;
        case 0x6: // XRL A,operand
            a ^= *value;
            set_bit_to( parity_addr, parity_of_byte( a ) );
            break;
        case 0x7: // MOV operand,#data
            if ( ls_nibble == 4 ) {
                a = *value;
                set_bit_to( parity_addr, parity_of_byte( a ) );
            } else {
                *value = *second_operand;
                inc_pc++;
            }
            break;
        case 0x8: // MOV address,operand
            if ( ls_nibble == 4 ) {
                // Actually encodes division
                set_bit_to( carry_addr, false );
                if ( b == 0 ) {
                    log( "Division by zero!" );
                    set_bit_to( overflow_addr, true );
                } else {
                    auto rem = a - a / b;
                    a = a / b;
                    b = rem;
                    set_bit_to( overflow_addr, false );
                    set_bit_to( parity_addr, parity_of_byte( a ) );
                }
            } else if ( ls_nibble == 5 ) {
                direct_acc( arg2 ) = *value; // Swapped parameters!
                inc_pc = 3;
            } else {
                direct_acc( arg1 ) = *value;
                inc_pc = 2;
            }
            break;
        case 0x9: // SUBB A,operand
            result = static_cast<i16>( a ) - static_cast<i16>( *value ) - ( is_bit_set( carry_addr ) ? 1 : 0 );
            set_bit_to( overflow_addr, result > 0xff );
            set_bit_to( carry_addr, result < 0 );
            set_bit_to( auxilary_addr, ( a & 0b1000 ) == 1 && ( static_cast<u8>( result ) & 0b1000 ) == 0 );
            a = result;
            set_bit_to( parity_addr, parity_of_byte( a ) );
            break;
        case 0xA: // MOV operand,address
            if ( ls_nibble == 4 ) {
                // Actually encodes multiplication
                set_bit_to( carry_addr, false );
                u16 prod = static_cast<u16>( a ) * static_cast<u16>( b );
                a = prod;
                b = prod >> 8;
                set_bit_to( overflow_addr, prod > 0xff );
                set_bit_to( parity_addr, parity_of_byte( a ) );
            } else if ( ls_nibble == 5 ) {
                // Reserved instruction
            } else {
                *value = direct_acc( arg1 );
                inc_pc = 2;
            }
            break;
        case 0xB: // CJNE operand,#data,offset
            if ( ls_nibble == 4 ) {
                pc += 3;
                if ( a != arg1 )
                    pc += *second_operand;
                set_bit_to( carry_addr, a < arg1 );
            } else if ( ls_nibble == 5 ) {
                pc += 3;
                if ( a != *value )
                    pc += *second_operand;
                set_bit_to( carry_addr, a < *value );
            } else {
                pc += 3;
                if ( *value != arg1 )
                    pc += *second_operand;
                set_bit_to( carry_addr, *value < arg1 );
            }
            inc_pc = 0;
            break;
        case 0xC: // XCH A,operand
            if ( ls_nibble == 4 ) {
                // Actually encodes swap A nibbles
                a = ( ( a & 0xf ) << 4 ) | ( ( a & 0xf0 ) >> 4 );
            } else {
                auto tmp = *value;
                *value = a;
                a = tmp;
                set_bit_to( parity_addr, parity_of_byte( a ) );
            }
            break;
        case 0xD: // DJNZ operand,offset
            if ( ls_nibble == 4 ) {
                // Actually encodes DA A
                if ( ( a & 0xf ) > 9 || is_bit_set( auxilary_addr ) ) {
                    if ( static_cast<u16>( a ) + 6 > 0xff )
                        set_bit_to( carry_addr, true );
                    a += 6;
                }
                if ( ( ( a & 0xf0 ) >> 4 ) > 9 || is_bit_set( carry_addr ) ) {
                    if ( static_cast<u16>( a ) + 6 > 0xff )
                        set_bit_to( carry_addr, true );
                    a += 0x60;
                }
                set_bit_to( parity_addr, parity_of_byte( a ) );
            } else if ( ls_nibble == 6 || ls_nibble == 7 ) {
                // Actually encodes XCHD
                u8 tmp = *value & 0xf;
                *value = ( *value & 0xf0 ) | ( a & 0xf );
                a = ( a & 0xf0 ) | tmp;
            } else if ( ls_nibble == 5 ) {
                pc += 3; // Documentation specifies 2, but 3 makes more sense.
                ( *value )--;
                if ( *value != 0 )
                    pc += *second_operand;
                inc_pc = 0;
            } else {
                pc += 2;
                ( *value )--;
                if ( *value != 0 )
                    pc += arg1;
                inc_pc = 0;
            }
            break;
        case 0xE: // MOV A,operand
            if ( ls_nibble == 4 ) {
                // Actually encodes CLR A
                a = 0;
            } else {
                a = *value;
                set_bit_to( parity_addr, parity_of_byte( a ) );
            }
            break;
        case 0xF: // MOV operand,A
            if ( ls_nibble == 4 ) {
                // Actually encodes CPL A
                a = ~a;
            } else {
                *value = a;
                set_bit_to( parity_addr, parity_of_byte( a ) );
            }
            break;

        default:
            break;
        }
    } else {
        // Irregular instruction
        if ( ( instr & 0b11111 ) == 1 ) {
            // AJMP addr11
            pc += 2;
            pc = ( pc & 0b1111100000000000 ) + ( static_cast<u16>( instr & 0b11100000 ) << 3 ) + arg1;
            inc_pc = 0;
        } else if ( ( instr & 0b11111 ) == 0x11 ) {
            // ACALL addr11
            pc += 2;
            sp++;
            iram[sp] = pc & 0xff;
            sp++;
            iram[sp] = pc & 0xff00;
            pc = ( pc & 0b1111100000000000 ) + ( static_cast<u16>( instr & 0b11100000 ) << 3 ) + arg1;
            inc_pc = 0;
        } else {
            if ( ls_nibble == 0 ) {
                switch ( ms_nibble ) {
                case 0x0: // NOP
                    break;
                case 0x1: // JBC bit,offset
                    pc += 3;
                    if ( is_bit_set( arg1 ) ) {
                        set_bit_to( arg1, false );
                        pc += arg2;
                    }
                    inc_pc = 0;
                    break;
                case 0x2: // JB bit,offset
                    pc += 3;
                    if ( is_bit_set( arg1 ) ) {
                        pc += arg2;
                    }
                    inc_pc = 0;
                    break;
                case 0x3: // JNB bit,offset
                    pc += 3;
                    if ( !is_bit_set( arg1 ) ) {
                        pc += arg2;
                    }
                    inc_pc = 0;
                    break;
                case 0x4: // JC offset
                    pc += 2;
                    if ( is_bit_set( carry_addr ) ) {
                        pc += arg1;
                    }
                    inc_pc = 0;
                    break;
                case 0x5: // JNC offset
                    pc += 2;
                    if ( !is_bit_set( carry_addr ) ) {
                        pc += arg1;
                    }
                    inc_pc = 0;
                    break;
                case 0x6: // JZ offset
                    pc += 2;
                    if ( a == 0 ) {
                        pc += arg1;
                    }
                    inc_pc = 0;
                    break;
                case 0x7: // JNZ offset
                    pc += 2;
                    if ( a != 0 ) {
                        pc += arg1;
                    }
                    inc_pc = 0;
                    break;
                case 0x8: // SJMP offset
                    pc += 2 + arg1;
                    inc_pc = 0;
                    break;
                case 0x9: // MOV DPTR,#data16
                    dph = arg1;
                    dpl = arg2;
                    inc_pc = 3;
                    break;
                case 0xA: // ORL C,/bit
                    set_bit_to( carry_addr, is_bit_set( carry_addr ) | !is_bit_set( arg1 ) );
                    inc_pc = 2;
                    break;
                case 0xB: // ANL C,/bit
                    set_bit_to( carry_addr, is_bit_set( carry_addr ) & !is_bit_set( arg1 ) );
                    inc_pc = 2;
                    break;
                case 0xC: // PUSH address
                    sp++;
                    iram[sp] = direct_acc( arg1 );
                    inc_pc = 2;
                    break;
                case 0xD: // POP address
                    direct_acc( arg1 ) = iram[sp];
                    sp--;
                    inc_pc = 2;
                    break;
                case 0xE: // MOVX A,@DPTR
                    a = xram[( static_cast<u16>( dph ) << 8 ) + dpl];
                    set_bit_to( parity_addr, parity_of_byte( a ) );
                    break;
                case 0xF: // MOVX @DPTR,A
                    xram[( static_cast<u16>( dph ) << 8 ) | dpl] = a; // Missing in documentation, but this makes sense.
                    break;

                default:
                    break;
                }
            } else if ( ls_nibble == 2 ) {
                switch ( ms_nibble ) {
                case 0x0: // LJMP addr16
                    pc = ( static_cast<u16>( arg1 ) << 8 ) | arg2;
                    inc_pc = 0;
                    break;
                case 0x1: // LCALL addr16
                    pc += 3;
                    sp++;
                    iram[sp] = pc & 0xff;
                    sp++;
                    iram[sp] = pc & 0xff00;
                    pc = ( static_cast<u16>( arg1 ) << 8 ) | arg2;
                    inc_pc = 0;
                    break;
                case 0x2: // RET
                    pc = ( static_cast<u16>( iram[sp - 1] ) << 8 ) | iram[sp];
                    sp -= 2;
                    inc_pc = 0;
                    break;
                case 0x3: // RETI
                    pc = ( static_cast<u16>( iram[sp - 1] ) << 8 ) | iram[sp];
                    sp -= 2;
                    inc_pc = 0;
                    // TODO restore interrupt logic.
                    break;
                case 0x4: // ORL address,A
                    direct_acc( arg1 ) |= a;
                    inc_pc = 2;
                    break;
                case 0x5: // ANL address,A
                    direct_acc( arg1 ) &= a;
                    inc_pc = 2;
                    break;
                case 0x6: // XRL address,A
                    direct_acc( arg1 ) ^= a;
                    inc_pc = 2;
                    break;
                case 0x7: // ORL C,bit
                    set_bit_to( carry_addr, is_bit_set( carry_addr ) | is_bit_set( arg1 ) );
                    inc_pc = 2;
                    break;
                case 0x8: // ANL C,bit
                    set_bit_to( carry_addr, is_bit_set( carry_addr ) & is_bit_set( arg1 ) );
                    inc_pc = 2;
                    break;
                case 0x9: // MOV bit,C
                    set_bit_to( arg1, is_bit_set( carry_addr ) );
                    inc_pc = 2;
                    break;
                case 0xA: // MOV C,bit
                    set_bit_to( carry_addr, is_bit_set( arg1 ) );
                    inc_pc = 2;
                    break;
                case 0xB: // CPL bit
                    set_bit_to( arg1, !is_bit_set( arg1 ) );
                    inc_pc = 2;
                    break;
                case 0xC: // CLR bit
                    set_bit_to( arg1, false );
                    inc_pc = 2;
                    break;
                case 0xD: // SETB bit
                    set_bit_to( arg1, true );
                    inc_pc = 2;
                    break;
                case 0xE: // MOVX A,@R0
                    a = xram[( static_cast<u16>( p2 ) << 8 ) + r0];
                    set_bit_to( parity_addr, parity_of_byte( a ) );
                    break;
                case 0xF: // MOVX @R0,A
                    xram[( static_cast<u16>( p2 ) << 8 ) + r0] = a;
                    break;

                default:
                    break;
                }
            } else if ( ls_nibble == 3 ) {
                switch ( ms_nibble ) {
                case 0x00: // RR A
                    bit = is_bit_set( acc_0_addr );
                    a >>= 1;
                    if ( bit )
                        a |= 0b10000000;
                    break;
                case 0x1: // RRC A
                    bit = is_bit_set( acc_0_addr );
                    a >>= 1;
                    set_bit_to( acc_7_addr, carry_addr );
                    set_bit_to( carry_addr, bit );
                    set_bit_to( parity_addr, parity_of_byte( a ) );
                    break;
                case 0x2: // RL A
                    bit = is_bit_set( acc_7_addr );
                    a <<= 1;
                    set_bit_to( acc_0_addr, bit );
                    break;
                case 0x3: // RLC A
                    bit = is_bit_set( acc_7_addr );
                    a <<= 1;
                    set_bit_to( acc_0_addr, carry_addr );
                    set_bit_to( carry_addr, bit );
                    set_bit_to( parity_addr, parity_of_byte( a ) );
                    break;
                case 0x4: // ORL address,#data
                    direct_acc( arg1 ) |= arg2;
                    inc_pc = 3;
                    break;
                case 0x5: // ANL address,#data
                    direct_acc( arg1 ) &= arg2;
                    inc_pc = 3;
                    break;
                case 0x6: // XRL address,#data
                    direct_acc( arg1 ) ^= arg2;
                    inc_pc = 3;
                    break;
                case 0x7: // JMP @A+DPTR
                    pc = ( ( static_cast<u16>( dph ) << 8 ) | dpl ) + static_cast<u16>( a );
                    inc_pc = 0;
                    break;
                case 0x8: // MOVC A,@A+PC
                    pc++;
                    a = text[pc + static_cast<u16>( a )];
                    set_bit_to( parity_addr, parity_of_byte( a ) );
                    inc_pc = 0;
                    break;
                case 0x9: // MOVC A,@A+DPTR
                    a = text[( ( static_cast<u16>( dph ) << 8 ) | dpl ) + static_cast<u16>( a )];
                    set_bit_to( parity_addr, parity_of_byte( a ) );
                    break;
                case 0xA: // INC DPTR
                    dpl++;
                    if ( dpl == 0 )
                        dph++;
                    break;
                case 0xB: // CPL C
                    set_bit_to( carry_addr, !is_bit_set( carry_addr ) );
                    break;
                case 0xC: // CLR C
                    set_bit_to( carry_addr, false );
                    break;
                case 0xD: // SETB C
                    set_bit_to( carry_addr, true );
                    break;
                case 0xE: // MOVX A,@R1
                    a = xram[( static_cast<u16>( p2 ) << 8 ) + r1];
                    set_bit_to( parity_addr, parity_of_byte( a ) );
                    break;
                case 0xF: // MOVX @R1,A
                    xram[( static_cast<u16>( p2 ) << 8 ) + r1] = a;
                    break;

                default:
                    break;
                }
            }
        }
    }

    pc += inc_pc;

    cycle_count++;
}
