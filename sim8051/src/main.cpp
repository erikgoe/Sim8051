#include "sim8051/stdafx.hpp"
#include "sim8051/Processor.hpp"

String to_hex_str( u16 val, u8 bytes = 8 ) {
    std::stringstream stream;
    stream << std::hex << std::setfill( '0' ) << std::setw( bytes / 4 ) << val;
    return stream.str();
}
String to_hex_str_signed( i8 val ) {
    std::stringstream stream;
    if ( val < 0 ) {
        stream << std::hex << std::setfill( '0' ) << std::setw( 2 ) << static_cast<u16>( -val );
        return "-" + stream.str();
    } else {
        stream << std::hex << std::setfill( '0' ) << std::setw( 2 ) << static_cast<u16>( val );
        return stream.str();
    }
}

void decode_instructions( const Processor &processor, std::vector<u16> &op_code_indices );
String get_decoded_instruction_string( Processor &processor, u16 code_addr );
void compile_assembly( const String &code, std::ostream &output );

// The main loop, including some stuff like scrolling
int main() {
    // Initialize all the stuff
    sf::ContextSettings context_settings( 0, 0, 4 );
    sf::RenderWindow window( sf::VideoMode( 1200, 800, 32 ), "Sim8051", sf::Style::Default, context_settings );
    bool fullscreen = false;
    window.setFramerateLimit( 60 );
    auto view = window.getDefaultView();
    window.setView( view );
    if ( !ImGui::SFML::Init( window ) ) {
        log( "Failed to initialize ImGui" );
        return -1;
    }
    sf::Clock timer;

    // Mouse bug workaround
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;

    bool running = true;
    sf::Event evt;


    // Simulation stuff
    size_t steps_per_frame = 0;
    bool pause_next_frame = false;
    bool max_speed = false;
    std::vector<u16> op_code_indices; // Pointers to the op codes.
    auto processor = std::make_shared<Processor>();
    String hex_filename = "tests/simple.hex";
    String editor_asm_filename = "tests/simple.a51";
    String editor_hex_file_dir = "tests";
    String editor_content = "";

    // Load simulation hex.
    if ( processor->load_hex_code( hex_filename ) )
        decode_instructions( *processor, op_code_indices );

    // Try to read editor file.
    {
        std::ifstream file( editor_asm_filename );
        if ( file.good() ) {
            String tmp;
            while ( std::getline( file, tmp ) )
                editor_content += tmp + '\n';
        }
    }

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
        }

        // Gui
        ImGui::Begin( "Control" );
        ImGui::Text( String( "Cycle count: " + to_string( processor->cycle_count ) +
                             ( steps_per_frame == 0 ? " (Paused)" : "" ) )
                         .c_str() );
        ImGui::Text( String( "Cycles per frame: " + to_string( steps_per_frame ) ).c_str() );
        ImGui::Text( String( "Frames per second: " + to_string( 1.f / delta_time.asSeconds() ) ).c_str() );
        if ( ImGui::Button( steps_per_frame != 0 ? "Pause" : "Run" ) ) {
            steps_per_frame = steps_per_frame == 0 ? 1 : 0;
            max_speed = false;
        }
        if ( ImGui::Button( "Single step" ) ) {
            steps_per_frame = 1;
            pause_next_frame = true;
            max_speed = false;
        }
        if ( ImGui::Button( "Max speed" ) ) {
            max_speed = true;
        }
        if ( ImGui::Button( "Reset MCU" ) ) {
            processor->reset();
        }
        ImGui::Spacing();
        if ( ImGui::InputText( "Hex file", &hex_filename ) | ImGui::Button( "Load" ) ) {
            steps_per_frame = 0;
            max_speed = false;
            if ( processor->load_hex_code( hex_filename ) )
                decode_instructions( *processor, op_code_indices );
        }

        ImGui::End();

        ImGui::Begin( "Special function registers" );
        ImGui::Text( String( "PSW  = " + to_hex_str( processor->direct_acc( 0xD0 ) ) + " (" +
                             to_string( processor->direct_acc( 0xD0 ) ) + ")" )
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
        ImGui::Text( String( "R0   = " + to_hex_str( *r0_ptr, 16 ) + " (" + to_string( *r0_ptr ) + ")" ).c_str() );
        ImGui::Text( String( "R1   = " + to_hex_str( *( r0_ptr + 1 ), 16 ) + " (" + to_string( *( r0_ptr + 1 ) ) + ")" )
                         .c_str() );
        ImGui::Text( String( "R2   = " + to_hex_str( *( r0_ptr + 2 ), 16 ) + " (" + to_string( *( r0_ptr + 2 ) ) + ")" )
                         .c_str() );
        ImGui::Text( String( "R3   = " + to_hex_str( *( r0_ptr + 3 ), 16 ) + " (" + to_string( *( r0_ptr + 3 ) ) + ")" )
                         .c_str() );
        ImGui::Text( String( "R4   = " + to_hex_str( *( r0_ptr + 4 ), 16 ) + " (" + to_string( *( r0_ptr + 4 ) ) + ")" )
                         .c_str() );
        ImGui::Text( String( "R5   = " + to_hex_str( *( r0_ptr + 5 ), 16 ) + " (" + to_string( *( r0_ptr + 5 ) ) + ")" )
                         .c_str() );
        ImGui::Text( String( "R6   = " + to_hex_str( *( r0_ptr + 6 ), 16 ) + " (" + to_string( *( r0_ptr + 6 ) ) + ")" )
                         .c_str() );
        ImGui::Text( String( "R7   = " + to_hex_str( *( r0_ptr + 7 ), 16 ) + " (" + to_string( *( r0_ptr + 7 ) ) + ")" )
                         .c_str() );
        ImGui::End();

        ImGui::Begin( "Internal RAM" );
        {
            ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 0, 0 ) );
            ImGuiListClipper clipper;
            clipper.Begin( processor->iram.size() / 8 );
            while ( clipper.Step() ) {
                for ( size_t i = clipper.DisplayStart; i < clipper.DisplayEnd; i++ ) {
                    String line = "Addr " + to_hex_str( i * 8 ) + ": ";
                    for ( size_t j = 0; j < 8; j++ )
                        line += " " + to_hex_str( processor->iram[i * 8 + j] );
                    ImGui::Text( line.c_str() );
                }
            }
            ImGui::PopStyleVar();
        }
        ImGui::End();

        ImGui::Begin( "Text (ROM)" );
        {
            ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 0, 0 ) );
            ImGuiListClipper clipper;
            clipper.Begin( processor->text.size() / 8 );
            while ( clipper.Step() ) {
                for ( size_t i = clipper.DisplayStart; i < clipper.DisplayEnd; i++ ) {
                    String line = "Addr " + to_hex_str( i * 8 ) + ": ";
                    for ( size_t j = 0; j < 8; j++ )
                        line += " " + to_hex_str( processor->text[i * 8 + j] );
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
                    for ( size_t j = 0; j < 8; j++ )
                        line += " " + to_hex_str( processor->xram[i * 8 + j] );
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
                    String line = to_hex_str( op_code_indices[i] ) + ": " +
                                  get_decoded_instruction_string( *processor, op_code_indices[i] );
                    if ( op_code_indices[i] == processor->pc ) {
                        ImGui::TextColored( ImVec4( 1.0f, 1.0f, 0.0f, 1.0f ), line.c_str() );
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
            bool should_load = ImGui::InputText( "In file", &editor_asm_filename );
            ImGui::InputText( "Out dir file", &editor_hex_file_dir );

            should_load |= ImGui::Button( "Load" );
            ImGui::SameLine();
            bool should_compile = ImGui::Button( "Compile" );

            should_compile |= ImGui::InputTextMultiline( "##content", &editor_content, ImVec2( -FLT_MIN, -FLT_MIN ),
                                                         ImGuiInputTextFlags_EnterReturnsTrue );

            if ( should_load ) {
                std::ifstream file( editor_asm_filename );
                String tmp;
                while ( std::getline( file, tmp ) )
                    editor_content += tmp + '\n';
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
                    if ( processor->load_hex_code( hex_filename ) )
                        decode_instructions( *processor, op_code_indices );
                }
            }
        }
        ImGui::End();

        ImGui::ShowDemoWindow();

        ImGui::EndFrame();

        // Rendering

        window.clear( sf::Color( 0x707070ff ) );

        // Info window
        ImGui::SFML::Render( window );

        window.display();
    }

    // Cleanup
    ImGui::SFML::Shutdown();

    return 0;
}

