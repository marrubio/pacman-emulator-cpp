#include "renderer.hpp"
#include "raylib.h"
#include <iostream>
#include <cmath>

namespace {
constexpr int kTopHudHeight = 24;
constexpr int kBottomHudHeight = 24;
constexpr int kFirstVisibleMazeRow = 2;
constexpr int kLastVisibleMazeRow = 32;
constexpr int kTilePixelSize = 8;
constexpr int kVisibleMazeRows = kLastVisibleMazeRow - kFirstVisibleMazeRow + 1;
constexpr int kMazeHeight = kVisibleMazeRows * kTilePixelSize;
constexpr int kHiddenTopMazePixels = kFirstVisibleMazeRow * kTilePixelSize;
}

PacmanRenderer::PacmanRenderer(int scale)
    : m_scale(scale)
    , m_window_width(224 * scale)
    , m_window_height((kTopHudHeight + kMazeHeight + kBottomHudHeight) * scale)
{
}

PacmanRenderer::~PacmanRenderer() {
    Shutdown();
}

bool PacmanRenderer::Initialize() {
    // Configurar calidad y modo de refresco en Raylib
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);
    InitWindow(m_window_width, m_window_height, "Pac-Man x86 Native Port [Raylib]");
    
    if (!IsWindowReady()) {
        std::cerr << "Error: No se pudo inicializar la ventana de Raylib." << std::endl;
        return false;
    }

    SetTargetFPS(60); // Estabilizar a 60 FPS por defecto
    std::cout << "Renderizador inicializado de forma óptima a escala x" << m_scale << std::endl;
    return true;
}

void PacmanRenderer::Shutdown() {
    if (IsWindowReady()) {
        CloseWindow();
    }
}

bool PacmanRenderer::ShouldClose() const {
    return WindowShouldClose();
}

void PacmanRenderer::Render(const PacmanMemory& memory) {
    BeginDrawing();
    ClearBackground(BLACK);

    // 1. Dibujar el mapa de fondo (muros y pastillas desde la VRAM)
    DrawBackgroundGrid(memory);

    // 2. Dibujar personajes activos desde los registros de Sprites (5060h)
    DrawSprites(memory);

    // 3. Dibujar información del HUD (Puntuación, Vidas, Nivel)
    DrawHUD(memory);

    EndDrawing();
}

void PacmanRenderer::DrawBackgroundGrid(const PacmanMemory& memory) {
    const uint8_t* vram = memory.GetVRAMPtr();
    const uint8_t* color_ram = memory.GetColorRAMPtr();
    const int tile_columns = 28;
    const int tile_rows = 36;
    const int vram_stride = 28;
    const int tile_size = 8 * m_scale;
    const int wall_thickness = std::max(1, m_scale * 2);
    const int maze_offset_y = kTopHudHeight * m_scale;

    auto is_wall_tile = [&](int x, int y) -> bool {
        if (x < 0 || x >= tile_columns || y < 0 || y >= tile_rows) {
            return false;
        }

        const uint8_t tile = vram[y * vram_stride + x];
        return tile > 0x00 && tile <= 0x0F;
    };

    // El área de juego es una cuadrícula de 28 columnas x 36 filas de mosaicos de 8x8 píxeles
    for (int y = 0; y < tile_rows; ++y) {
        for (int x = 0; x < tile_columns; ++x) {
            // Mapeo del índice de dirección de la cuadrícula lineal en la VRAM de Pac-Man
            // En el hardware real, la VRAM tiene una rotación de coordenadas.
            // Para simplificar, mapeamos bidimensionalmente:
            uint16_t tile_index = y * vram_stride + x;
            if (tile_index >= 1024) continue;

            uint8_t tile = vram[tile_index];
            uint8_t color_attr = color_ram[tile_index];

            // Coordenadas en píxeles en pantalla
            int screen_x = x * tile_size;
            if (y < kFirstVisibleMazeRow || y > kLastVisibleMazeRow) {
                continue;
            }

            int screen_y = maze_offset_y + (y - kFirstVisibleMazeRow) * tile_size;

            // Obtener color basado en el atributo
            uint8_t r = 0, g = 0, b = 0;
            GetColorFromIndex(color_attr & 0x1F, r, g, b);
            Color tile_color{r, g, b, 255};

            // Dibujar únicamente el contorno visible de los muros para evitar la rejilla
            // interna y recuperar el grosor visual del pasillo del arcade original.
            if (tile > 0x00 && tile <= 0x0F) {
                if (!is_wall_tile(x, y - 1)) {
                    DrawRectangle(screen_x, screen_y, tile_size, wall_thickness, tile_color);
                }
                if (!is_wall_tile(x, y + 1)) {
                    DrawRectangle(screen_x, screen_y + tile_size - wall_thickness, tile_size, wall_thickness, tile_color);
                }
                if (!is_wall_tile(x - 1, y)) {
                    DrawRectangle(screen_x, screen_y, wall_thickness, tile_size, tile_color);
                }
                if (!is_wall_tile(x + 1, y)) {
                    DrawRectangle(screen_x + tile_size - wall_thickness, screen_y, wall_thickness, tile_size, tile_color);
                }
            } 
            // Renderizar pastillas normales (pequeñas) - Mapeadas a la baldosa 0x10
            else if (tile == 0x10) {
                int center_x = screen_x + tile_size / 2;
                int center_y = screen_y + tile_size / 2;
                DrawCircle(center_x, center_y, 1.5f * m_scale, tile_color);
            }
            // Renderizar pastillas energizantes (grandes) - Mapeadas a la baldosa 0x14
            else if (tile == 0x14) {
                // Hacer que parpadee con el tiempo
                if ((int)(GetTime() * 4) % 2 == 0) {
                    int center_x = screen_x + tile_size / 2;
                    int center_y = screen_y + tile_size / 2;
                    DrawCircle(center_x, center_y, 4.0f * m_scale, tile_color);
                }
            }
        }
    }
}

