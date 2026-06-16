#include "game.hpp"
#include "raylib.h"
#include <iostream>

Game::Game()
    : m_is_running(false)
{
}

bool Game::Initialize() {
    // 1. Inicializar el renderizador gráfico
    if (!m_renderer.Initialize()) {
        return false;
    }

    // 2. Intentar cargar el archivo ROM original de Pac-Man Z80 si existe
    // Si no existe, continuará con el mapa por defecto interactivo
    m_memory.LoadROM("pacman.bin");

    // 3. Rellenar la memoria simulada con el mapa y configuraciones iniciales
    InitializeDefaultMaze();

    m_last_px = m_memory.ReadByte(0x5061);
    m_last_py = m_memory.ReadByte(0x5060);

    m_is_running = true;
    return true;
}

void Game::Run() {
    // Frecuencia de refresco real de Pac-Man arcade: ~60.606 Hz
    const double frame_time = 1.0 / 60.606;
    double accumulated_time = 0.0;
    double last_time = GetTime();

    while (m_is_running && !m_renderer.ShouldClose()) {
        double current_time = GetTime();
        double elapsed = current_time - last_time;
        last_time = current_time;
        accumulated_time += elapsed;

        // 1. Leer entradas de teclado
        ProcessInput();

        // 2. Ejecutar frames acumulados de física a velocidad fija
        while (accumulated_time >= frame_time) {
            Update(frame_time);
            accumulated_time -= frame_time;
        }

        // 3. Renderizar el frame actual
        m_renderer.Render(m_memory);
    }
}

void Game::Shutdown() {
    m_renderer.Shutdown();
    m_is_running = false;
    std::cout << "Juego finalizado de manera correcta." << std::endl;
}

static bool CheckWallCollision(const PacmanMemory& memory, uint8_t px, uint8_t py) {
    uint8_t px_vram = 272 - px;
    uint8_t py_vram = py - 16;

    int box_size = 4;
    int offset = 6;

    int x_start = px_vram + offset;
    int y_start = py_vram + offset;

    const uint8_t* vram = memory.GetVRAMPtr();

    for (int y = y_start; y < y_start + box_size; ++y) {
        for (int x = x_start; x < x_start + box_size; ++x) {
            int tile_x = x / 8;
            int tile_y = y / 8;

            if (tile_x >= 0 && tile_x < 28 && tile_y >= 0 && tile_y < 36) {
                uint8_t tile = vram[tile_y * 28 + tile_x];
                if (tile > 0x00 && tile <= 0x0F) {
                    return true;
                }
            } else {
                return true; // Fuera del laberinto se considera muro
            }
        }
    }
    return false;
}

