#include "sim8051/stdafx.hpp"

// The main loop, including some stuff like scrolling
int main() {
    // Initialize all the stuff
    sf::ContextSettings context_settings( 0, 0, 4 );
    sf::RenderWindow window( sf::VideoMode( 1400, 1000, 32 ), "Sim8051", sf::Style::Titlebar, context_settings );
    bool fullscreen = false;
    window.setFramerateLimit( 60 );
    auto view = window.getDefaultView();
    window.setView( view );
    if ( !ImGui::SFML::Init( window ) ) {
        log( "Failed to initialize ImGui" );
        return -1;
    }
    sf::Clock timer;

    // Mouse bug workaround TODO?
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;

    bool running = true;
    sf::Event evt;


    // Simulation stuff


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

        ImGui::EndFrame();

        // Rendering
        window.clear( sf::Color( 0x707070ff ) );



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