std::map<u8, u8> op_code_sizes = {
    { 0x00, 1 }, { 0x01, 2 }, { 0x02, 3 }, { 0x03, 1 }, { 0x04, 1 }, { 0x05, 2 }, { 0x06, 1 }, { 0x07, 1 }, { 0x08, 1 },
    { 0x09, 1 }, { 0x0A, 1 }, { 0x0B, 1 }, { 0x0C, 1 }, { 0x0D, 1 }, { 0x0E, 1 }, { 0x0F, 1 }, { 0x10, 3 }, { 0x11, 2 },
    { 0x12, 3 }, { 0x13, 1 }, { 0x14, 1 }, { 0x15, 2 }, { 0x16, 1 }, { 0x17, 1 }, { 0x18, 1 }, { 0x19, 1 }, { 0x1A, 1 },
    { 0x1B, 1 }, { 0x1C, 1 }, { 0x1D, 1 }, { 0x1E, 1 }, { 0x1F, 1 }, { 0x20, 3 }, { 0x21, 2 }, { 0x22, 1 }, { 0x23, 1 },
    { 0x24, 2 }, { 0x25, 2 }, { 0x26, 1 }, { 0x27, 1 }, { 0x28, 1 }, { 0x29, 1 }, { 0x2A, 1 }, { 0x2B, 1 }, { 0x2C, 1 },
    { 0x2D, 1 }, { 0x2E, 1 }, { 0x2F, 1 }, { 0x30, 3 }, { 0x31, 2 }, { 0x32, 1 }, { 0x33, 1 }, { 0x34, 2 }, { 0x35, 2 },
    { 0x36, 1 }, { 0x37, 1 }, { 0x38, 1 }, { 0x39, 1 }, { 0x3A, 1 }, { 0x3B, 1 }, { 0x3C, 1 }, { 0x3D, 1 }, { 0x3E, 1 },
    { 0x3F, 1 }, { 0x40, 2 }, { 0x41, 2 }, { 0x42, 2 }, { 0x43, 3 }, { 0x44, 2 }, { 0x45, 2 }, { 0x46, 1 }, { 0x47, 1 },
    { 0x48, 1 }, { 0x49, 1 }, { 0x4A, 1 }, { 0x4B, 1 }, { 0x4C, 1 }, { 0x4D, 1 }, { 0x4E, 1 }, { 0x4F, 1 }, { 0x50, 2 },
    { 0x51, 2 }, { 0x52, 2 }, { 0x53, 3 }, { 0x54, 2 }, { 0x55, 2 }, { 0x56, 1 }, { 0x57, 1 }, { 0x58, 1 }, { 0x59, 1 },
    { 0x5A, 1 }, { 0x5B, 1 }, { 0x5C, 1 }, { 0x5D, 1 }, { 0x5E, 1 }, { 0x5F, 1 }, { 0x60, 2 }, { 0x61, 2 }, { 0x62, 2 },
    { 0x63, 3 }, { 0x64, 2 }, { 0x65, 2 }, { 0x66, 1 }, { 0x67, 1 }, { 0x68, 1 }, { 0x69, 1 }, { 0x6A, 1 }, { 0x6B, 1 },
    { 0x6C, 1 }, { 0x6D, 1 }, { 0x6E, 1 }, { 0x6F, 1 }, { 0x70, 2 }, { 0x71, 2 }, { 0x72, 2 }, { 0x73, 1 }, { 0x74, 2 },
    { 0x75, 3 }, { 0x76, 2 }, { 0x77, 2 }, { 0x78, 2 }, { 0x79, 2 }, { 0x7A, 2 }, { 0x7B, 2 }, { 0x7C, 2 }, { 0x7D, 2 },
    { 0x7E, 2 }, { 0x7F, 2 }, { 0x80, 2 }, { 0x81, 2 }, { 0x82, 2 }, { 0x83, 1 }, { 0x84, 1 }, { 0x85, 3 }, { 0x86, 2 },
    { 0x87, 2 }, { 0x88, 2 }, { 0x89, 2 }, { 0x8A, 2 }, { 0x8B, 2 }, { 0x8C, 2 }, { 0x8D, 2 }, { 0x8E, 2 }, { 0x8F, 2 },
    { 0x90, 3 }, { 0x91, 2 }, { 0x92, 2 }, { 0x93, 1 }, { 0x94, 2 }, { 0x95, 2 }, { 0x96, 1 }, { 0x97, 1 }, { 0x98, 1 },
    { 0x99, 1 }, { 0x9A, 1 }, { 0x9B, 1 }, { 0x9C, 1 }, { 0x9D, 1 }, { 0x9E, 1 }, { 0x9F, 1 }, { 0xA0, 2 }, { 0xA1, 2 },
    { 0xA2, 2 }, { 0xA3, 1 }, { 0xA4, 1 }, { 0xA5, 1 }, { 0xA6, 2 }, { 0xA7, 2 }, { 0xA8, 2 }, { 0xA9, 2 }, { 0xAA, 2 },
    { 0xAB, 2 }, { 0xAC, 2 }, { 0xAD, 2 }, { 0xAE, 2 }, { 0xAF, 2 }, { 0xB0, 2 }, { 0xB1, 2 }, { 0xB2, 2 }, { 0xB3, 1 },
    { 0xB4, 3 }, { 0xB5, 3 }, { 0xB6, 3 }, { 0xB7, 3 }, { 0xB8, 3 }, { 0xB9, 3 }, { 0xBA, 3 }, { 0xBB, 3 }, { 0xBC, 3 },
    { 0xBD, 3 }, { 0xBE, 3 }, { 0xBF, 3 }, { 0xC0, 2 }, { 0xC1, 2 }, { 0xC2, 2 }, { 0xC3, 1 }, { 0xC4, 1 }, { 0xC5, 2 },
    { 0xC6, 1 }, { 0xC7, 1 }, { 0xC8, 1 }, { 0xC9, 1 }, { 0xCA, 1 }, { 0xCB, 1 }, { 0xCC, 1 }, { 0xCD, 1 }, { 0xCE, 1 },
    { 0xCF, 1 }, { 0xD0, 2 }, { 0xD1, 2 }, { 0xD2, 2 }, { 0xD3, 1 }, { 0xD4, 1 }, { 0xD5, 3 }, { 0xD6, 1 }, { 0xD7, 1 },
    { 0xD8, 2 }, { 0xD9, 2 }, { 0xDA, 2 }, { 0xDB, 2 }, { 0xDC, 2 }, { 0xDD, 2 }, { 0xDE, 2 }, { 0xDF, 2 }, { 0xE0, 1 },
    { 0xE1, 2 }, { 0xE2, 1 }, { 0xE3, 1 }, { 0xE4, 1 }, { 0xE5, 2 }, { 0xE6, 1 }, { 0xE7, 1 }, { 0xE8, 1 }, { 0xE9, 1 },
    { 0xEA, 1 }, { 0xEB, 1 }, { 0xEC, 1 }, { 0xED, 1 }, { 0xEE, 1 }, { 0xEF, 1 }, { 0xF0, 1 }, { 0xF1, 2 }, { 0xF2, 1 },
    { 0xF3, 1 }, { 0xF4, 1 }, { 0xF5, 2 }, { 0xF6, 1 }, { 0xF7, 1 }, { 0xF8, 1 }, { 0xF9, 1 }, { 0xFA, 1 }, { 0xFB, 1 },
    { 0xFC, 1 }, { 0xFD, 1 }, { 0xFE, 1 }, { 0xFF, 1 },
};
std::map<u8, std::vector<String>> op_code_signatures = {
    { 0x00, { "NOP " } },
    { 0x01, { "AJMP", "addr11" } },
    { 0x02, { "LJMP", "addr16" } },
    { 0x03, { "RR", "A" } },
    { 0x04, { "INC", "A" } },
    { 0x05, { "INC", "direct" } },
    { 0x06, { "INC", "@R0" } },
    { 0x07, { "INC", "@R1" } },
    { 0x08, { "INC", "R0" } },
    { 0x09, { "INC", "R1" } },
    { 0x0A, { "INC", "R2" } },
    { 0x0B, { "INC", "R3" } },
    { 0x0C, { "INC", "R4" } },
    { 0x0D, { "INC", "R5" } },
    { 0x0E, { "INC", "R6" } },
    { 0x0F, { "INC", "R7" } },
    { 0x10, { "JBC", "bit", "offset" } },
    { 0x11, { "ACALL", "addr11" } },
    { 0x12, { "LCALL", "addr16" } },
    { 0x13, { "RRC", "A" } },
    { 0x14, { "DEC", "A" } },
    { 0x15, { "DEC", "direct" } },
    { 0x16, { "DEC", "@R0" } },
    { 0x17, { "DEC", "@R1" } },
    { 0x18, { "DEC", "R0" } },
    { 0x19, { "DEC", "R1" } },
    { 0x1A, { "DEC", "R2" } },
    { 0x1B, { "DEC", "R3" } },
    { 0x1C, { "DEC", "R4" } },
    { 0x1D, { "DEC", "R5" } },
    { 0x1E, { "DEC", "R6" } },
    { 0x1F, { "DEC", "R7" } },
    { 0x20, { "JB", "bit", "offset" } },
    { 0x21, { "AJMP", "addr11" } },
    { 0x22, { "RET" } },
    { 0x23, { "RL", "A" } },
    { 0x24, { "ADD", "A", "#immed" } },
    { 0x25, { "ADD", "A", "direct" } },
    { 0x26, { "ADD", "A", "@R0" } },
    { 0x27, { "ADD", "A", "@R1" } },
    { 0x28, { "ADD", "A", "R0" } },
    { 0x29, { "ADD", "A", "R1" } },
    { 0x2A, { "ADD", "A", "R2" } },
    { 0x2B, { "ADD", "A", "R3" } },
    { 0x2C, { "ADD", "A", "R4" } },
    { 0x2D, { "ADD", "A", "R5" } },
    { 0x2E, { "ADD", "A", "R6" } },
    { 0x2F, { "ADD", "A", "R7" } },
    { 0x30, { "JNB", "bit", "offset" } },
    { 0x31, { "ACALL", "addr11" } },
    { 0x32, { "RETI" } },
    { 0x33, { "RLC", "A" } },
    { 0x34, { "ADDC", "A", "#immed" } },
    { 0x35, { "ADDC", "A", "direct" } },
    { 0x36, { "ADDC", "A", "@R0" } },
    { 0x37, { "ADDC", "A", "@R1" } },
    { 0x38, { "ADDC", "A", "R0" } },
    { 0x39, { "ADDC", "A", "R1" } },
    { 0x3A, { "ADDC", "A", "R2" } },
    { 0x3B, { "ADDC", "A", "R3" } },
    { 0x3C, { "ADDC", "A", "R4" } },
    { 0x3D, { "ADDC", "A", "R5" } },
    { 0x3E, { "ADDC", "A", "R6" } },
    { 0x3F, { "ADDC", "A", "R7" } },
    { 0x40, { "JC", "offset" } },
    { 0x41, { "AJMP", "addr11" } },
    { 0x42, { "ORL", "direct", "A" } },
    { 0x43, { "ORL", "direct", "#immed" } },
    { 0x44, { "ORL", "A", "#immed" } },
    { 0x45, { "ORL", "A", "direct" } },
    { 0x46, { "ORL", "A", "@R0" } },
    { 0x47, { "ORL", "A", "@R1" } },
    { 0x48, { "ORL", "A", "R0" } },
    { 0x49, { "ORL", "A", "R1" } },
    { 0x4A, { "ORL", "A", "R2" } },
    { 0x4B, { "ORL", "A", "R3" } },
    { 0x4C, { "ORL", "A", "R4" } },
    { 0x4D, { "ORL", "A", "R5" } },
    { 0x4E, { "ORL", "A", "R6" } },
    { 0x4F, { "ORL", "A", "R7" } },
    { 0x50, { "JNC", "offset" } },
    { 0x51, { "ACALL", "addr11" } },
    { 0x52, { "ANL", "direct", "A" } },
    { 0x53, { "ANL", "direct", "#immed" } },
    { 0x54, { "ANL", "A", "#immed" } },
    { 0x55, { "ANL", "A", "direct" } },
    { 0x56, { "ANL", "A", "@R0" } },
    { 0x57, { "ANL", "A", "@R1" } },
    { 0x58, { "ANL", "A", "R0" } },
    { 0x59, { "ANL", "A", "R1" } },
    { 0x5A, { "ANL", "A", "R2" } },
    { 0x5B, { "ANL", "A", "R3" } },
    { 0x5C, { "ANL", "A", "R4" } },
    { 0x5D, { "ANL", "A", "R5" } },
    { 0x5E, { "ANL", "A", "R6" } },
    { 0x5F, { "ANL", "A", "R7" } },
    { 0x60, { "JZ", "offset" } },
    { 0x61, { "AJMP", "addr11" } },
    { 0x62, { "XRL", "direct", "A" } },
    { 0x63, { "XRL", "direct", "#immed" } },
    { 0x64, { "XRL", "A", "#immed" } },
    { 0x65, { "XRL", "A", "direct" } },
    { 0x66, { "XRL", "A", "@R0" } },
    { 0x67, { "XRL", "A", "@R1" } },
    { 0x68, { "XRL", "A", "R0" } },
    { 0x69, { "XRL", "A", "R1" } },
    { 0x6A, { "XRL", "A", "R2" } },
    { 0x6B, { "XRL", "A", "R3" } },
    { 0x6C, { "XRL", "A", "R4" } },
    { 0x6D, { "XRL", "A", "R5" } },
    { 0x6E, { "XRL", "A", "R6" } },
    { 0x6F, { "XRL", "A", "R7" } },
    { 0x70, { "JNZ", "offset" } },
    { 0x71, { "ACALL", "addr11" } },
    { 0x72, { "ORL", "C", "bit" } },
    { 0x73, { "JMP", "@A+DPTR" } },
    { 0x74, { "MOV", "A", "#immed" } },
    { 0x75, { "MOV", "direct", "#immed" } },
    { 0x76, { "MOV", "@R0", "#immed" } },
    { 0x77, { "MOV", "@R1", "#immed" } },
    { 0x78, { "MOV", "R0", "#immed" } },
    { 0x79, { "MOV", "R1", "#immed" } },
    { 0x7A, { "MOV", "R2", "#immed" } },
    { 0x7B, { "MOV", "R3", "#immed" } },
    { 0x7C, { "MOV", "R4", "#immed" } },
    { 0x7D, { "MOV", "R5", "#immed" } },
    { 0x7E, { "MOV", "R6", "#immed" } },
    { 0x7F, { "MOV", "R7", "#immed" } },
    { 0x80, { "SJMP", "offset" } },
    { 0x81, { "AJMP", "addr11" } },
    { 0x82, { "ANL", "C", "bit" } },
    { 0x83, { "MOVC", "A", "@A+PC" } },
    { 0x84, { "DIV", "A", "B" } },
    { 0x85, { "MOV", "direct", "direct" } },
    { 0x86, { "MOV", "direct", "@R0" } },
    { 0x87, { "MOV", "direct", "@R1" } },
    { 0x88, { "MOV", "direct", "R0" } },
    { 0x89, { "MOV", "direct", "R1" } },
    { 0x8A, { "MOV", "direct", "R2" } },
    { 0x8B, { "MOV", "direct", "R3" } },
    { 0x8C, { "MOV", "direct", "R4" } },
    { 0x8D, { "MOV", "direct", "R5" } },
    { 0x8E, { "MOV", "direct", "R6" } },
    { 0x8F, { "MOV", "direct", "R7" } },
    { 0x90, { "MOV", "DPTR", "#immed" } },
    { 0x91, { "ACALL", "addr11" } },
    { 0x92, { "MOV", "bit", "C" } },
    { 0x93, { "MOVC", "A", "@A+DPTR" } },
    { 0x94, { "SUBB", "A", "#immed" } },
    { 0x95, { "SUBB", "A", "direct" } },
    { 0x96, { "SUBB", "A", "@R0" } },
    { 0x97, { "SUBB", "A", "@R1" } },
    { 0x98, { "SUBB", "A", "R0" } },
    { 0x99, { "SUBB", "A", "R1" } },
    { 0x9A, { "SUBB", "A", "R2" } },
    { 0x9B, { "SUBB", "A", "R3" } },
    { 0x9C, { "SUBB", "A", "R4" } },
    { 0x9D, { "SUBB", "A", "R5" } },
    { 0x9E, { "SUBB", "A", "R6" } },
    { 0x9F, { "SUBB", "A", "R7" } },
    { 0xA0, { "ORL", "C", "/bit" } },
    { 0xA1, { "AJMP", "addr11" } },
    { 0xA2, { "MOV", "C", "bit" } },
    { 0xA3, { "INC", "DPTR" } },
    { 0xA4, { "MUL", "A", "B" } },
    { 0xA5, { "reserved" } },
    { 0xA6, { "MOV", "@R0", "direct" } },
    { 0xA7, { "MOV", "@R1", "direct" } },
    { 0xA8, { "MOV", "R0", "direct" } },
    { 0xA9, { "MOV", "R1", "direct" } },
    { 0xAA, { "MOV", "R2", "direct" } },
    { 0xAB, { "MOV", "R3", "direct" } },
    { 0xAC, { "MOV", "R4", "direct" } },
    { 0xAD, { "MOV", "R5", "direct" } },
    { 0xAE, { "MOV", "R6", "direct" } },
    { 0xAF, { "MOV", "R7", "direct" } },
    { 0xB0, { "ANL", "C", "/bit" } },
    { 0xB1, { "ACALL", "addr11" } },
    { 0xB2, { "CPL", "bit" } },
    { 0xB3, { "CPL", "C" } },
    { 0xB4, { "CJNE", "A", "#immed", "offset" } },
    { 0xB5, { "CJNE", "A", "direct", "offset" } },
    { 0xB6, { "CJNE", "@R0", "#immed", "offset" } },
    { 0xB7, { "CJNE", "@R1", "#immed", "offset" } },
    { 0xB8, { "CJNE", "R0", "#immed", "offset" } },
    { 0xB9, { "CJNE", "R1", "#immed", "offset" } },
    { 0xBA, { "CJNE", "R2", "#immed", "offset" } },
    { 0xBB, { "CJNE", "R3", "#immed", "offset" } },
    { 0xBC, { "CJNE", "R4", "#immed", "offset" } },
    { 0xBD, { "CJNE", "R5", "#immed", "offset" } },
    { 0xBE, { "CJNE", "R6", "#immed", "offset" } },
    { 0xBF, { "CJNE", "R7", "#immed", "offset" } },
    { 0xC0, { "PUSH", "direct" } },
    { 0xC1, { "AJMP", "addr11" } },
    { 0xC2, { "CLR", "bit" } },
    { 0xC3, { "CLR", "C" } },
    { 0xC4, { "SWAP", "A" } },
    { 0xC5, { "XCH", "A", "direct" } },
    { 0xC6, { "XCH", "A", "@R0" } },
    { 0xC7, { "XCH", "A", "@R1" } },
    { 0xC8, { "XCH", "A", "R0" } },
    { 0xC9, { "XCH", "A", "R1" } },
    { 0xCA, { "XCH", "A", "R2" } },
    { 0xCB, { "XCH", "A", "R3" } },
    { 0xCC, { "XCH", "A", "R4" } },
    { 0xCD, { "XCH", "A", "R5" } },
    { 0xCE, { "XCH", "A", "R6" } },
    { 0xCF, { "XCH", "A", "R7" } },
    { 0xD0, { "POP", "direct" } },
    { 0xD1, { "ACALL", "addr11" } },
    { 0xD2, { "SETB", "bit" } },
    { 0xD3, { "SETB", "C" } },
    { 0xD4, { "DA", "A" } },
    { 0xD5, { "DJNZ", "direct", "offset" } },
    { 0xD6, { "XCHD", "A", "@R0" } },
    { 0xD7, { "XCHD", "A", "@R1" } },
    { 0xD8, { "DJNZ", "R0", "offset" } },
    { 0xD9, { "DJNZ", "R1", "offset" } },
    { 0xDA, { "DJNZ", "R2", "offset" } },
    { 0xDB, { "DJNZ", "R3", "offset" } },
    { 0xDC, { "DJNZ", "R4", "offset" } },
    { 0xDD, { "DJNZ", "R5", "offset" } },
    { 0xDE, { "DJNZ", "R6", "offset" } },
    { 0xDF, { "DJNZ", "R7", "offset" } },
    { 0xE0, { "MOVX", "A", "@DPTR" } },
    { 0xE1, { "AJMP", "addr11" } },
    { 0xE2, { "MOVX", "A", "@R0" } },
    { 0xE3, { "MOVX", "A", "@R1" } },
    { 0xE4, { "CLR", "A" } },
    { 0xE5, { "MOV", "A", "direct" } },
    { 0xE6, { "MOV", "A", "@R0" } },
    { 0xE7, { "MOV", "A", "@R1" } },
    { 0xE8, { "MOV", "A", "R0" } },
    { 0xE9, { "MOV", "A", "R1" } },
    { 0xEA, { "MOV", "A", "R2" } },
    { 0xEB, { "MOV", "A", "R3" } },
    { 0xEC, { "MOV", "A", "R4" } },
    { 0xED, { "MOV", "A", "R5" } },
    { 0xEE, { "MOV", "A", "R6" } },
    { 0xEF, { "MOV", "A", "R7" } },
    { 0xF0, { "MOVX", "@DPTR", "A" } },
    { 0xF1, { "ACALL", "addr11" } },
    { 0xF2, { "MOVX", "@R0", "A" } },
    { 0xF3, { "MOVX", "@R1", "A" } },
    { 0xF4, { "CPL", "A" } },
    { 0xF5, { "MOV", "direct", "A" } },
    { 0xF6, { "MOV", "@R0", "A" } },
    { 0xF7, { "MOV", "@R1", "A" } },
    { 0xF8, { "MOV", "R0", "A" } },
    { 0xF9, { "MOV", "R1", "A" } },
    { 0xFA, { "MOV", "R2", "A" } },
    { 0xFB, { "MOV", "R3", "A" } },
    { 0xFC, { "MOV", "R4", "A" } },
    { 0xFD, { "MOV", "R5", "A" } },
    { 0xFE, { "MOV", "R6", "A" } },
    { 0xFF, { "MOV", "R7", "A" } },

};

