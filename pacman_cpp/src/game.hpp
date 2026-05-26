#pragma once
#include "memory.hpp"
#include "renderer.hpp"

class Game {
public:
    Game();
    ~Game() = default;

    // Inicializa todos los componentes
    bool Initialize();
    
    // Bucle principal de ejecución a velocidad sincronizada
    void Run();

    // Detiene y libera recursos del juego
    void Shutdown();

private:
    PacmanMemory  m_memory;
    PacmanRenderer m_renderer;
    bool           m_is_running;

    // Métodos internos
    void ProcessInput();
    void Update(double delta_time);
    
    // Inicializa la memoria con el mapa de Pac-Man original (muros, pastillas, fantasmas)
    // para demostración interactiva incluso sin un archivo ROM cargado
    void InitializeDefaultMaze();
};
