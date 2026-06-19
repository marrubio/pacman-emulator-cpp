#pragma once
#include "memory.hpp"
#include "renderer.hpp"
#include <array>
#include <string>

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
    uint8_t        m_last_px;
    uint8_t        m_last_py;
    std::array<bool, 4> m_ghost_colliding = {false, false, false, false};

    // Métodos internos
    void ProcessInput();
    void Update(double delta_time);
    void HandleCollision(const std::string& object);
    
    // Inicializa la memoria con el mapa de Pac-Man original (muros, pastillas, fantasmas)
    // para demostración interactiva incluso sin un archivo ROM cargado
    void InitializeDefaultMaze();
};