void decode_instructions( const Processor &processor, std::vector<u16> &op_code_indices ) {
    size_t idx = 0;
    while ( idx < processor.text.size() ) {
        op_code_indices.push_back( idx );
        idx += op_code_sizes[processor.text[idx]];
    }
}

String sfr_name( u8 addr ) {
    if ( addr == 0xE0 ) {
        return "A";
    } else if ( addr == 0xF0 ) {
        return "B";
    } else if ( addr == 0xD0 ) {
        return "PSW";
    } else if ( addr == 0xB8 ) {
        return "IP";
    } else if ( addr == 0xA8 ) {
        return "IE";
    } else if ( addr == 0x82 ) {
        return "DPL";
    } else if ( addr == 0x83 ) {
        return "DPH";
    } else if ( addr == 0x80 ) {
        return "P0";
    } else if ( addr == 0x90 ) {
        return "P1";
    } else if ( addr == 0xA0 ) {
        return "P2";
    } else if ( addr == 0xB0 ) {
        return "P3";
    } else if ( addr == 0x87 ) {
        return "PCON";
    } else if ( addr == 0x98 ) {
        return "SCON";
    } else if ( addr == 0x99 ) {
        return "SBUF";
    } else if ( addr == 0x88 ) {
        return "TCON";
    } else if ( addr == 0xC8 ) {
        return "T2CON";
    } else if ( addr == 0x89 ) {
        return "TMOD";
    } else if ( addr == 0x9A ) {
        return "TL0";
    } else if ( addr == 0x9B ) {
        return "TL1";
    } else if ( addr == 0xCC ) {
        return "TL2";
    } else if ( addr == 0x9C ) {
        return "TH0";
    } else if ( addr == 0x9D ) {
        return "TH1";
    } else if ( addr == 0xCD ) {
        return "TH2";
    } else if ( addr == 0x81 ) {
        return "SP";
    } else {
        return "";
    }
}