void PacmanRenderer::DrawSprites(const PacmanMemory& memory) {
    const uint8_t* sprite_coords = memory.GetSpriteCoordsPtr();
    const int maze_offset_y = kTopHudHeight * m_scale;

    // El hardware de Pac-Man soporta 8 sprites activos en pantalla
    // Sprite 0: Pac-Man (Amarillo)
    // Sprite 1: Fantasma Rojo (Blinky)
    // Sprite 2: Fantasma Rosa (Pinky)
    // Sprite 3: Fantasma Azul (Inky)
    // Sprite 4: Fantasma Naranja (Clyde)
    // Sprites 5-7: Opcionales (Frutas, Efectos)
    
    for (int i = 7; i >= 0; --i) {
        // Cada sprite tiene 2 bytes de posición en los registros de I/O mapeados
        // Offset 0: Posición Y (invertida en hardware)
        // Offset 1: Posición X
        // Offset 2: Índice de animación
        // Offset 3: Color/paleta
        uint8_t y_reg = sprite_coords[i * 2];
        uint8_t x_reg = sprite_coords[i * 2 + 1];

        // Conversión a coordenadas de pantalla (Pac-Man escala X de derecha a izquierda e Y invertida)
        const int hardware_sprite_size = 16 * m_scale;
        const int size = 8 * m_scale;
        const int sprite_inset = (hardware_sprite_size - size) / 2;
        int screen_x = (272 - x_reg) * m_scale + sprite_inset;
        int screen_y = maze_offset_y + (y_reg - 16 - kHiddenTopMazePixels) * m_scale + sprite_inset;

        // Si la posición está fuera del rango visible, no dibujar
        if (x_reg == 0 || y_reg == 0) continue;

        // Definir color del sprite basado en su índice
        Color sprite_color = WHITE;
        switch (i) {
            case 0: sprite_color = YELLOW; break;       // Pac-Man
            case 1: sprite_color = RED; break;          // Blinky
            case 2: sprite_color = PINK; break;         // Pinky
            case 3: sprite_color = SKYBLUE; break;      // Inky
            case 4: sprite_color = ORANGE; break;       // Clyde
            default: sprite_color = GREEN; break;       // Frutas / Otros
        }

        // 1. Dibujar a Pac-Man (Sprite 0) como un sector circular dinámico (abriendo boca)
        if (i == 0) {
            float mouth_angle = abs(sinf(GetTime() * 12.0f)) * 45.0f; // Animación de masticar
            DrawCircleSector({(float)screen_x + size/2.f, (float)screen_y + size/2.f}, 
                             size/2.f, 
                             mouth_angle, 
                             360.0f - mouth_angle, 
                             20, 
                             sprite_color);
        } 
        // 2. Dibujar a los Fantasmas (Sprites 1 a 4)
        else if (i >= 1 && i <= 4) {
            float cx = (float)screen_x + size/2.f;
            
            // Cuerpo de campana del fantasma
            DrawRectangle(screen_x + 1*m_scale, screen_y + 3*m_scale, 6*m_scale, 4*m_scale, sprite_color);
            DrawCircle(cx, (float)screen_y + 3.f*m_scale, 3.f*m_scale, sprite_color);
            
            // Tres ondas/picos en la parte inferior del cuerpo
            DrawTriangle({(float)screen_x + 1*m_scale, (float)screen_y + 7*m_scale},
                         {(float)screen_x + 2.5f*m_scale, (float)screen_y + 8*m_scale},
                         {(float)screen_x + 4*m_scale, (float)screen_y + 7*m_scale}, sprite_color);
            DrawTriangle({(float)screen_x + 3*m_scale, (float)screen_y + 7*m_scale},
                         {(float)screen_x + 4*m_scale, (float)screen_y + 8*m_scale},
                         {(float)screen_x + 5*m_scale, (float)screen_y + 7*m_scale}, sprite_color);
            DrawTriangle({(float)screen_x + 4*m_scale, (float)screen_y + 7*m_scale},
                         {(float)screen_x + 5.5f*m_scale, (float)screen_y + 8*m_scale},
                         {(float)screen_x + 7*m_scale, (float)screen_y + 7*m_scale}, sprite_color);

            // Dibujar ojos (blancos con pupilas azules) mirando al frente
            DrawCircle(screen_x + 2.5f*m_scale, screen_y + 3*m_scale, 1.2f*m_scale, WHITE);
            DrawCircle(screen_x + 5.5f*m_scale, screen_y + 3*m_scale, 1.2f*m_scale, WHITE);
            DrawCircle(screen_x + 2.5f*m_scale, screen_y + 3*m_scale, 0.6f*m_scale, DARKBLUE);
            DrawCircle(screen_x + 5.5f*m_scale, screen_y + 3*m_scale, 0.6f*m_scale, DARKBLUE);
        }
    }
}

