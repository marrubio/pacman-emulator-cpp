#include "game.hpp"
#include <iostream>

int main() {
    std::cout << "Iniciando Pac-Man x86 Native Port..." << std::endl;

    Game game;

    // 1. Inicializar componentes de hardware simulado y motor gráfico
    if (!game.Initialize()) {
        std::cerr << "Error crítico: No se pudo inicializar el juego." << std::endl;
        return -1;
    }

    std::cout << "==================================================" << std::endl;
    std::cout << "¡Juego Inicializado!" << std::endl;
    std::cout << "Controles:" << std::endl;
    std::cout << "  - FLECHAS DE DIRECCIÓN: Mover a Pac-Man" << std::endl;
    std::cout << "  - Comer pastillas incrementa el HUD de puntuación." << std::endl;
    std::cout << "==================================================" << std::endl;

    // 2. Ejecutar el bucle principal
    game.Run();

    // 3. Apagar y liberar recursos
    game.Shutdown();

    return 0;
}