String get_decoded_instruction_string( Processor &processor, u16 code_addr ) {
    u8 code = processor.text[code_addr];
    u8 size = op_code_sizes[code];
    auto &signature = op_code_signatures[code];
    String ret;
    if ( size == 1 ) {
        ret += to_hex_str( code ) + "        ";
    } else if ( size == 2 ) {
        ret += to_hex_str( code ) + " " + to_hex_str( processor.text[code_addr + 1] ) + "     ";
    } else if ( size == 3 ) {
        ret += to_hex_str( code ) + " " + to_hex_str( processor.text[code_addr + 1] ) + " " +
               to_hex_str( processor.text[code_addr + 2] ) + "  ";
    }

    auto bank_nr = ( processor.iram[0xD0] & 0x18 ) >> 3;
    auto *r0_ptr = &processor.iram[8 * bank_nr];

    ret += signature.front() + " ";
    u8 operand_offset = 1;
    for ( size_t i = 1; i < signature.size(); i++ ) {
        // TODO reverse operand order of 0x85 move instruction
        bool two_byte_operand = size > signature.size() || ( size == 3 && i == 2 && operand_offset == 1 );
        auto &operand = signature[i];
        if ( i == 2 )
            ret += ", ";
        ret += operand;

        if ( operand == "A" ) {
            ret += " (" + to_hex_str( processor.direct_acc( 0xE0 ) ) + ")";
        } else if ( operand.size() == 2 && operand[0] == 'R' ) {
            ret += " (" + to_hex_str( *( r0_ptr + stoi( operand.substr( 1 ) ) ) ) + ")";
        } else if ( operand == "@R0" ) {
            if ( signature.front() == "MOVX" ) {
                ret += " (" +
                       to_hex_str( processor.xram[( static_cast<u16>( processor.iram[0xA0] ) << 8 ) | *r0_ptr] ) + ")";
            } else {
                ret += " (" + to_hex_str( processor.iram[*r0_ptr] ) + ")";
            }
        } else if ( operand == "@R1" ) {
            if ( signature.front() == "MOVX" ) {
                ret +=
                    " (" +
                    to_hex_str( processor.xram[( static_cast<u16>( processor.iram[0xA0] ) << 8 ) | *( r0_ptr + 1 )] ) +
                    ")";
            } else {
                ret += " (" + to_hex_str( processor.iram[*( r0_ptr + 1 )] ) + ")";
            }
        } else if ( operand == "#immed" || operand == "addr16" ) {
            if ( two_byte_operand ) {
                ret += " (" +
                       to_hex_str( ( ( static_cast<u16>( processor.text[code_addr + 1] ) << 8 ) ) |
                                       processor.text[code_addr + 2],
                                   16 ) +
                       ")";
                operand_offset++;
            } else {
                ret += " (" + to_hex_str( processor.text[code_addr + operand_offset] ) + ")";
            }
            operand_offset++;
        } else if ( operand == "direct" ) {
            auto addr = processor.text[code_addr + operand_offset];
            String special = sfr_name( addr );
            ret += " (&" + ( special != "" ? special : to_hex_str( addr ) ) + "; " +
                   to_hex_str( processor.direct_acc( addr ) ) + ")";
            operand_offset++;
        } else if ( operand == "addr11" ) {
            u16 addr = ( processor.pc & 0b1111100000000000 ) + ( static_cast<u16>( code & 0b11100000 ) << 3 ) +
                       processor.text[code_addr + 1];
            ret += " (" + to_hex_str( addr, 16 ) + ")";
            operand_offset++;
        } else if ( operand == "offset" ) {
            ret += " (" + to_hex_str_signed( *reinterpret_cast<i8 *>( &processor.text[code_addr + operand_offset] ) ) +
                   ")";
            operand_offset++;
        } else if ( operand == "bit" || operand == "/bit" ) {
            u8 bit_addr = processor.text[code_addr + operand_offset];
            if ( bit_addr < 0x80 ) {
                ret += " (IRAM " + to_hex_str( ( bit_addr & 0b11111000 ) >> 3 ) + "." +
                       to_hex_str( ( bit_addr & 0b111 ) ) + "; " + ( processor.is_bit_set( bit_addr ) ? "1" : "0" ) +
                       ")";
            } else {
                String special = sfr_name( ( bit_addr & 0b11111000 ) >> 3 );
                ret += " (" + ( special == "" ? to_hex_str( ( bit_addr & 0b11111000 ) >> 3 ) : special ) + "." +
                       to_hex_str( ( bit_addr & 0b111 ) ) + "; " + ( processor.is_bit_set( bit_addr ) ? "1" : "0" ) +
                       ")";
            }
            operand_offset++;
        } else if ( operand == "C" ) {
            ret += " (" + String( processor.is_bit_set( 0xD7 ) ? "1" : "0" ) + ")";
        } else if ( operand == "DPTR" ) {
            ret += " (" +
                   to_hex_str( ( static_cast<u16>( processor.direct_acc( 0x83 ) ) << 8 ) | processor.direct_acc( 0x82 ),
                               16 ) +
                   ")";
        } else if ( operand == "@DPTR" ) {
            // Always MOVX
            ret += " (" +
                   to_hex_str( processor.xram[( static_cast<u16>( processor.direct_acc( 0x83 ) ) << 8 ) |
                                              processor.direct_acc( 0x82 )] ) +
                   ")";
        } else if ( operand == "@A+DPTR" ) {
            ret += " (" +
                   to_hex_str( processor.text[( ( static_cast<u16>( processor.direct_acc( 0x83 ) ) << 8 ) |
                                                processor.direct_acc( 0x82 ) ) +
                                              processor.direct_acc( 0xE0 )] ) +
                   ")";
        } else if ( operand == "@A+PC" ) {
            ret += " (" + to_hex_str( processor.text[processor.pc + processor.direct_acc( 0xE0 )] ) + ")";
        } else if ( operand == "B" ) {
            ret += " (" + to_hex_str( processor.direct_acc( 0xF0 ) ) + ")";
        }
    }

    return ret;
}