void PacmanRenderer::DrawHUD(const PacmanMemory& memory) {
    // Obtener variables de estado desde los registros emulados
    uint16_t p1_score = memory.GetP1Score();
    uint16_t high_score = memory.GetHighScore();
    uint8_t lives = memory.GetLives();

    int font_size = 8 * m_scale;

    // Pintar textos del HUD clásico
    DrawText("1UP", 24 * m_scale, 8 * m_scale, font_size, RED);
    DrawText(TextFormat("%06d", p1_score), 24 * m_scale, 16 * m_scale, font_size, WHITE);

    DrawText("HIGH SCORE", 72 * m_scale, 8 * m_scale, font_size, RED);
    DrawText(TextFormat("%06d", high_score), 88 * m_scale, 16 * m_scale, font_size, WHITE);

    // Pintar indicador de vidas restantes abajo (máx 5 iconos de Pac-Man)
    int start_y = (kTopHudHeight + kMazeHeight + 12) * m_scale;
    int start_x = 16 * m_scale;
    for (int i = 0; i < lives && i < 5; ++i) {
        DrawCircleSector({(float)start_x + i * 16 * m_scale, (float)start_y}, 
                         6.f * m_scale, 
                         30.0f, 
                         330.0f, 
                         10, 
                         YELLOW);
    }
}

void PacmanRenderer::GetColorFromIndex(uint8_t color_index, uint8_t& r, uint8_t& g, uint8_t& b) const {
    // Mapeador simple de paletas del hardware arcade de Pac-Man
    switch (color_index) {
        case 0:  r = 0;   g = 0;   b = 0;   break; // Negro
        case 1:  r = 255; g = 0;   b = 0;   break; // Rojo
        case 2:  r = 255; g = 183; b = 174; break; // Rosa Pálido
        case 3:  r = 255; g = 255; b = 0;   break; // Amarillo
        case 4:  r = 0;   g = 255; b = 0;   break; // Verde
        case 5:  r = 0;   g = 255; b = 255; break; // Cyan
        case 6:  r = 255; g = 183; b = 81;  break; // Naranja
        case 7:  r = 71;  g = 183; b = 255; break; // Azul Suave
        default: r = 255; g = 255; b = 255; break; // Blanco por defecto
    }
}
