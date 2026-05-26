#pragma once
#include "memory.hpp"
#include <vector>

class PacmanRenderer {
public:
    PacmanRenderer(int scale = 3);
    ~PacmanRenderer();

    // Inicializa la ventana y el contexto gráfico de Raylib
    bool Initialize();
    void Shutdown();

    // Comprueba si se debe cerrar la ventana
    bool ShouldClose() const;

    // Dibuja un fotograma completo en la pantalla basándose en el estado actual de la memoria
    void Render(const PacmanMemory& memory);

private:
    int m_scale;
    int m_window_width;
    int m_window_height;

    // Métodos internos de renderizado detallado
    void DrawBackgroundGrid(const PacmanMemory& memory);
    void DrawSprites(const PacmanMemory& memory);
    void DrawHUD(const PacmanMemory& memory);

    // Mapeo básico de colores del hardware original de Pac-Man
    void GetColorFromIndex(uint8_t color_index, uint8_t& r, uint8_t& g, uint8_t& b) const;
};