void substitute_command( String &str, size_t arg1_idx, size_t &arg2_idx, std::vector<String> &rev_sfr_names ) {
    String arg1 = str.substr( arg1_idx, arg2_idx - arg1_idx - 1 );
    String arg2 = str.substr( arg2_idx );

    // First replace all numbers.
    bool found_substitution = false;
    if ( std::find( rev_sfr_names.begin(), rev_sfr_names.end(), arg1 ) == rev_sfr_names.end() && arg1 != "addr" &&
         arg1 != "imm" ) {
        // The first argument can't be an immediate value.
        if ( arg1[0] == '/' ) {
            str.replace( arg1_idx, arg1.size(), "/addr" );
            arg2_idx -= arg1.size() - 5; // fix index
        } else {
            str.replace( arg1_idx, arg1.size(), "addr" );
            arg2_idx -= arg1.size() - 4; // fix index
        }
        found_substitution = true;
    }
    if ( std::find( rev_sfr_names.begin(), rev_sfr_names.end(), arg2 ) == rev_sfr_names.end() && arg2 != "addr" &&
         arg2 != "imm" ) {
        if ( arg2[0] == '(' || arg2.find_first_not_of( "0123456789abcdef" ) != arg2.npos ) {
            // Indirect or label
            if ( arg2[0] == '/' ) {
                str.replace( arg2_idx, arg2.size(), "/addr" );
            } else {
                str.replace( arg2_idx, arg2.size(), "addr" );
            }
        } else {
            str.replace( arg2_idx, arg2.size(), "imm" );
        }
        found_substitution = true;
    }

    // Now replace the second parameter first.
    if ( !found_substitution && arg2 != "addr" && arg2 != "imm" ) {
        str.replace( arg2_idx, arg2.size(), "addr" );
        found_substitution = true;
    }

    // Finally try to replace the first parameter.
    if ( !found_substitution && arg1 != "addr" && arg1 != "imm" ) {
        str.replace( arg1_idx, arg1.size(), "addr" );
    }
}