void Game::ProcessInput() {
    // Obtener la posición actual de Pac-Man desde la memoria física emulada
    // Registros en 5060h y 5061h
    uint8_t py = m_memory.ReadByte(0x5060);
    uint8_t px = m_memory.ReadByte(0x5061);
    uint8_t direction = m_memory.ReadByte(0x5040); // 0=Derecha, 1=Izquierda, 2=Arriba, 3=Abajo

    int speed = 1; // Velocidad de Pac-Man en píxeles por frame

    // Mapear los controles también a los registros de puertos de entrada IN0 (5000h)
    m_memory.SetP1Button(0x01, IsKeyDown(KEY_UP));    // Bit 0: Arriba
    m_memory.SetP1Button(0x02, IsKeyDown(KEY_LEFT));  // Bit 1: Izquierda
    m_memory.SetP1Button(0x04, IsKeyDown(KEY_RIGHT)); // Bit 2: Derecha
    m_memory.SetP1Button(0x08, IsKeyDown(KEY_DOWN));  // Bit 3: Abajo

    uint8_t next_px = px;
    uint8_t next_py = py;

    // Procesar teclado mediante Raylib y escribir en la memoria emulada
    if (IsKeyDown(KEY_RIGHT)) {
        next_px -= speed; // En hardware real de Pac-Man, X decrece hacia la derecha
        direction = 0;
    }
    if (IsKeyDown(KEY_LEFT)) {
        next_px += speed; // X incrementa hacia la izquierda
        direction = 1;
    }
    if (IsKeyDown(KEY_UP)) {
        next_py -= speed; // Y decrece hacia arriba
        direction = 2;
    }
    if (IsKeyDown(KEY_DOWN)) {
        next_py += speed; // Y incrementa hacia abajo
        direction = 3;
    }

    // Mantener dentro de los límites de la pantalla playable usando aritmética de 8 bits
    uint8_t px_vram = 16 - next_px;
    if (py >= 140 && py <= 148) {
        // Túnel de escape
        if (px_vram > 224 && px_vram < 255) {
            next_px = 56; // Reaparece por la derecha
        } else if (px_vram > 216 && px_vram <= 224) {
            next_px = 16; // Reaparece por la izquierda
        }
    } else {
        // Fuera del túnel, limitar a la pantalla visible
        if (px_vram > 216 && px_vram < 240) {
            next_px = 56; // Limitar a la derecha
        } else if (px_vram > 240) {
            next_px = 16; // Limitar a la izquierda
        }
    }

    uint8_t py_vram = next_py - 16;
    if (py_vram < 16) {
        next_py = 32; // Limitar a fila 2
    } else if (py_vram > 242) {
        next_py = 2;  // Limitar a fila 32 (permitiendo bajar 2 píxeles más para el pasillo inferior)
    }

    // Verificar colisiones con muros
    if (next_px != px || next_py != py) {
        if (CheckWallCollision(m_memory, next_px, next_py)) {
            bool moved = false;
            
            // Intentar deslizarse: mover solo en horizontal
            if (next_px != px && !CheckWallCollision(m_memory, next_px, py)) {
                px = next_px;
                moved = true;
            }
            // Intentar deslizarse: mover solo en vertical
            if (next_py != py && !CheckWallCollision(m_memory, px, next_py)) {
                py = next_py;
                moved = true;
            }

            if (!moved) {
                // No se pudo realizar el movimiento. Mostrar traza de colisión.
                uint8_t cx_vram = 272 - next_px;
                uint8_t cy_vram = next_py - 16;
                int crash_tile_x = cx_vram / 8;
                int crash_tile_y = cy_vram / 8;
                static int last_crash_x = -1;
                static int last_crash_y = -1;
                if (crash_tile_x != last_crash_x || crash_tile_y != last_crash_y) {
                    std::cout << "[COLISIÓN] Pac-Man chocó con un muro en la baldosa [" 
                              << crash_tile_x << ", " << crash_tile_y << "]" << std::endl;
                    last_crash_x = crash_tile_x;
                    last_crash_y = crash_tile_y;
                }
            }
        } else {
            px = next_px;
            py = next_py;
        }
    }

    // Escribir la nueva posición de Pac-Man de vuelta a los registros
    m_memory.WriteByte(0x5060, py);
    m_memory.WriteByte(0x5061, px);
    m_memory.WriteByte(0x5040, direction);

    // Escribir en el log si la posición ha cambiado
    if (px != m_last_px || py != m_last_py) {
        uint8_t tx_vram = 272 - px;
        uint8_t ty_vram = py - 16;
        int tile_x = tx_vram / 8;
        int tile_y = ty_vram / 8;
        std::cout << "[POSICIÓN] Pac-Man se movió a: X = " << (int)px << ", Y = " << (int)py 
                  << " (Baldosa: [" << tile_x << ", " << tile_y << "])" << std::endl;
        m_last_px = px;
        m_last_py = py;
    }
}

void Game::Update(double delta_time) {
    (void)delta_time; // Sin usar por ahora en demostración

    // Obtener la posición en píxeles de Pac-Man para realizar detección de colisiones con los puntos
    uint8_t py = m_memory.ReadByte(0x5060);
    uint8_t px = m_memory.ReadByte(0x5061);

    // Convertir la coordenada en píxeles de Pac-Man a coordenadas de baldosa (tile) de 8x8 usando aritmética de 8 bits
    uint8_t tx_vram = 272 - px;
    uint8_t ty_vram = py - 16;
    int tile_x = tx_vram / 8;
    int tile_y = ty_vram / 8;

    if (tile_x >= 0 && tile_x < 28 && tile_y >= 0 && tile_y < 36) {
        uint16_t tile_address = 0x4000 + (tile_y * 28 + tile_x);
        uint8_t tile_content = m_memory.ReadByte(tile_address);

        // Si Pac-Man está sobre una pastilla (tile 0x10) o energizante (tile 0x14)
        if (tile_content == 0x10 || tile_content == 0x14) {
            // Eliminar pastilla escribiendo 0x00 (vacío) en VRAM
            m_memory.WriteByte(tile_address, 0x00);

            // Incrementar puntuación en la RAM (4E80h en adelante es la puntuación del P1 en BCD)
            // Para demostración, incrementamos directamente los dígitos en BCD
            uint8_t score_low = m_memory.ReadByte(0x4E80);
            score_low += 10; // Sumar 10 puntos en BCD
            
            if ((score_low & 0x0F) >= 0x0A) {
                score_low += 6; // Ajuste decimal BCD
            }
            if (score_low >= 0xA0) {
                score_low = 0;
                uint8_t score_mid = m_memory.ReadByte(0x4E81);
                score_mid += 1;
                if ((score_mid & 0x0F) >= 0x0A) score_mid += 6;
                m_memory.WriteByte(0x4E81, score_mid);
            }
            
            m_memory.WriteByte(0x4E80, score_low);
        }
    }
}

