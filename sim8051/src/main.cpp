#include "sim8051/stdafx.hpp"
#include "sim8051/Processor.hpp"
#include "sim8051/Encoding.hpp"

#include "SFML/System.hpp"
#include "SFML/Window.hpp"
#include "SFML/Graphics.hpp"
#include "imgui-SFML.h"
#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"

std::deque<String> global_log;
sf::Clock last_global_log_timer;

char to_human_readable_ascii( u8 c ) {
    if ( c < ' ' || c >= 0x7f ) {
        return '.';
    } else {
        return c;
    }
}

// The main loop, including some stuff like scrolling
int main() {
    // Initialize all the stuff
    sf::ContextSettings context_settings( 0, 0, 4 );
    sf::RenderWindow window( sf::VideoMode( 1200, 800, 32 ), "Sim8051", sf::Style::Default, context_settings );
    bool fullscreen = false;
    window.setVerticalSyncEnabled( true );
    auto view = window.getDefaultView();
    window.setView( view );
    if ( !ImGui::SFML::Init( window ) ) {
        log( "Failed to initialize ImGui" );
        return -1;
    }
    sf::Clock timer;
    last_global_log_timer.restart();
    size_t last_log_size = global_log.size();
    size_t last_pc = 0;
    bool should_save = false;
    bool should_compile = false;
    bool should_load = true;
    int calc_val = 0;

    // ImGui configuration
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    bool running = true;
    sf::Event evt;


    // Simulation stuff
    size_t steps_per_frame = 0;
    bool pause_next_frame = false;
    bool max_speed = false;
    u32 fix_target_frequency = 12000;
    bool use_fix_target_frequency = false;
    std::vector<u16> op_code_indices; // Pointers to the op codes.
    auto processor = std::make_shared<Processor>();
    String hex_filename = "tests/simple.hex";
    String editor_asm_filename = "tests/simple.a51";
    String editor_hex_file_dir = "tests";
    String editor_content = "";

    // Load simulation hex.
    if ( processor->load_hex_code( hex_filename ) )
        decode_instructions( *processor, op_code_indices );

    // Breakpoint callback
    processor->break_callback = [&]( auto &&processor ) {
        steps_per_frame = steps_per_frame == 0;
        max_speed = false;
        use_fix_target_frequency = false;
        log( "Hit breakpoint at instruction '" + to_hex_str( processor.pc ) + "'" );
    };

    // Main loop
    while ( running ) {
        // Calculate delta time
        auto delta_time = timer.restart();

        // Event handling
        while ( window.pollEvent( evt ) ) {
            ImGui::SFML::ProcessEvent( evt );
            bool not_on_gui = !ImGui::IsAnyItemHovered() && !ImGui::IsWindowHovered( ImGuiHoveredFlags_AnyWindow );
            bool not_key_insert_gui = !ImGui::IsAnyItemActive();

            switch ( evt.type ) {
            case sf::Event::Closed:
                running = false;
                break;
            case sf::Event::KeyPressed:
                if ( not_key_insert_gui ) {
                    // Key input
                    if ( evt.key.code == sf::Keyboard::Space ) {
                        // Single step
                        steps_per_frame = 1;
                        pause_next_frame = true;
                        max_speed = false;
                        use_fix_target_frequency = false;
                    } else if ( evt.key.code == sf::Keyboard::R ) {
                        processor->reset();
                    } else if ( evt.key.code == sf::Keyboard::P ) {
                        max_speed = false;
                        steps_per_frame = steps_per_frame == 0 ? 1 : 0;
                        use_fix_target_frequency = evt.key.shift;
                    } else if ( evt.key.code == sf::Keyboard::L ) {
                        should_compile = true;
                        should_load = true;
                    }
                }
                break;
            case sf::Event::MouseButtonPressed:
                if ( not_on_gui ) {
                    auto world_coords = window.mapPixelToCoords( sf::Vector2i( evt.mouseButton.x, evt.mouseButton.y ) );
                }
                break;
            case sf::Event::MouseButtonReleased:
                break;

            case sf::Event::MouseMoved: {
                auto world_coords = window.mapPixelToCoords( sf::Vector2i( evt.mouseMove.x, evt.mouseMove.y ) );

            } break;

            case sf::Event::MouseWheelScrolled:
                if ( not_on_gui ) {
                }
                break;
            }
        }

        // Updating

        ImGui::SFML::Update( window, delta_time );

        // Simulation
        for ( size_t i = 0; i < steps_per_frame; i++ ) {
            processor->do_cycle();
        }
        if ( pause_next_frame ) {
            pause_next_frame = false;
            steps_per_frame = 0;
        } else if ( max_speed ) {
            if ( delta_time.asSeconds() < 1.f / 40.f ) {
                steps_per_frame = steps_per_frame * 1.2 + 1;
            } else {
                steps_per_frame = std::max<u32>( 1, steps_per_frame / 1.2 );
            }
        } else if ( use_fix_target_frequency ) {
            steps_per_frame = delta_time.asSeconds() * fix_target_frequency;
        }

        // Gui
        ImGui::DockSpaceOverViewport();

        ImGui::Begin( "Control" );
        ImGui::Text( String( "Cycle count: " + to_string( processor->cycle_count ) +
                             ( steps_per_frame == 0 ? " (Paused)" : "" ) )
                         .c_str() );
        ImGui::Text( String( "Cycles per frame: " + to_string( steps_per_frame ) ).c_str() );
        ImGui::Text( String( "Frames per second: " + to_string( 1.f / delta_time.asSeconds() ) ).c_str() );
        if ( ImGui::Button( steps_per_frame != 0 ? "Pause" : "Run" ) ) {
            steps_per_frame = steps_per_frame == 0 ? 1 : 0;
            max_speed = false;
            use_fix_target_frequency = false;
        }
        if ( ImGui::Button( "Single step" ) ) {
            steps_per_frame = 1;
            pause_next_frame = true;
            max_speed = false;
            use_fix_target_frequency = false;
        }
        if ( ImGui::Button( "Max speed" ) ) {
            max_speed = true;
            use_fix_target_frequency = false;
        }
        if ( ImGui::Button( "Run (12 MHz)" ) ) {
            max_speed = false;
            use_fix_target_frequency = true;
        }
        if ( ImGui::Button( "Reset Pin" ) ) {
            processor->reset();
        }
        if ( ImGui::Button( "Reset MCU (full)" ) ) {
            processor->full_reset();
        }
        ImGui::Spacing();
        if ( ImGui::InputText( "Hex file", &hex_filename, ImGuiInputTextFlags_EnterReturnsTrue ) |
             ImGui::Button( "Load" ) ) {
            steps_per_frame = 0;
            max_speed = false;
            use_fix_target_frequency = false;
            if ( processor->load_hex_code( hex_filename ) ) {
                decode_instructions( *processor, op_code_indices );
                log( "Loaded hex file" );
            }
        }

        ImGui::Spacing();
        String break_instr_str = to_hex_str( processor->break_instruction );
        if ( ImGui::InputText( "Break instruction", &break_instr_str, ImGuiInputTextFlags_EnterReturnsTrue ) ) {
            processor->break_instruction = stoi( break_instr_str, 0, 16 );
        }

        ImGui::Spacing();
        if ( ImGui::Button( "Interrupt 0" ) ) {
            processor->set_bit_to( 0xB2, 0 );
        }
        if ( ImGui::Button( "Interrupt 1" ) ) {
            processor->set_bit_to( 0xB3, 0 );
        }

        ImGui::End();

        ImGui::Begin( "Special function registers" );
        ImGui::Text( String( "PSW  = " + to_hex_str( processor->direct_acc( 0xD0 ) ) + " (" +
                             to_string( processor->direct_acc( 0xD0 ) ) + ")" )
                         .c_str() );
        ImGui::Text( String( "  C=" + String( processor->is_bit_set( 0xD7 ) ? "1" : "0" ) +
                             ", AC=" + ( processor->is_bit_set( 0xD6 ) ? "1" : "0" ) +
                             ", F0=" + ( processor->is_bit_set( 0xD5 ) ? "1" : "0" ) +
                             ", RS1=" + ( processor->is_bit_set( 0xD4 ) ? "1" : "0" ) +
                             ", RS0=" + ( processor->is_bit_set( 0xD3 ) ? "1" : "0" ) +
                             ", OV=" + ( processor->is_bit_set( 0xD2 ) ? "1" : "0" ) +
                             ", UD=" + ( processor->is_bit_set( 0xD1 ) ? "1" : "0" ) +
                             ", P=" + ( processor->is_bit_set( 0xD0 ) ? "1" : "0" ) )
                         .c_str() );
        ImGui::Text( String( "A    = " + to_hex_str( processor->direct_acc( 0xE0 ) ) + " (" +
                             to_string( processor->direct_acc( 0xE0 ) ) + ")" )
                         .c_str() );
        ImGui::Text( String( "B    = " + to_hex_str( processor->direct_acc( 0xF0 ) ) + " (" +
                             to_string( processor->direct_acc( 0xF0 ) ) + ")" )
                         .c_str() );
        u16 dptr_value = ( static_cast<u16>( processor->direct_acc( 0x83 ) ) << 8 ) + processor->direct_acc( 0x82 );
        ImGui::Text(
            String( "DPTR = " + to_hex_str( dptr_value, 16 ) + " (" + to_string( dptr_value ) + ")" ).c_str() );
        ImGui::Text( String( "SP   = " + to_hex_str( processor->direct_acc( 0x81 ) ) + " (" +
                             to_string( processor->direct_acc( 0x81 ) ) + ")" )
                         .c_str() );
        ImGui::Text(
            String( "PC   = " + to_hex_str( processor->pc, 16 ) + " (" + to_string( processor->pc ) + ")" ).c_str() );

        ImGui::Spacing();
        auto bank_nr = ( processor->direct_acc( 0xD0 ) & 0x18 ) >> 3;
        auto *r0_ptr = &processor->iram[8 * bank_nr];
        ImGui::Text( String( "R0   = " + to_hex_str( *r0_ptr ) + " (" + to_string( *r0_ptr ) + ")" ).c_str() );
        ImGui::Text(
            String( "R1   = " + to_hex_str( *( r0_ptr + 1 ) ) + " (" + to_string( *( r0_ptr + 1 ) ) + ")" ).c_str() );
        ImGui::Text(
            String( "R2   = " + to_hex_str( *( r0_ptr + 2 ) ) + " (" + to_string( *( r0_ptr + 2 ) ) + ")" ).c_str() );
        ImGui::Text(
            String( "R3   = " + to_hex_str( *( r0_ptr + 3 ) ) + " (" + to_string( *( r0_ptr + 3 ) ) + ")" ).c_str() );
        ImGui::Text(
            String( "R4   = " + to_hex_str( *( r0_ptr + 4 ) ) + " (" + to_string( *( r0_ptr + 4 ) ) + ")" ).c_str() );
        ImGui::Text(
            String( "R5   = " + to_hex_str( *( r0_ptr + 5 ) ) + " (" + to_string( *( r0_ptr + 5 ) ) + ")" ).c_str() );
        ImGui::Text(
            String( "R6   = " + to_hex_str( *( r0_ptr + 6 ) ) + " (" + to_string( *( r0_ptr + 6 ) ) + ")" ).c_str() );
        ImGui::Text(
            String( "R7   = " + to_hex_str( *( r0_ptr + 7 ) ) + " (" + to_string( *( r0_ptr + 7 ) ) + ")" ).c_str() );

        ImGui::Spacing();
        ImGui::Text( String( "P0   = " + to_hex_str( processor->direct_acc( 0x80 ) ) + " (" +
                             to_string( processor->direct_acc( 0x80 ) ) + ")" )
                         .c_str() );
        ImGui::Text( String( "P1   = " + to_hex_str( processor->direct_acc( 0x90 ) ) + " (" +
                             to_string( processor->direct_acc( 0x90 ) ) + ")" )
                         .c_str() );
        ImGui::Text( String( "P2   = " + to_hex_str( processor->direct_acc( 0xA0 ) ) + " (" +
                             to_string( processor->direct_acc( 0xA0 ) ) + ")" )
                         .c_str() );
        ImGui::Text( String( "P3   = " + to_hex_str( processor->direct_acc( 0xB0 ) ) + " (" +
                             to_string( processor->direct_acc( 0xB0 ) ) + ")" )
                         .c_str() );

        ImGui::Spacing();
        ImGui::Text( String( "PCON = " + to_hex_str( processor->direct_acc( 0x87 ) ) + " (" +
                             to_string( processor->direct_acc( 0x87 ) ) + ")" )
                         .c_str() );
        ImGui::Text( String( "IE   = " + to_hex_str( processor->direct_acc( 0xA8 ) ) + " (" +
                             to_string( processor->direct_acc( 0xA8 ) ) + ")" )
                         .c_str() );
        ImGui::Text( String( "IP   = " + to_hex_str( processor->direct_acc( 0xB8 ) ) + " (" +
                             to_string( processor->direct_acc( 0xB8 ) ) + ")" )
                         .c_str() );
        ImGui::Text( String( "TCON = " + to_hex_str( processor->direct_acc( 0x88 ) ) + " (" +
                             to_string( processor->direct_acc( 0x88 ) ) + ")" )
                         .c_str() );
        ImGui::Text( String( "TMOD = " + to_hex_str( processor->direct_acc( 0x89 ) ) + " (" +
                             to_string( processor->direct_acc( 0x89 ) ) + ")" )
                         .c_str() );
        ImGui::Text( String( "TH0  = " + to_hex_str( processor->direct_acc( 0x9C ) ) + " (" +
                             to_string( processor->direct_acc( 0x9C ) ) + ")" )
                         .c_str() );
        ImGui::SameLine();
        ImGui::Text( String( "TL0  = " + to_hex_str( processor->direct_acc( 0x9A ) ) + " (" +
                             to_string( processor->direct_acc( 0x9A ) ) + ")" )
                         .c_str() );
        ImGui::Text( String( "TH1  = " + to_hex_str( processor->direct_acc( 0x9D ) ) + " (" +
                             to_string( processor->direct_acc( 0x9D ) ) + ")" )
                         .c_str() );
        ImGui::SameLine();
        ImGui::Text( String( "TL1  = " + to_hex_str( processor->direct_acc( 0x9B ) ) + " (" +
                             to_string( processor->direct_acc( 0x9B ) ) + ")" )
                         .c_str() );

        ImGui::End();

        ImGui::Begin( "Text (ROM)" );
        {
            ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 0, 0 ) );
            ImGuiListClipper clipper;
            clipper.Begin( processor->text.size() / 8 );
            while ( clipper.Step() ) {
                for ( size_t i = clipper.DisplayStart; i < clipper.DisplayEnd; i++ ) {
                    String line = "Addr " + to_hex_str( i * 8 ) + ": ";
                    String ascii = "  |  ";
                    for ( size_t j = 0; j < 8; j++ ) {
                        u8 byte = processor->text[i * 8 + j];
                        line += " " + to_hex_str( byte );
                        ascii +=
                            ( to_human_readable_ascii( byte ) == '%' ? String( "%%" )
                                                                     : String( 1, to_human_readable_ascii( byte ) ) );
                    }
                    line += ascii;
                    ImGui::Text( line.c_str() );
                }
            }
            ImGui::PopStyleVar();
        }
        ImGui::End();

        ImGui::Begin( "External RAM" );
        {
            ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 0, 0 ) );
            ImGuiListClipper clipper;
            clipper.Begin( processor->xram.size() / 8 );
            while ( clipper.Step() ) {
                for ( size_t i = clipper.DisplayStart; i < clipper.DisplayEnd; i++ ) {
                    String line = "Addr " + to_hex_str( i * 8 ) + ": ";
                    String ascii = "  |  ";
                    for ( size_t j = 0; j < 8; j++ ) {
                        u8 byte = processor->xram[i * 8 + j];
                        line += " " + to_hex_str( byte );
                        ascii +=
                            ( to_human_readable_ascii( byte ) == '%' ? String( "%%" )
                                                                     : String( 1, to_human_readable_ascii( byte ) ) );
                    }
                    line += ascii;
                    ImGui::Text( line.c_str() );
                }
            }
            ImGui::PopStyleVar();
        }
        ImGui::End();

        ImGui::Begin( "Internal RAM" );
        {
            ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 0, 0 ) );
            ImGuiListClipper clipper;
            clipper.Begin( processor->iram.size() / 8 );
            while ( clipper.Step() ) {
                for ( size_t i = clipper.DisplayStart; i < clipper.DisplayEnd; i++ ) {
                    String line = "Addr " + to_hex_str( i * 8 ) + ": ";
                    String ascii = "  |  ";
                    for ( size_t j = 0; j < 8; j++ ) {
                        u8 byte = processor->iram[i * 8 + j];
                        line += " " + to_hex_str( byte );
                        ascii +=
                            ( to_human_readable_ascii( byte ) == '%' ? String( "%%" )
                                                                     : String( 1, to_human_readable_ascii( byte ) ) );
                    }
                    line += ascii;
                    ImGui::Text( line.c_str() );
                }
            }
            ImGui::PopStyleVar();
        }
        ImGui::End();

        ImGui::Begin( "Assembly" );
        {
            ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 0, 0 ) );
            ImGuiListClipper clipper;
            clipper.Begin( op_code_indices.size() );
            while ( clipper.Step() ) {
                for ( size_t i = clipper.DisplayStart; i < clipper.DisplayEnd; i++ ) {
                    u16 code_index = op_code_indices[i];
                    String line = " " + to_hex_str( code_index ) + ": " +
                                  get_decoded_instruction_string( *processor, code_index );
                    auto bp_itr =
                        std::find( processor->break_addresses.begin(), processor->break_addresses.end(), code_index );
                    ImGui::PushID( i );
                    if ( bp_itr != processor->break_addresses.end() ) {
                        if ( ImGui::Button( "O" ) ) {
                            processor->break_addresses.erase( bp_itr );
                        }
                    } else {
                        if ( ImGui::Button( " " ) ) {
                            processor->break_addresses.push_back( code_index );
                        }
                    }
                    ImGui::PopID();
                    ImGui::SameLine();
                    if ( code_index == processor->pc ) {
                        ImGui::TextColored( ImVec4( 1.0f, 1.0f, 0.0f, 1.0f ), line.c_str() );
                        if ( last_pc != processor->pc ) {
                            ImGui::SetScrollHereY();
                            last_pc = processor->pc;
                        }
                    } else {
                        ImGui::Text( line.c_str() );
                    }
                }
            }
            ImGui::PopStyleVar();
        }
        ImGui::End();

        ImGui::Begin( "Editor" );
        {
            should_load |= ImGui::InputText( "In file", &editor_asm_filename );
            ImGui::InputText( "Out directory", &editor_hex_file_dir );

            should_load |= ImGui::Button( "Load" );
            ImGui::SameLine();
            should_save |= ImGui::Button( "Save" );
            ImGui::SameLine();
            should_compile |= ImGui::Button( "Compile" );

            should_compile |= ImGui::InputTextMultiline( "##content", &editor_content, ImVec2( -FLT_MIN, -FLT_MIN ),
                                                         ImGuiInputTextFlags_EnterReturnsTrue );
            should_save |= should_compile;

            if ( should_load ) {
                std::ifstream file( editor_asm_filename );
                if ( file.good() ) {
                    String tmp;
                    editor_content.clear();
                    while ( std::getline( file, tmp ) )
                        editor_content += tmp + '\n';
                    log( "Loaded assembler file" );
                }
                should_load = false;
            }

            if ( should_save ) {
                std::ofstream file( editor_asm_filename );
                file << editor_content;
                file.close();
                should_save = false;
            }

            if ( should_compile ) {
                auto name_begin = editor_asm_filename.find_last_of( "/" ) + 1;
                if ( name_begin == editor_asm_filename.npos + 1 )
                    name_begin = 0;
                auto name_size = editor_asm_filename.find_last_of( "." ) - name_begin;
                auto filename =
                    editor_hex_file_dir + "/" + editor_asm_filename.substr( name_begin, name_size ) + ".hex";
                std::ofstream file( filename );

                compile_assembly( editor_content, file );
                file.close();

                if ( hex_filename == filename ) {
                    steps_per_frame = 0;
                    max_speed = false;
                    use_fix_target_frequency = false;
                    if ( processor->load_hex_code( hex_filename ) )
                        decode_instructions( *processor, op_code_indices );
                }
                should_compile = false;
            }
        }
        ImGui::End();

        ImGui::Begin( "Log" );
        {
            ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 0, 0 ) );
            ImGuiListClipper clipper;
            clipper.Begin( global_log.size() );
            while ( clipper.Step() ) {
                for ( size_t i = clipper.DisplayStart; i < clipper.DisplayEnd; i++ ) {
                    String line =
                        global_log[i] + ( i == global_log.size() - 1
                                              ? " (" +
                                                    to_string( static_cast<size_t>(
                                                        last_global_log_timer.getElapsedTime().asSeconds() ) ) +
                                                    "s ago)"
                                              : "" );
                    if ( last_global_log_timer.getElapsedTime().asSeconds() < 10.f && i == global_log.size() - 1 ) {
                        if ( to_lower( line ).find( "success" ) != line.npos ) {
                            ImGui::TextColored( ImVec4( 0.0f, 1.0f, 0.0f, 1.0f ), line.c_str() );
                        } else if ( to_lower( line ).find( "fail" ) != line.npos ) {
                            ImGui::TextColored( ImVec4( 1.0f, 0.0f, 0.0f, 1.0f ), line.c_str() );
                        } else {
                            ImGui::Text( line.c_str() );
                        }
                    } else {
                        ImGui::Text( line.c_str() );
                    }
                }
            }
            ImGui::PopStyleVar();
            if ( last_log_size != global_log.size() ) {
                ImGui::SetScrollHereY();
                last_log_size = global_log.size();
            }
        }
        ImGui::End();

        ImGui::Begin( "Calculator" );
        {
            ImGui::InputInt( "DEC", &calc_val, 1, 10, ImGuiInputTextFlags_CharsDecimal );
            ImGui::InputInt( "HEX", &calc_val, 1, 10, ImGuiInputTextFlags_CharsHexadecimal );
        }
        ImGui::End();

        // ImGui::ShowDemoWindow();

        ImGui::EndFrame();

        // Rendering

        window.clear( sf::Color( 0x707070ff ) );

        // Info window
        ImGui::SFML::Render( window );

        window.display();
    }

    // Cleanup
    window.close();
    ImGui::SFML::Shutdown();

    return 0;
}

void log( const String &str ) {
    std::cout << str << std::endl;
    global_log.push_back( str );
    last_global_log_timer.restart();
}