void encode_hex_file( const std::vector<u8> &code, std::ostream &output ) {
    size_t buf_idx = 0;
    output << std::hex << std::setfill( '0' );
    while ( buf_idx < code.size() ) {
        u8 checksum = std::min<size_t>( 16, code.size() - buf_idx ) + ( buf_idx & 0xff ) + ( buf_idx >> 8 );
        output << std::setw( 1 ) << ':' << std::setw( 2 ) << std::min<size_t>( 16, code.size() - buf_idx );
        output << std::setw( 4 ) << buf_idx << "00" << std::setw( 2 );
        for ( size_t i = 0; i < 16 && buf_idx < code.size(); i++ ) {
            output << std::setw( 2 ) << static_cast<u16>( code[buf_idx] );
            checksum += code[buf_idx];
            buf_idx++;
        }
        output << std::setw( 2 ) << ( ( ~checksum + 1 ) & 0xff ) << '\n';
    }
    output << ":00000001ff\n";
}

String to_lower( const String &str ) {
    String tmp;
    tmp.reserve( str.size() );
    for ( auto c : str )
        tmp.push_back( tolower( c ) );
    return tmp;
}

void compile_assembly( const String &code, std::ostream &output ) {
    // Accepted signature:
    // Commas between parameters are optional.
    // All numbers are interpreted in base 16.
    // Addresses are written in parenthesis or as SFRs.
    // The first parameter is always an address.
    // Some registers are used as indirection address when written in parenthesis.
    //


    // Prepare maps
    std::map<String, u8> rev_op_codes;
    std::vector<String> rev_sfr_names = { "a",     "b",    "psw",  "ip",   "ie",     "dpl",      "dph",    "dptr",
                                          "p0",    "p1",   "p2",   "p3",   "pcon",   "scon",     "sbuf",   "tcon",
                                          "t2con", "tmod", "tl0",  "tl1",  "tl2",    "th0",      "th1",    "th2",
                                          "r0",    "r1",   "r2",   "r3",   "r4",     "r5",       "r6",     "r7",
                                          "sp",    "(pc)", "(r0)", "(r1)", "(a+pc)", "(a+dptr)", "(dptr)", "c",
                                          "p",     "ov",   "ac",   "f0",   "rs1",    "rs0",      "ud" };
    std::map<String, u8> rev_sfr_map = { { "a", 0xE0 },    { "b", 0xF0 },    { "psw", 0xD0 },  { "ip", 0xB8 },
                                         { "ie", 0xA8 },   { "dpl", 0x82 },  { "dph", 0x83 },  { "p0", 0x80 },
                                         { "p1", 0x90 },   { "p2", 0xA0 },   { "p3", 0xB0 },   { "pcon", 0x87 },
                                         { "scon", 0x98 }, { "sbuf", 0x99 }, { "tcon", 0x88 }, { "t2con", 0xC8 },
                                         { "tmod", 0x89 }, { "tl0", 0x9A },  { "tl1", 0x9B },  { "tl2", 0xCC },
                                         { "th0", 0x9C },  { "th1", 0x9D },  { "th2", 0xCD },  { "sp", 0x81 },
                                         { "c", 0xD7 },    { "p", 0xD0 },    { "ov", 0xD2 },   { "ac", 0xD6 },
                                         { "f0", 0xD5 },   { "rs1", 0xD4 },  { "rs0", 0xD3 },  { "ud", 0xD1 } };
    auto unify_name = []( const String &str ) {
        if ( str == "addr16" || str == "addr11" || str == "direct" || str == "offset" || str == "bit" ) {
            return String( "addr" );
        } else if ( str == "/bit" ) {
            return String( "/addr" );
        } else if ( str == "#immed" ) {
            return String( "imm" );
        } else if ( str[0] == '@' ) {
            return '(' + str.substr( 1 ) + ')';
        } else {
            return str;
        }
    };
    for ( auto &pair : op_code_signatures ) {
        String key = pair.second[0];
        if ( pair.second.size() > 1 )
            key += " " + unify_name( pair.second[1] );
        if ( pair.second.size() > 2 )
            key += " " + unify_name( pair.second[2] );
        rev_op_codes[to_lower( key )] = pair.first;
    }

    // Parse file.
    // We iterate all instructions and match their structure against known commands. Using subsitutions we can catch
    // pseudo-commands like "mov B 01", which is actually "mov f0 01".
    size_t index = 0;
    size_t line_no = 0;
    std::map<String, std::vector<size_t>> label_slots16; // Slots that must be filled in afterwards (absolute, 16 bits).
    std::map<String, std::vector<size_t>>
        label_slots11; // Slots that must be filled in afterwards (absolute, 11 bits including the instruction).
    std::map<String, std::vector<size_t>> label_slots8; // Slots that must be filled in afterwards (relative, 8 bits).
    std::map<String, u16> label_targets; // The collected label positions.
    std::vector<u8> hex; // The final machine code.
    while ( index < code.size() ) {
        String line = code.substr( index, code.find( "\n", index ) - index + 1 );
        index += line.size();
        line_no++;

        // Sanitize the command.
        if ( !line.empty() && line.back() == '\n' )
            line.pop_back();
        line = line.substr( 0, line.find( ";" ) );
        if ( line.find( "," ) != line.npos )
            line.replace( line.find( "," ), 1, " " );
        if ( line.find( "\t" ) != line.npos )
            line.replace( line.find( "\t" ), 1, " " );
        while ( line.find( "  " ) != line.npos )
            line.replace( line.find( "  " ), 2, " " );
        while ( !line.empty() && line.find_last_of( " " ) == line.size() - 1 )
            line.pop_back();
        while ( line.find_first_of( " " ) == 0 )
            line = line.substr( 1 );
        line = to_lower( line );

        if ( line.empty() )
            continue;

        if ( line.find( ":" ) != line.npos ) {
            // Label

            if ( line.find( ":" ) != line.size() - 1 )
                log( "Invalid label syntax." );
            line.pop_back();

            label_targets[line] = hex.size();
        } else {
            // Normal command

            size_t arg1_idx = line.find( " " ) + 1;
            size_t arg2_idx = line.find( " ", arg1_idx ) + 1;

            String real_arg1 = line.substr( arg1_idx, arg2_idx - arg1_idx - 1 );
            String real_arg2 = line.substr( arg2_idx );

            // Apply substitutions if necessary.
            if ( rev_op_codes.find( line ) == rev_op_codes.end() ) {
                substitute_command( line, arg1_idx, arg2_idx, rev_sfr_names );
                if ( rev_op_codes.find( line ) == rev_op_codes.end() ) {
                    substitute_command( line, arg1_idx, arg2_idx, rev_sfr_names );
                    if ( rev_op_codes.find( line ) == rev_op_codes.end() ) {
                        substitute_command( line, arg1_idx, arg2_idx, rev_sfr_names );
                        if ( rev_op_codes.find( line ) == rev_op_codes.end() ) {
                            log( "Unknown command/syntax at line " + to_string( line_no ) + "." );
                        }
                    }
                }
            }

            // Translate into machine code.
            auto signature = op_code_signatures[rev_op_codes[line]];
            hex.push_back( rev_op_codes[line] );
            if ( signature.size() > 1 ) {
                if ( signature[1] == "addr16" ) {
                    if ( real_arg1.find_first_not_of( "0123456789abcdef" ) == real_arg1.npos ) {
                        // Is direct value
                        u16 addr = stoi( real_arg1, 0, 16 );
                        hex.push_back( ( addr >> 8 ) & 0xFF );
                        hex.push_back( addr & 0xFF );
                    } else {
                        // Is label
                        label_slots16[real_arg1].push_back( hex.size() );
                        hex.push_back( 0 );
                        hex.push_back( 0 );
                    }
                } else if ( signature[1] == "addr11" ) {
                    if ( real_arg1.find_first_not_of( "0123456789abcdef" ) == real_arg1.npos ) {
                        // Is direct value
                        u16 addr = stoi( real_arg1, 0, 16 );
                        hex.back() = ( hex.back() & 0x1F ) | ( ( ( addr >> 8 ) & 0x07 ) << 5 );
                        hex.push_back( addr & 0xFF );
                    } else {
                        // Is label
                        label_slots16[real_arg1].push_back( hex.size() - 1 );
                        hex.push_back( 0 );
                    }
                } else {
                    size_t i = 1;
                    for ( auto arg : { real_arg1, real_arg2 } ) {
                        if ( signature[i] == "direct" || signature[i] == "bit" || signature[i] == "/bit" ||
                             signature[i] == "#immed" ) {
                            i8 val;
                            if ( rev_sfr_map.find( arg ) != rev_sfr_map.end() ) {
                                // Translate SFR
                                val = rev_sfr_map[arg];
                            } else {
                                if ( arg[0] == '(' )
                                    arg = arg.substr( 1, arg.size() - 2 );
                                val = static_cast<i8>( stoi( arg, 0, 16 ) ); // TODO test values > 127
                            }
                            hex.push_back( *reinterpret_cast<u8 *>( &val ) );
                        } else if ( signature[i] == "offset" ) {
                            if ( real_arg1.find_first_not_of( "0123456789abcdef" ) == real_arg1.npos ) {
                                // Is direct value
                                auto tmp = static_cast<i8>( stoi( arg, 0, 16 ) );
                                hex.push_back( *reinterpret_cast<u8 *>( &tmp ) );
                            } else {
                                // Is label
                                label_slots8[real_arg1].push_back( hex.size() );
                                hex.push_back( op_code_sizes[rev_op_codes[line]] - i ); // Point to the next op code.
                            }
                        }
                        i++;
                    }
                }
            }
        }
    }

    // Fill in label targets.
    for ( auto &pair : label_slots8 ) {
        auto target = label_targets[pair.first];
        for ( auto &pos : pair.second ) {
            // Relative offset
            i16 rel_target = target - pos - hex[pos];
            if ( rel_target > 127 || rel_target < -128 )
                log( "Relative jump offset is too far." );
            auto tmp = static_cast<i8>( rel_target );
            hex[pos] = *reinterpret_cast<u8 *>( &tmp );
        }
    }
    for ( auto &pair : label_slots11 ) {
        auto target = label_targets[pair.first];
        for ( auto &pos : pair.second ) {
            // Relative jump
            if ( ( target & 0xF800 ) != ( ( pos + 2 ) & 0xF800 ) )
                log( "Relative jump is too far (not the same 2Ki-block)." );
            hex[pos] = ( hex[pos] & 0x1F ) | ( ( ( target >> 8 ) & 0x07 ) << 5 );
            hex[pos + 1] = target & 0xFF;
        }
    }
    for ( auto &pair : label_slots16 ) {
        auto target = label_targets[pair.first];
        for ( auto &pos : pair.second ) {
            // Absolute jump
            hex[pos] = ( target >> 8 ) & 0xFF;
            hex[pos + 1] = target & 0xFF;
        }
    }

    encode_hex_file( hex, output );
}

void log( const String &str ) {
    std::cout << str << std::endl;
}