void Game::InitializeDefaultMaze() {
    // Mapa clásico de Pac-Man representado en ASCII
    // #: Muro, .: Pastilla pequeña, o: Energizante grande, G: Casa de Fantasmas
    const char* original_maze[36] = {
        "                            ", // Row 0 (HUD Score)
        "                            ", // Row 1
        "############################", // Row 2 (Muro Superior)
        "#............##............#", // Row 3
        "#.####.#####.##.#####.####.#", // Row 4
        "#o####.#####.##.#####.####o#", // Row 5
        "#.####.#####.##.#####.####.#", // Row 6
        "#..........................#", // Row 7
        "#.####.##.########.##.####.#", // Row 8
        "#.####.##.########.##.####.#", // Row 9
        "#......##....##....##......#", // Row 10
        "######.##### ## #####.######", // Row 11
        "     #.##### ## #####.#     ", // Row 12
        "     #.##          ##.#     ", // Row 13
        "     #.## ###--### ##.#     ", // Row 14
        "######.## #      # ##.######", // Row 15
        "      .   # GGGG #   .      ", // Row 16 (Túneles laterales)
        "######.## #      # ##.######", // Row 17
        "     #.## ######## ##.#     ", // Row 18
        "     #.##          ##.#     ", // Row 19
        "     #.## ######## ##.#     ", // Row 20
        "######.## ######## ##.######", // Row 21
        "#............##............#", // Row 22
        "#.####.#####.##.#####.####.#", // Row 23
        "#.####.#####.##.#####.####.#", // Row 24
        "#o..##................##..o#", // Row 25
        "###.##.##.########.##.##.###", // Row 26
        "###.##.##.########.##.##.###", // Row 27
        "#......##....##....##......#", // Row 28
        "#.##########.##.##########.#", // Row 29
        "#.##########.##.##########.#", // Row 30
        "#..........................#", // Row 31
        "############################", // Row 32
        "                            ", // Row 33
        "                            ", // Row 34
        "                            "  // Row 35
    };

    // 1. Cargar el mapa ASCII en la VRAM emulada
    for (int y = 0; y < 36; ++y) {
        for (int x = 0; x < 28; ++x) {
            uint16_t tile_address = 0x4000 + (y * 28 + x);
            uint16_t color_address = 0x4400 + (y * 28 + x);

            char ch = original_maze[y][x];
            uint8_t tile_value = 0x00;
            uint8_t color_attr = 0x07; // Color blanco por defecto

            if (ch == '#') {
                tile_value = 0x01; // Baldosa de muro
                color_attr = 0x07; // Atributo azul para muros
            } else if (ch == '-') {
                tile_value = 0x1B; // Baldosa de puerta
                color_attr = 0x02; // Atributo rosa para la puerta
            } else if (ch == '.') {
                tile_value = 0x10; // Baldosa de pastilla
                color_attr = 0x02; // Atributo color piel
            } else if (ch == 'o') {
                tile_value = 0x14; // Baldosa de energizante
                color_attr = 0x02;
            }

            m_memory.WriteByte(tile_address, tile_value);
            m_memory.WriteByte(color_address, color_attr);
        }
    }

    // 2. Establecer variables de estado iniciales en la RAM
    m_memory.WriteByte(0x4E6F, 3); // 3 vidas iniciales
    m_memory.WriteByte(0x4E80, 0x00); // Puntuación inicial a 0
    m_memory.WriteByte(0x4E81, 0x00);
    m_memory.WriteByte(0x4E88, 0x50); // Puntuación máxima por defecto (5000 puntos en BCD)
    m_memory.WriteByte(0x4E89, 0x00);

    // 3. Inicializar posiciones de los Sprites en los registros emulados
    // Las coordenadas X de sprite siguen la convención del hardware: X decrece hacia la derecha.
    // Estos valores compensan el inset del renderer para centrar cada sprite en su casilla.
    // Sprite 0: Pac-Man (Posición inicial en el laberinto: X = 164, Y = 212)
    m_memory.WriteByte(0x5060, 212); // Posición Y
    m_memory.WriteByte(0x5061, 164); // Posición X
    m_memory.WriteByte(0x5040, 0);   // Dirección inicial: Derecha (0)

    // Sprite 1: Blinky (Fantasma Rojo)
    m_memory.WriteByte(0x5062, 140); // Posición Y
    m_memory.WriteByte(0x5063, 180); // Posición X

    // Sprite 2: Pinky (Fantasma Rosa)
    m_memory.WriteByte(0x5064, 140); // Posición Y
    m_memory.WriteByte(0x5065, 172); // Posición X

    // Sprite 3: Inky (Fantasma Azul)
    m_memory.WriteByte(0x5066, 140); // Posición Y
    m_memory.WriteByte(0x5067, 164); // Posición X

    // Sprite 4: Clyde (Fantasma Naranja)
    m_memory.WriteByte(0x5068, 140); // Posición Y
    m_memory.WriteByte(0x5069, 156); // Posición X
}
