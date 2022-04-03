#include "sim8051/stdafx.hpp"
#include "sim8051/Processor.hpp"
#include "sim8051/Encoding.hpp"

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
            bool should_save = ImGui::Button( "Save" );
            ImGui::SameLine();
            bool should_compile = ImGui::Button( "Compile" );

            should_compile |= ImGui::InputTextMultiline( "##content", &editor_content, ImVec2( -FLT_MIN, -FLT_MIN ),
                                                         ImGuiInputTextFlags_EnterReturnsTrue );
            should_save |= should_compile;

            if ( should_load ) {
                std::ifstream file( editor_asm_filename );
                String tmp;
                editor_content.clear();
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

            if ( should_save ) {
                std::ofstream file( editor_asm_filename );
                file << editor_content;
                file.close();
            }
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
    ImGui::SFML::Shutdown();

    return 0;
}

void log( const String &str ) {
    std::cout << str << std::endl;
}
